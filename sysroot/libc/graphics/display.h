#pragma once
#include <stdint.h>

struct fb_info {
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint64_t bpp;
};

#define FB_IOC_GET_INFO 0x5001
#define FB_IOC_FLIP     0x5002

#define TTY_IOCTL_GET_FLAGS  0x5401
#define TTY_IOCTL_SET_FLAGS  0x5402

int get_fb_metadata(struct fb_info *out);