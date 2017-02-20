/* NetHack 3.6	uwp_message_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

MessageWindow g_messageWindow;

MessageWindow::MessageWindow() : CoreWindow(NHW_MESSAGE, MESSAGE_WINDOW)
{
    // msg
    m_mustBeSeen = false;
    m_mustBeErased = false;
    m_nextIsPrompt = false;

    m_msgIter = m_msgList.end();

    // core
    /* message window, 1 line long, very wide, top of screen */
    m_offx = 0;
    m_offy = 0;

    /* sanity check */
    if (iflags.msg_history < kMinMessageHistoryLength)
        iflags.msg_history = kMinMessageHistoryLength;
    else if (iflags.msg_history > kMaxMessageHistoryLength)
        iflags.msg_history = kMaxMessageHistoryLength;

    m_rows = iflags.msg_history;
    m_cols = kScreenWidth;
    m_stop = false;
}

MessageWindow::~MessageWindow()
{
}

void MessageWindow::free_window_info()
{
    m_msgList.clear();
    m_msgIter = m_msgList.end();
}

void MessageWindow::Clear()
{
    if (m_mustBeErased) {
        home();
        cl_end();
        if (m_cury)
            docorner(0, m_cury + 1);
        m_mustBeErased = false;
    }

    CoreWindow::Clear();
}

void MessageWindow::Display(bool blocking)
{
    if (m_stop)
        return;

    g_rawprint = 0;

    if (m_mustBeSeen) {
        more();
        assert(!m_mustBeSeen);

        if (m_mustBeErased)
            tty_clear_nhwindow(m_window);
    }

    m_curx = 0;
    m_cury = 0;

    if (!m_active)
        iflags.window_inited = TRUE;

    m_active = 1;
}

void MessageWindow::Dismiss()
{
    if (m_mustBeSeen)
        tty_display_nhwindow(WIN_MESSAGE, TRUE);

    CoreWindow::Dismiss();
    m_stop = false;
}

void MessageWindow::Putstr(int attr, const char *str)
{
    update_topl(str);
}

void MessageWindow::removetopl(int n)
{
    while (n-- > 0)
        putsyms("\b \b", TextColor::NoColor, TextAttribute::None);
}

void MessageWindow::redotoplin(const char *str)
{
    home();

    m_curx = 0;
    m_cury = 0;

    putsyms(str, TextColor::NoColor, TextAttribute::None);
    cl_end();
    m_mustBeSeen = true;
    m_mustBeErased = true;

    if (m_nextIsPrompt) {
        m_nextIsPrompt = false;
    } else if (m_cury != 0)
        more();

}

int MessageWindow::doprev_message()
{
    winid prevmsg_win;
    int i;
    if (iflags.prevmsg_window != 's') {           /* not single */
        if (iflags.prevmsg_window == 'f') { /* full */
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");

            m_msgIter = m_msgList.end();

            for (auto & msg : m_msgList)
                putstr(prevmsg_win, 0, msg.c_str());

            putstr(prevmsg_win, 0, toplines);
            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
        } else if (iflags.prevmsg_window == 'c') { /* combination */
            do {
                morc = 0;
                if (m_msgIter == m_msgList.end()) {
                    g_dismiss_more = C('p'); /* ^P ok at --More-- */
                    redotoplin(toplines);

                    if (m_msgIter != m_msgList.begin())
                        m_msgIter--;

                } else {
                    auto iter = m_msgIter;
                    iter++;

                    if (iter == m_msgList.end()) {
                        g_dismiss_more = C('p'); /* ^P ok at --More-- */
                        redotoplin(iter->c_str());

                        if (m_msgIter != m_msgList.begin())
                            m_msgIter--;
                        else
                            m_msgIter = m_msgList.end();

                    } else {
                        prevmsg_win = create_nhwindow(NHW_MENU);
                        putstr(prevmsg_win, 0, "Message History");
                        putstr(prevmsg_win, 0, "");

                        m_msgIter = m_msgList.end();

                        for (auto & msg : m_msgList)
                            putstr(prevmsg_win, 0, msg.c_str());

                        putstr(prevmsg_win, 0, toplines);
                        display_nhwindow(prevmsg_win, TRUE);
                        destroy_nhwindow(prevmsg_win);
                    }
                }

            } while (morc == C('p'));
            g_dismiss_more = 0;
        } else { /* reversed */
            morc = 0;
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            putstr(prevmsg_win, 0, toplines);

            auto & iter = m_msgList.end();
            while (iter != m_msgList.begin())
            {
                putstr(prevmsg_win, 0, iter->c_str());
                iter--;
            }

            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
            m_msgIter = m_msgList.end();
            g_dismiss_more = 0;
        }
    } else if (iflags.prevmsg_window == 's') { /* single */
        g_dismiss_more = C('p'); /* <ctrl/P> allowed at --More-- */
        do {
            morc = 0;
            if (m_msgIter == m_msgList.end()) {
                redotoplin(toplines);
            } else {
                redotoplin(m_msgIter->c_str());
            }

            if (m_msgIter != m_msgList.begin())
                m_msgIter--;
            else
                m_msgIter = m_msgList.end();

        } while (morc == C('p'));
        g_dismiss_more = 0;
    }
    return 0;
}

