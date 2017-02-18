/* NetHack 3.6	uwp_text_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

TextWindow::TextWindow() : CoreWindow(NHW_TEXT)
{
    // core
    /* inventory/menu window, variable length, full width, top of screen
    */
    /* help window, the same, different semantics for display, etc */
    m_offx = 0;
    m_offy = 0;
    m_rows = 0;
    m_cols = g_uwpDisplay->cols;
}

TextWindow::~TextWindow()
{
}

void TextWindow::Clear()
{
    if (m_active) {
        clear_screen();
    }

    /* can m_morestr even get set?  should switch to std::string */
    if (m_morestr) {
        free((genericptr_t) m_morestr);
        m_morestr = NULL;
    }

    CoreWindow::Clear();
}

void TextWindow::Dismiss()
{
    if (m_active) {
        if (iflags.window_inited) {
            /* otherwise dismissing the text endwin after other windows
            * are dismissed tries to redraw the map and panics.  since
            * the whole reason for dismissing the other windows was to
            * leave the ending window on the screen, we don't want to
            * erase it anyway.
            */
            docrt();

        }
        m_active = 0;
    }
    m_flags = 0;
}

void TextWindow::Putstr(int attr, const char *str)
{
    std::string input = std::string(compress_str(str));

    do {
        std::string line;

        if (input.size() > CO) {
            int split = input.find_last_of(" \n", CO - 1);
            if (split == std::string::npos || split == 0)
                split = CO - 1;

            line = input.substr(0, split);
            input = input.substr(split + 1);
        } else {
            line = input;
            input.clear();
        }

        m_lines.push_back(std::pair<int, std::string>(attr, line));

        /* TODO(bhouse) do we need either of these to be set really? */
        m_cury = m_lines.size();
        m_rows = m_lines.size();

    } while (input.size() > 0);
}

void TextWindow::Display(bool blocking)
{
    m_cols = g_uwpDisplay->cols; /* force full-screen mode */
    m_active = 1;
    m_offx = 0;

    MessageWindow * msgWin = GetMessageWindow();

    if (msgWin != NULL && msgWin->m_mustBeSeen)
        tty_display_nhwindow(WIN_MESSAGE, TRUE);

    if (msgWin != NULL)
        tty_clear_nhwindow(WIN_MESSAGE);

    /* this is nearly identcial to MenuWindow::process_lines except for where
    the more is placed and how we clear */
    assert(m_lines.size() > 0);

    auto iter = m_lines.begin();
    m_rows = g_textGrid.GetDimensions().m_y - m_offy;
    int row = 0;

    while (iter != m_lines.end()) {
        auto line = *iter++;

        tty_curs(m_window, 1, row++);
        cl_end();

        if (line.second.size() > 0) {
            const char * str = line.second.c_str();
            int attr = line.first;
            if (m_offx) {
                win_putc(m_window, ' ');
            }
            TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);
            const char *cp;

            for (cp = str;
                *cp && g_textGrid.GetCursor().m_x < g_textGrid.GetDimensions().m_x;
                cp++)
                win_putc(m_window, *cp, TextColor::NoColor, useAttribute);
        }

        if (row == (m_rows - 1) || iter == m_lines.end()) {
            tty_curs(m_window, 1, row);
            cl_eos();
            tty_curs(m_window, 1, m_rows - 1);
            dmore(this, quitchars);
            if (morc == '\033') {
                m_flags |= WIN_CANCELLED;
                break;
            }

            tty_curs(m_window, 1, 0);
            cl_eos();
            row = 0;
        }
    }

    m_active = 1;
}
