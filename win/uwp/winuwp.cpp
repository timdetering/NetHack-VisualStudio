/* NetHack 3.6	winuwp.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

#ifndef UWP_GRAPHICS
#error UWP_GRAPHICS must be defined
#endif

#ifndef TEXTCOLOR
#error TEXTCOLOR must be defined
#endif

#define NEWAUTOCOMP

using namespace Nethack;

extern "C" {

/*
 * Neither a standard out nor character-based control codes should be
 * part of the "tty look" windowing implementation.
 * h+ 930227
 */

/* this is only needed until tty_status_* routines are written */
extern NEARDATA winid WIN_STATUS;

/* erase_char and kill_char are usd by getline.c and topl.c */
char erase_char, kill_char;

/* Interface definition, for windows.c */
struct window_procs uwp_procs = {
    "uwp",
#ifdef MSDOS
    WC_TILED_MAP | WC_ASCII_MAP |
#endif
#if defined(WIN32CON)
        WC_MOUSE_SUPPORT |
#endif
        WC_COLOR | WC_HILITE_PET | WC_INVERSE | WC_EIGHT_BIT_IN,
#if defined(SELECTSAVED)
    WC2_SELECTSAVED |
#endif
        WC2_DARKGRAY,
    tty_init_nhwindows, tty_player_selection, tty_askname, tty_get_nh_event,
    tty_exit_nhwindows, tty_suspend_nhwindows, tty_resume_nhwindows,
    tty_create_nhwindow, tty_clear_nhwindow, tty_display_nhwindow,
    tty_destroy_nhwindow, tty_curs, tty_putstr, genl_putmixed,
    tty_display_file, tty_start_menu, tty_add_menu, tty_end_menu,
    tty_select_menu, tty_message_menu, tty_update_inventory, tty_mark_synch,
    tty_wait_synch,
#ifdef CLIPPING
    tty_cliparound,
#endif
#ifdef POSITIONBAR
    tty_update_positionbar,
#endif
    tty_print_glyph, tty_raw_print, tty_raw_print_bold, tty_nhgetch,
    tty_nh_poskey, tty_nhbell, tty_doprev_message, tty_yn_function,
    tty_getlin, tty_get_ext_cmd, tty_number_pad, tty_delay_output,
#ifdef CHANGE_COLOR /* the Mac uses a palette device */
    tty_change_color,
#ifdef MAC
    tty_change_background, set_tty_font_name,
#endif
    tty_get_color_string,
#endif

    /* other defs that really should go away (they're tty specific) */
    tty_start_screen, tty_end_screen, genl_outrip,
#if defined(WIN32)
    nttty_preference_update,
#else
    genl_preference_update,
#endif
    tty_getmsghistory, tty_putmsghistory,
#ifdef STATUS_VIA_WINDOWPORT
    tty_status_init,
    genl_status_finish, genl_status_enablefield,
    tty_status_update,
#ifdef STATUS_HILITES
    tty_status_threshold,
#endif
#endif
    genl_can_suspend_yes,
};

static int maxwin = 0; /* number of windows in use */
winid BASE_WINDOW;
struct WinDesc *g_wins[MAXWIN];
struct DisplayDesc *g_uwpDisplay; /* the tty display descriptor */

char winpanicstr[] = "Bad window id %d";
char defmorestr[] = "--More--";

#ifdef CLIPPING
#if defined(USE_TILES) && defined(MSDOS)
boolean clipping = FALSE; /* clipping on? */
int clipx = 0, clipxmax = 0;
#else
static boolean clipping = FALSE; /* clipping on? */
static int clipx = 0, clipxmax = 0;
#endif
static int clipy = 0, clipymax = 0;
#endif /* CLIPPING */

#if defined(USE_TILES) && defined(MSDOS)
extern void FDECL(adjust_cursor_flags, (struct WinDesc *));
#endif

#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
boolean GFlag = FALSE;
boolean HE_resets_AS; /* see termcap.c */
#endif

#if defined(MICRO) || defined(WIN32CON)
static const char to_continue[] = "to continue";
#define getret() getreturn(to_continue)
#else
STATIC_DCL void NDECL(getret);
#endif
STATIC_DCL void FDECL(erase_menu_or_text,
                      (winid, struct WinDesc *, BOOLEAN_P));
STATIC_DCL void FDECL(free_window_info, (struct WinDesc *, BOOLEAN_P));
STATIC_DCL void FDECL(dmore, (struct WinDesc *, const char *));
STATIC_DCL void FDECL(set_item_state, (winid, int, tty_menu_item *));
STATIC_DCL void FDECL(set_all_on_page, (winid, tty_menu_item *,
                                        tty_menu_item *));
STATIC_DCL void FDECL(unset_all_on_page, (winid, tty_menu_item *,
                                          tty_menu_item *));
STATIC_DCL void FDECL(invert_all_on_page, (winid, tty_menu_item *,
                                           tty_menu_item *, CHAR_P));
STATIC_DCL void FDECL(invert_all, (winid, tty_menu_item *,
                                   tty_menu_item *, CHAR_P));
