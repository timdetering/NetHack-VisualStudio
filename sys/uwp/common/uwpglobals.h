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

