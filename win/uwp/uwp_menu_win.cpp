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

MenuWindow::MenuWindow(winid window) : CoreWindow(NHW_MENU, window)
{
    // menu
    m_npages = 0;
    m_how = 0;

    m_offx = 0;
    m_offy = 0;
    m_rows = 0;
    m_cols = 0;

    m_cancelled = false;
}

MenuWindow::~MenuWindow()
{
}

void MenuWindow::Destroy()
{
    CoreWindow::Destroy();
}

void MenuWindow::Display(bool blocking)
{
    if (m_cancelled)
        return;

    g_rawprint = 0;

    m_active = 1;

    int screenWidth = kScreenWidth;
    int screenHeight = kScreenHeight;
    m_offx = m_cols < screenWidth ? screenWidth - m_cols - 1 : 0;
    m_offx = min(screenWidth / 2, m_offx);

    m_offy = 0;

    if (g_messageWindow.m_mustBeSeen)
        g_messageWindow.Display(true);

    /* if we are going to have more then one page to display the use full screen */
    /* or if we are not supposed to use menu overlays */;
    if (m_rows >= screenHeight || !iflags.menu_overlay)
    {
        m_offx = 0;
        /* TODO(bhouse) why do we test and do something different for overlay? */
        if (iflags.menu_overlay) {
            set_cursor(0, 0);
            clear_to_end_of_screen();
        } else
            clear_screen();

        /* we just cleared the message area so we no longer need to erase */
        g_messageWindow.m_mustBeErased = false;
    } else {
        g_messageWindow.Clear();
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
        assert(iflags.window_inited);
        docrt();
        m_active = 0;
    }
    m_flags = 0;
    m_cancelled = false;
}

