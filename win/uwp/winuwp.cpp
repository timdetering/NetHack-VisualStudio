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
CoreWindow *g_wins[MAXWIN];
struct DisplayDesc *g_uwpDisplay; /* the tty display descriptor */

char winpanicstr[] = "Bad window id %d";
char defmorestr[] = "--More--";

#if defined(USE_TILES) && defined(MSDOS)
extern void FDECL(adjust_cursor_flags, (CoreWindow *));
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
STATIC_DCL tty_menu_item *FDECL(reverse, (tty_menu_item *));
STATIC_DCL void FDECL(setup_rolemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_racemenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_gendmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL void FDECL(setup_algnmenu, (winid, BOOLEAN_P, int, int, int));
STATIC_DCL boolean NDECL(reset_role_filtering);

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
    g_uwpDisplay->rows = hgt;
    g_uwpDisplay->cols = wid;
    g_uwpDisplay->dismiss_more = 0;

    /* set up the default windows */
    winid base_window = tty_create_nhwindow(NHW_BASE);
    assert(base_window == BASE_WINDOW);

    g_wins[BASE_WINDOW]->m_active = 1;

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
    BaseWindow * baseWin = (BaseWindow *) g_wins[BASE_WINDOW];
    baseWin->tty_askname();
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
            g_wins[i]->free_window_info(TRUE);
            delete g_wins[i];
#endif
            g_wins[i] = NULL;
        }
    }
    WIN_MAP = WIN_MESSAGE = WIN_INVEN = WIN_ERR; /* these are all gone now */
#ifndef STATUS_VIA_WINDOWPORT
    WIN_STATUS = WIN_ERR;
#endif
#ifdef FREE_ALL_MEMORY
    if (BASE_WINDOW != WIN_ERR && g_wins[BASE_WINDOW]) {
        g_wins[BASE_WINDOW]->free_window_info(TRUE);
        delete g_wins[BASE_WINDOW];
        g_wins[BASE_WINDOW] = NULL;
    }
    free((genericptr_t) g_uwpDisplay);
    g_uwpDisplay = (struct DisplayDesc *) 0;
#endif

    iflags.window_inited = 0;
}

winid
tty_create_nhwindow(int type)
{
    int i;
    int newid;

    if (maxwin == MAXWIN)
        return WIN_ERR;

    CoreWindow *coreWin = NULL;

    switch (type) {
    case NHW_BASE:
    {
        if (g_wins[BASE_WINDOW] != NULL) return WIN_ERR;

        BaseWindow * baseWin = new BaseWindow();
        coreWin = baseWin;

        /* base window, used for absolute movement on the screen */
        break;
    }

    case NHW_MESSAGE:
    {
        MessageWindow * msgWin = new MessageWindow();
        coreWin = msgWin;

        break;
    }
    case NHW_STATUS:
    {
        StatusWindow * statusWin = new StatusWindow();
        coreWin = statusWin;
        break;
    }
    case NHW_MAP:
    {
        MapWindow * mapWin = new MapWindow();
        coreWin = mapWin;

        break;
    }
    case NHW_MENU:
    {
        MenuWindow * menuWin = new MenuWindow();
        coreWin = menuWin;

        break;
    }
    case NHW_TEXT:
    {
        TextWindow * textWin = new TextWindow();
        coreWin = textWin;
        break;
    }
    default:
        panic("Tried to create window type %d\n", (int) type);
        return WIN_ERR;
    }

    for (newid = 0; newid < MAXWIN; newid++) {
        if (g_wins[newid] == 0) {
            g_wins[newid] = coreWin;
            break;
        }
    }

    if (newid == MAXWIN) {
        panic("No window slots!");
        return WIN_ERR;
    }

    coreWin->m_window = newid;

    return newid;
}

CoreWindow * GetCoreWindow(winid window)
{
    if (window == WIN_ERR || g_wins[window] == NULL)
        panic(winpanicstr, window);

    return g_wins[window];
}

