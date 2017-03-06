/* NetHack 3.6	uwp.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#include "uwp.h"
#include "..\..\win\uwp\winuwp.h"

using namespace Nethack;

extern "C"  {

#ifndef HANGUPHANDLING
#error HANGUPHANDLING must be defined
#endif

#ifdef HOLD_LOCKFILE_OPEN
#error HOLD_LOCKFILE_OPEN should not be defined
#endif

/* clear the raw output area in preparation for raw output */
static void raw_clear_screen(void)
{
    g_textGrid.Clear();
}

/* erase all level files returning failure if we are
 * unable to erase the level 0 file.
 */
static bool erase_save_files()
{
    register int i;

    for (i = 1; i <= MAXDUNGEON * MAXLEVEL + 1; i++) {
        /* try to remove all */
        set_levelfile_name(lock, i);
        (void)unlink(fqname(lock, LEVELPREFIX, 0));
    }

    set_levelfile_name(lock, 0);

    if (unlink(fqname(lock, LEVELPREFIX, 0)))
        return false;

    return true;
}

/* Check if there is a game in progress and recover  */
static void check_game_in_progress()
{
    register int fd;
    int fcmask = FCMASK;
    char tbuf[BUFSZ];
    const char *fq_lock;

    Sprintf(tbuf, "%s", fqname(lock, LEVELPREFIX, 0));
    set_levelfile_name(lock, 0);
    fq_lock = fqname(lock, LEVELPREFIX, 1);

    if ((fd = open(fq_lock, 0)) == -1) {

        if (errno == ENOENT)
            return; /* no such file */

        uwp_error("Unexpected failure.  Cannot open %s", fq_lock);
    }

    (void)nhclose(fd);

    if (yn("There are files from a game in progress under your name. Recover?")) {
        if (!recover_savefile()) {
            if (yn("Recovery failed.  Continue by removing corrupt saved files?")) {
                if (!erase_save_files()) {
                    uwp_error("Cound not remove corrupt saved files.");
                }
            } else {
                uwp_error("Can not continue with a game in progress.");
            }
        }
    } else {
        uwp_error("Cannot start a new game with a game in progress.");
    }
}

/* create the checkpoint file (aka level 0 file, lock file) */
static void create_checkpoint_file()
{
    /* If checkpointing is turned on then the matching
     * pid is expected to be stored in the file.
     */

    int fd = create_levelfile(0, (char *)0);

    if (fd < 0) {
        uwp_error("Unable to create level 0 save file.");
    }

    hackpid = GetCurrentProcessId();
    write(fd, (genericptr_t)&hackpid, sizeof(hackpid));
    nhclose(fd);
}

/* verify we can open existing record file or create
 * a new record file.
 */
static void verify_record_file()
{
    const char *fq_record;
    int fd;

    char tmp[PATHLEN];

    Strcpy(tmp, RECORD);
    fq_record = fqname(RECORD, SCOREPREFIX, 0);

    if ((fd = open(fq_record, O_RDWR)) < 0) {
        /* try to create empty record */
        if ((fd = open(fq_record, O_CREAT | O_RDWR, S_IREAD | S_IWRITE))
            < 0) {
            uwp_warn("Unable to create record file");
        }
        else
            (void)nhclose(fd);
    }
    else /* open succeeded */
        (void)nhclose(fd);
}

/* kbhit is called by the engine's moveloop to determine if
 * there is any available input
 */
int kbhit(void)
{
    /* we checking for input -- flush our output */
    uwp_render_windows();
    return !g_eventQueue.Empty();
}

/* win32_abort() is called as part of panic processing */
void
win32_abort()
{
#if _DEBUG
    __debugbreak();
#endif

    abort();
}

/* immediately exit the nethack engine. if hang up processing is not in 
 * progress, then prompt the user that they are exiting. the reason for
 * the exit should already be displayed.
 */
void nethack_exit(int result)
{
    if(!program_state.done_hup) {
        if(iflags.window_inited) {
            putstr(BASE_WINDOW, 0, "Hit <ENTER> to exit.");
        } else {
            raw_printf("Hit <ENTER> to exit.");
        }

        uwp_wait_for_return();
    }

    longjmp(g_mainLoopJmpBuf, -1);
}

static HANDLE ffhandle = (HANDLE)0;
static WIN32_FIND_DATAA ffd;

/* findfirst() is called by files.c to iterate over the files of a directory */
int findfirst(char *path)
{
    if (ffhandle) {
        FindClose(ffhandle);
        ffhandle = (HANDLE)0;
    }
    ffhandle = FindFirstFileExA(path, FindExInfoStandard, &ffd, FindExSearchNameMatch, NULL, 0);
    return (ffhandle == INVALID_HANDLE_VALUE) ? 0 : 1;
}

/* findnext() is called by files.c to iterate over the files of a directory */
int findnext(void)
{
    return FindNextFileA(ffhandle, &ffd) ? 1 : 0;
}

/* foundfile_buffer() is used by files.c while iterating over the files of a directory */
char * foundfile_buffer()
{
    return &ffd.cFileName[0];
}

/* Delay is needed by files.c when locking a file. */
void Delay(int ms)
{
    Sleep(ms);
}

/* ensure the given string ends in a slash.  Note, this is a 
 * dangerous call since the slash is added in place and the
 * caller is responsible for ensuring there is sufficient
 * storage for the slash.  This is called by files.c.
 */
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

#define MAX_OVERRIDES 256
static unsigned char key_overrides[MAX_OVERRIDES];