STATIC_DCL void FDECL(toggle_menu_attr, (BOOLEAN_P, int, int));
STATIC_DCL tty_menu_item *FDECL(reverse, (tty_menu_item *));
STATIC_DCL const char *FDECL(compress_str, (const char *));
STATIC_DCL void FDECL(bail, (const char *)); /* __attribute__((noreturn)) */
STATIC_DCL void FDECL(setup_rolemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_racemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_gendmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_algnmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL boolean NDECL(reset_role_filtering);

/* clean up and quit */
STATIC_OVL void
bail(const char *mesg)
{
    clearlocks();
    tty_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

/*ARGSUSED*/
void
tty_init_nhwindows(int *, char **)
{
    int wid, hgt;

    erase_char = '\b';
    kill_char = 21; /* cntl-U */
    iflags.cbreak = TRUE;

    /* to port dependant tty setup */
    wid = g_textGrid.GetDimensions().m_x;
    hgt = g_textGrid.GetDimensions().m_y;

    LI = hgt;
    CO = wid;

    set_option_mod_status("mouse_support", SET_IN_GAME);

    start_screen();

    /* set up tty descriptor */
    g_uwpDisplay = (struct DisplayDesc *) alloc(sizeof(struct DisplayDesc));
    g_uwpDisplay->toplin = 0;
    g_uwpDisplay->rows = hgt;
    g_uwpDisplay->cols = wid;
    g_uwpDisplay->inmore = g_uwpDisplay->inread = g_uwpDisplay->intr = 0;
    g_uwpDisplay->dismiss_more = 0;

    /* set up the default windows */
    BASE_WINDOW = tty_create_nhwindow(NHW_BASE);
    g_wins[BASE_WINDOW]->active = 1;

    g_uwpDisplay->lastwin = WIN_ERR;

    tty_clear_nhwindow(BASE_WINDOW);
    tty_display_nhwindow(BASE_WINDOW, FALSE);
}

/*
 * plname is filled either by an option (-u Player  or  -uPlayer) or
 * explicitly (by being the wizard) or by askname.
 * It may still contain a suffix denoting the role, etc.
 * Always called after init_nhwindows() and before display_gamewindows().
 */
void
tty_askname()
{
    static const char who_are_you[] = "Who are you? ";
    register int c, ct, tryct = 0;

#ifdef SELECTSAVED
    if (iflags.wc2_selectsaved && !iflags.renameinprogress)
        switch (restore_menu(BASE_WINDOW)) {
        case -1:
            bail("Until next time then..."); /* quit */
            /*NOTREACHED*/
        case 0:
            break; /* no game chosen; start new game */
        case 1:
            return; /* plname[] has been set */
        }
#endif /* SELECTSAVED */

    int startLine = g_wins[BASE_WINDOW]->cury;

    do {
        if (++tryct > 1) {
            if (tryct > 10)
                bail("Giving up after 10 tries.\n");
            tty_curs(BASE_WINDOW, 1, startLine);
            win_puts(BASE_WINDOW, "Enter a name for your character...");
        }

        tty_curs(BASE_WINDOW, 1, startLine + 1), cl_end();

        win_puts(BASE_WINDOW, who_are_you);

        ct = 0;
        while ((c = tty_nhgetch()) != '\n') {
            if (c == EOF)
                c = '\033';
            if (c == '\033') {
                ct = 0;
                break;
            } /* continue outer loop */
#if defined(WIN32CON)
            if (c == '\003')
                bail("^C abort.\n");
#endif
            /* some people get confused when their erase char is not ^H */
            if (c == '\b' || c == '\177') {
                if (ct) {
                    ct--;
                    win_puts(BASE_WINDOW, "\b \b");
                }
                continue;
            }
            if (ct < (int) (sizeof plname) - 1) {
                win_putc(BASE_WINDOW, c);
                plname[ct++] = c;
            }
        }
        plname[ct] = 0;
    } while (ct == 0);

    /* move to next line to simulate echo of user's <return> */
    tty_curs(BASE_WINDOW, 1, g_wins[BASE_WINDOW]->cury + 1);

    /* since we let user pick an arbitrary name now, he/she can pick
       another one during role selection */
    iflags.renameallowed = TRUE;
}

void
tty_get_nh_event()
{
    // do nothing
    return;
}

void
tty_suspend_nhwindows(const char *str)
{
    // Should never get called
    assert(0);
}

void
tty_resume_nhwindows()
{
    // Should never get called
    assert(0);
}

void
tty_exit_nhwindows(const char *str)
{
    winid i;

    /*
     * Disable windows to avoid calls to window routines.
     */
    free_pickinv_cache(); /* reset its state as well as tear down window */
    for (i = 0; i < MAXWIN; i++) {
        if (i == BASE_WINDOW)
            continue; /* handle g_wins[BASE_WINDOW] last */
        if (g_wins[i]) {
#ifdef FREE_ALL_MEMORY
            free_window_info(g_wins[i], TRUE);
            free((genericptr_t) g_wins[i]);
#endif
            g_wins[i] = (struct WinDesc *) 0;
        }
    }
    WIN_MAP = WIN_MESSAGE = WIN_INVEN = WIN_ERR; /* these are all gone now */
#ifndef STATUS_VIA_WINDOWPORT
    WIN_STATUS = WIN_ERR;
#endif
#ifdef FREE_ALL_MEMORY
    if (BASE_WINDOW != WIN_ERR && g_wins[BASE_WINDOW]) {
        free_window_info(g_wins[BASE_WINDOW], TRUE);
        free((genericptr_t) g_wins[BASE_WINDOW]);
        g_wins[BASE_WINDOW] = (struct WinDesc *) 0;
        BASE_WINDOW = WIN_ERR;
    }
    free((genericptr_t) g_uwpDisplay);
    g_uwpDisplay = (struct DisplayDesc *) 0;
#endif

    iflags.window_inited = 0;
}

winid
tty_create_nhwindow(int type)
{
    struct WinDesc *newwin;
    int i;
    int newid;

    if (maxwin == MAXWIN)
        return WIN_ERR;

    newwin = (struct WinDesc *) alloc(sizeof(struct WinDesc));
    newwin->type = type;
    newwin->flags = 0;
    newwin->active = FALSE;
    newwin->curx = newwin->cury = 0;
    newwin->morestr = 0;
    newwin->mlist = (tty_menu_item *) 0;
    newwin->plist = (tty_menu_item **) 0;
    newwin->npages = newwin->plist_size = newwin->nitems = newwin->how = 0;
    switch (type) {
    case NHW_BASE:
        /* base window, used for absolute movement on the screen */
        newwin->offx = newwin->offy = 0;
        newwin->rows = g_uwpDisplay->rows;
        newwin->cols = g_uwpDisplay->cols;
        newwin->maxrow = newwin->maxcol = 0;
        break;
    case NHW_MESSAGE:
        /* message window, 1 line long, very wide, top of screen */
        newwin->offx = newwin->offy = 0;
        /* sanity check */
        if (iflags.msg_history < 20)
            iflags.msg_history = 20;
        else if (iflags.msg_history > 60)
            iflags.msg_history = 60;
        newwin->maxrow = newwin->rows = iflags.msg_history;
        newwin->maxcol = newwin->cols = 0;
        break;
    case NHW_STATUS:
        /* status window, 2 lines long, full width, bottom of screen */
        newwin->offx = 0;
#if defined(USE_TILES) && defined(MSDOS)
        if (iflags.grmode) {
            newwin->offy = g_uwpDisplay->rows - 2;
        } else
#endif
            newwin->offy = min((int) g_uwpDisplay->rows - 2, ROWNO + 1);
        newwin->rows = newwin->maxrow = 2;
        newwin->cols = newwin->maxcol = g_uwpDisplay->cols;
        break;
    case NHW_MAP:
        /* map window, ROWNO lines long, full width, below message window */
        newwin->offx = 0;
        newwin->offy = 1;
        newwin->rows = ROWNO;
        newwin->cols = COLNO;
        newwin->maxrow = 0; /* no buffering done -- let gbuf do it */
        newwin->maxcol = 0;
        break;
    case NHW_MENU:
    case NHW_TEXT:
        /* inventory/menu window, variable length, full width, top of screen
         */
        /* help window, the same, different semantics for display, etc */
        newwin->offx = newwin->offy = 0;
        newwin->rows = 0;
        newwin->cols = g_uwpDisplay->cols;
        newwin->maxrow = newwin->maxcol = 0;
        break;
    default:
        panic("Tried to create window type %d\n", (int) type);
        return WIN_ERR;
    }

    for (newid = 0; newid < MAXWIN; newid++) {
        if (g_wins[newid] == 0) {
            g_wins[newid] = newwin;
            break;
        }
    }
    if (newid == MAXWIN) {
        panic("No window slots!");
        return WIN_ERR;
    }

    if (newwin->maxrow) {
        newwin->data =
            (char **) alloc(sizeof(char *) * (unsigned) newwin->maxrow);
        newwin->datlen =
            (short *) alloc(sizeof(short) * (unsigned) newwin->maxrow);
        if (newwin->maxcol) {
            /* WIN_STATUS */
            for (i = 0; i < newwin->maxrow; i++) {
                newwin->data[i] = (char *) alloc((unsigned) newwin->maxcol);
                newwin->datlen[i] = (short) newwin->maxcol;
            }
        } else {
            for (i = 0; i < newwin->maxrow; i++) {
                newwin->data[i] = (char *) 0;
                newwin->datlen[i] = 0;
            }
        }
        if (newwin->type == NHW_MESSAGE)
            newwin->maxrow = 0;
    } else {
        newwin->data = (char **) 0;
        newwin->datlen = (short *) 0;
    }

    return newid;
}

STATIC_OVL void
erase_menu_or_text(winid window, struct WinDesc * cw, boolean clear)
{
    if (cw->offx == 0)
        if (cw->offy) {
            tty_curs(window, 1, 0);
            cl_eos();
        } else if (clear)
            clear_screen();
        else
            docrt();
    else
        docorner((int) cw->offx, cw->maxrow + 1);
}

STATIC_OVL void
free_window_info(
    struct WinDesc *cw,
    boolean free_data)
{
    int i;

    if (cw->data) {
        if (cw == g_wins[WIN_MESSAGE] && cw->rows > cw->maxrow)
            cw->maxrow = cw->rows; /* topl data */
        for (i = 0; i < cw->maxrow; i++)
            if (cw->data[i]) {
                free((genericptr_t) cw->data[i]);
                cw->data[i] = (char *) 0;
                if (cw->datlen)
                    cw->datlen[i] = 0;
            }
        if (free_data) {
            free((genericptr_t) cw->data);
            cw->data = (char **) 0;
            if (cw->datlen)
                free((genericptr_t) cw->datlen);
            cw->datlen = (short *) 0;
            cw->rows = 0;
        }
    }
    cw->maxrow = cw->maxcol = 0;
    if (cw->mlist) {
        tty_menu_item *temp;

        while ((temp = cw->mlist) != 0) {
            cw->mlist = cw->mlist->next;
            if (temp->str)
                free((genericptr_t) temp->str);
            free((genericptr_t) temp);
        }
    }
    if (cw->plist) {
        free((genericptr_t) cw->plist);
        cw->plist = 0;
    }
    cw->plist_size = cw->npages = cw->nitems = cw->how = 0;
    if (cw->morestr) {
        free((genericptr_t) cw->morestr);
        cw->morestr = 0;
    }
}

void
tty_clear_nhwindow(winid window)
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);
    g_uwpDisplay->lastwin = window;

    switch (cw->type) {
    case NHW_MESSAGE:
        if (g_uwpDisplay->toplin) {
            home();
            cl_end();
            if (cw->cury)
                docorner(1, cw->cury + 1);
            g_uwpDisplay->toplin = 0;
        }
        break;
    case NHW_STATUS:
        tty_curs(window, 1, 0);
        cl_end();
        tty_curs(window, 1, 1);
        cl_end();
        break;
    case NHW_MAP:
        /* cheap -- clear the whole thing and tell nethack to redraw botl */
        context.botlx = 1;
    /* fall into ... */
    case NHW_BASE:
        clear_screen();
        break;
    case NHW_MENU:
    case NHW_TEXT:
        if (cw->active)
            erase_menu_or_text(window, cw, TRUE);
        free_window_info(cw, FALSE);
        break;
    }
    cw->curx = cw->cury = 0;
}

