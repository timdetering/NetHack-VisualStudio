/* NetHack 3.6	uwp_message_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

/*
    NOTES:

    Toplines is the input buffer for the message window.

    Therea are three ways for m_toplines to get input.
    1. Putstr
    2. yn_function
    3. uwp_getmessagehistory
    

*/

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

    m_cols = kScreenWidth;

    cells_set_dimensions(kScreenWidth, kScreenHeight);
}

void MessageWindow::Init()
{
    m_mustBeSeen = false;
    m_mustBeErased = false;

    m_msgList.clear();
    m_msgIter = m_msgList.end();
    m_toplines.clear();

    m_outputMessages = true;

    m_rows = 1;
}

MessageWindow::~MessageWindow()
{
}

void MessageWindow::PrepareForInput()
{
    /* we are asking for input so lets start displaying messages again. */
    m_outputMessages = true;

    /* any messages shown are about to be seen. */
    if (m_mustBeSeen) {
        m_mustBeSeen = false;
        /* Note: m_cury can be > 0 here */
    }

}

void MessageWindow::Destroy()
{
    CoreWindow::Destroy();

    m_msgList.clear();
    m_msgIter = m_msgList.end();
}

void MessageWindow::Clear()
{
    if (m_mustBeErased)
        erase_message();

    assert(m_rows == 1);
    m_curx = 0;
    m_cury = 0;
}

void MessageWindow::Display(bool blocking)
{
    if (!m_outputMessages)
        return;

    g_rawprint = 0;

    if (m_mustBeSeen) {
        more();

        if (m_mustBeErased)
            erase_message();

        assert(!m_mustBeSeen);
    }

    m_curx = 0;
    m_cury = 0;

    m_active = 1;
}

void MessageWindow::Dismiss()
{
    if (m_mustBeSeen) {
        Display(true);
        m_mustBeSeen = false;
        assert(m_cury == 0);
    }

    CoreWindow::Dismiss();
    m_outputMessages = true;
}

void MessageWindow::Putstr(int attr, const char *str)
{
    Putstr(attr, str, false);
}

void MessageWindow::Putstr(int attr, const char *str, bool isPrompt)
{
    /* TODO(bhouse) could we get non-zero attr? */
    assert(attr == 0);

    std::string input = std::string(str);

    char *tl, *otl;
    int n0;

    const char * bp = input.c_str();

    bool died = (input.compare(0, 7, "You die") == 0);

    /* If there is room on the line, print message on same line */
    /* But messages like "You die..." deserve their own line */
    n0 = input.size();
    if ((m_mustBeSeen || !m_outputMessages) && m_cury == 0
        && n0 + m_toplines.size() + 3 < CO - 8 /* room for --More-- */
        && !died) {

        /* why don't we just check that m_toplines is non-empty?
           Can m_toplines be non-empty and !m_mustBeSeen && m_outputMessages
        */

        m_toplines += "  ";
        m_toplines += bp;

        if (m_outputMessages) {
            addtopl("  ");
            addtopl(bp);
        }

        return;
    }
    
    if (m_mustBeSeen) {
        assert(m_outputMessages);
        more();
    }

    assert(m_cury == 0);
    assert(!m_mustBeSeen);

    remember_topl();

    int offset = 0;
    while (input.size() - offset > CO) {
        size_t space_offset = input.find_last_of(' ', offset + CO);

        if (space_offset <= offset || space_offset == std::string::npos) {
            offset += CO;
        } else {
            offset = space_offset;
            input[offset] = '\n';
        }
    }

    m_toplines = input;

    if (died)
        m_outputMessages = true;

    if (m_outputMessages)
        redotoplin(input.c_str(), 0, isPrompt);
}

void MessageWindow::put_topline(const char *str)
{
    assert(!m_mustBeSeen);

    if (m_mustBeErased)
        erase_message();

    m_curx = 0;
    m_cury = 0;
    m_output.clear();

    putsyms(str, TextColor::NoColor, TextAttribute::None);

    m_mustBeSeen = true;
    m_mustBeErased = true;
}

/* set the topline message with the given string.
 * dismiss_more is an additional character that can be used to dismiss
 * a more prompt.
 */
