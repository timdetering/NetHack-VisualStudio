#include "uwpglobals.h"

namespace Nethack
{

    EventQueue g_eventQueue;
    TextGrid    g_textGrid(Int2D(80, 28));
    jmp_buf     g_mainLoopJmpBuf;

}