/* options.c calls map_subkeyvalue to set option based key overrides.
 * Note, this is currently not fully implemented
 */
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

/* called when windowing base options are configured */
void
nttty_preference_update(const char * pref)
{
    // do nothing
}

/* map a scan code event to a character */
char MapScanCode(const Event & e)
{
    assert(e.m_type == Event::Type::ScanCode);

    char c = 0;

    if (!e.m_alt) {
        if (e.m_scanCode >= ScanCode::Home && e.m_scanCode <= ScanCode::Delete) {
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

            int i = (int) e.m_scanCode - (int)ScanCode::Home;

            if (e.m_shift) {
                c = pad[i].shift;
            } else if (e.m_control) {
                c = pad[i].control;
            } else {
                c = pad[i].normal;
            }
        }
    } else {
        static const int scanToChar[(int)ScanCode::Count] = {
            0,  0,
            '1' | 0x80, '2' | 0x80, '3' | 0x80,  '4' | 0x80,  '5' | 0x80,
            '6' | 0x80,  '7' | 0x80,  '8' | 0x80,  '9' | 0x80,  '0' | 0x80,
            0,  0,  0,  0,
            'q' | 0x80,  'w' | 0x80, 'e' | 0x80,  'r' | 0x80, 't' | 0x80,
            'y' | 0x80, 'u' | 0x80, 'i' | 0x80,  'o' | 0x80,  'p' | 0x80,
            0, 0, 0, 0,
            'a' | 0x80, 's' | 0x80, 'd' | 0x80, 'f' | 0x80, 'g' | 0x80,
            'h' | 0x80,  'j' | 0x80, 'k' | 0x80, 'l' | 0x80,
            0, 0, 0, 0, 0,
            'z' | 0x80, 'x' | 0x80, 'c' | 0x80, 'v' | 0x80, 'b' | 0x80, 'n' | 0x80, 'm' | 0x80,
            0, 0, '?' | 0x80, 0
        };

        assert(((int) e.m_scanCode >= 0) && ((int) e.m_scanCode < (int) ScanCode::Count));

        if (e.m_scanCode >= ScanCode::Unknown && e.m_scanCode < ScanCode::Count) {
            c = (char) scanToChar[(int)e.m_scanCode];
            unsigned char uc = c & ~0x80;
            if (e.m_shift && isalpha(uc)) {
                c = toupper(uc) | 0x80;
            }
        }
        
    }

    return c;
}

/* get a character of input from the input stream.  if we were hung up
 * then always return ESCAPE. 
 */
int raw_getchar()
{
    if (program_state.done_hup)
        return kEscape;

    /* we checking for input -- flush our output */
    uwp_render_windows();

    Event e;

    while (e.m_type == Event::Type::Undefined ||
           (e.m_type == Event::Type::ScanCode && MapScanCode(e) == 0) ||
           (e.m_type == Event::Type::Mouse)) {
        e = g_eventQueue.PopFront();
    }

    if (e.m_type == Event::Type::ScanCode)
        return MapScanCode(e);
    else  {
        assert(e.m_type == Event::Type::Char);
        return e.m_char;
    }
}

/* error needs to be provided due to a few spots in the engine which call it */
void error(const char * s, ...)
{
    va_list the_args;
    va_start(the_args, s);
    uwp_error(s, the_args);
    va_end(the_args);
}

/* report error to the user and exit the game.  this may be called when the
 * windowing system is not yet initialized.
 */
void uwp_error(const char * s, ...)
{
    va_list the_args;
    char buf[BUFSZ];
    va_start(the_args, s);

    (void)vsprintf(buf, s, the_args);

    if(iflags.window_inited) {
        clear_nhwindow(BASE_WINDOW);
        putstr(BASE_WINDOW, 0, buf);
    } else {
        raw_clear_screen();
        raw_printf(buf);
    }

    va_end(the_args);
    nethack_exit(EXIT_FAILURE);
}

/* warn the player without exiting the game.  this may be called
 * when the windowing system is not yet initialized
 */
void uwp_warn(const char * s, ...)
{
    va_list the_args;
    char buf[BUFSZ];
    va_start(the_args, s);

    (void)vsprintf(buf, s, the_args);

    if (iflags.window_inited) {
        clear_nhwindow(BASE_WINDOW);
        putstr(BASE_WINDOW, 0, buf);
        putstr(BASE_WINDOW, 0, "Hit <ENTER> to continue.");
    }
    else {
        raw_clear_screen();
        raw_printf(buf);
        raw_printf("Hit <ENTER> to continue.");
    }

    va_end(the_args);
    uwp_wait_for_return();
}

/* check whether a given file exits */
static bool file_exists(std::string & filePath)
{
    std::ifstream f(filePath.c_str());
    return f.good();
}

/* rename the file if it exists */
static void rename_file(const char * from, const char * to)
{
    std::string toPath(to);

    if (file_exists(toPath))
        return;

    rename(from, to);
}

/* copy the give file from the install directory to the
 * local directory
 */
