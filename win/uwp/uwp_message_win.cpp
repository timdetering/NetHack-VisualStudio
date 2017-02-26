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
    Init();

    m_offx = 0;
    m_offy = 0;

    /* sanity check */
    if (iflags.msg_history < kMinMessageHistoryLength)
        iflags.msg_history = kMinMessageHistoryLength;
    else if (iflags.msg_history > kMaxMessageHistoryLength)
        iflags.msg_history = kMaxMessageHistoryLength;

    m_rows = iflags.msg_history;
    m_cols = kScreenWidth;
}

void MessageWindow::Init()
{
    m_mustBeSeen = false;
    m_mustBeErased = false;
    m_nextIsPrompt = false;

    m_msgIter = m_msgList.end();

    m_stop = false;
}

MessageWindow::~MessageWindow()
{
}

void MessageWindow::Destroy()
{
    CoreWindow::Destroy();

    m_msgList.clear();
    m_msgIter = m_msgList.end();
}

void MessageWindow::Clear()
{
    if (m_mustBeErased) {
        erase_message();
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
        Clear();
    }

    m_curx = 0;
    m_cury = 0;

    m_active = 1;
}

void MessageWindow::Dismiss()
{
    if (m_mustBeSeen)
        Display(true);

    CoreWindow::Dismiss();
    m_stop = false;
}

void MessageWindow::Putstr(int attr, const char *str)
{
    /* TODO(bhouse) could we get non-zero attr? */
    assert(attr == 0);

    std::string input = std::string(str);

    char *tl, *otl;
    int n0;
    int notdied = 1;

    const char * bp = input.c_str();

    /* If there is room on the line, print message on same line */
    /* But messages like "You die..." deserve their own line */
    n0 = strlen(bp);
    if ((m_mustBeSeen || m_stop) && m_cury == 0
        && n0 + (int)strlen(toplines) + 3 < CO - 8 /* room for --More-- */
        && (notdied = strncmp(bp, "You die", 7)) != 0) {
        strncat(toplines, "  ", sizeof(toplines) - strlen(toplines) - 1);
        strncat(toplines, bp, sizeof(toplines) - strlen(toplines) - 1);
        if (!m_stop) {
            m_curx += 2;
            addtopl(bp);
        }
        return;
    } else if (!m_stop) {
        if (m_mustBeSeen) {
            more();
        } else if (m_cury) {
            erase_message();
            m_curx = 0;
            m_cury = 0;
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
            tl = index(otl, kSpace);
            if (!tl)
                break; /* No choice but to spit it out whole. */
        }
        *tl++ = kNewline;
        n0 = strlen(tl);
    }
    if (!notdied)
        m_stop = false;
    if (!m_stop)
        redotoplin(toplines);
}

void MessageWindow::removetopl(int n)
{
    while (n-- > 0)
        putsyms("\b \b", TextColor::NoColor, TextAttribute::None);
}

/* set the topline message with the given string.
 * dismiss_more is an additional character that can be used to dismiss
 * a more prompt.
 */
int MessageWindow::redotoplin(const char *str, int dismiss_more)
{
    m_curx = 0;
    m_cury = 0;

    putsyms(str, TextColor::NoColor, TextAttribute::None);
    clear_to_end_of_line();
    m_mustBeSeen = true;
    m_mustBeErased = true;

    int response = 0;

    if (m_nextIsPrompt) {
        m_nextIsPrompt = false;
    } else if (m_cury != 0)
        response = more(dismiss_more);

    return response;
}

int MessageWindow::display_message_history(bool reverse)
{
    winid prevmsg_win;
    int response = 0;

    prevmsg_win = create_nhwindow(NHW_MENU);
    MenuWindow * menuWin = (MenuWindow *)g_wins[prevmsg_win];
    menuWin->Putstr(0, "Message History");
    menuWin->Putstr(0, "");

    if (!reverse) {
        m_msgIter = m_msgList.end();

        for (auto & msg : m_msgList)
            menuWin->Putstr(0, msg.c_str());

        menuWin->Putstr(0, toplines);
    } else {
        menuWin->Putstr(0, toplines);

        auto iter = m_msgList.end();
        while (iter != m_msgList.begin())
        {
            iter--;
            menuWin->Putstr(0, iter->c_str());
        }
    }

    menuWin->Display(true);
    if (menuWin->m_cancelled) response = kEscape;
    menuWin->Destroy();

    m_msgIter = m_msgList.end();

    return response;
}

