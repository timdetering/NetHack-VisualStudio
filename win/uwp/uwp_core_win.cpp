/* NetHack 3.6	uwp_text_win.cpp	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#include "..\..\sys\uwp\uwp.h"
#include "winuwp.h"

using namespace Nethack;

CoreWindow::CoreWindow(int inType, winid window) : m_type(inType), m_window(window)
{
    g_wins[m_window] = this;

    m_dimx = 0;
    m_dimy = 0;
    m_cell_count = 0;

    Init();
}

void CoreWindow::Init()
{
    m_active = FALSE;
    m_curx = 0;
    m_cury = 0;

    for (auto & cell : m_cells)
        cell = TextCell();
}

CoreWindow::~CoreWindow()
{
    g_wins[m_window] = NULL;
}

void CoreWindow::Render(std::vector<TextCell> & cells)
{
    if (m_active) {
        assert(m_cols + m_offx <= kScreenWidth);
        assert(m_cols <= m_dimx);
        assert(m_rows <= m_dimy);

        for (int y = 0; y < m_rows; y++) {
            int dst_offset = ((m_offy + y) * kScreenWidth) + m_offx;
            int src_offset = y * m_dimx;
            for (int x = 0; x < m_cols; x++)
                cells[dst_offset++] = m_cells[src_offset++];
        }
    }
}

void CoreWindow::Destroy()
{
    if (m_active)
        tty_dismiss_nhwindow(m_window);
}

void CoreWindow::set_cursor(int x, int y)
{
    m_curx = x;
    m_cury = y;

    assert(x >= 0 && x < m_cols);
    assert(y >= 0 && y < m_rows);

    x += m_offx;
    y += m_offy;

    g_textGrid.SetCursor(Int2D(x, y));
}

void CoreWindow::clear_to_end_of_line()
{
    assert((m_offx + m_cols) == kScreenWidth);
    g_textGrid.SetCursor(Int2D(m_curx + m_offx, m_cury + m_offy));
    g_textGrid.ClearToEndOfLine();
    cells_clear_to_end_of_line(m_curx, m_cury);
}

void CoreWindow::clear_to_end_of_screen()
{
    assert(m_rows == kScreenHeight);
    g_textGrid.SetCursor(Int2D(m_curx + m_offx, m_cury + m_offy));
    g_textGrid.ClearToEndOfScreen();
    cells_clear_to_end_of_window(m_curx, m_cury);
}

void CoreWindow::clear_whole_screen()
{
    g_textGrid.Clear();
    assert(m_rows == kScreenHeight);
    assert(m_cols == kScreenWidth);
    cells_clear_window();
}

void CoreWindow::clear_window()
{
    m_curx = 0;
    for (m_cury = 0; m_cury < m_rows; m_cury++)
        clear_to_end_of_line();
    m_cury = 0;

    cells_clear_window();
}

void CoreWindow::Dismiss()
{
    m_active = 0;
}

void CoreWindow::Clear()
{
    m_curx = 0;
    m_cury = 0;
}

int
CoreWindow::wait_for_response(
    const char *s,
    int dismiss_more) /* chars allowed besides return */
{
    int c;
    int response = 0;

    while (
#ifdef HANGUPHANDLING
        !program_state.done_hup &&
#endif
        1) {

        c = tty_nhgetch();

        if (c == kNewline)
            break;

        if (c == kEscape) {
            response = kEscape;
            break;
        }

        if ((s && index(s, c)) || c == dismiss_more) {
            response = (char)c;
            break;
        }

        tty_nhbell();
    }

    return response;
}

void CoreWindow::core_putc(char ch, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute)
{
    assert(m_cols >= 0 && m_cols <= m_dimx);
    assert(m_rows >= 0 && m_rows <= m_dimy);

    assert(m_curx >= 0 && m_curx < m_cols);
    assert(m_cury >= 0 && m_cury < m_rows);

    int x = m_curx + m_offx;
    int y = m_cury + m_offy;

    cells_put(m_curx, m_cury, ch, textColor, textAttribute);
    g_textGrid.Put(x, y, ch, textColor, textAttribute);

    assert(m_curx <= m_cols);
    assert(m_cury < m_rows);
}

void CoreWindow::core_puts(
    const char *s,
    TextColor textColor,
    TextAttribute textAttribute)
{
    while (*s)
        core_putc(*s++, textColor, textAttribute);
}

