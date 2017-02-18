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
    MenuWindow *menuWin = ToMenuWindow(GetCoreWindow(window));

    if (str == (const char *)0)
        return;

    if (menuWin == NULL)
        panic(winpanicstr, window);

    return menuWin->tty_add_menu(identifier, ch, gch, attr, str, preselected);
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
    MenuWindow *menuWin = ToMenuWindow(GetCoreWindow(window));

    if (menuWin == NULL)
        panic(winpanicstr, window);

    menuWin->tty_end_menu(prompt);
}

int
tty_select_menu(
    winid window,
    int how,
    menu_item **menu_list)
{
    MenuWindow *menuWin = ToMenuWindow(GetCoreWindow(window));

    if (menuWin == NULL)
        panic(winpanicstr, window);

    return menuWin->tty_select_menu(how, menu_list);
}

char tty_message_menu(char let, int how, const char *mesg)
{
    return GetMessageWindow()->tty_message_menu(let, how, mesg);
}

} /* extern "C" */