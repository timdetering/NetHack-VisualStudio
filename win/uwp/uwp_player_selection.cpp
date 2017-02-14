/* NetHack 3.6	uwp_player_selection.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

extern "C" {

/* this player selection code was taken from wintty implementation.  this code is nearly independent
 * from the windowing implementation.  the plan is to make this code completely independent of the
 * windowing implementation.
 */

/* try to reduce clutter in the code below... */
#define ROLE flags.initrole
#define RACE flags.initrace
#define GEND flags.initgend
#define ALGN flags.initalign

/* add entries a-Archeologist, b-Barbarian, &c to menu being built in 'win' */
STATIC_OVL void
setup_rolemenu(
    winid win,
    boolean filtering,  /* True => exclude filtered roles; False => filter reset */
    int race, int  gend, int algn) /* all ROLE_NONE for !filtering case */
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
        /* !filtering implies reset_role_filtering() where we want to
        mark this role as preseleted if current filter excludes it */
        add_menu(win, NO_GLYPH, &any, thisch, 0, ATR_NONE, an(rolenamebuf),
            (!filtering && !role_ok) ? MENU_SELECTED : MENU_UNSELECTED);
        lastch = thisch;
    }
}

STATIC_OVL void
setup_racemenu(winid win, boolean filtering, int role, int gend, int algn)
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
setup_gendmenu(winid win, boolean filtering, int role, int race, int algn)
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
        /* (see setup_racemenu for explanation of selector letters
        and setup_rolemenu for preselection) */
        add_menu(win, NO_GLYPH, &any,
            filtering ? this_ch : highc(this_ch),
            filtering ? highc(this_ch) : 0,
            ATR_NONE, genders[i].adj,
            (!filtering && !gend_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

STATIC_DCL void
setup_algnmenu(winid win, boolean filtering, int role, int race, int gend)
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
        /* (see setup_racemenu for explanation of selector letters
        and setup_rolemenu for preselection) */
        add_menu(win, NO_GLYPH, &any,
            filtering ? this_ch : highc(this_ch),
            filtering ? highc(this_ch) : 0,
            ATR_NONE, aligns[i].adj,
            (!filtering && !algn_ok) ? MENU_SELECTED : MENU_UNSELECTED);
    }
}

STATIC_OVL boolean
reset_role_filtering()
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
    setup_rolemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
        "Unacceptable races", MENU_UNSELECTED);
    setup_racemenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
        "Unacceptable genders", MENU_UNSELECTED);
    setup_gendmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
        "Uncceptable alignments", MENU_UNSELECTED);
    setup_algnmenu(win, FALSE, ROLE_NONE, ROLE_NONE, ROLE_NONE);

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

void
tty_player_selection()
{
    int i, k, n, choice, nextpick;
    boolean getconfirmation, picksomething;
    char pick4u = 'n';
    char pbuf[QBUFSZ], plbuf[QBUFSZ];
    winid win;
    anything any = zeroany;
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
        tty_putstr(BASE_WINDOW, 0, "");
        echoline = g_wins[BASE_WINDOW]->m_cury;
        tty_putstr(BASE_WINDOW, 0, prompt);
        do {
            pick4u = lowc(readchar());
            if (index(quitchars, pick4u))
                pick4u = 'y';
        } while (!index(ynaqchars, pick4u));
        if ((int)strlen(prompt) + 1 < CO) {
            /* Echo choice and move back down line */
            tty_putsym(BASE_WINDOW, (int)strlen(prompt) + 1, echoline,
                pick4u);
            tty_putstr(BASE_WINDOW, 0, "");
        }
        else
            /* Otherwise it's hard to tell where to echo, and things are
            * wrapping a bit messily anyway, so (try to) make sure the next
            * question shows up well and doesn't get wrapped at the
            * bottom of the window.
            */
            tty_clear_nhwindow(BASE_WINDOW);

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
                        tty_putstr(BASE_WINDOW, 0, "Incompatible role!");
                        k = randrole();
                    }
                }
                else {
                    /* Prompt for a role */
                    tty_clear_nhwindow(BASE_WINDOW);
                    role_selection_prolog(RS_ROLE, BASE_WINDOW);
                    win = create_nhwindow(NHW_MENU);
                    start_menu(win);
                    /* populate the menu with role choices */
                    setup_rolemenu(win, TRUE, RACE, GEND, ALGN);
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
                        (void)reset_role_filtering();
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
                        tty_putstr(BASE_WINDOW, 0, "Incompatible race!");
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
                        tty_clear_nhwindow(BASE_WINDOW);
                        role_selection_prolog(RS_RACE, BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                                       /* populate the menu with role choices */
                        setup_racemenu(win, TRUE, ROLE, GEND, ALGN);
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
                            if (reset_role_filtering())
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
                        tty_putstr(BASE_WINDOW, 0, "Incompatible gender!");
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
                        tty_clear_nhwindow(BASE_WINDOW);
                        role_selection_prolog(RS_GENDER, BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                                       /* populate the menu with gender choices */
                        setup_gendmenu(win, TRUE, ROLE, RACE, ALGN);
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
                            if (reset_role_filtering())
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
                        tty_putstr(BASE_WINDOW, 0, "Incompatible alignment!");
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
                        tty_clear_nhwindow(BASE_WINDOW);
                        role_selection_prolog(RS_ALGNMNT, BASE_WINDOW);
                        win = create_nhwindow(NHW_MENU);
                        start_menu(win);
                        any = zeroany; /* zero out all bits */
                        setup_algnmenu(win, TRUE, ROLE, RACE, GEND);
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
                            if (reset_role_filtering())
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
        tty_clear_nhwindow(BASE_WINDOW);
        role_selection_prolog(ROLE_NONE, BASE_WINDOW);
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
        case 3:
        { /* 'a' */
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
    tty_display_nhwindow(BASE_WINDOW, FALSE);
    return;

give_up:
    /* Quit */
    if (selected)
        free((genericptr_t)selected); /* [obsolete] */

    /* should we terminate or our caller? */
    if (iflags.window_inited)
        exit_nhwindows((char *)0);
    clearlocks();
    terminate(EXIT_SUCCESS);

    /*NOTREACHED*/
}

} /* extern "C" */