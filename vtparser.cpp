/* Copyright (c) 2017-2019 Rob King
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the copyright holder nor the
 *     names of contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS,
 * COPYRIGHT HOLDERS, OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "vtparser.h"
#include <stdbool.h>
#include <string.h>
#include <wchar.h>
#include <vector>
#include <functional>

/**** CONFIGURATION
 * VTPARSER_BAD_CHAR is the character that will be displayed when
 * an application sends an invalid multibyte sequence to the terminal.
 */
#ifndef VTPARSER_BAD_CHAR
#ifdef __STDC_ISO_10646__
#define VTPARSER_BAD_CHAR ((wchar_t)0xfffd)
#else
#define VTPARSER_BAD_CHAR L'?'
#endif
#endif

class ACTION
{
    wchar_t lo = 0;
    wchar_t hi = 0;
    using CB = std::function<void(VtParser *, void *, wchar_t)>;
    CB cb = nullptr;
    struct STATE *next = nullptr;

public:
    ACTION(wchar_t _lo, wchar_t _hi, CB _cb, STATE *_next)
        : lo(_lo), hi(_hi), cb(_cb), next(_next)
    {
    }
    bool process(void *p, wchar_t w, VtParser *vp, STATE **pNext)
    {
        if (w < lo || w > hi)
        {
            // not process
            return false;
        }

        // process
        this->cb(vp, p, w);
        *pNext = this->next;
        return true;
    }
};

struct STATE
{
    bool entry = false;
    std::vector<ACTION> actions;
};

/**** STATE DEFINITIONS
 * This was built by consulting the excellent state chart created by
 * Paul Flo Williams: http://vt100.net/emu/dec_ansi_parser
 * Please note that Williams does not (AFAIK) endorse this work.
 */
class StateMachine
{
    STATE ground;
    STATE escape;
    STATE escape_intermediate;
    STATE csi_entry;
    STATE csi_ignore;
    STATE csi_param;
    STATE csi_intermediate;
    STATE osc_string;

public:
    StateMachine()
    {
        MAKESTATE(ground, false, {{0x20, WCHAR_MAX, VtParser::doprint, NULL}});

        MAKESTATE(escape, true,
                  {{0x21, 0x21, VtParser::ignore, &osc_string},
                   {0x20, 0x2f, VtParser::collect, &escape_intermediate},
                   {0x30, 0x4f, VtParser::doescape, &ground},
                   {0x51, 0x57, VtParser::doescape, &ground},
                   {0x59, 0x59, VtParser::doescape, &ground},
                   {0x5a, 0x5a, VtParser::doescape, &ground},
                   {0x5c, 0x5c, VtParser::doescape, &ground},
                   {0x6b, 0x6b, VtParser::ignore, &osc_string},
                   {0x60, 0x7e, VtParser::doescape, &ground},
                   {0x5b, 0x5b, VtParser::ignore, &csi_entry},
                   {0x5d, 0x5d, VtParser::ignore, &osc_string},
                   {0x5e, 0x5e, VtParser::ignore, &osc_string},
                   {0x50, 0x50, VtParser::ignore, &osc_string},
                   {0x5f, 0x5f, VtParser::ignore, &osc_string}});

        MAKESTATE(escape_intermediate, false,
                  {{0x20, 0x2f, VtParser::collect, NULL},
                   {0x30, 0x7e, VtParser::doescape, &ground}});

        MAKESTATE(csi_entry, true,
                  {{0x20, 0x2f, VtParser::collect, &csi_intermediate},
                   {0x3a, 0x3a, VtParser::ignore, &csi_ignore},
                   {0x30, 0x39, VtParser::param, &csi_param},
                   {0x3b, 0x3b, VtParser::param, &csi_param},
                   {0x3c, 0x3f, VtParser::collect, &csi_param},
                   {0x40, 0x7e, VtParser::docsi, &ground}});

        MAKESTATE(csi_ignore, false,
                  {{0x20, 0x3f, VtParser::ignore, NULL},
                   {0x40, 0x7e, VtParser::ignore, &ground}});

        MAKESTATE(csi_param, false,
                  {{0x30, 0x39, VtParser::param, NULL},
                   {0x3b, 0x3b, VtParser::param, NULL},
                   {0x3a, 0x3a, VtParser::ignore, &csi_ignore},
                   {0x3c, 0x3f, VtParser::ignore, &csi_ignore},
                   {0x20, 0x2f, VtParser::collect, &csi_intermediate},
                   {0x40, 0x7e, VtParser::docsi, &ground}});

        MAKESTATE(csi_intermediate, false,
                  {{0x20, 0x2f, VtParser::collect, NULL},
                   {0x30, 0x3f, VtParser::ignore, &csi_ignore},
                   {0x40, 0x7e, VtParser::docsi, &ground}});

        MAKESTATE(osc_string, true,
                  {{0x07, 0x07, VtParser::doosc, &ground},
                   {0x20, 0x7f, VtParser::collectosc, NULL}});
    }

