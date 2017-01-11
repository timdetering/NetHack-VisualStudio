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
#include "spell.h"

#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif


#ifndef HANGUPHANDLING
#error HANGUPHANDLING must be defined
#endif


void
nttty_open(int mode)
{
    LI = Nethack::g_textGrid.GetDimensions().m_y;
    CO = Nethack::g_textGrid.GetDimensions().m_x;
}

    
int kbhit(void)
{
    return !Nethack::g_eventQueue.Empty();
}

// used in panic
#if 1
void
win32_abort()
{
    // TODO(bhouse) Decide whether there is any value in retail NetHack
    //              having abilty to enter debug break.  For now, we
    //              will not allow it.

#if 0
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
#endif

    abort();
}
#endif

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
    return "noname";
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
nttty_preference_update(const char * pref)
{
    // do nothing
}

char MapScanCode(const Nethack::Event & e)
{
    assert(e.m_type == Nethack::Event::Type::ScanCode);

    char c = 0;

    if (!e.m_alt)
    {
        if (e.m_scanCode >= Nethack::ScanCode::Home && e.m_scanCode <= Nethack::ScanCode::Delete)
        {
            typedef struct {
                char normal, shift, control;
            } PadMapping;

            static const PadMapping keypad[] =
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

            static const PadMapping numpad[] = {
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

            const PadMapping * pad = iflags.num_pad ? numpad : keypad;

            int i = (int) e.m_scanCode - (int)Nethack::ScanCode::Home;

            if (e.m_shift) {
                c = pad[i].shift;
            }
            else if (e.m_control) {
                c = pad[i].control;
            }
            else {
                c = pad[i].normal;
            }
        }
    }
    else
    {
        int scanToChar[(int)Nethack::ScanCode::Count] = {
            0, // Unknown
            0, // Escape
            '1' | 0x80, // One
            '2' | 0x80, // Two
            '3' | 0x80, // Three
            '4' | 0x80, // Four
            '5' | 0x80, // Five
            '6' | 0x80, // Six
            '7' | 0x80, // Seven
            '8' | 0x80, // Eight
            '9' | 0x80, // Nine
            '0' | 0x80, // Zero
            0, // Minus
            0, // Equal
            0, // Backspace
            0, // Tab
            'q' | 0x80, // Q
            'w' | 0x80, // W
            'e' | 0x80, // E
            'r' | 0x80, // R
            't' | 0x80, // T
            'y' | 0x80, // Y
            'u' | 0x80, // U
            'i' | 0x80, // I
            'o' | 0x80, // O
            'p' | 0x80, // P
            0, // LeftBracket
            0, //  RightBracket
            0, // Enter
            0, // Control
            'a' | 0x80, // A
            's' | 0x80, // S
            'd' | 0x80, // D
            'f' | 0x80, // F
            'g' | 0x80, // G
            'h' | 0x80, // H
            'j' | 0x80, // J
            'k' | 0x80, // K
            'l' | 0x80, // L
            0, // SemiColon
            0, // Quote
            0, // BackQuote
            0, // LeftShift
            0, // BackSlash
            'z' | 0x80, // Z
            'x' | 0x80, // X
            'c' | 0x80, // C
            'v' | 0x80, // V
            'b' | 0x80, // B
            'n' | 0x80, // N
            'm' | 0x80, // M
            0, // Comma
            0, // Period
            '?' | 0x80, // ForwardSlash
            0, // RightShift
            0, // NotSure
            0, // Alt
            0, // Space
            0, // Caps
            0, // F1
            0, // F2
            0, // F3
            0, // F4
            0, // F5
            0, // F6
            0, // F7
            0, // F8
            0, // F9
            0, // F10
            0, // Num
            0, // Scroll
            0, // Home
            0, // Up
            0, // PageUp
            0, // PadMinus
            0, // Left
            0, // Center
            0, // Right
            0, // PadPlus
            0, // End
            0, // Down
            0, // PageDown
            0, // Insert
            0, // Delete
            0, // Scan84
            0, // Scan85
            0, // Scan86
            0, // F11
            0, // F12
        };

        assert(((int) e.m_scanCode >= 0) && ((int) e.m_scanCode < (int) Nethack::ScanCode::Count));

        if (e.m_scanCode >= Nethack::ScanCode::Unknown && e.m_scanCode < Nethack::ScanCode::Count)
            c = (char) scanToChar[(int) e.m_scanCode];
        
    }

    return c;
}

int raw_getchar()
{
    if (program_state.done_hup)
        return ESCAPE;

    Nethack::Event e;

    while (e.m_type == Nethack::Event::Type::Undefined ||
           (e.m_type == Nethack::Event::Type::ScanCode && MapScanCode(e) == 0) ||
           (e.m_type == Nethack::Event::Type::Mouse))
    {
        e = Nethack::g_eventQueue.PopFront();
    }

    if (e.m_type == Nethack::Event::Type::ScanCode)
        return MapScanCode(e);
    else
    {
        assert(e.m_type == Nethack::Event::Type::Char);
        return e.m_char;
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

    raw_clear_screen();

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

void raw_clear_screen(void)
{
    Nethack::g_textGrid.Clear();
}

bool get_string(std::string & string, unsigned int maxLength)
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

bool file_exists(std::string & filePath)
{
    std::ifstream f(filePath.c_str());
    return f.good();
}

void rename_file(const char * from, const char * to)
{
    std::string toPath(to);

    if (file_exists(toPath))
        return;

    rename(from, to);
}

void copy_to_local(std::string & fileName, bool onlyIfMissing)
{
    std::string localPath = Nethack::g_localDir;
    localPath += "\\";
    localPath += fileName;

    std::string installPath = Nethack::g_installDir;
    installPath += "\\";
    installPath += fileName;

    if (onlyIfMissing && file_exists(localPath))
        return;

    FILE * dstFp = fopen(localPath.c_str(), "wb+");
    assert(dstFp != NULL);

    FILE * srcFp = fopen(installPath.c_str(), "rb");
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

void change_options()
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

        if (c == EOF || c == ESCAPE || c == '\n') return;

        c = tolower(c);

        switch (c)
        {
        case 'a': add_option();  break;
        case 'b': remove_option();  break;
        }
    }
}

char * uwp_getenv(const char * env)
{
    static std::string options = Nethack::g_options.GetString();
    return (char *) options.c_str();
}

void save_file(std::string & filePath)
{
    std::string readText;
    std::fstream input(filePath.c_str(), std::fstream::in | std::fstream::binary);
    if (input.is_open())
    {
        input.seekg(0, std::ios::end);
        size_t size = (size_t)input.tellg();
        input.seekg(0, std::ios::beg);
        std::vector<char> bytes(size);
        input.read(bytes.data(), size);
        input.close();
        readText = std::string(bytes.data(), size);
    }

    std::string fileExtension = filePath.substr(filePath.rfind('.'), std::string::npos);
    std::string fileName = filePath.substr(filePath.rfind('\\') + 1, filePath.rfind('.') - filePath.rfind('\\') - 1);

    Platform::String ^ fileText = Nethack::to_platform_string(readText);
    Platform::String ^ fileNameStr = Nethack::to_platform_string(fileName);
    Platform::String ^ extensionStr = Nethack::to_platform_string(fileExtension);
    Nethack::FileHandler::s_instance->SaveFilePicker(fileText, fileNameStr, extensionStr);
}

void load_file(std::string & filePath)
{
    std::string fileExtension = filePath.substr(filePath.rfind('.'), std::string::npos);
    Platform::String ^ extensionStr = Nethack::to_platform_string(fileExtension);
    Platform::String ^ fileText = Nethack::FileHandler::s_instance->LoadFilePicker(extensionStr);
    if (fileText != nullptr)
    {
        std::string writeText = Nethack::to_string(fileText);
        std::fstream output(filePath.c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);
        if (output.is_open())
        {
            output.write(writeText.c_str(), writeText.length());
            output.close();
        }
    }
}

bool main_menu(void)
{
    std::string defaultsFileName = "defaults.nh";
    std::string defaultsFilePath = Nethack::g_localDir;
    defaultsFilePath += "\\";
    defaultsFilePath += defaultsFileName;

    std::string guidebookFilePath = Nethack::g_installDir;
    guidebookFilePath += "\\Guidebook.txt";

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
        Nethack::g_textGrid.Putstr(19, 6, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "b - Change NETHACKOPTIONS");
        Nethack::g_textGrid.Putstr(19, 7, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "c - Save defaults.nh to file");
        Nethack::g_textGrid.Putstr(19, 8, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "d - Load defaults.nh from file");
        Nethack::g_textGrid.Putstr(19, 9, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "e - Reset defaults.nh");
        Nethack::g_textGrid.Putstr(19, 10, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "f - Save Guidebook.txt");
        Nethack::g_textGrid.Putstr(19, 11, Nethack::TextColor::White, Nethack::TextAttribute::None,
            "Select action:");

        char c = raw_getchar();

        if (c == EOF) return false;

        c = tolower(c);

        switch (c)
        {
        case 'a': done = true; break;
        case 'b': change_options(); break;
        case 'c': save_file(defaultsFilePath); break;
        case 'd': load_file(defaultsFilePath); break;
        case 'e': 
            copy_to_local(defaultsFileName, false);
            Nethack::g_textGrid.Putstr(19, 12, Nethack::TextColor::White, Nethack::TextAttribute::None,
                "Reset complete <hit key to continue>");
            raw_getchar();
            break;
        case 'f': save_file(guidebookFilePath); break;
        }

    }

    return true;

}

extern boolean FDECL(uwpmain, (const char *, const char *));

void mainloop(const char * localDir, const char * installDir)
{
    Nethack::g_localDir = std::string(localDir);
    Nethack::g_installDir = std::string(installDir);

    std::string defaultsFileName("defaults.nh");

    copy_to_local(defaultsFileName, true);

    std::string optionsFilePath = Nethack::g_localDir;
    optionsFilePath += "\\nethackoptions";

    Nethack::g_options.Load(optionsFilePath);

    if (!main_menu())
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
