# Tarefa: Roteiro de FreeRTOS #2 - EmbarcaTech 2025

**Autores:** Guilherme Westphall & Vitor Feijó </br>
**Curso:** Residência Tecnológica em Sistemas Embarcados </br>
**Instituição:** EmbarcaTech - HBr </br>
📍 Brasília, 04 de junho de 2025

---

## 🎯 Objetivo do Projeto

Desenvolver um **cofre digital** com suporte a multitarefas usando **FreeRTOS**, com entrada via **teclado matricial** e interface em **display OLED SSD1306**, permitindo:

- Registro e confirmação de senha na memória flash;
- Tentativas de acesso com feedback visual;
- Bloqueio após três tentativas incorretas;
- Reset da senha com botão físico;
- Feedback por LED e buzzer.

---

## 🛠️ Componentes Utilizados

| Componente                 | Função                                        | Observações                        |
| -------------------------- | --------------------------------------------- | ---------------------------------- |
| BitDogLab (Rasp Pi Pico W) | Plataforma principal                          | Executa o FreeRTOS                 |
| Teclado Matricial 4x3      | Entrada da senha                              | Utilizado para digitação           |
| Display OLED SSD1306       | Interface visual do sistema                   | Comunicação via I2C (GPIO 14 e 15) |
| LEDs RGB                   | Feedback de status (sucesso/erro/reset)       | Verde, vermelho e azul             |
| Buzzer                     | Alerta sonoro para interação                  | Ativado ao pressionar teclas       |
| Botões físicos (A/B)       | A: Reset de senha, B: Visualizar senha/travar | Entrada digital com pull-up        |
| Memória Flash interna      | Armazenamento persistente da senha            | Escrita via `flash_range_program`  |

---

## 🗂 Estrutura do Projeto

```bash
.
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── main.c
├── README.md
│
├── include/
│   ├── display.h
│   ├── flashpswd.h
│   ├── FreeRTOSConfig.h
│   ├── matrixkey.h
│   ├── ssd1306.h
│   ├── ssd1306_font.h
│   └── ssd1306_i2c.h
│
└── src/
    ├── display.c
    ├── flashpswd.c
    └── ssd1306_i2c.c
    ├── matrixkey.c
```

---

## ⚙️ Como Executar

### 1. Conecte os os seguintes componentes no GPIOs:

- Teclado Matricial: ROWS (18, 16, 19, 17), COLS (4, 20, 9)
- Display OLED: SDA (GPIO 14), SCL (GPIO 15)
- LEDs: R (13), G (11), B (12)
- Buzzer: GPIO 21
- Botões: A (GPIO 5), B (GPIO 6)

### 2. Compile o projeto

```bash
mkdir build
cd build
cmake ..
make
```

### 3. Embarque o .uf2 gerado na BitDogLab via USB.

---

## 🚪 Fluxo do Cofre Digital

### 1. Registro e Confirmação da Senha

- Ao iniciar sem senha gravada, o sistema entra no modo de cadastro.
- O usuário digita uma senha de 6 dígitos e a confirma.
- Se as senhas coincidirem, ela é gravada na memória flash com persistência.
- A senha só é aceita se for composta por números de '0' a '9'.

### 2. Tentativas de Acesso

- Com uma senha já registrada, o sistema solicita o acesso.
- O usuário tem 3 tentativas para digitar a senha corretamente.
- Cada erro reduz o contador e exibe feedback visual.
- Após 3 erros, o sistema exibe LOCKED OUT e trava.

### 3. Acesso Concedido

- Com a senha correta, o sistema exibe ACCESS GRANTED.
- A interface passa a mostrar:
- BTN A RESET – Apagar a senha e voltar ao início;
- BTN B LOCK – Rebloquear o cofre, exigindo nova digitação.

### 4. Reset da Senha

- Ao pressionar BTN A, a senha armazenada é apagada da memória flash.
- O sistema retorna ao estado inicial de cadastro.

---

## 🔄 Tarefas RTOS

|    Tarefa     |              Responsabilidade               |
| :-----------: | :-----------------------------------------: |
|  task_input   |   Coleta senha e confirmação via teclado    |
|  task_verify  |  Verifica senha com contador de tentativas  |
| task_unlocked | Mostra menu pós-desbloqueio (reset ou lock) |
|  task_vault   |     Gerencia o estado global do sistema     |

---

## 📜 Licença

GNU GPL-3.0.