    STATE *getGround()
    {
        return &ground;
    }

private:
    void MAKESTATE(STATE &state, bool entry, const std::vector<ACTION> actions)
    {
        state.entry = entry;
        state.actions.push_back({0x00, 0x00, VtParser::ignore, NULL});
        state.actions.push_back({0x7f, 0x7f, VtParser::ignore, NULL});
        state.actions.push_back({0x18, 0x18, VtParser::docontrol, &ground});
        state.actions.push_back({0x1a, 0x1a, VtParser::docontrol, &ground});
        state.actions.push_back({0x1b, 0x1b, VtParser::ignore, &escape});
        state.actions.push_back({0x01, 0x06, VtParser::docontrol, NULL});
        state.actions.push_back({0x08, 0x17, VtParser::docontrol, NULL});
        state.actions.push_back({0x19, 0x19, VtParser::docontrol, NULL});
        state.actions.push_back({0x1c, 0x1f, VtParser::docontrol, NULL});

        for (auto &a : actions)
        {
            state.actions.push_back(a);
        }

        state.actions.push_back({0x07, 0x07, VtParser::docontrol, NULL});
    }
};
StateMachine g_stateMachine;

//
// VtParser
//

void VtParser::write(void *p, const char *s, unsigned int n)
{
    wchar_t w = 0;
    while (n)
    {
        size_t r = mbrtowc(&w, s, n, &this->ms);
        switch (r)
        {
        case (size_t)-2: /* incomplete character, try again */
            return;

        case (size_t)-1: /* invalid character, skip it */
            w = VTPARSER_BAD_CHAR;
            r = 1;
            break;

        case 0: /* literal zero, write it but advance */
            r = 1;
            break;
        }

        n -= r;
        s += r;
        handlechar(p, w);
    }
}

void VtParser::handlechar(void *p, wchar_t w)
{
    if (!this->s)
    {
        this->s = g_stateMachine.getGround();
    }
    for (auto &a : this->s->actions)
    {
        STATE *pNext;
        if (a.process(p, w, this, &pNext))
        {
            if (pNext)
            {
                this->s = pNext;
                if (pNext->entry)
                {
                    this->reset();
                }
            }

            return;
        }
    }
}

void VtParser::reset()
{
    this->inter = this->narg = this->nosc = 0;
    memset(this->args, 0, sizeof(this->args));
    memset(this->oscbuf, 0, sizeof(this->oscbuf));
}

/**** ACTION FUNCTIONS */
void VtParser::ignore(VtParser *v, void *p, wchar_t w)
{
    (void)v;
    (void)w; /* avoid warnings */
}
void VtParser::collect(VtParser *v, void *p, wchar_t w)
{
    v->inter = v->inter ? v->inter : (int)w;
}

void VtParser::collectosc(VtParser *v, void *p, wchar_t w)
{
    if (v->nosc < MAXOSC)
        v->oscbuf[v->nosc++] = w;
}

void VtParser::param(VtParser *v, void *p, wchar_t w)
{
    v->narg = v->narg ? v->narg : 1;

    if (w == L';')
        v->args[v->narg++] = 0;
    else if (v->narg < MAXPARAM && v->args[v->narg - 1] < 9999)
        v->args[v->narg - 1] = v->args[v->narg - 1] * 10 + (w - 0x30);
}

void VtParser::docontrol(VtParser *v, void *p, wchar_t w)
{
    if (w < MAXCALLBACK && v->m_controls[w])
        v->m_controls[w]({p, w, v->inter, 0, NULL, (const wchar_t *)v->oscbuf});
}
void VtParser::doescape(VtParser *v, void *p, wchar_t w)
{
    if (w < MAXCALLBACK && v->m_escapes[w])
        v->m_escapes[w]({p, w, v->inter, v->inter > 0, &v->inter,
                         (const wchar_t *)v->oscbuf});
}
void VtParser::docsi(VtParser *v, void *p, wchar_t w)
{
    if (w < MAXCALLBACK && v->m_csis[w])
        v->m_csis[w]({p, w, v->inter, v->narg, v->args,
                     (const wchar_t *)v->oscbuf});
}
void VtParser::doprint(VtParser *v, void *p, wchar_t w)
{
    if (v->m_print)
        v->m_print({p, w, v->inter, 0, NULL, (const wchar_t *)v->oscbuf});
}
void VtParser::doosc(VtParser *v, void *p, wchar_t w)
{
    if (v->m_osc)
        v->m_osc({p, w, v->inter, v->nosc, NULL, (const wchar_t *)v->oscbuf});
}