void MenuWindow::Putstr(int attr, const char *str)
{
    if (m_cancelled)
        return;

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

void MenuWindow::Clear()
{
    if (m_active)
        clear_screen();

    m_items.clear();
    m_pages.clear();

    m_npages = 0;
    m_how = 0;

    CoreWindow::Clear();
}


void MenuWindow::process_menu()
{
    itemIter page_start;
    itemIter page_end;
    long count;
    int n, attr_n, curr_page, page_lines, resp_len;
    boolean finished, counting, reset_count;
    char *rp, resp[QBUFSZ], gacc[QBUFSZ], really_morc;
    const char *cp;

#define MENU_EXPLICIT_CHOICE 0x7f /* pseudo menu manipulation char */

    curr_page = page_lines = 0;
    page_start = m_items.end();
    page_end = m_items.end();
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

        n = 0;
        for (auto & item : m_items) {
            if (item.gselector && item.gselector != item.selector) {
                ++n;
                ++gcnt[GSELIDX(item.gselector)];
            }
        }

        if (n > 0) { /* at least one group accelerator found */
            rp = gacc;
            for(auto & item : m_items) {
                if (item.gselector && item.gselector != item.selector
                    && !index(gacc, item.gselector)
                    && (m_how == PICK_ANY
                    || gcnt[GSELIDX(item.gselector)] == 1)) {
                    *rp++ = item.gselector;
                    *rp = '\0'; /* re-terminate for index() */
                }
            }
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

        int response;

        if (page_start == m_items.end()) {
            /* new page to be displayed */
            if (curr_page < 0 || (m_npages > 0 && curr_page >= m_npages))
                panic("bad menu screen page #%d", curr_page);

            /* clear screen */
            if (!m_offx) { /* if not corner, do clearscreen */
                if (m_offy) {
                    set_cursor(0, 0);
                    clear_to_end_of_screen();
                } else
                    clear_screen();
            }

            rp = resp;
            if (m_npages > 0) {
                /* collect accelerators */
                page_start = m_pages[curr_page];
                page_end = m_pages[curr_page + 1];
                auto iter = page_start;
                for (page_lines = 0; iter != page_end; page_lines++, iter++) {
                    int attr, color = NO_COLOR;
                    auto curr = *iter;
                    if (curr.selector)
                        *rp++ = curr.selector;

                    set_cursor(0, page_lines);
                    if (m_offx)
                        clear_to_end_of_line();

                    TextColor useColor = TextColor::NoColor;
                    TextAttribute useAttribute = TextAttribute::None;

                    core_putc(' ', useColor, useAttribute);

                    if (!iflags.use_menu_color
                        || !get_menu_coloring((char *) curr.str.c_str(), &color, &attr))
                        attr = curr.attr;

                    /* which character to start attribute highlighting;
                    whole line for headers and such, after the selector
                    character and space and selection indicator for menu
                    lines (including fake ones that simulate grayed-out
                    entries, so we don't rely on curr->identifier here) */
                    attr_n = 0; /* whole line */
                    if (curr.str[0] && curr.str[1] == ' '
                        && curr.str[2] && index("-+#", curr.str[2])
                        && curr.str[3] == ' ')
                        /* [0]=letter, [1]==space, [2]=[-+#], [3]=space */
                        attr_n = 4; /* [4:N]=entry description */

                                    /*
                                    * Don't use xputs() because (1) under unix it calls
                                    * tputstr() which will interpret a '*' as some kind
                                    * of padding information and (2) it calls xputc to
                                    * actually output the character.  We're faster doing
                                    * this.
                                    */
                    for (n = 0, cp = curr.str.c_str();
                        *cp &&
                        g_textGrid.GetCursor().m_x < kScreenWidth;
                        cp++, n++) {

                        if (n == attr_n && (color != NO_COLOR
                            || attr != ATR_NONE)) {
                            useColor = (TextColor)color;
                            useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);
                        }

                        if (n == 2
                            && curr.identifier.a_void != 0
                            && curr.selected) {
                            if (curr.count == -1L)
                                core_putc('+', useColor, useAttribute);
                            else
                                core_putc('#', useColor, useAttribute);
                        } else
                            core_putc(*cp, useColor, useAttribute);
                    } /* for *cp */
                } /* if npages > 0 */
            } else {
                page_start = m_items.end();
                page_end = m_items.end();
                page_lines = 0;
            }
            *rp = 0;
            /* remember how many explicit menu choices there are */
            resp_len = (int)strlen(resp);

            /* corner window - clear extra lines from last page */
            if (m_offx) {
                for (n = page_lines + 1; n < m_rows; n++) {
                    set_cursor(0, n);
                    clear_to_end_of_line();
                }
            }

            /* set extra chars.. */
            Strcat(resp, default_menu_cmds);
            Strcat(resp, " ");                  /* next page or end */
            Strcat(resp, "0123456789\033\n\r"); /* counts, quit */
            Strcat(resp, gacc);                 /* group accelerators */
            Strcat(resp, mapped_menu_cmds);

            if (m_npages > 1) {
                char buf[128];
                snprintf(buf, sizeof(buf), " (%d of %d)", curr_page + 1, m_npages);
                m_morestr = std::string(buf);
            } else {
                m_morestr = std::string(" (end) ");
            }

            set_cursor(0, page_lines);
            clear_to_end_of_line();
            response = dmore(resp);
        } else {
            /* just put the cursor back... */
            m_morestr = std::string(" (end) ");
            set_cursor(m_morestr.size() + 1, page_lines);
            response = wait_for_response(resp);
        }

        really_morc = response; /* (only used with MENU_EXPLICIT_CHOICE */
        if ((rp = index(resp, response)) != 0 && rp < resp + resp_len)
            /* explicit menu selection; don't override it if it also
            happens to match a mapped menu command (such as ':' to
            look inside a container vs ':' to search) */
            response = MENU_EXPLICIT_CHOICE;
        else
            response = map_menu_cmd(response);

        switch (response) {
        case '0':
            /* special case: '0' is also the default ball class */
            if (!counting && index(gacc, response))
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
            count = (count * 10L) + (long)(response - '0');
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
                for (auto & item : m_items) {
                    item.selected = FALSE;
                    item.count = -1L;
                }
                m_cancelled = true;
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
                page_start = m_items.end();
            } else if (response == ' ') {
                /* ' ' finishes menus here, but stop '>' doing the same. */
                finished = TRUE;
            }
            break;
        case MENU_PREVIOUS_PAGE:
            if (m_npages > 0 && curr_page != 0) {
                --curr_page;
                page_start = m_items.end();
            }
            break;
        case MENU_FIRST_PAGE:
            if (m_npages > 0 && curr_page != 0) {
                page_start = m_items.end();
                curr_page = 0;
            }
            break;
        case MENU_LAST_PAGE:
            if (m_npages > 0 && curr_page != m_npages - 1) {
                page_start = m_items.end();
                curr_page = m_npages - 1;
            }
            break;
        case MENU_SELECT_PAGE:
            if (m_how == PICK_ANY)
                set_all_on_page(page_start, page_end);
            break;
        case MENU_UNSELECT_PAGE:
            unset_all_on_page(page_start, page_end);
            break;
        case MENU_INVERT_PAGE:
            if (m_how == PICK_ANY)
                invert_all_on_page(page_start, page_end, 0);
            break;
        case MENU_SELECT_ALL:
            if (m_how == PICK_ANY) {
                set_all_on_page(page_start, page_end);
                /* set the rest */
                for (auto & item : m_items) {
                    if (item.identifier.a_void && !item.selected)
                        item.selected = TRUE;
                }
            }
            break;
        case MENU_UNSELECT_ALL:
            unset_all_on_page(page_start, page_end);
            /* unset the rest */
            for (auto & item : m_items) {
                if (item.identifier.a_void && item.selected) {
                    item.selected = FALSE;
                    item.count = -1;
                }
            }
            break;
        case MENU_INVERT_ALL:
            if (m_how == PICK_ANY)
                invert_all(page_start, page_end, 0);
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

                for (auto iter = m_items.begin(); iter != m_items.end(); iter++) {
                    auto & item = *iter;
                    if (on_curr_page)
                        lineno++;
                    if (iter == page_start)
                        on_curr_page = TRUE;
                    else if (iter == page_end)
                        on_curr_page = FALSE;

                    if (item.identifier.a_void
                        && pmatchi(searchbuf, item.str.c_str())) {
                        toggle_menu_curr(item, lineno, on_curr_page,
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
            response = really_morc;
            /*FALLTHRU*/
        default:
            if (m_how == PICK_NONE || !index(resp, response)) {
                /* unacceptable input received */
                tty_nhbell();
                break;
            } else if (index(gacc, response)) {
            group_accel:
                /* group accelerator; for the PICK_ONE case, we know that
                it matches exactly one item in order to be in gacc[] */
                invert_all(page_start, page_end, response);
                if (m_how == PICK_ONE)
                    finished = TRUE;
                break;
            }
            /* find, toggle, and possibly update */
            auto iter = page_start;
            for (n = 0; iter != page_end; n++, iter++)
            {
                auto & item = *iter;
                if (response == item.selector) {
                    toggle_menu_curr(item, n, TRUE, counting, count);
                    if (m_how == PICK_ONE)
                        finished = TRUE;
                    break; /* from `for' loop */
                }
            }
            break;
        }

    } /* while */
}

void
MenuWindow::process_lines()
{
    assert(m_lines.size() > 0);

    auto iter = m_lines.begin();
    m_rows = kScreenHeight - m_offy;
    int row = 0;

    while (iter != m_lines.end()) {
        auto line = *iter++;

        set_cursor(0, row++);
        clear_to_end_of_line();

        if (line.second.size() > 0) {
            const char * str = line.second.c_str();
            int attr = line.first;
            if (m_offx) {
                core_putc(' ');
            }
            TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);
            const char *cp;

            for (cp = str;
                *cp && g_textGrid.GetCursor().m_x < kScreenWidth;
                cp++)
                core_putc(*cp, TextColor::NoColor, useAttribute);
        }

        if (row == (m_rows - 1) || iter == m_lines.end()) {
            set_cursor(0, row);
            clear_to_end_of_line();
            int response = dmore(quitchars);
            if (response == '\033') {
                m_cancelled = true;
                break;
            }

            set_cursor(0, 0);
            clear_to_end_of_screen();
            row = 0;
        }

    }
}

void MenuWindow::set_all_on_page(
    itemIter page_start,
    itemIter page_end)
{
    int n;

    auto iter = page_start;
    for (n = 0; iter != page_end; n++, iter++) {
        auto & item = *iter;
        if (item.identifier.a_void && !item.selected) {
            item.selected = TRUE;
            set_item_state(n, item);
        }
    }
}

void MenuWindow::unset_all_on_page(
    itemIter page_start,
    itemIter page_end)
{
    int n;

    auto iter = page_start;
    for (n = 0; iter != page_end; n++, iter++)
    {
        auto & item = *iter;
        if (item.identifier.a_void && item.selected) {
            item.selected = FALSE;
            item.count = -1L;
            set_item_state(n, item);
        }
    }
}

/* acc - group accelerator, 0 => all */
void
MenuWindow::invert_all_on_page(itemIter page_start, itemIter page_end, char acc)
{
    int n;

    auto iter = page_start;
    for (n = 0; iter != page_end; n++, iter++) {
        auto & item = *iter;
        if (item.identifier.a_void && (acc == 0 || item.gselector == acc)) {
            if (item.selected) {
                item.selected = FALSE;
                item.count = -1L;
            } else
                item.selected = TRUE;
            set_item_state(n, item);
        }
    }
}

/*
* Invert all entries that match the give group accelerator (or all if zero).
* acc - group accelerator, 0 => all
*/
void
MenuWindow::invert_all(itemIter page_start, itemIter page_end, char acc)
{
    boolean on_curr_page;

    invert_all_on_page(page_start, page_end, acc);

    /* invert the rest */
    for (auto iter = m_items.begin(); iter != m_items.end(); iter++) {
        auto & item = *iter;
        if (iter == page_start)
            on_curr_page = TRUE;
        else if (iter == page_end)
            on_curr_page = FALSE;

        if (!on_curr_page && item.identifier.a_void
            && (acc == 0 || item.gselector == acc)) {
            if (item.selected) {
                item.selected = FALSE;
                item.count = -1;
            } else
                item.selected = TRUE;
        }
    }
}

boolean
MenuWindow::toggle_menu_curr(
    tty_menu_item & item,
    int lineno,
    boolean in_view,
    boolean counting,
    long count)
{
    if (item.selected) {
        if (counting && count > 0) {
            item.count = count;
            if (in_view)
                set_item_state(lineno, item);
            return TRUE;
        } else { /* change state */
            item.selected = FALSE;
            item.count = -1L;
            if (in_view)
                set_item_state(lineno, item);
            return TRUE;
        }
    } else { /* !selected */
        if (counting && count > 0) {
            item.count = count;
            item.selected = TRUE;
            if (in_view)
                set_item_state(lineno, item);
            return TRUE;
        } else if (!counting) {
            item.selected = TRUE;
            if (in_view)
                set_item_state(lineno, item);
            return TRUE;
        }
        /* do nothing counting&&count==0 */
    }
    return FALSE;
}

void
MenuWindow::set_item_state(
    int lineno,
    tty_menu_item  & item)
{
    char ch = item.selected ? (item.count == -1L ? '+' : '#') : '-';

    set_cursor(3, lineno);
    core_putc(ch, TextColor::NoColor, (TextAttribute)(item.attr != 0 ? 1 << item.attr : 0));

}

int MenuWindow::uwp_select_menu(
    int how,
    menu_item **menu_list)
{
    menu_item *mi;
    int n;

    *menu_list = (menu_item *)0;
    m_how = (short)how;

    Display(TRUE);
    Dismiss();

    if (m_cancelled) {
        n = -1;
    } else {
        n = 0;
        for (auto & item : m_items)
            if (item.selected)
                n++;
    }

    if (n > 0) {
        *menu_list = (menu_item *)alloc(n * sizeof(menu_item));
        mi = *menu_list;
        for (auto & item : m_items) {
            if (item.selected) {
                mi->item = item.identifier;
                mi->count = item.count;
                mi++;
            }
        }
    }

    return n;
}

void MenuWindow::uwp_add_menu(
    const anything *identifier, /* what to return if selected */
    char ch,                    /* keyboard accelerator (0 = pick our own) */
    char gch,                   /* group accelerator (0 = no group) */
    int attr,                   /* attribute for string (like tty_putstr()) */
    const char *str,            /* menu string */
    boolean preselected)        /* item is marked as selected */
{
    tty_menu_item item;
    const char *newstr;
    char buf[4 + BUFSZ];

    if (str == (const char *)0)
        return;

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
    } else
        newstr = str;

    item.identifier = *identifier;
    item.count = -1L;
    item.selected = preselected;
    item.selector = ch;
    item.gselector = gch;
    item.attr = attr;
    item.str = std::string(newstr ? newstr : "");

    m_items.push_back(item);

}

