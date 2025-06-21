#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "ssd1306.h"
#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "FreeRTOS.h"
#include "task.h"
#include "matrixkey.h"
#include "display.h"
#include "flashpswd.h"

#define R_LED 13
#define G_LED 11
#define BTN_A 5
#define BTN_B 6
#define BUZZER 21
#define PASSWORD_SIZE 6
#define FLASH_TARGET_OFFSET 0x1F000

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

TaskHandle_t input_task_handle = NULL;
TaskHandle_t verify_task_handle = NULL;
TaskHandle_t vault_task_handle = NULL;

struct render_area frame = {
    .start_column = 0,
    .end_column = ssd1306_width - 1,
    .start_page = 0,
    .end_page = ssd1306_n_pages - 1,
};

uint8_t *ssd;

extern const uint8_t ROW_PINS[ROWS_SIZE];
extern const uint8_t COL_PINS[COLS_SIZE];
extern const char keyboard_map[ROWS_SIZE][COLS_SIZE];

const uint8_t *flash_pswd = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
char pswd[PASSWORD_SIZE + 1] = {0};

void task_input(void *params)
{
    char pswd1[PASSWORD_SIZE + 1] = {0};
    char pswd2[PASSWORD_SIZE + 1] = {0};
    int idx1 = 0;
    int idx2 = 0;
    bool confirming = false; // false: estamos coletando pswd1, true: pswd2

    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 0, 0, "ENTER PASSWORD");
    render_on_display(ssd, &frame);

    while (true)
    {
        char digit = read_digit(ROW_PINS, COL_PINS);
        if (!confirming)
        {
            if (digit != '\0' && idx1 < PASSWORD_SIZE)
            {
                pswd1[idx1++] = digit;
                pswd1[idx1] = '\0';
                click_feedback(R_LED, BUZZER, 100);
            }

            bool show_pswd = (gpio_get(BTN_B) == 0);
            draw_pswd(ssd, ssd1306_buffer_length, &frame, pswd1, idx1, 5, 32, show_pswd);

            if (idx1 == PASSWORD_SIZE)
            {
                confirming = true;
                idx2 = 0;
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 0, "CONFIRM PASSWORD");
                render_on_display(ssd, &frame);
                vTaskDelay(pdMS_TO_TICKS(500)); // Pequeno delay visual
            }
        }
        else
        {
            if (digit != '\0' && idx2 < PASSWORD_SIZE)
            {
                pswd2[idx2++] = digit;
                pswd2[idx2] = '\0';
                click_feedback(R_LED, BUZZER, 100);
            }

            bool show_pswd = (gpio_get(BTN_B) == 0);
            draw_pswd(ssd, ssd1306_buffer_length, &frame, pswd2, idx2, 5, 32, show_pswd);

            if (idx2 == PASSWORD_SIZE)
            {
                if (strncmp(pswd1, pswd2, PASSWORD_SIZE) == 0)
                {
                    flash_write_pswd(pswd1, PASSWORD_SIZE);
                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 0, 0, "PASSWORD SAVED");
                    render_on_display(ssd, &frame);
                    gpio_put(G_LED, 1);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    gpio_put(G_LED, 0);

                    vTaskResume(verify_task_handle);
                    vTaskDelete(NULL);
                }
                else
                {
                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 0, 0, "DOES NOT MATCH");
                    render_on_display(ssd, &frame);
                    gpio_put(R_LED, 1);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    gpio_put(R_LED, 0);

                    memset(pswd1, 0, sizeof(pswd1));
                    memset(pswd2, 0, sizeof(pswd2));
                    idx1 = 0;
                    idx2 = 0;
                    confirming = false;

                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 0, 0, "ENTER PASSWORD");
                    render_on_display(ssd, &frame);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Delay para aliviar o scheduler
    }
}

void task_verify(void *params)
{
    char attempt[PASSWORD_SIZE + 1] = {0};
    int try_count = 4;
    int idx = 0;

    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 0, 0, "TRY PASSWORD");
    render_on_display(ssd, &frame);
    
    while (true)
    {
        char digit = read_digit(ROW_PINS, COL_PINS);

        if (digit != '\0' && idx < PASSWORD_SIZE)
        {
            attempt[idx++] = digit;
            attempt[idx] = '\0';
            click_feedback(R_LED, BUZZER, 100);
        }

        bool show_pswd = (gpio_get(BTN_B) == 0);
        draw_pswd(ssd, ssd1306_buffer_length, &frame, attempt, idx, 5, 32, show_pswd);

        if (idx == PASSWORD_SIZE)
        {
            if (pswd_matches(attempt, flash_pswd))
            {
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 0, "ACCESS GRANTED");
                render_on_display(ssd, &frame);
                gpio_put(G_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(1500));
                gpio_put(G_LED, 0);

                idx = 0; // Reset index for next attempt
                memset(attempt, 0, sizeof(attempt));
            }
            else
            {
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 0, "ACCESS DENIED");
                render_on_display(ssd, &frame);
                gpio_put(R_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(1500));
                gpio_put(R_LED, 0);

                try_count--;
                if (try_count <= 0)
                {
                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 0, 0, "LOCKED OUT");
                    render_on_display(ssd, &frame);
                    vTaskSuspend(verify_task_handle); // Suspende a tarefa de verificação
                    return;
                }

                idx = 0; 

                memset(attempt, 0, sizeof(attempt));
                memset(ssd, 0, ssd1306_buffer_length);
                
                char msg[32];
                snprintf(msg, sizeof(msg), "TRIES LEFT: %d", try_count);
                ssd1306_draw_string(ssd, 0, 0, msg);
                render_on_display(ssd, &frame);
            }
        }
    }
}

void task_vault(void *params)
{   
    while (true)
    {
        if (!flash_pswd_exists(flash_pswd))
        {
            vTaskSuspend(verify_task_handle); // Suspende a tarefa se não houver 
        } 
        else {
            vTaskResume(verify_task_handle); // Resume a tarefa se houver
        }

        if (flash_pswd_exists(flash_pswd))
        {
            vTaskSuspend(input_task_handle); // Suspende a tarefa de entrada se a senha já existir
        }
        else
        {
            vTaskResume(input_task_handle); // Resume a tarefa de entrada se a senha não existir
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // Delay para aliviar o scheduler
    }
}

int main()
{
    stdio_init_all();

    init_matrix_keypad();

    gpio_init(R_LED);
    gpio_set_dir(R_LED, GPIO_OUT);
    gpio_put(R_LED, 0);

    gpio_init(G_LED);
    gpio_set_dir(G_LED, GPIO_OUT);
    gpio_put(G_LED, 0);

    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 0);

    i2c_init(I2C_PORT, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    ssd1306_init();

    calculate_render_area_buffer_length(&frame);
    // uint8_t ssd[ssd1306_buffer_length];
    ssd = (uint8_t *)malloc(ssd1306_buffer_length);
    if (ssd == NULL)
    {
        printf("Failed to allocate memory for display buffer\n");
        gpio_put(R_LED, 1);
        sleep_ms(2000);
        return -1;
    }
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame);

    gpio_put(G_LED, 1);
    sleep_ms(1500);
    gpio_put(G_LED, 0);

    xTaskCreate(task_input, "Input Task", 2048, NULL, 1, &input_task_handle);
    xTaskCreate(task_verify, "Verify Task", 2048, NULL, 1, &verify_task_handle);
    xTaskCreate(task_vault, "Vault Task", 2048, NULL, 2, &vault_task_handle);
    vTaskStartScheduler();

    while (true) tight_loop_contents();
}