char MessageWindow::yn_function(
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
    boolean doprev = 0;
    char prompt[BUFSZ];

    if (m_mustBeSeen && !m_stop)
        more();

    m_stop = false;
    m_nextIsPrompt = true;

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
        assert(!m_nextIsPrompt);
    } else {
        /* no restriction on allowed response, so always preserve case */
        /* preserve_case = TRUE; -- moot since we're jumping to the end */
        pline("%s ", query);
        assert(!m_nextIsPrompt);

        q = readchar();
        goto clean_up;
    }

    do { /* loop until we get valid input */
        q = readchar();
        if (!preserve_case)
            q = lowc(q);
        if (q == '\020') { /* ctrl-P */
            if (iflags.prevmsg_window != 's') {
                (void)tty_doprev_message();
                tty_clear_nhwindow(WIN_MESSAGE);
                m_msgIter = m_msgList.end();
                addtopl(prompt);
            } else {
                if (!doprev)
                    (void) tty_doprev_message(); /* need two initially */
                (void)tty_doprev_message();
                doprev = 1;
            }
            q = '\0'; /* force another loop iteration */
            continue;
        } else if (doprev) {
            /* BUG[?]: this probably ought to check whether the
            character which has just been read is an acceptable
            response; if so, skip the reprompt and use it. */
            tty_clear_nhwindow(WIN_MESSAGE);
            m_msgIter = m_msgList.end();
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
        } else if (index(quitchars, q)) {
            q = def;
            break;
        }
        if (!index(resp, q) && !digit_ok) {
            tty_nhbell();
            q = (char)0;
        } else if (q == '#' || digit_ok) {
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
                } else if (z == 'y' || index(quitchars, z)) {
                    if (z == '\033')
                        value = -1; /* abort */
                    z = '\n';       /* break */
                } else if (z == '\b') {
                    if (n_len <= 1) {
                        value = -1;
                        break;
                    } else {
                        value /= 10;
                        removetopl(1);
                        n_len--;
                    }
                } else {
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
                removetopl(n_len);
                n_len = 0;
                q = '\0';
            }
        }
    } while (!q);

    if (q != '#') {
        Sprintf(rtmp, "%c", q);
        addtopl(rtmp);
    }
clean_up:
    m_mustBeSeen = false;

    if (m_cury)
        tty_clear_nhwindow(WIN_MESSAGE);

    return q;
}

/* add to the top line */
void MessageWindow::update_topl(const char *bp)
{
    register char *tl, *otl;
    register int n0;
    int notdied = 1;

    /* If there is room on the line, print message on same line */
    /* But messages like "You die..." deserve their own line */
    n0 = strlen(bp);
    if ((m_mustBeSeen || m_stop) && m_cury == 0
        && n0 + (int)strlen(toplines) + 3 < CO - 8 /* room for --More-- */
        && (notdied = strncmp(bp, "You die", 7)) != 0) {
        strncat(toplines, "  ", sizeof(toplines) - strlen(toplines) - 1);
        strncat(toplines, bp, sizeof(toplines) - strlen(toplines) - 1);
        m_curx += 2;
        if (!m_stop)
            addtopl(bp);
        return;
    } else if (!m_stop) {
        if (m_mustBeSeen) {
            more();
        } else if (m_cury) { /* for when flags.toplin == 2 && cury > 1 */
            docorner(0, m_cury + 1); /* reset cury = 0 if redraw screen */
            m_curx = m_cury = 0;   /* from home--cls() & docorner(0,n) */
        }
    }
    remember_topl();
    strncpy(toplines, bp, TBUFSZ - 1);

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
        m_stop = false;
    if (!m_stop)
        redotoplin(toplines);
}

