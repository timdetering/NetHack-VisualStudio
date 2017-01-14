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

using namespace Nethack;

CellBuffer g_cellBuffer;

extern "C"  {

#include "hack.h"
#include "spell.h"

#ifdef TTY_GRAPHICS
#include "wintty.h"
#endif

#include "date.h"
#include "patchlevel.h"


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

void Delay(int ms)
{
    Sleep(ms);
}

// TODO(bhouse): Dangerous call ... string needs to have been allocated with enough space.
//               Caller should pass in length of allocation.
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

    if (!e.m_alt) {
        if (e.m_scanCode >= Nethack::ScanCode::Home && e.m_scanCode <= Nethack::ScanCode::Delete) {
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
            } else if (e.m_control) {
                c = pad[i].control;
            } else {
                c = pad[i].normal;
            }
        }
    } else {
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
           (e.m_type == Nethack::Event::Type::Mouse)) {
        e = Nethack::g_eventQueue.PopFront();
    }

    if (e.m_type == Nethack::Event::Type::ScanCode)
        return MapScanCode(e);
    else  {
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

void raw_clear_screen(void)
{
    Nethack::g_textGrid.Clear();
}

bool get_string(std::string & string, unsigned int maxLength)
{
    string = "";

    bool done = false;
    while (1) {
        char c = raw_getchar();

        if (c == EOF || c == ESCAPE) return false;

        if (c == '\n') return true;

        if (c == '\b') {
            if (string.length() > 0) {
                Nethack::g_textGrid.Putstr(Nethack::TextColor::White, Nethack::TextAttribute::None, "\b");
                string = string.substr(0, string.length() - 1);
            }
            continue;
        }

        if (!isprint(c))
            continue;

        if (string.length() < maxLength)  {
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
    localPath += fileName;

    std::string installPath = Nethack::g_installDir;
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
    while (!done) {
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

    Nethack::g_textGrid.Clear();
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
    if (input.is_open())  {
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
    if (fileText != nullptr)  {
        std::string writeText = Nethack::to_string(fileText);
        std::fstream output(filePath.c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);
        if (output.is_open())  {
            output.write(writeText.c_str(), writeText.length());
            output.close();
        }
    }
}

void reset_defaults_file(void)
{
    copy_to_local(g_defaultsFileName, false);
    clear_nhwindow(BASE_WINDOW);
    getreturn("Reset complete");
}

bool main_menu(void)
{
    // TODO(bhouse): Need to review use of BASE_WINDOW.  Really like to have this code be
    //               windowing system agnostic.  Use and knowledge of BASE_WINDOW breaks
    //               that goal.
    clear_nhwindow(BASE_WINDOW);

    bool play = false;
    bool done = false;
    while (!done && !play) {
        // TODO(bhouse): this can be removed when we switch to using nh menus for all actions
        Nethack::g_textGrid.Clear();

        anything any = zeroany;

        winid menu = create_nhwindow(NHW_MENU);
        start_menu(menu);

        const char * items[] = {
            "[ ]" COPYRIGHT_BANNER_A,
            "[ ]" COPYRIGHT_BANNER_B,
            "[ ]Universal Windows Port, Copyright 2016-2017",
            "[ ]         By Bart House",
            "[ ]" COPYRIGHT_BANNER_C,
            "[ ]" COPYRIGHT_BANNER_D,
            "[ ]",
            "[ ]Source: https://github.com/barthouse/NetHackPublic",
            "[ ]Support: https://github.com/barthouse/NetHackPublic/wiki/Support",
            "[ ]Email: nethack@barthouse.com",
            "[ ]",
            "[ ]Pick an action",
            "[ ]",
            "[a]Play",
            "[b]Change NETHACKOPTIONS",
            "[c]Save defaults.nh to file",
            "[d]Load defaults.nh from file",
            "[e]Reset defaults.nh",
            "[f]Save Guidebook.txt"
        };

        for (int i = 0; i < SIZE(items); i++) {
            any.a_int = items[i][1] == ' ' ? 0 : items[i][1];
            add_menu(menu, NO_GLYPH, &any, 0, 0, ATR_NONE, &items[i][3], MENU_UNSELECTED);
        }

        end_menu(menu, (char *)0);

        menu_item *pick = NULL;
        int count = select_menu(menu, PICK_ONE, &pick);

        destroy_nhwindow(menu);

        if (count == 1) {
            switch (pick->item.a_int) {
            case 'a': play = true; break;
            case 'b': change_options(); break;
            case 'c': save_file(g_defaultsFilePath); break;
            case 'd': load_file(g_defaultsFilePath); break;
            case 'e': reset_defaults_file(); break;
            case 'f': save_file(g_guidebookFilePath); break;
            }
        } else if (count == -1) {
            done = true;
        }

        free((genericptr_t)pick);
    }

    return play;
}

extern void rename_file(const char * from, const char * to);

/* to support previous releases, we rename save games to new save game format */
void rename_save_files()
{
    char *foundfile;
    const char *fq_save;

    const char * oldsaves[] = { 
        "bhouse-*.NetHack-saved-game",
        "noname-*.NetHack-saved-game" };

    for (int i = 0; i < SIZE(oldsaves); i++) {
        fq_save = fqname(oldsaves[i], SAVEPREFIX, 0);

        foundfile = foundfile_buffer();
        if (findfirst((char *)fq_save)) {
            do {
                char oldPath[512];
                char newname[512];

                strcpy(newname, &foundfile[7]);

                fq_save = fqname(foundfile, SAVEPREFIX, 0);
                strcpy(oldPath, fq_save);

                const char * newPath = fqname(newname, SAVEPREFIX, 0);

                rename_file(oldPath, newPath);

            } while (findnext());
        }
    }
}

extern boolean FDECL(uwpmain, (const char *, const char *));
extern void decl_clean_up(void);

void mainloop(const char * localDir, const char * installDir)
{
    hname = "NetHack"; /* used for syntax messages */

    g_localDir = std::string(localDir);
    g_localDir += "\\";

    g_installDir = std::string(installDir);
    g_installDir += "\\";

    fqn_prefix[HACKPREFIX] = (char *) g_installDir.c_str();
    fqn_prefix[LEVELPREFIX] = (char *) g_localDir.c_str();
    fqn_prefix[SAVEPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[BONESPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[DATAPREFIX] = (char *) g_installDir.c_str();
    fqn_prefix[SCOREPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[LOCKPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[SYSCONFPREFIX] = (char *) g_installDir.c_str();
    fqn_prefix[CONFIGPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[TROUBLEPREFIX] = (char *)g_localDir.c_str();

    copy_to_local(g_defaultsFileName, true);
    rename_save_files();

    g_nethackOptionsFilePath = g_localDir;
    g_nethackOptionsFilePath += g_nethackOptionsFileName;

    g_defaultsFilePath = Nethack::g_localDir;
    g_defaultsFilePath += g_defaultsFileName;

    g_guidebookFilePath = Nethack::g_installDir;
    g_guidebookFilePath += g_guidebookFileName;

    Nethack::g_options.Load(g_nethackOptionsFilePath);

    if (setjmp(Nethack::g_mainLoopJmpBuf) == 0) {
        choose_windows(DEFAULT_WINDOW_SYS);

        initoptions();

        int argc = 1;
        char * argv[1] = { "nethack" };

        init_nhwindows(&argc, argv);
        // TODO: we need to clear text grid for now since init_nhwindows() will draw banner
        //       we will have the same banner on splash screen making this uncessary
        Nethack::g_textGrid.Clear();
        display_gamewindows();

        assert(iflags.window_inited);

        // TODO: should we use panic here?
        if (!iflags.window_inited) exit(0);

        bool bPlay = main_menu();

        exit_nhwindows((char *)0);

        if (bPlay) {
            sys_early_init();

            // TODO(bhouse): should we have cleared during exit_nhwindows
            raw_clear_screen();

            char failbuf[BUFSZ];
            if (!validate_prefix_locations(failbuf)) {
                raw_printf("Some invalid directory locations were specified:\n\t%s\n",
                    failbuf);
                nethack_exit(EXIT_FAILURE);
            }

            bool resuming = uwpmain(localDir, installDir);

            moveloop(resuming);
        }
    }

    decl_clean_up();
}


} // extern "C"
