#include <windows.h>
#include <assert.h>

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>

#include "common\CellBuffer.h"
#include "common\TextGrid.h"

CellBuffer g_cellBuffer;
Nethack::TextGrid g_textGrid(Nethack::Int2D(80, 28));

extern "C"  {

#include "hack.h"
#include "wintty.h"

struct console_t {
    WORD background;
    WORD foreground;
    WORD attr;
    int current_nhcolor;
    WORD current_nhattr;
    COORD cursor;
} console = {
    0,
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    NO_COLOR,
    0,
    { 0, 0 }
};


void
    chdrive(char *str)
{
    assert(0);
}

void
nttty_open(int mode)
{
    return;
}

void
getreturn(const char *str)
{
    assert(0);
#if 0
#ifdef WIN32
    if (!getreturn_enabled)
        return;
#endif
#ifdef TOS
    msmsg("Hit <Return> %s.", str);
#else
    msmsg("Hit <Enter> %s.", str);
#endif
    while (pgetchar() != '\n')
        ;
    return;
#endif
}

/* this is used as a printf() replacement when the window
* system isn't initialized yet
*/
void msmsg
VA_DECL(const char *, fmt)
{
    assert(0);
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    VA_END();
    return;
}
    
int kbhit(void)
{
    return 0;
}

void error(const char * fmt, ...)
{
    assert(0);
}

#ifdef PORT_DEBUG
void
    win32con_debug_keystrokes()
{
    assert(0);
    return;
}
void
    win32con_handler_info()
{
    assert(0);
    return;
}
#endif

void interject_assistance(int num, int interjection_type, void * ptr1, void * ptr2)
{
    assert(0);
}

void interject(int interjection_type)
{
    assert(0);
}

void win32_abort(void)
{
    assert(0);
}

void nethack_exit(int result)
{
    assert(0);
}

HANDLE ffhandle = (HANDLE)0;
WIN32_FIND_DATAA ffd;

int findfirst(char *path)
{
    if (ffhandle) {
        FindClose(ffhandle);
        ffhandle = (HANDLE)0;
    }
    ffhandle = FindFirstFileExA(path, FindExInfoStandard, &ffd, FindExSearchNameMatch, NULL, 0);
    return (ffhandle == INVALID_HANDLE_VALUE) ? 0 : 1;
}

int findnext(void)
{
    return FindNextFileA(ffhandle, &ffd) ? 1 : 0;
}

char * foundfile_buffer()
{
    return &ffd.cFileName[0];
}

char * get_username(int * lan_username_size)
{
#if 0
    static TCHAR username_buffer[BUFSZ];
    unsigned int status;
    DWORD i = BUFSZ - 1;

    /* i gets updated with actual size */
    status = GetUserName(username_buffer, &i);
    if (status)
        username_buffer[i] = '\0';
    else
        Strcpy(username_buffer, "NetHack");
    if (lan_username_size)
        *lan_username_size = strlen(username_buffer);
    return username_buffer;
#endif
    // TODO: Determine the right way to get user name in UWP
    return "bhouse";
}

void Delay(int ms)
{
    assert(0);
    // (void)Sleep(ms);
}

// TODO: Dangerous call ... string needs to have been allocated with enough space
void append_slash(char * name)
{
    char *ptr;

    if (!*name)
        return;

    ptr = name + (strlen(name) - 1);

    if (*ptr != '\\' && *ptr != '/' && *ptr != ':') {
        *++ptr = '\\';
        *++ptr = '\0';
    }
}

void tty_number_pad(int state)
{
    // do nothing for now
}

void
tty_startup(int * wid, int * hgt)
{
    *wid = g_cellBuffer.GetWidth();
    *hgt = g_cellBuffer.GetHeight();

    set_option_mod_status("mouse_support", SET_IN_GAME);
}

void
tty_start_screen()
{
    if (iflags.num_pad)
        tty_number_pad(1); /* make keypad send digits */
}

void
home()
{
    console.cursor.X = console.cursor.Y = 0;
    ttyDisplay->curx = ttyDisplay->cury = 0;
}

void
backsp()
{
    assert(0);
}

void
tty_nhbell()
{
    assert(0);
}

void
tty_end_screen()
{
    assert(0);
}

void
cmov(int x, int y)
{
    ttyDisplay->cury = y;
    ttyDisplay->curx = x;
    console.cursor.X = x;
    console.cursor.Y = y;
}

void
nocmov(int x, int y)
{
    console.cursor.X = x;
    console.cursor.Y = y;
    ttyDisplay->curx = x;
    ttyDisplay->cury = y;
}

void
term_start_color(int color)
{
    console.current_nhcolor = color;
}

void
term_end_color(void)
{
    console.current_nhcolor = NO_COLOR;
}

void
term_end_raw_bold(void)
{
    term_end_attr(ATR_BOLD);
}

void
term_start_raw_bold(void)
{
    term_start_attr(ATR_BOLD);
}

void
term_start_attr(int attrib)
{
    console.current_nhattr |= 1 << attrib;
}

void
term_end_attr(int attrib)
{
    console.current_nhattr &= ~(1 << attrib);
}

void
cl_eos()
{
    assert(0);
}

void
cl_end()
{
    assert(0);
}

void
raw_clear_screen()
{
    g_cellBuffer.Clear();
}

void
clear_screen()
{
    raw_clear_screen();
    home();
}

char erase_char, kill_char;

int
has_color(int color)
{
    assert(0);
    return 0;
}

void
map_subkeyvalue(char *op)
{
    assert(0);
    return;
}

void
load_keyboard_handler()
{
    assert(0);
    return;
}

void
standoutbeg()
{
    assert(0);
}

void
standoutend()
{
    assert(0);
}

void
g_putch(int in_ch)
{
    assert(0);
}

#ifdef USER_SOUNDS
void
play_usersound(const char *filename, int volume)
{
    assert(0);
}
#endif /*USER_SOUNDS*/

#ifdef RUNTIME_PORT_ID
void
append_port_id(char *buf)
{
    assert(0);
}
#endif /* RUNTIME_PORT_ID */

void
xputc_core(char ch)
{
    boolean inverse = FALSE;
    switch (ch) {
    case '\n':
        console.cursor.Y++;
        /* fall through */
    case '\r':
        console.cursor.X = 1;
        break;
    case '\b':
        console.cursor.X--;
        break;
    default:

        ConsoleColor color = (ConsoleColor) console.current_nhcolor;
        ConsoleAttribute attribute = (ConsoleAttribute) console.current_nhattr;

        g_cellBuffer.Put(console.cursor.X, console.cursor.Y, CellAttribute(color, attribute), ch, 1);

        Nethack::TextCell textCell((Nethack::TextColor) console.current_nhcolor, (Nethack::TextAttribute) console.current_nhattr, ch);

        g_textGrid.Put(console.cursor.X, console.cursor.Y, textCell, 1);

        console.cursor.X++;
    }
}

void xputc(char ch)
{
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;
    xputc_core(ch);
    return;
}

void
xputs(const char *s)
{
    int k;
    int slen = strlen(s);

    if (ttyDisplay) {
        console.cursor.X = ttyDisplay->curx;
        console.cursor.Y = ttyDisplay->cury;
    }

    if (s) {
        for (k = 0; k < slen && s[k]; ++k)
            xputc_core(s[k]);
    }
}

void
tty_delay_output()
{
    assert(0);
}

void
nttty_preference_update(const char * pref)
{
    assert(0);
}

static void
really_move_cursor()
{

#if 0
#ifdef PORT_DEBUG
    char oldtitle[BUFSZ], newtitle[BUFSZ];
    if (display_cursor_info && wizard) {
        oldtitle[0] = '\0';
        if (GetConsoleTitle(oldtitle, BUFSZ)) {
            oldtitle[39] = '\0';
        }
        Sprintf(newtitle, "%-55s tty=(%02d,%02d) nttty=(%02d,%02d)", oldtitle,
            ttyDisplay->curx, ttyDisplay->cury, console.cursor.X, console.cursor.Y);
        (void)SetConsoleTitle(newtitle);
    }
#endif
    if (ttyDisplay) {
        console.cursor.X = ttyDisplay->curx;
        console.cursor.Y = ttyDisplay->cury;
    }
    SetConsoleCursorPosition(hConOut, console.cursor);
#endif

}

int
tgetch()
{
    int mod;
    coord cc;
    DWORD count;
    really_move_cursor();

    if (program_state.done_hup)
        return '\033';

    boolean done = false;
    while (!done)
    {
        // wait for character to show up
        Sleep(1000);
    }

    return 0;

}

void
gettty()
{
    erase_char = '\b';
    kill_char = 21; /* cntl-U */
    iflags.cbreak = TRUE;
}

void
settty(const char *s)
{
    assert(0);
}

void
setftty()
{
    start_screen();
}

#ifndef NO_MOUSE_ALLOWED
void
toggle_mouse_support()
{
    return;
}
#endif


extern boolean FDECL(uwpmain, (const char *, const char *));

void mainloop(const char * localDir, const char * installDir)
{
    boolean resuming;

    //    g_localDir = strdup(localDir);
    //    g_installDir = strdup(installDir);

    //    CopyInstallFile("nhdat");
    //    CopyInstallFile("defaults.nh");

    sys_early_init();

//    Strcpy(default_window_sys, "tty");

    resuming = uwpmain(localDir, installDir);

    moveloop(resuming);

#if 0
    if (setjmp(g_mainLoopJumpBuf) == 0)
    {
        moveloop();
    }
#endif

}


} // extern "C"
