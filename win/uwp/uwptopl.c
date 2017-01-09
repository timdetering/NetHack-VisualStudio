/* NetHack 3.6	uwptopl.c	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef UWP_GRAPHICS

#include "tcap.h"
#include "winuwp.h"

#ifndef C /* this matches src/cmd.c */
#define C(c) (0x1f & (c))
#endif

STATIC_DCL void FDECL(uwp_redotoplin, (const char *));
STATIC_DCL void FDECL(uwp_topl_putsym, (CHAR_P));
STATIC_DCL void NDECL(uwp_remember_topl);
STATIC_DCL void FDECL(uwp_removetopl, (int));
STATIC_DCL void FDECL(uwp_msghistory_snapshot, (BOOLEAN_P));
STATIC_DCL void FDECL(uwp_free_msghistory_snapshot, (BOOLEAN_P));

int
uwp_doprev_message()
{
    register struct UwpWinDesc *cw = uwp_wins[WIN_MESSAGE];

    winid prevmsg_win;
    int i;
    if ((iflags.prevmsg_window != 's')
        && !uwpDisplay->inread) {           /* not single */
        if (iflags.prevmsg_window == 'f') { /* full */
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            cw->maxcol = cw->maxrow;
            i = cw->maxcol;
            do {
                if (cw->data[i] && strcmp(cw->data[i], ""))
                    putstr(prevmsg_win, 0, cw->data[i]);
                i = (i + 1) % cw->rows;
            } while (i != cw->maxcol);
            putstr(prevmsg_win, 0, toplines);
            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
        } else if (iflags.prevmsg_window == 'c') { /* combination */
            do {
                uwp_morc = 0;
                if (cw->maxcol == cw->maxrow) {
                    uwpDisplay->dismiss_more = C('p'); /* ^P ok at --More-- */
                    uwp_redotoplin(toplines);
                    cw->maxcol--;
                    if (cw->maxcol < 0)
                        cw->maxcol = cw->rows - 1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else if (cw->maxcol == (cw->maxrow - 1)) {
                    uwpDisplay->dismiss_more = C('p'); /* ^P ok at --More-- */
                    uwp_redotoplin(cw->data[cw->maxcol]);
                    cw->maxcol--;
                    if (cw->maxcol < 0)
                        cw->maxcol = cw->rows - 1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else {
                    prevmsg_win = create_nhwindow(NHW_MENU);
                    putstr(prevmsg_win, 0, "Message History");
                    putstr(prevmsg_win, 0, "");
                    cw->maxcol = cw->maxrow;
                    i = cw->maxcol;
                    do {
                        if (cw->data[i] && strcmp(cw->data[i], ""))
                            putstr(prevmsg_win, 0, cw->data[i]);
                        i = (i + 1) % cw->rows;
                    } while (i != cw->maxcol);
                    putstr(prevmsg_win, 0, toplines);
                    display_nhwindow(prevmsg_win, TRUE);
                    destroy_nhwindow(prevmsg_win);
                }

            } while (uwp_morc == C('p'));
            uwpDisplay->dismiss_more = 0;
        } else { /* reversed */
            uwp_morc = 0;
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            putstr(prevmsg_win, 0, toplines);
            cw->maxcol = cw->maxrow - 1;
            if (cw->maxcol < 0)
                cw->maxcol = cw->rows - 1;
            do {
                putstr(prevmsg_win, 0, cw->data[cw->maxcol]);
                cw->maxcol--;
                if (cw->maxcol < 0)
                    cw->maxcol = cw->rows - 1;
                if (!cw->data[cw->maxcol])
                    cw->maxcol = cw->maxrow;
            } while (cw->maxcol != cw->maxrow);

            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
            cw->maxcol = cw->maxrow;
            uwpDisplay->dismiss_more = 0;
        }
    } else if (iflags.prevmsg_window == 's') { /* single */
        uwpDisplay->dismiss_more = C('p'); /* <ctrl/P> allowed at --More-- */
        do {
            uwp_morc = 0;
            if (cw->maxcol == cw->maxrow)
                uwp_redotoplin(toplines);
            else if (cw->data[cw->maxcol])
                uwp_redotoplin(cw->data[cw->maxcol]);
            cw->maxcol--;
            if (cw->maxcol < 0)
                cw->maxcol = cw->rows - 1;
            if (!cw->data[cw->maxcol])
                cw->maxcol = cw->maxrow;
        } while (uwp_morc == C('p'));
        uwpDisplay->dismiss_more = 0;
    }
    return 0;
}

STATIC_OVL void
uwp_redotoplin(str)
const char *str;
{
    int otoplin = uwpDisplay->toplin;

    uwp_home();
    if (*str & 0x80) {
        /* kludge for the / command, the only time we ever want a */
        /* graphics character on the top line */
        uwp_g_putch((int) *str++);
        uwpDisplay->curx++;
    }
    uwp_end_glyphout(); /* in case message printed during graphics output */
    uwp_putsyms(str);
    uwp_cl_end();
    uwpDisplay->toplin = 1;
    if (uwpDisplay->cury && otoplin != 3)
        uwp_more();
}

STATIC_OVL void
remember_topl()
{
    register struct UwpWinDesc *cw = uwp_wins[WIN_MESSAGE];
    int idx = cw->maxrow;
    unsigned len = strlen(toplines) + 1;

    if ((cw->flags & UWP_WIN_LOCKHISTORY) || !*toplines)
        return;

    if (len > (unsigned) cw->datlen[idx]) {
        if (cw->data[idx])
            free(cw->data[idx]);
        len += (8 - (len & 7)); /* pad up to next multiple of 8 */
        cw->data[idx] = (char *) alloc(len);
        cw->datlen[idx] = (short) len;
    }
    Strcpy(cw->data[idx], toplines);
    *toplines = '\0';
    cw->maxcol = cw->maxrow = (idx + 1) % cw->rows;
}

void
uwp_addtopl(s)
const char *s;
{
    register struct UwpWinDesc *cw = uwp_wins[WIN_MESSAGE];

    uwp_curs(UWP_BASE_WINDOW, cw->curx + 1, cw->cury);
    uwp_putsyms(s);
    uwp_cl_end();
    uwpDisplay->toplin = 1;
}

void
uwp_more()
{
    struct UwpWinDesc *cw = uwp_wins[WIN_MESSAGE];

    /* avoid recursion -- only happens from interrupts */
    if (uwpDisplay->inmore++)
        return;

    if (uwpDisplay->toplin) {
        uwp_curs(UWP_BASE_WINDOW, cw->curx + 1, cw->cury);
        if (cw->curx >= CO - 8)
            uwp_topl_putsym('\n');
    }

    if (flags.standout)
        uwp_standoutbeg();
    uwp_putsyms(uwp_defmorestr);
    if (flags.standout)
        uwp_standoutend();

    uwp_xwaitforspace("\033 ");

    if (uwp_morc == '\033')
        cw->flags |= UWP_WIN_STOP;

    if (uwpDisplay->toplin && cw->cury) {
        uwp_docorner(1, cw->cury + 1);
        cw->curx = cw->cury = 0;
        uwp_home();
    } else if (uwp_morc == '\033') {
        cw->curx = cw->cury = 0;
        uwp_home();
        uwp_cl_end();
    }
    uwpDisplay->toplin = 0;
    uwpDisplay->inmore = 0;
}

void
uwp_update_topl(bp)
register const char *bp;
{
    register char *tl, *otl;
    register int n0;
    int notdied = 1;
    struct UwpWinDesc *cw = uwp_wins[WIN_MESSAGE];

    /* If there is room on the line, print message on same line */
    /* But messages like "You die..." deserve their own line */
    n0 = strlen(bp);
    if ((uwpDisplay->toplin == 1 || (cw->flags & UWP_WIN_STOP)) && cw->cury == 0
        && n0 + (int) strlen(toplines) + 3 < CO - 8 /* room for --More-- */
        && (notdied = strncmp(bp, "You die", 7)) != 0) {
        Strcat(toplines, "  ");
        Strcat(toplines, bp);
        cw->curx += 2;
        if (!(cw->flags & UWP_WIN_STOP))
            uwp_addtopl(bp);
        return;
    } else if (!(cw->flags & UWP_WIN_STOP)) {
        if (uwpDisplay->toplin == 1) {
            uwp_more();
        } else if (cw->cury) { /* for when flags.toplin == 2 && cury > 1 */
            uwp_docorner(1, cw->cury + 1); /* reset cury = 0 if redraw screen */
            cw->curx = cw->cury = 0;   /* from uwp_home--cls() & uwp_docorner(1,n) */
        }
    }
    remember_topl();
    (void) strncpy(toplines, bp, TBUFSZ);
    toplines[TBUFSZ - 1] = 0;

    for (tl = toplines; n0 >= CO; ) {
        otl = tl;
        for (tl += CO - 1; tl != otl; --tl)
            if (*tl == ' ')
                break;
        if (tl == otl) {
            /* Eek!  A huge token.  Try splitting after it. */
            tl = index(otl, ' ');
            if (!tl)
                break; /* No choice but to spit it out whole. */
        }
        *tl++ = '\n';
        n0 = strlen(tl);
    }
    if (!notdied)
        cw->flags &= ~UWP_WIN_STOP;
    if (!(cw->flags & UWP_WIN_STOP))
        uwp_redotoplin(toplines);
}

STATIC_OVL
void
uwp_topl_putsym(c)
char c;
{
    register struct UwpWinDesc *cw = uwp_wins[WIN_MESSAGE];

    if (cw == (struct UwpWinDesc *) 0)
        panic("Putsym window MESSAGE nonexistant");

    switch (c) {
    case '\b':
        if (uwpDisplay->curx == 0 && uwpDisplay->cury > 0)
            uwp_curs(UWP_BASE_WINDOW, CO, (int) uwpDisplay->cury - 1);
        uwp_backsp();
        uwpDisplay->curx--;
        cw->curx = uwpDisplay->curx;
        return;
    case '\n':
        uwp_cl_end();
        uwpDisplay->curx = 0;
        uwpDisplay->cury++;
        cw->cury = uwpDisplay->cury;
#ifdef WIN32CON
        (void) putchar(c);
#endif
        break;
    default:
        if (uwpDisplay->curx == CO - 1)
            uwp_topl_putsym('\n'); /* 1 <= curx < CO; avoid CO */
#ifdef WIN32CON
        (void) putchar(c);
#endif
        uwpDisplay->curx++;
    }
    cw->curx = uwpDisplay->curx;
    if (cw->curx == 0)
        uwp_cl_end();
#ifndef WIN32CON
    (void) putchar(c);
#endif
}

void
uwp_putsyms(str)
const char *str;
{
    while (*str)
        uwp_topl_putsym(*str++);
}

STATIC_OVL void
removetopl(n)
register int n;
{
    /* assume uwp_addtopl() has been done, so uwpDisplay->toplin is already set */
    while (n-- > 0)
        uwp_putsyms("\b \b");
}

extern char erase_char; /* from xxxtty.c; don't need kill_char */

char
uwp_yn_function(query, resp, def)
const char *query, *resp;
char def;
/*
 *   Generic yes/no function. 'def' is the default (returned by space or
 *   return; 'esc' returns 'q', or 'n', or the default, depending on
 *   what's in the string. The 'query' string is printed before the user
 *   is asked about the string.
 *   If resp is NULL, any single character is accepted and returned.
 *   If not-NULL, only characters in it are allowed (exceptions:  the
 *   quitchars are always allowed, and if it contains '#' then digits
 *   are allowed); if it includes an <esc>, anything beyond that won't
 *   be shown in the prompt to the user but will be acceptable as input.
 */
{
    register char q;
    char rtmp[40];
    boolean digit_ok, allow_num, preserve_case = FALSE;
    struct UwpWinDesc *cw = uwp_wins[WIN_MESSAGE];
    boolean doprev = 0;
    char prompt[BUFSZ];

    if (uwpDisplay->toplin == 1 && !(cw->flags & UWP_WIN_STOP))
        uwp_more();
    cw->flags &= ~UWP_WIN_STOP;
    uwpDisplay->toplin = 3; /* special prompt state */
    uwpDisplay->inread++;
    if (resp) {
        char *rb, respbuf[QBUFSZ];

        allow_num = (index(resp, '#') != 0);
        Strcpy(respbuf, resp);
        /* normally we force lowercase, but if any uppercase letters
           are present in the allowed response, preserve case;
           check this before stripping the hidden choices */
        for (rb = respbuf; *rb; ++rb)
            if ('A' <= *rb && *rb <= 'Z') {
                preserve_case = TRUE;
                break;
            }
        /* any acceptable responses that follow <esc> aren't displayed */
        if ((rb = index(respbuf, '\033')) != 0)
            *rb = '\0';
        (void) strncpy(prompt, query, QBUFSZ - 1);
        prompt[QBUFSZ - 1] = '\0';
        Sprintf(eos(prompt), " [%s]", respbuf);
        if (def)
            Sprintf(eos(prompt), " (%c)", def);
        /* not pline("%s ", prompt);
           trailing space is wanted here in case of reprompt */
        Strcat(prompt, " ");
        pline("%s", prompt);
    } else {
        /* no restriction on allowed response, so always preserve case */
        /* preserve_case = TRUE; -- moot since we're jumping to the end */
        pline("%s ", query);
        q = readchar();
        goto clean_up;
    }

    do { /* loop until we get valid input */
        q = readchar();
        if (!preserve_case)
            q = lowc(q);
        if (q == '\020') { /* ctrl-P */
            if (iflags.prevmsg_window != 's') {
                int sav = uwpDisplay->inread;
                uwpDisplay->inread = 0;
                (void) uwp_doprev_message();
                uwpDisplay->inread = sav;
                uwp_clear_nhwindow(WIN_MESSAGE);
                cw->maxcol = cw->maxrow;
                uwp_addtopl(prompt);
            } else {
                if (!doprev)
                    (void) uwp_doprev_message(); /* need two initially */
                (void) uwp_doprev_message();
                doprev = 1;
            }
            q = '\0'; /* force another loop iteration */
            continue;
        } else if (doprev) {
            /* BUG[?]: this probably ought to check whether the
               character which has just been read is an acceptable
               response; if so, skip the reprompt and use it. */
            uwp_clear_nhwindow(WIN_MESSAGE);
            cw->maxcol = cw->maxrow;
            doprev = 0;
            uwp_addtopl(prompt);
            q = '\0'; /* force another loop iteration */
            continue;
        }
        digit_ok = allow_num && digit(q);
        if (q == '\033') {
            if (index(resp, 'q'))
                q = 'q';
            else if (index(resp, 'n'))
                q = 'n';
            else
                q = def;
            break;
        } else if (index(quitchars, q)) {
            q = def;
            break;
        }
        if (!index(resp, q) && !digit_ok) {
            uwp_nhbell();
            q = (char) 0;
        } else if (q == '#' || digit_ok) {
            char z, digit_string[2];
            int n_len = 0;
            long value = 0;
            uwp_addtopl("#"), n_len++;
            digit_string[1] = '\0';
            if (q != '#') {
                digit_string[0] = q;
                uwp_addtopl(digit_string), n_len++;
                value = q - '0';
                q = '#';
            }
            do { /* loop until we get a non-digit */
                z = readchar();
                if (!preserve_case)
                    z = lowc(z);
                if (digit(z)) {
                    value = (10 * value) + (z - '0');
                    if (value < 0)
                        break; /* overflow: try again */
                    digit_string[0] = z;
                    uwp_addtopl(digit_string), n_len++;
                } else if (z == 'y' || index(quitchars, z)) {
                    if (z == '\033')
                        value = -1; /* abort */
                    z = '\n';       /* break */
                } else if (z == erase_char || z == '\b') {
                    if (n_len <= 1) {
                        value = -1;
                        break;
                    } else {
                        value /= 10;
                        removetopl(1), n_len--;
                    }
                } else {
                    value = -1; /* abort */
                    uwp_nhbell();
                    break;
                }
            } while (z != '\n');
            if (value > 0)
                yn_number = value;
            else if (value == 0)
                q = 'n'; /* 0 => "no" */
            else {       /* remove number from top line, then try again */
                removetopl(n_len), n_len = 0;
                q = '\0';
            }
        }
    } while (!q);

    if (q != '#') {
        Sprintf(rtmp, "%c", q);
        uwp_addtopl(rtmp);
    }
clean_up:
    uwpDisplay->inread--;
    uwpDisplay->toplin = 2;
    if (uwpDisplay->intr)
        uwpDisplay->intr--;
    if (uwp_wins[WIN_MESSAGE]->cury)
        uwp_clear_nhwindow(WIN_MESSAGE);

    return q;
}

/* shared by uwp_getmsghistory() and uwp_putmsghistory() */
static char **snapshot_mesgs = 0;

/* collect currently available message history data into a sequential array;
   optionally, purge that data from the active circular buffer set as we go */
STATIC_OVL void
uwp_msghistory_snapshot(purge)
boolean purge; /* clear message history buffer as we copy it */
{
    char *mesg;
    int i, inidx, outidx;
    struct UwpWinDesc *cw;

    /* paranoia (too early or too late panic save attempt?) */
    if (WIN_MESSAGE == WIN_ERR || !uwp_wins[WIN_MESSAGE])
        return;
    cw = uwp_wins[WIN_MESSAGE];

    /* flush toplines[], moving most recent message to history */
    remember_topl();

    /* for a passive snapshot, we just copy pointers, so can't allow further
       history updating to take place because that could clobber them */
    if (!purge)
        cw->flags |= UWP_WIN_LOCKHISTORY;

    snapshot_mesgs = (char **) alloc((cw->rows + 1) * sizeof(char *));
    outidx = 0;
    inidx = cw->maxrow;
    for (i = 0; i < cw->rows; ++i) {
        snapshot_mesgs[i] = (char *) 0;
        mesg = cw->data[inidx];
        if (mesg && *mesg) {
            snapshot_mesgs[outidx++] = mesg;
            if (purge) {
                /* we're taking this pointer away; subsequest history
                   updates will eventually allocate a new one to replace it */
                cw->data[inidx] = (char *) 0;
                cw->datlen[inidx] = 0;
            }
        }
        inidx = (inidx + 1) % cw->rows;
    }
    snapshot_mesgs[cw->rows] = (char *) 0; /* sentinel */

    /* for a destructive snapshot, history is now completely empty */
    if (purge)
        cw->maxcol = cw->maxrow = 0;
}

/* release memory allocated to message history snapshot */
STATIC_OVL void
uwp_free_msghistory_snapshot(purged)
boolean purged; /* True: took history's pointers, False: just cloned them */
{
    if (snapshot_mesgs) {
        /* snapshot pointers are no longer in use */
        if (purged) {
            int i;

            for (i = 0; snapshot_mesgs[i]; ++i)
                free((genericptr_t) snapshot_mesgs[i]);
        }

        free((genericptr_t) snapshot_mesgs), snapshot_mesgs = (char **) 0;

        /* history can resume being updated at will now... */
        if (!purged)
            uwp_wins[WIN_MESSAGE]->flags &= ~UWP_WIN_LOCKHISTORY;
    }
}

/*
 * This is called by the core save routines.
 * Each time we are called, we return one string from the
 * message history starting with the oldest message first.
 * When none are left, we return a final null string.
 *
 * History is collected at the time of the first call.
 * Any new messages issued after that point will not be
 * included among the output of the subsequent calls.
 */
char *
uwp_getmsghistory(init)
boolean init;
{
    static int nxtidx;
    char *nextmesg;
    char *result = 0;

    if (init) {
        uwp_msghistory_snapshot(FALSE);
        nxtidx = 0;
    }

    if (snapshot_mesgs) {
        nextmesg = snapshot_mesgs[nxtidx++];
        if (nextmesg) {
            result = (char *) nextmesg;
        } else {
            uwp_free_msghistory_snapshot(FALSE);
        }
    }
    return result;
}

/*
 * This is called by the core savefile restore routines.
 * Each time we are called, we stuff the string into our message
 * history recall buffer. The core will send the oldest message
 * first (actually it sends them in the order they exist in the
 * save file, but that is supposed to be the oldest first).
 * These messages get pushed behind any which have been issued
 * since this session with the program has been started, since
 * they come from a previous session and logically precede
 * anything (like "Restoring save file...") that's happened now.
 *
 * Called with a null pointer to finish up restoration.
 *
 * It's also called by the quest pager code when a block message
 * has a one-line summary specified.  We put that line directly
 * message history for ^P recall without having displayed it.
 */
void
uwp_putmsghistory(msg, restoring_msghist)
const char *msg;
boolean restoring_msghist;
{
    static boolean initd = FALSE;
    int idx;

    if (restoring_msghist && !initd) {
        /* we're restoring history from the previous session, but new
           messages have already been issued this session ("Restoring...",
           for instance); collect current history (ie, those new messages),
           and also clear it out so that nothing will be present when the
           restored ones are being put into place */
        uwp_msghistory_snapshot(TRUE);
        initd = TRUE;
    }

    if (msg) {
        /* move most recent message to history, make this become most recent
         */
        remember_topl();
        Strcpy(toplines, msg);
    } else if (snapshot_mesgs) {
        /* done putting arbitrary messages in; put the snapshot ones back */
        for (idx = 0; snapshot_mesgs[idx]; ++idx) {
            remember_topl();
            Strcpy(toplines, snapshot_mesgs[idx]);
        }
        /* now release the snapshot */
        uwp_free_msghistory_snapshot(TRUE);
        initd = FALSE; /* reset */
    }
}

#endif /* UWP_GRAPHICS */