int MessageWindow::doprev_message()
{
    int response = 0;

    if (iflags.prevmsg_window == 'f') { /* full */
        response = display_message_history();
    } else if (iflags.prevmsg_window == 'c') { /* combination */
        do {
            if (m_msgIter == m_msgList.end()) {
                response = redotoplin(toplines, C('p'));

                if (m_msgIter != m_msgList.begin())
                    m_msgIter--;

            } else {
                auto iter = m_msgIter;
                iter++;

                if (iter == m_msgList.end()) {
                    iter--;
                    response = redotoplin(iter->c_str(), C('p'));

                    if (m_msgIter != m_msgList.begin())
                        m_msgIter--;
                    else
                        m_msgIter = m_msgList.end();

                } else {
                    response = display_message_history();
                }
            }

        } while (response == C('p'));
    } else if (iflags.prevmsg_window == 'r') { /* reversed */
        response = display_message_history(true);
    } else if (iflags.prevmsg_window == 's') { /* single */
        do {
            if (m_msgIter == m_msgList.end()) {
                response = redotoplin(toplines, C('p'));
            } else {
                response = redotoplin(m_msgIter->c_str(), C('p'));
            }

            if (m_msgIter != m_msgList.begin())
                m_msgIter--;
            else
                m_msgIter = m_msgList.end();

        } while (response == C('p'));
    }

    return response;
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
        if ((rb = index(respbuf, kEscape)) != 0)
            *rb = kNull;
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
        if (q == kControlP) { /* ctrl-P */
            if (iflags.prevmsg_window != 's') {
                doprev_message();
                Clear();
                m_msgIter = m_msgList.end();
                addtopl(prompt);
            } else {
                if (!doprev)
                    doprev_message(); /* need two initially */
                doprev_message();
                doprev = 1;
            }
            q = '\0'; /* force another loop iteration */
            continue;
        } else if (doprev) {
            /* BUG[?]: this probably ought to check whether the
            character which has just been read is an acceptable
            response; if so, skip the reprompt and use it. */
            Clear();
            m_msgIter = m_msgList.end();
            doprev = 0;
            addtopl(prompt);
            q = '\0'; /* force another loop iteration */
            continue;
        }
        digit_ok = allow_num && digit(q);
        if (q == kEscape) {
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
                    if (z == kEscape)
                        value = -1; /* abort */
                    z = kNewline;       /* break */
                } else if (z == kBackspace) {
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
            } while (z != kNewline);
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
        Clear();

    return q;
}

int MessageWindow::handle_prev_message()
{
    int c = kControlP;

    if (m_msgList.size() > 0) {
        m_msgIter = m_msgList.end();
        m_msgIter--;
        do {
            c = doprev_message();
            if (m_msgIter == m_msgList.end() || c == kEscape)
                break;
            c = pgetchar();
        } while (c == kControlP);
    }

    m_msgIter = m_msgList.end();

    return c;
}

void MessageWindow::hooked_tty_getlin(const char *query, char *bufp, getlin_hook_proc hook)
{
    char *obufp = bufp;
    int c;
    std::string prompt = std::string(query);
    std::string input;
    std::string guess;
    std::string line;

    if (m_mustBeSeen && !m_stop)
        more();

    /* getting input ... new messages should be seen (stop == false) */
    m_stop = false;

    m_nextIsPrompt = true;

    prompt += ' ';

    Putstr(0, prompt.c_str());

    assert(!m_nextIsPrompt);

    for (;;) {

        line = std::string(prompt);
        line += input;
        line += guess;

        strncpy(toplines, line.c_str(), sizeof(toplines) - 1);

        set_cursor(0, 0);
        core_puts(line.c_str());
        clear_to_end_of_line();
        set_cursor(prompt.size() + input.size(), 0);
        m_mustBeErased = true;
        m_mustBeSeen = true;

        c = pgetchar();

        if (c == kControlP) {
            guess.clear();
            c = handle_prev_message();
        }

        if (c == kEscape && input.size() == 0) {
            input = std::string("\033");
            toplines[0] = kNull;
            break;
        }

        if (c == kEscape || c == kKillChar || c == kDelete) {
            guess.clear();
            input.clear();
        }

        if (c == kBackspace) {
            if (input.size() > 0) {
                input = input.substr(0, input.size() - 1);
                guess.clear();
            } else
                tty_nhbell();
        } else if (c == kNewline) {
            break;
        } else if (kSpace <= (unsigned char)c && c != kDelete
            && (input.size() < BUFSZ - 1 && input.size() < COLNO)) {

            input += c;
            guess.clear();
            strcpy(obufp, input.c_str());

            if (hook && (*hook)(obufp))
                guess = std::string(obufp + input.size());

        } else
            tty_nhbell();
    }

    strcpy(obufp, input.c_str());
    strcat(obufp, guess.c_str());

    assert(!m_mustBeSeen);
    Clear();

    /* toplines has the result of the getlin */
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
    *toplines = kNull;
    m_msgIter = m_msgList.end();
}