/*ARGSUSED*/
void
tty_display_nhwindow(winid window, boolean blocking)
{
    register struct WinDesc *cw = 0;
    short s_maxcol;

    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);
    if (cw->flags & WIN_CANCELLED)
        return;
    g_uwpDisplay->lastwin = window;
    g_uwpDisplay->rawprint = 0;

    switch (cw->type) {
    case NHW_MESSAGE:
        if (g_uwpDisplay->toplin == 1) {
            more();
            g_uwpDisplay->toplin = 1; /* more resets this */
            tty_clear_nhwindow(window);
        } else
            g_uwpDisplay->toplin = 0;
        cw->curx = cw->cury = 0;
        if (!cw->active)
            iflags.window_inited = TRUE;
        break;
    case NHW_MAP:
        if (blocking) {
            if (!g_uwpDisplay->toplin)
                g_uwpDisplay->toplin = 1;
            tty_display_nhwindow(WIN_MESSAGE, TRUE);
            return;
        }
    case NHW_BASE:
        break;
    case NHW_TEXT:
        cw->maxcol = g_uwpDisplay->cols; /* force full-screen mode */
        /*FALLTHRU*/
    case NHW_MENU:
        cw->active = 1;
        /* cw->maxcol is a long, but its value is constrained to
           be <= g_uwpDisplay->cols, so is sure to fit within a short */
        s_maxcol = (short) cw->maxcol;
#ifdef H2344_BROKEN
        cw->offx = (cw->type == NHW_TEXT)
                       ? 0
                       : min(min(82, g_uwpDisplay->cols / 2),
                             g_uwpDisplay->cols - s_maxcol - 1);
#else
        /* avoid converting to uchar before calculations are finished */
        cw->offx = (uchar) max((int) 10,
                               (int) (g_uwpDisplay->cols - s_maxcol - 1));
#endif
        if (cw->offx < 0)
            cw->offx = 0;
        if (cw->type == NHW_MENU)
            cw->offy = 0;
        if (g_uwpDisplay->toplin == 1)
            tty_display_nhwindow(WIN_MESSAGE, TRUE);
#ifdef H2344_BROKEN
        if (cw->maxrow >= (int) g_uwpDisplay->rows
            || !iflags.menu_overlay)
#else
        if (cw->offx == 10 || cw->maxrow >= (int) g_uwpDisplay->rows
            || !iflags.menu_overlay)
#endif
        {
            cw->offx = 0;
            if (cw->offy || iflags.menu_overlay) {
                tty_curs(window, 1, 0);
                cl_eos();
            } else
                clear_screen();
            g_uwpDisplay->toplin = 0;
        } else {
            if (WIN_MESSAGE != WIN_ERR)
                tty_clear_nhwindow(WIN_MESSAGE);
        }

        if (cw->data || !cw->maxrow)
            process_text_window(window, cw);
        else
            process_menu_window(window, cw);
        break;
    }
    cw->active = 1;
}

void
tty_dismiss_nhwindow(winid window)
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        if (g_uwpDisplay->toplin)
            tty_display_nhwindow(WIN_MESSAGE, TRUE);
    /*FALLTHRU*/
    case NHW_STATUS:
    case NHW_BASE:
    case NHW_MAP:
        /*
         * these should only get dismissed when the game is going away
         * or suspending
         */
        tty_curs(BASE_WINDOW, 1, (int) g_uwpDisplay->rows - 1);
        cw->active = 0;
        break;
    case NHW_MENU:
    case NHW_TEXT:
        if (cw->active) {
            if (iflags.window_inited) {
                /* otherwise dismissing the text endwin after other windows
                 * are dismissed tries to redraw the map and panics.  since
                 * the whole reason for dismissing the other windows was to
                 * leave the ending window on the screen, we don't want to
                 * erase it anyway.
                 */
                erase_menu_or_text(window, cw, FALSE);
            }
            cw->active = 0;
        }
        break;
    }
    cw->flags = 0;
}

void
tty_destroy_nhwindow(winid window)
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);

    if (cw->active)
        tty_dismiss_nhwindow(window);

    if (cw->type == NHW_MESSAGE)
        iflags.window_inited = 0;

    if (cw->type == NHW_MAP)
        clear_screen();

    free_window_info(cw, TRUE);
    free((genericptr_t) cw);
    g_wins[window] = 0;
}

void
tty_curs(winid window, int x, int y)
{
    struct WinDesc *cw = 0;
    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);

    g_uwpDisplay->lastwin = window;

#if defined(USE_TILES) && defined(MSDOS)
    adjust_cursor_flags(cw);
#endif

    cw->curx = --x; /* column 0 is never used */
    cw->cury = y;

    assert(x >= 0 && x <= cw->cols);
    assert(y >= 0 && y < cw->rows);

    x += cw->offx;
    y += cw->offy;

    g_textGrid.SetCursor(Int2D(x, y));

}

void
tty_putsym(winid window, int x, int y, char ch)
{
    register struct WinDesc *cw = 0;

    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0)
        panic(winpanicstr, window);

    switch (cw->type) {
    case NHW_STATUS:
    case NHW_MAP:
    case NHW_BASE:
        tty_curs(window, x, y);
        win_putc(window, ch);
        break;
    case NHW_MESSAGE:
    case NHW_MENU:
    case NHW_TEXT:
        impossible("Can't putsym to window type %d", cw->type);
        break;
    }
}

STATIC_OVL const char *
compress_str(const char * str)
{
    static char cbuf[BUFSZ];

    /* compress out consecutive spaces if line is too long;
       topline wrapping converts space at wrap point into newline,
       we reverse that here */
    if ((int) strlen(str) >= CO || index(str, '\n')) {
        const char *in_str = str;
        char c, *outstr = cbuf, *outend = &cbuf[sizeof cbuf - 1];
        boolean was_space = TRUE; /* True discards all leading spaces;
                                     False would retain one if present */

        while ((c = *in_str++) != '\0' && outstr < outend) {
            if (c == '\n')
                c = ' ';
            if (was_space && c == ' ')
                continue;
            *outstr++ = c;
            was_space = (c == ' ');
        }
        if ((was_space && outstr > cbuf) || outstr == outend)
            --outstr; /* remove trailing space or make room for terminator */
        *outstr = '\0';
        str = cbuf;
    }
    return str;
}

