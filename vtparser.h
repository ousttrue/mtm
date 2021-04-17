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
#pragma once
#include "curses_term.h"
#include <wchar.h>
#include "vtcontext.h"

using VTCALLBACK = void (*)(VtContext context);

/**** DATA TYPES */
#define MAXPARAM 16
#define MAXOSC 100
#define MAXBUF 100
#define MAXACTIONS 128
#define MAXCALLBACK 128

class VtParser
{
    struct STATE *s = nullptr;
    int narg = 0;
    int nosc = 0;
    int args[MAXPARAM] = {};
    int inter = 0;
    int oscbuf[MAXOSC + 1] = {};
    mbstate_t ms = {};
    VTCALLBACK m_print = nullptr;
    VTCALLBACK m_osc = nullptr;
    VTCALLBACK m_controls[MAXCALLBACK] = {};
    VTCALLBACK m_escapes[MAXCALLBACK] = {};
    VTCALLBACK m_csis[MAXCALLBACK] = {};

public:
    // void onevent(VtEvent t, wchar_t w, VTCALLBACK cb);
    void setPrint(VTCALLBACK cb)
    {
        m_print = cb;
    }
    void setOsc(VTCALLBACK cb)
    {
        m_osc = cb;
    }
    void setControl(wchar_t w, VTCALLBACK cb)
    {
        m_controls[w] = cb;
    }
    void setEscape(wchar_t w, VTCALLBACK cb)
    {
        m_escapes[w] = cb;
    }
    void setCsi(wchar_t w, VTCALLBACK cb)
    {
        m_csis[w] = cb;
    }

    void write(void *p, const char *s, unsigned int n);

private:
    void handlechar(void *p, wchar_t w);
    void reset();

public:
    /**** ACTION FUNCTIONS */
    static void ignore(VtParser *v, void *p, wchar_t w);
    static void collect(VtParser *v, void *p, wchar_t w);
    static void collectosc(VtParser *v, void *p, wchar_t w);
    static void param(VtParser *v, void *p, wchar_t w);
    static void docontrol(VtParser *v, void *p, wchar_t w);
    static void doescape(VtParser *v, void *p, wchar_t w);
    static void docsi(VtParser *v, void *p, wchar_t w);
    static void doprint(VtParser *v, void *p, wchar_t w);
    static void doosc(VtParser *v, void *p, wchar_t w);
};
