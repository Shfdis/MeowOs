#pragma once
#include "types/types.h"

class Framebuffer {
private:
    uint16_t (*buffer_)[80];
    int row_;
    int column_;
    uint16_t color_;

public:
    Framebuffer();
    explicit Framebuffer(void* vga_buffer);

    void init();
    void putchar(char c);
    void puts(const char* str);
    void write(const void* buf, unsigned int size);
    void clear();
    void scroll();

    void set_color(uint8_t fg, uint8_t bg);
    void set_cursor(int row, int col);
    int get_row() const;
    int get_column() const;

    void* raw_buffer();
    const void* raw_buffer() const;
};

extern Framebuffer g_framebuffer;