void
tty_putstr(winid window, int attr, const char *str)
{
    register struct WinDesc *cw = 0;
    register char *ob;
    register const char *nb;
    register long i, j, n0;

    TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);

    /* Assume there's a real problem if the window is missing --
     * probably a panic message
     */
    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0) {
        tty_raw_print(str);
        return;
    }

    if (str == (const char *) 0
        || ((cw->flags & WIN_CANCELLED) && (cw->type != NHW_MESSAGE)))
        return;
    if (cw->type != NHW_MESSAGE)
        str = compress_str(str);

    g_uwpDisplay->lastwin = window;

    switch (cw->type) {
    case NHW_MESSAGE:
        /* really do this later */
#if defined(USER_SOUNDS) && defined(WIN32CON)
        play_sound_for_message(str);
#endif
        update_topl(str);
        break;

    case NHW_STATUS:
        ob = &cw->data[cw->cury][j = cw->curx];
        if (context.botlx)
            *ob = 0;
        if (!cw->cury && (int) strlen(str) >= CO) {
            /* the characters before "St:" are unnecessary */
            nb = index(str, ':');
            if (nb && nb > str + 2)
                str = nb - 2;
        }
        nb = str;
        for (i = cw->curx + 1, n0 = cw->cols; i < n0; i++, nb++) {
            if (!*nb) {
                if (*ob || context.botlx) {
                    /* last char printed may be in middle of line */
                    tty_curs(WIN_STATUS, i, cw->cury);
                    cl_end();
                }
                break;
            }
            if (*ob != *nb)
                tty_putsym(WIN_STATUS, i, cw->cury, *nb);
            if (*ob)
                ob++;
        }

        (void) strncpy(&cw->data[cw->cury][j], str, cw->cols - j - 1);
        cw->data[cw->cury][cw->cols - 1] = '\0'; /* null terminate */
#ifdef STATUS_VIA_WINDOWPORT
        if (!iflags.use_status_hilites) {
#endif
            cw->cury = (cw->cury + 1) % 2;
            cw->curx = 0;
#ifdef STATUS_VIA_WINDOWPORT
        }
#endif
        break;
    case NHW_MAP:
        tty_curs(window, cw->curx + 1, cw->cury);
        win_puts(window, str, TextColor::NoColor, useAttribute);
        cw->curx = 0;
        cw->cury++;
        break;
    case NHW_BASE:
        tty_curs(window, cw->curx + 1, cw->cury);
        win_puts(window, str, TextColor::NoColor, useAttribute);
        cw->curx = 0;
        cw->cury++;
        break;
    case NHW_MENU:
    case NHW_TEXT:
#ifdef H2344_BROKEN
        if (cw->type == NHW_TEXT
            && (cw->cury + cw->offy) == g_textGrid.GetDimensions().m_y - 1)
#else
        if (cw->type == NHW_TEXT && cw->cury == g_uwpDisplay->rows - 1)
#endif
        {
            /* not a menu, so save memory and output 1 page at a time */
            cw->maxcol = g_textGrid.GetDimensions().m_x; /* force full-screen mode */
            tty_display_nhwindow(window, TRUE);
            for (i = 0; i < cw->maxrow; i++)
                if (cw->data[i]) {
                    free((genericptr_t) cw->data[i]);
                    cw->data[i] = 0;
                }
            cw->maxrow = cw->cury = 0;
        }
        /* always grows one at a time, but alloc 12 at a time */
        if (cw->cury >= cw->rows) {
            char **tmp;

            cw->rows += 12;
            tmp = (char **) alloc(sizeof(char *) * (unsigned) cw->rows);
            for (i = 0; i < cw->maxrow; i++)
                tmp[i] = cw->data[i];
            if (cw->data)
                free((genericptr_t) cw->data);
            cw->data = tmp;

            for (i = cw->maxrow; i < cw->rows; i++)
                cw->data[i] = 0;
        }
        if (cw->data[cw->cury])
            free((genericptr_t) cw->data[cw->cury]);
        n0 = (long) strlen(str) + 1L;
        ob = cw->data[cw->cury] = (char *) alloc((unsigned) n0 + 1);
        *ob++ = (char) (attr + 1); /* avoid nuls, for convenience */
        Strcpy(ob, str);

        if (n0 > cw->maxcol)
            cw->maxcol = n0;
        if (++cw->cury > cw->maxrow)
            cw->maxrow = cw->cury;
        if (n0 > CO) {
            /* attempt to break the line */
            for (i = CO - 1; i && str[i] != ' ' && str[i] != '\n';)
                i--;
            if (i) {
                cw->data[cw->cury - 1][++i] = '\0';
                tty_putstr(window, attr, &str[i]);
            }
        }
        break;
    }
}

void
tty_display_file(const char *fname, boolean complain)
{
    dlb *f;
    char buf[BUFSZ];
    char *cr;

    tty_clear_nhwindow(WIN_MESSAGE);
    f = dlb_fopen(fname, "r");
    if (!f) {
        if (complain) {
            home();
            tty_mark_synch();
            tty_raw_print("");
            perror(fname);
            tty_wait_synch();
            pline("Cannot open \"%s\".", fname);
        } else if (u.ux)
            docrt();
    } else {
        winid datawin = tty_create_nhwindow(NHW_TEXT);
        boolean empty = TRUE;

        if (complain
#ifndef NO_TERMS
            && nh_CD
#endif
            ) {
            /* attempt to scroll text below map window if there's room */
            g_wins[datawin]->offy = g_wins[WIN_STATUS]->offy + 3;
            if ((int)g_wins[datawin]->offy + 12 > g_textGrid.GetDimensions().m_y)
                    g_wins[datawin]->offy = 0;
        }
        while (dlb_fgets(buf, BUFSZ, f)) {
            if ((cr = index(buf, '\n')) != 0)
                *cr = 0;
            if (index(buf, '\t') != 0)
                (void) tabexpand(buf);
            empty = FALSE;
            tty_putstr(datawin, 0, buf);
            if (g_wins[datawin]->flags & WIN_CANCELLED)
                break;
        }
        if (!empty)
            tty_display_nhwindow(datawin, FALSE);
        tty_destroy_nhwindow(datawin);
        (void) dlb_fclose(f);
    }
}

void
tty_update_inventory()
{
    return;
}

void
tty_mark_synch()
{
    // do nothing
}

void
tty_wait_synch()
{
    /* we just need to make sure all windows are synch'd */
    if (!g_uwpDisplay || g_uwpDisplay->rawprint) {
        getret();
        if (g_uwpDisplay)
            g_uwpDisplay->rawprint = 0;
    } else {
        tty_display_nhwindow(WIN_MAP, FALSE);
        if (g_uwpDisplay->inmore) {
            addtopl("--More--");
        } else if (g_uwpDisplay->inread > program_state.gameover) {
            /* this can only happen if we were reading and got interrupted */
            g_uwpDisplay->toplin = 3;
            /* do this twice; 1st time gets the Quit? message again */
            (void) tty_doprev_message();
            (void) tty_doprev_message();
            g_uwpDisplay->intr++;
        }
    }
}

void
docorner(
    int xmin, 
    int ymax)
{
    register int y;
    register struct WinDesc *cw = g_wins[WIN_MAP];

    for (y = 0; y < ymax; y++) {
        tty_curs(BASE_WINDOW, xmin, y); /* move cursor */
        cl_end();                       /* clear to end of line */
#ifdef CLIPPING
        if (y < (int) cw->offy || y + clipy > ROWNO)
            continue; /* only refresh board */
#if defined(USE_TILES) && defined(MSDOS)
        if (iflags.tile_view)
            row_refresh((xmin / 2) + clipx - ((int) cw->offx / 2), COLNO - 1,
                        y + clipy - (int) cw->offy);
        else
#endif
            row_refresh(xmin + clipx - (int) cw->offx, COLNO - 1,
                        y + clipy - (int) cw->offy);
#else
        if (y < cw->offy || y > ROWNO)
            continue; /* only refresh board  */
        row_refresh(xmin - (int) cw->offx, COLNO - 1, y - (int) cw->offy);
#endif
    }

    if (ymax >= (int) g_wins[WIN_STATUS]->offy) {
        /* we have wrecked the bottom line */
        context.botlx = 1;
        bot();
    }
}

#ifdef CLIPPING
void
setclipped()
{
    clipping = TRUE;
    clipx = clipy = 0;
    clipxmax = CO;
    clipymax = LI - 3;
}

void
tty_cliparound(int x, int y)
{
    extern boolean restoring;
    int oldx = clipx, oldy = clipy;

    if (!clipping)
        return;
    if (x < clipx + 5) {
        clipx = max(0, x - 20);
        clipxmax = clipx + CO;
    } else if (x > clipxmax - 5) {
        clipxmax = min(COLNO, clipxmax + 20);
        clipx = clipxmax - CO;
    }
    if (y < clipy + 2) {
        clipy = max(0, y - (clipymax - clipy) / 2);
        clipymax = clipy + (LI - 3);
    } else if (y > clipymax - 2) {
        clipymax = min(ROWNO, clipymax + (clipymax - clipy) / 2);
        clipy = clipymax - (LI - 3);
    }
    if (clipx != oldx || clipy != oldy) {
        if (on_level(&u.uz0, &u.uz) && !restoring)
            (void) doredraw();
    }
}
#endif /* CLIPPING */

