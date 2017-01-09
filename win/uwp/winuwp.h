/* NetHack 3.6	winuwp.h	$NHDT-Date: $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) Bart House, 2017				  */
/* NetHack may be freely redistributed.  See license for details. */
#pragma once

#ifndef UWP_GRAPHICS
#error UWP_GRAPHICS must be defined
#endif

/* menu structure */
typedef struct uwp_mi {
    struct uwp_mi *next;
    anything identifier; /* user identifier */
    long count;          /* user count */
    char *str;           /* description string (including accelerator) */
    int attr;            /* string attribute */
    boolean selected;    /* TRUE if selected by user */
    char selector;       /* keyboard accelerator */
    char gselector;      /* group accelerator */
} uwp_menu_item;

/* descriptor for tty-based windows */
struct UwpWinDesc {
    int flags;           /* window flags */
    xchar type;          /* type of window */
    boolean active;      /* true if window is active */
    short offx, offy;    /* offset from topleft of display */
    long rows, cols;     /* dimensions */
    long curx, cury;     /* current cursor position */
    long maxrow, maxcol; /* the maximum size used -- for MENU wins */
    /* maxcol is also used by WIN_MESSAGE for */
    /* tracking the ^P command */
    short *datlen;         /* allocation size for *data */
    char **data;           /* window data [row][column] */
    char *morestr;         /* string to display instead of default */
    uwp_menu_item *mlist;  /* menu information (MENU) */
    uwp_menu_item **plist; /* menu page pointers (MENU) */
    long plist_size;       /* size of allocated plist (MENU) */
    long npages;           /* number of pages in menu (MENU) */
    long nitems;           /* total number of items (MENU) */
    short how;             /* menu mode - pick 1 or N (MENU) */
    char menu_ch;          /* menu char (MENU) */
};

/* window flags */
#define UWP_WIN_CANCELLED 1
#define UWP_WIN_STOP 1        /* for NHW_MESSAGE; stops output */
#define UWP_WIN_LOCKHISTORY 2 /* for NHW_MESSAGE; suppress history updates */

/* descriptor for uwp-based displays -- all the per-display data */
struct UwpDisplayDesc {
    short rows, cols; /* width and height of uwp display */
    short curx, cury; /* current cursor position on the screen */
#ifdef TEXTCOLOR
    int color; /* current color */
#endif
    int attrs;         /* attributes in effect */
    int toplin;        /* flag for topl stuff */
    int rawprint;      /* number of raw_printed lines since synch */
    int inmore;        /* non-zero if more() is active */
    int inread;        /* non-zero if reading a character */
    int intr;          /* non-zero if inread was interrupted */
    winid lastwin;     /* last window used for I/O */
    char dismiss_more; /* extra character accepted at --More-- */
};

#define UWP_MAXWIN 20 /* maximum number of windows, cop-out */
#define UWP_NHW_BASE 6

extern struct window_procs uwp_procs;

/* port specific variable declarations */
extern winid UWP_BASE_WINDOW;

extern struct UwpWinDesc *uwp_wins[UWP_MAXWIN];

extern struct UwpDisplayDesc *uwpDisplay; /* the uwp display descriptor */

extern char uwp_morc;         /* last character typed to xwaitforspace */

extern char uwp_defmorestr[]; /* default --more-- prompt */

/* port specific external function references */

void uwp_msmsg(const char * fmt, ...);

/* ### getline.c ### */
void FDECL(uwp_xwaitforspace, (const char *));

/* ### termcap.c, video.c ### */

void FDECL(uwp_startup, (int *, int *));

void FDECL(uwp_xputc, (CHAR_P));

void FDECL(uwp_xputs, (const char *));

void NDECL(uwp_cl_end);
void NDECL(uwp_clear_screen);
void NDECL(uwp_home);
void NDECL(uwp_standoutbeg);
void NDECL(uwp_standoutend);

void NDECL(uwp_backsp);
void NDECL(uwp_graph_on);
void NDECL(uwp_graph_off);
void NDECL(uwp_cl_eos);

/*
 * termcap.c (or facsimiles in other ports) is the right place for doing
 * strange and arcane things such as outputting escape sequences to select
 * a color or whatever.  wintty.c should concern itself with WHERE to put
 * stuff in a window.
 */
void FDECL(uwp_term_start_attr, (int attr));
void FDECL(uwp_term_end_attr, (int attr));
void NDECL(uwp_term_start_raw_bold);
void NDECL(uwp_term_end_raw_bold);

