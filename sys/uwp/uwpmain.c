/* NetHack 3.6	uwpmain.c	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* NetHack may be freely redistributed.  See license for details. */

/* uwpmain.c - Universal Windows Platform NetHack */

#include "hack.h"
#include "wintty.h"
#include "dlb.h"

#include <assert.h>
#include <string.h>

#ifndef NO_SIGNAL
#include <signal.h>
#endif

#include <ctype.h>

#include <sys\stat.h>

#ifdef WIN32
#include "win32api.h" /* for GetModuleFileName */
#endif

char * orgdir = NULL;

STATIC_DCL void FDECL(process_options, (int argc, char **argv));
STATIC_DCL void NDECL(nhusage);

extern void FDECL(nethack_exit, (int));

#ifdef WIN32
extern int redirect_stdout;       /* from sys/share/pcsys.c */
extern int GUILaunched;
HANDLE hStdOut;
char *NDECL(exename);
char default_window_sys[] = "tty";
boolean NDECL(fakeconsole);
void NDECL(freefakeconsole);
#endif

#ifdef EXEPATH
STATIC_DCL char *FDECL(exepath, (char *));
#endif

void
verify_record_file()
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
            raw_printf("Warning: cannot write record %s", tmp);
            wait_synch();
        }
        else
            (void)nhclose(fd);
        }
    else /* open succeeded */
        (void)nhclose(fd);
}

boolean
uwpmain(const char * inLocalDir, const char * inInstallDir)
{
    int fd;
//    char *dir;
#if defined(WIN32) || defined(MSDOS)
    char *envp = NULL;
    char *sptr = NULL;
#endif
#if defined(WIN32)
    char fnamebuf[BUFSZ], encodedfnamebuf[BUFSZ];
#endif
#ifdef NOCWD_ASSUMPTIONS
    char failbuf[BUFSZ];
#endif
    boolean resuming = FALSE; /* assume new game */

#ifdef _MSC_VER
# ifdef DEBUG
                              /* set these appropriately for VS debugging */
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG); 
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
# endif
#endif

    hname = "NetHack"; /* used for syntax messages */

    choose_windows(default_window_sys);

    Strcpy(hackdir, inInstallDir);

    char * installDir = (char *)malloc(strlen(inInstallDir) + 2);
    char * localDir = (char *)malloc(strlen(inLocalDir) + 2);

    strcpy(installDir, inInstallDir);
    strcpy(localDir, inLocalDir);

    append_slash(installDir);
    append_slash(localDir);

    orgdir = installDir;
    fqn_prefix[HACKPREFIX] = installDir;
    fqn_prefix[LEVELPREFIX] = localDir;
    fqn_prefix[SAVEPREFIX] = localDir;
    fqn_prefix[BONESPREFIX] = localDir;
    fqn_prefix[DATAPREFIX] = installDir;
    fqn_prefix[SCOREPREFIX] = localDir;
    fqn_prefix[LOCKPREFIX] = localDir;
    fqn_prefix[SYSCONFPREFIX] = installDir;
    fqn_prefix[CONFIGPREFIX] = installDir;
    fqn_prefix[TROUBLEPREFIX] = localDir;

    {
        int fd;
        boolean have_syscf = FALSE;
#ifdef NOCWD_ASSUMPTIONS
        {

#if defined(WIN32) || defined(MSDOS)

            /* okay so we have the overriding and definitive locaton
            for sysconf, but only in the event that there is not a
            sysconf file there (for whatever reason), check a secondary
            location rather than abort. */

            /* Is there a SYSCF_FILE there? */
            fd = open(fqname(SYSCF_FILE, SYSCONFPREFIX, 0), O_RDONLY);
            if (fd >= 0) {
                /* readable */
                close(fd);
                have_syscf = TRUE;
            }

            if (!have_syscf) {
                /* No SYSCF_FILE where there should be one, and
                without an installer, a user may not be able
                to place one there. So, let's try somewhere else... */
                fqn_prefix[SYSCONFPREFIX] = fqn_prefix[0];

                /* Is there a SYSCF_FILE there? */
                fd = open(fqname(SYSCF_FILE, SYSCONFPREFIX, 0), O_RDONLY);
                if (fd >= 0) {
                    /* readable */
                    close(fd);
                    have_syscf = TRUE;
                }
            }

            /* user's home directory should default to this - unless
            * overridden */
            envp = nh_getenv("USERPROFILE");
            if (envp) {
                if ((sptr = index(envp, ';')) != 0)
                    *sptr = '\0';
                if (strlen(envp) > 0) {
                    fqn_prefix[CONFIGPREFIX] =
                        (char *)alloc(strlen(envp) + 2);
                    Strcpy(fqn_prefix[CONFIGPREFIX], envp);
                    append_slash(fqn_prefix[CONFIGPREFIX]);
                }
            }
#endif
        }
#endif
    }
