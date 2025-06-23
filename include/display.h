#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"
#include "ssd1306.h"
#include <string.h>

#define DIGIT_SPACING 8

void draw_pswd(uint8_t *ssd, size_t ssd_len, struct render_area *frame, char *pswd, uint8_t pswd_len, int16_t x, int16_t y, bool visible);


#endif