#pragma once
#include <stdint.h>

#define TTY_ECHO        (1 << 1)
#define TTY_CANNONICAL  (1 << 2)
#define TTY_NONBLOCK    (1 << 4)

#define TTY_IOCTL_GET_FLAGS  0x5401
#define TTY_IOCTL_SET_FLAGS  0x5402

#define TTY_IOCTL_GET_CURSOR    0x5403
#define TTY_IOCTL_SET_CURSOR    0x5404

typedef struct {
    uint32_t x;
    uint32_t y;
} tty_cursor_t;