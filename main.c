#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "FreeRTOS.h"
#include "task.h"

#define LINHAS 4
#define COLUNAS 3

uint linhas[LINHAS] = {18, 16, 19, 17};
uint colunas[COLUNAS] = {4, 20, 9};

char mapa_teclado[LINHAS][COLUNAS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

#define LED_VERMELHO 13
#define BUZZER 21
#define TAMANHO_SENHA 6
#define FLASH_TARGET_OFFSET 0x1F000

const uint8_t *flash_senha = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
char senha_memoria[TAMANHO_SENHA + 1] = {0}; // senha persistida

void init_gpio() {
    for (int i = 0; i < LINHAS; i++) {
        gpio_init(linhas[i]);
        gpio_set_dir(linhas[i], GPIO_OUT);
        gpio_put(linhas[i], 1);
    }

    for (int i = 0; i < COLUNAS; i++) {
        gpio_init(colunas[i]);
        gpio_set_dir(colunas[i], GPIO_IN);
        gpio_pull_up(colunas[i]);
    }

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_put(LED_VERMELHO, 0);

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, 0);
}

char ler_tecla() {
    for (int l = 0; l < LINHAS; l++) {
        for (int i = 0; i < LINHAS; i++) gpio_put(linhas[i], 1);
        gpio_put(linhas[l], 0);
        sleep_us(3);

        for (int c = 0; c < COLUNAS; c++) {
            if (gpio_get(colunas[c]) == 0) {
                while (gpio_get(colunas[c]) == 0) tight_loop_contents();
                return mapa_teclado[l][c];
            }
        }
    }
    return '\0';
}

void feedback() {
    gpio_put(LED_VERMELHO, 1);
    gpio_put(BUZZER, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_put(LED_VERMELHO, 0);
    gpio_put(BUZZER, 0);
}

void aguardar_e_capturar(char *destino, const char *msg) {
    printf("%s", msg);
    fflush(stdout);
    int pos = 0;
    while (pos < TAMANHO_SENHA) {
        char tecla = ler_tecla();
        if (tecla != '\0') {
            destino[pos++] = tecla;
            printf("*"); fflush(stdout);
            feedback();
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    destino[pos] = '\0';
    printf("\n");
}

void escrever_flash(const char *senha) {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t *)senha, TAMANHO_SENHA);
    restore_interrupts(ints);
}

bool senha_persistida_existe() {
    for (int i = 0; i < TAMANHO_SENHA; i++) {
        if (flash_senha[i] < '0' || flash_senha[i] > '9') return false;
    }
    return true;
}

bool senha_valida(const char *tentativa) {
    return strncmp(tentativa, (const char *)flash_senha, TAMANHO_SENHA) == 0;
}

void task_cofre(void *params) {
    char senha1[TAMANHO_SENHA + 1] = {0};
    char senha2[TAMANHO_SENHA + 1] = {0};
    char tentativa[TAMANHO_SENHA + 1] = {0};

    if (!senha_persistida_existe()) {
        // Cadastro
        while (true) {
            aguardar_e_capturar(senha1, "Cadastre uma senha (6 digitos): ");
            aguardar_e_capturar(senha2, "Confirme a senha: ");

            if (strncmp(senha1, senha2, TAMANHO_SENHA) == 0) {
                escrever_flash(senha1);
                printf("Senha cadastrada com sucesso!\n");
                break;
            } else {
                printf("As senhas não coincidem. Tente novamente.\n");
            }
        }
    }

    // Modo travado
    while (true) {
        aguardar_e_capturar(tentativa, "Digite a senha para desbloquear: ");

        if (senha_valida(tentativa)) {
            printf("Cofre desbloqueado!\n");
            // Aqui você pode executar uma função que abre o cofre
        } else {
            printf("Senha incorreta!\n");
        }
    }
}

int main() {
    stdio_init_all();
    sleep_ms(10000);

    init_gpio();
    xTaskCreate(task_cofre, "Cofre", 2048, NULL, 1, NULL);
    vTaskStartScheduler();

    while (1) tight_loop_contents();
}