static void copy_to_local(std::string & fileName, bool onlyIfMissing)
{
    std::string localPath = g_localDir;
    localPath += fileName;

    std::string installPath = g_installDir;
    installPath += fileName;

    if (onlyIfMissing && file_exists(localPath))
        return;

    FILE * dstFp = fopen(localPath.c_str(), "wb+");
    assert(dstFp != NULL);

    if(dstFp != NULL) {
        FILE * srcFp = fopen(installPath.c_str(), "rb");
        assert(srcFp != NULL);
        if(srcFp != NULL) {
            int failure = fseek(srcFp, 0, SEEK_END);
            assert(!failure);
            if(!failure) {
                size_t fileSize = ftell(srcFp);
                rewind(srcFp);
                char * data = (char *)malloc(fileSize);
                assert(data != NULL);
                if(data != NULL) {
                    int readBytes = fread(data, 1, fileSize, srcFp);
                    assert(readBytes == fileSize);
                    if(readBytes == fileSize) {
                        int writeBytes = fwrite(data, 1, fileSize, dstFp);
                        assert(writeBytes == fileSize);
                    }
                    free(data);
                }
            }
            fclose(srcFp);
        }
        fclose(dstFp);
    }
}

/* determine whether the given font family name can be used for rendering.
 * it must be among the known font families and must be monospaced.
 */
static bool validate_font_map(std::string & fontFamilyName)
{
    bool fontIsValid = false;

    if (g_fontCollection.m_fontFamilies.count(fontFamilyName) != 0)
    {
        auto & font = g_fontCollection.m_fontFamilies[fontFamilyName];
        if (!font.m_fonts.begin()->second.m_monospaced) {
            uwp_warn("font_map font '%s' must be monospaced.", fontFamilyName.c_str());
        } else {
            fontIsValid = true;
        }
    }
    else {
        uwp_warn("unable to find font_map font '%s'", fontFamilyName.c_str());
    }
    return fontIsValid;
}

/* process iflags.wc_font_map option.  if the font is valid
 * use that font otherwise use the default font
 */
static void process_font_map()
{
    std::string fontFamilyName;

    if (iflags.wc_font_map) {
        fontFamilyName = std::string(iflags.wc_font_map);
        if (!validate_font_map(fontFamilyName))
            fontFamilyName = g_defaultFontMap;
    } else {
        fontFamilyName = g_defaultFontMap;
    }

    g_textGrid.SetFontFamilyName(fontFamilyName);
}

/* allow the user to add to NETHACKOPTIONS */
static void add_option()
{
    clear_nhwindow(BASE_WINDOW);

    char buf[BUFSZ];
    getlin("Option:", buf);

    if(!buf[0]) return;

    // TODO(bhouse) It would be better if validateoptons caused event
    //              for setting font
    std::string option(buf);
    if (option.compare(0, 9, "font_map:") == 0) {
        std::string fontFamilyName = option.substr(9, std::string::npos);
        if (!validate_font_map(fontFamilyName))
            return;
    }

    if(validateoptions(buf, FALSE)) {
        g_options.m_options.push_back(option);
        g_options.Store();
        uwp_init_options();
    }
}

/* allow the user to remove one or more of the NETHACKOPTIONS */
static void remove_options()
{
    clear_nhwindow(BASE_WINDOW);

    winid menu = create_nhwindow(NHW_MENU);
    start_menu(menu);

    anything any = zeroany;

    add_menu(menu, NO_GLYPH, &any, 0, 0, ATR_NONE, "NETHACKOPTIONS", MENU_UNSELECTED);
    add_menu(menu, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);

    for(size_t i = 0; i < g_options.m_options.size(); i++) {
        auto & option = g_options.m_options[i];
        any.a_int = i + 1;
        add_menu(menu, NO_GLYPH, &any, 0, 0, ATR_NONE, option.c_str(), MENU_UNSELECTED);
    }

    end_menu(menu, "Pick options to remove");

    menu_item *picks = NULL;
    int count = select_menu(menu, PICK_ANY, &picks);
    destroy_nhwindow(menu);

    std::vector<bool> removals(g_options.m_options.size(), false);
    for(int i = 0; i < count; i++)
        removals[picks[i].item.a_int-1] = true;

    for(int i = g_options.m_options.size() - 1; i >= 0; i--)
        if(removals[i]) g_options.m_options.erase(g_options.m_options.begin() + i);

    if(count > 0) {
        g_options.Store();
        uwp_init_options();
    }

}

/* allow the user to add/remove from NETHACKOPTIONS. */
static void change_options()
{
    bool done = false;
    while (!done) {

        clear_nhwindow(BASE_WINDOW);

        std::string optionsString = "[ ]";
        optionsString += g_options.GetString();

        winid menu = create_nhwindow(NHW_MENU);
        start_menu(menu);

        anything any = zeroany;

        for(auto & option : g_options.m_options) {
            add_menu(menu, NO_GLYPH, &any, 0, 0, ATR_NONE, option.c_str(), MENU_UNSELECTED);
        }

        const char * items[] = {
            "[ ]",
            "[a]Add Options",
            "[b]Remove Options",
        };

        for (int i = 0; i < SIZE(items); i++) {
            any.a_int = items[i][1] == ' ' ? 0 : items[i][1];
            add_menu(menu, NO_GLYPH, &any, 0, 0, ATR_NONE, &items[i][3], MENU_UNSELECTED);
        }

        end_menu(menu, "Change NETHACKOPTIONS");

        menu_item *pick = NULL;
        int count = select_menu(menu, PICK_ONE, &pick);
        destroy_nhwindow(menu);

        if (count == 1) {
            switch(pick->item.a_int) {
            case 'a': add_option(); break;
            case'b': remove_options(); break;
            }
        } else       
            done = true;

        free((genericptr_t)pick);
    }
}

/* put up a menu allowing the user to change the font that will be
 * used.  if a change is made, that choice is reflected in the
 * NETHACKOPTIONS.
 */
