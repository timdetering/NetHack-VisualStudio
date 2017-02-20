/* NetHack 3.6	uwp_text_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

TextWindow::TextWindow(winid window) : CoreWindow(NHW_TEXT, window)
{
    m_offx = 0;
    m_offy = 0;
    m_rows = kScreenHeight;
    m_cols = kScreenWidth;
    m_cancelled = false;
}

TextWindow::~TextWindow()
{
}

void TextWindow::Clear()
{
    if (m_active) {
        clear_screen();
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
    m_cancelled = false;
}

void TextWindow::Putstr(int attr, const char *str)
{
    /* TODO(bhouse) can this window type get cancelled? */
    if (m_cancelled)
        return;

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
        TextAttribute textAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);
        m_lines.push_back(std::pair<TextAttribute, std::string>(textAttribute, line));

    } while (input.size() > 0);
}

void TextWindow::Display(bool blocking)
{
    if (m_cancelled)
        return;

    g_rawprint = 0;

    assert(m_offx == 0);

    MessageWindow * msgWin = GetMessageWindow();

    if (msgWin != NULL && msgWin->m_mustBeSeen)
        tty_display_nhwindow(WIN_MESSAGE, TRUE);

    if (msgWin != NULL)
        tty_clear_nhwindow(WIN_MESSAGE);

    assert(m_lines.size() > 0);

    auto iter = m_lines.begin();
    int row = 0;

    tty_curs(m_window, 1, 0);
    cl_eos();

    while (iter != m_lines.end()) {
        auto line = *iter++;

        tty_curs(m_window, 1, row++);
        win_puts(m_window, line.second.c_str(), TextColor::NoColor, line.first);

        if (row == (m_rows - 1) || iter == m_lines.end()) {
            tty_curs(m_window, 1, row);
            cl_eos();
            tty_curs(m_window, 1, m_rows - 1);
            dmore(quitchars);
            if (morc == '\033') {
                m_cancelled = true;
                break;
            }

            tty_curs(m_window, 1, 0);
            cl_eos();
            row = 0;
        }
    }

    m_active = 1;
}
