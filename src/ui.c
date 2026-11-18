/*
 * ui.c - ANSI output buffering, line composition, raw terminal and input.
 * Nothing in here knows about game rules; every screen is built on Line.
 */
#include "game.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/* ------------------------------------------------------------------------- */
/*  Small output buffer                                                      */
/* ------------------------------------------------------------------------- */
static char g_out[65536];
int g_out_len;

void out_add(const char *s, int n);

void out_begin(void) {
    g_out_len = 0;
    out_add("\x1b[H", 3);
}

void out_add(const char *s, int n) {
    if (n <= 0) return;
    if (g_out_len + n >= (int)sizeof(g_out)) return;
    memcpy(g_out + g_out_len, s, (size_t)n);
    g_out_len += n;
}

void out_str(const char *s) {
    out_add(s, (int)strlen(s));
}

void out_flush(void) {
    const char *p = g_out;
    int left = g_out_len;
    while (left > 0) {
        long n = asm_write(1, p, (unsigned long)left);
        if (n <= 0) break;
        p += n;
        left -= (int)n;
    }
}

void line_init(Line *l) {
    l->s[0] = '\0';
    l->len = 0;
    l->vis = 0;
}

void lraw(Line *l, const char *s) {
    size_t n = strlen(s);
    if (l->len + (int)n >= (int)sizeof(l->s) - 1)
        n = sizeof(l->s) - 1 - (size_t)l->len;
    if (n == 0) return;
    memcpy(l->s + l->len, s, n);
    l->len += (int)n;
    l->s[l->len] = '\0';
}

void ltxt(Line *l, const char *s) {
    lraw(l, s);
    l->vis += (int)strlen(s);
}

void lcol(Line *l, const char *color) {
    lraw(l, color);
}

void lfmt(Line *l, const char *fmt, ...) {
    char tmp[512];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n >= (int)sizeof(tmp)) n = (int)sizeof(tmp) - 1;
    lraw(l, tmp);
    l->vis += n;
}

void lpad(Line *l, int col) {
    while (l->vis < col) {
        lraw(l, " ");
        l->vis++;
    }
}

void out_line(Line *l, int last) {
    out_add(l->s, l->len);
    out_str("\x1b[K");
    if (!last) out_str("\r\n");
}

/* ------------------------------------------------------------------------- */
/*  Terminal handling                                                        */
/* ------------------------------------------------------------------------- */
static struct termios g_saved_termios;
int g_raw_active;
int g_alt_active;
int g_atexit_installed;
int g_cleanup_done;

void term_restore(void) {
    if (g_cleanup_done) return;
    g_cleanup_done = 1;
    if (g_raw_active) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_saved_termios);
        g_raw_active = 0;
    }
    if (g_alt_active) {
        const char *seq = "\x1b[0m\x1b[?25h\x1b[?1049l";
        asm_write(1, seq, (unsigned long)strlen(seq));
        g_alt_active = 0;
    }
}

int term_setup(void) {
    struct winsize ws;

    if (!g_atexit_installed) {
        atexit(term_restore);
        g_atexit_installed = 1;
    }

    if (isatty(STDOUT_FILENO)) {
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
            ws.ws_col > 0 && ws.ws_row > 0) {
            if (ws.ws_col < 70 || ws.ws_row < 24) {
                fprintf(stderr,
                        "ASCII QUEST needs a terminal of at least 70x24 "
                        "(current %ux%u).\n",
                        (unsigned)ws.ws_col, (unsigned)ws.ws_row);
                return -1;
            }
        }
        const char *enter = "\x1b[?1049h\x1b[?25l\x1b[2J";
        asm_write(1, enter, (unsigned long)strlen(enter));
        g_alt_active = 1;
    }

    if (isatty(STDIN_FILENO)) {
        if (tcgetattr(STDIN_FILENO, &g_saved_termios) == 0) {
            struct termios raw = g_saved_termios;
            raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
            raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
            raw.c_oflag &= ~OPOST;
            raw.c_cflag |= CS8;
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0)
                g_raw_active = 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/*  Keyboard                                                                 */
/* ------------------------------------------------------------------------- */

/* Wait up to 30 ms for one more byte of an escape sequence.  Without this
 * timeout a lone Esc key would block the game until the next keypress. */
static int read_escape_byte(unsigned char *c) {
    fd_set set;
    struct timeval tv;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    tv.tv_sec = 0;
    tv.tv_usec = 30000;
    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) <= 0) return 0;
    return asm_read(STDIN_FILENO, c, 1) == 1;
}

int read_key(void) {
    unsigned char c;
    long n = asm_read(STDIN_FILENO, &c, 1);
    if (n <= 0) return KEY_EOF;

    if (c == 27) {
        unsigned char a, b;
        if (!read_escape_byte(&a)) return 27;
        if (!read_escape_byte(&b)) return 27;
        if (a == '[' && b >= '0' && b <= '9') {
            unsigned char d;
            if (!read_escape_byte(&d)) return 27;
            if (b == '1' && d == '5') { read_escape_byte(&d); return KEY_F5; }
            if (b == '2' && d == '0') { read_escape_byte(&d); return KEY_F9; }
            return 27;
        }
        if (a == '[' || a == 'O') {
            switch (b) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            default: break;
            }
        }
        return 27;
    }
    if (c == '\r' || c == '\n') return '\n';
    return (int)c;
}

/* ------------------------------------------------------------------------- */
/*  Utility                                                                  */
