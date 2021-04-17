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

/**** DATA TYPES */
#define MAXPARAM 16
#define MAXOSC 100
#define MAXBUF 100
#define MAXACTIONS 128

using CB = void (*)(VTPARSERImpl *p, wchar_t w);

struct ACTION
{
    wchar_t lo = 0;
    wchar_t hi = 0;
    CB cb = nullptr;
    struct STATE *next = nullptr;

    bool process(wchar_t w, VTPARSERImpl *vp);
};

using ENTRY = void (*)(VTPARSERImpl *v);

struct STATE
{
    bool entry = false;
    std::vector<ACTION> actions;
};

/**** GLOBALS */
STATE ground;
STATE escape;
STATE escape_intermediate;
STATE csi_entry;
STATE csi_ignore;
STATE csi_param;
STATE csi_intermediate;
STATE osc_string;

struct VTPARSERImpl
{
#define MAXCALLBACK 128

    STATE *s = nullptr;
    int narg = 0;
    int nosc = 0;
    int args[MAXPARAM] = {};
    int inter = 0;
    int oscbuf[MAXOSC + 1] = {};
    mbstate_t ms = {};
    void *p = nullptr;
    VTCALLBACK print = nullptr;
    VTCALLBACK osc = nullptr;
    VTCALLBACK cons[MAXCALLBACK] = {};
    VTCALLBACK escs[MAXCALLBACK] = {};
    VTCALLBACK csis[MAXCALLBACK] = {};

    void vtonevent(VtEvent t, wchar_t w, VTCALLBACK cb)
    {
        if (w >= MAXCALLBACK)
        {
            throw std::exception();
        }

        switch (t)
        {
        case VtEvent::CONTROL:
            this->cons[w] = cb;
            break;
        case VtEvent::ESCAPE:
            this->escs[w] = cb;
            break;
        case VtEvent::CSI:
            this->csis[w] = cb;
            break;
        case VtEvent::PRINT:
            this->print = cb;
            break;
        case VtEvent::OSC:
            this->osc = cb;
            break;
        }
    }

    void vtwrite(const char *s, unsigned int n)
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
            handlechar(w);
        }
    }

    void handlechar(wchar_t w)
    {
        if (!this->s)
        {
            this->s = &ground;
        }
        for (auto &a : this->s->actions)
        {
            if (a.process(w, this))
            {
                return;
            }
        }
    }

    void reset()
    {
        this->inter = this->narg = this->nosc = 0;
        memset(this->args, 0, sizeof(this->args));
        memset(this->oscbuf, 0, sizeof(this->oscbuf));
    }
};

bool ACTION::process(wchar_t w, VTPARSERImpl *vp)
{
    if (w >= this->lo && w <= this->hi)
    {
        this->cb(vp, w);
        if (this->next)
        {
            vp->s = this->next;
            if (this->next->entry)
            {
                vp->reset();
            }
        }
        return true;
    }

    return false;
}

/**** ACTION FUNCTIONS */

static void ignore(VTPARSERImpl *v, wchar_t w)
{
    (void)v;
    (void)w; /* avoid warnings */
}

static void collect(VTPARSERImpl *v, wchar_t w)
{
    v->inter = v->inter ? v->inter : (int)w;
}

static void collectosc(VTPARSERImpl *v, wchar_t w)
{
    if (v->nosc < MAXOSC)
        v->oscbuf[v->nosc++] = w;
}

static void param(VTPARSERImpl *v, wchar_t w)
{
    v->narg = v->narg ? v->narg : 1;

    if (w == L';')
        v->args[v->narg++] = 0;
    else if (v->narg < MAXPARAM && v->args[v->narg - 1] < 9999)
        v->args[v->narg - 1] = v->args[v->narg - 1] * 10 + (w - 0x30);
}

#define DO(k, t, f, n, a)                                                      \
    static void do##k(VTPARSERImpl *v, wchar_t w)                              \
    {                                                                          \
        if (t)                                                                 \
            f(v->p, w, v->inter, n, a, (const wchar_t *)v->oscbuf);            \
    }

DO(control, w < MAXCALLBACK && v->cons[w], v->cons[w], 0, NULL)
DO(escape, w<MAXCALLBACK && v->escs[w], v->escs[w], v->inter> 0, &v->inter)
DO(csi, w < MAXCALLBACK && v->csis[w], v->csis[w], v->narg, v->args)
DO(print, v->print, v->print, 0, NULL)
DO(osc, v->osc, v->osc, v->nosc, NULL)

