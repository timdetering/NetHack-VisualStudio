/* NetHack 3.6	uwp_menu_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

/*
* A string containing all the default commands -- to add to a list
* of acceptable inputs.
*/
static const char default_menu_cmds[] = {
    MENU_FIRST_PAGE,    MENU_LAST_PAGE,   MENU_NEXT_PAGE,
    MENU_PREVIOUS_PAGE, MENU_SELECT_ALL,  MENU_UNSELECT_ALL,
    MENU_INVERT_ALL,    MENU_SELECT_PAGE, MENU_UNSELECT_PAGE,
    MENU_INVERT_PAGE,   MENU_SEARCH,      0 /* null terminator */
};

extern "C" {
    extern char mapped_menu_cmds[]; /* from options.c */
}

using namespace Nethack;

MenuWindow::MenuWindow() : CoreWindow(NHW_MENU)
{
    // menu
    m_mlist = (tty_menu_item *)0;
    m_plist = (tty_menu_item **)0;
    m_npages = 0;
    m_plist_size = 0;
    m_nitems = 0;
    m_how = 0;

    // core
    /* inventory/menu window, variable length, full width, top of screen
    */
    /* help window, the same, different semantics for display, etc */
    m_offx = 0;
    m_offy = 0;
    m_rows = 0;
    m_cols = 0;

}

MenuWindow::~MenuWindow()
{
}

void MenuWindow::Display(bool blocking)
{
    m_active = 1;

    m_offx = m_cols < g_uwpDisplay->cols ? g_uwpDisplay->cols - m_cols - 1 : 0;
    m_offx = min(g_uwpDisplay->cols / 2, m_offx);

    m_offy = 0;

    MessageWindow * msgWin = GetMessageWindow();

    if (msgWin != NULL && msgWin->m_mustBeSeen)
        tty_display_nhwindow(WIN_MESSAGE, TRUE);

    /* if we are going to have more then one page to display the use full screen */
    /* or if we are not supposed to use menu overlays */;
    if (m_rows >= (int)g_uwpDisplay->rows || !iflags.menu_overlay)
    {
        m_offx = 0;
        /* TODO(bhouse) why do we test and do something different for overlay? */
        if (iflags.menu_overlay) {
            tty_curs(m_window, 1, 0);
            cl_eos();
        } else
            clear_screen();

        /* we just cleared the message area so we no longer need to erase */
        if (msgWin != NULL)
            msgWin->m_mustBeErased = false;
    } else {
        /* TODO(bhouse) why do we have this complexity ... why not just
        * always clear?
        */
        if (msgWin != NULL)
            tty_clear_nhwindow(WIN_MESSAGE);
    }

    if (m_lines.size() > 0) {
        process_lines();
    } else
        process_menu();

    assert(m_active);
}


void MenuWindow::Dismiss()
{
    if (m_active) {
        if (iflags.window_inited) {
            /* otherwise dismissing the text endwin after other windows
            * are dismissed tries to redraw the map and panics.  since
            * the whole reason for dismissing the other windows was to
            * leave the ending window on the screen, we don't want to
            * erase it anyway.
            */
            docrt();
        }
        m_active = 0;
    }
    m_flags = 0;
}

void MenuWindow::Putstr(int attr, const char *str)
{
    std::string input = std::string(compress_str(str));

    do {
        std::string line;

        if (input.size() > CO) {
            int split = input.find_last_of(" \n", CO - 1);
            if (split == std::string::npos || split == 0)
                split = CO - 1;

            line = input.substr(0, split);
            input = input.substr(split + 1);
        } else {
            line = input;
            input.clear();
        }

        m_lines.push_back(std::pair<int, std::string>(attr, line));

        /* TODO(bhouse) do we need either of these to be set really? */
        m_cury = m_lines.size();
        m_rows = m_lines.size();

        if (line.size() > m_cols)
            m_cols = line.size();
    } while (input.size() > 0);
}

void MenuWindow::free_window_info(
    boolean free_data)
{
    if (m_mlist) {
        tty_menu_item *temp;

        while ((temp = m_mlist) != 0) {
            m_mlist = m_mlist->next;
            if (temp->str)
                free((genericptr_t)temp->str);
            free((genericptr_t)temp);
        }
    }
    if (m_plist) {
        free((genericptr_t)m_plist);
        m_plist = 0;
    }

    m_plist_size = 0;
    m_npages = 0;
    m_nitems = 0;
    m_how = 0;

    CoreWindow::free_window_info(free_data);
}

void MenuWindow::Clear()
{
    if (m_active) {
        clear_screen();
    }

    free_window_info(FALSE);

    CoreWindow::Clear();
}