static void change_font(void)
{
    clear_nhwindow(BASE_WINDOW);

    winid menu = create_nhwindow(NHW_MENU);
    start_menu(menu);

    anything any = zeroany;

    for(auto & fontFamily : g_fontCollection.m_fontFamilies) {
        if (fontFamily.second.m_fonts.begin()->second.m_monospaced) {
            any.a_void = (void *)&fontFamily.first;
            add_menu(menu, NO_GLYPH, &any, 0, 0, ATR_NONE, fontFamily.first.c_str(), MENU_UNSELECTED);
        }
    }

    end_menu(menu, "Pick font");

    menu_item *pick = NULL;
    int count = select_menu(menu, PICK_ONE, &pick);
    destroy_nhwindow(menu);

    if(count == 1) {
        std::string fontFamilyName = *((std::string *) pick->item.a_void);

        // remove any options for font_map
        g_options.RemoveOption(std::string("font_map"));

        // add font_map option for picked font
        std::string option = "font_map:";
        option += fontFamilyName;
        g_options.m_options.push_back(option);
        g_options.Store();

        // set font into grid
        g_textGrid.SetFontFamilyName(fontFamilyName);
    }

    free((genericptr_t)pick);
}

/* every port must provide support for getenv.  We redefine
 * getenv to uwp_getenv in uwpconf.h and support it here.  The
 * only environment variable we support is NETHACKOPTIONS which is
 * set via the main menu.
 */
char * uwp_getenv(const char * env)
{
    if(strcmp(env, "NETHACKOPTIONS") == 0) {
        static std::string options;
        options = g_options.GetString();
        return (char *)options.c_str();
    }
    return NULL;
}

/* prompt the user to pick a file name and location.  if the user picks,
 * a copy of file from the given filePath is made to the picked name and location.
 */
static void save_file(std::string & filePath)
{
    std::string fileExtension = filePath.substr(filePath.rfind('.'), std::string::npos);
    std::string fileName = filePath.substr(filePath.rfind('\\') + 1, filePath.rfind('.') - filePath.rfind('\\') - 1);

    std::string readText;
    std::fstream input(filePath.c_str(), std::fstream::in | std::fstream::binary);
    if(input.is_open()) {
        input.seekg(0, std::ios::end);
        size_t size = (size_t)input.tellg();
        input.seekg(0, std::ios::beg);
        std::vector<char> bytes(size);
        input.read(bytes.data(), size);
        input.close();
        readText = std::string(bytes.data(), size);

        Platform::String ^ fileText = to_platform_string(readText);
        Platform::String ^ fileNameStr = to_platform_string(fileName);
        Platform::String ^ extensionStr = to_platform_string(fileExtension);
        FileHandler::s_instance->SaveFilePicker(fileText, fileNameStr, extensionStr);
    } else {
        uwp_error("Unable to open %s%s", fileName.c_str(), fileExtension.c_str());
    }

}

/* prompt the user to select a file.  if the file is picked, then copy
 * its contents to the file path given.
 */
void load_file(std::string & filePath)
{
    std::string fileExtension = filePath.substr(filePath.rfind('.'), std::string::npos);
    Platform::String ^ extensionStr = to_platform_string(fileExtension);
    Platform::String ^ fileText = FileHandler::s_instance->LoadFilePicker(extensionStr);
    if (fileText != nullptr)  {
        std::string writeText = to_string(fileText);
        std::fstream output(filePath.c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);
        if (output.is_open())  {
            output.write(writeText.c_str(), writeText.length());
            output.close();
        }
    }
}

/* copy the defaults file from the installation location */
static void reset_defaults_file(void)
{
    copy_to_local(g_defaultsFileName, false);
    clear_nhwindow(BASE_WINDOW);

    puts("Reset complete. Hit <Enter>");
    uwp_wait_for_return();
}