int MessageWindow::redotoplin(const char *str, int dismiss_more, bool isPrompt)
{
    assert(!m_mustBeSeen);
    assert(m_cury == 0);
    m_curx = 0;
    m_cury = 0;
    m_output.clear();

    putsyms(str, TextColor::NoColor, TextAttribute::None);

    m_mustBeSeen = true;
    m_mustBeErased = true;

    int response = 0;

    if (!isPrompt && m_cury != 0)
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

        menuWin->Putstr(0, m_toplines.c_str());
    } else {
        menuWin->Putstr(0, m_toplines.c_str());

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
    assert(!m_mustBeSeen);

    int response = 0;

    if (iflags.prevmsg_window == 'f') { /* full */
        response = display_message_history();
    } else if (iflags.prevmsg_window == 'c') { /* combination */
        do {
            if (m_msgIter == m_msgList.end()) {
                response = redotoplin(m_toplines.c_str(), kControlP);

                if (m_msgIter != m_msgList.begin())
                    m_msgIter--;

            } else {
                auto iter = m_msgIter;
                iter++;

                if (iter == m_msgList.end()) {
                    iter--;
                    response = redotoplin(iter->c_str(), kControlP);

                    if (m_msgIter != m_msgList.begin())
                        m_msgIter--;
                    else
                        m_msgIter = m_msgList.end();

                } else {
                    response = display_message_history();
                }
            }

        } while (response == kControlP);
    } else if (iflags.prevmsg_window == 'r') { /* reversed */
        response = display_message_history(true);
    } else if (iflags.prevmsg_window == 's') { /* single */
        do {
            if (m_msgIter == m_msgList.end()) {
                response = redotoplin(m_toplines.c_str(), kControlP);
            } else {
                response = redotoplin(m_msgIter->c_str(), kControlP);
            }

            if (m_msgIter != m_msgList.begin())
                m_msgIter--;
            else
                m_msgIter = m_msgList.end();

        } while (response == kControlP);
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
    std::string input;
    char q;
    boolean doprev = 0;
    char *tl;
    bool preserve_case = false;
    bool allow_num = false;

    g_focus_stack.push_front(&g_messageWindow);

    if (m_mustBeSeen) {
        assert(m_outputMessages);
        more();
    }

    assert(!m_mustBeSeen);

    remember_topl();

    m_outputMessages = true;

    std::string prompt = query;
    prompt += ' ';

    if (resp) {

        std::string response = resp;

        allow_num = (response.find_first_of('#') != std::string::npos);
        preserve_case = (response.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != std::string::npos);

        std::string displayed_response(resp);

        auto offset = displayed_response.find_first_of(kEscape);
        if (offset != std::string::npos)
            displayed_response = displayed_response.substr(0, offset);

        prompt += "[";
        prompt += displayed_response;
        prompt += ']';

        if (def) {
            prompt += " (";
            prompt += def;
            prompt += ')';
        }

        prompt += ' ';

        do { /* loop until we get valid input */

            input.clear();
            m_toplines = prompt;
            assert(m_toplines.compare(prompt) == 0);

            put_topline(m_toplines.c_str());

            q = readchar();

            if (q == kControlP) {
                q = handle_prev_message();
                Clear();
                if (q == kNull)
                    continue;
            }

            if (!preserve_case)
                q = lowc(q);

            if (q == kEscape) {
                if (index(resp, 'q'))
                    q = 'q';
                else if (index(resp, 'n'))
                    q = 'n';
                else
                    q = def;
                break;
            }
            
            if (index(quitchars, q)) {
                q = def;
                break;
            }

            bool digit_ok = allow_num && digit(q);

            if (!index(resp, q) && !digit_ok) {
                tty_nhbell();
                q = kNull;
                continue;
            }
            
            char z;

            if (q == '#' || digit_ok) {
                long value;

                input += '#';

                if (q != '#')
                    input += q;

                do { /* loop until we get a non-digit */
                    value = atoi(input.c_str() + 1);

                    m_toplines = prompt;
                    m_toplines += input;

                    put_topline(m_toplines.c_str());

                    z = readchar();

                    if (!preserve_case)
                        z = lowc(z);

                    if (digit(z)) {
                        input += z;

                        value = atoi(input.c_str() + 1);

                        if (value == INT_MAX) {
                            value = -1;
                            break;
                        }
                    } else if (z == 'y' || index(quitchars, z)) {
                        if (z == kEscape)
                            value = -1; /* abort */
                        z = kNewline;       /* break */
                    } else if (z == kBackspace) {
                        if (input.size() <= 1) {
                            value = -1;
                            break;
                        } else {
                            input = input.substr(0, input.size() - 1);
                        }
                    } else {
                        value = -1; /* abort */
                        tty_nhbell();
                        break;
                    }
                } while (z != kNewline);

                if (value > 0) {
                    yn_number = value;
                    q = '#';
                    break;
                } else if (value == 0) {
                    q = 'n'; /* 0 => "no" */
                    break;
                } else {
                    q = kNull;
                    continue;
                }
            }

        } while (!q);

        if (q != '#') {
            input += q;
        }

    } else {

        m_toplines = prompt;

        put_topline(m_toplines.c_str());

        q = readchar();

        input += q;
    }

    m_toplines = prompt;
    m_toplines += input;

    put_topline(m_toplines.c_str());

    /* user has seen this message */
    m_mustBeSeen = false;

    /* clear message if we are bleeding into map */
    if (m_cury)
        Clear();

    /* output can not be disabled via yn_function */
    assert(m_outputMessages);
    assert(m_cury == 0);

    g_focus_stack.pop_front();

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

            if (m_msgIter == m_msgList.end()) {
                c = kNull;
                break;
            }

            if (c == kNull)
                c = pgetchar();

            if (c == kSpace || c == kEscape || c == kNewline)
                c = kNull;

        } while (c == kControlP);
    }

    m_msgIter = m_msgList.end();

    return c;
}

