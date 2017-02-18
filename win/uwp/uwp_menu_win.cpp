/* NetHack 3.6	uwp_menu_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

void MenuWindow::free_window_info(
    boolean free_data)
{
    if (m_mlist) {
        tty_menu_item *temp;

        while ((temp = m_mlist) != 0) {
            m_mlist = m_mlist->next;
            if (temp->str)
                free((genericptr_t)temp->str);
            free((genericptr_t)temp);
        }
    }
    if (m_plist) {
        free((genericptr_t)m_plist);
        m_plist = 0;
    }

    m_plist_size = 0;
    m_npages = 0;
    m_nitems = 0;
    m_how = 0;

    CoreWindow::free_window_info(free_data);
}

void MenuWindow::Clear()
{
    if (m_active) {
        clear_screen();
    }

    free_window_info(FALSE);

    CoreWindow::Clear();
}

