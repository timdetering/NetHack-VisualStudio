/* NetHack 3.6	uwp_menu.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

extern "C" {

extern char winpanicstr[];

void
dmore(
    CoreWindow *coreWin,
    const char *s) /* valid responses */
{
    const char *prompt = coreWin->m_morestr ? coreWin->m_morestr : defmorestr;
    int offset = (coreWin->m_type == NHW_TEXT) ? 1 : 2;

    tty_curs(BASE_WINDOW, (int)coreWin->m_curx + coreWin->m_offx + offset, (int)coreWin->m_cury + coreWin->m_offy);

    win_puts(BASE_WINDOW, prompt, TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);

    xwaitforspace(s);
}

void
tty_start_menu(winid window)
{
    tty_clear_nhwindow(window);
    return;
}

/*ARGSUSED*/
/*
* Add a menu item to the beginning of the menu list.  This list is reversed
* later.
*/
void
tty_add_menu(
    winid window,               /* window to use, must be of type NHW_MENU */
    int glyph UNUSED,           /* glyph to display with item (not used) */
    const anything *identifier, /* what to return if selected */
    char ch,                    /* keyboard accelerator (0 = pick our own) */
    char gch,                   /* group accelerator (0 = no group) */
    int attr,                   /* attribute for string (like tty_putstr()) */
    const char *str,            /* menu string */
    boolean preselected)        /* item is marked as selected */
{
    CoreWindow *coreWin = GetCoreWindow(window);
    MenuWindow *menuWin = ToMenuWindow(coreWin);
    tty_menu_item *item;
    const char *newstr;
    char buf[4 + BUFSZ];

    if (str == (const char *)0)
        return;

    if (coreWin->m_type != NHW_MENU)
        panic(winpanicstr, window);

    menuWin->m_nitems++;
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

    item = (tty_menu_item *)alloc(sizeof(tty_menu_item));
    item->identifier = *identifier;
    item->count = -1L;
    item->selected = preselected;
    item->selector = ch;
    item->gselector = gch;
    item->attr = attr;
    item->str = dupstr(newstr ? newstr : "");

    item->next = menuWin->m_mlist;
    menuWin->m_mlist = item;
}