void MenuWindow::uwp_end_menu(
    const char *prompt) /* prompt to for menu */
{
    short len;
    int lmax, n;
    char menu_ch;

    /* Put the prompt at the beginning of the menu. */
    if (prompt) {
        anything any;

        any = zeroany; /* not selectable */
        uwp_add_menu(&any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
        uwp_add_menu(&any, 0, 0, ATR_NONE, prompt, MENU_UNSELECTED);
    }

    lmax = kScreenHeight - 1;    /* # lines per page */
    m_npages = (m_items.size() + (lmax - 1)) / lmax; /* # of pages */

    m_cols = 0;  /* cols is set when the win is initialized... (why?) */
    menu_ch = '?'; /* lint suppression */
    m_pages.resize(m_npages+1);
    n = 0;
    for (auto iter = m_items.begin(); iter != m_items.end(); iter++) {
        auto & item = *iter;
        /* set page boundaries and character accelerators */
        if ((n % lmax) == 0) {
            menu_ch = 'a';
            m_pages[n/lmax] = iter;
        }
        if (item.identifier.a_void && !item.selector) {
            item.selector = menu_ch;
            item.str[0] = menu_ch;
            if (menu_ch++ == 'z')
                menu_ch = 'A';
        }

        /* cut off any lines that are too long */
        len = item.str.size() + 2; /* extra space at beg & end */
        int screenWidth = kScreenWidth;
        if (len > screenWidth) {
            item.str = item.str.substr(0, screenWidth - 2);
            len = screenWidth;
        }
        if (len > m_cols)
            m_cols = len;
        n++;
    }

    m_pages[m_npages] = m_items.end();

    /*
    * If greater than 1 page, morestr is "(x of y) " otherwise, "(end) "
    */
    if (m_npages > 1) {
        char buf[QBUFSZ];
        /* produce the largest demo string */
        Sprintf(buf, "(%ld of %ld) ", m_npages, m_npages);
        len = strlen(buf);
        m_morestr = dupstr("");
    } else {
        m_morestr = std::string("(end) ");
        len = m_morestr.size();
    }

    assert(len <= kScreenWidth);

    if (len > m_cols)
        m_cols = len;

    /*
    * The number of lines in the first page plus the morestr will be the
    * maximum size of the window.
    */
    if (m_npages > 1)
        m_rows = lmax + 1;
    else
        m_rows = m_items.size() + 1;
}

int MenuWindow::dmore(
    const char *s) /* valid responses */
{
    assert(m_morestr.size() > 0);
    core_puts(m_morestr.c_str(), TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);
    return wait_for_response(s);
}

