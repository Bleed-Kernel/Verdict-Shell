#include <syscalls/read.h>
#include <syscalls/open.h>
#include <devices/console.h>
#include <devices/keyboard.h>
#include <devices/hpet.h>
#include <syscalls/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <main.h>
#include <signal.h>

#define HISTORY_SIZE 20
#define MAX_LINE 256
#define BLINK_VISIBLE_US 600000
#define BLINK_INVISIBLE_US 300000

typedef struct {
    char buf[MAX_LINE];
    size_t len;
    size_t pos;
    char history[HISTORY_SIZE][MAX_LINE];
    int history_count;
    int nav_index;

    int insert_mode;
    int cursor_visible;
    uint64_t last_blink_time;
    uint64_t last_time_read;
    int accumulated_usec;
    int accumulated_sec;
} shell_state_t;

static shell_state_t shell;
static int hpet_fd = -2;
static volatile int shell_sigint_pending = 0;
static int shell_tty_fd = -2;

static int resolve_shell_tty_fd(void) {
    if (shell_tty_fd != -2)
        return shell_tty_fd >= 0 ? shell_tty_fd : 1;

    uint32_t tty_index = 0;
    if (_ioctl(1, TTY_IOCTL_GET_INDEX, &tty_index) == 0) {
        char tty_path[16];
        snprintf(tty_path, sizeof(tty_path), "/dev/tty%u", tty_index);
        shell_tty_fd = (int)_open(tty_path, O_RDWR);
        if (shell_tty_fd >= 0)
            return shell_tty_fd;
    }

    shell_tty_fd = -1;
    return 1;
}

void shell_sigint_handler(int sig) {
    (void)sig;
    shell_sigint_pending = 1;
}

int shell_install_signal_handlers(void) {
    struct sigaction act = {0};
    act.handler = (uintptr_t)shell_sigint_handler;
    act.flags = SA_RESTART;
    if (sigemptyset(&act.mask) < 0)
        return -1;
    if (sigaction(SIGINT, &act, NULL) < 0)
        return -1;
    return 0;
}

static uint64_t read_femtoseconds(void) {
    uint64_t now = 0;

    if (hpet_fd == -2) {
        hpet_fd = (int)_open("/dev/hpet", O_RDONLY);
    }
    if (hpet_fd < 0) {
        return 0;
    }

    if (_ioctl(hpet_fd, HPET_IOCTL_GET_FEMTOSECONDS, &now) == 0) {
        return now;
    }

    if (_read(hpet_fd, &now, sizeof(now)) == (long)sizeof(now)) {
        return now;
    }

    return 0;
}

static void update_time_counters() {
    uint64_t new_time = read_femtoseconds();
    if (new_time == 0) {
        // if the device becomes unavailable, we can catch up
        shell.accumulated_usec += 1000;
    } else {
        if (shell.last_time_read == 0 || new_time < shell.last_time_read) {
            shell.last_time_read = new_time;
            return;
        }

        shell.accumulated_usec += (new_time - shell.last_time_read) / femtosecondsPerMicrosecond;
        shell.last_time_read = new_time;
    }
    while (shell.accumulated_usec >= 1000000){
        shell.accumulated_usec -= 1000000;
        shell.accumulated_sec++;
    }
}

static const char* get_cursor_char() {
    return shell.insert_mode ? "_" : "█";
}

static uint64_t get_now_us() {
    update_time_counters();
    return (uint64_t)shell.accumulated_sec * 1000000 + shell.accumulated_usec;
}

static void cursor_get_pos(tty_cursor_t* c) {
    int tty_fd = resolve_shell_tty_fd();
    _ioctl((uint64_t)tty_fd, TTY_IOCTL_GET_CURSOR, c);
}

static void cursor_set_pos(tty_cursor_t* c) {
    int tty_fd = resolve_shell_tty_fd();
    _ioctl((uint64_t)tty_fd, TTY_IOCTL_SET_CURSOR, c);
}

static void cursor_move_rel(int dx) {
    tty_cursor_t c;
    cursor_get_pos(&c);
    c.x += dx;
    cursor_set_pos(&c);
}

static void remove_visual_cursor() {
    if (!shell.cursor_visible) return;

    char under = (shell.pos < shell.len) ? shell.buf[shell.pos] : ' ';
    printf("%c", under);
    cursor_move_rel(-1);
    shell.cursor_visible = 0;
}

static void force_visible_cursor() {
    printf("%s", get_cursor_char());
    cursor_move_rel(-1);
    shell.cursor_visible = 1;
    shell.last_blink_time = get_now_us();
}

static void process_blink() {
    uint64_t now = get_now_us();
    uint64_t limit = shell.cursor_visible ? BLINK_VISIBLE_US : BLINK_INVISIBLE_US;

    if (now - shell.last_blink_time >= limit) {
        shell.last_blink_time = now;

        if (shell.cursor_visible) {
            char under = (shell.pos < shell.len) ? shell.buf[shell.pos] : ' ';
            printf("%c", under);
            shell.cursor_visible = 0;
        } else {
            printf("%s", get_cursor_char());
            shell.cursor_visible = 1;
        }
        cursor_move_rel(-1);
    }
}

