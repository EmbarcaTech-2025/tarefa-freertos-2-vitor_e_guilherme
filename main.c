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
#include "semphr.h"

#define R_LED 13
#define B_LED 12
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
TaskHandle_t unlocked_task_handle = NULL;

struct render_area frame = {
    .start_column = 0,
    .end_column = ssd1306_width - 1,
    .start_page = 0,
    .end_page = ssd1306_n_pages - 1,
};

uint8_t *ssd;
volatile bool unlocked = false;

extern const uint8_t ROW_PINS[ROWS_SIZE];
extern const uint8_t COL_PINS[COLS_SIZE];
extern const char keyboard_map[ROWS_SIZE][COLS_SIZE];

const uint8_t *flash_pswd = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
char pswd[PASSWORD_SIZE + 1] = {0};

char *text[] = {
    "ENTER PASSWORD  ",
    "CONFIRM PASSWORD",
    "TRY PASSWORD    ",
    "ACCESS GRANTED  ",
    "ACCESS DENIED   ",
    "LOCKED OUT      ",
    "PASSWORD SAVED  ",
    "DOES NOT MATCH  "};

void task_input(void *params)
{
    char pswd1[PASSWORD_SIZE + 1] = {0};
    char pswd2[PASSWORD_SIZE + 1] = {0};
    int idx1 = 0;
    int idx2 = 0;
    bool confirming = false; // false: estamos coletando pswd1, true: pswd2

    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 0, 0, text[0]); // ENTER PASSWORD
    render_on_display(ssd, &frame);

    while (true)
    {
        char digit = read_digit(ROW_PINS, COL_PINS);
        if (!confirming)
        {
            if (digit != '\0' && idx < PASSWORD_SIZE && digit != '*' && digit != '#')
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
                ssd1306_draw_string(ssd, 0, 0, text[1]); // CONFIRM PASSWORD
                render_on_display(ssd, &frame);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
        else
        {
            if (digit != '\0' && idx < PASSWORD_SIZE && digit != '*' && digit != '#')
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
                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 5, 32, text[6]); // PASSWORD SAVED
                    render_on_display(ssd, &frame);

                    gpio_put(G_LED, 1);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    gpio_put(G_LED, 0);

                    flash_write_pswd(pswd1, PASSWORD_SIZE);
                    // vTaskResume(verify_task_handle);
                    vTaskSuspend(NULL);
                }
                else
                {
                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 5, 32, text[7]); // DOES NOT MATCH
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
                    ssd1306_draw_string(ssd, 0, 0, text[0]); // ENTER PASSWORD
                    render_on_display(ssd, &frame);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}

void task_verify(void *params)
{
    int idx = 0;
    int try_count = 4;
    char attempt[PASSWORD_SIZE + 1] = {0};

    while (true)
    {
        idx = 0;
        try_count = 4;
        memset(attempt, 0, sizeof(attempt));
        unlocked = false;

        memset(ssd, 0, ssd1306_buffer_length);
        ssd1306_draw_string(ssd, 0, 0, text[2]); // TRY PASSWORD
        render_on_display(ssd, &frame);

        while (!unlocked)
        {
            char digit = read_digit(ROW_PINS, COL_PINS);

            if (digit != '\0' && idx < PASSWORD_SIZE && digit != '*' && digit != '#')
            {
                attempt[idx++] = digit;
                attempt[idx] = '\0';
                click_feedback(R_LED, BUZZER, 100);

                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 0, text[2]); // TRY PASSWORD
                render_on_display(ssd, &frame);
            }

            bool show_pswd = (gpio_get(BTN_B) == 0);
            draw_pswd(ssd, ssd1306_buffer_length, &frame, attempt, idx, 5, 32, show_pswd);

            if (idx == PASSWORD_SIZE)
            {
                if (pswd_matches(attempt, flash_pswd))
                {
                    memset(ssd, 0, ssd1306_buffer_length);
                    ssd1306_draw_string(ssd, 5, 32, text[3]); // ACCESS GRANTED
                    render_on_display(ssd, &frame);

                    gpio_put(G_LED, 1);
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    gpio_put(G_LED, 0);

                    unlocked = true;
                }
                else
                {
                    try_count--;
                    memset(ssd, 0, ssd1306_buffer_length);

                    if (try_count <= 0)
                    {
                        ssd1306_draw_string(ssd, 5, 32, text[5]); // LOCKED OUT
                        render_on_display(ssd, &frame);
                        gpio_put(R_LED, 1);
                        vTaskSuspend(NULL);
                    }
                    else
                    {
                        ssd1306_draw_string(ssd, 5, 16, text[4]); // ACCESS DENIED
                        char msg[32];
                        snprintf(msg, sizeof(msg), "TRIES LEFT: %d", try_count);
                        ssd1306_draw_string(ssd, 5, 32, msg);
                        render_on_display(ssd, &frame);

                        gpio_put(R_LED, 1);
                        vTaskDelay(pdMS_TO_TICKS(1500));
                        gpio_put(R_LED, 0);

                        // Reseta para a próxima tentativa
                        idx = 0;
                        memset(attempt, 0, sizeof(attempt));

                        // Limpa a tela para a próxima tentativa
                        /* memset(ssd, 0, ssd1306_buffer_length);
                        ssd1306_draw_string(ssd, 0, 0, text[2]); // TRY PASSWORD
                        render_on_display(ssd, &frame); */
                    }
                }
            }
        }
        vTaskSuspend(NULL);
    }
}

