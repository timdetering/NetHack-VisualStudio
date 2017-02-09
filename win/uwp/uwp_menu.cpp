/* NetHack 3.6	uwp_menu.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

extern "C" {

extern char winpanicstr[];
extern char mapped_menu_cmds[]; /* from options.c */

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

STATIC_OVL void
dmore(
    struct WinDesc *cw,
    const char *s) /* valid responses */
{
    const char *prompt = cw->morestr ? cw->morestr : defmorestr;
    int offset = (cw->type == NHW_TEXT) ? 1 : 2;

    tty_curs(BASE_WINDOW, (int)g_uwpDisplay->curx + offset,
        (int)g_uwpDisplay->cury);
    xputs(prompt, TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);
    g_uwpDisplay->curx += strlen(prompt);

    xwaitforspace(s);
}

STATIC_OVL void
set_item_state(
    winid window,
    int lineno,
    tty_menu_item *item)
{
    char ch = item->selected ? (item->count == -1L ? '+' : '#') : '-';

    tty_curs(window, 4, lineno);
    (void)xputc(ch, TextColor::NoColor, (TextAttribute) (item->attr != 0 ? 1 << item->attr : 0));
    g_uwpDisplay->curx++;
}


boolean
toggle_menu_curr(
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
                set_item_state(window, lineno, curr);
            return TRUE;
        }
        else { /* change state */
            curr->selected = FALSE;
            curr->count = -1L;
            if (in_view)
                set_item_state(window, lineno, curr);
            return TRUE;
        }
    }
    else { /* !selected */
        if (counting && count > 0) {
            curr->count = count;
            curr->selected = TRUE;
            if (in_view)
                set_item_state(window, lineno, curr);
            return TRUE;
        }
        else if (!counting) {
            curr->selected = TRUE;
            if (in_view)
                set_item_state(window, lineno, curr);
            return TRUE;
        }
        /* do nothing counting&&count==0 */
    }
    return FALSE;
}


STATIC_OVL void
set_all_on_page(
    winid window,
    tty_menu_item *page_start,
    tty_menu_item *page_end)
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && !curr->selected) {
            curr->selected = TRUE;
            set_item_state(window, n, curr);
        }
}

STATIC_OVL void
unset_all_on_page(
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
            set_item_state(window, n, curr);
        }
}

/* acc - group accelerator, 0 => all */
STATIC_OVL void
invert_all_on_page(winid window, tty_menu_item *page_start, tty_menu_item *page_end, char acc)
{
    tty_menu_item *curr;
    int n;

    for (n = 0, curr = page_start; curr != page_end; n++, curr = curr->next)
        if (curr->identifier.a_void && (acc == 0 || curr->gselector == acc)) {
            if (curr->selected) {
                curr->selected = FALSE;
                curr->count = -1L;
            }
            else
                curr->selected = TRUE;
            set_item_state(window, n, curr);
        }
}

