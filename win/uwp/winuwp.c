/* NetHack 3.6	winuwp.c	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2017                                */
/* NetHack may be freely redistributed.  See license for details. */

/* It's still not clear I've caught all the cases for H2344.  #undef this
* to back out the changes. */
#define H2344_BROKEN

#include "hack.h"

#ifdef UWP_GRAPHICS

#include "dlb.h"

#ifndef NO_TERMS
#error NO_TERMS must be defined
#endif

#ifndef UWP_GRAPHICS
#define UWP_GRAPHICS must be defined
#endif

#include "winuwp.h"

#ifdef UWP_TILES_ESCCODES
extern short glyph2tile[];
#define TILE_ANSI_COMMAND 'z'
#define AVTC_GLYPH_START   0
#define AVTC_GLYPH_END     1
#define AVTC_SELECT_WINDOW 2
#define AVTC_INLINE_SYNC   3
#endif

extern char mapped_menu_cmds[]; /* from options.c */

                                /* this is only needed until uwp_status_* routines are written */
extern NEARDATA winid WIN_STATUS;

/* Interface definition, for windows.c */
struct window_procs uwp_procs = {
    "uwp",
#ifdef MSDOS
    WC_TILED_MAP | WC_ASCII_MAP |
#endif
    WC_MOUSE_SUPPORT |
    WC_COLOR | WC_HILITE_PET | WC_INVERSE | WC_EIGHT_BIT_IN,
#if defined(SELECTSAVED)
    WC2_SELECTSAVED |
#endif
    WC2_DARKGRAY,
    uwp_init_nhwindows, uwp_player_selection, uwp_askname, uwp_get_nh_event,
    uwp_exit_nhwindows, uwp_suspend_nhwindows, uwp_resume_nhwindows,
    uwp_create_nhwindow, uwp_clear_nhwindow, uwp_display_nhwindow,
    uwp_destroy_nhwindow, uwp_curs, uwp_putstr, genl_putmixed,
    uwp_display_file, uwp_start_menu, uwp_add_menu, uwp_end_menu,
    uwp_select_menu, uwp_message_menu, uwp_update_inventory, uwp_mark_synch,
    uwp_wait_synch,
#ifdef CLIPPING
    uwp_cliparound,
#endif
#ifdef POSITIONBAR
    uwp_update_positionbar,
#endif
    uwp_print_glyph, uwp_raw_print, uwp_raw_print_bold, uwp_nhgetch,
    uwp_nh_poskey, uwp_nhbell, uwp_doprev_message, uwp_yn_function,
    uwp_getlin, uwp_get_ext_cmd, uwp_number_pad, uwp_delay_output,
#ifdef CHANGE_COLOR /* the Mac uses a palette device */
    uwp_change_color,
#ifdef MAC
    uwp_change_background, set_uwp_font_name,
#endif
    uwp_get_color_string,
#endif

    /* other defs that really should go away (they're uwp specific) */
    uwp_start_screen, uwp_end_screen, genl_outrip,
#if defined(WIN32)
    nttty_preference_update,
#else
    genl_preference_update,
#endif
    uwp_getmsghistory, uwp_putmsghistory,
#ifdef STATUS_VIA_WINDOWPORT
    uwp_status_init,
    genl_status_finish, genl_status_enablefield,
    uwp_status_update,
#ifdef STATUS_HILITES
    uwp_status_threshold,
#endif
#endif
    genl_can_suspend_yes,
};

static int uwp_maxwin = 0; /* number of windows in use */
winid UWP_BASE_WINDOW;
struct UwpWinDesc *uwp_wins[UWP_MAXWIN];
struct UwpDisplayDesc *uwpDisplay; /* the uwp display descriptor */

extern void FDECL(uwp_cmov, (int, int));
extern void FDECL(uwp_nocmov, (int, int));

extern void uwp_settty(const char *s);
extern int uwp_tgetch(void);
extern int uwp_ntposkey(int *x, int *y, int * mod);
extern void uwp_getreturn(const char * str);


static char uwp_winpanicstr[] = "Bad window id %d";
char uwp_defmorestr[] = "--More--";

#ifdef CLIPPING
#if defined(USE_TILES) && defined(MSDOS)
boolean uwp_clipping = FALSE; /* clipping on? */
int uwp_clipx = 0, uwp_clipxmax = 0;
#else
static boolean uwp_clipping = FALSE; /* clipping on? */
static int uwp_clipx = 0, uwp_clipxmax = 0;
#endif
static int uwp_clipy = 0, uwp_clipymax = 0;
#endif /* CLIPPING */

#if defined(USE_TILES) && defined(MSDOS)
extern void FDECL(uwp_adjust_cursor_flags, (struct UwpWinDesc *));
#endif

#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
boolean uwp_GFlag = FALSE;
boolean uwp_HE_resets_AS; /* see termcap.c */
#endif

#if defined(MICRO) || defined(WIN32CON)
static const char uwp_to_continue[] = "to continue";
#define getret() uwp_getreturn(uwp_to_continue)
#else
STATIC_DCL void NDECL(uwp_getret);
#endif

