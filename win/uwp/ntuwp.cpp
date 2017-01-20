
#include "..\..\sys\uwp\uwp.h"
#include "..\..\sys\uwp\uwptextgrid.h"
#include "..\..\sys\uwp\uwpglobals.h"

extern "C" {

#include "hack.h"

#ifdef UWP_GRAPHICS

#include "winuwp.h"

struct uwp_console_t {
    WORD background;
    WORD foreground;
    WORD attr;
    int current_nhcolor;
    WORD current_nhattr;
    COORD cursor;
};

struct uwp_console_t uwp_console = {
    0,
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED),
    NO_COLOR,
    0,
    { 0, 0 }
};

void uwp_really_move_cursor()
{

    if (uwpDisplay) {
        uwp_console.cursor.X = uwpDisplay->curx;
        uwp_console.cursor.Y = uwpDisplay->cury;
    }

    Nethack::g_textGrid.SetCursor(Nethack::Int2D(uwp_console.cursor.X, uwp_console.cursor.Y));

}

void uwp_getreturn(const char * str)
{
    uwp_msmsg("Hit <Enter> %s.", str);

    while (pgetchar() != '\n')
        ;
    return;
}

/* this is used as a printf() replacement when the window
* system isn't initialized yet
*/
void uwp_msmsg
    VA_DECL(const char *, fmt)
{
    char buf[ROWNO * COLNO]; /* worst case scenario */
    VA_START(fmt);
    VA_INIT(fmt, const char *);
    Vsprintf(buf, fmt, VA_ARGS);

    uwp_xputs(buf);

    if (uwpDisplay)
        curs(UWP_BASE_WINDOW, uwp_console.cursor.X + 1, uwp_console.cursor.Y);

    VA_END();
    return;
}

void uwp_number_pad(int state)
{
    // do nothing for now
}

void
    uwp_startup(int * wid, int * hgt)
{
    *wid = Nethack::g_textGrid.GetDimensions().m_x;
    *hgt = Nethack::g_textGrid.GetDimensions().m_y;

    set_option_mod_status("mouse_support", SET_IN_GAME);
}

void
    uwp_start_screen()
{
    if (iflags.num_pad)
        uwp_number_pad(1); /* make keypad send digits */
}

void
    uwp_home()
{
    uwp_console.cursor.X = uwp_console.cursor.Y = 0;
    uwpDisplay->curx = uwpDisplay->cury = 0;
}

void
    uwp_end_screen()
{
    uwp_clear_screen();
    uwp_really_move_cursor();
}

void
    uwp_cmov(int x, int y)
{
    uwpDisplay->cury = y;
    uwpDisplay->curx = x;
    uwp_console.cursor.X = x;
    uwp_console.cursor.Y = y;
}

void
    uwp_nocmov(int x, int y)
{
    uwp_console.cursor.X = x;
    uwp_console.cursor.Y = y;
    uwpDisplay->curx = x;
    uwpDisplay->cury = y;
}

void
    uwp_term_start_color(int color)
{
    uwp_console.current_nhcolor = color;
}

void
    uwp_term_end_color(void)
{
    uwp_console.current_nhcolor = NO_COLOR;
}

void
    uwp_term_end_raw_bold(void)
{
    uwp_term_end_attr(ATR_BOLD);
}

void
    uwp_term_start_raw_bold(void)
{
    uwp_term_start_attr(ATR_BOLD);
}

void
    uwp_term_start_attr(int attrib)
{
    assert(uwp_console.current_nhattr == 0);
    if (attrib != ATR_NONE && uwp_console.current_nhattr == 0)
        uwp_console.current_nhattr |= 1 << attrib;
}

void
    uwp_term_end_attr(int attrib)
{
    uwp_console.current_nhattr &= ~(1 << attrib);
    assert(uwp_console.current_nhattr == 0);
}

void
    uwp_cl_eos()
{
    int x = uwpDisplay->curx;
    int y = uwpDisplay->cury;

    int cx = Nethack::g_textGrid.GetDimensions().m_x - x;
    int cy = (Nethack::g_textGrid.GetDimensions().m_y - (y + 1)) * Nethack::g_textGrid.GetDimensions().m_x;

    Nethack::g_textGrid.Put(x, y, Nethack::TextCell(), cx + cy);

    uwp_curs(UWP_BASE_WINDOW, x + 1, y);
}

