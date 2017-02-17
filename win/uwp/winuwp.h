/* NetHack 3.6	winuwp.h	$NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2016-2017. */
/* Nethack for the Universal Windows Platform (UWP) */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#ifndef UWP_GRAPHICS
#error UWP_GRAPHICS must be defined
#endif

/* menu structure */
typedef struct tty_mi {
    struct tty_mi *next;
    anything identifier; /* user identifier */
    long count;          /* user count */
    char *str;           /* description string (including accelerator) */
    int attr;            /* string attribute */
    boolean selected;    /* TRUE if selected by user */
    char selector;       /* keyboard accelerator */
    char gselector;      /* group accelerator */
} tty_menu_item;

/* struct for windows */
struct CoreWindow {

    CoreWindow(int type);
    virtual ~CoreWindow();

    virtual void Clear();
    virtual void Display(bool blocking) = 0;
    virtual void Dismiss();
    void Destroy();
    void Curs(int x, int y);
    virtual void Putstr(int attr, const char *str) = 0;

    winid m_window;         /* winid */
    int m_flags;           /* window flags */
    xchar m_type;          /* type of window */
    boolean m_active;      /* true if window is active */
    int m_offx, m_offy;    /* offset from topleft of display */
    long m_rows, m_cols;     /* dimensions */
    long m_curx, m_cury;     /* current cursor position */
    char *m_morestr;         /* string to display instead of default */
};

struct GenericWindow : public CoreWindow
{
    GenericWindow(int type);
    virtual ~GenericWindow();

    virtual void Clear();
    virtual void Display(bool blocking) = 0;
    virtual void Dismiss();
    virtual void Putstr(int attr, const char *str);

    std::vector<std::string> m_lines;

};

static const int kMaxMessageHistoryLength = 60;
static const int kMinMessageHistoryLength = 20;

struct MessageWindow : public CoreWindow {

    MessageWindow();
    virtual ~MessageWindow();

    virtual void Clear();
    virtual void Display(bool blocking);
    virtual void Dismiss();
    virtual void Putstr(int attr, const char *str);

    std::list<std::string> m_msgList;
    std::list<std::string>::iterator m_msgIter;
    bool m_mustBeSeen;       /* message must be seen */
    bool m_mustBeErased;     /* message must be erased */
    bool m_nextIsPrompt;     /* next output message is a prompt */
};

struct MenuWindow : public GenericWindow {

    MenuWindow();
    virtual ~MenuWindow();

    virtual void Clear() { GenericWindow::Clear(); }
    virtual void Display(bool blocking);
    virtual void Dismiss();
    virtual void Putstr(int attr, const char *str);
    
    void process_lines();
    void process_menu();

    tty_menu_item *m_mlist;  /* menu information (MENU) */
    tty_menu_item **m_plist; /* menu page pointers (MENU) */
    long m_plist_size;       /* size of allocated plist (MENU) */
    long m_npages;           /* number of pages in menu (MENU) */
    long m_nitems;           /* total number of items (MENU) */
    short m_how;             /* menu mode - pick 1 or N (MENU) */
    char m_menu_ch;          /* menu char (MENU) */

};

struct BaseWindow : public GenericWindow {
    BaseWindow();
    virtual ~BaseWindow();

    virtual void Clear();
    virtual void Display(bool blocking);
    virtual void Dismiss();
    virtual void Putstr(int attr, const char *str);

};

struct StatusWindow : public CoreWindow {
    StatusWindow();
    virtual ~StatusWindow();

    virtual void Clear();
    virtual void Display(bool blocking);
    virtual void Dismiss();
    virtual void Putstr(int attr, const char *str);

    static const int kStatusHeight = 2;
    static const int kStatusWidth = 80;

    long m_statusWidth;
    long m_statusHeight;
    char m_statusLines[kStatusHeight][kStatusWidth];
};

struct MapWindow : public GenericWindow {
    MapWindow();
    virtual ~MapWindow();

    virtual void Clear();
    virtual void Display(bool blocking);
    virtual void Dismiss();
    virtual void Putstr(int attr, const char *str);

};

