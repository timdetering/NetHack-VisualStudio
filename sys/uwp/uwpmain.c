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

#ifndef NOCWD_ASSUMPTIONS
#error NOCWD_ASSUMPTIONS must be defined
#endif

#ifndef NO_SIGNAL
#error NO_SIGNAL must be defined
#endif

//char * orgdir = NULL;

STATIC_DCL void FDECL(process_options, (int argc, char **argv));
STATIC_DCL void NDECL(nhusage);

extern void FDECL(nethack_exit, (int));

extern int redirect_stdout;       /* from sys/share/pcsys.c */
extern int GUILaunched;
HANDLE hStdOut;
char *NDECL(exename);

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



boolean
uwpmain(void)
{
    int fd;
    char *envp = NULL;
    char *sptr = NULL;

    char fnamebuf[BUFSZ], encodedfnamebuf[BUFSZ];
    boolean resuming = FALSE; /* assume new game */

    u.uhp = 1; /* prevent RIP on early quits */
    u.ux = 0;  /* prevent flush_screen() */
   
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

    /* unlike Unix where the game might be invoked with a script
    which forces a particular character name for each player
    using a shared account, we always allow player to rename
    the character during role/race/&c selection */
    iflags.renameallowed = TRUE;

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
    Sprintf(fnamebuf, "%s", plname);
    (void)fname_encode(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.", '%',
        fnamebuf, encodedfnamebuf, BUFSZ);
    Sprintf(lock, "%s", encodedfnamebuf);
    getlock();
#endif /* PC_LOCKING */

                      /* Set up level 0 file to keep the game state.
                      */
    fd = create_levelfile(0, (char *)0);
    if (fd < 0) {
        /* TODO(bhouse) do we need to pring anything here? 
           commenting out for now.  Either we have an error and
           can't continue or we just forge ahead. */
        /* raw_print("Cannot create lock file"); */
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
                delete_levelfile(0); /* remove empty lock file */
                getlock();
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
