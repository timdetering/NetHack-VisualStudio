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
        assert(iflags.window_inited);
        docrt();
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

    if (g_messageWindow.m_mustBeSeen)
        g_messageWindow.Display(true);

    g_messageWindow.Clear();

    assert(m_lines.size() > 0);

    auto iter = m_lines.begin();
    int row = 0;

    set_cursor(0, 0);
    clear_to_end_of_screen();

    while (iter != m_lines.end()) {
        auto & line = *iter++;

        set_cursor(0, row++);
        core_puts(line.second.c_str(), TextColor::NoColor, line.first);

        if (row == (m_rows - 1) || iter == m_lines.end()) {
            set_cursor(0, row);
            clear_to_end_of_screen();
            set_cursor(0, m_rows - 1);
            int response = dmore(quitchars);
            if (response == ESCAPE) {
                m_cancelled = true;
                break;
            }

            set_cursor(0, 0);
            clear_to_end_of_screen();
            row = 0;
        }
    }

    m_active = 1;
}

int TextWindow::dmore(
    const char *s) /* valid responses */
{
    const char *prompt = m_morestr.size() ? m_morestr.c_str() : defmorestr;

    set_cursor(m_curx, m_cury);
    core_puts(prompt, TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);

    return wait_for_response(s);
}
