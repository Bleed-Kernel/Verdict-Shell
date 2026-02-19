#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} theme_color_t;

void theme_init(void);
int theme_load(const char *path);
const char *theme_active_path(void);

theme_color_t theme_primary(void);
theme_color_t theme_secondary(void);
theme_color_t theme_background(void);
theme_color_t theme_text(void);

const char *theme_primary_fg(void);
const char *theme_secondary_fg(void);
const char *theme_background_bg(void);
const char *theme_text_fg(void);