STATIC_DCL void FDECL(uwp_erase_menu_or_text, (winid, struct UwpWinDesc *, BOOLEAN_P));
STATIC_DCL void FDECL(uwp_free_window_info, (struct UwpWinDesc *, BOOLEAN_P));
STATIC_DCL void FDECL(uwp_dmore, (struct UwpWinDesc *, const char *));
STATIC_DCL void FDECL(uwp_set_item_state, (winid, int, uwp_menu_item *));
STATIC_DCL void FDECL(uwp_set_all_on_page, (winid, uwp_menu_item *, uwp_menu_item *));
STATIC_DCL void FDECL(unset_all_on_page, (winid, uwp_menu_item *, uwp_menu_item *));
STATIC_DCL void FDECL(invert_all_on_page, (winid, uwp_menu_item *, uwp_menu_item *, CHAR_P));
STATIC_DCL void FDECL(invert_all, (winid, uwp_menu_item *, uwp_menu_item *, CHAR_P));
STATIC_DCL void FDECL(uwp_toggle_menu_attr, (BOOLEAN_P, int, int));
STATIC_DCL void FDECL(uwp_process_menu_window, (winid, struct UwpWinDesc *));
STATIC_DCL void FDECL(uwp_process_text_window, (winid, struct UwpWinDesc *));
STATIC_DCL uwp_menu_item *FDECL(uwp_reverse, (uwp_menu_item *));
STATIC_DCL const char *FDECL(uwp_compress_str, (const char *));
STATIC_DCL void FDECL(uwp_putsym, (winid, int, int, CHAR_P));
STATIC_DCL void FDECL(uwp_bail, (const char *)); /* __attribute__((noreturn)) */
STATIC_DCL void FDECL(uwp_setup_rolemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(uwp_setup_racemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(uwp_setup_gendmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(uwp_setup_algnmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL boolean NDECL(uwp_reset_role_filtering);

/*
* A string containing all the default commands -- to add to a list
* of acceptable inputs.
*/
static const char uwp_default_menu_cmds[] = {
    MENU_FIRST_PAGE,    MENU_LAST_PAGE,   MENU_NEXT_PAGE,
    MENU_PREVIOUS_PAGE, MENU_SELECT_ALL,  MENU_UNSELECT_ALL,
    MENU_INVERT_ALL,    MENU_SELECT_PAGE, MENU_UNSELECT_PAGE,
    MENU_INVERT_PAGE,   MENU_SEARCH,      0 /* null terminator */
};

#ifdef UWP_TILES_ESCCODES
static int uwp_vt_tile_current_window = -2;

void
uwp_print_vt_code(i, c, d)
int i, c, d;
{
    if (iflags.vt_tiledata) {
        if (c >= 0) {
            if (i == AVTC_SELECT_WINDOW) {
                if (c == uwp_vt_tile_current_window) return;
                uwp_vt_tile_current_window = c;
            }
            if (d >= 0)
                printf("\033[1;%d;%d;%d%c", i, c, d, TILE_ANSI_COMMAND);
            else
                printf("\033[1;%d;%d%c", i, c, TILE_ANSI_COMMAND);
        }
        else {
            printf("\033[1;%d%c", i, TILE_ANSI_COMMAND);
        }
    }
}
#else
# define uwp_print_vt_code(i, c, d) ;
#endif /* !UWP_TILES_ESCCODES */

#define uwp_print_vt_code1(i)     uwp_print_vt_code((i), -1, -1)
#define uwp_print_vt_code2(i,c)   uwp_print_vt_code((i), (c), -1)
#define uwp_print_vt_code3(i,c,d) uwp_print_vt_code((i), (c), (d))


/* clean up and quit */
STATIC_OVL void
uwp_bail(mesg)
const char *mesg;
{
    clearlocks();
    uwp_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

/*ARGSUSED*/
void
uwp_init_nhwindows(argcp, argv)
int *argcp UNUSED;
char **argv UNUSED;
{
    int wid, hgt, i;

    /*
    *  Remember uwp modes, to be restored on exit.
    *
    *  gettty() must be called before uwp_startup()
    *    due to ordering of LI/CO settings
    *  uwp_startup() must be called before initoptions()
    *    due to ordering of graphics settings
    */
    // TODO: thing through gettty().  Why is it needed?
    gettty();

    /* to port dependant uwp setup */
    uwp_startup(&wid, &hgt);

    // TODO: think through setftty().  Why is it needed?
    setftty(); /* calls start_screen */

               /* set up uwp descriptor */
    uwpDisplay = (struct UwpDisplayDesc *) alloc(sizeof(struct UwpDisplayDesc));
    uwpDisplay->toplin = 0;
    uwpDisplay->rows = hgt;
    uwpDisplay->cols = wid;
    uwpDisplay->curx = uwpDisplay->cury = 0;
    uwpDisplay->inmore = uwpDisplay->inread = uwpDisplay->intr = 0;
    uwpDisplay->dismiss_more = 0;
#ifdef TEXTCOLOR
    uwpDisplay->color = NO_COLOR;
#endif
    uwpDisplay->attrs = 0;

    /* set up the default windows */
    UWP_BASE_WINDOW = uwp_create_nhwindow(UWP_NHW_BASE);
    uwp_wins[UWP_BASE_WINDOW]->active = 1;

    uwpDisplay->lastwin = WIN_ERR;

    uwp_clear_nhwindow(UWP_BASE_WINDOW);

    uwp_putstr(UWP_BASE_WINDOW, 0, "");
    for (i = 1; i <= 4; ++i)
        uwp_putstr(UWP_BASE_WINDOW, 0, copyright_banner_line(i));
    uwp_putstr(UWP_BASE_WINDOW, 0, "");
    uwp_display_nhwindow(UWP_BASE_WINDOW, FALSE);
}

/* try to reduce clutter in the code below... */
#define ROLE flags.initrole
#define RACE flags.initrace
#define GEND flags.initgend
#define ALGN flags.initalign

void
uwp_player_selection()
{
    int i, k, n, choice, nextpick;
    boolean getconfirmation, picksomething;
    char pick4u = 'n';
    char pbuf[QBUFSZ], plbuf[QBUFSZ];
    winid win;
    anything any;
    menu_item *selected = 0;

    /* Used to avoid "Is this ok?" if player has already specified all
    * four facets of role.
    * Note that rigid_role_checks might force any unspecified facets to
    * have a specific value, but that will still require confirmation;
    * player can specify the forced ones if avoiding that is demanded.
    */
    picksomething = (ROLE == ROLE_NONE || RACE == ROLE_NONE
        || GEND == ROLE_NONE || ALGN == ROLE_NONE);
    /* Used for '-@';
    * choose randomly without asking for all unspecified facets.
    */
    if (flags.randomall && picksomething) {
        if (ROLE == ROLE_NONE)
            ROLE = ROLE_RANDOM;
        if (RACE == ROLE_NONE)
            RACE = ROLE_RANDOM;
        if (GEND == ROLE_NONE)
            GEND = ROLE_RANDOM;
        if (ALGN == ROLE_NONE)
            ALGN = ROLE_RANDOM;
    }

    /* prevent unnecessary prompting if role forces race (samurai) or gender
    (valkyrie) or alignment (rogue), or race forces alignment (orc), &c */
    rigid_role_checks();

    /* Should we randomly pick for the player? */
    if (ROLE == ROLE_NONE || RACE == ROLE_NONE || GEND == ROLE_NONE
        || ALGN == ROLE_NONE) {
        int echoline;
        char *prompt = build_plselection_prompt(pbuf, QBUFSZ,
            ROLE, RACE, GEND, ALGN);

        /* this prompt string ends in "[ynaq]?":
        y - game picks role,&c then asks player to confirm;
        n - player manually chooses via menu selections;
        a - like 'y', but skips confirmation and starts game;
        q - quit
        */
        uwp_putstr(UWP_BASE_WINDOW, 0, "");
        echoline = uwp_wins[UWP_BASE_WINDOW]->cury;
        uwp_putstr(UWP_BASE_WINDOW, 0, prompt);
        do {
            pick4u = lowc(readchar());
            if (index(quitchars, pick4u))
                pick4u = 'y';
        } while (!index(ynaqchars, pick4u));
        if ((int)strlen(prompt) + 1 < CO) {
            /* Echo choice and move back down line */
            uwp_putsym(UWP_BASE_WINDOW, (int)strlen(prompt) + 1, echoline,
                pick4u);
            uwp_putstr(UWP_BASE_WINDOW, 0, "");
        }
        else
            /* Otherwise it's hard to tell where to echo, and things are
            * wrapping a bit messily anyway, so (try to) make sure the next
            * question shows up well and doesn't get wrapped at the
            * bottom of the window.
            */
            uwp_clear_nhwindow(UWP_BASE_WINDOW);

        if (pick4u != 'y' && pick4u != 'a' && pick4u != 'n')
            goto give_up;
    }

makepicks:
    nextpick = RS_ROLE;
    do {
        if (nextpick == RS_ROLE) {
            nextpick = RS_RACE;
            /* Select a role, if necessary;
            we'll try to be compatible with pre-selected
            race/gender/alignment, but may not succeed. */
            if (ROLE < 0) {
                /* Process the choice */
                if (pick4u == 'y' || pick4u == 'a' || ROLE == ROLE_RANDOM) {
                    /* Pick a random role */
                    k = pick_role(RACE, GEND, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        uwp_putstr(UWP_BASE_WINDOW, 0, "Incompatible role!");
                        k = randrole();
                    }
                }
                else {
                    /* Prompt for a role */
                    uwp_clear_nhwindow(UWP_BASE_WINDOW);
                    role_selection_prolog(RS_ROLE, UWP_BASE_WINDOW);
                    win = create_nhwindow(NHW_MENU);
                    start_menu(win);
                    /* populate the menu with role choices */
                    uwp_setup_rolemenu(win, TRUE, RACE, GEND, ALGN);
                    /* add miscellaneous menu entries */
                    role_menu_extra(ROLE_RANDOM, win, TRUE);
                    any.a_int = 0; /* separator, not a choice */
                    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                        MENU_UNSELECTED);
                    role_menu_extra(RS_RACE, win, FALSE);
                    role_menu_extra(RS_GENDER, win, FALSE);
                    role_menu_extra(RS_ALGNMNT, win, FALSE);
                    if (gotrolefilter())
                        role_menu_extra(RS_filter, win, FALSE);
                    role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                    Strcpy(pbuf, "Pick a role or profession");
                    end_menu(win, pbuf);
                    n = select_menu(win, PICK_ONE, &selected);
                    /*
                    * PICK_ONE with preselected choice behaves strangely:
                    *  n == -1 -- <escape>, so use quit choice;
                    *  n ==  0 -- explicitly chose preselected entry,
                    *             toggling it off, so use it;
                    *  n ==  1 -- implicitly chose preselected entry
                    *             with <space> or <return>;
                    *  n ==  2 -- explicitly chose a different entry, so
                    *             both it and preselected one are in list.
                    */
                    if (n > 0) {
                        choice = selected[0].item.a_int;
                        if (n > 1 && choice == ROLE_RANDOM)
                            choice = selected[1].item.a_int;
                    }
                    else
                        choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                    if (selected)
                        free((genericptr_t)selected), selected = 0;
                    destroy_nhwindow(win);

                    if (choice == ROLE_NONE) {
                        goto give_up; /* Selected quit */
                    }
                    else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                        ALGN = k = ROLE_NONE;
                        nextpick = RS_ALGNMNT;
                    }
                    else if (choice == RS_menu_arg(RS_GENDER)) {
                        GEND = k = ROLE_NONE;
                        nextpick = RS_GENDER;
                    }
                    else if (choice == RS_menu_arg(RS_RACE)) {
                        RACE = k = ROLE_NONE;
                        nextpick = RS_RACE;
                    }
                    else if (choice == RS_menu_arg(RS_filter)) {
                        ROLE = k = ROLE_NONE;
                        (void)uwp_reset_role_filtering();
                        nextpick = RS_ROLE;
                    }
                    else if (choice == ROLE_RANDOM) {
                        k = pick_role(RACE, GEND, ALGN, PICK_RANDOM);
                        if (k < 0)
                            k = randrole();
                    }
                    else {
                        k = choice - 1;
                    }
                }
                ROLE = k;
            } /* needed role */
        }     /* picking role */

        if (nextpick == RS_RACE) {
            nextpick = (ROLE < 0) ? RS_ROLE : RS_GENDER;
            /* Select a race, if necessary;
            force compatibility with role, try for compatibility
            with pre-selected gender/alignment. */
            if (RACE < 0 || !validrace(ROLE, RACE)) {
                /* no race yet, or pre-selected race not valid */
                if (pick4u == 'y' || pick4u == 'a' || RACE == ROLE_RANDOM) {
                    k = pick_race(ROLE, GEND, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        uwp_putstr(UWP_BASE_WINDOW, 0, "Incompatible race!");
                        k = randrace(ROLE);
                    }
                }
                else { /* pick4u == 'n' */
                       /* Count the number of valid races */
                    n = 0; /* number valid */
                    k = 0; /* valid race */
                    for (i = 0; races[i].noun; i++)
                        if (ok_race(ROLE, i, GEND, ALGN)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; races[i].noun; i++)
                            if (validrace(ROLE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        uwp_clear_nhwindow(UWP_BASE_WINDOW);
                        role_selection_prolog(RS_RACE, UWP_BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                                       /* populate the menu with role choices */
                        uwp_setup_racemenu(win, TRUE, ROLE, GEND, ALGN);
                        /* add miscellaneous menu entries */
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                            MENU_UNSELECTED);
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_GENDER, win, FALSE);
                        role_menu_extra(RS_ALGNMNT, win, FALSE);
                        if (gotrolefilter())
                            role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick a race or species");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        }
                        else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t)selected), selected = 0;
                        destroy_nhwindow(win);

                        if (choice == ROLE_NONE) {
                            goto give_up; /* Selected quit */
                        }
                        else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                            ALGN = k = ROLE_NONE;
                            nextpick = RS_ALGNMNT;
                        }
                        else if (choice == RS_menu_arg(RS_GENDER)) {
                            GEND = k = ROLE_NONE;
                            nextpick = RS_GENDER;
                        }
                        else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        }
                        else if (choice == RS_menu_arg(RS_filter)) {
                            RACE = k = ROLE_NONE;
                            if (uwp_reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_RACE;
                        }
                        else if (choice == ROLE_RANDOM) {
                            k = pick_race(ROLE, GEND, ALGN, PICK_RANDOM);
                            if (k < 0)
                                k = randrace(ROLE);
                        }
                        else {
                            k = choice - 1;
                        }
                    }
                }
                RACE = k;
            } /* needed race */
        }     /* picking race */

        if (nextpick == RS_GENDER) {
            nextpick = (ROLE < 0) ? RS_ROLE : (RACE < 0) ? RS_RACE
                : RS_ALGNMNT;
            /* Select a gender, if necessary;
            force compatibility with role/race, try for compatibility
            with pre-selected alignment. */
            if (GEND < 0 || !validgend(ROLE, RACE, GEND)) {
                /* no gender yet, or pre-selected gender not valid */
                if (pick4u == 'y' || pick4u == 'a' || GEND == ROLE_RANDOM) {
                    k = pick_gend(ROLE, RACE, ALGN, PICK_RANDOM);
                    if (k < 0) {
                        uwp_putstr(UWP_BASE_WINDOW, 0, "Incompatible gender!");
                        k = randgend(ROLE, RACE);
                    }
                }
                else { /* pick4u == 'n' */
                       /* Count the number of valid genders */
                    n = 0; /* number valid */
                    k = 0; /* valid gender */
                    for (i = 0; i < ROLE_GENDERS; i++)
                        if (ok_gend(ROLE, RACE, i, ALGN)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; i < ROLE_GENDERS; i++)
                            if (validgend(ROLE, RACE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        uwp_clear_nhwindow(UWP_BASE_WINDOW);
                        role_selection_prolog(RS_GENDER, UWP_BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                                       /* populate the menu with gender choices */
                        uwp_setup_gendmenu(win, TRUE, ROLE, RACE, ALGN);
                        /* add miscellaneous menu entries */
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                            MENU_UNSELECTED);
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_RACE, win, FALSE);
                        role_menu_extra(RS_ALGNMNT, win, FALSE);
                        if (gotrolefilter())
                            role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick a gender or sex");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        }
                        else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t)selected), selected = 0;
                        destroy_nhwindow(win);

                        if (choice == ROLE_NONE) {
                            goto give_up; /* Selected quit */
                        }
                        else if (choice == RS_menu_arg(RS_ALGNMNT)) {
                            ALGN = k = ROLE_NONE;
                            nextpick = RS_ALGNMNT;
                        }
                        else if (choice == RS_menu_arg(RS_RACE)) {
                            RACE = k = ROLE_NONE;
                            nextpick = RS_RACE;
                        }
                        else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        }
                        else if (choice == RS_menu_arg(RS_filter)) {
                            GEND = k = ROLE_NONE;
                            if (uwp_reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_GENDER;
                        }
                        else if (choice == ROLE_RANDOM) {
                            k = pick_gend(ROLE, RACE, ALGN, PICK_RANDOM);
                            if (k < 0)
                                k = randgend(ROLE, RACE);
                        }
                        else {
                            k = choice - 1;
                        }
                    }
                }
                GEND = k;
            } /* needed gender */
        }     /* picking gender */

        if (nextpick == RS_ALGNMNT) {
            nextpick = (ROLE < 0) ? RS_ROLE : (RACE < 0) ? RS_RACE : RS_GENDER;
            /* Select an alignment, if necessary;
            force compatibility with role/race/gender. */
            if (ALGN < 0 || !validalign(ROLE, RACE, ALGN)) {
                /* no alignment yet, or pre-selected alignment not valid */
                if (pick4u == 'y' || pick4u == 'a' || ALGN == ROLE_RANDOM) {
                    k = pick_align(ROLE, RACE, GEND, PICK_RANDOM);
                    if (k < 0) {
                        uwp_putstr(UWP_BASE_WINDOW, 0, "Incompatible alignment!");
                        k = randalign(ROLE, RACE);
                    }
                }
                else { /* pick4u == 'n' */
                       /* Count the number of valid alignments */
                    n = 0; /* number valid */
                    k = 0; /* valid alignment */
                    for (i = 0; i < ROLE_ALIGNS; i++)
                        if (ok_align(ROLE, RACE, GEND, i)) {
                            n++;
                            k = i;
                        }
                    if (n == 0) {
                        for (i = 0; i < ROLE_ALIGNS; i++)
                            if (validalign(ROLE, RACE, i)) {
                                n++;
                                k = i;
                            }
                    }
                    /* Permit the user to pick, if there is more than one */
                    if (n > 1) {
                        uwp_clear_nhwindow(UWP_BASE_WINDOW);
                        role_selection_prolog(RS_ALGNMNT, UWP_BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                        uwp_setup_algnmenu(win, TRUE, ROLE, RACE, GEND);
                        role_menu_extra(ROLE_RANDOM, win, TRUE);
                        any.a_int = 0; /* separator, not a choice */
                        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
                            MENU_UNSELECTED);
                        role_menu_extra(RS_ROLE, win, FALSE);
                        role_menu_extra(RS_RACE, win, FALSE);
                        role_menu_extra(RS_GENDER, win, FALSE);
                        if (gotrolefilter())
                            role_menu_extra(RS_filter, win, FALSE);
                        role_menu_extra(ROLE_NONE, win, FALSE); /* quit */
                        Strcpy(pbuf, "Pick an alignment or creed");
                        end_menu(win, pbuf);
                        n = select_menu(win, PICK_ONE, &selected);
                        if (n > 0) {
                            choice = selected[0].item.a_int;
                            if (n > 1 && choice == ROLE_RANDOM)
                                choice = selected[1].item.a_int;
                        }
                        else
                            choice = (n == 0) ? ROLE_RANDOM : ROLE_NONE;
                        if (selected)
                            free((genericptr_t)selected), selected = 0;
                        destroy_nhwindow(win);

                        if (choice == ROLE_NONE) {
                            goto give_up; /* Selected quit */
                        }
                        else if (choice == RS_menu_arg(RS_GENDER)) {
                            GEND = k = ROLE_NONE;
                            nextpick = RS_GENDER;
                        }
                        else if (choice == RS_menu_arg(RS_RACE)) {
                            RACE = k = ROLE_NONE;
                            nextpick = RS_RACE;
                        }
                        else if (choice == RS_menu_arg(RS_ROLE)) {
                            ROLE = k = ROLE_NONE;
                            nextpick = RS_ROLE;
                        }
                        else if (choice == RS_menu_arg(RS_filter)) {
                            ALGN = k = ROLE_NONE;
                            if (uwp_reset_role_filtering())
                                nextpick = RS_ROLE;
                            else
                                nextpick = RS_ALGNMNT;
                        }
                        else if (choice == ROLE_RANDOM) {
                            k = pick_align(ROLE, RACE, GEND, PICK_RANDOM);
                            if (k < 0)
                                k = randalign(ROLE, RACE);
                        }
                        else {
                            k = choice - 1;
                        }
                    }
                }
                ALGN = k;
            } /* needed alignment */
        }     /* picking alignment */

    } while (ROLE < 0 || RACE < 0 || GEND < 0 || ALGN < 0);

    /*
    *  Role, race, &c have now been determined;
    *  ask for confirmation and maybe go back to choose all over again.
    *
    *  Uses ynaq for familiarity, although 'a' is usually a
    *  superset of 'y' but here is an alternate form of 'n'.
    *  Menu layout:
    *   title:  Is this ok? [ynaq]
    *   blank:
    *    text:  $name, $alignment $gender $race $role
    *   blank:
    *    menu:  y + yes; play
    *           n - no; pick again
    *   maybe:  a - no; rename hero
    *           q - quit
    *           (end)
    */
    getconfirmation = (picksomething && pick4u != 'a' && !flags.randomall);
    while (getconfirmation) {
        uwp_clear_nhwindow(UWP_BASE_WINDOW);
        role_selection_prolog(ROLE_NONE, UWP_BASE_WINDOW);
        win = create_nhwindow(NHW_MENU);
        start_menu(win);
        any = zeroany; /* zero out all bits */
        any.a_int = 0;
        if (!roles[ROLE].name.f
            && (roles[ROLE].allow & ROLE_GENDMASK)
            == (ROLE_MALE | ROLE_FEMALE))
            Sprintf(plbuf, " %s", genders[GEND].adj);
        else
            *plbuf = '\0'; /* omit redundant gender */
        Sprintf(pbuf, "%s, %s%s %s %s", plname, aligns[ALGN].adj, plbuf,
            races[RACE].adj,
            (GEND == 1 && roles[ROLE].name.f) ? roles[ROLE].name.f
            : roles[ROLE].name.m);
        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, pbuf,
            MENU_UNSELECTED);
        /* blank separator */
        any.a_int = 0;
        add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
        /* [ynaq] menu choices */
        any.a_int = 1;
        add_menu(win, NO_GLYPH, &any, 'y', 0, ATR_NONE, "Yes; start game",
            MENU_SELECTED);
        any.a_int = 2;
        add_menu(win, NO_GLYPH, &any, 'n', 0, ATR_NONE,
            "No; choose role again", MENU_UNSELECTED);
        if (iflags.renameallowed) {
            any.a_int = 3;
            add_menu(win, NO_GLYPH, &any, 'a', 0, ATR_NONE,
                "Not yet; choose another name", MENU_UNSELECTED);
        }
        any.a_int = -1;
        add_menu(win, NO_GLYPH, &any, 'q', 0, ATR_NONE, "Quit",
            MENU_UNSELECTED);
        Sprintf(pbuf, "Is this ok? [yn%sq]", iflags.renameallowed ? "a" : "");
        end_menu(win, pbuf);
        n = select_menu(win, PICK_ONE, &selected);
        /* [pick-one menus with a preselected entry behave oddly...] */
        choice = (n > 0) ? selected[n - 1].item.a_int : (n == 0) ? 1 : -1;
        if (selected)
            free((genericptr_t)selected), selected = 0;
        destroy_nhwindow(win);

        switch (choice) {
        default:          /* 'q' or ESC */
            goto give_up; /* quit */
            break;
        case 3: { /* 'a' */
                  /*
                  * TODO: what, if anything, should be done if the name is
                  * changed to or from "wizard" after port-specific startup
                  * code has set flags.debug based on the original name?
                  */
            int saveROLE, saveRACE, saveGEND, saveALGN;

            iflags.renameinprogress = TRUE;
            /* plnamesuffix() can change any or all of ROLE, RACE,
            GEND, ALGN; we'll override that and honor only the name */
            saveROLE = ROLE, saveRACE = RACE, saveGEND = GEND,
                saveALGN = ALGN;
            *plname = '\0';
            plnamesuffix(); /* calls askname() when plname[] is empty */
            ROLE = saveROLE, RACE = saveRACE, GEND = saveGEND,
                ALGN = saveALGN;
            break; /* getconfirmation is still True */
        }
        case 2:    /* 'n' */
                   /* start fresh, but bypass "shall I pick everything for you?"
                   step; any partial role selection via config file, command
                   line, or name suffix is discarded this time */
            pick4u = 'n';
            ROLE = RACE = GEND = ALGN = ROLE_NONE;
            goto makepicks;
            break;
        case 1: /* 'y' or Space or Return/Enter */
                /* success; drop out through end of function */
            getconfirmation = FALSE;
            break;
        }
    }

    /* Success! */
    uwp_display_nhwindow(UWP_BASE_WINDOW, FALSE);
    return;

