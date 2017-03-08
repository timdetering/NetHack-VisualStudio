/* NetHack 3.6	uwp_base_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

BaseWindow g_baseWindow;

BaseWindow::BaseWindow() : CoreWindow(NHW_BASE, BASE_WINDOW)
{
    // core
    m_offx = 0;
    m_offy = 0;
    m_rows = kScreenHeight;
    m_cols = kScreenWidth;

    cells_set_dimensions(m_cols, m_rows);
}

BaseWindow::~BaseWindow()
{
}

void BaseWindow::Clear()
{
    clear_whole_screen();

    CoreWindow::Clear();
}

void BaseWindow::Display(bool blocking)
{
    g_rawprint = 0;

    m_active = 1;
}

void BaseWindow::Dismiss()
{
    CoreWindow::Dismiss();
}

void BaseWindow::Putstr(int attr, const char *str)
{
    str = compress_str(str);
    TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);

    set_cursor(m_curx, m_cury);
    core_puts(str, TextColor::NoColor, useAttribute);
    m_curx = 0;
    m_cury++;
}

void CoreWindow::bail(const char *mesg)
{
    clearlocks();
    tty_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}