/*
 *  tty_print_glyph
 *
 *  Print the glyph to the output device.  Don't flush the output device.
 *
 *  Since this is only called from show_glyph(), it is assumed that the
 *  position and glyph are always correct (checked there)!
 */

void
tty_print_glyph(
    winid window,
    xchar x, xchar y,
    int glyph,
    int bkglyph UNUSED)
{
    int ch;
    boolean reverse_on = FALSE;
    int color;
    unsigned special;

#ifdef CLIPPING
    if (clipping) {
        if (x <= clipx || y < clipy || x >= clipxmax || y >= clipymax)
            return;
    }
#endif
    /* map glyph to character and color */
    (void) mapglyph(glyph, &ch, &color, &special, x, y);

    /* Move the cursor. */
    tty_curs(window, x, y);

    /* must be after color check; term_end_color may turn off inverse too */
    if (((special & MG_PET) && iflags.hilite_pet)
        || ((special & MG_OBJPILE) && iflags.hilite_pile)
        || ((special & MG_DETECT) && iflags.use_inverse)
        || ((special & MG_BW_LAVA) && iflags.use_inverse)) {
        reverse_on = TRUE;
    }

    win_putc(window, ch, (TextColor)color, reverse_on ? TextAttribute::Inverse : TextAttribute::None);

}

void
tty_raw_print(const char *str)
{
    if (g_uwpDisplay)
        g_uwpDisplay->rawprint++;

#if defined(MICRO) || defined(WIN32CON)
    msmsg("%s\n", str);
#else
    xputs(str);
#endif
}

void
tty_raw_print_bold(const char *str)
{
    if (g_uwpDisplay)
        g_uwpDisplay->rawprint++;

#if defined(MICRO) || defined(WIN32CON)
    msmsg_bold("%s", str);
#else
    (void) fputs(str, stdout);
#endif
#if defined(MICRO) || defined(WIN32CON)
    msmsg("\n");
#else
    xputs("");
#endif
}

