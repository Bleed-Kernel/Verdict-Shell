#pragma once
#include <stdint.h>

typedef enum {
    KEY_RELEASE,
    KEY_DOWN
} key_action_t;

enum {
    KEYMOD_SHIFT    = 1 << 0,
    KEYMOD_CTRL     = 1 << 1,
    KEYMOD_ALT      = 1 << 2,
    KEYMOD_CAPS     = 1 << 3
};

typedef struct {
    uint16_t    keycode;
    uint8_t     action;
    uint8_t     keymod;
} keyboard_event_t;

static const char keymap[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a',
    's','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c',
    'v','b','n','m',',','.','/',0,'*',0,' ',0
};

static const char keymap_shift[128] = {
    0,27,'!','"','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A',
    'S','D','F','G','H','J','K','L',':','@','~',0,'|','Z','X','C',
    'V','B','N','M','<','>','?',0,'*',0,' ',0
};

char tty_key_to_ascii(const keyboard_event_t *ev) {
    uint16_t sc = ev->keycode;
    if (sc >= 128) return 0;
    if (ev->keymod & KEYMOD_SHIFT)
        return keymap_shift[sc];
    if (ev->keymod & KEYMOD_CAPS) {
        char c = keymap[sc];
        if (c >= 'a' && c <= 'z')
            c -= 32;
        return c;
    }
    return keymap[sc];
}