void MessageWindow::topl_putsym(char c, TextColor color, TextAttribute attribute)
{
    switch (c) {
    case kBackspace:
        core_putc(kBackspace);
        return;
    case kNewline:
        clear_to_end_of_line();
        core_putc(kNewline);
        break;
    default:
        if (m_curx + m_offx == CO - 1)
            topl_putsym(kNewline, TextColor::NoColor, TextAttribute::None); /* 1 <= curx < CO; avoid CO */
        core_putc(c);
    }

    if (m_curx == 0)
        clear_to_end_of_line();
}

void MessageWindow::addtopl(const char *s)
{
    putsyms(s, TextColor::NoColor, TextAttribute::None);
    clear_to_end_of_line();
    m_mustBeSeen = true;
    m_mustBeErased = true;
}

int MessageWindow::more(int dismiss_more)
{
    assert(!m_nextIsPrompt);

    if (m_mustBeErased) {
        set_cursor(m_curx, m_cury);
        if (m_curx >= CO - 8)
            topl_putsym(kNewline, TextColor::NoColor, TextAttribute::None);
    }

    putsyms(defmorestr, TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);

    int response = wait_for_response("\033 ", dismiss_more);

    /* if the message is more then one line or message was cancelled then erase the entire message. 
     * otherwise we leave the message visible.
     */
    if ((m_mustBeErased && m_cury != 0) || response == kEscape)
        erase_message();

    /* message has been seen and confirmed */
    m_mustBeSeen = false;

    /* note: if we are stopping messages there are no messages to be seen
     *       or messages that need erasing.
     */
    if (response == kEscape)
        m_stop = true;

    return response;
}

/* special hack for treating top line --More-- as a one item menu */
char MessageWindow::uwp_message_menu(char let, int how, const char *mesg)
{
    /* "menu" without selection; use ordinary pline, no more() */
    if (how == PICK_NONE) {
        pline("%s", mesg);
        return 0;
    }

    int response = 0;

    /* we set m_nextIsPrompt to ensure that we don't post a "more" prompt
       and getting a response within putstr handling */
    m_nextIsPrompt = true;
    g_messageWindow.Putstr(0, mesg);

    if (m_mustBeSeen) {
        response = more(let);
        assert(!m_mustBeSeen);

        if (m_mustBeErased)
            Clear();
    }
    /* normally <ESC> means skip further messages, but in this case
    it means cancel the current prompt; any other messages should
    continue to be output normally */
    m_stop = false;

    return ((how == PICK_ONE && response == let) || response == '\033') ? response : '\0';
}

void MessageWindow::erase_message()
{
    int ymax = m_cury + 1;

    for (int y = 0; y < ymax; y++) {
        set_cursor(0, y);
        clear_to_end_of_line();
        if (y < g_mapWindow.m_offy || y > ROWNO)
            continue; /* only refresh board  */
        row_refresh(0 - g_mapWindow.m_offx, COLNO - 1, y - g_mapWindow.m_offy);
    }

    if (ymax >= (int)g_statusWindow.m_offy) {
        /* we have wrecked the bottom line */
        context.botlx = 1;
        bot();
    }

    set_cursor(0, 0);
    m_mustBeErased = false;
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
        for (auto & msg : m_snapshot_msgList) {
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