static void unit_test_yn_function()
{
    int result;

    g_messageWindow.Init();

    g_testInput.push_back(TestInput('y', "test 1? [yn#aqB] (y) "));
    result = yn_function("test 1?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'y');
    assert(strcmp("test 1? [yn#aqB] (y) y", g_messageWindow.m_toplines.c_str()) == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("test 1? [yn#aqB] (y) y") == 0);

    g_testInput.push_back(TestInput('b', "test 2? [yn#aqB] (y) "));
    result = yn_function("test 2?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'b');
    assert(strcmp("test 2? [yn#aqB] (y) b", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('B', "test 3? [yn#aqB] (y) "));
    result = yn_function("test 3?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'B');
    assert(strcmp("test 3? [yn#aqB] (y) B", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('n', "test 4? [yn#aqB] (y) "));
    result = yn_function("test 4?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'n');
    assert(strcmp("test 4? [yn#aqB] (y) n", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('1', "test 5? [yn#aqB] (y) ", "test 5? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput('0', "test 5? [yn#aqB] (y) #1", "test 5? [yn#aqB] (y) #1"));
    g_testInput.push_back(TestInput('5', "test 5? [yn#aqB] (y) #10", "test 5? [yn#aqB] (y) #10"));
    g_testInput.push_back(TestInput('\b', "test 5? [yn#aqB] (y) #105", "test 5? [yn#aqB] (y) #105"));
    g_testInput.push_back(TestInput('\n', "test 5? [yn#aqB] (y) #10", "test 5? [yn#aqB] (y) #10"));
    result = yn_function("test 5?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == '#');
    assert(yn_number == 10);
    assert(strcmp("test 5? [yn#aqB] (y) #10", g_messageWindow.m_toplines.c_str()) == 0);

    g_testInput.push_back(TestInput('#', "test 6? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput('1', "test 6? [yn#aqB] (y) #"));
    g_testInput.push_back(TestInput('\b', "test 6? [yn#aqB] (y) #1"));
    g_testInput.push_back(TestInput('\n', "test 6? [yn#aqB] (y) #"));
    result = yn_function("test 6?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'n');
    assert(strcmp("test 6? [yn#aqB] (y) #n", g_messageWindow.m_toplines.c_str()) == 0);

    iflags.prevmsg_window = 's';
    g_testInput.push_back(TestInput(kControlP, "test 7? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kControlP, "test 7? [yn#aqB] (y) ", "test 6? [yn#aqB] (y) #n"));
    g_testInput.push_back(TestInput(kControlP, "test 7? [yn#aqB] (y) ", "test 5? [yn#aqB] (y) #10"));
    g_testInput.push_back(TestInput(kEscape, "test 7? [yn#aqB] (y) ", "test 4? [yn#aqB] (y) n"));
    g_testInput.push_back(TestInput(kEscape, "test 7? [yn#aqB] (y) ", "test 7? [yn#aqB] (y) "));
    result = yn_function("test 7?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'q');
    assert(strcmp("test 7? [yn#aqB] (y) q", g_messageWindow.m_toplines.c_str()) == 0);

    iflags.prevmsg_window = 'f';
    g_testInput.push_back(TestInput(kControlP, "test 8? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kSpace, "test 8? [yn#aqB] (y) ", NULL, []() {
        auto line = g_textGrid.ReadScreen(40, 0);  
        assert(line.compare(" Message History") == 0);
        line = g_textGrid.ReadScreen(40, 9);  
        assert(line.compare(" test 8? [yn#aqB] (y) ") == 0);
        line = g_textGrid.ReadScreen(40, 10); 
        assert(line.compare(" --More--") == 0);
    }));
    g_testInput.push_back(TestInput(kEscape, "test 8? [yn#aqB] (y) ", "test 8? [yn#aqB] (y) "));
    result = yn_function("test 8?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'q');
    assert(g_messageWindow.m_toplines.compare("test 8? [yn#aqB] (y) q") == 0);
    assert(g_textGrid.IsCornerClear(40, 10));

    g_testInput.push_back(TestInput(kControlP, "test 9? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kEscape, "test 9? [yn#aqB] (y) "));
    g_testInput.push_back(TestInput(kEscape, "test 9? [yn#aqB] (y) "));
    result = yn_function("test 9?", "yn#aqB\033b", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'q');
    assert(g_messageWindow.m_toplines.compare("test 9? [yn#aqB] (y) q") == 0);
    assert(g_textGrid.IsCornerClear(40, 10));

    g_testInput.push_back(TestInput('x', "test 10? "));
    result = yn_function("test 10?", NULL, 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'x');
    assert(g_messageWindow.m_toplines.compare("test 10? x") == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("test 10? x") == 0);

    g_testInput.push_back(TestInput('Y', "test 11? [yn] (y) "));
    result = yn_function("test 11?", "yn", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'y');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 12?", "yn", 'y');
    assert(g_testInput.size() == 0);
    assert(result == 'n');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 13?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 14?", "abq", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'q');

    g_testInput.push_back(TestInput(kNewline));
    result = yn_function("test 15?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput(kCarriageReturn));
    result = yn_function("test 16?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 17?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput('5'));
    g_testInput.push_back(TestInput(kEscape));
    g_testInput.push_back(TestInput(kEscape));
    result = yn_function("test 18?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'a');

    g_testInput.push_back(TestInput('5'));
    g_testInput.push_back(TestInput(kNewline));
    result = yn_function("test 19?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == '#');
    assert(yn_number == 5);

    g_testInput.push_back(TestInput('c'));
    g_testInput.push_back(TestInput('b'));
    result = yn_function("test 20?", "ab", 'a');
    assert(g_testInput.size() == 0);
    assert(result == 'b');

    g_testInput.push_back(TestInput('5', "test 19? [#] (a) ", "test 19? [#] (a) "));
    g_testInput.push_back(TestInput('9', "test 19? [#] (a) #5", "test 19? [#] (a) #5"));
    g_testInput.push_back(TestInput('8', "test 19? [#] (a) #59", "test 19? [#] (a) #59"));
    g_testInput.push_back(TestInput('7', "test 19? [#] (a) #598", "test 19? [#] (a) #598"));
    g_testInput.push_back(TestInput('6', "test 19? [#] (a) #5987", "test 19? [#] (a) #5987"));
    g_testInput.push_back(TestInput('5', "test 19? [#] (a) #59876", "test 19? [#] (a) #59876"));
    g_testInput.push_back(TestInput('4', "test 19? [#] (a) #598765", "test 19? [#] (a) #598765"));
    g_testInput.push_back(TestInput('3', "test 19? [#] (a) #5987654", "test 19? [#] (a) #5987654"));
    g_testInput.push_back(TestInput('2', "test 19? [#] (a) #59876543", "test 19? [#] (a) #59876543"));
    g_testInput.push_back(TestInput('1', "test 19? [#] (a) #598765432", "test 19? [#] (a) #598765432"));
    g_testInput.push_back(TestInput('4', "test 19? [#] (a) ", "test 19? [#] (a) "));
    g_testInput.push_back(TestInput('2', "test 19? [#] (a) #4", "test 19? [#] (a) #4"));
    g_testInput.push_back(TestInput(kNewline, "test 19? [#] (a) #42", "test 19? [#] (a) #42"));
    result = yn_function("test 19?", "#", 'a');
    assert(g_testInput.size() == 0);
    assert(result == '#');
    assert(yn_number == 42);
}

static void unit_test_display_message_window()
{
    /* notes: the blocking flag to display_nhwindow() is ignored. */ 

    pline("Test 1");
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);  assert(line.compare("Test 1--More--") == 0);
    }));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("") == 0);

    pline("Test 2");
    pline("Test 2");
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("Test 2  Test 2--More--") == 0);
    }));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);
    assert(g_textGrid.ReadScreen(0, 0).compare("") == 0);

    pline("Test 3");
    g_testInput.push_back(TestInput(kEscape));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);

}

static void unit_test_putstr()
{
    std::string long_msg("012345678 012345678 012345678 012345678 012345678 01234567.");
    std::string two_line_msg("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678 01234567.");
    std::string three_line_msg(
        "012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678"
        " 012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678"
        " 012345678.");
    std::string long_word_msg("012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 test.");

    g_messageWindow.Init();

    /* test wrap */
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 01234567.--More--") == 0);
    }));
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678") == 0);
        line = g_textGrid.ReadScreen(0, 1);
        assert(line.compare("01234567.--More--")==0);
    }));
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678") == 0);
        line = g_textGrid.ReadScreen(0, 1);
        assert(line.compare("012345678 012345678 012345678 012345678 012345678 012345678 012345678 012345678") == 0);
        line = g_textGrid.ReadScreen(0, 2);
        assert(line.compare("012345678.--More--") == 0);
    }));
    g_testInput.push_back(TestInput(kSpace, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(0, 0);
        assert(line.compare("01234567890123456789012345678901234567890123456789012345678901234567890123456789") == 0);
        line = g_textGrid.ReadScreen(0, 1);
        assert(line.compare("0123456789 test.--More--") == 0);
    }));
    g_messageWindow.Putstr(0, long_msg.c_str());
    g_messageWindow.Putstr(0, two_line_msg.c_str());
    g_messageWindow.Putstr(0, three_line_msg.c_str());
    g_messageWindow.Putstr(0, long_word_msg.c_str());
    auto line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("")==0);

    g_testInput.push_back(TestInput(kEscape));
    g_messageWindow.Putstr(0, two_line_msg.c_str());
    assert(g_testInput.size() == 0);
    line = g_textGrid.ReadScreen(0, 1);
    assert(line.compare("01234567."));

    /* test accumulation */
    g_messageWindow.PrepareForInput();
    g_messageWindow.Putstr(0, "test");
    g_messageWindow.Putstr(0, "test");
    g_testInput.push_back(TestInput(kEscape));
    display_nhwindow(WIN_MESSAGE, TRUE);
    assert(g_testInput.size() == 0);
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("")==0);

    g_messageWindow.Putstr(0, "test");
    g_messageWindow.PrepareForInput();
    g_messageWindow.Putstr(0, "test");
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("test") == 0);
    g_messageWindow.PrepareForInput();
    g_messageWindow.Putstr(0, long_msg.c_str());
    g_testInput.push_back(TestInput(kEscape));
    g_messageWindow.Putstr(0, "012345678901234567.");
    line = g_textGrid.ReadScreen(0, 0);
    /* last message should not have been written to screen */
    assert(line.compare("") == 0);
    /* but m_toplines should still be accumulating the message */
    assert(g_messageWindow.m_toplines.compare("012345678901234567.") == 0);
    g_messageWindow.Putstr(0, "test.");

    assert(g_testInput.size() == 0);
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);
    assert(g_messageWindow.m_toplines.compare("012345678901234567.  test.") == 0);

    /* long message does not fit, should be on its own line */
    g_messageWindow.Putstr(0, long_msg.c_str());
    assert(g_messageWindow.m_toplines.compare(long_msg) == 0);

    /* test clear */
    g_messageWindow.PrepareForInput();
    g_messageWindow.Clear();
    g_messageWindow.Putstr(0, "test.");
    g_messageWindow.PrepareForInput();
    g_messageWindow.Clear();
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);
    g_messageWindow.Putstr(0, "test.");
    g_messageWindow.Putstr(0, "test.");
    line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("test.  test.") == 0);
    assert(g_testInput.size() == 0);

//    g_textGrid.Flush();
//    Sleep(5000);


}