give_up:
    /* Quit */
    if (selected)
        free((genericptr_t)selected); /* [obsolete] */
    uwp_bail((char *)0);
    /*NOTREACHED*/
    return;
}

STATIC_OVL boolean
uwp_reset_role_filtering()
{
    winid win;
    anything any;
    int i, n;
    menu_item *selected = 0;

    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    any = zeroany;

    /* no extra blank line preceding this entry; end_menu supplies one */
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
        "Unacceptable roles", MENU_UNSELECTED);
    uwp_setup_rolemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
        "Unacceptable races", MENU_UNSELECTED);
    uwp_setup_racemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
        "Unacceptable genders", MENU_UNSELECTED);
    uwp_setup_gendmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
        "Uncceptable alignments", MENU_UNSELECTED);
    uwp_setup_algnmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    end_menu(win, "Pick all that apply");
    n = select_menu(win, PICK_ANY, &selected);

    if (n > 0) {
        clearrolefilter();
        for (i = 0; i < n; i++)
            setrolefilter(selected[i].item.a_string);

        ROLE = RACE = GEND = ALGN = ROLE_NONE;
    }
    if (selected)
        free((genericptr_t)selected), selected = 0;
    destroy_nhwindow(win);
    return (n > 0) ? TRUE : FALSE;
}

#undef ROLE
#undef RACE
#undef GEND
#undef ALGN

