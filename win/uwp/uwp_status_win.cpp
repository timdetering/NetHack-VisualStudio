/* NetHack 3.6	uwp_text_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

StatusWindow::StatusWindow() : CoreWindow(NHW_STATUS)
{
    // core
    /* status window, 2 lines long, full width, bottom of screen */
    m_offx = 0;
    m_offy = min(g_textGrid.GetDimensions().m_y - kStatusHeight, ROWNO + 1);
    m_rows = kStatusHeight;
    m_cols = kStatusWidth;

}

StatusWindow::~StatusWindow()
{
}

void StatusWindow::Clear()
{
    tty_curs(m_window, 1, 0);
    cl_end();
    tty_curs(m_window, 1, 1);
    cl_end();

    CoreWindow::Clear();
}

void StatusWindow::Display(bool blocking)
{
    m_active = 1;
}

void StatusWindow::Dismiss()
{
    CoreWindow::Dismiss();
}

void StatusWindow::Putstr(int attr, const char *str)
{
    /* TODO(bhouse) can this window type get cancelled? */
    if (m_flags & WIN_CANCELLED)
        return;

    str = compress_str(str);

    char *ob;
    const char *nb;
    long i, j, n0;

    ob = &m_statusLines[m_cury][j = m_curx];
    if (context.botlx)
        *ob = 0;
    if (!m_cury && (int)strlen(str) >= CO) {
        /* the characters before "St:" are unnecessary */
        nb = index(str, ':');
        if (nb && nb > str + 2)
            str = nb - 2;
    }
    nb = str;
    for (i = m_curx + 1, n0 = m_cols; i < n0; i++, nb++) {
        if (!*nb) {
            if (*ob || context.botlx) {
                /* last char printed may be in middle of line */
                tty_curs(WIN_STATUS, i, m_cury);
                cl_end();
            }
            break;
        }
        if (*ob != *nb)
            tty_putsym(WIN_STATUS, i, m_cury, *nb);
        if (*ob)
            ob++;
    }

    (void)strncpy(&m_statusLines[m_cury][j], str, m_cols - j - 1);
    m_statusLines[m_cury][m_cols - 1] = '\0'; /* null terminate */
#ifdef STATUS_VIA_WINDOWPORT
    if (!iflags.use_status_hilites) {
#endif
        m_cury = (m_cury + 1) % 2;
        m_curx = 0;
#ifdef STATUS_VIA_WINDOWPORT
    }
#endif
}
