#include "display.h"

void draw_pswd(uint8_t *ssd, size_t ssd_len, struct render_area *area, char *pswd, uint8_t pswd_len, int16_t x, int16_t y, bool visible)
{
    char buffer[pswd_len + 1];
    int len = strlen(pswd);

    for (int i = 0; i < len; i++)
    {
        buffer[i] = visible ? pswd[i] : 'x';
    }
    buffer[len] = '\0';

    ssd1306_draw_string(ssd, x, y, buffer);
    render_on_display(ssd, area);
}