/* add entries a-Archeologist, b-Barbarian, &c to menu being built in 'win' */
STATIC_OVL void
uwp_setup_rolemenu(win, filtering, race, gend, algn)
winid win;
boolean filtering; /* True => exclude filtered roles; False => filter reset */
int race, gend, algn; /* all ROLE_NONE for !filtering case */
{
    anything any;
    int i;
    boolean role_ok;
    char thisch, lastch = '\0', rolenamebuf[50];

    any = zeroany; /* zero out all bits */
    for (i = 0; roles[i].name.m; i++) {
        role_ok = ok_role(i, race, gend, algn);
        if (filtering && !role_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = roles[i].name.m;
        thisch = lowc(*roles[i].name.m);
        if (thisch == lastch)
            thisch = highc(thisch);
        Strcpy(rolenamebuf, roles[i].name.m);
        if (roles[i].name.f) {
            /* role has distinct name for female (C,P) */
            if (gend == 1) {
                /* female already chosen; replace male name */
                Strcpy(rolenamebuf, roles[i].name.f);
            }
            else if (gend < 0) {
                /* not chosen yet; append slash+female name */
                Strcat(rolenamebuf, "/");
                Strcat(rolenamebuf, roles[i].name.f);
            }
        }
        /* !filtering implies uwp_reset_role_filtering() where we want to
        mark this role as preseleted if current filter excludes it */
        add_menu(win, NO_GLYPH, &any, thisch, 0, ATR_NONE, an(rolenamebuf),
            (!filtering && !role_ok) ? MENU_SELECTED : MENU_UNSELECTED);
        lastch = thisch;
    }
}

STATIC_OVL void
uwp_setup_racemenu(win, filtering, role, gend, algn)
winid win;
boolean filtering;
int role, gend, algn;
{
    anything any;
    boolean race_ok;
    int i;
    char this_ch;

    any = zeroany;
    for (i = 0; races[i].noun; i++) {
        race_ok = ok_race(role, i, gend, algn);
        if (filtering && !race_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = races[i].noun;
        this_ch = *races[i].noun;
        /* filtering: picking race, so choose by first letter, with
        capital letter as unseen accelerator;
        !filtering: resetting filter rather than picking, choose by
        capital letter since lowercase role letters will be present */
        add_menu(win, NO_GLYPH, &any,
            filtering ? this_ch : highc(this_ch),
            filtering ? highc(this_ch) : 0,
            ATR_NONE, races[i].noun,
            (!filtering && !race_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

STATIC_DCL void
uwp_setup_gendmenu(win, filtering, role, race, algn)
winid win;
boolean filtering;
int role, race, algn;
{
    anything any;
    boolean gend_ok;
    int i;
    char this_ch;

    any = zeroany;
    for (i = 0; i < ROLE_GENDERS; i++) {
        gend_ok = ok_gend(role, race, i, algn);
        if (filtering && !gend_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = genders[i].adj;
        this_ch = *genders[i].adj;
        /* (see uwp_setup_racemenu for explanation of selector letters
        and uwp_setup_rolemenu for preselection) */
        add_menu(win, NO_GLYPH, &any,
            filtering ? this_ch : highc(this_ch),
            filtering ? highc(this_ch) : 0,
            ATR_NONE, genders[i].adj,
            (!filtering && !gend_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

STATIC_DCL void
uwp_setup_algnmenu(win, filtering, role, race, gend)
winid win;
boolean filtering;
int role, race, gend;
{
    anything any;
    boolean algn_ok;
    int i;
    char this_ch;

    any = zeroany;
    for (i = 0; i < ROLE_ALIGNS; i++) {
        algn_ok = ok_align(role, race, gend, i);
        if (filtering && !algn_ok)
            continue;
        if (filtering)
            any.a_int = i + 1;
        else
            any.a_string = aligns[i].adj;
        this_ch = *aligns[i].adj;
        /* (see uwp_setup_racemenu for explanation of selector letters
        and uwp_setup_rolemenu for preselection) */
        add_menu(win, NO_GLYPH, &any,
            filtering ? this_ch : highc(this_ch),
            filtering ? highc(this_ch) : 0,
            ATR_NONE, aligns[i].adj,
            (!filtering && !algn_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

/*
* plname is filled either by an option (-u Player  or  -uPlayer) or
* explicitly (by being the wizard) or by askname.
* It may still contain a suffix denoting the role, etc.
* Always called after init_nhwindows() and before display_gamewindows().
*/
void
uwp_askname()
{
    static const char who_are_you[] = "Who are you? ";
    register int c, ct, tryct = 0;

#ifdef SELECTSAVED
    if (iflags.wc2_selectsaved && !iflags.renameinprogress)
        switch (restore_menu(UWP_BASE_WINDOW)) {
        case -1:
            uwp_bail("Until next time then..."); /* quit */
                                             /*NOTREACHED*/
        case 0:
            break; /* no game chosen; start new game */
        case 1:
            return; /* plname[] has been set */
        }
#endif /* SELECTSAVED */

    uwp_putstr(UWP_BASE_WINDOW, 0, "");
    do {
        if (++tryct > 1) {
            if (tryct > 10)
                uwp_bail("Giving up after 10 tries.\n");
            uwp_curs(UWP_BASE_WINDOW, 1, uwp_wins[UWP_BASE_WINDOW]->cury - 1);
            uwp_putstr(UWP_BASE_WINDOW, 0, "Enter a name for your character...");
            /* erase previous prompt (in case of ESC after partial response)
            */
            uwp_curs(UWP_BASE_WINDOW, 1, uwp_wins[UWP_BASE_WINDOW]->cury), uwp_cl_end();
        }
        uwp_putstr(UWP_BASE_WINDOW, 0, who_are_you);
        uwp_curs(UWP_BASE_WINDOW, (int)(sizeof who_are_you),
            uwp_wins[UWP_BASE_WINDOW]->cury - 1);
        ct = 0;
        while ((c = uwp_nhgetch()) != '\n') {
            if (c == EOF)
                c = '\033';
            if (c == '\033') {
                ct = 0;
                break;
            } /* continue outer loop */
#if defined(WIN32CON)
            if (c == '\003')
                uwp_bail("^C abort.\n");
#endif
            /* some people get confused when their erase char is not ^H */
            if (c == '\b' || c == '\177') {
                if (ct) {
                    ct--;
#ifdef WIN32CON
                    uwpDisplay->curx--;
#endif
#if defined(MICRO) || defined(WIN32CON)
#if defined(WIN32CON) || defined(MSDOS)
                    uwp_backsp(); /* \b is visible on NT */
                    (void)putchar(' ');
                    uwp_backsp();
#else
                    uwp_msmsg("\b \b");
#endif
#else
                    (void)putchar('\b');
                    (void)putchar(' ');
                    (void)putchar('\b');
#endif
                }
                continue;
            }
#if defined(UNIX) || defined(VMS)
            if (c != '-' && c != '@')
                if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
                    /* reject leading digit but allow digits elsewhere
                    (avoids ambiguity when character name gets
                    appended to uid to construct save file name) */
                    !(c >= '0' && c <= '9' && ct > 0))
                    c = '_';
#endif
            if (ct < (int)(sizeof plname) - 1) {
#if defined(MICRO)
#if defined(MSDOS)
                if (iflags.grmode) {
                    (void)putchar(c);
                }
                else
#endif
                    uwp_msmsg("%c", c);
#else
                (void)putchar(c);
#endif
                plname[ct++] = c;
#ifdef WIN32CON
                uwpDisplay->curx++;
#endif
            }
        }
        plname[ct] = 0;
    } while (ct == 0);

    /* move to next line to simulate echo of user's <return> */
    uwp_curs(UWP_BASE_WINDOW, 1, uwp_wins[UWP_BASE_WINDOW]->cury + 1);

    /* since we let user pick an arbitrary name now, he/she can pick
    another one during role selection */
    iflags.renameallowed = TRUE;
}

void
uwp_get_nh_event()
{
    return;
}

#if !defined(MICRO) && !defined(WIN32CON)
STATIC_OVL void
getret()
{
    uwp_xputs("\n");
    if (flags.standout)
        uwp_standoutbeg();
    uwp_xputs("Hit ");
    uwp_xputs(iflags.cbreak ? "space" : "return");
    uwp_xputs(" to continue: ");
    if (flags.standout)
        uwp_standoutend();
    uwp_xwaitforspace(" ");
}
#endif

void
uwp_suspend_nhwindows(str)
const char *str;
{
    uwp_settty(str); /* calls end_screen, perhaps raw_print */
    if (!str)
        uwp_raw_print(""); /* calls fflush(stdout) */
}

void
uwp_resume_nhwindows()
{
    gettty();
    setftty(); /* calls start_screen */
    docrt();
}

void
uwp_exit_nhwindows(str)
const char *str;
{
    winid i;

    uwp_suspend_nhwindows(str);
    /*
    * Disable windows to avoid calls to window routines.
    */
    free_pickinv_cache(); /* reset its state as well as tear down window */
    for (i = 0; i < UWP_MAXWIN; i++) {
        if (i == UWP_BASE_WINDOW)
            continue; /* handle uwp_wins[UWP_BASE_WINDOW] last */
        if (uwp_wins[i]) {
#ifdef FREE_ALL_MEMORY
            uwp_free_window_info(uwp_wins[i], TRUE);
            free((genericptr_t)uwp_wins[i]);
#endif
            uwp_wins[i] = (struct UwpWinDesc *) 0;
        }
    }
    WIN_MAP = WIN_MESSAGE = WIN_INVEN = WIN_ERR; /* these are all gone now */
#ifndef STATUS_VIA_WINDOWPORT
    WIN_STATUS = WIN_ERR;
#endif
#ifdef FREE_ALL_MEMORY
    if (UWP_BASE_WINDOW != WIN_ERR && uwp_wins[UWP_BASE_WINDOW]) {
        uwp_free_window_info(uwp_wins[UWP_BASE_WINDOW], TRUE);
        free((genericptr_t)uwp_wins[UWP_BASE_WINDOW]);
        uwp_wins[UWP_BASE_WINDOW] = (struct UwpWinDesc *) 0;
        UWP_BASE_WINDOW = WIN_ERR;
    }
    free((genericptr_t)uwpDisplay);
    uwpDisplay = (struct UwpDisplayDesc *) 0;
#endif

#ifndef NO_TERMS    /*(until this gets added to the window interface)*/
    uwp_shutdown(); /* cleanup termcap/terminfo/whatever */
#endif
    iflags.window_inited = 0;
}

winid
uwp_create_nhwindow(type)
int type;
{
    struct UwpWinDesc *newwin;
    int i;
    int newid;

    if (uwp_maxwin == UWP_MAXWIN)
        return WIN_ERR;

    newwin = (struct UwpWinDesc *) alloc(sizeof(struct UwpWinDesc));
    newwin->type = type;
    newwin->flags = 0;
    newwin->active = FALSE;
    newwin->curx = newwin->cury = 0;
    newwin->morestr = 0;
    newwin->mlist = (uwp_menu_item *)0;
    newwin->plist = (uwp_menu_item **)0;
    newwin->npages = newwin->plist_size = newwin->nitems = newwin->how = 0;
    switch (type) {
    case UWP_NHW_BASE:
        /* base window, used for absolute movement on the screen */
        newwin->offx = newwin->offy = 0;
        newwin->rows = uwpDisplay->rows;
        newwin->cols = uwpDisplay->cols;
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
            newwin->offy = uwpDisplay->rows - 2;
        }
        else
#endif
            newwin->offy = min((int)uwpDisplay->rows - 2, ROWNO + 1);
        newwin->rows = newwin->maxrow = 2;
        newwin->cols = newwin->maxcol = uwpDisplay->cols;
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
        newwin->cols = uwpDisplay->cols;
        newwin->maxrow = newwin->maxcol = 0;
        break;
    default:
        panic("Tried to create window type %d\n", (int)type);
        return WIN_ERR;
    }

    for (newid = 0; newid < UWP_MAXWIN; newid++) {
        if (uwp_wins[newid] == 0) {
            uwp_wins[newid] = newwin;
            break;
        }
    }
    if (newid == UWP_MAXWIN) {
        panic("No window slots!");
        return WIN_ERR;
    }

    if (newwin->maxrow) {
        newwin->data =
            (char **)alloc(sizeof(char *) * (unsigned)newwin->maxrow);
        newwin->datlen =
            (short *)alloc(sizeof(short) * (unsigned)newwin->maxrow);
        if (newwin->maxcol) {
            /* WIN_STATUS */
            for (i = 0; i < newwin->maxrow; i++) {
                newwin->data[i] = (char *)alloc((unsigned)newwin->maxcol);
                newwin->datlen[i] = (short)newwin->maxcol;
            }
        }
        else {
            for (i = 0; i < newwin->maxrow; i++) {
                newwin->data[i] = (char *)0;
                newwin->datlen[i] = 0;
            }
        }
        if (newwin->type == NHW_MESSAGE)
            newwin->maxrow = 0;
    }
    else {
        newwin->data = (char **)0;
        newwin->datlen = (short *)0;
    }

    return newid;
}

STATIC_OVL void
erase_menu_or_text(window, cw, clear)
winid window;
struct UwpWinDesc *cw;
boolean clear;
{
    if (cw->offx == 0)
        if (cw->offy) {
            uwp_curs(window, 1, 0);
            uwp_cl_eos();
        }
        else if (clear)
            uwp_clear_screen();
        else
            docrt();
    else
        uwp_docorner((int)cw->offx, cw->maxrow + 1);
}

STATIC_OVL void
uwp_free_window_info(cw, free_data)
struct UwpWinDesc *cw;
boolean free_data;
{
    int i;

    if (cw->data) {
        if (cw == uwp_wins[WIN_MESSAGE] && cw->rows > cw->maxrow)
            cw->maxrow = cw->rows; /* topl data */
        for (i = 0; i < cw->maxrow; i++)
            if (cw->data[i]) {
                free((genericptr_t)cw->data[i]);
                cw->data[i] = (char *)0;
                if (cw->datlen)
                    cw->datlen[i] = 0;
            }
        if (free_data) {
            free((genericptr_t)cw->data);
            cw->data = (char **)0;
            if (cw->datlen)
                free((genericptr_t)cw->datlen);
            cw->datlen = (short *)0;
            cw->rows = 0;
        }
    }
    cw->maxrow = cw->maxcol = 0;
    if (cw->mlist) {
        uwp_menu_item *temp;

        while ((temp = cw->mlist) != 0) {
            cw->mlist = cw->mlist->next;
            if (temp->str)
                free((genericptr_t)temp->str);
            free((genericptr_t)temp);
        }
    }
    if (cw->plist) {
        free((genericptr_t)cw->plist);
        cw->plist = 0;
    }
    cw->plist_size = cw->npages = cw->nitems = cw->how = 0;
    if (cw->morestr) {
        free((genericptr_t)cw->morestr);
        cw->morestr = 0;
    }
}

void
uwp_clear_nhwindow(window)
winid window;
{
    register struct UwpWinDesc *cw = 0;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0)
        panic(uwp_winpanicstr, window);
    uwpDisplay->lastwin = window;

    uwp_print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        if (uwpDisplay->toplin) {
            uwp_home();
            uwp_cl_end();
            if (cw->cury)
                uwp_docorner(1, cw->cury + 1);
            uwpDisplay->toplin = 0;
        }
        break;
    case NHW_STATUS:
        uwp_curs(window, 1, 0);
        uwp_cl_end();
        uwp_curs(window, 1, 1);
        uwp_cl_end();
        break;
    case NHW_MAP:
        /* cheap -- clear the whole thing and tell nethack to redraw botl */
        context.botlx = 1;
        /* fall into ... */
    case UWP_NHW_BASE:
        uwp_clear_screen();
        break;
    case NHW_MENU:
    case NHW_TEXT:
        if (cw->active)
            erase_menu_or_text(window, cw, TRUE);
        uwp_free_window_info(cw, FALSE);
        break;
    }
    cw->curx = cw->cury = 0;
}

boolean
toggle_menu_curr(window, curr, lineno, in_view, counting, count)
winid window;
uwp_menu_item *curr;
int lineno;
boolean in_view, counting;
long count;
{
    if (curr->selected) {
        if (counting && count > 0) {
            curr->count = count;
            if (in_view)
                uwp_set_item_state(window, lineno, curr);
            return TRUE;
        }
        else { /* change state */
            curr->selected = FALSE;
            curr->count = -1L;
            if (in_view)
                uwp_set_item_state(window, lineno, curr);
            return TRUE;
        }
    }
    else { /* !selected */
        if (counting && count > 0) {
            curr->count = count;
            curr->selected = TRUE;
            if (in_view)
                uwp_set_item_state(window, lineno, curr);
            return TRUE;
        }
        else if (!counting) {
            curr->selected = TRUE;
            if (in_view)
                uwp_set_item_state(window, lineno, curr);
            return TRUE;
        }
        /* do nothing counting&&count==0 */
    }
    return FALSE;
}

STATIC_OVL void
dmore(cw, s)
register struct UwpWinDesc *cw;
const char *s; /* valid responses */
{
    const char *prompt = cw->morestr ? cw->morestr : uwp_defmorestr;
    int offset = (cw->type == NHW_TEXT) ? 1 : 2;

    uwp_curs(UWP_BASE_WINDOW, (int)uwpDisplay->curx + offset,
        (int)uwpDisplay->cury);
    if (flags.standout)
        uwp_standoutbeg();
    uwp_xputs(prompt);
    uwpDisplay->curx += strlen(prompt);
    if (flags.standout)
        uwp_standoutend();

    uwp_xwaitforspace(s);
}

STATIC_OVL void
uwp_set_item_state(window, lineno, item)
winid window;
int lineno;
uwp_menu_item *item;
{
    char ch = item->selected ? (item->count == -1L ? '+' : '#') : '-';

    uwp_curs(window, 4, lineno);
    uwp_term_start_attr(item->attr);
    (void)putchar(ch);
    uwpDisplay->curx++;
    uwp_term_end_attr(item->attr);
}

STATIC_OVL void
set_all_on_page(window, page_start, page_end)
winid window;
uwp_menu_item *page_start, *page_end;
{
    uwp_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && !curr->selected) {
            curr->selected = TRUE;
            uwp_set_item_state(window, n, curr);
        }
}

STATIC_OVL void
unset_all_on_page(window, page_start, page_end)
winid window;
uwp_menu_item *page_start, *page_end;
{
    uwp_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && curr->selected) {
            curr->selected = FALSE;
            curr->count = -1L;
            uwp_set_item_state(window, n, curr);
        }
}

STATIC_OVL void
invert_all_on_page(window, page_start, page_end, acc)
winid window;
uwp_menu_item *page_start, *page_end;
char acc; /* group accelerator, 0 => all */
{
    uwp_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && (acc == 0 || curr->gselector == acc)) {
            if (curr->selected) {
                curr->selected = FALSE;
                curr->count = -1L;
            }
            else
                curr->selected = TRUE;
            uwp_set_item_state(window, n, curr);
        }
}

/*
* Invert all entries that match the give group accelerator (or all if zero).
*/
STATIC_OVL void
invert_all(window, page_start, page_end, acc)
winid window;
uwp_menu_item *page_start, *page_end;
char acc; /* group accelerator, 0 => all */
{
    uwp_menu_item *curr;
    boolean on_curr_page;
    struct UwpWinDesc *cw = uwp_wins[window];

    invert_all_on_page(window, page_start, page_end, acc);

    /* invert the rest */
    for (on_curr_page = FALSE, curr = cw->mlist; curr; curr = curr->next) {
        if (curr == page_start)
            on_curr_page = TRUE;
        else if (curr == page_end)
            on_curr_page = FALSE;

        if (!on_curr_page && curr->identifier.a_void
            && (acc == 0 || curr->gselector == acc)) {
            if (curr->selected) {
                curr->selected = FALSE;
                curr->count = -1;
            }
            else
                curr->selected = TRUE;
        }
    }
}

/* support menucolor in addition to caller-supplied attribute */
STATIC_OVL void
toggle_menu_attr(on, color, attr)
boolean on;
int color, attr;
{
    if (on) {
        uwp_term_start_attr(attr);
#ifdef TEXTCOLOR
        if (color != NO_COLOR)
            uwp_term_start_color(color);
#endif
    }
    else {
#ifdef TEXTCOLOR
        if (color != NO_COLOR)
            uwp_term_end_color();
#endif
        uwp_term_end_attr(attr);
    }

#ifndef TEXTCOLOR
    nhUse(color);
#endif
}

STATIC_OVL void
process_menu_window(window, cw)
winid window;
struct UwpWinDesc *cw;
{
    uwp_menu_item *page_start, *page_end, *curr;
    long count;
    int n, attr_n, curr_page, page_lines, resp_len;
    boolean finished, counting, reset_count;
    char *cp, *rp, resp[QBUFSZ], gacc[QBUFSZ], *msave, *morestr, really_morc;
#define MENU_EXPLICIT_CHOICE 0x7f /* pseudo menu manipulation char */

    curr_page = page_lines = 0;
    page_start = page_end = 0;
    msave = cw->morestr; /* save the morestr */
    cw->morestr = morestr = (char *)alloc((unsigned)QBUFSZ);
    counting = FALSE;
    count = 0L;
    reset_count = TRUE;
    finished = FALSE;

    /* collect group accelerators; for PICK_NONE, they're ignored;
    for PICK_ONE, only those which match exactly one entry will be
    accepted; for PICK_ANY, those which match any entry are okay */
    gacc[0] = '\0';
    if (cw->how != PICK_NONE) {
        int i, gcnt[128];
#define GSELIDX(c) (c & 127) /* guard against `signed char' */

        for (i = 0; i < SIZE(gcnt); i++)
            gcnt[i] = 0;
        for (n = 0, curr = cw->mlist; curr; curr = curr->next)
            if (curr->gselector && curr->gselector != curr->selector) {
                ++n;
                ++gcnt[GSELIDX(curr->gselector)];
            }

        if (n > 0) /* at least one group accelerator found */
            for (rp = gacc, curr = cw->mlist; curr; curr = curr->next)
                if (curr->gselector && curr->gselector != curr->selector
                    && !index(gacc, curr->gselector)
                    && (cw->how == PICK_ANY
                        || gcnt[GSELIDX(curr->gselector)] == 1)) {
                    *rp++ = curr->gselector;
                    *rp = '\0'; /* re-terminate for index() */
                }
    }
    resp_len = 0; /* lint suppression */

                  /* loop until finished */
    while (!finished) {
        if (reset_count) {
            counting = FALSE;
            count = 0;
        }
        else
            reset_count = TRUE;

        if (!page_start) {
            /* new page to be displayed */
            if (curr_page < 0 || (cw->npages > 0 && curr_page >= cw->npages))
                panic("bad menu screen page #%d", curr_page);

            /* clear screen */
            if (!cw->offx) { /* if not corner, do clearscreen */
                if (cw->offy) {
                    uwp_curs(window, 1, 0);
                    uwp_cl_eos();
                }
                else
                    uwp_clear_screen();
            }

            rp = resp;
            if (cw->npages > 0) {
                /* collect accelerators */
                page_start = cw->plist[curr_page];
                page_end = cw->plist[curr_page + 1];
                for (page_lines = 0, curr = page_start; curr != page_end;
                    page_lines++, curr = curr->next) {
                    int attr, color = NO_COLOR;

                    if (curr->selector)
                        *rp++ = curr->selector;

                    uwp_curs(window, 1, page_lines);
                    if (cw->offx)
                        uwp_cl_end();

                    (void)putchar(' ');
                    ++uwpDisplay->curx;

                    if (!iflags.use_menu_color
                        || !get_menu_coloring(curr->str, &color, &attr))
                        attr = curr->attr;

                    /* which character to start attribute highlighting;
                    whole line for headers and such, after the selector
                    character and space and selection indicator for menu
                    lines (including fake ones that simulate grayed-out
                    entries, so we don't rely on curr->identifier here) */
                    attr_n = 0; /* whole line */
                    if (curr->str[0] && curr->str[1] == ' '
                        && curr->str[2] && index("-+#", curr->str[2])
                        && curr->str[3] == ' ')
                        /* [0]=letter, [1]==space, [2]=[-+#], [3]=space */
                        attr_n = 4; /* [4:N]=entry description */

                                    /*
                                    * Don't use uwp_xputs() because (1) under unix it calls
                                    * tputstr() which will interpret a '*' as some kind
                                    * of padding information and (2) it calls xputc to
                                    * actually output the character.  We're faster doing
                                    * this.
                                    */
                    for (n = 0, cp = curr->str;
                        *cp &&
#ifndef WIN32CON
                        (int) ++uwpDisplay->curx < (int)uwpDisplay->cols;
#else
                        (int)uwpDisplay->curx < (int)uwpDisplay->cols;
                        uwpDisplay->curx++,
#endif
                        cp++, n++) {
                        if (n == attr_n && (color != NO_COLOR
                            || attr != ATR_NONE))
                            toggle_menu_attr(TRUE, color, attr);
                        if (n == 2
                            && curr->identifier.a_void != 0
                            && curr->selected) {
                            if (curr->count == -1L)
                                (void) putchar('+'); /* all selected */
                            else
                                (void)putchar('#'); /* count selected */
                        }
                        else
                            (void)putchar(*cp);
                    } /* for *cp */
                    if (n > attr_n && (color != NO_COLOR || attr != ATR_NONE))
                        toggle_menu_attr(FALSE, color, attr);
                } /* if npages > 0 */
            }
            else {
                page_start = 0;
                page_end = 0;
                page_lines = 0;
            }
            *rp = 0;
            /* remember how many explicit menu choices there are */
            resp_len = (int)strlen(resp);

            /* corner window - clear extra lines from last page */
            if (cw->offx) {
                for (n = page_lines + 1; n < cw->maxrow; n++) {
                    uwp_curs(window, 1, n);
                    uwp_cl_end();
                }
            }

            /* set extra chars.. */
            Strcat(resp, uwp_default_menu_cmds);
            Strcat(resp, " ");                  /* next page or end */
            Strcat(resp, "0123456789\033\n\r"); /* counts, quit */
            Strcat(resp, gacc);                 /* group accelerators */
            Strcat(resp, mapped_menu_cmds);

            if (cw->npages > 1)
                Sprintf(cw->morestr, "(%d of %d)", curr_page + 1,
                (int)cw->npages);
            else if (msave)
                Strcpy(cw->morestr, msave);
            else
                Strcpy(cw->morestr, uwp_defmorestr);

            uwp_curs(window, 1, page_lines);
            uwp_cl_end();
            dmore(cw, resp);
        }
        else {
            /* just put the cursor back... */
            uwp_curs(window, (int)strlen(cw->morestr) + 2, page_lines);
            uwp_xwaitforspace(resp);
        }

        really_morc = uwp_morc; /* (only used with MENU_EXPLICIT_CHOICE */
        if ((rp = index(resp, uwp_morc)) != 0 && rp < resp + resp_len)
            /* explicit menu selection; don't override it if it also
            happens to match a mapped menu command (such as ':' to
            look inside a container vs ':' to search) */
            uwp_morc = MENU_EXPLICIT_CHOICE;
        else
            uwp_morc = map_menu_cmd(uwp_morc);

        switch (uwp_morc) {
        case '0':
            /* special case: '0' is also the default ball class */
            if (!counting && index(gacc, uwp_morc))
                goto group_accel;
            /* fall through to count the zero */
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            count = (count * 10L) + (long)(uwp_morc - '0');
            /*
            * It is debatable whether we should allow 0 to
            * start a count.  There is no difference if the
            * item is selected.  If not selected, then
            * "0b" could mean:
            *
            *  count starting zero:    "zero b's"
            *  ignore starting zero:   "select b"
            *
            * At present I don't know which is better.
            */
            if (count != 0L) { /* ignore leading zeros */
                counting = TRUE;
                reset_count = FALSE;
            }
            break;
        case '\033': /* cancel - from counting or loop */
            if (!counting) {
                /* deselect everything */
                for (curr = cw->mlist; curr; curr = curr->next) {
                    curr->selected = FALSE;
                    curr->count = -1L;
                }
                cw->flags |= UWP_WIN_CANCELLED;
                finished = TRUE;
            }
            /* else only stop count */
            break;
        case '\0': /* finished (commit) */
        case '\n':
        case '\r':
            /* only finished if we are actually picking something */
            if (cw->how != PICK_NONE) {
                finished = TRUE;
                break;
            }
            /* else fall through */
        case ' ':
        case MENU_NEXT_PAGE:
            if (cw->npages > 0 && curr_page != cw->npages - 1) {
                curr_page++;
                page_start = 0;
            }
            else if (uwp_morc == ' ') {
                /* ' ' finishes menus here, but stop '>' doing the same. */
                finished = TRUE;
            }
            break;
        case MENU_PREVIOUS_PAGE:
            if (cw->npages > 0 && curr_page != 0) {
                --curr_page;
                page_start = 0;
            }
            break;
        case MENU_FIRST_PAGE:
            if (cw->npages > 0 && curr_page != 0) {
                page_start = 0;
                curr_page = 0;
            }
            break;
        case MENU_LAST_PAGE:
            if (cw->npages > 0 && curr_page != cw->npages - 1) {
                page_start = 0;
                curr_page = cw->npages - 1;
            }
            break;
        case MENU_SELECT_PAGE:
            if (cw->how == PICK_ANY)
                set_all_on_page(window, page_start, page_end);
            break;
        case MENU_UNSELECT_PAGE:
            unset_all_on_page(window, page_start, page_end);
            break;
        case MENU_INVERT_PAGE:
            if (cw->how == PICK_ANY)
                invert_all_on_page(window, page_start, page_end, 0);
            break;
        case MENU_SELECT_ALL:
            if (cw->how == PICK_ANY) {
                set_all_on_page(window, page_start, page_end);
                /* set the rest */
                for (curr = cw->mlist; curr; curr = curr->next)
                    if (curr->identifier.a_void && !curr->selected)
                        curr->selected = TRUE;
            }
            break;
        case MENU_UNSELECT_ALL:
            unset_all_on_page(window, page_start, page_end);
            /* unset the rest */
            for (curr = cw->mlist; curr; curr = curr->next)
                if (curr->identifier.a_void && curr->selected) {
                    curr->selected = FALSE;
                    curr->count = -1;
                }
            break;
        case MENU_INVERT_ALL:
            if (cw->how == PICK_ANY)
                invert_all(window, page_start, page_end, 0);
            break;
        case MENU_SEARCH:
            if (cw->how == PICK_NONE) {
                uwp_nhbell();
                break;
            }
            else {
                char searchbuf[BUFSZ + 2], tmpbuf[BUFSZ];
                boolean on_curr_page = FALSE;
                int lineno = 0;

                uwp_getlin("Search for:", tmpbuf);
                if (!tmpbuf[0] || tmpbuf[0] == '\033')
                    break;
                Sprintf(searchbuf, "*%s*", tmpbuf);

                for (curr = cw->mlist; curr; curr = curr->next) {
                    if (on_curr_page)
                        lineno++;
                    if (curr == page_start)
                        on_curr_page = TRUE;
                    else if (curr == page_end)
                        on_curr_page = FALSE;
                    if (curr->identifier.a_void
                        && pmatchi(searchbuf, curr->str)) {
                        toggle_menu_curr(window, curr, lineno, on_curr_page,
                            counting, count);
                        if (cw->how == PICK_ONE) {
                            finished = TRUE;
                            break;
                        }
                    }
                }
            }
            break;
        case MENU_EXPLICIT_CHOICE:
            uwp_morc = really_morc;
            /*FALLTHRU*/
        default:
            if (cw->how == PICK_NONE || !index(resp, uwp_morc)) {
                /* unacceptable input received */
                uwp_nhbell();
                break;
            }
            else if (index(gacc, uwp_morc)) {
            group_accel:
                /* group accelerator; for the PICK_ONE case, we know that
                it matches exactly one item in order to be in gacc[] */
                invert_all(window, page_start, page_end, uwp_morc);
                if (cw->how == PICK_ONE)
                    finished = TRUE;
                break;
            }
            /* find, toggle, and possibly update */
            for (n = 0, curr = page_start; curr != page_end;
                n++, curr = curr->next)
                if (uwp_morc == curr->selector) {
                    toggle_menu_curr(window, curr, n, TRUE, counting, count);
                    if (cw->how == PICK_ONE)
                        finished = TRUE;
                    break; /* from `for' loop */
                }
            break;
        }

    } /* while */
    cw->morestr = msave;
    free((genericptr_t)morestr);
}

STATIC_OVL void
process_text_window(window, cw)
winid window;
struct UwpWinDesc *cw;
{
    int i, n, attr;
    register char *cp;

    for (n = 0, i = 0; i < cw->maxrow; i++) {
        if (!cw->offx && (n + cw->offy == uwpDisplay->rows - 1)) {
            uwp_curs(window, 1, n);
            uwp_cl_end();
            dmore(cw, quitchars);
            if (uwp_morc == '\033') {
                cw->flags |= UWP_WIN_CANCELLED;
                break;
            }
            if (cw->offy) {
                uwp_curs(window, 1, 0);
                uwp_cl_eos();
            }
            else
                uwp_clear_screen();
            n = 0;
        }
        uwp_curs(window, 1, n++);
#ifdef H2344_BROKEN
        uwp_cl_end();
#else
        if (cw->offx)
            uwp_cl_end();
#endif
        if (cw->data[i]) {
            attr = cw->data[i][0] - 1;
            if (cw->offx) {
                (void)putchar(' ');
                ++uwpDisplay->curx;
            }
            uwp_term_start_attr(attr);
            for (cp = &cw->data[i][1];
#ifndef WIN32CON
                *cp && (int) ++uwpDisplay->curx < (int)uwpDisplay->cols;
                cp++)
#else
                *cp && (int)uwpDisplay->curx < (int)uwpDisplay->cols;
            cp++, uwpDisplay->curx++)
#endif
                (void)putchar(*cp);
            uwp_term_end_attr(attr);
        }
    }
    if (i == cw->maxrow) {
#ifdef H2344_BROKEN
        if (cw->type == NHW_TEXT) {
            uwp_curs(UWP_BASE_WINDOW, 1, (int)uwpDisplay->cury + 1);
            uwp_cl_eos();
        }
#endif
        uwp_curs(UWP_BASE_WINDOW, (int)cw->offx + 1,
            (cw->type == NHW_TEXT) ? (int)uwpDisplay->rows - 1 : n);
        uwp_cl_end();
        dmore(cw, quitchars);
        if (uwp_morc == '\033')
            cw->flags |= UWP_WIN_CANCELLED;
    }
}

/*ARGSUSED*/
void
uwp_display_nhwindow(window, blocking)
winid window;
boolean blocking; /* with ttys, all windows are blocking */
{
    register struct UwpWinDesc *cw = 0;
    short s_maxcol;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0)
        panic(uwp_winpanicstr, window);
    if (cw->flags & UWP_WIN_CANCELLED)
        return;
    uwpDisplay->lastwin = window;
    uwpDisplay->rawprint = 0;

    uwp_print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        if (uwpDisplay->toplin == 1) {
            uwp_more();
            uwpDisplay->toplin = 1; /* uwp_more resets this */
            uwp_clear_nhwindow(window);
        }
        else
            uwpDisplay->toplin = 0;
        cw->curx = cw->cury = 0;
        if (!cw->active)
            iflags.window_inited = TRUE;
        break;
    case NHW_MAP:
        uwp_end_glyphout();
        if (blocking) {
            if (!uwpDisplay->toplin)
                uwpDisplay->toplin = 1;
            uwp_display_nhwindow(WIN_MESSAGE, TRUE);
            return;
        }
    case UWP_NHW_BASE:
        (void)fflush(stdout);
        break;
    case NHW_TEXT:
        cw->maxcol = uwpDisplay->cols; /* force full-screen mode */
                                       /*FALLTHRU*/
    case NHW_MENU:
        cw->active = 1;
        /* cw->maxcol is a long, but its value is constrained to
        be <= uwpDisplay->cols, so is sure to fit within a short */
        s_maxcol = (short)cw->maxcol;
#ifdef H2344_BROKEN
        cw->offx = (cw->type == NHW_TEXT)
            ? 0
            : min(min(82, uwpDisplay->cols / 2),
                uwpDisplay->cols - s_maxcol - 1);
#else
        /* avoid converting to uchar before calculations are finished */
        cw->offx = (uchar)max((int)10,
            (int)(uwpDisplay->cols - s_maxcol - 1));
#endif
        if (cw->offx < 0)
            cw->offx = 0;
        if (cw->type == NHW_MENU)
            cw->offy = 0;
        if (uwpDisplay->toplin == 1)
            uwp_display_nhwindow(WIN_MESSAGE, TRUE);
#ifdef H2344_BROKEN
        if (cw->maxrow >= (int)uwpDisplay->rows
            || !iflags.menu_overlay)
#else
        if (cw->offx == 10 || cw->maxrow >= (int)uwpDisplay->rows
            || !iflags.menu_overlay)
#endif
        {
            cw->offx = 0;
            if (cw->offy || iflags.menu_overlay) {
                uwp_curs(window, 1, 0);
                uwp_cl_eos();
            }
            else
                uwp_clear_screen();
            uwpDisplay->toplin = 0;
        }
        else {
            if (WIN_MESSAGE != WIN_ERR)
                uwp_clear_nhwindow(WIN_MESSAGE);
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
uwp_dismiss_nhwindow(window)
winid window;
{
    register struct UwpWinDesc *cw = 0;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0)
        panic(uwp_winpanicstr, window);

    uwp_print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        if (uwpDisplay->toplin)
            uwp_display_nhwindow(WIN_MESSAGE, TRUE);
        /*FALLTHRU*/
    case NHW_STATUS:
    case UWP_NHW_BASE:
    case NHW_MAP:
        /*
        * these should only get dismissed when the game is going away
        * or suspending
        */
        uwp_curs(UWP_BASE_WINDOW, 1, (int)uwpDisplay->rows - 1);
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
uwp_destroy_nhwindow(window)
winid window;
{
    register struct UwpWinDesc *cw = 0;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0)
        panic(uwp_winpanicstr, window);

    if (cw->active)
        uwp_dismiss_nhwindow(window);
    if (cw->type == NHW_MESSAGE)
        iflags.window_inited = 0;
    if (cw->type == NHW_MAP)
        uwp_clear_screen();

    uwp_free_window_info(cw, TRUE);
    free((genericptr_t)cw);
    uwp_wins[window] = 0;
}

void
uwp_curs(window, x, y)
winid window;
register int x, y; /* not xchar: perhaps xchar is unsigned and
                   curx-x would be unsigned as well */
{
    struct UwpWinDesc *cw = 0;
    int cx = uwpDisplay->curx;
    int cy = uwpDisplay->cury;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0)
        panic(uwp_winpanicstr, window);
    uwpDisplay->lastwin = window;

    uwp_print_vt_code2(AVTC_SELECT_WINDOW, window);

#if defined(USE_TILES) && defined(MSDOS)
    adjust_cursor_flags(cw);
#endif
    cw->curx = --x; /* column 0 is never used */
    cw->cury = y;
#ifdef DEBUG
    if (x < 0 || y < 0 || y >= cw->rows || x > cw->cols) {
        const char *s = "[unknown type]";
        switch (cw->type) {
        case NHW_MESSAGE:
            s = "[topl window]";
            break;
        case NHW_STATUS:
            s = "[status window]";
            break;
        case NHW_MAP:
            s = "[map window]";
            break;
        case NHW_MENU:
            s = "[corner window]";
            break;
        case NHW_TEXT:
            s = "[text window]";
            break;
        case UWP_NHW_BASE:
            s = "[base window]";
            break;
        }
        debugpline4("bad curs positioning win %d %s (%d,%d)", window, s, x,
            y);
        return;
    }
#endif
    x += cw->offx;
    y += cw->offy;

#ifdef CLIPPING
    if (uwp_clipping && window == WIN_MAP) {
        x -= uwp_clipx;
        y -= uwp_clipy;
    }
#endif

    if (y == cy && x == cx)
        return;

    if (cw->type == NHW_MAP)
        uwp_end_glyphout();

#ifndef NO_TERMS
    if (!nh_ND && (cx != x || x <= 3)) { /* Extremely primitive */
        uwp_cmov(x, y);                      /* bunker!wtm */
        return;
    }
#endif

    if ((cy -= y) < 0)
        cy = -cy;
    if ((cx -= x) < 0)
        cx = -cx;
    if (cy <= 3 && cx <= 3) {
        uwp_nocmov(x, y);
#ifndef NO_TERMS
    }
    else if ((x <= 3 && cy <= 3) || (!nh_CM && x < cx)) {
        (void)putchar('\r');
        uwpDisplay->curx = 0;
        uwp_nocmov(x, y);
    }
    else if (!nh_CM) {
        uwp_nocmov(x, y);
#endif
    }
    else
        uwp_cmov(x, y);

    uwpDisplay->curx = x;
    uwpDisplay->cury = y;
}

STATIC_OVL void
uwp_putsym(window, x, y, ch)
winid window;
int x, y;
char ch;
{
    register struct UwpWinDesc *cw = 0;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0)
        panic(uwp_winpanicstr, window);

    uwp_print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_STATUS:
    case NHW_MAP:
    case UWP_NHW_BASE:
        uwp_curs(window, x, y);
        (void)putchar(ch);
        uwpDisplay->curx++;
        cw->curx++;
        break;
    case NHW_MESSAGE:
    case NHW_MENU:
    case NHW_TEXT:
        impossible("Can't putsym to window type %d", cw->type);
        break;
    }
}

STATIC_OVL const char *
compress_str(str)
const char *str;
{
    static char cbuf[BUFSZ];

    /* compress out consecutive spaces if line is too long;
    topline wrapping converts space at wrap point into newline,
    we reverse that here */
    if ((int)strlen(str) >= CO || index(str, '\n')) {
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
uwp_putstr(window, attr, str)
winid window;
int attr;
const char *str;
{
    register struct UwpWinDesc *cw = 0;
    register char *ob;
    register const char *nb;
    register long i, j, n0;

    /* Assume there's a real problem if the window is missing --
    * probably a panic message
    */
    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0) {
        uwp_raw_print(str);
        return;
    }

    if (str == (const char *)0
        || ((cw->flags & UWP_WIN_CANCELLED) && (cw->type != NHW_MESSAGE)))
        return;
    if (cw->type != NHW_MESSAGE)
        str = compress_str(str);

    uwpDisplay->lastwin = window;

    uwp_print_vt_code2(AVTC_SELECT_WINDOW, window);

    switch (cw->type) {
    case NHW_MESSAGE:
        /* really do this later */
#if defined(USER_SOUNDS) && defined(WIN32CON)
        play_sound_for_message(str);
#endif
        uwp_update_topl(str);
        break;

    case NHW_STATUS:
        ob = &cw->data[cw->cury][j = cw->curx];
        if (context.botlx)
            *ob = 0;
        if (!cw->cury && (int)strlen(str) >= CO) {
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
                    uwp_curs(WIN_STATUS, i, cw->cury);
                    uwp_cl_end();
                }
                break;
            }
            if (*ob != *nb)
                uwp_putsym(WIN_STATUS, i, cw->cury, *nb);
            if (*ob)
                ob++;
        }

        (void)strncpy(&cw->data[cw->cury][j], str, cw->cols - j - 1);
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
        uwp_curs(window, cw->curx + 1, cw->cury);
        uwp_term_start_attr(attr);
        while (*str && (int)uwpDisplay->curx < (int)uwpDisplay->cols - 1) {
            (void)putchar(*str);
            str++;
            uwpDisplay->curx++;
        }
        cw->curx = 0;
        cw->cury++;
        uwp_term_end_attr(attr);
        break;
    case UWP_NHW_BASE:
        uwp_curs(window, cw->curx + 1, cw->cury);
        uwp_term_start_attr(attr);
        while (*str) {
            if ((int)uwpDisplay->curx >= (int)uwpDisplay->cols - 1) {
                cw->curx = 0;
                cw->cury++;
                uwp_curs(window, cw->curx + 1, cw->cury);
            }
            (void)putchar(*str);
            str++;
            uwpDisplay->curx++;
        }
        cw->curx = 0;
        cw->cury++;
        uwp_term_end_attr(attr);
        break;
    case NHW_MENU:
    case NHW_TEXT:
#ifdef H2344_BROKEN
        if (cw->type == NHW_TEXT
            && (cw->cury + cw->offy) == uwpDisplay->rows - 1)
#else
        if (cw->type == NHW_TEXT && cw->cury == uwpDisplay->rows - 1)
#endif
        {
            /* not a menu, so save memory and output 1 page at a time */
            cw->maxcol = uwpDisplay->cols; /* force full-screen mode */
            uwp_display_nhwindow(window, TRUE);
            for (i = 0; i < cw->maxrow; i++)
                if (cw->data[i]) {
                    free((genericptr_t)cw->data[i]);
                    cw->data[i] = 0;
                }
            cw->maxrow = cw->cury = 0;
        }
        /* always grows one at a time, but alloc 12 at a time */
        if (cw->cury >= cw->rows) {
            char **tmp;

            cw->rows += 12;
            tmp = (char **)alloc(sizeof(char *) * (unsigned)cw->rows);
            for (i = 0; i < cw->maxrow; i++)
                tmp[i] = cw->data[i];
            if (cw->data)
                free((genericptr_t)cw->data);
            cw->data = tmp;

            for (i = cw->maxrow; i < cw->rows; i++)
                cw->data[i] = 0;
        }
        if (cw->data[cw->cury])
            free((genericptr_t)cw->data[cw->cury]);
        n0 = (long)strlen(str) + 1L;
        ob = cw->data[cw->cury] = (char *)alloc((unsigned)n0 + 1);
        *ob++ = (char)(attr + 1); /* avoid nuls, for convenience */
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
                uwp_putstr(window, attr, &str[i]);
            }
        }
        break;
    }
}

void
uwp_display_file(fname, complain)
const char *fname;
boolean complain;
{
#ifdef DEF_PAGER /* this implies that UNIX is defined */
    {
        /* use external pager; this may give security problems */
        register int fd = open(fname, 0);

        if (fd < 0) {
            if (complain)
                pline("Cannot open %s.", fname);
            else
                docrt();
            return;
        }
        if (child(1)) {
            /* Now that child() does a setuid(getuid()) and a chdir(),
            we may not be able to open file fname anymore, so make
            it stdin. */
            (void)close(0);
            if (dup(fd)) {
                if (complain)
                    raw_printf("Cannot open %s as stdin.", fname);
            }
            else {
                (void)execlp(catmore, "page", (char *)0);
                if (complain)
                    raw_printf("Cannot exec %s.", catmore);
            }
            if (complain)
                sleep(10); /* want to wait_synch() but stdin is gone */
            terminate(EXIT_FAILURE);
        }
        (void)close(fd);
#ifdef notyet
        winch_seen = 0;
#endif
    }
#else /* DEF_PAGER */
    {
        dlb *f;
        char buf[BUFSZ];
        char *cr;

        uwp_clear_nhwindow(WIN_MESSAGE);
        f = dlb_fopen(fname, "r");
        if (!f) {
            if (complain) {
                uwp_home();
                uwp_mark_synch();
                uwp_raw_print("");
                perror(fname);
                uwp_wait_synch();
                pline("Cannot open \"%s\".", fname);
            }
            else if (u.ux)
                docrt();
        }
        else {
            winid datawin = uwp_create_nhwindow(NHW_TEXT);
            boolean empty = TRUE;

            if (complain
#ifndef NO_TERMS
                && nh_CD
#endif
                ) {
                /* attempt to scroll text below map window if there's room */
                uwp_wins[datawin]->offy = uwp_wins[WIN_STATUS]->offy + 3;
                if ((int)uwp_wins[datawin]->offy + 12 > (int)uwpDisplay->rows)
                    uwp_wins[datawin]->offy = 0;
            }
            while (dlb_fgets(buf, BUFSZ, f)) {
                if ((cr = index(buf, '\n')) != 0)
                    *cr = 0;
#ifdef MSDOS
                if ((cr = index(buf, '\r')) != 0)
                    *cr = 0;
#endif
                if (index(buf, '\t') != 0)
                    (void) tabexpand(buf);
                empty = FALSE;
                uwp_putstr(datawin, 0, buf);
                if (uwp_wins[datawin]->flags & UWP_WIN_CANCELLED)
                    break;
            }
            if (!empty)
                uwp_display_nhwindow(datawin, FALSE);
            uwp_destroy_nhwindow(datawin);
            (void)dlb_fclose(f);
        }
    }
#endif /* DEF_PAGER */
}

void
uwp_start_menu(window)
winid window;
{
    uwp_clear_nhwindow(window);
    return;
}

/*ARGSUSED*/
/*
* Add a menu item to the beginning of the menu list.  This list is reversed
* later.
*/
void
uwp_add_menu(window, glyph, identifier, ch, gch, attr, str, preselected)
winid window;               /* window to use, must be of type NHW_MENU */
int glyph UNUSED;           /* glyph to display with item (not used) */
const anything *identifier; /* what to return if selected */
char ch;                    /* keyboard accelerator (0 = pick our own) */
char gch;                   /* group accelerator (0 = no group) */
int attr;                   /* attribute for string (like uwp_putstr()) */
const char *str;            /* menu string */
boolean preselected;        /* item is marked as selected */
{
    register struct UwpWinDesc *cw = 0;
    uwp_menu_item *item;
    const char *newstr;
    char buf[4 + BUFSZ];

    if (str == (const char *)0)
        return;

    if (window == WIN_ERR
        || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0
        || cw->type != NHW_MENU)
        panic(uwp_winpanicstr, window);

    cw->nitems++;
    if (identifier->a_void) {
        int len = strlen(str);

        if (len >= BUFSZ) {
            /* We *think* everything's coming in off at most BUFSZ bufs... */
            impossible("Menu item too long (%d).", len);
            len = BUFSZ - 1;
        }
        Sprintf(buf, "%c - ", ch ? ch : '?');
        (void)strncpy(buf + 4, str, len);
        buf[4 + len] = '\0';
        newstr = buf;
    }
    else
        newstr = str;

    item = (uwp_menu_item *)alloc(sizeof(uwp_menu_item));
    item->identifier = *identifier;
    item->count = -1L;
    item->selected = preselected;
    item->selector = ch;
    item->gselector = gch;
    item->attr = attr;
    item->str = dupstr(newstr ? newstr : "");

    item->next = cw->mlist;
    cw->mlist = item;
}

/* Invert the given list, can handle NULL as an input. */
STATIC_OVL uwp_menu_item *
reverse(curr)
uwp_menu_item *curr;
{
    uwp_menu_item *next, *head = 0;

    while (curr) {
        next = curr->next;
        curr->next = head;
        head = curr;
        curr = next;
    }
    return head;
}

/*
* End a menu in this window, window must a type NHW_MENU.  This routine
* processes the string list.  We calculate the # of pages, then assign
* keyboard accelerators as needed.  Finally we decide on the width and
* height of the window.
*/
void
uwp_end_menu(window, prompt)
winid window;       /* menu to use */
const char *prompt; /* prompt to for menu */
{
    struct UwpWinDesc *cw = 0;
    uwp_menu_item *curr;
    short len;
    int lmax, n;
    char menu_ch;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0
        || cw->type != NHW_MENU)
        panic(uwp_winpanicstr, window);

    /* Reverse the list so that items are in correct order. */
    cw->mlist = reverse(cw->mlist);

    /* Put the prompt at the beginning of the menu. */
    if (prompt) {
        anything any;

        any = zeroany; /* not selectable */
        uwp_add_menu(window, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
            MENU_UNSELECTED);
        uwp_add_menu(window, NO_GLYPH, &any, 0, 0, ATR_NONE, prompt,
            MENU_UNSELECTED);
    }

    /* XXX another magic number? 52 */
    lmax = min(52, (int)uwpDisplay->rows - 1);    /* # lines per page */
    cw->npages = (cw->nitems + (lmax - 1)) / lmax; /* # of pages */

                                                   /* make sure page list is large enough */
    if (cw->plist_size < cw->npages + 1 /*need 1 slot beyond last*/) {
        if (cw->plist)
            free((genericptr_t)cw->plist);
        cw->plist_size = cw->npages + 1;
        cw->plist = (uwp_menu_item **)alloc(cw->plist_size
            * sizeof(uwp_menu_item *));
    }

    cw->cols = 0;  /* cols is set when the win is initialized... (why?) */
    menu_ch = '?'; /* lint suppression */
    for (n = 0, curr = cw->mlist; curr; n++, curr = curr->next) {
        /* set page boundaries and character accelerators */
        if ((n % lmax) == 0) {
            menu_ch = 'a';
            cw->plist[n / lmax] = curr;
        }
        if (curr->identifier.a_void && !curr->selector) {
            curr->str[0] = curr->selector = menu_ch;
            if (menu_ch++ == 'z')
                menu_ch = 'A';
        }

        /* cut off any lines that are too long */
        len = strlen(curr->str) + 2; /* extra space at beg & end */
        if (len > (int)uwpDisplay->cols) {
            curr->str[uwpDisplay->cols - 2] = 0;
            len = uwpDisplay->cols;
        }
        if (len > cw->cols)
            cw->cols = len;
    }
    cw->plist[cw->npages] = 0; /* plist terminator */

                               /*
                               * If greater than 1 page, morestr is "(x of y) " otherwise, "(end) "
                               */
    if (cw->npages > 1) {
        char buf[QBUFSZ];
        /* produce the largest demo string */
        Sprintf(buf, "(%ld of %ld) ", cw->npages, cw->npages);
        len = strlen(buf);
        cw->morestr = dupstr("");
    }
    else {
        cw->morestr = dupstr("(end) ");
        len = strlen(cw->morestr);
    }

    if (len > (int)uwpDisplay->cols) {
        /* truncate the prompt if it's too long for the screen */
        if (cw->npages <= 1) /* only str in single page case */
            cw->morestr[uwpDisplay->cols] = 0;
        len = uwpDisplay->cols;
    }
    if (len > cw->cols)
        cw->cols = len;

    cw->maxcol = cw->cols;

    /*
    * The number of lines in the first page plus the morestr will be the
    * maximum size of the window.
    */
    if (cw->npages > 1)
        cw->maxrow = cw->rows = lmax + 1;
    else
        cw->maxrow = cw->rows = cw->nitems + 1;
}