void MenuWindow::process_menu()
{
    tty_menu_item *page_start, *page_end, *curr;
    long count;
    int n, attr_n, curr_page, page_lines, resp_len;
    boolean finished, counting, reset_count;
    char *cp, *rp, resp[QBUFSZ], gacc[QBUFSZ], *msave, *morestr, really_morc;
#define MENU_EXPLICIT_CHOICE 0x7f /* pseudo menu manipulation char */

    curr_page = page_lines = 0;
    page_start = page_end = 0;
    msave = m_morestr; /* save the morestr */
    m_morestr = morestr = (char *)alloc((unsigned)QBUFSZ);
    counting = FALSE;
    count = 0L;
    reset_count = TRUE;
    finished = FALSE;

    /* collect group accelerators; for PICK_NONE, they're ignored;
    for PICK_ONE, only those which match exactly one entry will be
    accepted; for PICK_ANY, those which match any entry are okay */
    gacc[0] = '\0';
    if (m_how != PICK_NONE) {
        int i, gcnt[128];
#define GSELIDX(c) (c & 127) /* guard against `signed char' */

        for (i = 0; i < SIZE(gcnt); i++)
            gcnt[i] = 0;
        for (n = 0, curr = m_mlist; curr; curr = curr->next)
            if (curr->gselector && curr->gselector != curr->selector) {
                ++n;
                ++gcnt[GSELIDX(curr->gselector)];
            }

        if (n > 0) /* at least one group accelerator found */
            for (rp = gacc, curr = m_mlist; curr; curr = curr->next)
                if (curr->gselector && curr->gselector != curr->selector
                    && !index(gacc, curr->gselector)
                    && (m_how == PICK_ANY
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
        } else
            reset_count = TRUE;

        if (!page_start) {
            /* new page to be displayed */
            if (curr_page < 0 || (m_npages > 0 && curr_page >= m_npages))
                panic("bad menu screen page #%d", curr_page);

            /* clear screen */
            if (!m_offx) { /* if not corner, do clearscreen */
                if (m_offy) {
                    tty_curs(m_window, 1, 0);
                    cl_eos();
                } else
                    clear_screen();
            }

            rp = resp;
            if (m_npages > 0) {
                /* collect accelerators */
                page_start = m_plist[curr_page];
                page_end = m_plist[curr_page + 1];
                for (page_lines = 0, curr = page_start; curr != page_end;
                    page_lines++, curr = curr->next) {
                    int attr, color = NO_COLOR;

                    if (curr->selector)
                        *rp++ = curr->selector;

                    tty_curs(m_window, 1, page_lines);
                    if (m_offx)
                        cl_end();

                    TextColor useColor = TextColor::NoColor;
                    TextAttribute useAttribute = TextAttribute::None;

                    win_putc(m_window, ' ', useColor, useAttribute);

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
                                    * Don't use xputs() because (1) under unix it calls
                                    * tputstr() which will interpret a '*' as some kind
                                    * of padding information and (2) it calls xputc to
                                    * actually output the character.  We're faster doing
                                    * this.
                                    */
                    for (n = 0, cp = curr->str;
                        *cp &&
                        g_textGrid.GetCursor().m_x < g_textGrid.GetDimensions().m_x;
                        cp++, n++) {

                        if (n == attr_n && (color != NO_COLOR
                            || attr != ATR_NONE)) {
                            useColor = (TextColor)color;
                            useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);
                        }

                        if (n == 2
                            && curr->identifier.a_void != 0
                            && curr->selected) {
                            if (curr->count == -1L)
                                win_putc(m_window, '+', useColor, useAttribute);
                            else
                                win_putc(m_window, '#', useColor, useAttribute);
                        } else
                            win_putc(m_window, *cp, useColor, useAttribute);
                    } /* for *cp */
                } /* if npages > 0 */
            } else {
                page_start = 0;
                page_end = 0;
                page_lines = 0;
            }
            *rp = 0;
            /* remember how many explicit menu choices there are */
            resp_len = (int)strlen(resp);

            /* corner window - clear extra lines from last page */
            if (m_offx) {
                for (n = page_lines + 1; n < m_rows; n++) {
                    tty_curs(m_window, 1, n);
                    cl_end();
                }
            }

            /* set extra chars.. */
            Strcat(resp, default_menu_cmds);
            Strcat(resp, " ");                  /* next page or end */
            Strcat(resp, "0123456789\033\n\r"); /* counts, quit */
            Strcat(resp, gacc);                 /* group accelerators */
            Strcat(resp, mapped_menu_cmds);

            if (m_npages > 1)
                Sprintf(m_morestr, "(%d of %d)", curr_page + 1,
                (int)m_npages);
            else if (msave)
                Strcpy(m_morestr, msave);
            else
                Strcpy(m_morestr, defmorestr);

            tty_curs(m_window, 1, page_lines);
            cl_end();
            dmore(this, resp);
        } else {
            /* just put the cursor back... */
            tty_curs(m_window, (int)strlen(m_morestr) + 2, page_lines);
            xwaitforspace(resp);
        }

        really_morc = morc; /* (only used with MENU_EXPLICIT_CHOICE */
        if ((rp = index(resp, morc)) != 0 && rp < resp + resp_len)
            /* explicit menu selection; don't override it if it also
            happens to match a mapped menu command (such as ':' to
            look inside a container vs ':' to search) */
            morc = MENU_EXPLICIT_CHOICE;
        else
            morc = map_menu_cmd(morc);

        switch (morc) {
        case '0':
            /* special case: '0' is also the default ball class */
            if (!counting && index(gacc, morc))
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
            count = (count * 10L) + (long)(morc - '0');
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
                for (curr = m_mlist; curr; curr = curr->next) {
                    curr->selected = FALSE;
                    curr->count = -1L;
                }
                m_flags |= WIN_CANCELLED;
                finished = TRUE;
            }
            /* else only stop count */
            break;
        case '\0': /* finished (commit) */
        case '\n':
        case '\r':
            /* only finished if we are actually picking something */
            if (m_how != PICK_NONE) {
                finished = TRUE;
                break;
            }
            /* else fall through */
        case ' ':
        case MENU_NEXT_PAGE:
            if (m_npages > 0 && curr_page != m_npages - 1) {
                curr_page++;
                page_start = 0;
            } else if (morc == ' ') {
                /* ' ' finishes menus here, but stop '>' doing the same. */
                finished = TRUE;
            }
            break;
        case MENU_PREVIOUS_PAGE:
            if (m_npages > 0 && curr_page != 0) {
                --curr_page;
                page_start = 0;
            }
            break;
        case MENU_FIRST_PAGE:
            if (m_npages > 0 && curr_page != 0) {
                page_start = 0;
                curr_page = 0;
            }
            break;
        case MENU_LAST_PAGE:
            if (m_npages > 0 && curr_page != m_npages - 1) {
                page_start = 0;
                curr_page = m_npages - 1;
            }
            break;
        case MENU_SELECT_PAGE:
            if (m_how == PICK_ANY)
                set_all_on_page(m_window, page_start, page_end);
            break;
        case MENU_UNSELECT_PAGE:
            unset_all_on_page(m_window, page_start, page_end);
            break;
        case MENU_INVERT_PAGE:
            if (m_how == PICK_ANY)
                invert_all_on_page(m_window, page_start, page_end, 0);
            break;
        case MENU_SELECT_ALL:
            if (m_how == PICK_ANY) {
                set_all_on_page(m_window, page_start, page_end);
                /* set the rest */
                for (curr = m_mlist; curr; curr = curr->next)
                    if (curr->identifier.a_void && !curr->selected)
                        curr->selected = TRUE;
            }
            break;
        case MENU_UNSELECT_ALL:
            unset_all_on_page(m_window, page_start, page_end);
            /* unset the rest */
            for (curr = m_mlist; curr; curr = curr->next)
                if (curr->identifier.a_void && curr->selected) {
                    curr->selected = FALSE;
                    curr->count = -1;
                }
            break;
        case MENU_INVERT_ALL:
            if (m_how == PICK_ANY)
                invert_all(m_window, page_start, page_end, 0);
            break;
        case MENU_SEARCH:
            if (m_how == PICK_NONE) {
                tty_nhbell();
                break;
            } else {
                char searchbuf[BUFSZ + 2], tmpbuf[BUFSZ];
                boolean on_curr_page = FALSE;
                int lineno = 0;

                tty_getlin("Search for:", tmpbuf);
                if (!tmpbuf[0] || tmpbuf[0] == '\033')
                    break;
                Sprintf(searchbuf, "*%s*", tmpbuf);

                for (curr = m_mlist; curr; curr = curr->next) {
                    if (on_curr_page)
                        lineno++;
                    if (curr == page_start)
                        on_curr_page = TRUE;
                    else if (curr == page_end)
                        on_curr_page = FALSE;
                    if (curr->identifier.a_void
                        && pmatchi(searchbuf, curr->str)) {
                        toggle_menu_curr(m_window, curr, lineno, on_curr_page,
                            counting, count);
                        if (m_how == PICK_ONE) {
                            finished = TRUE;
                            break;
                        }
                    }
                }
            }
            break;
        case MENU_EXPLICIT_CHOICE:
            morc = really_morc;
            /*FALLTHRU*/
        default:
            if (m_how == PICK_NONE || !index(resp, morc)) {
                /* unacceptable input received */
                tty_nhbell();
                break;
            } else if (index(gacc, morc)) {
            group_accel:
                /* group accelerator; for the PICK_ONE case, we know that
                it matches exactly one item in order to be in gacc[] */
                invert_all(m_window, page_start, page_end, morc);
                if (m_how == PICK_ONE)
                    finished = TRUE;
                break;
            }
            /* find, toggle, and possibly update */
            for (n = 0, curr = page_start; curr != page_end;
                n++, curr = curr->next)
                if (morc == curr->selector) {
                    toggle_menu_curr(m_window, curr, n, TRUE, counting, count);
                    if (m_how == PICK_ONE)
                        finished = TRUE;
                    break; /* from `for' loop */
                }
            break;
        }

    } /* while */
    m_morestr = msave;
    free((genericptr_t)morestr);
}