void MessageWindow::hooked_tty_getlin(const char *query, char *bufp, getlin_hook_proc hook)
{
    register char *obufp = bufp;
    register int c;
    boolean doprev = 0;

    if (m_mustBeSeen && !m_stop)
        more();

    m_stop = false;

    m_nextIsPrompt = true;
    pline("%s ", query);
    assert(!m_nextIsPrompt);
    *obufp = 0;
    for (;;) {
        strncpy(toplines, query, sizeof(toplines) - 1);
        strncat(toplines, " ", sizeof(toplines) - strlen(toplines) - 1);
        strncat(toplines, obufp, sizeof(toplines) - strlen(toplines) - 1);
        c = pgetchar();
        if (c == '\033' || c == EOF) {
            if (c == '\033' && obufp[0] != '\0') {
                obufp[0] = '\0';
                bufp = obufp;
                tty_clear_nhwindow(WIN_MESSAGE);
                m_msgIter = m_msgList.end();
                addtopl(query);
                addtopl(" ");
                addtopl(obufp);
            } else {
                obufp[0] = '\033';
                obufp[1] = '\0';
                break;
            }
        }
        if (c == '\020') { /* ctrl-P */
            if (iflags.prevmsg_window != 's') {
                (void)tty_doprev_message();
                tty_clear_nhwindow(WIN_MESSAGE);
                m_msgIter = m_msgList.end();
                addtopl(query);
                addtopl(" ");
                *bufp = 0;
                addtopl(obufp);
            } else {
                if (!doprev)
                    (void) tty_doprev_message(); /* need two initially */
                (void)tty_doprev_message();
                doprev = 1;
                continue;
            }
        } else if (doprev && iflags.prevmsg_window == 's') {
            tty_clear_nhwindow(WIN_MESSAGE);
            m_msgIter = m_msgList.end();
            doprev = 0;
            addtopl(query);
            addtopl(" ");
            *bufp = 0;
            addtopl(obufp);
        }
        if (c == '\b') {
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
            } else
                tty_nhbell();
#if defined(apollo)
        } else if (c == '\n' || c == '\r') {
#else
        } else if (c == '\n') {
#endif
#ifndef NEWAUTOCOMP
            *bufp = 0;
#endif /* not NEWAUTOCOMP */
            break;
        } else if (' ' <= (unsigned char)c && c != '\177'
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
            } else if (i > bufp) {
                char *s = i;

                /* erase rest of prior guess */
                for (; i > bufp; --i)
                    putsyms(" ", TextColor::NoColor, TextAttribute::None);
                for (; s > bufp; --s)
                    putsyms("\b", TextColor::NoColor, TextAttribute::None);
#endif /* NEWAUTOCOMP */
            }
        } else if (c == kKillChar || c == '\177') { /* Robert Viduya */
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
        } else
            tty_nhbell();
    }
    m_mustBeSeen = false;
    clear_nhwindow(WIN_MESSAGE); /* clean up after ourselves */
}

void
MessageWindow::putsyms(const char *str, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute)
{
    while (*str)
        topl_putsym(*str++, textColor, textAttribute);
}

void MessageWindow::remember_topl()
{
    if (!*toplines)
        return;

    m_msgList.push_back(std::string(toplines));
    while (m_msgList.size() > m_rows)
        m_msgList.pop_front();
    *toplines = '\0';
    m_msgIter = m_msgList.end();
}

void MessageWindow::topl_putsym(char c, TextColor color, TextAttribute attribute)
{
    switch (c) {
    case '\b':
        core_putc('\b');
        return;
    case '\n':
        cl_end();
        core_putc('\n');
        break;
    default:
        if (m_curx + m_offx == CO - 1)
            topl_putsym('\n', TextColor::NoColor, TextAttribute::None); /* 1 <= curx < CO; avoid CO */
        core_putc(c);
    }

    if (m_curx == 0)
        cl_end();
}