int
uwp_select_menu(window, how, menu_list)
winid window;
int how;
menu_item **menu_list;
{
    register struct UwpWinDesc *cw = 0;
    uwp_menu_item *curr;
    menu_item *mi;
    int n, cancelled;

    if (window == WIN_ERR || (cw = uwp_wins[window]) == (struct UwpWinDesc *) 0
        || cw->type != NHW_MENU)
        panic(uwp_winpanicstr, window);

    *menu_list = (menu_item *)0;
    cw->how = (short)how;
    uwp_morc = 0;
    uwp_display_nhwindow(window, TRUE);
    cancelled = !!(cw->flags & UWP_WIN_CANCELLED);
    uwp_dismiss_nhwindow(window); /* does not destroy window data */

    if (cancelled) {
        n = -1;
    }
    else {
        for (n = 0, curr = cw->mlist; curr; curr = curr->next)
            if (curr->selected)
                n++;
    }

    if (n > 0) {
        *menu_list = (menu_item *)alloc(n * sizeof(menu_item));
        for (mi = *menu_list, curr = cw->mlist; curr; curr = curr->next)
            if (curr->selected) {
                mi->item = curr->identifier;
                mi->count = curr->count;
                mi++;
            }
    }

    return n;
}

/* special hack for treating top line --More-- as a one item menu */
char
uwp_message_menu(let, how, mesg)
char let;
int how;
const char *mesg;
{
    /* "menu" without selection; use ordinary pline, no uwp_more() */
    if (how == PICK_NONE) {
        pline("%s", mesg);
        return 0;
    }

    uwpDisplay->dismiss_more = let;
    uwp_morc = 0;
    /* barebones pline(); since we're only supposed to be called after
    response to a prompt, we'll assume that the display is up to date */
    uwp_putstr(WIN_MESSAGE, 0, mesg);
    /* if `mesg' didn't wrap (triggering --More--), force --More-- now */
    if (uwpDisplay->toplin == 1) {
        uwp_more();
        uwpDisplay->toplin = 1; /* uwp_more resets this */
        uwp_clear_nhwindow(WIN_MESSAGE);
    }
    /* normally <ESC> means skip further messages, but in this case
    it means cancel the current prompt; any other messages should
    continue to be output normally */
    uwp_wins[WIN_MESSAGE]->flags &= ~UWP_WIN_CANCELLED;
    uwpDisplay->dismiss_more = 0;

    return ((how == PICK_ONE && uwp_morc == let) || uwp_morc == '\033') ? uwp_morc : '\0';
}