MessageWindow * GetMessageWindow()
{
    if (WIN_MESSAGE != WIN_ERR)
        return (MessageWindow *)g_wins[WIN_MESSAGE];
    return NULL;
}

MessageWindow * ToMessageWindow(CoreWindow * coreWin)
{
    if (coreWin->m_type == NHW_MESSAGE)
        return (MessageWindow *)coreWin;
    return NULL;
}

MenuWindow * ToMenuWindow(CoreWindow * coreWin)
{
    if (coreWin->m_type == NHW_MENU)
        return (MenuWindow *)coreWin;
    return NULL;
}

void
tty_clear_nhwindow(winid window)
{
    GetCoreWindow(window)->Clear();
}

/*ARGSUSED*/
void
tty_display_nhwindow(winid window, boolean blocking)
{
    CoreWindow *coreWin = GetCoreWindow(window);

    if (coreWin->m_flags & WIN_CANCELLED)
        return;

    g_uwpDisplay->rawprint = 0;

    coreWin->Display(blocking != 0);
}


void
tty_dismiss_nhwindow(winid window)
{
    GetCoreWindow(window)->Dismiss();
}



void
tty_destroy_nhwindow(winid window)
{
    CoreWindow *coreWin = GetCoreWindow(window);

    coreWin->Destroy();
    delete coreWin;

    g_wins[window] = 0;
}


void
tty_curs(winid window, int x, int y)
{
    CoreWindow *coreWin = GetCoreWindow(window);

    coreWin->Curs(x, y);
}

void
tty_putsym(winid window, int x, int y, char ch)
{
    CoreWindow *coreWin = GetCoreWindow(window);

    switch (coreWin->m_type) {
    case NHW_STATUS:
    case NHW_MAP:
    case NHW_BASE:
        tty_curs(window, x, y);
        coreWin->core_putc(ch);
        break;
    case NHW_MESSAGE:
    case NHW_MENU:
    case NHW_TEXT:
        impossible("Can't putsym to window type %d", coreWin->m_type);
        break;
    }
}

const char *
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
    /* Assume there's a real problem if the window is missing --
     * probably a panic message
     */
    if (window == WIN_ERR || g_wins[window] == NULL) {
        tty_raw_print(str);
        return;
    }

    CoreWindow *coreWin = g_wins[window];

    if (str == NULL || ((coreWin->m_flags & WIN_CANCELLED) && (coreWin->m_type != NHW_MESSAGE)))
        return;

    coreWin->Putstr(attr, str);
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
            g_wins[datawin]->m_offy = g_wins[WIN_STATUS]->m_offy + 3;
            if ((int)g_wins[datawin]->m_offy + 12 > g_textGrid.GetDimensions().m_y)
                    g_wins[datawin]->m_offy = 0;
        }
        while (dlb_fgets(buf, BUFSZ, f)) {
            if ((cr = index(buf, '\n')) != 0)
                *cr = 0;
            if (index(buf, '\t') != 0)
                (void) tabexpand(buf);
            empty = FALSE;
            tty_putstr(datawin, 0, buf);
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
    }
}

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

    /* map glyph to character and color */
    (void) mapglyph(glyph, &ch, &color, &special, x, y);

    CoreWindow * coreWin = GetCoreWindow(window);

    /* Move the cursor. */
    coreWin->Curs(x, y);

    /* must be after color check; term_end_color may turn off inverse too */
    if (((special & MG_PET) && iflags.hilite_pet)
        || ((special & MG_OBJPILE) && iflags.hilite_pile)
        || ((special & MG_DETECT) && iflags.use_inverse)
        || ((special & MG_BW_LAVA) && iflags.use_inverse)) {
        reverse_on = TRUE;
    }

    coreWin->core_putc(ch, (TextColor)color, reverse_on ? TextAttribute::Inverse : TextAttribute::None);
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