#ifdef WIN32
    raw_clear_screen();
#endif
    initoptions();

#ifdef NOCWD_ASSUMPTIONS
    if (!validate_prefix_locations(failbuf)) {
        raw_printf("Some invalid directory locations were specified:\n\t%s\n",
            failbuf);
        nethack_exit(EXIT_FAILURE);
    }
#endif

    if (!hackdir[0])
        Strcpy(hackdir, orgdir);

    /*
    * It seems you really want to play.
    */

    if (!dlb_init()) {
        pline(
            "%s\n%s\n%s\n%s\n\nNetHack was unable to open the required file "
            "\"%s\".%s",
            copyright_banner_line(1), copyright_banner_line(2),
            copyright_banner_line(3), copyright_banner_line(4), DLBFILE,
            "\nAre you perhaps trying to run NetHack within a zip utility?");
        error("dlb_init failure.");
    }

    u.uhp = 1; /* prevent RIP on early quits */
    u.ux = 0;  /* prevent flush_screen() */

               /* chdir shouldn't be called before this point to keep the
               * code parallel to other ports.
               */
    
    verify_record_file();

    /* In 3.6.0, several ports process options before they init
    * the window port. This allows settings that impact window
    * ports to be specified or read from the sys or user config files.
    */

    iflags.use_background_glyph = FALSE;
    nttty_open(1);

#if defined(MSDOS) || defined(WIN32)
    /* Player didn't specify any symbol set so use IBM defaults */
    if (!symset[PRIMARY].name) {
        load_symset("IBMGraphics_2", PRIMARY);
    }
    if (!symset[ROGUESET].name) {
        load_symset("RogueEpyx", ROGUESET);
    }
#endif

    int argc = 1;
    char * argv[1] = { "nethack" };
    init_nhwindows(&argc, argv);

#ifdef WIN32
    toggle_mouse_support(); /* must come after process_options */
#endif

    /* strip role,race,&c suffix; calls askname() if plname[] is empty
    or holds a generic user name like "player" or "games" */
    plnamesuffix();
    clear_screen();

    set_playmode(); /* sets plname to "wizard" for wizard mode */
#if 0
                    /* unlike Unix where the game might be invoked with a script
                    which forces a particular character name for each player
                    using a shared account, we always allow player to rename
                    the character during role/race/&c selection */
    iflags.renameallowed = TRUE;
#else
                    /* until the getlock code is resolved, override askname()'s
                    setting of renameallowed; when False, player_selection()
                    won't resent renaming as an option */
    iflags.renameallowed = FALSE;
#endif

#if defined(PC_LOCKING)
    /* 3.3.0 added this to support detection of multiple games
    * under the same plname on the same machine in a windowed
    * or multitasking environment.
    *
    * That allows user confirmation prior to overwriting the
    * level files of a game in progress.
    *
    * Also prevents an aborted game's level files from being
    * overwritten without confirmation when a user starts up
    * another game with the same player name.
    */
    /* Obtain the name of the logged on user and incorporate
    * it into the name. */
    Sprintf(fnamebuf, "%s-%s", get_username(0), plname);
    (void)fname_encode(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.", '%',
        fnamebuf, encodedfnamebuf, BUFSZ);
    Sprintf(lock, "%s", encodedfnamebuf);
    /* regularize(lock); */ /* we encode now, rather than substitute */
    getlock();
#endif /* PC_LOCKING */

                      /* Set up level 0 file to keep the game state.
                      */
    fd = create_levelfile(0, (char *)0);
    if (fd < 0) {
        raw_print("Cannot create lock file");
    }
    else {
        hackpid = GetCurrentProcessId();
        write(fd, (genericptr_t)&hackpid, sizeof(hackpid));
        nhclose(fd);
    }

    /*
    *  Initialize the vision system.  This must be before mklev() on a
    *  new game or before a level restore on a saved game.
    */
    vision_init();

    display_gamewindows();

    /*
    * First, try to find and restore a save file for specified character.
    * We'll return here if new game player_selection() renames the hero.
    */
