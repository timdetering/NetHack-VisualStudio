/* NetHack 3.6	uwp_text_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

CoreWindow::CoreWindow(int inType) : m_type(inType)
{
    m_flags = 0;
    m_active = FALSE;
    m_curx = 0;
    m_cury = 0;
    m_morestr = 0;
}

CoreWindow::~CoreWindow()
{
}

void CoreWindow::Destroy()
{
    if (m_active)
        tty_dismiss_nhwindow(m_window);

    if (m_type == NHW_MESSAGE)
        iflags.window_inited = 0;

    if (m_type == NHW_MAP)
        clear_screen();

    free_window_info(TRUE);
}

void CoreWindow::Curs(int x, int y)
{

    m_curx = --x; /* column 0 is never used */
    m_cury = y;

    assert(x >= 0 && x <= m_cols);
    assert(y >= 0 && y < m_rows);

    x += m_offx;
    y += m_offy;

    g_textGrid.SetCursor(Int2D(x, y));
}

void CoreWindow::Dismiss()
{
    /*
    * these should only get dismissed when the game is going away
    * or suspending
    */
    tty_curs(BASE_WINDOW, 1, (int)g_uwpDisplay->rows - 1);
    m_active = 0;
    m_flags = 0;
}

void CoreWindow::Clear()
{
    m_curx = 0;
    m_cury = 0;
}

void CoreWindow::free_window_info(
    boolean free_data)
{
    if (m_morestr) {
        free((genericptr_t)m_morestr);
        m_morestr = 0;
    }
}