struct TextWindow : public GenericWindow {
    TextWindow();
    virtual ~TextWindow();

    virtual void Clear() { GenericWindow::Clear(); }
    virtual void Display(bool blocking);
    virtual void Dismiss();
    virtual void Putstr(int attr, const char *str);

    void process_lines();

};

extern "C" {

/* window flags */
#define WIN_CANCELLED 1
#define WIN_STOP 1        /* for NHW_MESSAGE; stops output */

/* descriptor for tty-based displays -- all the per-display data */
struct DisplayDesc {
    int rows, cols; /* width and height of tty display */
    int rawprint;      /* number of raw_printed lines since synch */
    char dismiss_more; /* extra character accepted at --More-- */
};

#define MAXWIN 20 /* maximum number of windows, cop-out */

/* tty dependent window types */
#ifdef NHW_BASE
#undef NHW_BASE
#endif
#define NHW_BASE 6

/* It's still not clear I've caught all the cases for H2344.  #undef this
* to back out the changes. */
#define H2344_BROKEN


extern struct window_procs uwp_procs;

/* port specific variable declarations */

extern CoreWindow *g_wins[MAXWIN];

extern struct DisplayDesc *g_uwpDisplay; /* the tty display descriptor */

extern char morc;         /* last character typed to xwaitforspace */
extern char defmorestr[]; /* default --more-- prompt */

/* port specific external function references */

void process_text_window(winid window, GenericWindow * cw);
void process_menu_window(winid window, GenericWindow * cw);


/* ### getline.c ### */
extern void FDECL(xwaitforspace, (const char *));

/* ### termcap.c, video.c ### */

extern void FDECL(tty_startup, (int *, int *));
#ifndef NO_TERMS
extern void NDECL(tty_shutdown);
#endif
#if defined(apollo)
/* Apollos don't widen old-style function definitions properly -- they try to
 * be smart and use the prototype, or some such strangeness.  So we have to
 * define UNWIDENDED_PROTOTYPES (in tradstdc.h), which makes CHAR_P below a
 * char.  But the tputs termcap call was compiled as if xputc's argument
 * actually would be expanded.	So here, we have to make an exception. */
extern void FDECL(xputc, (int));
#else
void xputc(
    char ch,
    Nethack::TextColor textColor = Nethack::TextColor::NoColor,
    Nethack::TextAttribute textAttribute = Nethack::TextAttribute::None);
#endif
void xputs(
    const char *s,
    Nethack::TextColor textColor = Nethack::TextColor::NoColor,
    Nethack::TextAttribute textAttribute = Nethack::TextAttribute::None);
#if defined(SCREEN_VGA) || defined(SCREEN_8514)
extern void FDECL(xputg, (int, int, unsigned));
#endif
extern void NDECL(cl_end);
extern void NDECL(clear_screen);
extern void NDECL(home);
extern void NDECL(backsp);
extern void NDECL(graph_on);
extern void NDECL(graph_off);
extern void NDECL(cl_eos);

/* ### topl.c ### */

extern void FDECL(addtopl, (const char *));
extern void NDECL(more);
extern void FDECL(update_topl, (const char *));
void putsyms(const char * str, Nethack::TextColor textColor, Nethack::TextAttribute textAttribute);

/* ### wintty.c ### */
#ifdef CLIPPING
extern void NDECL(setclipped);
#endif
extern void FDECL(docorner, (int, int));
void g_putch( int in_ch , Nethack::TextColor textColor, Nethack::TextAttribute textAttribute);
extern void FDECL(win_tty_init, (int));

/* external declarations */
extern void FDECL(tty_init_nhwindows, (int *, char **));
extern void NDECL(tty_player_selection);
extern void NDECL(tty_askname);
extern void NDECL(tty_get_nh_event);
extern void FDECL(tty_exit_nhwindows, (const char *));
extern void FDECL(tty_suspend_nhwindows, (const char *));
extern void NDECL(tty_resume_nhwindows);
extern winid FDECL(tty_create_nhwindow, (int));
extern void FDECL(tty_clear_nhwindow, (winid));
extern void FDECL(tty_display_nhwindow, (winid, BOOLEAN_P));
extern void FDECL(tty_dismiss_nhwindow, (winid));
extern void FDECL(tty_destroy_nhwindow, (winid));
extern void FDECL(tty_curs, (winid, int, int));
extern void FDECL(tty_putstr, (winid, int, const char *));
extern void FDECL(tty_putsym, (winid, int, int, CHAR_P));
extern void FDECL(tty_display_file, (const char *, BOOLEAN_P));
extern void FDECL(tty_start_menu, (winid));
extern void FDECL(tty_add_menu, (winid, int, const ANY_P *, CHAR_P, CHAR_P, int,
                            const char *, BOOLEAN_P));
extern void FDECL(tty_end_menu, (winid, const char *));
extern int FDECL(tty_select_menu, (winid, int, MENU_ITEM_P **));
extern char FDECL(tty_message_menu, (CHAR_P, int, const char *));
extern void NDECL(tty_update_inventory);
extern void NDECL(tty_mark_synch);
extern void NDECL(tty_wait_synch);
#ifdef CLIPPING
extern void FDECL(tty_cliparound, (int, int));
#endif
#ifdef POSITIONBAR
extern void FDECL(tty_update_positionbar, (char *));
#endif
extern void FDECL(tty_print_glyph, (winid, XCHAR_P, XCHAR_P, int, int));
extern void FDECL(tty_raw_print, (const char *));
extern void FDECL(tty_raw_print_bold, (const char *));
extern int NDECL(tty_nhgetch);
extern int FDECL(tty_nh_poskey, (int *, int *, int *));
extern void NDECL(tty_nhbell);
extern int NDECL(tty_doprev_message);
extern char FDECL(tty_yn_function, (const char *, const char *, CHAR_P));
extern void FDECL(tty_getlin, (const char *, char *));
extern int NDECL(tty_get_ext_cmd);
extern void FDECL(tty_number_pad, (int));
extern void NDECL(tty_delay_output);
#ifdef CHANGE_COLOR
extern void FDECL(tty_change_color, (int color, long rgb, int reverse));
#ifdef MAC
extern void FDECL(tty_change_background, (int white_or_black));
extern short FDECL(set_tty_font_name, (winid, char *));
#endif
extern char *NDECL(tty_get_color_string);
#endif
#ifdef STATUS_VIA_WINDOWPORT
extern void NDECL(tty_status_init);
extern void FDECL(tty_status_update, (int, genericptr_t, int, int));
#ifdef STATUS_HILITES
extern void FDECL(tty_status_threshold, (int, int, anything, int, int, int));
#endif
#endif

/* other defs that really should go away (they're tty specific) */
extern void NDECL(tty_start_screen);
extern void NDECL(tty_end_screen);

extern void FDECL(genl_outrip, (winid, int, time_t));

extern char *FDECL(tty_getmsghistory, (BOOLEAN_P));
extern void FDECL(tty_putmsghistory, (const char *, BOOLEAN_P));

void msmsg_bold(const char *, ...);

/* New window output functions.
 * These window based output functions use the window cursor position
 * for output and update all three cursor positions (window, g_uwpDisplay
 * and g_textGrid) appropriately.
 */
void win_putc(
    winid window,
    char ch,
    Nethack::TextColor textColor = Nethack::TextColor::NoColor,
    Nethack::TextAttribute textAttribute = Nethack::TextAttribute::None);

void win_puts(
    winid window,
    const char *s,
    Nethack::TextColor textColor = Nethack::TextColor::NoColor,
    Nethack::TextAttribute textAttribute = Nethack::TextAttribute::None);

CoreWindow * GetCoreWindow(winid window);
GenericWindow * GetGenericWindow(winid window);
MessageWindow * GetMessageWindow();
MessageWindow * ToMessageWindow(CoreWindow * coreWin);
GenericWindow * ToGenericWindow(CoreWindow * coreWin);
MenuWindow * ToMenuWindow(CoreWindow * coreWin);

}