const char *
CoreWindow::compress_str(const char * str)
{
    static char cbuf[BUFSZ];

    /* compress out consecutive spaces if line is too long;
    topline wrapping converts space at wrap point into newline,
    we reverse that here */
    if ((int)strlen(str) >= CO || index(str, kNewline)) {
        const char *in_str = str;
        char c, *outstr = cbuf, *outend = &cbuf[sizeof cbuf - 1];
        boolean was_space = TRUE; /* True discards all leading spaces;
                                  False would retain one if present */

        while ((c = *in_str++) != kNull && outstr < outend) {
            if (c == kNewline)
                c = kSpace;
            if (was_space && c == kSpace)
                continue;
            *outstr++ = c;
            was_space = (c == kSpace);
        }
        if ((was_space && outstr > cbuf) || outstr == outend)
            --outstr; /* remove trailing space or make room for terminator */
        *outstr = kNull;
        str = cbuf;
    }
    return str;
}

void CoreWindow::Putsym(int x, int y, char ch)
{

    switch (m_type) {
    case NHW_STATUS:
    case NHW_MAP:
    case NHW_BASE:
        set_cursor(x - 1, y);
        core_putc(ch);
        break;
    case NHW_MESSAGE:
    case NHW_MENU:
    case NHW_TEXT:
        impossible("Can't putsym to window type %d", m_type);
        break;
    }
}

void CoreWindow::cells_set_dimensions(int x, int y)
{
    m_dimx = x;
    m_dimy = y;
    m_cell_count = m_dimx * m_dimy;
    m_cells.resize(m_cell_count);
}

void CoreWindow::cells_clear_to_end_of_window(int x, int y)
{
    assert(m_cols >= 0 && m_cols <= m_dimx);
    assert(m_rows >= 0 && m_rows <= m_dimy);

    assert(x >= 0 && x < m_cols);
    assert(y >= 0 && y < m_rows);

    TextCell clearCell;
    int offset = (y * m_dimx) + x;

    while (offset < m_cell_count)
        m_cells[offset++] = clearCell;
}

void CoreWindow::cells_clear_to_end_of_line(int x, int y)
{
    assert(m_cols >= 0 && m_cols <= m_dimx);
    assert(m_rows >= 0 && m_rows <= m_dimy);

    assert(x >= 0 && x < m_cols);
    assert(y >= 0 && y < m_rows);

    TextCell clearCell;
    int offset = (y * m_dimx) + x;
    int sentinel = (y + 1) * m_dimx;

    while (offset < sentinel)
        m_cells[offset++] = clearCell;
}

void CoreWindow::cells_clear_window()
{
    TextCell clearCell;
    int offset = 0;
    int sentinel = m_dimy * m_dimx;

    while (offset < sentinel)
        m_cells[offset++] = clearCell;
}

void CoreWindow::cells_puts(int x, int y, const char * str, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute)
{
    while (*str)
        cells_put(m_curx, m_cury, *str++, textColor, textAttribute);
}

void CoreWindow::cells_put(int x, int y, const char c, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute)
{
    assert(m_cols >= 0 && m_cols <= m_dimx);
    assert(m_rows >= 0 && m_rows <= m_dimy);

    assert(x >= 0 && x < m_cols);
    assert(y >= 0 && y < m_rows);

    m_curx = x;
    m_cury = y;

    int offset = (m_cury * m_dimx) + m_curx;

    assert(offset >= 0 && offset < m_cell_count);

    if (c == '\b') {
        if (m_curx > 0) {
            m_curx--;
       } else if (m_cury > 0) {
            m_cury--; m_curx = m_cols - 1;
        }
    } else if (c == '\n') {
        m_curx = m_cols;
    } else {
        int offset = (m_cury * m_dimx) + m_curx;
        m_cells[offset] = TextCell(textColor, textAttribute, c);
        m_curx++;
    }

    if (m_curx == m_cols) {
        m_curx = 0;
        m_cury++;
    }

    if (m_cury == m_rows) {
        m_curx = m_cols - 1;
        m_cury = m_rows - 1;
    }

}

void CoreWindow::bail(const char *mesg)
{
    clearlocks();
    tty_exit_nhwindows(mesg);
    terminate(EXIT_SUCCESS);
    /*NOTREACHED*/
}
