#include <syscalls/read.h>
#include <devices/keyboard.h>
#include <stdio.h>
#include <string.h>
#include <main.h>

#define HISTORY_SIZE 20
#define MAX_LINE 256

static char history[HISTORY_SIZE][MAX_LINE];
static int history_len = 0;

int shell_read_line(char *buf, size_t max) {
    size_t len = 0;
    keyboard_event_t input;
    int nav_index = -1;

    buf[0] = 0;

    while (len + 1 < max) {
        if (_read(0, &input, sizeof(input)) <= 0)
            continue;

        if (input.action == KEY_RELEASE)
            continue;

        if (input.keycode == ArrowUp) {
            if (history_len == 0) continue;

            if (nav_index == -1)
                nav_index = history_len - 1;
            else if (nav_index > 0)
                nav_index--;

            while (len > 0) {
                printf("\b \b");
                len--;
            }

            strcpy(buf, history[nav_index]);
            len = strlen(buf);
            printf("%s", buf);
            continue;
        }

        if (input.keycode == ArrowDown) {
            if (history_len == 0 || nav_index == -1) continue;

            nav_index++;
            if (nav_index >= history_len) {
                nav_index = -1;
                while (len > 0) {
                    printf("\b \b");
                    len--;
                }
                buf[0] = 0;
            } else {
                while (len > 0) {
                    printf("\b \b");
                    len--;
                }
                strcpy(buf, history[nav_index]);
                len = strlen(buf);
                printf("%s", buf);
            }
            continue;
        }

        char c = tty_key_to_ascii(&input);

        if (c == '\n') {
            buf[len] = 0;
            printf("\n");

            if (len > 0) {
                if (history_len < HISTORY_SIZE) {
                    strcpy(history[history_len++], buf);
                } else {
                    memmove(history, history + 1, sizeof(history) - sizeof(history[0]));
                    strcpy(history[HISTORY_SIZE - 1], buf);
                }
            }

            return len;
        }

        if (c == '\b') {
            if (len > 0) {
                len--;
                printf("\b \b");
                buf[len] = 0;
            }
            continue;
        }

        if (c >= 32 && c <= 126) {
            buf[len++] = c;
            buf[len] = 0;
            printf("%c", c);
        }

        nav_index = -1;
    }

    buf[len] = 0;
    return len;
}
