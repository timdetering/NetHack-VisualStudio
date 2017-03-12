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

    cells_set_dimensions(m_cols, m_rows);
}

MapWindow::~MapWindow()
{
}

void MapWindow::Destroy()
{
    CoreWindow::Destroy();

    clear_window();
}

void MapWindow::Clear()
{
    context.botlx = 1;

    clear_window();
    // TODO(bhouse): original code appears to clear entire display ... so we need to clear all three windows
    //       the code really should have to clear them seperately.
    //       We should see if we can do without clearing the status and message windows.
    g_statusWindow.Clear();
    g_messageWindow.Clear();

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
    } else {
        m_active = 1;
    }

    auto cursor = g_textGrid.GetCursor();
    int offset = 0;
    for(int y = 0; y < m_rows; y++)
        for (int x = 0; x < m_cols; x++)
            g_textGrid.Put(x + m_offx, y + m_offy, m_cells[offset++]);

    g_textGrid.SetCursor(cursor);

}

void MapWindow::Dismiss()
{
    CoreWindow::Dismiss();
}

void MapWindow::Putstr(int attr, const char *str)
{
    TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);

    cells_puts(str, TextColor::NoColor, useAttribute);
    cells_putc('\n', TextColor::NoColor, useAttribute);
}

