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

#include "common\uwpglobals.h"
#include "common\CellBuffer.h"
#include "common\TextGrid.h"

CellBuffer g_cellBuffer;

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

void really_move_cursor()
{
    if (ttyDisplay) {
        console.cursor.X = ttyDisplay->curx;
        console.cursor.Y = ttyDisplay->cury;
    }

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


void
    chdrive(char *str)
{
    assert(0);
}

void
nttty_open(int mode)
{
    LI = Nethack::g_textGrid.GetDimensions().m_y;
    CO = Nethack::g_textGrid.GetDimensions().m_x;
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
    char buf[ROWNO * COLNO]; /* worst case scenario */
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    Vsprintf(buf, fmt, VA_ARGS);

    xputs(buf);

    if (ttyDisplay)
        curs(BASE_WINDOW, console.cursor.X + 1, console.cursor.Y);

    VA_END();
    return;
}
    
int kbhit(void)
{
    assert(0);
    return 0;
}

void error VA_DECL(const char *, s)
{
    char buf[BUFSZ];
    VA_START(s);
    VA_INIT(s, const char *);
    /* error() may get called before tty is initialized */
    if (iflags.window_inited)
        end_screen();
    if (!strncmpi(windowprocs.name, "tty", 3)) {
        buf[0] = '\n';
        (void)vsprintf(&buf[1], s, VA_ARGS);
        Strcat(buf, "\n");
        msmsg(buf);
    }
    else {
        (void)vsprintf(buf, s, VA_ARGS);
        Strcat(buf, "\n");
        raw_printf(buf);
    }
    VA_END();
    exit(EXIT_FAILURE);
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
    longjmp(Nethack::g_mainLoopJmpBuf, -1);
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
    *wid = Nethack::g_textGrid.GetDimensions().m_x;
    *hgt = Nethack::g_textGrid.GetDimensions().m_y;

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
tty_nhbell()
{
    // do nothing
}

void
tty_end_screen()
{
    clear_screen();
    really_move_cursor();
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
    int x = ttyDisplay->curx;
    int y = ttyDisplay->cury;

    int cx = Nethack::g_textGrid.GetDimensions().m_x - x;
    int cy = (Nethack::g_textGrid.GetDimensions().m_y - (y + 1)) * Nethack::g_textGrid.GetDimensions().m_x;

    Nethack::g_textGrid.Put(x, y, Nethack::TextCell(), cx + cy);

    tty_curs(BASE_WINDOW, x + 1, y);
}

void
cl_end()
{
    int cx;
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;
    cx = Nethack::g_textGrid.GetDimensions().m_x - console.cursor.X;

    Nethack::g_textGrid.Put(console.cursor.X, console.cursor.Y, Nethack::TextCell(), cx);

    tty_curs(BASE_WINDOW, (int)ttyDisplay->curx + 1, (int)ttyDisplay->cury);
}

void
raw_clear_screen()
{
    Nethack::g_textGrid.Clear();
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
    if ((color >= 0) && (color < (int) Nethack::TextColor::Count))
        return 1;

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
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;

    Nethack::TextCell textCell((Nethack::TextColor) console.current_nhcolor, (Nethack::TextAttribute) console.current_nhattr, in_ch);

    Nethack::g_textGrid.Put(console.cursor.X, console.cursor.Y, textCell, 1);
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

        Nethack::TextCell textCell((Nethack::TextColor) console.current_nhcolor, (Nethack::TextAttribute) console.current_nhattr, ch);

        Nethack::g_textGrid.Put(console.cursor.X, console.cursor.Y, textCell, 1);

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
backsp()
{
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;
    xputc_core('\b');
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

int
tgetch()
{
    really_move_cursor();

    if (program_state.done_hup)
        return '\033';

    Nethack::Event e = Nethack::g_eventQueue.Pop();

    assert(e.m_type == Nethack::Event::Type::Char);

    return e.m_char;

}

int
ntposkey(int *x, int *y, int * mod)
{
    really_move_cursor();

    if (program_state.done_hup)
        return '\033';

    Nethack::Event e = Nethack::g_eventQueue.Pop();

    assert(e.m_type == Nethack::Event::Type::Char);

    if (!e.m_char) {
        *x = e.m_x;
        *y = e.m_y;
    }

    return e.m_char;
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
    cmov(ttyDisplay->curx, ttyDisplay->cury);
    end_screen();
    if (s)
        raw_print(s);
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
    if (setjmp(Nethack::g_mainLoopJmpBuf) == 0)
    {
        boolean resuming;

        sys_early_init();

        resuming = uwpmain(localDir, installDir);

        moveloop(resuming);
    }
}


} // extern "C"
