#include "flashpswd.h"

void flash_write_pswd(const char *password, size_t length)
{
    if (length > PASSWORD_SIZE)
    {
        return; // Password too long
    }

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t *)password, PASSWORD_SIZE);
    restore_interrupts(ints);
}

void flash_erase_pswd(size_t length)
{
    if (length > PASSWORD_SIZE)
    {
        return; // Password too long
    }

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

bool flash_pswd_exists(const uint8_t *flash_pswd)
{
    for (size_t i = 0; i < PASSWORD_SIZE; i++)
        if (flash_pswd[i] < '0' || flash_pswd[i] > '9')
            return false;

    return true;
}

bool pswd_matches(const char *input_pswd, const uint8_t *flash_pswd)
{
    return strncmp(input_pswd, (const char *)flash_pswd, PASSWORD_SIZE) == 0;
}