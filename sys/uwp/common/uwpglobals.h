/* NetHack 3.6	uwpglobals.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include <setjmp.h>
#include "uwpeventqueue.h"
#include "TextGrid.h"

namespace Nethack
{
    extern EventQueue  g_eventQueue;
    extern TextGrid    g_textGrid;
    extern jmp_buf     g_mainLoopJmpBuf;
    
}

