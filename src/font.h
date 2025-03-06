#pragma once

#include <stdint.h>

#ifndef ___FONT_H
#define ___FONT_H

typedef struct Font {
    uint8_t first_char;
    uint8_t num_chars;
    uint8_t char_width;
    uint8_t char_height;
    uint16_t size;
    const char* name;
    const uint8_t* font_array;
} Font;

extern Font font_ubuntu_mono_16x24;
extern Font font_jetbrains_mono_16x24;

#endif