void
    uwp_cl_end()
{
    int cx;
    uwp_console.cursor.X = uwpDisplay->curx;
    uwp_console.cursor.Y = uwpDisplay->cury;
    cx = Nethack::g_textGrid.GetDimensions().m_x - uwp_console.cursor.X;

    Nethack::g_textGrid.Put(uwp_console.cursor.X, uwp_console.cursor.Y, Nethack::TextCell(), cx);

    uwp_curs(UWP_BASE_WINDOW, (int)uwpDisplay->curx + 1, (int)uwpDisplay->cury);
}

void uwp_clear_screen(void)
{
    raw_clear_screen();
    uwp_home();
}

void
    uwp_standoutbeg()
{
    uwp_term_start_attr(ATR_BOLD);
}

void
    uwp_standoutend()
{
    uwp_term_end_attr(ATR_BOLD);
}

void
    uwp_g_putch(int in_ch)
{
    uwp_console.cursor.X = uwpDisplay->curx;
    uwp_console.cursor.Y = uwpDisplay->cury;

    Nethack::TextCell textCell((Nethack::TextColor) uwp_console.current_nhcolor, (Nethack::TextAttribute) uwp_console.current_nhattr, in_ch);

    Nethack::g_textGrid.Put(uwp_console.cursor.X, uwp_console.cursor.Y, textCell, 1);
}

void
    uwp_xputc_core(char ch)
{
    switch (ch) {
    case '\n':
        uwp_console.cursor.Y++;
        /* fall through */
    case '\r':
        uwp_console.cursor.X = 0;
        break;
    case '\b':
        uwp_console.cursor.X--;
        break;
    default:

        Nethack::TextCell textCell((Nethack::TextColor) uwp_console.current_nhcolor, (Nethack::TextAttribute) uwp_console.current_nhattr, ch);

        Nethack::g_textGrid.Put(uwp_console.cursor.X, uwp_console.cursor.Y, textCell, 1);

        uwp_console.cursor.X++;
    }
}

void uwp_xputc(char ch)
{
    uwp_console.cursor.X = uwpDisplay->curx;
    uwp_console.cursor.Y = uwpDisplay->cury;
    uwp_xputc_core(ch);
    return;
}

void
    uwp_xputs(const char *s)
{
    int k;
    int slen = strlen(s);

    if (uwpDisplay) {
        uwp_console.cursor.X = uwpDisplay->curx;
        uwp_console.cursor.Y = uwpDisplay->cury;
    }

    if (s) {
        for (k = 0; k < slen && s[k]; ++k)
            uwp_xputc_core(s[k]);
    }
}

void
    uwp_backsp()
{
    uwp_console.cursor.X = uwpDisplay->curx;
    uwp_console.cursor.Y = uwpDisplay->cury;
    uwp_xputc_core('\b');
}

void
    uwp_delay_output()
{
    // Delay 50ms
    Sleep(50);
}

int
    uwp_tgetch()
{
    uwp_really_move_cursor();
    char c = raw_getchar();

    if (c == EOF)
    {
        hangup(0);
        c = '\033';
    }

    return c;
}

void
    uwp_settty(const char *s)
{
    uwp_cmov(uwpDisplay->curx, uwpDisplay->cury);
    end_screen();
    if (s)
        raw_print(s);
}

int
    uwp_ntposkey(int *x, int *y, int * mod)
{
    uwp_really_move_cursor();

    if (program_state.done_hup)
        return '\033';

    Nethack::Event e;

    while (e.m_type == Nethack::Event::Type::Undefined ||
            (e.m_type == Nethack::Event::Type::Mouse && !iflags.wc_mouse_support) ||
            (e.m_type == Nethack::Event::Type::ScanCode && MapScanCode(e) == 0))
        e = Nethack::g_eventQueue.PopFront();

    if (e.m_type == Nethack::Event::Type::Char)
    {
        if (e.m_char == EOF)
        {
            hangup(0);
            e.m_char = '\033';
        }

        return e.m_char;
    }
    else if (e.m_type == Nethack::Event::Type::Mouse)
    {
        *x = e.m_pos.m_x;
        *y = e.m_pos.m_y;
        *mod = (e.m_tap == Nethack::Event::Tap::Left ? CLICK_1 : CLICK_2);

        return 0;
    }
    else
    {
        assert(e.m_type == Nethack::Event::Type::ScanCode);
        return MapScanCode(e);
    }
}

#endif // UWP_GRAPHICS

}