void
MenuWindow::process_lines()
{
    assert(m_lines.size() > 0);

    auto iter = m_lines.begin();
    m_rows = g_textGrid.GetDimensions().m_y - m_offy;
    int row = 0;

    while (iter != m_lines.end()) {
        auto line = *iter++;

        tty_curs(m_window, 1, row++);
        cl_end();

        if (line.second.size() > 0) {
            const char * str = line.second.c_str();
            int attr = line.first;
            if (m_offx) {
                win_putc(m_window, ' ');
            }
            TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);
            const char *cp;

            for (cp = str;
                *cp && g_textGrid.GetCursor().m_x < g_textGrid.GetDimensions().m_x;
                cp++)
                win_putc(m_window, *cp, TextColor::NoColor, useAttribute);
        }

        if (row == (m_rows - 1) || iter == m_lines.end()) {
            tty_curs(m_window, 1, row);
            cl_end();
            dmore(this, quitchars);
            if (morc == '\033') {
                m_flags |= WIN_CANCELLED;
                break;
            }

            tty_curs(m_window, 1, 0);
            cl_eos();
            row = 0;
        }

    }
}

void MenuWindow::set_all_on_page(
    winid window,
    tty_menu_item *page_start,
    tty_menu_item *page_end)
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && !curr->selected) {
            curr->selected = TRUE;
            set_item_state(n, curr);
        }
}

