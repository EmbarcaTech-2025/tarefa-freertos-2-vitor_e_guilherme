#ifndef MATRIXKEY_H
#define MATRIXKEY_H

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#define ROWS_SIZE 4
#define COLS_SIZE 3

extern const uint8_t ROW_PINS[ROWS_SIZE];
extern const uint8_t COL_PINS[COLS_SIZE];
extern const char keyboard_map[ROWS_SIZE][COLS_SIZE];

void init_matrix_keypad();
char read_digit(const uint8_t *rows, const uint8_t *cols);
void click_feedback(uint led_gpio, uint buzzer_gpio, uint delay_ms);

//bool read_matrix_step(char *pswd, size_t pswd_size, int *idx, uint led_gpio, uint buzzer_gpio);

//void read_matrix(char *pswd, const char *msg, uint led_gpio, uint buzzer_gpio, uint8_t *ssd, bool visible);

#endif