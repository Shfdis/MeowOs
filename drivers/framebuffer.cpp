#include "framebuffer.h"
#include "types/cpu.h"

static constexpr int ROWS = 25;
static constexpr int COLS = 80;
static constexpr uintptr_t VGA_BASE = 0xB8000;
static constexpr uint16_t VGA_CRTC_INDEX = 0x3D4;
static constexpr uint16_t VGA_CRTC_DATA = 0x3D5;

Framebuffer g_framebuffer;

namespace {
inline void update_hw_cursor(int row, int col) {
    uint16_t pos = static_cast<uint16_t>(row * COLS + col);
    outb(VGA_CRTC_INDEX, 0x0F);
    outb(VGA_CRTC_DATA, static_cast<uint8_t>(pos & 0xFF));
    outb(VGA_CRTC_INDEX, 0x0E);
    outb(VGA_CRTC_DATA, static_cast<uint8_t>((pos >> 8) & 0xFF));
}
}

Framebuffer::Framebuffer()
    : buffer_(reinterpret_cast<uint16_t(*)[80]>(VGA_BASE)),
      row_(0),
      column_(0),
      color_(0x0F) {
}

Framebuffer::Framebuffer(void* vga_buffer)
    : buffer_(reinterpret_cast<uint16_t(*)[80]>(vga_buffer)),
      row_(0),
      column_(0),
      color_(0x0F) {
}

void Framebuffer::init() {
    buffer_ = reinterpret_cast<uint16_t(*)[80]>(VGA_BASE);
    row_ = 0;
    column_ = 0;
    color_ = 0x0F;
    update_hw_cursor(row_, column_);
}

void Framebuffer::putchar(char c) {
    if (c == '\n' || c == '\r') {
        row_++;
        column_ = 0;
        if (row_ >= ROWS) {
            scroll();
            row_ = ROWS - 1;
        }
        update_hw_cursor(row_, column_);
        return;
    }
    if (c == '\b') {
        if (column_ > 0) {
            column_--;
            buffer_[row_][column_] = ' ' | (color_ << 8);
        } else if (row_ > 0) {
            row_--;
            column_ = COLS - 1;
            buffer_[row_][column_] = ' ' | (color_ << 8);
        }
        update_hw_cursor(row_, column_);
        return;
    }
    if (column_ >= COLS) {
        column_ = 0;
        row_++;
        if (row_ >= ROWS) {
            scroll();
            row_ = ROWS - 1;
        }
    }
    buffer_[row_][column_] = (static_cast<uint8_t>(c) & 0xFF) | (color_ << 8);
    column_++;
    update_hw_cursor(row_, column_);
}

void Framebuffer::puts(const char* str) {
    while (*str) {
        putchar(*str++);
    }
}

void Framebuffer::write(const void* buf, unsigned int size) {
    const char* p = reinterpret_cast<const char*>(buf);
    for (unsigned int i = 0; i < size; i++) {
        putchar(p[i]);
    }
}

void Framebuffer::clear() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            buffer_[i][j] = ' ' | (color_ << 8);
        }
    }
    row_ = 0;
    column_ = 0;
    update_hw_cursor(row_, column_);
}

void Framebuffer::scroll() {
    for (int i = 0; i < ROWS - 1; i++) {
        for (int j = 0; j < COLS; j++) {
            buffer_[i][j] = buffer_[i + 1][j];
        }
    }
    for (int j = 0; j < COLS; j++) {
        buffer_[ROWS - 1][j] = ' ' | (color_ << 8);
    }
}

void Framebuffer::set_color(uint8_t fg, uint8_t bg) {
    color_ = (fg & 0x0F) | ((bg & 0x0F) << 4);
}

void Framebuffer::set_cursor(int row, int col) {
    row_ = row;
    column_ = col;
    if (row_ >= ROWS) row_ = ROWS - 1;
    if (row_ < 0) row_ = 0;
    if (column_ >= COLS) column_ = COLS - 1;
    if (column_ < 0) column_ = 0;
    update_hw_cursor(row_, column_);
}

int Framebuffer::get_row() const {
    return row_;
}

int Framebuffer::get_column() const {
    return column_;
}

void* Framebuffer::raw_buffer() {
    return buffer_;
}

const void* Framebuffer::raw_buffer() const {
    return buffer_;
}