static void unit_test_get_ext_cmd()
{
    g_messageWindow.Init();

    g_testInput.push_back(TestInput('l', "# ", "# ", NULL));
    g_testInput.push_back(TestInput('o', "# loot", "# loot", NULL));
    g_testInput.push_back(TestInput(kNewline, "# loot", "# loot", NULL));
    int cmd = get_ext_cmd();
    assert(strcmp("loot", extcmdlist[cmd].ef_txt) == 0);
    assert(g_testInput.size() == 0);

    iflags.prevmsg_window = 'f';
    g_testInput.push_back(TestInput('j', "# ", "# ", NULL));
    g_testInput.push_back(TestInput(kControlP, "# jump", "# jump", NULL));
    g_testInput.push_back(TestInput(kEscape, NULL, NULL, []() {
        auto line = g_textGrid.ReadScreen(40, 0);
        assert(line.compare(" Message History") == 0);
        line = g_textGrid.ReadScreen(40, 2);
        assert(line.compare(" # loot") == 0);
        line = g_textGrid.ReadScreen(40, 3);
        assert(line.compare(" # jump") == 0);
        line = g_textGrid.ReadScreen(40, 4);
        assert(line.compare(" --More--") == 0);
    }));
    g_testInput.push_back(TestInput(kNewline, "# jump", "# jump", NULL));
    cmd = get_ext_cmd();
    assert(cmd != -1 && strcmp("jump", extcmdlist[cmd].ef_txt) == 0);
    assert(g_testInput.size() == 0);

    iflags.prevmsg_window = 's';
    g_testInput.push_back(TestInput('d', "# ", "# ", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# jump", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kNewline, "# dip", "# jump", NULL));
    g_testInput.push_back(TestInput(kNewline, "# dip", "# dip",  NULL));
    cmd = get_ext_cmd();
    assert(cmd != -1 && strcmp("dip", extcmdlist[cmd].ef_txt) == 0);
    assert(g_testInput.size() == 0);
    auto line = g_textGrid.ReadScreen(0, 0);
    assert(line.compare("") == 0);

    iflags.prevmsg_window = 's';
    g_testInput.push_back(TestInput('d', "# ", "# ", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kControlP, "# dip", "# dip", NULL));
    g_testInput.push_back(TestInput(kNewline, "# dip", "# jump", NULL));
    g_testInput.push_back(TestInput(kEscape, "# dip", "# dip", NULL));
    cmd = get_ext_cmd();
    assert(cmd == -1);
    assert(g_testInput.size() == 0);

}

static void unit_test_message_window()
{
    unit_test_putstr();
    unit_test_yn_function();
    unit_test_display_message_window();
    unit_test_get_ext_cmd();
}

static void unit_tests()
{
    unit_test_message_window();
}

/* using the windowing system, show a menu that allows the player
 * to perform various actions prior to playing a game.
 */
static bool main_menu(void)
{
    /* we will using windowing so setup windowing system */
    init_nhwindows(NULL, NULL);

    display_gamewindows();

    unit_tests();

#if 0

#if 0
    putstr(BASE_WINDOW, 0, "0123456789012345678901234567890123456789012345678901234567890123456789012345678");
    putstr(BASE_WINDOW, 0, "0123456789012345678901234567890123456789012345678901234567890123456789012345678");
    putstr(BASE_WINDOW, 0, "0123456789012345678901234567890123456789012345678901234567890123456789012345678");
    putstr(BASE_WINDOW, 0, "0123456789012345678901234567890123456789012345678901234567890123456789012345678");
    putstr(BASE_WINDOW, 0, "0123456789012345678901234567890123456789012345678901234567890123456789012345678");
#endif

#if 0
    winid tmpwin = create_nhwindow(NHW_TEXT);
    putstr(tmpwin, 0, "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");
    putstr(tmpwin, 0, "");
    putstr(tmpwin, 0, "0123456789012345678901234567890123456789012345678901234567890123456789 0123456789");
    putstr(tmpwin, 0, "0123456789012345678901234567890123456789012345678901234567890123456789\n0123456789");
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
#endif

#if 0
    tmpwin = create_nhwindow(NHW_TEXT);
    for (int i = 0; i < 25; i++)
    {
        char line[256];
        sprintf(line, "Line %d", i);
        putstr(tmpwin, 0, line);
    }
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
#endif


#if 0
    int picked;
    for (int i = 0; i < 10; i++) {
        picked = message_menu('a', 1, "pick object a 0123456789012345678901 23456789012345678901234 5678901234567 8901234567890123456789");
        clear_nhwindow(BASE_WINDOW);
        char line[256];
        sprintf(line, "try %d picked %d", i, picked);
        curs(BASE_WINDOW, 1, 3);
        putstr(BASE_WINDOW, 0, line);
    }
#endif

#endif

    assert(iflags.window_inited);
    if (!iflags.window_inited) {
        uwp_error("Windowing system failed to initialize");
    }

    // TODO(bhouse): Need to review use of BASE_WINDOW.  Really like to have this code be
    //               windowing system agnostic.  Use and knowledge of BASE_WINDOW breaks
    //               that goal.
    clear_nhwindow(BASE_WINDOW);

    bool play = false;
    bool done = false;
    while (!done && !play) {
        // TODO(bhouse): this can be removed when we switch to using nh menus for all actions
        g_textGrid.Clear();

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
            "[ ]",
            "[b]Save Guidebook.txt to file",
            "[c]Save License.txt to file",
            "[d]Change font",
            "[e]Change NETHACKOPTIONS",
            "[f]Save defaults.nh to file",
            "[g]Load defaults.nh from file",
            "[h]Reset defaults.nh"
        };

        winid menu = create_nhwindow(NHW_MENU);
        start_menu(menu);

        anything any = zeroany;

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
            case 'b': save_file(g_guidebookFilePath); break;
            case 'c': save_file(g_licenseFilePath); break;
            case 'd': change_font(); break;
            case 'e': change_options(); break;
            case 'f': save_file(g_defaultsFilePath); break;
            case 'g': load_file(g_defaultsFilePath); initoptions();  break;
            case 'h': reset_defaults_file(); break;
            }
        } else if (count == -1) {
            done = true;
        }

        free((genericptr_t)pick);
    }

    exit_nhwindows((char *)0);

    return play;
}