void task_unlocked(void *params)
{
    while (true)
    {
        bool running = true;
        while (running)
        {
            // Espera BTN_B (bloquear) ou BTN_A (resetar)
            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 8, 8, "BTN A  RESET");
            ssd1306_draw_string(ssd, 8, 24, "BTN B  LOCK");
            render_on_display(ssd, &frame);

            if (gpio_get(BTN_B) == 0)
            {
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 32, 32, "LOCKED");
                render_on_display(ssd, &frame);

                gpio_put(R_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(1500));
                gpio_put(R_LED, 0);

                // Bloquear novamente
                unlocked = false;
                running = false;
            }

            if (gpio_get(BTN_A) == 0)
            {
                // Resetar senha
                flash_erase_pswd(PASSWORD_SIZE);
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 24, 24, "RESET DONE");
                render_on_display(ssd, &frame);
                gpio_put(B_LED, 1);
                vTaskDelay(pdMS_TO_TICKS(1500));
                gpio_put(B_LED, 0);

                unlocked = false;
                running = false; // Sair do loop e suspender a task
            }

            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskSuspend(NULL);
    }
}

void task_vault(void *params)
{
    vTaskSuspend(input_task_handle);
    vTaskSuspend(verify_task_handle);
    vTaskSuspend(unlocked_task_handle);

    while (true)
    {
        bool pswd_exists = flash_pswd_exists(flash_pswd);

        if (unlocked)
        {
            // Estado: Desbloqueado. Gerenciado pela task_unlocked.
            vTaskSuspend(input_task_handle);
            vTaskSuspend(verify_task_handle);
            vTaskResume(unlocked_task_handle);
        }
        else if (pswd_exists)
        {
            // Estado: Bloqueado, com senha. Gerenciado pela task_verify.
            vTaskSuspend(input_task_handle);
            vTaskSuspend(unlocked_task_handle);
            vTaskResume(verify_task_handle);
        }
        else // !unlocked && !pswd_exists
        {
            // Estado: Bloqueado, sem senha. Gerenciado pela task_input.
            vTaskSuspend(verify_task_handle);
            vTaskSuspend(unlocked_task_handle);
            vTaskResume(input_task_handle);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

int main()
{
    stdio_init_all();

    init_matrix_keypad();

    gpio_init(R_LED);
    gpio_set_dir(R_LED, GPIO_OUT);
    gpio_put(R_LED, 0);

    gpio_init(B_LED);
    gpio_set_dir(B_LED, GPIO_OUT);
    gpio_put(B_LED, 0);

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
    xTaskCreate(task_unlocked, "Unlocked Task", 2048, NULL, 1, &unlocked_task_handle);

    // Task Gerente maior prioridade
    xTaskCreate(task_vault, "Vault Task", 2048, NULL, 2, &vault_task_handle);

    vTaskStartScheduler();

    while (true)
        tight_loop_contents();
}