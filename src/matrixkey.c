#include "matrixkey.h"

const uint8_t ROW_PINS[ROWS_SIZE] = {18, 16, 19, 17};
const uint8_t COL_PINS[COLS_SIZE] = {4, 20, 9};

const char keyboard_map[ROWS_SIZE][COLS_SIZE] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

void init_matrix_keypad()
{
    for (int i = 0; i < ROWS_SIZE; i++)
    {
        gpio_init(ROW_PINS[i]);
        gpio_set_dir(ROW_PINS[i], GPIO_OUT);
        gpio_put(ROW_PINS[i], 1);
    }

    for (int i = 0; i < COLS_SIZE; i++)
    {
        gpio_init(COL_PINS[i]);
        gpio_set_dir(COL_PINS[i], GPIO_IN);
        gpio_pull_up(COL_PINS[i]);
    }
}

char read_digit(const uint8_t *rows, const uint8_t *cols)
{
    for (int l = 0; l < ROWS_SIZE; l++)
    {
        for (int i = 0; i < ROWS_SIZE; i++)
            gpio_put(ROW_PINS[i], 1);

        gpio_put(ROW_PINS[l], 0);
        sleep_us(3);

        for (int c = 0; c < COLS_SIZE; c++)
        {
            if (gpio_get(COL_PINS[c]) == 0)
            {
                while (gpio_get(COL_PINS[c]) == 0)
                    tight_loop_contents();

                return keyboard_map[l][c];
            }
        }
    }
    return '\0';
}

void click_feedback(uint led_gpio, uint buzzer_gpio, uint delay_ms) {
    gpio_put(led_gpio, 1);
    gpio_put(buzzer_gpio, 1);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    gpio_put(led_gpio, 0);
    gpio_put(buzzer_gpio, 0);
}

/* bool read_matrix_step(char *pswd, size_t pswd_size,int *idx, uint led_gpio, uint buzzer_gpio)
{
    char digit = read_digit(ROW_PINS, COL_PINS);
    if (digit != '\0' && *idx < pswd_size)
    {
        pswd[*idx] = digit;
        (*idx)++;
        pswd[*idx] = '\0';
        click_feedback(led_gpio, buzzer_gpio, 100);
        return true; // houve atualização
    }
    return false; // nada lido
}
 */
/* void read_matrix(char *pswd, const char *msg, uint led_gpio, uint buzzer_gpio)
{
    int idx = 0;
    char digit;

    while (idx < PASSWORD_SIZE)
    {
        digit = read_digit(ROW_PINS, COL_PINS);
        if (digit != '\0')
        {
            pswd[idx++] = digit;
            click_feedback(led_gpio, buzzer_gpio, 100);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    pswd[idx] = '\0'; 
} */