static void handle_input_char(char c) {
    if (!shell.insert_mode) {
        if (shell.len + 1 >= MAX_LINE) return;
        memmove(shell.buf + shell.pos + 1, shell.buf + shell.pos, shell.len - shell.pos + 1);
        shell.buf[shell.pos] = c;
        shell.len++;
        printf("%s", shell.buf + shell.pos);
        shell.pos++;
        int chars_to_rewind = shell.len - shell.pos;
        if (chars_to_rewind > 0) cursor_move_rel(-chars_to_rewind);
    } else {
        if (shell.pos >= MAX_LINE - 1) return;
        shell.buf[shell.pos] = c;
        if (shell.pos == shell.len) shell.len++;
        printf("%c", c);
        shell.pos++;
    }
}

static void handle_backspace() {
    if (shell.pos == 0) return;

    memmove(shell.buf + shell.pos - 1, shell.buf + shell.pos, shell.len - shell.pos + 1);
    shell.len--;
    shell.pos--;

    cursor_move_rel(-1);
    printf("%s ", shell.buf + shell.pos);

    int chars_to_rewind = (shell.len - shell.pos) + 1;
    cursor_move_rel(-chars_to_rewind);
}

static int is_word_delim(char c) {
    return c == ' ' || c == '\t';
}

static void handle_ctrl_backspace() {
    if (shell.pos == 0) return;

    while (shell.pos > 0 && is_word_delim(shell.buf[shell.pos - 1])) {
        handle_backspace();
    }

    while (shell.pos > 0 && !is_word_delim(shell.buf[shell.pos - 1])) {
        handle_backspace();
    }
}

static void handle_history_nav(int direction) {
    if (shell.history_count == 0) return;

    if (direction == ArrowUp) {
        if (shell.nav_index == -1) shell.nav_index = shell.history_count - 1;
        else if (shell.nav_index > 0) shell.nav_index--;
    } else {
        if (shell.nav_index == -1) return;
        shell.nav_index++;
        if (shell.nav_index >= shell.history_count) {
            shell.nav_index = -1;
        }
    }

    cursor_move_rel(-(int)shell.pos);
    for (size_t i = 0; i < shell.len; i++) printf(" ");
    cursor_move_rel(-(int)shell.len);

    if (shell.nav_index != -1) {
        strcpy(shell.buf, shell.history[shell.nav_index]);
        shell.len = strlen(shell.buf);
    } else {
        shell.buf[0] = 0;
        shell.len = 0;
    }

    printf("%s", shell.buf);
    shell.pos = shell.len;
}

static void save_history() {
    if (shell.len == 0) return;

    if (shell.history_count < HISTORY_SIZE) {
        strcpy(shell.history[shell.history_count++], shell.buf);
    } else {
        memmove(shell.history, shell.history + 1, sizeof(shell.history) - sizeof(shell.history[0]));
        strcpy(shell.history[HISTORY_SIZE - 1], shell.buf);
    }
}

int shell_read_line(char *out_buf, size_t max) {
    shell.len = 0;
    shell.pos = 0;
    shell.nav_index = -1;
    shell.buf[0] = 0;

    if(max > 0) out_buf[0] = 0;

    keyboard_event_t input;

    while (1) {
        if (shell_sigint_pending) {
            tty_cursor_t cursor;
            shell_sigint_pending = 0;
            shell.len = 0;
            shell.pos = 0;
            shell.buf[0] = 0;
            if (max > 0) out_buf[0] = 0;
            cursor_get_pos(&cursor);
            if (cursor.x != 0) printf("\n");
            return 0;
        }

        process_blink();

        if (_read(0, &input, sizeof(input)) <= 0) {
            for (volatile int i = 0; i < 10000; i++);
            continue;
        }

        if (input.action == KEY_RELEASE) continue;

        char c = tty_key_to_ascii(&input);
        if ((input.keymod & KEYMOD_CTRL) && (c == 'c' || c == 'C')) {
            remove_visual_cursor();
            shell_sigint_pending = 1;
            continue;
        }

        remove_visual_cursor();

        if (input.keycode == Home) {
            if (shell.pos > 0) {
                int move = shell.pos;
                shell.pos = 0;
                cursor_move_rel(-move);
            }

            force_visible_cursor();
            continue;
        }

        if (input.keycode == End) {
            if (shell.pos < shell.len) {
                int move = shell.len - shell.pos;
                shell.pos = shell.len;
                cursor_move_rel(move);
            }

            force_visible_cursor();
            continue;
        }

        if (input.keycode == ArrowUp || input.keycode == ArrowDown) {
            handle_history_nav(input.keycode);
            force_visible_cursor();
            continue;
        }

        if (input.keycode == Insert) {
            shell.insert_mode = !shell.insert_mode;
            remove_visual_cursor();
            force_visible_cursor();
            continue;
        }

        if (input.keycode == ArrowLeft) {
            if (shell.pos > 0) {
                shell.pos--;
                cursor_move_rel(-1);
            }
            force_visible_cursor();
            continue;
        }

        if (input.keycode == ArrowRight) {
            if (shell.pos < shell.len) {
                shell.pos++;
                cursor_move_rel(1);
            }
            force_visible_cursor();
            continue;
        }


        if (c == '\n') {
            printf("\n");
            save_history();
            if (max > 0) {
                size_t copy_len = (shell.len < max) ? shell.len : max - 1;
                memcpy(out_buf, shell.buf, copy_len);
                out_buf[copy_len] = 0;
            }
            return shell.len;
        }

        if (c == '\b') {
            if (input.keymod & KEYMOD_CTRL) {
                handle_ctrl_backspace();
                force_visible_cursor();
                continue;
            }

            handle_backspace();
            force_visible_cursor();
            continue;
        }

        if (c >= 32 && c <= 126) {
            handle_input_char(c);
            force_visible_cursor();
        }
    }
}
