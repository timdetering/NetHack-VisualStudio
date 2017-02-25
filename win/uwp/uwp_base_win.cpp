/* NetHack 3.6	uwp_base_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

BaseWindow g_baseWindow;

BaseWindow::BaseWindow() : CoreWindow(NHW_BASE, BASE_WINDOW)
{
    // core
    m_offx = 0;
    m_offy = 0;
    m_rows = kScreenHeight;
    m_cols = kScreenWidth;
}

BaseWindow::~BaseWindow()
{
}

void BaseWindow::Clear()
{
    clear_screen();

    CoreWindow::Clear();
}

void BaseWindow::Display(bool blocking)
{
    g_rawprint = 0;

    m_active = 1;
}

void BaseWindow::Dismiss()
{
    CoreWindow::Dismiss();
}

void BaseWindow::Putstr(int attr, const char *str)
{
    str = compress_str(str);
    TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);

    set_cursor(m_curx, m_cury);
    core_puts(str, TextColor::NoColor, useAttribute);
    m_curx = 0;
    m_cury++;
}

void BaseWindow::bail(const char *mesg)
{
    clearlocks();
    tty_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}

void BaseWindow::tty_askname()
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

    int startLine = m_cury;

    do {
        if (++tryct > 1) {
            if (tryct > 10)
                bail("Giving up after 10 tries.\n");
            set_cursor(0, startLine);
            core_puts("Enter a name for your character...");
        }

        set_cursor(0, startLine + 1);
        clear_to_end_of_line();

        core_puts(who_are_you);

        ct = 0;
        while ((c = tty_nhgetch()) != kNewline) {

            if (c == kEscape) {
                ct = 0;
                break;
            } /* continue outer loop */

            if (c == kControlC)
                bail("^C abort.\n");

            /* some people get confused when their erase char is not ^H */
            if (c == kBackspace || c == kDelete) {
                if (ct) {
                    ct--;
                    core_puts("\b \b");
                }
                continue;
            }

            if (ct < (int)(sizeof plname) - 1) {
                core_putc(c);
                plname[ct++] = c;
            }
        }
        plname[ct] = 0;
    } while (ct == 0);

    /* move to next line to simulate echo of user's <return> */
    set_cursor(0, m_cury + 1);

    /* since we let user pick an arbitrary name now, he/she can pick
    another one during role selection */
    iflags.renameallowed = TRUE;
}