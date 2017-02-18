/* NetHack 3.6	uwp_message_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

void MessageWindow::free_window_info(
    boolean free_data)
{
    m_msgList.clear();
    m_msgIter = m_msgList.end();

    CoreWindow::free_window_info(free_data);
}