#ifdef TEXTCOLOR
void NDECL(uwp_term_end_color);
void FDECL(uwp_term_start_color, (int color));
int FDECL(uwp_has_color, (int color));
#endif /* TEXTCOLOR */

/* ### topl.c ### */

void FDECL(uwp_addtopl, (const char *));
void NDECL(uwp_more);
void FDECL(uwp_update_topl, (const char *));
void FDECL(uwp_putsyms, (const char *));

/* ### wintty.c ### */
#ifdef CLIPPING
void NDECL(uwp_setclipped);
#endif
void FDECL(uwp_docorner, (int, int));
void NDECL(uwp_end_glyphout);
void FDECL(uwp_g_putch, (int));
void FDECL(win_uwp_init, (int));

/* external declarations */
void FDECL(uwp_init_nhwindows, (int *, char **));
void NDECL(uwp_player_selection);
void NDECL(uwp_askname);
void NDECL(uwp_get_nh_event);
void FDECL(uwp_exit_nhwindows, (const char *));
void FDECL(uwp_suspend_nhwindows, (const char *));
void NDECL(uwp_resume_nhwindows);
winid FDECL(uwp_create_nhwindow, (int));
void FDECL(uwp_clear_nhwindow, (winid));
void FDECL(uwp_display_nhwindow, (winid, BOOLEAN_P));
void FDECL(uwp_dismiss_nhwindow, (winid));
void FDECL(uwp_destroy_nhwindow, (winid));
void FDECL(uwp_curs, (winid, int, int));
void FDECL(uwp_putstr, (winid, int, const char *));
void FDECL(uwp_display_file, (const char *, BOOLEAN_P));
void FDECL(uwp_start_menu, (winid));
void FDECL(uwp_add_menu, (winid, int, const ANY_P *, CHAR_P, CHAR_P, int,
                            const char *, BOOLEAN_P));
void FDECL(uwp_end_menu, (winid, const char *));
int FDECL(uwp_select_menu, (winid, int, MENU_ITEM_P **));
char FDECL(uwp_message_menu, (CHAR_P, int, const char *));
void NDECL(uwp_update_inventory);
void NDECL(uwp_mark_synch);
void NDECL(uwp_wait_synch);
#ifdef CLIPPING
void FDECL(uwp_cliparound, (int, int));
#endif
#ifdef POSITIONBAR
void FDECL(uwp_update_positionbar, (char *));
#endif
void FDECL(uwp_print_glyph, (winid, XCHAR_P, XCHAR_P, int, int));
void FDECL(uwp_raw_print, (const char *));
void FDECL(uwp_raw_print_bold, (const char *));
int NDECL(uwp_nhgetch);
int FDECL(uwp_nh_poskey, (int *, int *, int *));
void NDECL(uwp_nhbell);
int NDECL(uwp_doprev_message);
char FDECL(uwp_yn_function, (const char *, const char *, CHAR_P));
void FDECL(uwp_getlin, (const char *, char *));
int NDECL(uwp_get_ext_cmd);
void FDECL(uwp_number_pad, (int));
void NDECL(uwp_delay_output);
#ifdef CHANGE_COLOR
void FDECL(uwp_change_color, (int color, long rgb, int reverse));
#ifdef MAC
void FDECL(uwp_change_background, (int white_or_black));
short FDECL(set_uwp_font_name, (winid, char *));
#endif
char *NDECL(uwp_get_color_string);
#endif
#ifdef STATUS_VIA_WINDOWPORT
void NDECL(uwp_status_init);
void FDECL(uwp_status_update, (int, genericptr_t, int, int));
#ifdef STATUS_HILITES
void FDECL(uwp_status_threshold, (int, int, anything, int, int, int));
#endif
#endif

/* other defs that really should go away (they're tty specific) */
void NDECL(uwp_start_screen);
void NDECL(uwp_end_screen);

void FDECL(uwp_genl_outrip, (winid, int, time_t));

char *FDECL(uwp_getmsghistory, (BOOLEAN_P));
void FDECL(uwp_putmsghistory, (const char *, BOOLEAN_P));


#undef putchar
#undef putc
#undef puts
#define putchar(x) uwp_xputc(x) /* these are in video.c, nttty.c */
#define putc(x) uwp_xputc(x)
#define puts(x) uwp_xputs(x)

#ifdef POSITIONBAR
void FDECL(uwp_video_update_positionbar, (char *));
#endif