void really_move_cursor()
{
    // do nothing
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
tty_nhgetch()
{
    int i;

    /* Note: if raw_print() and wait_synch() get called to report terminal
     * initialization problems, then g_wins[] and g_uwpDisplay might not be
     * available yet.  Such problems will probably be fatal before we get
     * here, but validate those pointers just in case...
     */
    if (WIN_MESSAGE != WIN_ERR && g_wins[WIN_MESSAGE])
        g_wins[WIN_MESSAGE]->flags &= ~WIN_STOP;
    i = tgetch();
    if (!i)
        i = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    else if (i == EOF)
        i = '\033'; /* same for EOF */
    if (g_uwpDisplay && g_uwpDisplay->toplin == 1)
        g_uwpDisplay->toplin = 2;
    return i;
}

/*
 * return a key, or 0, in which case a mouse button was pressed
 * mouse events should be returned as character postitions in the map window.
 * Since normal tty's don't have mice, just return a key.
 */
/*ARGSUSED*/
int
tty_nh_poskey(int *x, int *y, int *mod)
{
#if defined(WIN32CON)
    int i;
    /* Note: if raw_print() and wait_synch() get called to report terminal
     * initialization problems, then g_wins[] and g_uwpDisplay might not be
     * available yet.  Such problems will probably be fatal before we get
     * here, but validate those pointers just in case...
     */
    if (WIN_MESSAGE != WIN_ERR && g_wins[WIN_MESSAGE])
        g_wins[WIN_MESSAGE]->flags &= ~WIN_STOP;
    i = ntposkey(x, y, mod);
    if (!i && mod && (*mod == 0 || *mod == EOF))
        i = '\033'; /* map NUL or EOF to ESC, nethack doesn't expect either */
    if (g_uwpDisplay && g_uwpDisplay->toplin == 1)
        g_uwpDisplay->toplin = 2;
    return i;
#else /* !WIN32CON */
    nhUse(x);
    nhUse(y);
    nhUse(mod);

    return tty_nhgetch();
#endif /* ?WIN32CON */
}

char morc = 0; /* tell the outside world what char you chose */
STATIC_DCL boolean FDECL(ext_cmd_getlin_hook, (char *));

typedef boolean FDECL((*getlin_hook_proc), (char *));

STATIC_DCL void FDECL(hooked_tty_getlin,
(const char *, char *, getlin_hook_proc));
extern int NDECL(extcmd_via_menu); /* cmd.c */

extern char erase_char, kill_char; /* from appropriate tty.c file */

                                   /*
                                   * Read a line closed with '\n' into the array char bufp[BUFSZ].
                                   * (The '\n' is not stored. The string is closed with a '\0'.)
                                   * Reading can be interrupted by an escape ('\033') - now the
                                   * resulting string is "\033".
                                   */
void
tty_getlin(
    const char *query,
    char *bufp)
{
    hooked_tty_getlin(query, bufp, (getlin_hook_proc)0);
}

STATIC_OVL void
hooked_tty_getlin(
    const char *query,
    char *bufp,
    getlin_hook_proc hook)
{
    register char *obufp = bufp;
    register int c;
    struct WinDesc *cw = g_wins[WIN_MESSAGE];
    boolean doprev = 0;

    if (g_uwpDisplay->toplin == 1 && !(cw->flags & WIN_STOP))
        more();
    cw->flags &= ~WIN_STOP;
    g_uwpDisplay->toplin = 3; /* special prompt state */
    g_uwpDisplay->inread++;
    pline("%s ", query);
    *obufp = 0;
    for (;;) {
        Strcat(strcat(strcpy(toplines, query), " "), obufp);
        c = pgetchar();
        if (c == '\033' || c == EOF) {
            if (c == '\033' && obufp[0] != '\0') {
                obufp[0] = '\0';
                bufp = obufp;
                tty_clear_nhwindow(WIN_MESSAGE);
                cw->maxcol = cw->maxrow;
                addtopl(query);
                addtopl(" ");
                addtopl(obufp);
            }
            else {
                obufp[0] = '\033';
                obufp[1] = '\0';
                break;
            }
        }
        if (g_uwpDisplay->intr) {
            g_uwpDisplay->intr--;
            *bufp = 0;
        }
        if (c == '\020') { /* ctrl-P */
            if (iflags.prevmsg_window != 's') {
                int sav = g_uwpDisplay->inread;
                g_uwpDisplay->inread = 0;
                (void)tty_doprev_message();
                g_uwpDisplay->inread = sav;
                tty_clear_nhwindow(WIN_MESSAGE);
                cw->maxcol = cw->maxrow;
                addtopl(query);
                addtopl(" ");
                *bufp = 0;
                addtopl(obufp);
            }
            else {
                if (!doprev)
                    (void) tty_doprev_message(); /* need two initially */
                (void)tty_doprev_message();
                doprev = 1;
                continue;
            }
        }
        else if (doprev && iflags.prevmsg_window == 's') {
            tty_clear_nhwindow(WIN_MESSAGE);
            cw->maxcol = cw->maxrow;
            doprev = 0;
            addtopl(query);
            addtopl(" ");
            *bufp = 0;
            addtopl(obufp);
        }
        if (c == erase_char || c == '\b') {
            if (bufp != obufp) {
#ifdef NEWAUTOCOMP
                char *i;

#endif /* NEWAUTOCOMP */
                bufp--;
#ifndef NEWAUTOCOMP
                putsyms("\b \b", TextColor::NoColor, TextAttribute::None); /* putsym converts \b */
#else                             /* NEWAUTOCOMP */
                putsyms("\b", TextColor::NoColor, TextAttribute::None);
                for (i = bufp; *i; ++i)
                    putsyms(" ", TextColor::NoColor, TextAttribute::None);
                for (; i > bufp; --i)
                    putsyms("\b", TextColor::NoColor, TextAttribute::None);
                *bufp = 0;
#endif                            /* NEWAUTOCOMP */
            }
            else
                tty_nhbell();
#if defined(apollo)
        }
        else if (c == '\n' || c == '\r') {
#else
        }
        else if (c == '\n') {
#endif
#ifndef NEWAUTOCOMP
            *bufp = 0;
#endif /* not NEWAUTOCOMP */
            break;
        }
        else if (' ' <= (unsigned char)c && c != '\177'
            && (bufp - obufp < BUFSZ - 1 && bufp - obufp < COLNO)) {
            /* avoid isprint() - some people don't have it
            ' ' is not always a printing char */
#ifdef NEWAUTOCOMP
            char *i = eos(bufp);

#endif /* NEWAUTOCOMP */
            *bufp = c;
            bufp[1] = 0;
            putsyms(bufp, TextColor::NoColor, TextAttribute::None);
            bufp++;
            if (hook && (*hook)(obufp)) {
                putsyms(bufp, TextColor::NoColor, TextAttribute::None);
#ifndef NEWAUTOCOMP
                bufp = eos(bufp);
#else  /* NEWAUTOCOMP */
                /* pointer and cursor left where they were */
                for (i = bufp; *i; ++i)
                    putsyms("\b", TextColor::NoColor, TextAttribute::None);
            }
            else if (i > bufp) {
                char *s = i;

                /* erase rest of prior guess */
                for (; i > bufp; --i)
                    putsyms(" ", TextColor::NoColor, TextAttribute::None);
                for (; s > bufp; --s)
                    putsyms("\b", TextColor::NoColor, TextAttribute::None);
#endif /* NEWAUTOCOMP */
            }
        }
        else if (c == kill_char || c == '\177') { /* Robert Viduya */
                                                  /* this test last - @ might be the kill_char */
#ifndef NEWAUTOCOMP
            while (bufp != obufp) {
                bufp--;
                putsyms("\b \b");
            }
#else  /* NEWAUTOCOMP */
            for (; *bufp; ++bufp)
                putsyms(" ", TextColor::NoColor, TextAttribute::None);
            for (; bufp != obufp; --bufp)
                putsyms("\b \b", TextColor::NoColor, TextAttribute::None);
            *bufp = 0;
#endif /* NEWAUTOCOMP */
        }
        else
            tty_nhbell();
    }
    g_uwpDisplay->toplin = 2; /* nonempty, no --More-- required */
    g_uwpDisplay->inread--;
    clear_nhwindow(WIN_MESSAGE); /* clean up after ourselves */
}

void
xwaitforspace(
    register const char *s) /* chars allowed besides return */
{
    register int c, x = g_uwpDisplay ? (int)g_uwpDisplay->dismiss_more : '\n';

    morc = 0;
    while (
#ifdef HANGUPHANDLING
        !program_state.done_hup &&
#endif
        (c = tty_nhgetch()) != EOF) {
        if (c == '\n')
            break;

        if (iflags.cbreak) {
            if (c == '\033') {
                if (g_uwpDisplay)
                    g_uwpDisplay->dismiss_more = 1;
                morc = '\033';
                break;
            }
            if ((s && index(s, c)) || c == x) {
                morc = (char)c;
                break;
            }
            tty_nhbell();
        }
    }
}

/*
* Implement extended command completion by using this hook into
* tty_getlin.  Check the characters already typed, if they uniquely
* identify an extended command, expand the string to the whole
* command.
*
* Return TRUE if we've extended the string at base.  Otherwise return FALSE.
* Assumptions:
*
*	+ we don't change the characters that are already in base
*	+ base has enough room to hold our string
*/
STATIC_OVL boolean
ext_cmd_getlin_hook(
    char *base)
{
    int oindex, com_index;

    com_index = -1;
    for (oindex = 0; extcmdlist[oindex].ef_txt != (char *)0; oindex++) {
        if ((extcmdlist[oindex].flags & AUTOCOMPLETE)
            && !(!wizard && (extcmdlist[oindex].flags & WIZMODECMD))
            && !strncmpi(base, extcmdlist[oindex].ef_txt, strlen(base))) {
            if (com_index == -1) /* no matches yet */
                com_index = oindex;
            else /* more than 1 match */
                return FALSE;
        }
    }
    if (com_index >= 0) {
        Strcpy(base, extcmdlist[com_index].ef_txt);
        return TRUE;
    }

    return FALSE; /* didn't match anything */
}

/*
* Read in an extended command, doing command line completion.  We
* stop when we have found enough characters to make a unique command.
*/
int
tty_get_ext_cmd()
{
    int i;
    char buf[BUFSZ];

    if (iflags.extmenu)
        return extcmd_via_menu();
    /* maybe a runtime option? */
    /* hooked_tty_getlin("#", buf, flags.cmd_comp ? ext_cmd_getlin_hook :
    * (getlin_hook_proc) 0); */
    hooked_tty_getlin("#", buf, in_doagain ? (getlin_hook_proc)0
        : ext_cmd_getlin_hook);
    (void)mungspaces(buf);
    if (buf[0] == 0 || buf[0] == '\033')
        return -1;

    for (i = 0; extcmdlist[i].ef_txt != (char *)0; i++)
        if (!strcmpi(buf, extcmdlist[i].ef_txt))
            break;

    if (!in_doagain) {
        int j;
        for (j = 0; buf[j]; j++)
            savech(buf[j]);
        savech('\n');
    }

    if (extcmdlist[i].ef_txt == (char *)0) {
        pline("%s: unknown extended command.", buf);
        i = -1;
    }

    return i;
}

#ifndef C /* this matches src/cmd.c */
#define C(c) (0x1f & (c))
#endif

STATIC_DCL void FDECL(redotoplin, (const char *));
static void topl_putsym(char c, TextColor color, TextAttribute attribute);
STATIC_DCL void NDECL(remember_topl);
STATIC_DCL void FDECL(removetopl, (int));
STATIC_DCL void FDECL(msghistory_snapshot, (BOOLEAN_P));
STATIC_DCL void FDECL(free_msghistory_snapshot, (BOOLEAN_P));

int
tty_doprev_message()
{
    register struct WinDesc *cw = g_wins[WIN_MESSAGE];

    winid prevmsg_win;
    int i;
    if ((iflags.prevmsg_window != 's')
        && !g_uwpDisplay->inread) {           /* not single */
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
        }
        else if (iflags.prevmsg_window == 'c') { /* combination */
            do {
                morc = 0;
                if (cw->maxcol == cw->maxrow) {
                    g_uwpDisplay->dismiss_more = C('p'); /* ^P ok at --More-- */
                    redotoplin(toplines);
                    cw->maxcol--;
                    if (cw->maxcol < 0)
                        cw->maxcol = cw->rows - 1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                }
                else if (cw->maxcol == (cw->maxrow - 1)) {
                    g_uwpDisplay->dismiss_more = C('p'); /* ^P ok at --More-- */
                    redotoplin(cw->data[cw->maxcol]);
                    cw->maxcol--;
                    if (cw->maxcol < 0)
                        cw->maxcol = cw->rows - 1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                }
                else {
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

            } while (morc == C('p'));
            g_uwpDisplay->dismiss_more = 0;
        }
        else { /* reversed */
            morc = 0;
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
            g_uwpDisplay->dismiss_more = 0;
        }
    }
    else if (iflags.prevmsg_window == 's') { /* single */
        g_uwpDisplay->dismiss_more = C('p'); /* <ctrl/P> allowed at --More-- */
        do {
            morc = 0;
            if (cw->maxcol == cw->maxrow)
                redotoplin(toplines);
            else if (cw->data[cw->maxcol])
                redotoplin(cw->data[cw->maxcol]);
            cw->maxcol--;
            if (cw->maxcol < 0)
                cw->maxcol = cw->rows - 1;
            if (!cw->data[cw->maxcol])
                cw->maxcol = cw->maxrow;
        } while (morc == C('p'));
        g_uwpDisplay->dismiss_more = 0;
    }
    return 0;
}

STATIC_OVL void
redotoplin(
    const char *str)
{
    register struct WinDesc *cw = g_wins[WIN_MESSAGE];

    assert(cw != NULL);

    int otoplin = g_uwpDisplay->toplin;

    home();

    cw->curx = 0;
    cw->cury = 0;

    putsyms(str, TextColor::NoColor, TextAttribute::None);
    cl_end();
    g_uwpDisplay->toplin = 1;
    if (cw->cury != 0 && otoplin != 3)
        more();
}

STATIC_OVL void
remember_topl()
{
    register struct WinDesc *cw = g_wins[WIN_MESSAGE];
    int idx = cw->maxrow;
    unsigned len = strlen(toplines) + 1;

    if ((cw->flags & WIN_LOCKHISTORY) || !*toplines)
        return;

    if (len > (unsigned)cw->datlen[idx]) {
        if (cw->data[idx])
            free(cw->data[idx]);
        len += (8 - (len & 7)); /* pad up to next multiple of 8 */
        cw->data[idx] = (char *)alloc(len);
        cw->datlen[idx] = (short)len;
    }
    Strcpy(cw->data[idx], toplines);
    *toplines = '\0';
    cw->maxcol = cw->maxrow = (idx + 1) % cw->rows;
}

void
addtopl(
    const char *s)
{
    register struct WinDesc *cw = g_wins[WIN_MESSAGE];

    tty_curs(BASE_WINDOW, cw->curx + 1, cw->cury);
    putsyms(s, TextColor::NoColor, TextAttribute::None);
    cl_end();
    g_uwpDisplay->toplin = 1;
}

void
more()
{
    struct WinDesc *cw = g_wins[WIN_MESSAGE];

    /* avoid recursion -- only happens from interrupts */
    if (g_uwpDisplay->inmore++)
        return;

    if (g_uwpDisplay->toplin) {
        tty_curs(BASE_WINDOW, cw->curx + 1, cw->cury);
        if (cw->curx >= CO - 8)
            topl_putsym('\n', TextColor::NoColor, TextAttribute::None);
    }

    putsyms(defmorestr, TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);

    xwaitforspace("\033 ");

    if (morc == '\033')
        cw->flags |= WIN_STOP;

    if (g_uwpDisplay->toplin && cw->cury) {
        docorner(1, cw->cury + 1);
        cw->curx = cw->cury = 0;
        home();
    }
    else if (morc == '\033') {
        cw->curx = cw->cury = 0;
        home();
        cl_end();
    }
    g_uwpDisplay->toplin = 0;
    g_uwpDisplay->inmore = 0;
}

void
update_topl(
    const char *bp)
{
    register char *tl, *otl;
    register int n0;
    int notdied = 1;
    struct WinDesc *cw = g_wins[WIN_MESSAGE];

    /* If there is room on the line, print message on same line */
    /* But messages like "You die..." deserve their own line */
    n0 = strlen(bp);
    if ((g_uwpDisplay->toplin == 1 || (cw->flags & WIN_STOP)) && cw->cury == 0
        && n0 + (int)strlen(toplines) + 3 < CO - 8 /* room for --More-- */
        && (notdied = strncmp(bp, "You die", 7)) != 0) {
        Strcat(toplines, "  ");
        Strcat(toplines, bp);
        cw->curx += 2;
        if (!(cw->flags & WIN_STOP))
            addtopl(bp);
        return;
    }
    else if (!(cw->flags & WIN_STOP)) {
        if (g_uwpDisplay->toplin == 1) {
            more();
        }
        else if (cw->cury) { /* for when flags.toplin == 2 && cury > 1 */
            docorner(1, cw->cury + 1); /* reset cury = 0 if redraw screen */
            cw->curx = cw->cury = 0;   /* from home--cls() & docorner(1,n) */
        }
    }
    remember_topl();
    (void)strncpy(toplines, bp, TBUFSZ);
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
        cw->flags &= ~WIN_STOP;
    if (!(cw->flags & WIN_STOP))
        redotoplin(toplines);
}

STATIC_OVL
void
topl_putsym(char c, TextColor color, TextAttribute attribute)
{
    register struct WinDesc *cw = g_wins[WIN_MESSAGE];

    if (cw == (struct WinDesc *) 0)
        panic("Putsym window MESSAGE nonexistant");

    switch (c) {
    case '\b':
        win_putc(WIN_MESSAGE, '\b');
        return;
    case '\n':
        cl_end();
        win_putc(WIN_MESSAGE, '\n');
        break;
    default:
        if (cw->curx + cw->offx == CO - 1)
            topl_putsym('\n', TextColor::NoColor, TextAttribute::None); /* 1 <= curx < CO; avoid CO */
        win_putc(WIN_MESSAGE, c);
    }

    if (cw->curx == 0)
        cl_end();
}

void
putsyms(const char *str, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute)
{
    while (*str)
        topl_putsym(*str++, textColor, textAttribute);
}

STATIC_OVL void
removetopl(
    int n)
{
    /* assume addtopl() has been done, so g_uwpDisplay->toplin is already set */
    while (n-- > 0)
        putsyms("\b \b", TextColor::NoColor, TextAttribute::None);
}

extern char erase_char; /* from xxxtty.c; don't need kill_char */

char
tty_yn_function(
    const char *query, 
    const char *resp,
    char def)
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
    struct WinDesc *cw = g_wins[WIN_MESSAGE];
    boolean doprev = 0;
    char prompt[BUFSZ];

    if (g_uwpDisplay->toplin == 1 && !(cw->flags & WIN_STOP))
        more();
    cw->flags &= ~WIN_STOP;
    g_uwpDisplay->toplin = 3; /* special prompt state */
    g_uwpDisplay->inread++;
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
        (void)strncpy(prompt, query, QBUFSZ - 1);
        prompt[QBUFSZ - 1] = '\0';
        Sprintf(eos(prompt), " [%s]", respbuf);
        if (def)
            Sprintf(eos(prompt), " (%c)", def);
        /* not pline("%s ", prompt);
        trailing space is wanted here in case of reprompt */
        Strcat(prompt, " ");
        pline("%s", prompt);
    }
    else {
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
                int sav = g_uwpDisplay->inread;
                g_uwpDisplay->inread = 0;
                (void)tty_doprev_message();
                g_uwpDisplay->inread = sav;
                tty_clear_nhwindow(WIN_MESSAGE);
                cw->maxcol = cw->maxrow;
                addtopl(prompt);
            }
            else {
                if (!doprev)
                    (void) tty_doprev_message(); /* need two initially */
                (void)tty_doprev_message();
                doprev = 1;
            }
            q = '\0'; /* force another loop iteration */
            continue;
        }
        else if (doprev) {
            /* BUG[?]: this probably ought to check whether the
            character which has just been read is an acceptable
            response; if so, skip the reprompt and use it. */
            tty_clear_nhwindow(WIN_MESSAGE);
            cw->maxcol = cw->maxrow;
            doprev = 0;
            addtopl(prompt);
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
        }
        else if (index(quitchars, q)) {
            q = def;
            break;
        }
        if (!index(resp, q) && !digit_ok) {
            tty_nhbell();
            q = (char)0;
        }
        else if (q == '#' || digit_ok) {
            char z, digit_string[2];
            int n_len = 0;
            long value = 0;
            addtopl("#"), n_len++;
            digit_string[1] = '\0';
            if (q != '#') {
                digit_string[0] = q;
                addtopl(digit_string), n_len++;
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
                    addtopl(digit_string), n_len++;
                }
                else if (z == 'y' || index(quitchars, z)) {
                    if (z == '\033')
                        value = -1; /* abort */
                    z = '\n';       /* break */
                }
                else if (z == erase_char || z == '\b') {
                    if (n_len <= 1) {
                        value = -1;
                        break;
                    }
                    else {
                        value /= 10;
                        removetopl(1), n_len--;
                    }
                }
                else {
                    value = -1; /* abort */
                    tty_nhbell();
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
        addtopl(rtmp);
    }
clean_up:
    g_uwpDisplay->inread--;
    g_uwpDisplay->toplin = 2;
    if (g_uwpDisplay->intr)
        g_uwpDisplay->intr--;
    if (g_wins[WIN_MESSAGE]->cury)
        tty_clear_nhwindow(WIN_MESSAGE);

    return q;
}

