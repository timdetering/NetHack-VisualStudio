/* NetHack 3.6	uwp_map_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

MapWindow g_mapWindow;

MapWindow::MapWindow() : CoreWindow(NHW_MAP, MAP_WINDOW)
{
    // core
    /* map window, ROWNO lines long, full width, below message window */
    m_offx = 0;
    m_offy = 1;
    m_rows = ROWNO;
    m_cols = COLNO;
}

MapWindow::~MapWindow()
{
}

void MapWindow::Destroy()
{
    CoreWindow::Destroy();

    clear_screen();
}

void MapWindow::Clear()
{
    context.botlx = 1;
    clear_screen();

    CoreWindow::Clear();
}

void MapWindow::Display(bool blocking)
{
    g_rawprint = 0;

    if (blocking) {
        /* blocking map (i.e. ask user to acknowledge it as seen) */
        if (!g_messageWindow.m_mustBeSeen)
        {
            g_messageWindow.m_mustBeSeen = true;
            g_messageWindow.m_mustBeErased = true;
        }

        g_messageWindow.Display(true);
        return;
    }

    m_active = 1;
}

void MapWindow::Dismiss()
{
    CoreWindow::Dismiss();
}

void MapWindow::Putstr(int attr, const char *str)
{
    assert(0);  // not expected to get called

    str = compress_str(str);
    TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);

    core_puts(str, TextColor::NoColor, useAttribute);
    m_curx = 0;
    m_cury++;
}

