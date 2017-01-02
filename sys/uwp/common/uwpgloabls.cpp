#include "uwpglobals.h"

namespace Nethack
{

    EventQueue g_eventQueue;
    TextGrid    g_textGrid(Int2D(80, 25));
    jmp_buf     g_mainLoopJmpBuf;

}
