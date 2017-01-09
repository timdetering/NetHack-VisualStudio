/* NetHack 3.6	uwp.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#pragma once

#define ESCAPE 27

#include "common\uwpeventqueue.h"

extern"C" {

    extern char MapScanCode(const Nethack::Event & e);
    extern int raw_getchar();

}
