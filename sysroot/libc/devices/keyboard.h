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

typedef enum KEYCODE {
    Null = 0,
    StartOfHeading = 1,
    StartOfText = 2,
    EndOfText = 3,
    EndOfTransmission = 4,
    Enquiry = 5,
    Acknowledge = 6,
    Bell = 7,
    Backspace = 8,
    HorizontalTab = 9,
    LineFeed = 10,
    VerticalTab = 11,
    FormFeed = 12,
    CarriageReturn = 13,
    ShiftOut = 14,
    ShiftIn = 15,
    DataLinkEscape = 16,
    DeviceControl1 = 17,
    DeviceControl2 = 18,
    DeviceControl3 = 19,
    DeviceControl4 = 20,
    NegativeAcknowledge = 21,
    SynchronousIdle = 22,
    EndOfTransmissionBlock = 23,
    Cancel = 24,
    EndOfMedium = 25,
    Substitute = 26,
    Escape = 27,
    FileSeparator = 28,
    GroupSeparator = 29,
    RecordSeparator = 30,
    UnitSeparator = 31,

    Space = 32,
    ExclamationMark = 33,
    DoubleQuote = 34,
    NumberSign = 35,
    DollarSign = 36,
    PercentSign = 37,
    Ampersand = 38,
    SingleQuote = 39,
    LeftParenthesis = 40,
    RightParenthesis = 41,
    Asterisk = 42,
    Plus = 43,
    Comma = 44,
    HyphenMinus = 45,
    Period = 46,
    Slash = 47,
    Zero = 48,
    One = 49,
    Two = 50,
    Three = 51,
    Four = 52,
    Five = 53,
    Six = 54,
    Seven = 55,
    Eight = 56,
    Nine = 57,
    Colon = 58,
    Semicolon = 59,
    LessThan = 60,
    Equals = 61,
    GreaterThan = 62,
    QuestionMark = 63,
    AtSign = 64,
    UpperA = 65,
    UpperB = 66,
    UpperC = 67,
    UpperD = 68,
    UpperE = 69,
    UpperF = 70,
    UpperG = 71,
    UpperH = 72,
    UpperI = 73,
    UpperJ = 74,
    UpperK = 75,
    UpperL = 76,
    UpperM = 77,
    UpperN = 78,
    UpperO = 79,
    UpperP = 80,
    UpperQ = 81,
    UpperR = 82,
    UpperS = 83,
    UpperT = 84,
    UpperU = 85,
    UpperV = 86,
    UpperW = 87,
    UpperX = 88,
    UpperY = 89,
    UpperZ = 90,
    LeftBracket = 91,
    Backslash = 92,
    RightBracket = 93,
    Caret = 94,
    Underscore = 95,
    GraveAccent = 96,
    LowerA = 97,
    LowerB = 98,
    LowerC = 99,
    LowerD = 100,
    LowerE = 101,
    LowerF = 102,
    LowerG = 103,
    LowerH = 104,
    LowerI = 105,
    LowerJ = 106,
    LowerK = 107,
    LowerL = 108,
    LowerM = 109,
    LowerN = 110,
    LowerO = 111,
    LowerP = 112,
    LowerQ = 113,
    LowerR = 114,
    LowerS = 115,
    LowerT = 116,
    LowerU = 117,
    LowerV = 118,
    LowerW = 119,
    LowerX = 120,
    LowerY = 121,
    LowerZ = 122,
    LeftBrace = 123,
    VerticalBar = 124,
    RightBrace = 125,
    Tilde = 126,
    Delete = 127,

    ArrowUp     = 0x48,
    ArrowDown   = 0x50,
    ArrowLeft   = 0x4B,
    ArrowRight  = 0x4D,
    Insert = 0x52,

    F1 = 132,
    F2 = 133,
    F3 = 134,
    F4 = 135,
    F5 = 136,
    F6 = 137,
    F7 = 138,
    F8 = 139,
    F9 = 140,
    F10 = 141,
    F11 = 142,
    F12 = 143
} KEYCODE;

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

static inline char tty_key_to_ascii(const keyboard_event_t *ev) {
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