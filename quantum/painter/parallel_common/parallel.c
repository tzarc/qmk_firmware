#include "parallel.h"
#include <hal_pal.h>

uint16_t bitmask = 0;
pin_t pins[16] = 0;
uint8_t port_width;
ioportid_t pin_port;
pin_t w_pin;
pin_t r_pin;
bool waiting = false;
const uint16_t nanoseconds_per_nop = 1000000000 / STM32_SYSCLK;
int32_t wait_cycles = 0;

void wait_ns(uint16_t nanoseconds) {
    wait_cycles = (nanoseconds/nanoseconds_per_nop) - 2;
    wait_cpuclock(wait_cycles > 0 ? wait_cycles : 0);
}

void parallel_init(pin_t write_pin, pin_t read_pin, const pin_t* data_pins, uint8_t data_pin_count) {
    static bool is_initialised = false;
    if (!is_initialised) {
        w_pin = write_pin;
        r_pin = read_pin;
        if (bitmask == 0) {
            memcpy(pins, data_pins, sizeof(pin_t)*data_pin_count);
            pin_port = PAL_PORT(pins[0]);
            for (uint8_t i = 0; i < data_pin_count ; i++) {
               bitmask |= 0x1 << pins[i];
            }
        }
        setPinOutput(w_pin);
        setPinOutput(r_pin);
        writePinHigh(w_pin);
        writePinHigh(r_pin);
        port_width = data_pin_count;
        is_initialised = true;
    }
}

bool parallel_start(pin_t chip_select_pin) {
    setPinOutput(chip_select_pin);
    writePinLow(chip_select_pin);
    return true;
}

void internal_write_word(uint16_t word) {
    internal_write_word(word, PARALLEL_WRITE_DELAY_NS, PARALLEL_POST_WRITE_DELAY_NS);
}

void internal_write_byte(uint8_t byte) {
    internal_write_word(byte, PARALLEL_WRITE_DELAY_NS, PARALLEL_POST_WRITE_DELAY_NS);
}

void internal_write_word(uint16_t word, uint16_t pre_delay, uint16_t post_delay) {
    static uint16_t bits = 0;

    writePinLow(w_pin);

    for (uint8_t i = 0; i < port_width; i++) {
        bits |= ((word >> i ) & 0x1) << pins[i];
    }

    wait_ns(pre_delay);
    palWriteGroup(pin_port, bitmask, 0, bits);
    writePinHigh(w_pin);
    wait_ns(post_delay);
}

void internal_write_init() {
    writePinHigh(r_pin);
    palSetGroupMode(pin_port, bitmask, 0, PAL_MODE_OUTPUT_PUSHPULL);
}

bool parallel_write(uint16_t word) {
    internal_write_init();
    internal_write_word(word);
}

void internal_read_init() {
    writePinHigh(w_pin);
    palSetGroupMode(pin_port, bitmask, 0, PAL_MODE_INPUT);
}

uint16_t parallel_read() {
    writePinLow(r_pin);

    uint16_t bits = 0;
    wait_ns(PARALLEL_READ_DELAY_NS);
    uint16_t portbits = palReadGroup(pin_port, bitmask, 0);
    start_wait_timer();
    for (uint8_t i = 0; i < port_width ; i++) {
        bits |= ((portbits >> PAL_PAD(pins[i])) & 0x1 ) << i;
    }
    writePinHigh(r_pin);
    wait_timer_elapse_ns(PARALLEL_POST_READ_DELAY_NS);
    return bits;
}

void parallel_stop() {
    bitmask = 0;
    pins = 0;
    writePinHigh(cs_pin);
}

void parallel_transmit(const uint8_t *data, uint16_t length) {
    internal_write_init();
    uint16_t data_n;
    static const uint8_t incrementer = port_width / sizeof(uint8_t);

    while (length > 0)
    {
        if(length < incrementer) {
            memcpy(data_n, data, length);
            length = 0;
            }
        else {
            memcpy(data_n, data, incrementer);
            *data += incrementer;
            length -= incrementer;
        }
        internal_write_word(data_n);
    }
}