attempt_restore:
    if ((fd = restore_saved_game()) >= 0) {
#ifndef NO_SIGNAL
        (void) signal(SIGINT, (SIG_RET_TYPE)done1);
#endif
#ifdef NEWS
        if (iflags.news) {
            display_file(NEWS, FALSE);
            iflags.news = FALSE;
        }
#endif
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
        /* new game:  start by choosing role, race, etc;
        player might change the hero's name while doing that,
        in which case we try to restore under the new name
        and skip selection this time if that didn't succeed */
        if (!iflags.renameinprogress) {
            player_selection();
            if (iflags.renameinprogress) {
                /* player has renamed the hero while selecting role;
                discard current lock file and create another for
                the new character name */
#if 0 /* this needs to be reconciled with the getlock mess above... */
                delete_levelfile(0); /* remove empty lock file */
                getlock();
#endif
                goto attempt_restore;
            }
        }
        newgame();
        if (discover)
            You("are in non-scoring discovery mode.");
    }

#ifndef NO_SIGNAL
    (void) signal(SIGINT, SIG_IGN);
#endif
    return resuming;
}

STATIC_OVL void
process_options(argc, argv)
int argc;
char *argv[];
{
    int i;

    /*
    * Process options.
    */
    while (argc > 1 && argv[1][0] == '-') {
        argv++;
        argc--;
        switch (argv[0][1]) {
        case 'a':
            if (argv[0][2]) {
                if ((i = str2align(&argv[0][2])) >= 0)
                    flags.initalign = i;
            }
            else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2align(argv[0])) >= 0)
                    flags.initalign = i;
            }
            break;
        case 'D':
            wizard = TRUE, discover = FALSE;
            break;
        case 'X':
            discover = TRUE, wizard = FALSE;
            break;
#ifdef NEWS
        case 'n':
            iflags.news = FALSE;
            break;
#endif
        case 'u':
            if (argv[0][2])
                (void) strncpy(plname, argv[0] + 2, sizeof(plname) - 1);
            else if (argc > 1) {
                argc--;
                argv++;
                (void)strncpy(plname, argv[0], sizeof(plname) - 1);
            }
            else
                raw_print("Player name expected after -u");
            break;
#ifndef AMIGA
        case 'I':
        case 'i':
            if (!strncmpi(argv[0] + 1, "IBM", 3)) {
                load_symset("IBMGraphics", PRIMARY);
                load_symset("RogueIBM", ROGUESET);
                switch_symbols(TRUE);
            }
            break;
            /*	case 'D': */
        case 'd':
            if (!strncmpi(argv[0] + 1, "DEC", 3)) {
                load_symset("DECGraphics", PRIMARY);
                switch_symbols(TRUE);
            }
            break;
#endif
        case 'g':
            if (argv[0][2]) {
                if ((i = str2gend(&argv[0][2])) >= 0)
                    flags.initgend = i;
            }
            else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2gend(argv[0])) >= 0)
                    flags.initgend = i;
            }
            break;
        case 'p': /* profession (role) */
            if (argv[0][2]) {
                if ((i = str2role(&argv[0][2])) >= 0)
                    flags.initrole = i;
            }
            else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2role(argv[0])) >= 0)
                    flags.initrole = i;
            }
            break;
        case 'r': /* race */
            if (argv[0][2]) {
                if ((i = str2race(&argv[0][2])) >= 0)
                    flags.initrace = i;
            }
            else if (argc > 1) {
                argc--;
                argv++;
                if ((i = str2race(argv[0])) >= 0)
                    flags.initrace = i;
            }
            break;
#ifdef WIN32
        case 'w': /* windowtype */
            if (strncmpi(&argv[0][2], "tty", 3)) {
                nttty_open(1);
            }
            /*
            else {
            NHWinMainInit();
            }
            */
            choose_windows(&argv[0][2]);
            break;
#endif
        case '@':
            flags.randomall = 1;
            break;
        default:
            if ((i = str2role(&argv[0][1])) >= 0) {
                flags.initrole = i;
                break;
            }
            else
                raw_printf("\nUnknown switch: %s", argv[0]);
            /* FALL THROUGH */
        case '?':
            nhusage();
            nethack_exit(EXIT_SUCCESS);
        }
    }
}