void
uwp_update_inventory()
{
    return;
}

void
uwp_mark_synch()
{
    (void)fflush(stdout);
}

void
uwp_wait_synch()
{
    /* we just need to make sure all windows are synch'd */
    if (!uwpDisplay || uwpDisplay->rawprint) {
        getret();
        if (uwpDisplay)
            uwpDisplay->rawprint = 0;
    }
    else {
        uwp_display_nhwindow(WIN_MAP, FALSE);
        if (uwpDisplay->inmore) {
            uwp_addtopl("--More--");
            (void)fflush(stdout);
        }
        else if (uwpDisplay->inread > program_state.gameover) {
            /* this can only happen if we were reading and got interrupted */
            uwpDisplay->toplin = 3;
            /* do this twice; 1st time gets the Quit? message again */
            (void)uwp_doprev_message();
            (void)uwp_doprev_message();
            uwpDisplay->intr++;
            (void)fflush(stdout);
        }
    }
}

void
uwp_docorner(xmin, ymax)
register int xmin, ymax;
{
    register int y;
    register struct UwpWinDesc *cw = uwp_wins[WIN_MAP];

#if 0   /* this optimization is not valuable enough to justify
        abusing core internals... */
    if (u.uswallow) { /* Can be done uwp_more efficiently */
        swallowed(1);
        /* without this flush, if we happen to follow --More-- displayed in
        leftmost column, the cursor gets left in the wrong place after
        <uwp_docorner<more<uwp_update_topl<uwp_putstr calls unwind back to core */
        flush_screen(0);
        return;
    }
#endif /*0*/

#if defined(SIGWINCH) && defined(CLIPPING)
    if (ymax > LI)
        ymax = LI; /* can happen if window gets smaller */
#endif
    for (y = 0; y < ymax; y++) {
        uwp_curs(UWP_BASE_WINDOW, xmin, y); /* move cursor */
        uwp_cl_end();                       /* clear to end of line */
#ifdef CLIPPING
        if (y < (int)cw->offy || y + uwp_clipy > ROWNO)
            continue; /* only refresh board */
#if defined(USE_TILES) && defined(MSDOS)
        if (iflags.tile_view)
            row_refresh((xmin / 2) + uwp_clipx - ((int)cw->offx / 2), COLNO - 1,
                y + uwp_clipy - (int)cw->offy);
        else
#endif
            row_refresh(xmin + uwp_clipx - (int)cw->offx, COLNO - 1,
                y + uwp_clipy - (int)cw->offy);
#else
        if (y < cw->offy || y > ROWNO)
            continue; /* only refresh board  */
        row_refresh(xmin - (int)cw->offx, COLNO - 1, y - (int)cw->offy);
#endif
    }

    uwp_end_glyphout();
    if (ymax >= (int)uwp_wins[WIN_STATUS]->offy) {
        /* we have wrecked the bottom line */
        context.botlx = 1;
        bot();
    }
}