int
tgetch()
{
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

    MessageWindow *msgWin = NULL;
    
    if (WIN_MESSAGE != WIN_ERR)
        msgWin = (MessageWindow *) g_wins[WIN_MESSAGE];

    /* Note: if raw_print() and wait_synch() get called to report terminal
     * initialization problems, then g_wins[] and g_uwpDisplay might not be
     * available yet.  Such problems will probably be fatal before we get
     * here, but validate those pointers just in case...
     */
    if (msgWin != NULL)
        msgWin->m_flags &= ~WIN_STOP;

    i = tgetch();
    if (!i)
        i = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    else if (i == EOF)
        i = '\033'; /* same for EOF */

    if (msgWin != NULL)
        msgWin->m_mustBeSeen = false;

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

    MessageWindow *msgWin = NULL;

    if (WIN_MESSAGE != WIN_ERR)
        msgWin = (MessageWindow *) g_wins[WIN_MESSAGE];

    /* Note: if raw_print() and wait_synch() get called to report terminal
     * initialization problems, then g_wins[] and g_uwpDisplay might not be
     * available yet.  Such problems will probably be fatal before we get
     * here, but validate those pointers just in case...
     */
    if (msgWin != NULL)
        msgWin->m_flags &= ~WIN_STOP;

    i = ntposkey(x, y, mod);
    if (!i && mod && (*mod == 0 || *mod == EOF))
        i = '\033'; /* map NUL or EOF to ESC, nethack doesn't expect either */

    if (msgWin != NULL)
        msgWin->m_mustBeSeen = false;

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
    GetMessageWindow()->hooked_tty_getlin(query, bufp, (getlin_hook_proc)0);
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
    GetMessageWindow()->hooked_tty_getlin("#", buf, in_doagain ? (getlin_hook_proc)0
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

STATIC_DCL void FDECL(msghistory_snapshot, (BOOLEAN_P));

int
tty_doprev_message()
{
    return GetMessageWindow()->doprev_message();
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
    return GetMessageWindow()->yn_function(query, resp, def);
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
    return GetMessageWindow()->uwp_getmsghistory(init);
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
    GetMessageWindow()->uwp_putmsghistory(msg, restoring_msghist);
}

void tty_number_pad(int state)
{
    // do nothing for now
}

void
tty_start_screen()
{
    // do nothing for now
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
    if (program_state.done_hup)
        return '\033';

    /* we checking for input -- flush our output */
    g_textGrid.Flush();

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
        g_wins[BASE_WINDOW]->core_puts(buf, TextColor::NoColor, TextAttribute::None);
    else
        g_textGrid.Putstr(TextColor::NoColor, TextAttribute::None, buf);

    if (g_uwpDisplay)
        g_wins[BASE_WINDOW]->Curs(g_textGrid.GetCursor().m_x + 1, g_textGrid.GetCursor().m_y);

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
        g_wins[BASE_WINDOW]->core_puts(buf, TextColor::NoColor, TextAttribute::Bold);
    else
        g_textGrid.Putstr(TextColor::NoColor, TextAttribute::Bold, buf);

    if (g_uwpDisplay)
        g_wins[BASE_WINDOW]->Curs(g_textGrid.GetCursor().m_x + 1, g_textGrid.GetCursor().m_y);

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

void uwp_puts(const char *s)
{
    assert(g_wins[BASE_WINDOW] != NULL);
    g_wins[BASE_WINDOW]->core_puts(s, TextColor::NoColor, TextAttribute::None);
}

void win_putc(
    winid window,
    char ch,
    Nethack::TextColor textColor,
    Nethack::TextAttribute textAttribute)
{
    assert(g_wins[window] != NULL);
    g_wins[window]->core_putc(ch, textColor, textAttribute);
}

void win_puts(
    winid window,
    const char *s,
    TextColor textColor,
    TextAttribute textAttribute)
{
    assert(g_wins[window] != NULL);
    g_wins[window]->core_puts(s, textColor, textAttribute);
}

} /* extern "C" */