/* shared by tty_getmsghistory() and tty_putmsghistory() */
static char **snapshot_mesgs = 0;

/* collect currently available message history data into a sequential array;
optionally, purge that data from the active circular buffer set as we go */
STATIC_OVL void
msghistory_snapshot(
    boolean purge) /* clear message history buffer as we copy it */
{
    char *mesg;
    int i, inidx, outidx;
    struct WinDesc *cw;

    /* paranoia (too early or too late panic save attempt?) */
    if (WIN_MESSAGE == WIN_ERR || !g_wins[WIN_MESSAGE])
        return;
    cw = g_wins[WIN_MESSAGE];

    /* flush toplines[], moving most recent message to history */
    remember_topl();

    /* for a passive snapshot, we just copy pointers, so can't allow further
    history updating to take place because that could clobber them */
    if (!purge)
        cw->flags |= WIN_LOCKHISTORY;

    snapshot_mesgs = (char **)alloc((cw->rows + 1) * sizeof(char *));
    outidx = 0;
    inidx = cw->maxrow;
    for (i = 0; i < cw->rows; ++i) {
        snapshot_mesgs[i] = (char *)0;
        mesg = cw->data[inidx];
        if (mesg && *mesg) {
            snapshot_mesgs[outidx++] = mesg;
            if (purge) {
                /* we're taking this pointer away; subsequest history
                updates will eventually allocate a new one to replace it */
                cw->data[inidx] = (char *)0;
                cw->datlen[inidx] = 0;
            }
        }
        inidx = (inidx + 1) % cw->rows;
    }
    snapshot_mesgs[cw->rows] = (char *)0; /* sentinel */

                                          /* for a destructive snapshot, history is now completely empty */
    if (purge)
        cw->maxcol = cw->maxrow = 0;
}