void
uwp_end_glyphout()
{
#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
    if (GFlag) {
        GFlag = FALSE;
        graph_off();
    }
#endif
#ifdef TEXTCOLOR
    if (uwpDisplay->color != NO_COLOR) {
        uwp_term_end_color();
        uwpDisplay->color = NO_COLOR;
    }
#endif
}

#ifndef WIN32
void
uwp_g_putch(in_ch)
int in_ch;
{
    register char ch = (char)in_ch;

#if defined(ASCIIGRAPH) && !defined(NO_TERMS)
    if (SYMHANDLING(H_IBM) || iflags.eight_bit_tty) {
        /* IBM-compatible displays don't need other stuff */
        (void)putchar(ch);
    }
    else if (ch & 0x80) {
        if (!GFlag || HE_resets_AS) {
            graph_on();
            GFlag = TRUE;
        }
        (void)putchar((ch ^ 0x80)); /* Strip 8th bit */
    }
    else {
        if (GFlag) {
            graph_off();
            GFlag = FALSE;
        }
        (void)putchar(ch);
    }

#else
    (void)putchar(ch);

#endif /* ASCIIGRAPH && !NO_TERMS */

    return;
}
#endif /* !WIN32 */

#ifdef CLIPPING
void
setclipped()
{
    uwp_clipping = TRUE;
    uwp_clipx = uwp_clipy = 0;
    uwp_clipxmax = CO;
    uwp_clipymax = LI - 3;
}

void
uwp_cliparound(x, y)
int x, y;
{
    extern boolean restoring;
    int oldx = uwp_clipx, oldy = uwp_clipy;

    if (!uwp_clipping)
        return;
    if (x < uwp_clipx + 5) {
        uwp_clipx = max(0, x - 20);
        uwp_clipxmax = uwp_clipx + CO;
    }
    else if (x > uwp_clipxmax - 5) {
        uwp_clipxmax = min(COLNO, uwp_clipxmax + 20);
        uwp_clipx = uwp_clipxmax - CO;
    }
    if (y < uwp_clipy + 2) {
        uwp_clipy = max(0, y - (uwp_clipymax - uwp_clipy) / 2);
        uwp_clipymax = uwp_clipy + (LI - 3);
    }
    else if (y > uwp_clipymax - 2) {
        uwp_clipymax = min(ROWNO, uwp_clipymax + (uwp_clipymax - uwp_clipy) / 2);
        uwp_clipy = uwp_clipymax - (LI - 3);
    }
    if (uwp_clipx != oldx || uwp_clipy != oldy) {
        if (on_level(&u.uz0, &u.uz) && !restoring)
            (void) doredraw();
    }
}
#endif /* CLIPPING */

/*
*  uwp_print_glyph
*
*  Print the glyph to the output device.  Don't flush the output device.
*
*  Since this is only called from show_glyph(), it is assumed that the
*  position and glyph are always correct (checked there)!
*/

void
uwp_print_glyph(window, x, y, glyph, bkglyph)
winid window;
xchar x, y;
int glyph;
int bkglyph UNUSED;
{
    int ch;
    boolean reverse_on = FALSE;
    int color;
    unsigned special;

#ifdef CLIPPING
    if (uwp_clipping) {
        if (x <= uwp_clipx || y < uwp_clipy || x >= uwp_clipxmax || y >= uwp_clipymax)
            return;
    }
#endif
    /* map glyph to character and color */
    (void)mapglyph(glyph, &ch, &color, &special, x, y);

    uwp_print_vt_code2(AVTC_SELECT_WINDOW, window);

    /* Move the cursor. */
    uwp_curs(window, x, y);

    uwp_print_vt_code3(AVTC_GLYPH_START, glyph2tile[glyph], special);

#ifndef NO_TERMS
    if (ul_hack && ch == '_') { /* non-destructive underscore */
        (void)putchar((char) ' ');
        backsp();
    }
#endif

#ifdef TEXTCOLOR
    if (color != uwpDisplay->color) {
        if (uwpDisplay->color != NO_COLOR)
            uwp_term_end_color();
        uwpDisplay->color = color;
        if (color != NO_COLOR)
            uwp_term_start_color(color);
    }
#endif /* TEXTCOLOR */

    /* must be after color check; uwp_term_end_color may turn off inverse too */
    if (((special & MG_PET) && iflags.hilite_pet)
        || ((special & MG_OBJPILE) && iflags.hilite_pile)
        || ((special & MG_DETECT) && iflags.use_inverse)
        || ((special & MG_BW_LAVA) && iflags.use_inverse)) {
        uwp_term_start_attr(ATR_INVERSE);
        reverse_on = TRUE;
    }

#if defined(USE_TILES) && defined(MSDOS)
    if (iflags.grmode && iflags.tile_view)
        xputg(glyph, ch, special);
    else
#endif
        uwp_g_putch(ch); /* print the character */

    if (reverse_on) {
        uwp_term_end_attr(ATR_INVERSE);
#ifdef TEXTCOLOR
        /* turn off color as well, ATR_INVERSE may have done this already */
        if (uwpDisplay->color != NO_COLOR) {
            uwp_term_end_color();
            uwpDisplay->color = NO_COLOR;
        }
#endif
    }

    uwp_print_vt_code1(AVTC_GLYPH_END);

    uwp_wins[window]->curx++; /* one character over */
    uwpDisplay->curx++;   /* the real cursor moved too */
}

void
uwp_raw_print(str)
const char *str;
{
    if (uwpDisplay)
        uwpDisplay->rawprint++;
    uwp_print_vt_code2(AVTC_SELECT_WINDOW, UWP_NHW_BASE);
#if defined(MICRO) || defined(WIN32CON)
    uwp_msmsg("%s\n", str);
#else
    puts(str);
    (void)fflush(stdout);
#endif
}

void
uwp_raw_print_bold(str)
const char *str;
{
    if (uwpDisplay)
        uwpDisplay->rawprint++;
    uwp_print_vt_code2(AVTC_SELECT_WINDOW, UWP_NHW_BASE);
    uwp_term_start_raw_bold();
#if defined(MICRO) || defined(WIN32CON)
    uwp_msmsg("%s", str);
#else
    (void)fputs(str, stdout);
#endif
    uwp_term_end_raw_bold();
#if defined(MICRO) || defined(WIN32CON)
    uwp_msmsg("\n");
#else
    puts("");
    (void)fflush(stdout);
#endif
}

int
uwp_nhgetch()
{
    int i;
#ifdef UNIX
    /* kludge alert: Some Unix variants return funny values if getc()
    * is called, interrupted, and then called again.  There
    * is non-reentrant code in the internal _filbuf() routine, called by
    * getc().
    */
    static volatile int nesting = 0;
    char nestbuf;
#endif

    uwp_print_vt_code1(AVTC_INLINE_SYNC);
    (void)fflush(stdout);
    /* Note: if raw_print() and wait_synch() get called to report terminal
    * initialization problems, then uwp_wins[] and uwpDisplay might not be
    * available yet.  Such problems will probably be fatal before we get
    * here, but validate those pointers just in case...
    */
    if (WIN_MESSAGE != WIN_ERR && uwp_wins[WIN_MESSAGE])
        uwp_wins[WIN_MESSAGE]->flags &= ~UWP_WIN_STOP;
#ifdef UNIX
    i = (++nesting == 1) ? uwp_tgetch()
        : (read(fileno(stdin), (genericptr_t)&nestbuf, 1)
            == 1) ? (int)nestbuf : EOF;
    --nesting;
#else
    i = uwp_tgetch();
#endif
    if (!i)
        i = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    else if (i == EOF)
        i = '\033'; /* same for EOF */
    if (uwpDisplay && uwpDisplay->toplin == 1)
        uwpDisplay->toplin = 2;
#ifdef UWP_TILES_ESCCODES
    {
        /* hack to force output of the window select code */
        int tmp = vt_tile_current_window;
        vt_tile_current_window++;
        uwp_print_vt_code2(AVTC_SELECT_WINDOW, tmp);
    }
#endif /* UWP_TILES_ESCCODES */
    return i;
}

/*
* return a key, or 0, in which case a mouse button was pressed
* mouse events should be returned as character postitions in the map window.
* Since normal tty's don't have mice, just return a key.
*/
/*ARGSUSED*/
int
uwp_nh_poskey(x, y, mod)
int *x, *y, *mod;
{
#if defined(WIN32CON)
    int i;
    (void)fflush(stdout);
    /* Note: if raw_print() and wait_synch() get called to report terminal
    * initialization problems, then uwp_wins[] and uwpDisplay might not be
    * available yet.  Such problems will probably be fatal before we get
    * here, but validate those pointers just in case...
    */
    if (WIN_MESSAGE != WIN_ERR && uwp_wins[WIN_MESSAGE])
        uwp_wins[WIN_MESSAGE]->flags &= ~UWP_WIN_STOP;
    i = uwp_ntposkey(x, y, mod);
    if (!i && mod && (*mod == 0 || *mod == EOF))
        i = '\033'; /* map NUL or EOF to ESC, nethack doesn't expect either */
    if (uwpDisplay && uwpDisplay->toplin == 1)
        uwpDisplay->toplin = 2;
    return i;
#else /* !WIN32CON */
    nhUse(x);
    nhUse(y);
    nhUse(mod);

    return uwp_nhgetch();
#endif /* ?WIN32CON */
}

void
win_uwp_init(dir)
int dir;
{
    if (dir != WININIT)
        return;
#if defined(WIN32CON)
    if (!strncmpi(windowprocs.name, "tty", 3))
        nttty_open(0);
#endif
    return;
}

// TODO: This shoudl be move to system support which
//       in turn leverages either the windowing system
//       or raw output system.
void getreturn(const char * str)
{
    uwp_msmsg("Hit <Enter> %s.", str);

    while (pgetchar() != '\n')
        ;
    return;
}



#ifdef POSITIONBAR
void
uwp_update_positionbar(posbar)
char *posbar;
{
#ifdef MSDOS
    video_update_positionbar(posbar);
#endif
}
#endif

#ifdef STATUS_VIA_WINDOWPORT
/*
* The following data structures come from the genl_ routines in
* src/windows.c and as such are considered to be on the window-port
* "side" of things, rather than the NetHack-core "side" of things.
*/

extern const char *status_fieldnm[MAXBLSTATS];
extern const char *status_fieldfmt[MAXBLSTATS];
extern char *status_vals[MAXBLSTATS];
extern boolean status_activefields[MAXBLSTATS];
extern winid WIN_STATUS;

#ifdef STATUS_HILITES
typedef struct hilite_data_struct {
    int thresholdtype;
    anything threshold;
    int behavior;
    int under;
    int over;
} hilite_data_t;
static hilite_data_t uwp_status_hilites[MAXBLSTATS];
static int uwp_status_colors[MAXBLSTATS];

struct color_option {
    int color;
    int attr_bits;
};

static void FDECL(start_color_option, (struct color_option));
static void FDECL(end_color_option, (struct color_option));
static void FDECL(apply_color_option, (struct color_option, const char *));
static void FDECL(add_colored_text, (const char *, char *));
#endif

