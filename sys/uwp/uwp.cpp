/* NetHack 3.6	uwp.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#include <windows.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string>
#include <streambuf>
#include <sstream>

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

#include "uwp.h"

#include "common\uwpglobals.h"
#include "common\CellBuffer.h"
#include "common\TextGrid.h"
#include "common\ScaneCode.h"
#include "common\uwpoption.h"

#include "uwpfilehandler.h"
#include "uwputil.h"

CellBuffer g_cellBuffer;

extern "C"  {

#include "hack.h"
#include "wintty.h"
#include "spell.h"

#ifndef HANGUPHANDLING
#error HANGUPHANDLING must be defined
#endif

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

    Nethack::g_textGrid.SetCursor(Nethack::Int2D(console.cursor.X, console.cursor.Y));

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
nttty_open(int mode)
{
    LI = Nethack::g_textGrid.GetDimensions().m_y;
    CO = Nethack::g_textGrid.GetDimensions().m_x;
}

void
getreturn(const char * str)
{
    msmsg("Hit <Enter> %s.", str);

    while (pgetchar() != '\n')
        ;
    return;
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
    return !Nethack::g_eventQueue.Empty();
}

void
win32_abort()
{
    if (wizard) {
        int c, ci, ct;

        if (!iflags.window_inited)
            c = 'n';
        ct = 0;
        msmsg("Execute debug breakpoint wizard?");
        while ((ci = nhgetch()) != '\n') {
            if (ct > 0) {
                backsp(); /* \b is visible on NT */
                (void)putchar(' ');
                backsp();
                ct = 0;
                c = 'n';
            }
            if (ci == 'y' || ci == 'n' || ci == 'Y' || ci == 'N') {
                ct = 1;
                c = ci;
                msmsg("%c", c);
            }
        }
        if (c == 'y')
            __debugbreak();
    }
    abort();
}

void nethack_exit(int result)
{
    if (!program_state.done_hup)
        getreturn("to exit");
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
    Sleep(ms);
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
    assert(console.current_nhattr == 0);
    if (attrib != ATR_NONE && console.current_nhattr == 0)
        console.current_nhattr |= 1 << attrib;
}

