/* NetHack 3.6	uwp_base_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

void BaseWindow::Clear()
{
    clear_screen();

    CoreWindow::Clear();
}

void BaseWindow::Display(bool blocking)
{
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

    tty_curs(m_window, m_curx + 1, m_cury);
    win_puts(m_window, str, TextColor::NoColor, useAttribute);
    m_curx = 0;
    m_cury++;
}