/* one time initialization.  this state must be constant state. this does
 * not include constant state managed (i.e. set) by the game engine.
 */
static void uwp_one_time_init(std::wstring & localDirW, std::wstring & installDirW)
{
    bool static initialized = false;

    if (initialized) return;

    g_localDir = std::string(localDirW.begin(), localDirW.end());
    g_localDir += "\\";

    g_installDir = std::string(installDirW.begin(), installDirW.end());
    g_installDir += "\\";

    g_nethackOptionsFilePath = g_localDir;
    g_nethackOptionsFilePath += g_nethackOptionsFileName;

    g_defaultsFilePath = g_localDir;
    g_defaultsFilePath += g_defaultsFileName;

    g_guidebookFilePath = g_installDir;
    g_guidebookFilePath += g_guidebookFileName;

    g_licenseFilePath = g_installDir;
    g_licenseFilePath += g_licenseFileName;

    fqn_prefix[HACKPREFIX] = (char *)g_installDir.c_str();
    fqn_prefix[LEVELPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[SAVEPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[BONESPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[DATAPREFIX] = (char *)g_installDir.c_str();
    fqn_prefix[SCOREPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[LOCKPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[SYSCONFPREFIX] = (char *)g_installDir.c_str();
    fqn_prefix[CONFIGPREFIX] = (char *)g_localDir.c_str();
    fqn_prefix[TROUBLEPREFIX] = (char *)g_localDir.c_str();

    char failbuf[BUFSZ];
    if (!validate_prefix_locations(failbuf)) {
        uwp_error("Some invalid directory locations were specified:\n\t%s",
            failbuf);
    }

    copy_to_local(g_defaultsFileName, true);
    copy_to_local(g_guidebookFileName, true);
    copy_to_local(g_licenseFileName, true);

    verify_record_file();

    g_options.Load(g_nethackOptionsFilePath);

    initialized = true;
}

/* Initialize options.  This can get called multiple times prior
 * to game start.
 */
static void uwp_init_options()
{
    g_textGrid.SetDefaultPalette();

    initoptions();

    process_font_map();

    if (!symset[PRIMARY].name) {
        load_symset("IBMGraphics_2", PRIMARY);
    }

    if (!symset[ROGUESET].name) {
        load_symset("RogueEpyx", ROGUESET);
    }
}

/* player has elected to play a game of nethack.
 * let play chose between resuming a game in progress
 * or start a new game and then play.
 */
static void 
uwp_play_nethack(void)
{
    sys_early_init();

    if (!dlb_init()) {
        uwp_error("dlb_init failure");
    }

    init_nhwindows(NULL, NULL);

    display_gamewindows();

    plnamesuffix();

    set_playmode(); /* sets plname to "wizard" for wizard mode */

   /* do not let player rename the character during role/race/&c selection */
    iflags.renameallowed = FALSE;

    /* encode player name into a file name.  Store the result to lock.
     * Lock is a global variable used to store the level files
     */

    (void)fname_encode(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.", '%',
        plname, lock, sizeof(lock));

    check_game_in_progress();
    create_checkpoint_file();

    /*
    *  Initialize the vision system.  This must be before mklev() on a
    *  new game or before a level restore on a saved game.
    */
    vision_init();

    boolean resuming = FALSE;
    int fd;

    if ((fd = restore_saved_game()) >= 0) {
        pline("Restoring save file...");
        mark_synch(); /* flush output */

        if (dorecover(fd)) {
            resuming = TRUE; /* not starting new game */
            if (discover)
                You("are in non-scoring discovery mode.");
            if (discover || wizard) {
                if (yn("Do you want to keep the save file?") == 'n')
                    (void) delete_savefile();
                else {
                    nh_compress(fqname(SAVEF, SAVEPREFIX, 0));
                }
            }
        }
    }

    if (!resuming) {
        player_selection();
        newgame();
        if (discover)
            You("are in non-scoring discovery mode.");
    }

    moveloop(resuming);

}

#ifdef PORT_HELP
void
port_help()
{
    /* display port specific help file */
    display_file(PORT_HELP, 1);
}
#endif /* PORT_HELP */

/* validate wizard mode if player has requested access to it.
 * This call is required by options.c.
 */
boolean
authorize_wizard_mode()
{
    if (!strcmp(plname, WIZARD_NAME))
        return TRUE;
    return FALSE;
}

/* main entry point of nethack engine thread.
 * this thread never exits (until process termination).
 */
void uwp_main(std::wstring & localDirW, std::wstring & installDirW)
{
    /* we must treat early long jumps as fatal to avoid endless error loops */
    bool jumpIsFatal = true;

    /* any error encountered within this thread will cause us to
     * long jump out.  this has the potential of leaving any
     * global or static state in a bad state.  becaues of this, we initialize
     * all global and static state to a good known state at the start
     */

    if (setjmp(g_mainLoopJmpBuf) == 0) {

        /* set the windowing system -- this is necessary for
         * raw printing to work which is used during warning and error
         * reporting 
         */
        choose_windows(DEFAULT_WINDOW_SYS);

        uwp_one_time_init(localDirW, installDirW);

        /* set engine initialization state */
        first_init();

        /* initialize options -- we do this here to ensure options
         * can influence all other systems (such as windowing)
         */
        uwp_init_options();

        /* show a main menu allowing the player to change some settings
         * prior to starting a game.
         */
        bool bPlay = main_menu();

        if (bPlay) {
            /* errors will no longer be considered fatal and thus will cause
             * us to long jump out
             */
            jumpIsFatal = false;

            uwp_play_nethack();
        }
    } else {
        if (jumpIsFatal) exit(EXIT_FAILURE);
    }

    final_cleanup();
}

void uwp_wait_for_return()
{
    while (1)
    {
        char c = pgetchar();

        if (c == kEscape || c == kNewline)
            break;
    }
}

} // extern "C"
