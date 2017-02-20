/* NetHack 3.6	uwp_text_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

CoreWindow::CoreWindow(int inType, winid window) : m_type(inType), m_window(window)
{
    m_flags = 0;
    m_active = FALSE;
    m_curx = 0;
    m_cury = 0;

    g_wins[m_window] = this;
}

CoreWindow::~CoreWindow()
{
    g_wins[m_window] = NULL;
}


void CoreWindow::Destroy()
{
    if (m_active)
        tty_dismiss_nhwindow(m_window);

    if (m_type == NHW_MESSAGE)
        iflags.window_inited = 0;

    if (m_type == NHW_MAP)
        clear_screen();

    free_window_info();
}

void CoreWindow::free_window_info()
{
    // do nothing
}

void CoreWindow::set_cursor(int x, int y)
{
    m_curx = x;
    m_cury = y;

    assert(x >= 0 && x < m_cols);
    assert(y >= 0 && y < m_rows);

    x += m_offx;
    y += m_offy;

    g_textGrid.SetCursor(Int2D(x, y));
}

void CoreWindow::Dismiss()
{
    m_active = 0;
    m_flags = 0;
}

void CoreWindow::Clear()
{
    m_curx = 0;
    m_cury = 0;
}

void
CoreWindow::xwaitforspace(
    register const char *s) /* chars allowed besides return */
{
    register int c, x = g_dismiss_more;

    morc = 0;
    while (
#ifdef HANGUPHANDLING
        !program_state.done_hup &&
#endif
        (c = tty_nhgetch()) != EOF) {

        if (c == '\n')
            break;

        if (c == '\033') {
            g_dismiss_more = 1;
            morc = '\033';
            break;
        }

        if ((s && index(s, c)) || c == x) {
            morc = (char)c;
            break;
        }

        tty_nhbell();
    }
}

void CoreWindow::core_putc(char ch, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute)
{
    int x = m_curx + m_offx;
    int y = m_cury + m_offy;

    g_textGrid.Put(x, y, ch, textColor, textAttribute);

    m_curx = g_textGrid.GetCursor().m_x - m_offx;
    m_cury = g_textGrid.GetCursor().m_y - m_offy;
}

void CoreWindow::core_puts(
    const char *s,
    TextColor textColor,
    TextAttribute textAttribute)
{
    int x = m_curx + m_offx;
    int y = m_cury + m_offy;

    g_textGrid.Putstr(textColor, textAttribute, s);

    m_curx = g_textGrid.GetCursor().m_x - m_offx;
    m_cury = g_textGrid.GetCursor().m_y - m_offy;
}

const char *
CoreWindow::compress_str(const char * str)
{
    static char cbuf[BUFSZ];

    /* compress out consecutive spaces if line is too long;
    topline wrapping converts space at wrap point into newline,
    we reverse that here */
    if ((int)strlen(str) >= CO || index(str, '\n')) {
        const char *in_str = str;
        char c, *outstr = cbuf, *outend = &cbuf[sizeof cbuf - 1];
        boolean was_space = TRUE; /* True discards all leading spaces;
                                  False would retain one if present */

        while ((c = *in_str++) != '\0' && outstr < outend) {
            if (c == '\n')
                c = ' ';
            if (was_space && c == ' ')
                continue;
            *outstr++ = c;
            was_space = (c == ' ');
        }
        if ((was_space && outstr > cbuf) || outstr == outend)
            --outstr; /* remove trailing space or make room for terminator */
        *outstr = '\0';
        str = cbuf;
    }
    return str;
}

void CoreWindow::Putsym(int x, int y, char ch)
{

    switch (m_type) {
    case NHW_STATUS:
    case NHW_MAP:
    case NHW_BASE:
        set_cursor(x - 1, y);
        core_putc(ch);
        break;
    case NHW_MESSAGE:
    case NHW_MENU:
    case NHW_TEXT:
        impossible("Can't putsym to window type %d", m_type);
        break;
    }
}