/**** STATE DEFINITIONS
 * This was built by consulting the excellent state chart created by
 * Paul Flo Williams: http://vt100.net/emu/dec_ansi_parser
 * Please note that Williams does not (AFAIK) endorse this work.
 */

void MAKESTATE(STATE &state, bool entry, const std::vector<ACTION> actions)
{
    state.entry = entry;
    state.actions.push_back({0x00, 0x00, ignore, NULL});
    state.actions.push_back({0x7f, 0x7f, ignore, NULL});
    state.actions.push_back({0x18, 0x18, docontrol, &ground});
    state.actions.push_back({0x1a, 0x1a, docontrol, &ground});
    state.actions.push_back({0x1b, 0x1b, ignore, &escape});
    state.actions.push_back({0x01, 0x06, docontrol, NULL});
    state.actions.push_back({0x08, 0x17, docontrol, NULL});
    state.actions.push_back({0x19, 0x19, docontrol, NULL});
    state.actions.push_back({0x1c, 0x1f, docontrol, NULL});

    for (auto &a : actions)
    {
        state.actions.push_back(a);
    }

    state.actions.push_back({0x07, 0x07, docontrol, NULL});
    state.actions.push_back({0x00, 0x00, NULL, NULL});
}

void Initialize()
{
    MAKESTATE(ground, false, {{0x20, WCHAR_MAX, doprint, NULL}});

    MAKESTATE(escape, true,
              {{0x21, 0x21, ignore, &osc_string},
               {0x20, 0x2f, collect, &escape_intermediate},
               {0x30, 0x4f, doescape, &ground},
               {0x51, 0x57, doescape, &ground},
               {0x59, 0x59, doescape, &ground},
               {0x5a, 0x5a, doescape, &ground},
               {0x5c, 0x5c, doescape, &ground},
               {0x6b, 0x6b, ignore, &osc_string},
               {0x60, 0x7e, doescape, &ground},
               {0x5b, 0x5b, ignore, &csi_entry},
               {0x5d, 0x5d, ignore, &osc_string},
               {0x5e, 0x5e, ignore, &osc_string},
               {0x50, 0x50, ignore, &osc_string},
               {0x5f, 0x5f, ignore, &osc_string}});

    MAKESTATE(escape_intermediate, false,
              {{0x20, 0x2f, collect, NULL}, {0x30, 0x7e, doescape, &ground}});

    MAKESTATE(csi_entry, true,
              {{0x20, 0x2f, collect, &csi_intermediate},
               {0x3a, 0x3a, ignore, &csi_ignore},
               {0x30, 0x39, param, &csi_param},
               {0x3b, 0x3b, param, &csi_param},
               {0x3c, 0x3f, collect, &csi_param},
               {0x40, 0x7e, docsi, &ground}});

    MAKESTATE(csi_ignore, false,
              {{0x20, 0x3f, ignore, NULL}, {0x40, 0x7e, ignore, &ground}});

    MAKESTATE(csi_param, false,
              {{0x30, 0x39, param, NULL},
               {0x3b, 0x3b, param, NULL},
               {0x3a, 0x3a, ignore, &csi_ignore},
               {0x3c, 0x3f, ignore, &csi_ignore},
               {0x20, 0x2f, collect, &csi_intermediate},
               {0x40, 0x7e, docsi, &ground}});

    MAKESTATE(csi_intermediate, false,
              {{0x20, 0x2f, collect, NULL},
               {0x30, 0x3f, ignore, &csi_ignore},
               {0x40, 0x7e, docsi, &ground}});

    MAKESTATE(osc_string, true,
              {{0x07, 0x07, doosc, &ground}, {0x20, 0x7f, collectosc, NULL}});
}

///
/// VtParser
///
VtParser::VtParser(void *p)
{
    static bool s_initialized = false;
    if (!s_initialized)
    {
        Initialize();
        s_initialized = true;
    }
    m_impl = new VTPARSERImpl;
    m_impl->p = p;
}

VtParser::~VtParser()
{
    delete m_impl;
}

void VtParser::vtonevent(VtEvent t, wchar_t w, VTCALLBACK cb)
{
    m_impl->vtonevent(t, w, cb);
}

void VtParser::vtwrite(const char *s, unsigned int n)
{
    m_impl->vtwrite(s, n);
}