void MessageWindow::Prompt(const char * prompt)
{
    assert(m_outputMessages);
    assert(!m_mustBeSeen);
    Putstr(0, prompt, true);
}

void MessageWindow::hooked_tty_getlin(const char *query, char *bufp, getlin_hook_proc hook, int bufSize)
{
    char *obufp = bufp;
    int c;
    std::string prompt = std::string(query);
    std::string input;
    std::string guess;
    std::string line;

    g_focus_stack.push_front(&g_messageWindow);

    if (m_mustBeSeen) {
        assert(m_outputMessages);
        more();
        assert(!m_mustBeSeen);
    }

    /* getting input ... new messages should be seen (stop == false) */
    m_outputMessages = true;

    prompt += ' ';

    Prompt(prompt.c_str());

    for (;;) {

        line = std::string(prompt);
        line += input;
        line += guess;

        m_toplines = line;

        m_mustBeErased = true;
        m_mustBeSeen = false;
        erase_message();
        assert(m_cury == 0);

        // Should we let rows increase to some maximum?
        m_rows = 3;
        putsyms(line.c_str(), TextColor::NoColor, TextAttribute::None);
        m_rows = m_cury + 1;

        if (guess.size())
            set_cursor(prompt.size() + input.size(), 0);

        m_mustBeErased = true;
        m_mustBeSeen = true;

        c = pgetchar();

        if (c == kControlP)
            c = handle_prev_message();

        if (c == kEscape) {
            input = std::string("\033");
            m_toplines.clear();
            break;
        }

        if (c == kKillChar || c == kDelete) {
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
            && (input.size() < bufSize - 1 && input.size() < COLNO)) {

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

    g_focus_stack.pop_front();

    /* m_toplines has the result of the getlin */
}

void
MessageWindow::putsyms(const char *str, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute)
{
    while (*str)
        topl_putsym(*str++, textColor, textAttribute);
}

void MessageWindow::remember_topl()
{
    if (m_toplines.size() == 0)
        return;

    m_msgList.push_back(m_toplines);
    while (m_msgList.size() > iflags.msg_history)
        m_msgList.pop_front();

    m_toplines.clear();
    m_msgIter = m_msgList.end();
}

void MessageWindow::topl_putsym(char c, TextColor color, TextAttribute attribute)
{
    int curx = m_curx;
    int cury = m_cury;

    switch (c) {
    case kBackspace:
        m_output.pop_back();

        if (m_curx > 0) {
            m_curx--;
        } else if (m_cury > 0) {
            m_cury--;
            m_curx = m_cols - 1;
        }
        break;
    case kNewline:
        assert(m_cury < m_rows);
        // TODO: come up with a better way of keeping m_rows in range
        if (m_cury == (m_rows - 1)) m_rows = m_cury + 2;
        m_curx = m_cols;
        m_output.push_back(TextCell(kNewline));
        break;
    default:
        {
        bool forceNewLine = (m_curx + m_offx == CO - 1);

        // TODO: come up with a better way of keeping m_rows in range
        if (forceNewLine) m_rows = m_cury + 2;
        m_curx++;
        m_output.push_back(TextCell(c));
        
        break;
        }
    }

    if (m_curx == m_cols) {
        m_curx = 0;
        m_cury++;
    }

    if (cury == m_rows) {
        m_curx = m_cols - 1;
        m_cury = m_rows - 1;
    }

}

void MessageWindow::addtopl(const char *s)
{
    putsyms(s, TextColor::NoColor, TextAttribute::None);

    m_mustBeSeen = true;
    m_mustBeErased = true;
}

void MessageWindow::addtopl(char c)
{
    topl_putsym(c, TextColor::NoColor, TextAttribute::None);

    m_mustBeSeen = true;
    m_mustBeErased = true;
}

int MessageWindow::more(int dismiss_more)
{
    if (m_mustBeErased) {
        if (m_curx >= CO - 8)
            topl_putsym(kNewline, TextColor::NoColor, TextAttribute::None);
    }

    putsyms(defmorestr, TextColor::NoColor, flags.standout ? TextAttribute::Bold : TextAttribute::None);

    /* message will be seen and confirmed */
    m_mustBeSeen = false;

    /* we need to erase the more message */
    m_mustBeErased = true;

    int response = wait_for_response("\033 ", dismiss_more);

    /* if the message is more then one line or message was cancelled then erase the entire message. 
     * otherwise we leave the message visible.
     */
    if (m_cury != 0 || response == kEscape)
        erase_message();

    assert(m_cury == 0);


    /* note: if we are stopping messages there are no messages to be seen
     *       or messages that need erasing.
     */
    if (response == kEscape) {
        assert(!m_mustBeSeen);
        assert(!m_mustBeErased);
        assert(m_output.size() == 0);
        assert(m_cury == 0);
        assert(m_curx == 0);
        assert(m_rows == 1);
        m_outputMessages = false;
    }

    return response;
}

/* special hack for treating top line --More-- as a one item menu */
char MessageWindow::uwp_message_menu(char let, int how, const char *mesg)
{
    assert(m_outputMessages);

    /* not sure if this assertion holds for all scenarios */
    assert(!m_mustBeSeen);

    /* "menu" without selection; use ordinary pline, no more() */
    if (how == PICK_NONE) {
        pline("%s", mesg);
        return 0;
    }

    int response = 0;

    Prompt(mesg);

    assert(m_mustBeSeen);

    response = more(let);
    assert(!m_mustBeSeen);

    assert(m_cury == 0);
    if (m_mustBeErased)
        Clear();

    /* normally <ESC> means skip further messages, but in this case
    it means cancel the current prompt; any other messages should
    continue to be output normally */
    m_outputMessages = true;

    return ((how == PICK_ONE && response == let) || response == '\033') ? response : '\0';
}

void MessageWindow::erase_message()
{
    assert(m_mustBeErased);
    assert(!m_mustBeSeen);

    m_curx = 0;
    m_cury = 0;
    m_mustBeErased = false;

    m_output.clear();
    m_rows = 1;
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
        m_toplines = msg;
    } else {

        assert(initd);
        for (auto & msg : m_snapshot_msgList) {
            remember_topl();
            m_toplines = msg;
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
    /* flush m_toplines, moving most recent message to history */
    remember_topl();

    m_snapshot_msgList = m_msgList;

    /* for a destructive snapshot, history is now completely empty */
    if (purge) {
        m_msgList.clear();
        m_msgIter = m_msgList.end();
    }
}

void MessageWindow::Render(std::vector<Nethack::TextCell> & cells)
{
    int x = 0;

    int offset = 0;
    for (auto cell : m_output) {
        if (cell.m_char == kNewline) {
            while (x < kScreenWidth) {
                m_cells[offset++] = TextCell();
                x++;
            }
        } else {
            m_cells[offset++] = cell;
            x++;
        }

        x %= kScreenWidth;
    }

    while (x < kScreenWidth) {
        m_cells[offset++] = TextCell();
        x++;
    }

    CoreWindow::Render(cells);
}

