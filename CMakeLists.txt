# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# Caminho do FreeRTOS Kernel
if (DEFINED ENV{FREERTOS_PATH})
    set(FREERTOS_PATH $ENV{FREERTOS_PATH})
else()
    set(FREERTOS_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS)
endif()

message("FreeRTOS Kernel located in ${FREERTOS_PATH}")

# Import those libraries
include(pico_sdk_import.cmake)
include(${FREERTOS_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Define project
project(embarcatech-tarefa-freertos-2 C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(embarcatech-tarefa-freertos-2
    main.c
    src/ssd1306_i2c.c
)

target_include_directories(embarcatech-tarefa-freertos-2 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${FREERTOS_PATH}/include
    ${FREERTOS_PATH}/portable
    ${FREERTOS_PATH}/portable/ThirdParty/GCC/RP2040/include
)

# Adiciona arquivos-fonte do kernel
target_sources(embarcatech-tarefa-freertos-2 PRIVATE
    ${FREERTOS_PATH}/tasks.c
    ${FREERTOS_PATH}/queue.c
    ${FREERTOS_PATH}/list.c
    ${FREERTOS_PATH}/timers.c
    ${FREERTOS_PATH}/event_groups.c
    ${FREERTOS_PATH}/stream_buffer.c
    ${FREERTOS_PATH}/croutine.c
    ${FREERTOS_PATH}/portable/ThirdParty/GCC/RP2040/port.c
    ${FREERTOS_PATH}/portable/MemMang/heap_4.c
)

target_link_libraries(embarcatech-tarefa-freertos-2 
    pico_stdlib
    hardware_i2c
    hardware_irq
    hardware_timer
    hardware_sync
    hardware_exception
    hardware_structs
    hardware_gpio
    hardware_clocks
    hardware_flash
)

pico_add_extra_outputs(embarcatech-tarefa-freertos-2)


