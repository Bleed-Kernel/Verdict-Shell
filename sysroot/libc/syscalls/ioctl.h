#pragma once

#define TTY_ECHO        (1 << 1)
#define TTY_CANNONICAL  (1 << 2)
#define TTY_NONBLOCK    (1 << 4)

#define TTY_IOCTL_GET_FLAGS  0x5401
#define TTY_IOCTL_SET_FLAGS  0x5402

int _ioctl(int fd, unsigned long request, void* arg);