/* release memory allocated to message history snapshot */
STATIC_OVL void
free_msghistory_snapshot(
    boolean purged) /* True: took history's pointers, False: just cloned them */
{
    if (snapshot_mesgs) {
        /* snapshot pointers are no longer in use */
        if (purged) {
            int i;

            for (i = 0; snapshot_mesgs[i]; ++i)
                free((genericptr_t)snapshot_mesgs[i]);
        }

        free((genericptr_t)snapshot_mesgs), snapshot_mesgs = (char **)0;

        /* history can resume being updated at will now... */
        if (!purged)
            g_wins[WIN_MESSAGE]->flags &= ~WIN_LOCKHISTORY;
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
tty_getmsghistory(boolean init)
{
    static int nxtidx;
    char *nextmesg;
    char *result = 0;

    if (init) {
        msghistory_snapshot(FALSE);
        nxtidx = 0;
    }

    if (snapshot_mesgs) {
        nextmesg = snapshot_mesgs[nxtidx++];
        if (nextmesg) {
            result = (char *)nextmesg;
        }
        else {
            free_msghistory_snapshot(FALSE);
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
tty_putmsghistory(
    const char *msg,
    boolean restoring_msghist)
{
    static boolean initd = FALSE;
    int idx;

    if (restoring_msghist && !initd) {
        /* we're restoring history from the previous session, but new
        messages have already been issued this session ("Restoring...",
        for instance); collect current history (ie, those new messages),
        and also clear it out so that nothing will be present when the
        restored ones are being put into place */
        msghistory_snapshot(TRUE);
        initd = TRUE;
    }

    if (msg) {
        /* move most recent message to history, make this become most recent
        */
        remember_topl();
        Strcpy(toplines, msg);
    }
    else if (snapshot_mesgs) {
        /* done putting arbitrary messages in; put the snapshot ones back */
        for (idx = 0; snapshot_mesgs[idx]; ++idx) {
            remember_topl();
            Strcpy(toplines, snapshot_mesgs[idx]);
        }
        /* now release the snapshot */
        free_msghistory_snapshot(TRUE);
        initd = FALSE; /* reset */
    }
}

void tty_number_pad(int state)
{
    // do nothing for now
}

void
tty_start_screen()
{
    if (iflags.num_pad)
        tty_number_pad(1); /* make keypad send digits */
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
tty_delay_output()
{
    // Delay 50ms
    Sleep(50);
}

int
ntposkey(int *x, int *y, int * mod)
{
    really_move_cursor();

    if (program_state.done_hup)
        return '\033';

    Event e;

    while (e.m_type == Event::Type::Undefined ||
        (e.m_type == Event::Type::Mouse && !iflags.wc_mouse_support) ||
        (e.m_type == Event::Type::ScanCode && MapScanCode(e) == 0))
        e = g_eventQueue.PopFront();

    if (e.m_type == Event::Type::Char)
    {
        if (e.m_char == EOF)
        {
            hangup(0);
            e.m_char = '\033';
        }

        return e.m_char;
    }
    else if (e.m_type == Event::Type::Mouse)
    {
        *x = e.m_pos.m_x;
        *y = e.m_pos.m_y;
        *mod = (e.m_tap == Event::Tap::Left ? CLICK_1 : CLICK_2);

        return 0;
    }
    else
    {
        assert(e.m_type == Event::Type::ScanCode);
        return MapScanCode(e);
    }
}


void getreturn(const char * str)
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

    if (g_wins[BASE_WINDOW])
        win_puts(BASE_WINDOW, buf);
    else
        g_textGrid.Putstr(TextColor::NoColor, TextAttribute::None, buf);

    if (g_uwpDisplay)
        curs(BASE_WINDOW, g_textGrid.GetCursor().m_x + 1, g_textGrid.GetCursor().m_y);

    VA_END();
    return;
}

/* This should probably be removed (replaced) since it is used in only one spot */
void msmsg_bold
VA_DECL(const char *, fmt)
{
    char buf[ROWNO * COLNO]; /* worst case scenario */
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    Vsprintf(buf, fmt, VA_ARGS);

    if (g_wins[BASE_WINDOW])
        win_puts(BASE_WINDOW, buf, TextColor::NoColor, TextAttribute::Bold);
    else
        g_textGrid.Putstr(TextColor::NoColor, TextAttribute::Bold, buf);

    if (g_uwpDisplay)
        curs(BASE_WINDOW, g_textGrid.GetCursor().m_x + 1, g_textGrid.GetCursor().m_y);

    VA_END();
    return;
}

void
home()
{
    g_textGrid.SetCursor(Int2D(0, 0));
}

void
cl_eos()
{
    int x = g_textGrid.GetCursor().m_x;
    int y = g_textGrid.GetCursor().m_y;

    int cx = g_textGrid.GetDimensions().m_x - x;
    int cy = (g_textGrid.GetDimensions().m_y - (y + 1)) * g_textGrid.GetDimensions().m_x;

    g_textGrid.Put(x, y, TextCell(), cx + cy);
    g_textGrid.SetCursor(Int2D(x, y));

    tty_curs(BASE_WINDOW, x + 1, y);
}

void
cl_end()
{
    Int2D cursor = g_textGrid.GetCursor();
    g_textGrid.Put(TextCell(), g_textGrid.GetDimensions().m_x - cursor.m_x);
    g_textGrid.SetCursor(cursor);
}

void clear_screen(void)
{
    g_textGrid.Clear();
    home();
}

void
xputc_core(
    char ch,
    TextColor textColor = TextColor::NoColor, 
    TextAttribute textAttribute = TextAttribute::None)
{
    Int2D cursor = g_textGrid.GetCursor();

    switch (ch) {
    case '\n':
        if (cursor.m_y < (g_textGrid.GetDimensions().m_y - 1))
            cursor.m_y++;
        /* fall through */
    case '\r':
        cursor.m_x = 0;

        g_textGrid.SetCursor(cursor);
        break;

    case '\b':
        /* should we perhaps support blanking out character at current position? */
        if (cursor.m_x > 0) {
            cursor.m_x--;

            g_textGrid.SetCursor(cursor);
        }
        break;

    default:

        TextCell textCell(textColor, textAttribute, ch);

        g_textGrid.Put(cursor.m_x, cursor.m_y, textCell, 1);
    }
}

void uwp_puts(const char *s)
{
    win_puts(BASE_WINDOW, s);
}

void win_putc(
    winid window,
    char ch,
    Nethack::TextColor textColor,
    Nethack::TextAttribute textAttribute)
{
    struct WinDesc *cw = g_wins[window];

    assert(cw != NULL);

    int x = cw->curx + cw->offx;
    int y = cw->cury + cw->offy;

    g_textGrid.Put(x, y, TextCell(textColor, textAttribute, ch));

    cw->curx = g_textGrid.GetCursor().m_x - cw->offx;
    cw->cury = g_textGrid.GetCursor().m_y - cw->offy;

}

void win_puts(
    winid window,
    const char *s,
    TextColor textColor,
    TextAttribute textAttribute)
{
    struct WinDesc *cw = g_wins[window];

    assert(cw != NULL);

    int x = cw->curx + cw->offx;
    int y = cw->cury + cw->offy;

    g_textGrid.Putstr(textColor, textAttribute, s);

    cw->curx = g_textGrid.GetCursor().m_x - cw->offx;
    cw->cury = g_textGrid.GetCursor().m_y - cw->offy;
}



} /* extern "C" */
