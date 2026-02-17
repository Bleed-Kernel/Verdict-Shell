#pragma once

#include <stddef.h>
#include <stdint.h>

#define TTY_ANSI_ESC "\x1b"
#define TTY_ANSI_CSI "\x1b["

typedef enum {
    TTY_ANSI_COLOR_BLACK = 0,
    TTY_ANSI_COLOR_RED = 1,
    TTY_ANSI_COLOR_GREEN = 2,
    TTY_ANSI_COLOR_YELLOW = 3,
    TTY_ANSI_COLOR_BLUE = 4,
    TTY_ANSI_COLOR_MAGENTA = 5,
    TTY_ANSI_COLOR_CYAN = 6,
    TTY_ANSI_COLOR_WHITE = 7
} tty_ansi_color_t;

int tty_ansi_write(int fd, const char *seq);
int tty_ansi_csi(int fd, const char *params, char final);

int tty_ansi_reset(int fd);
int tty_ansi_bold(int fd, int enable);
int tty_ansi_underline(int fd, int enable);
int tty_ansi_inverse(int fd, int enable);
int tty_ansi_fg8(int fd, tty_ansi_color_t color, int bright);
int tty_ansi_bg8(int fd, tty_ansi_color_t color, int bright);
int tty_ansi_fg256(int fd, uint8_t index);
int tty_ansi_bg256(int fd, uint8_t index);
int tty_ansi_fg_rgb(int fd, uint8_t r, uint8_t g, uint8_t b);
int tty_ansi_bg_rgb(int fd, uint8_t r, uint8_t g, uint8_t b);

int tty_ansi_cursor_up(int fd, uint32_t n);
int tty_ansi_cursor_down(int fd, uint32_t n);
int tty_ansi_cursor_forward(int fd, uint32_t n);
int tty_ansi_cursor_back(int fd, uint32_t n);
int tty_ansi_cursor_next_line(int fd, uint32_t n);
int tty_ansi_cursor_prev_line(int fd, uint32_t n);
int tty_ansi_cursor_col(int fd, uint32_t col);
int tty_ansi_cursor_pos(int fd, uint32_t row, uint32_t col);
int tty_ansi_save_cursor(int fd);
int tty_ansi_restore_cursor(int fd);
int tty_ansi_hide_cursor(int fd);
int tty_ansi_show_cursor(int fd);

int tty_ansi_erase_display(int fd, uint8_t mode);
int tty_ansi_erase_line(int fd, uint8_t mode);
int tty_ansi_clear_screen(int fd);
int tty_ansi_clear_line(int fd);
int tty_ansi_scroll_up(int fd, uint32_t n);
int tty_ansi_scroll_down(int fd, uint32_t n);
