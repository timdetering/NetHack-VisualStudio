/* NetHack 3.6	uwpglobals.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#include "uwpglobals.h"

namespace Nethack
{

    EventQueue      g_eventQueue;
    TextGrid        g_textGrid(Int2D(80, 25));
    jmp_buf         g_mainLoopJmpBuf;
    Options         g_options;
    FontCollection  g_fontCollection;

}
