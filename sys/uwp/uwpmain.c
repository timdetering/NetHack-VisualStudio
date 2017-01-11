/* NetHack 3.6	uwpmain.c	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

//#include "wintty.h"

#include "dlb.h"

#include <assert.h>
#include <string.h>

#include <ctype.h>

#include <sys\stat.h>

#ifdef WIN32
#include "win32api.h" /* for GetModuleFileName */
#endif

#include "common\uwpglobals.h"

#ifndef NOCWD_ASSUMPTIONS
#error NOCWD_ASSUMPTIONS must be defined
#endif

#ifndef NO_SIGNAL
#error NO_SIGNAL must be defined
#endif

char * orgdir = NULL;

STATIC_DCL void FDECL(process_options, (int argc, char **argv));
STATIC_DCL void NDECL(nhusage);

extern void FDECL(nethack_exit, (int));

extern int redirect_stdout;       /* from sys/share/pcsys.c */
extern int GUILaunched;
HANDLE hStdOut;
char *NDECL(exename);

char default_window_sys[] = DEFAULT_WINDOW_SYS;

boolean NDECL(fakeconsole);
void NDECL(freefakeconsole);
extern void NDECL(clear_screen);

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

extern void rename_file(const char * from, const char * to);

void rename_save_files()
{
    char *foundfile;
    const char *fq_save;

    const char * oldsave = "bhouse-*.NetHack-saved-game";

    fq_save = fqname(oldsave, SAVEPREFIX, 0);

    foundfile = foundfile_buffer();
    if (findfirst((char *)fq_save)) {
        do {
            char oldPath[512];
            char newname[512];

            strcpy(newname, "noname-");
            strcpy(&newname[7], &foundfile[7]);

            fq_save = fqname(foundfile, SAVEPREFIX, 0);
            strcpy(oldPath, fq_save);

            const char * newPath = fqname(newname, SAVEPREFIX, 0);

            rename_file(oldPath, newPath);

        } while (findnext());
    }
}

boolean
uwpmain(const char * inLocalDir, const char * inInstallDir)
{
    int fd;
    char *envp = NULL;
    char *sptr = NULL;

    char fnamebuf[BUFSZ], encodedfnamebuf[BUFSZ];
    char failbuf[BUFSZ];
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
    fqn_prefix[CONFIGPREFIX] = localDir;
    fqn_prefix[TROUBLEPREFIX] = localDir;

    raw_clear_screen();

    rename_save_files();

    initoptions();

    if (!validate_prefix_locations(failbuf)) {
        raw_printf("Some invalid directory locations were specified:\n\t%s\n",
            failbuf);
        nethack_exit(EXIT_FAILURE);
    }

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

    /* Player didn't specify any symbol set so use IBM defaults */
    if (!symset[PRIMARY].name) {
        load_symset("IBMGraphics_2", PRIMARY);
    }
    if (!symset[ROGUESET].name) {
        load_symset("RogueEpyx", ROGUESET);
    }

    int argc = 1;
    char * argv[1] = { "nethack" };
    init_nhwindows(&argc, argv);

    toggle_mouse_support(); /* must come after process_options */

    // load up the game windows before we start asking questions
    display_gamewindows();

    /* strip role,race,&c suffix; calls askname() if plname[] is empty
    or holds a generic user name like "player" or "games" */
    plnamesuffix();

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

    /*
    * First, try to find and restore a save file for specified character.
    * We'll return here if new game player_selection() renames the hero.
    */
attempt_restore:
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

    return resuming;
}

#ifdef PORT_HELP
void
port_help()
{
    /* display port specific help file */
    display_file(PORT_HELP, 1);
}
#endif /* PORT_HELP */

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    if (!strcmp(plname, WIZARD_NAME))
        return TRUE;
    return FALSE;
}


/*uwpmain.c*/