void MenuWindow::unset_all_on_page(
    winid window,
    tty_menu_item *page_start,
    tty_menu_item *page_end)
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && curr->selected) {
            curr->selected = FALSE;
            curr->count = -1L;
            set_item_state(n, curr);
        }
}

/* acc - group accelerator, 0 => all */
void
MenuWindow::invert_all_on_page(winid window, tty_menu_item *page_start, tty_menu_item *page_end, char acc)
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && (acc == 0 || curr->gselector == acc)) {
            if (curr->selected) {
                curr->selected = FALSE;
                curr->count = -1L;
            } else
                curr->selected = TRUE;
            set_item_state(n, curr);
        }
}

/*
* Invert all entries that match the give group accelerator (or all if zero).
* acc - group accelerator, 0 => all
*/
void
MenuWindow::invert_all(winid window, tty_menu_item *page_start, tty_menu_item *page_end, char acc)
{
    tty_menu_item *curr;
    boolean on_curr_page;
    CoreWindow *coreWin = GetCoreWindow(window);
    MenuWindow *menuWin = ToMenuWindow(coreWin);

    invert_all_on_page(window, page_start, page_end, acc);

    /* invert the rest */
    for (on_curr_page = FALSE, curr = menuWin->m_mlist; curr; curr = curr->next) {
        if (curr == page_start)
            on_curr_page = TRUE;
        else if (curr == page_end)
            on_curr_page = FALSE;

        if (!on_curr_page && curr->identifier.a_void
            && (acc == 0 || curr->gselector == acc)) {
            if (curr->selected) {
                curr->selected = FALSE;
                curr->count = -1;
            } else
                curr->selected = TRUE;
        }
    }
}

boolean
MenuWindow::toggle_menu_curr(
    winid window,
    tty_menu_item *curr,
    int lineno,
    boolean in_view,
    boolean counting,
    long count)
{
    if (curr->selected) {
        if (counting && count > 0) {
            curr->count = count;
            if (in_view)
                set_item_state(lineno, curr);
            return TRUE;
        } else { /* change state */
            curr->selected = FALSE;
            curr->count = -1L;
            if (in_view)
                set_item_state(lineno, curr);
            return TRUE;
        }
    } else { /* !selected */
        if (counting && count > 0) {
            curr->count = count;
            curr->selected = TRUE;
            if (in_view)
                set_item_state(lineno, curr);
            return TRUE;
        } else if (!counting) {
            curr->selected = TRUE;
            if (in_view)
                set_item_state(lineno, curr);
            return TRUE;
        }
        /* do nothing counting&&count==0 */
    }
    return FALSE;
}

void
MenuWindow::set_item_state(
    int lineno,
    tty_menu_item *item)
{
    char ch = item->selected ? (item->count == -1L ? '+' : '#') : '-';

    tty_curs(m_window, 4, lineno);
    win_putc(m_window, ch, TextColor::NoColor, (TextAttribute)(item->attr != 0 ? 1 << item->attr : 0));

}
