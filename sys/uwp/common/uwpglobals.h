/* NetHack 3.6	uwpglobals.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include <setjmp.h>

#ifdef __cplusplus
#include <string>

#include "uwpeventqueue.h"
#include "TextGrid.h"
#include "uwpoption.h"
#include "..\uwpfont.h"

namespace Nethack
{
    extern EventQueue       g_eventQueue;
    extern TextGrid         g_textGrid;
    extern jmp_buf          g_mainLoopJmpBuf;
    extern Options          g_options;
    extern FontCollection   g_fontCollection;
    extern std::string      g_localDir;
    extern std::string      g_installDir;
    extern std::string      g_defaultsFileName;
    extern std::string      g_defaultsFilePath;
    extern std::string      g_nethackOptionsFileName;
    extern std::string      g_nethackOptionsFilePath;
    extern std::string      g_guidebookFileName;
    extern std::string      g_guidebookFilePath;
    extern std::string      g_licenseFileName;
    extern std::string      g_licenseFilePath;

}
#endif


