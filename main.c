#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "task.h"
#include "ssd1306.h"
#include "ssd1306_i2c.h"

// === Configurações do I2C e display ===
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C

// === Tamanho da tela ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// === Buffer da senha ===
#define MAX_DIGITOS 16
char senha_digitada[MAX_DIGITOS + 1] = {0};

// === Estrutura do display ===
ssd1306_t display;
struct render_area frame_area;

// === Inicializa o display ===
void init_display() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init_bm(&display, SCREEN_WIDTH, SCREEN_HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&display);  // ESSENCIAL: liga o display

    // Configura a área de renderização
    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&frame_area);

    // Mensagem inicial
    memset(display.ram_buffer, 0, ssd1306_buffer_length);
    ssd1306_draw_string(display.ram_buffer, 0, 0, "Digite a senha:");
    render_on_display(display.ram_buffer, &frame_area);
}

// === Tarefa: Atualiza o display ===
void task_display(void *params) {
    char *buffer = (char *)params;

    while (1) {
        memset(display.ram_buffer, 0, ssd1306_buffer_length);
        ssd1306_draw_string(display.ram_buffer, 0, 0, "Senha:");
        ssd1306_draw_string(display.ram_buffer, 0, 16, buffer);
        render_on_display(display.ram_buffer, &frame_area);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// === main ===
int main() {
    stdio_init_all();
    init_display();

    // Inicia a task de exibir a senha
    xTaskCreate(task_display, "Display", 512, senha_digitada, 1, NULL);

    // Inicia o scheduler
    vTaskStartScheduler();

    // Nunca deve chegar aqui
    while (1);
}
