#include "timer.h"
#include "types/cpu.h"

namespace {

constexpr uint16_t PIT_COMMAND = 0x43;
constexpr uint16_t PIT_CHANNEL0 = 0x40;
constexpr uint32_t PIT_BASE_FREQ = 1193182;

}

void timer_init(uint32_t frequency) {
    uint32_t divisor = PIT_BASE_FREQ / frequency;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}
