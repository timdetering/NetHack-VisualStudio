#include <assert.h>

extern "C"  {

#include "hack.h"

int kbhit(void)
{
    return 0;
}

void error(const char * fmt, ...)
{
    assert(0);
}

#ifdef PORT_DEBUG
void
    win32con_debug_keystrokes()
{
    assert(0);
    return;
}
void
    win32con_handler_info()
{
    assert(0);
    return;
}
#endif

void interject_assistance(int num, int interjection_type, void * ptr1, void * ptr2)
{
    assert(0);
}

void interject(int interjection_type)
{
    assert(0);
}

void win32_abort(void)
{
    assert(0);
}

void nethack_exit(int result)
{
    assert(0);
}

int findfirst(char *path)
{
    assert(0);
    return 0;
}

int findnext(void)
{
    assert(0);
    return 0;
}

char * foundfile_buffer()
{
    assert(0);
    return NULL;
}

char * get_username(int * lan_username_size)
{
    assert(0);
    return NULL;
}

void Delay(int ms)
{
    assert(0);
    // (void)Sleep(ms);
}

void append_slash(char * name)
{
    assert(0);
#if 0
    char *ptr;

    if (!*name)
        return;
    ptr = name + (strlen(name) - 1);
    if (*ptr != '\\' && *ptr != '/' && *ptr != ':') {
        *++ptr = '\\';
        *++ptr = '\0';
    }
#endif
    return;
}

void xputc(char ch)
{
    assert(0);
    return;
}

void tty_number_pad(int state)
{
    assert(0);
}

void
tty_startup(int * wid, int * hgt)
{
    assert(0);
}

void
tty_start_screen()
{
    assert(0);
}

void
home()
{
    assert(0);
}

void
backsp()
{
    assert(0);
}

void
tty_nhbell()
{
    assert(0);
}

void
tty_end_screen()
{
    assert(0);
}

void
cmov(int x, int y)
{
    assert(0);
}

void
nocmov(int x, int y)
{
    assert(0);
}

void
term_start_color(int color)
{
    assert(0);
}

void
term_end_color(void)
{
    assert(0);
}

void
term_end_raw_bold(void)
{
    assert(0);
}

void
term_start_raw_bold(void)
{
    assert(0);
}

void
term_start_attr(int attrib)
{
    assert(0);
}

void
term_end_attr(int attrib)
{
    assert(0);
}

void
cl_eos()
{
    assert(0);
}

void
cl_end()
{
    assert(0);
}

void
clear_screen()
{
    assert(0);
}

char erase_char, kill_char;

int
has_color(int color)
{
    assert(0);
    return 0;
}

void
map_subkeyvalue(char *op)
{
    assert(0);
    return;
}

void
load_keyboard_handler()
{
    assert(0);
    return;
}

void
standoutbeg()
{
    assert(0);
}

void
standoutend()
{
    assert(0);
}

void
g_putch(int in_ch)
{
    assert(0);
}

/* validate wizard mode if player has requested access to it */
boolean
authorize_wizard_mode()
{
    if (!strcmp(plname, WIZARD_NAME))
        return TRUE;
    return FALSE;
}

#ifdef PORT_HELP
void
port_help()
{
    assert(0);
}
#endif /* PORT_HELP */

#ifdef USER_SOUNDS
void
play_usersound(const char *filename, int volume)
{
    assert(0);
}
#endif /*USER_SOUNDS*/

#ifdef RUNTIME_PORT_ID
void
append_port_id(char *buf)
{
    assert(0);
}
#endif /* RUNTIME_PORT_ID */

void
xputs(const char *s)
{
    assert(0);
}

void
tty_delay_output()
{
    assert(0);
}

void
nttty_preference_update(const char * pref)
{
    assert(0);
}

int
tgetch()
{
    assert(0);
    return 0;
}

void
gettty()
{
    assert(0);
}

void
settty(const char *s)
{
    assert(0);
}

void
setftty()
{
    assert(0);
}

} // extern "C"
