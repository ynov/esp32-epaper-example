#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "font.h"

#ifndef __EPAPER_H
#define __EPAPER_H

#define SCREEN_WHITE 1
#define SCREEN_BLACK 0

void epaper_setup();
void epaper_deep_sleep();

void epaper_clear_screen();
void epaper_toggle_screen_color();
void epaper_white_screen();
void epaper_black_screen();

void epaper_dummy_screen();

void epaper_draw_text(uint16_t pos_x, uint16_t pos_y, const char* text, Font* font);
void epaper_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);

void epaper_set_pixel(uint16_t x, uint16_t y, uint8_t color);
void epaper_set_pixel_bits_8(uint16_t x, uint16_t y, uint8_t bits);

uint8_t epaper_get_pixel(uint16_t x, uint16_t y);
uint8_t epaper_get_pixel_bits_8(uint16_t x, uint16_t y);

#endif
