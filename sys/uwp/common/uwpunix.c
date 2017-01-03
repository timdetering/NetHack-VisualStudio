/* NetHack 3.6	uwpunix.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

/* This file collects some Unix dependencies; pager.c contains some more */

#include "hack.h"
#include "wintty.h"

#include <sys/stat.h>
#if defined(WIN32) || defined(MSDOS)
#include <errno.h>
#endif

#if defined(WIN32) || defined(MSDOS)
extern char orgdir[];
#ifdef WIN32
extern void NDECL(backsp);
#endif
extern void NDECL(clear_screen);
#endif

#if 0
static struct stat buf;
#endif

#ifdef WANT_GETHDATE
static struct stat hbuf;
#endif

#ifdef PC_LOCKING
static int NDECL(eraseoldlocks);
#endif

#if 0
int
uptodate(fd)
int fd;
{
#ifdef WANT_GETHDATE
    if (fstat(fd, &buf)) {
        pline("Cannot get status of saved level? ");
        return(0);
    }
    if (buf.st_mtime < hbuf.st_mtime) {
        pline("Saved level is out of date. ");
        return(0);
    }
#else
#if (defined(MICRO) || defined(WIN32)) && !defined(NO_FSTAT)
    if (fstat(fd, &buf)) {
        if (moves > 1) pline("Cannot get status of saved level? ");
        else pline("Cannot get status of saved game.");
        return(0);
    }
    if (comp_times(buf.st_mtime)) {
        if (moves > 1) pline("Saved level is out of date.");
        else pline("Saved game is out of date. ");
        /* This problem occurs enough times we need to give the player
        * some more information about what causes it, and how to fix.
        */
#ifdef MSDOS
        pline("Make sure that your system's date and time are correct.");
        pline("They must be more current than NetHack.EXE's date/time stamp.");
#endif /* MSDOS */
        return(0);
    }
#endif /* MICRO */
#endif /* WANT_GETHDATE */
    return(1);
}
#endif

#ifdef PC_LOCKING
static int
eraseoldlocks()
{
    register int i;

    /* cannot use maxledgerno() here, because we need to find a lock name
    * before starting everything (including the dungeon initialization
    * that sets astral_level, needed for maxledgerno()) up
    */
    for (i = 1; i <= MAXDUNGEON * MAXLEVEL + 1; i++) {
        /* try to remove all */
        set_levelfile_name(lock, i);
        (void)unlink(fqname(lock, LEVELPREFIX, 0));
    }
    set_levelfile_name(lock, 0);
#ifdef HOLD_LOCKFILE_OPEN
    really_close();
#endif
    if (unlink(fqname(lock, LEVELPREFIX, 0)))
        return 0; /* cannot remove it */
    return (1);   /* success! */
}

// This prompt will work even when the windowing system has not been initialized
boolean prompt_yn(const char * prompt)
{
    char c;

    if (iflags.window_inited) {
        c = yn(prompt);
    }
    else {
        c = 'n';
        int ct = 0;

        char buf[512];
        Strcpy(buf, prompt);
        Strcat(buf, " [yn]");

        msmsg(buf);

        int ci;

        while ((ci = nhgetch()) != '\n') {
            if (ct > 0) {
                msmsg("\b \b");
                ct = 0;
                c = 'n';
            }
            if (ci == 'y' || ci == 'n' || ci == 'Y' || ci == 'N') {
                ct = 1;
                c = ci;
                msmsg("%c", c);
            }
        }
    }

    return (c == 'y') || (c == 'Y');
}

void
getlock()
{
    register int fd, ern;
    int fcmask = FCMASK;
    char tbuf[BUFSZ];
    const char *fq_lock;

    /* we ignore QUIT and INT at this point */
    if (!lock_file(HLOCK, LOCKPREFIX, 10)) {
        wait_synch();
        error("Quitting.");
    }

    /* regularize(lock); */ /* already done in uwpmains */
    Sprintf(tbuf, "%s", fqname(lock, LEVELPREFIX, 0));
    set_levelfile_name(lock, 0);
    fq_lock = fqname(lock, LEVELPREFIX, 1);

    if ((fd = open(fq_lock, 0)) == -1) {

        if (errno == ENOENT)
            goto gotlock; /* no such file */

        unlock_file(HLOCK);
        error("Unexpected failure.  Cannot open %s", fq_lock);
    }

    (void)nhclose(fd);


    if (prompt_yn("There are files from a game in progress under your name. Recover?")) {
        clear_screen();
        if (recover_savefile()) {
            goto gotlock;
        }
        else if (prompt_yn("Recovery failed.  Continue by removing corrupt saved files?")) {
            clear_screen();
            if (!eraseoldlocks()) {
                unlock_file(HLOCK);
                error("Cound not remove corrupt saved files.  Exiting.");
            }
            goto gotlock;
        }
        else {
            unlock_file(HLOCK);
            error("Can not continue.  Exiting.");
        }
    }
    else {
        unlock_file(HLOCK);
        error("Cannot start a new game.");
    }

gotlock:
    fd = creat(fq_lock, fcmask);

    if (fd == -1)
        ern = errno;

    unlock_file(HLOCK);

    if (fd == -1) {
        error("Unexpected error creating file (%s).", fq_lock);
    }
    else {
        if (write(fd, (char *)&hackpid, sizeof(hackpid)) != sizeof(hackpid)) {
            error("Unexpected error writing to file (%s)", fq_lock);
        }
        if (nhclose(fd) == -1) {
            error("Cannot close file (%s)", fq_lock);
        }
    }
}
#endif /* PC_LOCKING */