void MessageWindow::addtopl(const char *s)
{
    set_cursor(m_curx, m_cury);
    putsyms(s, TextColor::NoColor, TextAttribute::None);
    cl_end();
    m_mustBeSeen = true;
    m_mustBeErased = true;
}

void MessageWindow::more()
{
    assert(!m_nextIsPrompt);

    if (m_mustBeErased) {
        set_cursor(m_curx, m_cury);
        if (m_curx >= CO - 8)
            topl_putsym('\n', TextColor::NoColor, TextAttribute::None);
    }

    putsyms(defmorestr, TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);

    xwaitforspace("\033 ");

    if (morc == '\033')
        m_stop = true;

    /* if the message is more then one line then erase the entire message */
    if (m_mustBeErased && m_cury) {
        docorner(0, m_cury + 1);
        m_curx = m_cury = 0;
        home();
        m_mustBeErased = false;
    }
    /* if the single line message was cancelled then erase the message */
    else if (morc == '\033') {
        m_curx = m_cury = 0;
        home();
        cl_end();
        m_mustBeErased = false;
    }
    /* otherwise we have left the message visible */

    m_mustBeSeen = false;
}

/* special hack for treating top line --More-- as a one item menu */
char MessageWindow::uwp_message_menu(char let, int how, const char *mesg)
{
    /* "menu" without selection; use ordinary pline, no more() */
    if (how == PICK_NONE) {
        pline("%s", mesg);
        return 0;
    }

    g_dismiss_more = let;
    morc = 0;
    /* barebones pline(); since we're only supposed to be called after
    response to a prompt, we'll assume that the display is up to date */
    tty_putstr(WIN_MESSAGE, 0, mesg);
    /* if `mesg' didn't wrap (triggering --More--), force --More-- now */

    if (m_mustBeSeen) {
        more();
        assert(!m_mustBeSeen);

        if (m_mustBeErased)
            tty_clear_nhwindow(WIN_MESSAGE);
    }
    /* normally <ESC> means skip further messages, but in this case
    it means cancel the current prompt; any other messages should
    continue to be output normally */
    MessageWindow * winMsg = (MessageWindow *) g_wins[WIN_MESSAGE];
    winMsg->m_stop = false;
    g_dismiss_more = 0;

    return ((how == PICK_ONE && morc == let) || morc == '\033') ? morc : '\0';
}

void MessageWindow::docorner(int xmin, int ymax)
{
    register int y;
    MapWindow *mapWin = (MapWindow *)g_wins[WIN_MAP];
    StatusWindow * statusWin = (StatusWindow *)g_wins[WIN_STATUS];

    for (y = 0; y < ymax; y++) {
        set_cursor(xmin, y);
        cl_end();                       /* clear to end of line */
        if (y < mapWin->m_offy || y > ROWNO)
            continue; /* only refresh board  */
        row_refresh(xmin - (int)mapWin->m_offx, COLNO - 1, y - (int)mapWin->m_offy);
    }

    if (ymax >= (int)statusWin->m_offy) {
        /* we have wrecked the bottom line */
        context.botlx = 1;
        bot();
    }
}

void
MessageWindow::uwp_putmsghistory(
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
        strncpy(toplines, msg, sizeof(toplines) - 1);
    } else {

        assert(initd);
        for (auto msg : m_snapshot_msgList) {
            remember_topl();
            strncpy(toplines, msg.c_str(), sizeof(toplines) - 1);
        }

        /* now release the snapshot */
        initd = FALSE; /* reset */
    }
}

char *
MessageWindow::uwp_getmsghistory(boolean init)
{
    static std::list<std::string>::iterator iter;

    if (init) {
        msghistory_snapshot(FALSE);
        iter = m_snapshot_msgList.begin();
    }

    if (iter == m_snapshot_msgList.end())
        return NULL;

    return (char *)(iter++)->c_str();
}

/* collect currently available message history data into a sequential array;
optionally, purge that data from the active circular buffer set as we go */
void
MessageWindow::msghistory_snapshot(
    boolean purge) /* clear message history buffer as we copy it */
{
    /* flush toplines[], moving most recent message to history */
    remember_topl();

    m_snapshot_msgList = m_msgList;

    /* for a destructive snapshot, history is now completely empty */
    if (purge) {
        m_msgList.clear();
        m_msgIter = m_msgList.end();
    }
}