/* Invert the given list, can handle NULL as an input. */
STATIC_OVL tty_menu_item *
reverse(tty_menu_item *curr)
{
    tty_menu_item *next, *head = 0;

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
tty_end_menu(
    winid window,       /* menu to use */
    const char *prompt) /* prompt to for menu */
{
    CoreWindow *coreWin = GetCoreWindow(window);
    MenuWindow *menuWin = ToMenuWindow(coreWin);
    tty_menu_item *curr;
    short len;
    int lmax, n;
    char menu_ch;

    if (coreWin->m_type != NHW_MENU)
        panic(winpanicstr, window);

    /* Reverse the list so that items are in correct order. */
    menuWin->m_mlist = reverse(menuWin->m_mlist);

    /* Put the prompt at the beginning of the menu. */
    if (prompt) {
        anything any;

        any = zeroany; /* not selectable */
        tty_add_menu(window, NO_GLYPH, &any, 0, 0, ATR_NONE, "",
            MENU_UNSELECTED);
        tty_add_menu(window, NO_GLYPH, &any, 0, 0, ATR_NONE, prompt,
            MENU_UNSELECTED);
    }

    /* XXX another magic number? 52 */
    lmax = min(52, (int)g_uwpDisplay->rows - 1);    /* # lines per page */
    menuWin->m_npages = (menuWin->m_nitems + (lmax - 1)) / lmax; /* # of pages */

                                                   /* make sure page list is large enough */
    if (menuWin->m_plist_size < menuWin->m_npages + 1 /*need 1 slot beyond last*/) {
        if (menuWin->m_plist)
            free((genericptr_t)menuWin->m_plist);
        menuWin->m_plist_size = menuWin->m_npages + 1;
        menuWin->m_plist = (tty_menu_item **)alloc(menuWin->m_plist_size
            * sizeof(tty_menu_item *));
    }

    coreWin->m_cols = 0;  /* cols is set when the win is initialized... (why?) */
    menu_ch = '?'; /* lint suppression */
    for (n = 0, curr = menuWin->m_mlist; curr; n++, curr = curr->next) {
        /* set page boundaries and character accelerators */
        if ((n % lmax) == 0) {
            menu_ch = 'a';
            menuWin->m_plist[n / lmax] = curr;
        }
        if (curr->identifier.a_void && !curr->selector) {
            curr->str[0] = curr->selector = menu_ch;
            if (menu_ch++ == 'z')
                menu_ch = 'A';
        }

        /* cut off any lines that are too long */
        len = strlen(curr->str) + 2; /* extra space at beg & end */
        if (len > (int)g_uwpDisplay->cols) {
            curr->str[g_uwpDisplay->cols - 2] = 0;
            len = g_uwpDisplay->cols;
        }
        if (len > coreWin->m_cols)
            coreWin->m_cols = len;
    }
    menuWin->m_plist[menuWin->m_npages] = 0; /* plist terminator */

                               /*
                               * If greater than 1 page, morestr is "(x of y) " otherwise, "(end) "
                               */
    if (menuWin->m_npages > 1) {
        char buf[QBUFSZ];
        /* produce the largest demo string */
        Sprintf(buf, "(%ld of %ld) ", menuWin->m_npages, menuWin->m_npages);
        len = strlen(buf);
        coreWin->m_morestr = dupstr("");
    }
    else {
        coreWin->m_morestr = dupstr("(end) ");
        len = strlen(coreWin->m_morestr);
    }

    if (len > (int)g_uwpDisplay->cols) {
        /* truncate the prompt if it's too long for the screen */
        if (menuWin->m_npages <= 1) /* only str in single page case */
            coreWin->m_morestr[g_uwpDisplay->cols] = 0;
        len = g_uwpDisplay->cols;
    }
    if (len > coreWin->m_cols)
        coreWin->m_cols = len;

    /*
    * The number of lines in the first page plus the morestr will be the
    * maximum size of the window.
    */
    if (menuWin->m_npages > 1)
        coreWin->m_rows = lmax + 1;
    else
        coreWin->m_rows = menuWin->m_nitems + 1;
}

int
tty_select_menu(
    winid window,
    int how,
    menu_item **menu_list)
{
    CoreWindow *coreWin = GetCoreWindow(window);
    MenuWindow *menuWin = ToMenuWindow(coreWin);
    tty_menu_item *curr;
    menu_item *mi;
    int n, cancelled;

    if (coreWin->m_type != NHW_MENU)
        panic(winpanicstr, window);

    *menu_list = (menu_item *)0;
    menuWin->m_how = (short)how;
    morc = 0;
    tty_display_nhwindow(window, TRUE);
    cancelled = !!(coreWin->m_flags & WIN_CANCELLED);
    tty_dismiss_nhwindow(window); /* does not destroy window data */

    if (cancelled) {
        n = -1;
    }
    else {
        for (n = 0, curr = menuWin->m_mlist; curr; curr = curr->next)
            if (curr->selected)
                n++;
    }

    if (n > 0) {
        *menu_list = (menu_item *)alloc(n * sizeof(menu_item));
        for (mi = *menu_list, curr = menuWin->m_mlist; curr; curr = curr->next)
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
tty_message_menu(
    char let,
    int how,
    const char *mesg)
{
    MessageWindow *coreWin = GetMessageWindow();

    assert(coreWin != NULL);

    /* "menu" without selection; use ordinary pline, no more() */
    if (how == PICK_NONE) {
        pline("%s", mesg);
        return 0;
    }

    g_uwpDisplay->dismiss_more = let;
    morc = 0;
    /* barebones pline(); since we're only supposed to be called after
    response to a prompt, we'll assume that the display is up to date */
    tty_putstr(WIN_MESSAGE, 0, mesg);
    /* if `mesg' didn't wrap (triggering --More--), force --More-- now */

    if (coreWin->m_mustBeSeen) {
        more();
        assert(!coreWin->m_mustBeSeen);

        if (coreWin->m_mustBeErased)
            tty_clear_nhwindow(WIN_MESSAGE);
    }
    /* normally <ESC> means skip further messages, but in this case
    it means cancel the current prompt; any other messages should
    continue to be output normally */
    g_wins[WIN_MESSAGE]->m_flags &= ~WIN_CANCELLED;
    g_uwpDisplay->dismiss_more = 0;

    return ((how == PICK_ONE && morc == let) || morc == '\033') ? morc : '\0';
}

} /* extern "C" */