void
uwp_status_init()
{
    int i;

    /* let genl_status_init do most of the initialization */
    genl_status_init();

    for (i = 0; i < MAXBLSTATS; ++i) {
#ifdef STATUS_HILITES
        uwp_status_colors[i] = NO_COLOR; /* no color */
        uwp_status_hilites[i].thresholdtype = 0;
        uwp_status_hilites[i].behavior = BL_TH_NONE;
        uwp_status_hilites[i].under = BL_HILITE_NONE;
        uwp_status_hilites[i].over = BL_HILITE_NONE;
#endif /* STATUS_HILITES */
    }
}

/*
*  *_status_update()
*      -- update the value of a status field.
*      -- the fldindex identifies which field is changing and
*         is an integer index value from botl.h
*      -- fldindex could be any one of the following from botl.h:
*         BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
*         BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX,
*         BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, BL_HPMAX,
*         BL_LEVELDESC, BL_EXP, BL_CONDITION
*      -- fldindex could also be BL_FLUSH (-1), which is not really
*         a field index, but is a special trigger to tell the
*         windowport that it should redisplay all its status fields,
*         even if no changes have been presented to it.
*      -- ptr is usually a "char *", unless fldindex is BL_CONDITION.
*         If fldindex is BL_CONDITION, then ptr is a long value with
*         any or none of the following bits set (from botl.h):
*              BL_MASK_STONE           0x00000001L
*              BL_MASK_SLIME           0x00000002L
*              BL_MASK_STRNGL          0x00000004L
*              BL_MASK_FOODPOIS        0x00000008L
*              BL_MASK_TERMILL         0x00000010L
*              BL_MASK_BLIND           0x00000020L
*              BL_MASK_DEAF            0x00000040L
*              BL_MASK_STUN            0x00000080L
*              BL_MASK_CONF            0x00000100L
*              BL_MASK_HALLU           0x00000200L
*              BL_MASK_LEV             0x00000400L
*              BL_MASK_FLY             0x00000800L
*              BL_MASK_RIDE            0x00001000L
*      -- The value passed for BL_GOLD includes an encoded leading
*         symbol for GOLD "\GXXXXNNNN:nnn". If the window port needs to use
*         the textual gold amount without the leading "$:" the port will
*         have to skip past ':' in the passed "ptr" for the BL_GOLD case.
*/
void
uwp_status_update(fldidx, ptr, chg, percent)
int fldidx, chg, percent;
genericptr_t ptr;
{
    long cond, *condptr = (long *)ptr;
    register int i;
    char *text = (char *)ptr;
    /* Mapping BL attributes to tty attributes
    * BL_HILITE_NONE     -1 + 3 = 2 (statusattr[2])
    * BL_HILITE_INVERSE  -2 + 3 = 1 (statusattr[1])
    * BL_HILITE_BOLD     -3 + 3 = 0 (statusattr[0])
    */
    long value = -1L;
    static boolean beenhere = FALSE;
    enum statusfields fieldorder[2][15] = {
        { BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH, BL_ALIGN,
        BL_SCORE, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH, BL_FLUSH,
        BL_FLUSH },
        { BL_LEVELDESC, BL_GOLD, BL_HP, BL_HPMAX, BL_ENE, BL_ENEMAX,
        BL_AC, BL_XP, BL_EXP, BL_HD, BL_TIME, BL_HUNGER,
        BL_CAP, BL_CONDITION, BL_FLUSH }
    };
#ifdef STATUS_HILITES
    static int statusattr[] = { ATR_BOLD, ATR_INVERSE, ATR_NONE };
    int attridx = 0;
#else
    nhUse(chg);
    nhUse(percent);
#endif

    if (fldidx != BL_FLUSH) {
        if (!status_activefields[fldidx])
            return;
        switch (fldidx) {
        case BL_CONDITION:
            cond = *condptr;
            *status_vals[fldidx] = '\0';
            if (cond & BL_MASK_STONE)
                Strcat(status_vals[fldidx], " Stone");
            if (cond & BL_MASK_SLIME)
                Strcat(status_vals[fldidx], " Slime");
            if (cond & BL_MASK_STRNGL)
                Strcat(status_vals[fldidx], " Strngl");
            if (cond & BL_MASK_FOODPOIS)
                Strcat(status_vals[fldidx], " FoodPois");
            if (cond & BL_MASK_TERMILL)
                Strcat(status_vals[fldidx], " TermIll");
            if (cond & BL_MASK_BLIND)
                Strcat(status_vals[fldidx], " Blind");
            if (cond & BL_MASK_DEAF)
                Strcat(status_vals[fldidx], " Deaf");
            if (cond & BL_MASK_STUN)
                Strcat(status_vals[fldidx], " Stun");
            if (cond & BL_MASK_CONF)
                Strcat(status_vals[fldidx], " Conf");
            if (cond & BL_MASK_HALLU)
                Strcat(status_vals[fldidx], " Hallu");
            if (cond & BL_MASK_LEV)
                Strcat(status_vals[fldidx], " Lev");
            if (cond & BL_MASK_FLY)
                Strcat(status_vals[fldidx], " Fly");
            if (cond & BL_MASK_RIDE)
                Strcat(status_vals[fldidx], " Ride");
            value = cond;
            break;
        default:
            value = atol(text);
            Sprintf(status_vals[fldidx],
                status_fieldfmt[fldidx] ? status_fieldfmt[fldidx] : "%s",
                text);
            break;
        }

#ifdef STATUS_HILITES
        switch (uwp_status_hilites[fldidx].behavior) {
        case BL_TH_NONE:
            uwp_status_colors[fldidx] = NO_COLOR;
            break;
        case BL_TH_UPDOWN:
            if (chg > 0)
                uwp_status_colors[fldidx] = uwp_status_hilites[fldidx].over;
            else if (chg < 0)
                uwp_status_colors[fldidx] = uwp_status_hilites[fldidx].under;
            else
                uwp_status_colors[fldidx] = NO_COLOR;
            break;
        case BL_TH_VAL_PERCENTAGE: {
            int pct_th = 0;

            if (uwp_status_hilites[fldidx].thresholdtype != ANY_INT) {
                impossible(
                    "uwp_status_update: unsupported percentage threshold type %d",
                    uwp_status_hilites[fldidx].thresholdtype);
            }
            else {
                pct_th = uwp_status_hilites[fldidx].threshold.a_int;
                uwp_status_colors[fldidx] = (percent >= pct_th)
                    ? uwp_status_hilites[fldidx].over
                    : uwp_status_hilites[fldidx].under;
            }
            break;
        }
        case BL_TH_VAL_ABSOLUTE: {
            int c = NO_COLOR;
            int o = uwp_status_hilites[fldidx].over;
            int u = uwp_status_hilites[fldidx].under;
            anything *t = &uwp_status_hilites[fldidx].threshold;

            switch (uwp_status_hilites[fldidx].thresholdtype) {
            case ANY_LONG:
                c = (value >= t->a_long) ? o : u;
                break;
            case ANY_INT:
                c = (value >= t->a_int) ? o : u;
                break;
            case ANY_UINT:
                c = ((unsigned long)value >= t->a_uint) ? o : u;
                break;
            case ANY_ULONG:
                c = ((unsigned long)value >= t->a_ulong) ? o : u;
                break;
            case ANY_MASK32:
                c = (value & t->a_ulong) ? o : u;
                break;
            default:
                impossible(
                    "uwp_status_update: unsupported absolute threshold type %d\n",
                    uwp_status_hilites[fldidx].thresholdtype);
                break;
            }
            uwp_status_colors[fldidx] = c;
            break;
        } /* case */
        } /* switch */
#endif /* STATUS_HILITES */
    }

    /* For now, this version copied from the genl_ version currently
    * updates everything on the display, everytime
    */

    if (!beenhere || !iflags.use_status_hilites) {
        char newbot1[MAXCO], newbot2[MAXCO];

        newbot1[0] = '\0';
        for (i = 0; fieldorder[0][i] >= 0; ++i) {
            int idx1 = fieldorder[0][i];

            if (status_activefields[idx1])
                Strcat(newbot1, status_vals[idx1]);
        }
        newbot2[0] = '\0';
        for (i = 0; fieldorder[1][i] >= 0; ++i) {
            int idx2 = fieldorder[1][i];

            if (status_activefields[idx2])
                Strcat(newbot2, status_vals[idx2]);
        }

        curs(WIN_STATUS, 1, 0);
        putstr(WIN_STATUS, 0, newbot1);
        curs(WIN_STATUS, 1, 1);
        putmixed(WIN_STATUS, 0, newbot2); /* putmixed() due to GOLD glyph */
        beenhere = TRUE;
        return;
    }

    curs(WIN_STATUS, 1, 0);
    for (i = 0; fieldorder[0][i] != BL_FLUSH; ++i) {
        int fldidx1 = fieldorder[0][i];

        if (status_activefields[fldidx1]) {
#ifdef STATUS_HILITES
            if (uwp_status_colors[fldidx1] < 0
                && uwp_status_colors[fldidx1] >= -3) {
                /* attribute, not a color */
                attridx = uwp_status_colors[fldidx1] + 3;
                uwp_term_start_attr(statusattr[attridx]);
                putstr(WIN_STATUS, 0, status_vals[fldidx1]);
                uwp_term_end_attr(statusattr[attridx]);
            }
            else
#ifdef TEXTCOLOR
                if (uwp_status_colors[fldidx1] != CLR_MAX) {
                    if (uwp_status_colors[fldidx1] != NO_COLOR)
                        uwp_term_start_color(uwp_status_colors[fldidx1]);
                    putstr(WIN_STATUS, 0, status_vals[fldidx1]);
                    if (uwp_status_colors[fldidx1] != NO_COLOR)
                        uwp_term_end_color();
                }
                else
#endif
#endif /* STATUS_HILITES */
                    putstr(WIN_STATUS, 0, status_vals[fldidx1]);
        }
    }
    curs(WIN_STATUS, 1, 1);
    for (i = 0; fieldorder[1][i] != BL_FLUSH; ++i) {
        int fldidx2 = fieldorder[1][i];

        if (status_activefields[fldidx2]) {
#ifdef STATUS_HILITES
            if (uwp_status_colors[fldidx2] < 0
                && uwp_status_colors[fldidx2] >= -3) {
                /* attribute, not a color */
                attridx = uwp_status_colors[fldidx2] + 3;
                uwp_term_start_attr(statusattr[attridx]);
                putstr(WIN_STATUS, 0, status_vals[fldidx2]);
                uwp_term_end_attr(statusattr[attridx]);
            }
            else
#ifdef TEXTCOLOR
                if (uwp_status_colors[fldidx2] != CLR_MAX) {
                    if (uwp_status_colors[fldidx2] != NO_COLOR)
                        uwp_term_start_color(uwp_status_colors[fldidx2]);
                    if (fldidx2 == BL_GOLD) {
                        /* putmixed() due to GOLD glyph */
                        putmixed(WIN_STATUS, 0, status_vals[fldidx2]);
                    }
                    else {
                        putstr(WIN_STATUS, 0, status_vals[fldidx2]);
                    }
                    if (uwp_status_colors[fldidx2] != NO_COLOR)
                        uwp_term_end_color();
                }
                else
#endif
#endif /* STATUS_HILITES */
                    putstr(WIN_STATUS, 0, status_vals[fldidx2]);
        }
    }
    return;
}

#ifdef STATUS_HILITES
/*
*  status_threshold(int fldidx, int threshholdtype, anything threshold,
*                   int behavior, int under, int over)
*
*        -- called when a hiliting preference is added, changed, or
*           removed.
*        -- the fldindex identifies which field is having its hiliting
*           preference set. It is an integer index value from botl.h
*        -- fldindex could be any one of the following from botl.h:
*           BL_TITLE, BL_STR, BL_DX, BL_CO, BL_IN, BL_WI, BL_CH,
*           BL_ALIGN, BL_SCORE, BL_CAP, BL_GOLD, BL_ENE, BL_ENEMAX,
*           BL_XP, BL_AC, BL_HD, BL_TIME, BL_HUNGER, BL_HP, BL_HPMAX,
*           BL_LEVELDESC, BL_EXP, BL_CONDITION
*        -- datatype is P_INT, P_UINT, P_LONG, or P_MASK.
*        -- threshold is an "anything" union which can contain the
*           datatype value.
*        -- behavior is used to define how threshold is used and can
*           be BL_TH_NONE, BL_TH_VAL_PERCENTAGE, BL_TH_VAL_ABSOLUTE,
*           or BL_TH_UPDOWN. BL_TH_NONE means don't do anything above
*           or below the threshold.  BL_TH_VAL_PERCENTAGE treats the
*           threshold value as a precentage of the maximum possible
*           value. BL_TH_VAL_ABSOLUTE means that the threshold is an
*           actual value. BL_TH_UPDOWN means that threshold is not
*           used, and the two below/above hilite values indicate how
*           to display something going down (under) or rising (over).
*        -- under is the hilite attribute used if value is below the
*           threshold. The attribute can be BL_HILITE_NONE,
*           BL_HILITE_INVERSE, BL_HILITE_BOLD (-1, -2, or -3), or one
*           of the color indexes of CLR_BLACK, CLR_RED, CLR_GREEN,
*           CLR_BROWN, CLR_BLUE, CLR_MAGENTA, CLR_CYAN, CLR_GRAY,
*           CLR_ORANGE, CLR_BRIGHT_GREEN, CLR_YELLOW, CLR_BRIGHT_BLUE,
*           CLR_BRIGHT_MAGENTA, CLR_BRIGHT_CYAN, or CLR_WHITE (0 - 15).
*        -- over is the hilite attribute used if value is at or above
*           the threshold. The attribute can be BL_HILITE_NONE,
*           BL_HILITE_INVERSE, BL_HILITE_BOLD (-1, -2, or -3), or one
*           of the color indexes of CLR_BLACK, CLR_RED, CLR_GREEN,
*           CLR_BROWN, CLR_BLUE, CLR_MAGENTA, CLR_CYAN, CLR_GRAY,
*           CLR_ORANGE, CLR_BRIGHT_GREEN, CLR_YELLOW, CLR_BRIGHT_BLUE,
*           CLR_BRIGHT_MAGENTA, CLR_BRIGHT_CYAN, or CLR_WHITE (0 - 15).
*/
void
uwp_status_threshold(fldidx, thresholdtype, threshold, behavior, under, over)
int fldidx, thresholdtype;
int behavior, under, over;
anything threshold;
{
    uwp_status_hilites[fldidx].thresholdtype = thresholdtype;
    uwp_status_hilites[fldidx].threshold = threshold;
    uwp_status_hilites[fldidx].behavior = behavior;
    uwp_status_hilites[fldidx].under = under;
    uwp_status_hilites[fldidx].over = over;
    return;
}

#endif /* STATUS_HILITES */
#endif /*STATUS_VIA_WINDOWPORT*/

void
uwp_nhbell()
{
    if (flags.silent)
        return;

    // TODO(bhouse): Figure out how we can play a beep sounds
    // Beep(8000, 500);
}

#endif // UWP_GRAPHICS