/*
* Invert all entries that match the give group accelerator (or all if zero).
* acc - group accelerator, 0 => all
*/
STATIC_OVL void
invert_all(winid window, tty_menu_item *page_start, tty_menu_item *page_end, char acc)
{
    tty_menu_item *curr;
    boolean on_curr_page;
    struct WinDesc *cw = g_wins[window];

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

void
process_menu_window(winid window, struct WinDesc * cw)
{
    tty_menu_item *page_start, *page_end, *curr;
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
                    tty_curs(window, 1, 0);
                    cl_eos();
                }
                else
                    clear_screen();
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

                    tty_curs(window, 1, page_lines);
                    if (cw->offx)
                        cl_end();

                    TextColor useColor = TextColor::NoColor;
                    TextAttribute useAttribute = TextAttribute::None;

                    (void)xputc(' ', useColor, useAttribute);
                    ++g_uwpDisplay->curx;

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
#ifndef WIN32CON
                        (int) ++g_uwpDisplay->curx < (int)g_uwpDisplay->cols;
#else
                        (int)g_uwpDisplay->curx < (int)g_uwpDisplay->cols;
                        g_uwpDisplay->curx++,
#endif
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
                                (void) xputc('+', useColor, useAttribute); /* all selected */
                            else
                                (void)xputc('#', useColor, useAttribute); /* count selected */
                        }
                        else
                            (void)xputc(*cp, useColor, useAttribute);
                    } /* for *cp */
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
                    tty_curs(window, 1, n);
                    cl_end();
                }
            }

            /* set extra chars.. */
            Strcat(resp, default_menu_cmds);
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
                Strcpy(cw->morestr, defmorestr);

            tty_curs(window, 1, page_lines);
            cl_end();
            dmore(cw, resp);
        }
        else {
            /* just put the cursor back... */
            tty_curs(window, (int)strlen(cw->morestr) + 2, page_lines);
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
                for (curr = cw->mlist; curr; curr = curr->next) {
                    curr->selected = FALSE;
                    curr->count = -1L;
                }
                cw->flags |= WIN_CANCELLED;
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
            else if (morc == ' ') {
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
                tty_nhbell();
                break;
            }
            else {
                char searchbuf[BUFSZ + 2], tmpbuf[BUFSZ];
                boolean on_curr_page = FALSE;
                int lineno = 0;

                tty_getlin("Search for:", tmpbuf);
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
            morc = really_morc;
            /*FALLTHRU*/
        default:
            if (cw->how == PICK_NONE || !index(resp, morc)) {
                /* unacceptable input received */
                tty_nhbell();
                break;
            }
            else if (index(gacc, morc)) {
            group_accel:
                /* group accelerator; for the PICK_ONE case, we know that
                it matches exactly one item in order to be in gacc[] */
                invert_all(window, page_start, page_end, morc);
                if (cw->how == PICK_ONE)
                    finished = TRUE;
                break;
            }
            /* find, toggle, and possibly update */
            for (n = 0, curr = page_start; curr != page_end;
                n++, curr = curr->next)
                if (morc == curr->selector) {
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

void
process_text_window(winid window, struct WinDesc *cw)
{
    int i, n, attr;
    register char *cp;

    for (n = 0, i = 0; i < cw->maxrow; i++) {
        if (!cw->offx && (n + cw->offy == g_uwpDisplay->rows - 1)) {
            tty_curs(window, 1, n);
            cl_end();
            dmore(cw, quitchars);
            if (morc == '\033') {
                cw->flags |= WIN_CANCELLED;
                break;
            }
            if (cw->offy) {
                tty_curs(window, 1, 0);
                cl_eos();
            }
            else
                clear_screen();
            n = 0;
        }
        tty_curs(window, 1, n++);
#ifdef H2344_BROKEN
        cl_end();
#else
        if (cw->offx)
            cl_end();
#endif
        if (cw->data[i]) {
            attr = cw->data[i][0] - 1;
            if (cw->offx) {
                (void)xputc(' ', TextColor::NoColor, TextAttribute::None);
                ++g_uwpDisplay->curx;
            }
            TextAttribute useAttribute = (TextAttribute)(attr != 0 ? 1 << attr : 0);
            for (cp = &cw->data[i][1];
#ifndef WIN32CON
                *cp && (int) ++g_uwpDisplay->curx < (int)g_uwpDisplay->cols;
                cp++)
#else
                *cp && (int)g_uwpDisplay->curx < (int)g_uwpDisplay->cols;
            cp++, g_uwpDisplay->curx++)
#endif
                (void)xputc(*cp, TextColor::NoColor, useAttribute);
        }
    }
    if (i == cw->maxrow) {
#ifdef H2344_BROKEN
        if (cw->type == NHW_TEXT) {
            tty_curs(BASE_WINDOW, 1, (int)g_uwpDisplay->cury + 1);
            cl_eos();
        }
#endif
        tty_curs(BASE_WINDOW, (int)cw->offx + 1,
            (cw->type == NHW_TEXT) ? (int)g_uwpDisplay->rows - 1 : n);
        cl_end();
        dmore(cw, quitchars);
        if (morc == '\033')
            cw->flags |= WIN_CANCELLED;
    }
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
    register struct WinDesc *cw = 0;
    tty_menu_item *item;
    const char *newstr;
    char buf[4 + BUFSZ];

    if (str == (const char *)0)
        return;

    if (window == WIN_ERR
        || (cw = g_wins[window]) == (struct WinDesc *) 0
        || cw->type != NHW_MENU)
        panic(winpanicstr, window);

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

    item = (tty_menu_item *)alloc(sizeof(tty_menu_item));
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
    struct WinDesc *cw = 0;
    tty_menu_item *curr;
    short len;
    int lmax, n;
    char menu_ch;

    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0
        || cw->type != NHW_MENU)
        panic(winpanicstr, window);

    /* Reverse the list so that items are in correct order. */
    cw->mlist = reverse(cw->mlist);

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
    cw->npages = (cw->nitems + (lmax - 1)) / lmax; /* # of pages */

                                                   /* make sure page list is large enough */
    if (cw->plist_size < cw->npages + 1 /*need 1 slot beyond last*/) {
        if (cw->plist)
            free((genericptr_t)cw->plist);
        cw->plist_size = cw->npages + 1;
        cw->plist = (tty_menu_item **)alloc(cw->plist_size
            * sizeof(tty_menu_item *));
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
        if (len > (int)g_uwpDisplay->cols) {
            curr->str[g_uwpDisplay->cols - 2] = 0;
            len = g_uwpDisplay->cols;
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

    if (len > (int)g_uwpDisplay->cols) {
        /* truncate the prompt if it's too long for the screen */
        if (cw->npages <= 1) /* only str in single page case */
            cw->morestr[g_uwpDisplay->cols] = 0;
        len = g_uwpDisplay->cols;
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
tty_select_menu(
    winid window,
    int how,
    menu_item **menu_list)
{
    register struct WinDesc *cw = 0;
    tty_menu_item *curr;
    menu_item *mi;
    int n, cancelled;

    if (window == WIN_ERR || (cw = g_wins[window]) == (struct WinDesc *) 0
        || cw->type != NHW_MENU)
        panic(winpanicstr, window);

    *menu_list = (menu_item *)0;
    cw->how = (short)how;
    morc = 0;
    tty_display_nhwindow(window, TRUE);
    cancelled = !!(cw->flags & WIN_CANCELLED);
    tty_dismiss_nhwindow(window); /* does not destroy window data */

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
tty_message_menu(
    char let,
    int how,
    const char *mesg)
{
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
    if (g_uwpDisplay->toplin == 1) {
        more();
        g_uwpDisplay->toplin = 1; /* more resets this */
        tty_clear_nhwindow(WIN_MESSAGE);
    }
    /* normally <ESC> means skip further messages, but in this case
    it means cancel the current prompt; any other messages should
    continue to be output normally */
    g_wins[WIN_MESSAGE]->flags &= ~WIN_CANCELLED;
    g_uwpDisplay->dismiss_more = 0;

    return ((how == PICK_ONE && morc == let) || morc == '\033') ? morc : '\0';
}

} /* extern "C" */