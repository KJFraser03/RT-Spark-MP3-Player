#ifndef NHD_0216HZ_H
#define NHD_0216HZ_H

#include "stm32f4xx_hal.h"

class NHD_0216HZ {
public:
    NHD_0216HZ();

    void init_lcd(void);
    void write_cmd(uint8_t data);
    void write_data(uint8_t data);
    void printf(const char *format, ...);
    void set_cursor(int column, int row);
    void clr_lcd(void);
    void set_pixel(uint16_t x, uint16_t y, uint16_t color);
    void set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

private:
    uint16_t cursor_x;
    uint16_t cursor_y;
    void draw_char(char c, uint16_t x, uint16_t y, uint16_t color, uint16_t bg);
};

#endif