STATIC_OVL void
nhusage()
{
    char buf1[BUFSZ], buf2[BUFSZ], *bufptr;

    buf1[0] = '\0';
    bufptr = buf1;

#define ADD_USAGE(s)                              \
    if ((strlen(buf1) + strlen(s)) < (BUFSZ - 1)) \
        Strcat(bufptr, s);

    /* -role still works for those cases which aren't already taken, but
    * is deprecated and will not be listed here.
    */
    (void)Sprintf(buf2, "\nUsage:\n%s [-d dir] -s [-r race] [-p profession] "
        "[maxrank] [name]...\n       or",
        hname);
    ADD_USAGE(buf2);

    (void)Sprintf(
        buf2, "\n%s [-d dir] [-u name] [-r race] [-p profession] [-[DX]]",
        hname);
    ADD_USAGE(buf2);
#ifdef NEWS
    ADD_USAGE(" [-n]");
#endif
#ifndef AMIGA
    ADD_USAGE(" [-I] [-i] [-d]");
#endif
    if (!iflags.window_inited)
        raw_printf("%s\n", buf1);
    else
        (void)printf("%s\n", buf1);
#undef ADD_USAGE
}

#ifdef PORT_HELP
#if defined(MSDOS) || defined(WIN32)
void
port_help()
{
    /* display port specific help file */
    display_file(PORT_HELP, 1);
}
#endif /* MSDOS || WIN32 */
#endif /* PORT_HELP */

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    if (!strcmp(plname, WIZARD_NAME))
        return TRUE;
    return FALSE;
}

#ifdef EXEPATH
#ifdef __DJGPP__
#define PATH_SEPARATOR '/'
#else
#define PATH_SEPARATOR '\\'
#endif

#ifdef WIN32
static char exenamebuf[PATHLEN];
extern HANDLE hConIn;
extern HANDLE hConOut;
boolean has_fakeconsole;

char *
exename()
{
    int bsize = PATHLEN;
    char *tmp = exenamebuf, *tmp2;

#ifdef UNICODE
    {
        TCHAR wbuf[PATHLEN * 4];
        GetModuleFileName((HANDLE)0, wbuf, PATHLEN * 4);
        WideCharToMultiByte(CP_ACP, 0, wbuf, -1, tmp, bsize, NULL, NULL);
    }
#else
    *(tmp + GetModuleFileName((HANDLE)0, tmp, bsize)) = '\0';
#endif
    tmp2 = strrchr(tmp, PATH_SEPARATOR);
    if (tmp2)
        *tmp2 = '\0';
    tmp2++;
    return tmp2;
}

#if 0
boolean
fakeconsole(void)
{
    if (!hStdOut) {
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);

        if (!hStdOut && !hStdIn) {
            /* Bool rval; */
            AllocConsole();
            AttachConsole(GetCurrentProcessId());
            /* 	rval = SetStdHandle(STD_OUTPUT_HANDLE, hWrite); */
            freopen("CON", "w", stdout);
            freopen("CON", "r", stdin);
        }
        has_fakeconsole = TRUE;
    }

    /* Obtain handles for the standard Console I/O devices */
    hConIn = GetStdHandle(STD_INPUT_HANDLE);
    hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
#if 0
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE)) {
        /* Unable to set control handler */
        cmode = 0; /* just to have a statement to break on for debugger */
    }
#endif
    return has_fakeconsole;
}
void freefakeconsole()
{
    if (has_fakeconsole) {
        FreeConsole();
    }
}
#endif
#endif

#define EXEPATHBUFSZ 256
char exepathbuf[EXEPATHBUFSZ];

char *
exepath(str)
char *str;
{
    char *tmp, *tmp2;
    int bsize;

    if (!str)
        return (char *)0;
    bsize = EXEPATHBUFSZ;
    tmp = exepathbuf;
#ifndef WIN32
    Strcpy(tmp, str);
#else
#ifdef UNICODE
    {
        TCHAR wbuf[BUFSZ];
        GetModuleFileName((HANDLE)0, wbuf, BUFSZ);
        WideCharToMultiByte(CP_ACP, 0, wbuf, -1, tmp, bsize, NULL, NULL);
    }
#else
    *(tmp + GetModuleFileName((HANDLE)0, tmp, bsize)) = '\0';
#endif
#endif
    tmp2 = strrchr(tmp, PATH_SEPARATOR);
    if (tmp2)
        *tmp2 = '\0';
    return tmp;
}
#endif /* EXEPATH */

/*pcmain.c*/