void
term_end_attr(int attrib)
{
    console.current_nhattr &= ~(1 << attrib);
    assert(console.current_nhattr == 0);
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
    console.cursor.X = 0;
    console.cursor.Y = 0;
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

#define MAX_OVERRIDES 256
unsigned char key_overrides[MAX_OVERRIDES];

void
map_subkeyvalue(char * op)
{
    char digits[] = "0123456789";
    int length, i, idx, val;
    char *kp;

    idx = -1;
    val = -1;
    kp = index(op, '/');
    if (kp) {
        *kp = '\0';
        kp++;
        length = strlen(kp);
        if (length < 1 || length > 3)
            return;
        for (i = 0; i < length; i++)
            if (!index(digits, kp[i]))
                return;
        val = atoi(kp);
        length = strlen(op);
        if (length < 1 || length > 3)
            return;
        for (i = 0; i < length; i++)
            if (!index(digits, op[i]))
                return;
        idx = atoi(op);
    }
    if (idx >= MAX_OVERRIDES || idx < 0 || val >= MAX_OVERRIDES || val < 1)
        return;
    key_overrides[idx] = val;
}

// TODO: we have no keyboard handlers -- we should remove need to define
void
load_keyboard_handler()
{ 
    return;
}

void
standoutbeg()
{
    term_start_attr(ATR_BOLD);
}

void
standoutend()
{
    term_end_attr(ATR_BOLD);
}

void
g_putch(int in_ch)
{
    console.cursor.X = ttyDisplay->curx;
    console.cursor.Y = ttyDisplay->cury;

    Nethack::TextCell textCell((Nethack::TextColor) console.current_nhcolor, (Nethack::TextAttribute) console.current_nhattr, in_ch);

    Nethack::g_textGrid.Put(console.cursor.X, console.cursor.Y, textCell, 1);
}

void
xputc_core(char ch)
{
    switch (ch) {
    case '\n':
        console.cursor.Y++;
        /* fall through */
    case '\r':
        console.cursor.X = 0;
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
    // Delay 50ms
    Sleep(50);
}



void
nttty_preference_update(const char * pref)
{
    // do nothing
}

char MapScanCode(const Nethack::Event & e)
{
    assert(e.m_type == Nethack::Event::Type::ScanCode);
    assert(e.m_scanCode >= ScanCode::Home && e.m_scanCode <= ScanCode::Delete);

    typedef struct {
        char normal, shift, control;
    } PadMapping;

    PadMapping keypad[] =
    {
        { 'y', 'Y', C('y') },    /* 7 */
        { 'k', 'K', C('k') },    /* 8 */
        { 'u', 'U', C('u') },    /* 9 */
        { 'm', C('p'), C('p') }, /* - */
        { 'h', 'H', C('h') },    /* 4 */
        { 'g', 'G', 'g' },       /* 5 */
        { 'l', 'L', C('l') },    /* 6 */
        { '+', 'P', C('p') },    /* + */
        { 'b', 'B', C('b') },    /* 1 */
        { 'j', 'J', C('j') },    /* 2 */
        { 'n', 'N', C('n') },    /* 3 */
        { 'i', 'I', C('i') },    /* Ins */
        { '.', ':', ':' }        /* Del */
    };

    PadMapping numpad[] = {
        { '7', M('7'), '7' },    /* 7 */
        { '8', M('8'), '8' },    /* 8 */
        { '9', M('9'), '9' },    /* 9 */
        { 'm', C('p'), C('p') }, /* - */
        { '4', M('4'), '4' },    /* 4 */
        { '5', M('5'), '5' },    /* 5 */
        { '6', M('6'), '6' },    /* 6 */
        { '+', 'P', C('p') },    /* + */
        { '1', M('1'), '1' },    /* 1 */
        { '2', M('2'), '2' },    /* 2 */
        { '3', M('3'), '3' },    /* 3 */
        { '0', M('0'), '0' },    /* Ins */
        { '.', ':', ':' }        /* Del */
    };

    PadMapping * pad = iflags.num_pad ? numpad : keypad;

    char c;
    int i = e.m_scanCode - ScanCode::Home;

    if (e.m_shift) {
        c = pad[i].shift;
    }
    else if (e.m_control) {
        c = pad[i].control;
    }
    else {
        c = pad[i].normal;
    }

    return c;
}

int raw_getchar()
{
    if (program_state.done_hup)
        return ESCAPE;

    Nethack::Event e;

    while (e.m_type != Nethack::Event::Type::Char &&
           e.m_type != Nethack::Event::Type::ScanCode)
    {
        e = Nethack::g_eventQueue.PopFront();
    }

    if (e.m_type == Nethack::Event::Type::ScanCode)
        return MapScanCode(e);
    else
    {
        return e.m_char;
    }
}

int
tgetch()
{
    really_move_cursor();
    char c = raw_getchar();

    if (c == EOF)
    {
        hangup(0);
        c = '\033';
    }

    return c;
}

int
ntposkey(int *x, int *y, int * mod)
{
    really_move_cursor();

    if (program_state.done_hup)
        return '\033';

    Nethack::Event e;
    
    while(e.m_type == Nethack::Event::Type::Undefined ||
          (e.m_type == Nethack::Event::Type::Mouse && !iflags.wc_mouse_support))
        e = Nethack::g_eventQueue.PopFront();

    if (e.m_type == Nethack::Event::Type::Char)
    {
        if (e.m_char == EOF)
        {
            hangup(0);
            e.m_char = '\033';
        }

        return e.m_char;
    }
    else if(e.m_type == Nethack::Event::Type::Mouse)
    {
        *x = e.m_pos.m_x;
        *y = e.m_pos.m_y;
        *mod = (e.m_tap == Nethack::Event::Tap::Left ? CLICK_1 : CLICK_2);

        return 0;
    }
    else
    {
        assert(e.m_type == Nethack::Event::Type::ScanCode);
        return MapScanCode(e);
    }


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
    // TODO: toggle reporting mouse events
    return;
}
#endif

void error VA_DECL(const char *, s)
{
    char buf[BUFSZ];
    VA_START(s);
    VA_INIT(s, const char *);

    clear_screen();

    (void)vsprintf(buf, s, VA_ARGS);
    Strcat(buf, "\n");
    raw_printf(buf);

    VA_END();

    nethack_exit(EXIT_FAILURE);
}

// TODO: These are either not needed or should
//       be declared extern appropriately.
extern struct menucoloring *menu_colorings;
extern unsigned nhUse_dummy;

void decl_clean_up(void)
{
    // TODO: Should review to see if we need to free memory before NULLing pointers

    hname = NULL;

    hackpid = 0;

#define ZEROARRAY(x) memset((void *) &x[0], 0, sizeof(x))
#define ZEROARRAYN(x,n) memset((void *) &x[0], 0, sizeof(x[0])*(n))
#define ZERO(x) memset((void *) &x, 0, sizeof(x))

    ZEROARRAY(bases);

    multi = 0;
    multi_reason = NULL;

    nroom = 0;
    nsubroom = 0;
    occtime = 0;

    ZERO(dungeon_topology);
    ZERO(quest_status);

    warn_obj_cnt = 0;
    ZEROARRAYN(smeq, MAXNROFROOMS + 1);
    doorindex = 0;
    save_cm = NULL;

    ZERO(killer);
    done_money = 0;
    nomovemsg = NULL;
    ZEROARRAY(plname);
    ZEROARRAY(pl_character);
    pl_race = '\0';

    ZEROARRAY(pl_fruit);
    ffruit = NULL;

    ZEROARRAY(tune);

    occtxt = NULL;

    yn_number = 0;

    ZEROARRAYN(hackdir, PATHLEN);

    ZEROARRAY(level_info);
    ZERO(program_state);

    tbx = 0;
    tby = 0;

    ZERO(m_shot);

    ZEROARRAYN(dungeons, MAXDUNGEON);
    sp_levchn = NULL;
    ZERO(upstair);
    ZERO(dnstair);
    ZERO(upladder);
    ZERO(dnladder);
    ZERO(sstairs);
    ZERO(updest);
    ZERO(dndest);
    ZERO(inv_pos);

    defer_see_monsters = FALSE;
    in_mklev = FALSE;
    stoned = FALSE;
    unweapon = FALSE;
    mrg_to_wielded = FALSE;

    in_steed_dismounting = FALSE;

    ZERO(bhitpos);
    ZEROARRAY(doors);

    ZEROARRAY(rooms);
    upstairs_room = NULL;
    dnstairs_room = NULL;
    sstairs_room = NULL;

    ZERO(level);
    ftrap = NULL;
    ZERO(youmonst);
    ZERO(context);
    ZERO(flags);
    ZERO(iflags);
    ZERO(u);
    ZERO(ubirthday);
    ZERO(urealtime);
    ZEROARRAY(lastseentyp);

    invent = NULL;
    uwep = NULL;
    uarm = NULL;
    uswapwep = NULL;
    uquiver = NULL;
    uarmu = NULL;
    uskin = NULL;
    uarmc = NULL;
    uarmh = NULL;
    uarms = NULL;
    uarmg = NULL;
    uarmf = NULL;
    uamul = NULL;
    uright = NULL;
    uleft = NULL;
    ublindf = NULL;
    uchain = NULL;
    uball = NULL;

    current_wand = NULL;
    thrownobj = NULL;
    kickedobj = NULL;

    ZEROARRAYN(spl_book, MAXSPELL + 1);

    moves = 1;
    monstermoves = 1;

    wailmsg = 0L;

    migrating_objs = NULL;
    billobjs = NULL;

    ZERO(zeroobj);
    ZERO(zeromonst);
    ZERO(zeroany);

    ZEROARRAYN(plname, PL_PSIZ);
    ZEROARRAYN(dogname, PL_PSIZ);
    ZEROARRAYN(catname, PL_PSIZ);
    ZEROARRAYN(horsename, PL_PSIZ);

    mydogs = NULL;
    migrating_mons = NULL;

    ZEROARRAY(mvitals);

    menu_colorings = NULL;

    vision_full_recalc = 0;

    WIN_MESSAGE = WIN_ERR;
    WIN_STATUS = WIN_ERR;
    WIN_MAP = WIN_ERR;
    WIN_INVEN = WIN_ERR;

    ZEROARRAYN(toplines, TBUFSZ);

    ZERO(tc_gbl_data);

    // TODO: storage should be freed
    ZEROARRAY(fqn_prefix);

    plinemsg_types = NULL;

    nhUse_dummy = 0;
}

bool get_string(std::string & string, int maxLength)
{
    string = "";

    bool done = false;
    while (1)
    {
        char c = raw_getchar();

        if (c == EOF || c == ESCAPE) return false;

        if (c == '\n') return true;

        if (c == '\b')
        {
            if (string.length() > 0)
            {
                Nethack::g_textGrid.Putstr(Nethack::TextColor::White, Nethack::TextAttribute::None, "\b");
                string = string.substr(0, string.length() - 1);
            }
            continue;
        }

        if (!isprint(c))
            continue;

        if (string.length() < maxLength)
        {
            Nethack::g_textGrid.Put(Nethack::TextColor::White, Nethack::TextAttribute::None, c);
            string += c;
        }
    }
}

void add_option()
{
    Nethack::g_textGrid.Clear();
    Nethack::g_textGrid.Putstr(0, 0, Nethack::TextColor::White, Nethack::TextAttribute::None,
        "Add NETHACKOPTION");
    Nethack::g_textGrid.Putstr(10, 1, Nethack::TextColor::White, Nethack::TextAttribute::None,
        "Name:");

    std::string name;
    if (!get_string(name, 60)) return;
    if (name.length() == 0) return;

    Nethack::g_textGrid.Putstr(10, 2, Nethack::TextColor::White, Nethack::TextAttribute::None,
        "Value:");

    std::string value;
    if (!get_string(value, 60)) return;

    Nethack::g_options.m_options.push_back(Nethack::Option(name, value));
    Nethack::g_options.Store();
}

void remove_option()
{
    Nethack::g_textGrid.Clear();
    Nethack::g_textGrid.Putstr(0, 0, Nethack::TextColor::White, Nethack::TextAttribute::None,
        "Remove NETHACKOPTION");
    Nethack::g_textGrid.Putstr(10, 1, Nethack::TextColor::White, Nethack::TextAttribute::None,
        "Name:");

    std::string name;
    if (!get_string(name, 60)) return;
    if (name.length() == 0) return;

    Nethack::g_options.Remove(name);
    Nethack::g_options.Store();

}

bool change_options()
{
    bool done = false;
    while (!done)
    {
        std::string optionsString = Nethack::g_options.GetString();

        Nethack::g_textGrid.Clear();
        Nethack::g_textGrid.Putstr(0, 0, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "Change NETHACKOPTIONS");

        Nethack::g_textGrid.Putstr(10, 2, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "a - Add");
        Nethack::g_textGrid.Putstr(10, 3, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "b - Remove");

        Nethack::g_textGrid.Putstr(0, 8, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "Current Options:");
        Nethack::g_textGrid.Putstr(10, 9, Nethack::TextColor::White, Nethack::TextAttribute::None,
            optionsString.c_str());

        Nethack::g_textGrid.Putstr(0, 6, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "Select action:");

        char c = raw_getchar();

        if (c == EOF || c == ESCAPE) return false;

        switch (c)
        {
        case 'a': add_option();  break;
        case 'b': remove_option();  break;
        }
    }

    return true;
}

const char * uwp_getenv(const char * env)
{
    static std::string options = Nethack::g_options.GetString();
    return options.c_str();
}

bool main_menu(const char * localDir)
{
    std::string optionsFilePath(localDir);
    optionsFilePath += "\\envoptions.nh";

    Nethack::g_options.Load(optionsFilePath);

    Platform::String ^ fileText = ref new Platform::String();


    bool done = false;
    while (!done)
    {
        Nethack::g_textGrid.Clear();
        Nethack::g_textGrid.Putstr(10, 0, Nethack::TextColor::White, Nethack::TextAttribute::None, 
            "NetHack, Universal Windows Port by Bart House");
        Nethack::g_textGrid.Putstr(19, 1, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "Copyright 2016-2017");

        Nethack::g_textGrid.Putstr(19, 4, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "a - Play");
        Nethack::g_textGrid.Putstr(19, 5, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "b - Change NETHACKOPTIONS");
        Nethack::g_textGrid.Putstr(19, 6, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "c - Save default options to file");
        Nethack::g_textGrid.Putstr(19, 7, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "d - Load default options from file");

        Nethack::g_textGrid.Putstr(19, 9, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "Select action:");

        char c = raw_getchar();

        if (c == EOF) return false;

        switch (c)
        {
        case 'a': done = true; break;
        case 'b': if (!change_options()) return false; break;
        case 'c':
            {
                std::string readText;
                std::fstream input(optionsFilePath.c_str(), std::fstream::in | std::fstream::binary);
                if (input.is_open())
                {
                    input.seekg(0, std::ios::end);
                    size_t size = input.tellg();
                    input.seekg(0, std::ios::beg);
                    std::vector<char> bytes(size);
                    input.read(bytes.data(), size);
                    input.close();
                    readText = std::string(bytes.data(), size);
                }

                fileText = Nethack::to_string(readText);

                Nethack::FileHandler::s_instance->SaveFilePicker(fileText);
                break;

            }
        case 'd': 
            {
                fileText = Nethack::FileHandler::s_instance->LoadFilePicker();
                std::string writeText = Nethack::to_string(fileText);
                std::fstream output(optionsFilePath.c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);
                if (output.is_open())
                {
                    output.write(writeText.c_str(), writeText.length());
                    output.close();
                }

                break;

            }
        }

    }

    return true;

}

void copy_missing_file(const char * filename, const char * srcDir, const char * dstDir)
{
    // TODO(bhouse) Copy files from install dir to local dir
    char srcPath[256];
    sprintf(srcPath, "%s\\%s", srcDir, filename);

    char dstPath[256];
    sprintf(dstPath, "%s\\%s", dstDir, filename);

    FILE * dstFp = fopen(dstPath, "rb");
    if (dstFp != NULL)
    {
        fclose(dstFp);
    }
    else
    {
        dstFp = fopen(dstPath, "wb+");
        assert(dstFp != NULL);

        FILE * srcFp = fopen(srcPath, "rb");
        assert(srcFp != NULL);

        int failure = fseek(srcFp, 0, SEEK_END);
        assert(!failure);

        size_t fileSize = ftell(srcFp);
        rewind(srcFp);

        char * data = (char *)malloc(fileSize);
        assert(data != NULL);

        int readBytes = fread(data, 1, fileSize, srcFp);
        assert(readBytes == fileSize);

        int writeBytes = fwrite(data, 1, fileSize, dstFp);
        assert(writeBytes == fileSize);

        free(data);
        fclose(srcFp);
        fclose(dstFp);
    }
}

extern boolean FDECL(uwpmain, (const char *, const char *));

void mainloop(const char * localDir, const char * installDir)
{
    copy_missing_file("defaults.nh", installDir, localDir);

    if (!main_menu(localDir))
        return;

    if (setjmp(Nethack::g_mainLoopJmpBuf) == 0)
    {
        boolean resuming;

        sys_early_init();

        resuming = uwpmain(localDir, installDir);

        moveloop(resuming);
    }

    decl_clean_up();
}


} // extern "C"
