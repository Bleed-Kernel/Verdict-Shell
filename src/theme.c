#include <theme.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <fs/file.h>

#define THEME_CONFIG_PATH "/initrd/etc/verdict/default.conf"

typedef struct {
    theme_color_t primary;
    theme_color_t secondary;
    theme_color_t background;
    theme_color_t foreground;
    char primary_fg[20];
    char secondary_fg[20];
    char background_bg[20];
    char foreground_fg[20];
} theme_state_t;

static theme_state_t g_theme = {
    .primary = {255, 255, 255},
    .secondary = {255, 255, 255},
    .background = {0, 0, 0},
    .foreground = {255, 255, 255},
    .primary_fg = "",
    .secondary_fg = "",
    .background_bg = "",
    .foreground_fg = ""
};

static const theme_state_t g_theme_defaults = {
    .primary = {212, 44, 44},
    .secondary = {128, 128, 128},
    .background = {0, 0, 0},
    .foreground = {255, 255, 255},
    .primary_fg = "",
    .secondary_fg = "",
    .background_bg = "",
    .foreground_fg = ""
};

static char g_theme_path[PATH_MAX] = THEME_CONFIG_PATH;

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s))
        s++;
    if (*s == '\0')
        return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

static int parse_triplet(const char *value, theme_color_t *out) {
    unsigned r = 0;
    unsigned g = 0;
    unsigned b = 0;
    if (sscanf(value, " %u , %u , %u ", &r, &g, &b) != 3)
        return -1;
    if (r > 255 || g > 255 || b > 255)
        return -1;
    out->r = (uint8_t)r;
    out->g = (uint8_t)g;
    out->b = (uint8_t)b;
    return 0;
}

static int parse_hex(const char *value, theme_color_t *out) {
    unsigned v = 0;
    if (sscanf(value, " #%6x ", &v) != 1)
        return -1;
    if (v > 0xFFFFFF)
        return -1;
    out->r = (uint8_t)((v >> 16) & 0xFFu);
    out->g = (uint8_t)((v >> 8) & 0xFFu);
    out->b = (uint8_t)(v & 0xFFu);
    return 0;
}

static void refresh_sequences(theme_state_t *state) {
    snprintf(state->primary_fg, sizeof(state->primary_fg),
             "\x1b[38;2;%u;%u;%um",
             (unsigned)state->primary.r,
             (unsigned)state->primary.g,
             (unsigned)state->primary.b);

    snprintf(state->secondary_fg, sizeof(state->secondary_fg),
             "\x1b[38;2;%u;%u;%um",
             (unsigned)state->secondary.r,
             (unsigned)state->secondary.g,
             (unsigned)state->secondary.b);

    snprintf(state->background_bg, sizeof(state->background_bg),
             "\x1b[48;2;%u;%u;%um",
             (unsigned)state->background.r,
             (unsigned)state->background.g,
             (unsigned)state->background.b);

    snprintf(state->foreground_fg, sizeof(state->foreground_fg),
             "\x1b[38;2;%u;%u;%um",
             (unsigned)state->foreground.r,
             (unsigned)state->foreground.g,
             (unsigned)state->foreground.b);
}

int theme_load(const char *path) {
    if (!path || !*path)
        return -1;

    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    theme_state_t next = g_theme_defaults;

    char filebuf[1024];
    size_t nread = fread(filebuf, 1, sizeof(filebuf) - 1, f);
    filebuf[nread] = '\0';
    fclose(f);

    char *save = NULL;
    for (char *line = strtok_r(filebuf, "\n", &save);
         line;
         line = strtok_r(NULL, "\n", &save)) {

        char *entry = trim(line);
        if (*entry == '\0' || *entry == '#')
            continue;

        char *eq = strchr(entry, '=');
        if (!eq)
            continue;

        *eq = '\0';

        char *key = trim(entry);
        char *value = trim(eq + 1);

        theme_color_t parsed;
        int ok = (parse_hex(value, &parsed) == 0) ||
                 (parse_triplet(value, &parsed) == 0);
        if (!ok)
            continue;

        if (strcmp(key, "primary") == 0) {
            next.primary = parsed;
        } else if (strcmp(key, "secondary") == 0) {
            next.secondary = parsed;
        } else if (strcmp(key, "background") == 0) {
            next.background = parsed;
        } else if (strcmp(key, "foreground") == 0) {
            next.foreground = parsed;
        }
    }

    refresh_sequences(&next);
    g_theme = next;

    strncpy(g_theme_path, path, sizeof(g_theme_path) - 1);
    g_theme_path[sizeof(g_theme_path) - 1] = '\0';

    return 0;
}

void theme_init(void) {
    if (theme_load(THEME_CONFIG_PATH) < 0) {
        g_theme = g_theme_defaults;
        refresh_sequences(&g_theme);
        strncpy(g_theme_path, THEME_CONFIG_PATH, sizeof(g_theme_path) - 1);
        g_theme_path[sizeof(g_theme_path) - 1] = '\0';
    }
}

theme_color_t theme_primary(void) {
    return g_theme.primary;
}

theme_color_t theme_secondary(void) {
    return g_theme.secondary;
}

theme_color_t theme_background(void) {
    return g_theme.background;
}

theme_color_t theme_text(void) {
    return g_theme.foreground;
}

const char *theme_primary_fg(void) {
    return g_theme.primary_fg;
}

const char *theme_secondary_fg(void) {
    return g_theme.secondary_fg;
}

const char *theme_background_bg(void) {
    return g_theme.background_bg;
}

const char *theme_text_fg(void) {
    return g_theme.foreground_fg;
}

const char *theme_active_path(void) {
    return g_theme_path;
}
