#include "NHD_0216HZ.h"
#include "font8x8.h"
#include <stdio.h>
#include <stdarg.h>

#define LCD_REG  (*((volatile uint8_t *)0x68000000))
#define LCD_DATA (*((volatile uint8_t *)(0x68000000 | (1 << 18))))

NHD_0216HZ::NHD_0216HZ() {
    cursor_x = 0;
    cursor_y = 0;
}

void NHD_0216HZ::write_cmd(uint8_t cmd) { LCD_REG = cmd; }
void NHD_0216HZ::write_data(uint8_t data) { LCD_DATA = data; }

void NHD_0216HZ::set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    write_cmd(0x2A); write_data(x1 >> 8); write_data(x1 & 0xFF); write_data(x2 >> 8); write_data(x2 & 0xFF);
    write_cmd(0x2B); write_data(y1 >> 8); write_data(y1 & 0xFF); write_data(y2 >> 8); write_data(y2 & 0xFF);
    write_cmd(0x2C);
}

void NHD_0216HZ::init_lcd(void) {
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(120);

    write_cmd(0x11); HAL_Delay(120);
    write_cmd(0x36); write_data(0x00);
    write_cmd(0x3A); write_data(0x05);
    
    write_cmd(0xB2); write_data(0x0C); write_data(0x0C); write_data(0x00); write_data(0x33); write_data(0x33); 
    write_cmd(0xB7); write_data(0x35);
    write_cmd(0xBB); write_data(0x19);
    write_cmd(0xC0); write_data(0x2C);
    write_cmd(0xC2); write_data(0x01);
    write_cmd(0xC3); write_data(0x12);
    write_cmd(0xC4); write_data(0x20);
    write_cmd(0xC6); write_data(0x0F);
    write_cmd(0xD0); write_data(0xA4); write_data(0xA1);
    
    write_cmd(0xE0); 
    const uint8_t gp[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
    for(int i=0; i<14; i++) write_data(gp[i]);
    
    write_cmd(0xE1); 
    const uint8_t gn[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
    for(int i=0; i<14; i++) write_data(gn[i]);

    write_cmd(0x21);
    write_cmd(0x29);
    HAL_Delay(50);

    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_SET);
    clr_lcd();
}

void NHD_0216HZ::set_cursor(int column, int row) {
    cursor_x = column * 8;
    cursor_y = row * 16; 
}

void NHD_0216HZ::set_pixel(uint16_t x, uint16_t y, uint16_t color) {
    set_window(x, y, x, y);
    write_data(color >> 8); write_data(color & 0xFF);
}

void NHD_0216HZ::draw_char(char c, uint16_t x, uint16_t y, uint16_t color, uint16_t bg) {
    if (c < 32 || c > 127) c = 32;
    set_window(x, y, x + 7, y + 7);
    
    for (int i = 0; i < 8; i++) {
        uint8_t line = font8x8[c - 32][i];
        for (int j = 0; j < 8; j++) {
            if (line & (1 << j)) {
                write_data(color >> 8); write_data(color & 0xFF);
            } else {
                write_data(bg >> 8); write_data(bg & 0xFF);
            }
        }
    }
}

void NHD_0216HZ::printf(const char *format, ...) {
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '\n') {
            cursor_y += 16;
            cursor_x = 0;
            continue;
        }
        draw_char(buffer[i], cursor_x, cursor_y, 0xFFFF, 0x0000);
        cursor_x += 8;
        if (cursor_x >= 240) { cursor_x = 0; cursor_y += 16; }
    }
}

void NHD_0216HZ::clr_lcd(void) {
    set_window(0, 0, 239, 239);
    for(uint32_t i = 0; i < 240 * 240; i++) {
        write_data(0x00); write_data(0x00);
    }
    set_cursor(0, 0);
}