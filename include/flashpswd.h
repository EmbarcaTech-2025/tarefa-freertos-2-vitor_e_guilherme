#ifndef FLASH_H
#define FLASH_H

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <string.h>

#define FLASH_TARGET_OFFSET 0x1F000
#define FLASH_SIZE 4096
#define PASSWORD_SIZE 6

void flash_write_pswd(const char *password, size_t length);
void flash_erase_pswd(size_t length);
bool flash_pswd_exists(const uint8_t *flash_pswd);
bool pswd_matches(const char *input_pswd, const uint8_t *flash_pswd);

#endif