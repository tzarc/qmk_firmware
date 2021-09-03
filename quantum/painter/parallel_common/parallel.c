#include "parallel.h"
#include <hal_pal.h>
#include <string.h>
#include "print.h"

#ifdef CONSOLE_ENABLE
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')
#endif // CONSOLE_ENABLE

#define CONVERT_PINS(a,b,c,d,e,f,g,h) \
    PAL_PAD(a), \
    PAL_PAD(b), \
    PAL_PAD(c), \
    PAL_PAD(d), \
    PAL_PAD(e), \
    PAL_PAD(f), \
    PAL_PAD(g), \
    PAL_PAD(h)
#define PINS_TO_PORT(a,b,c,d,e,f,g,h) PAL_PORT(a)

#define CONVERT_PINS_16(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) CONVERT_PINS(a,b,c,d,e,f,g,h), CONVERT_PINS(i,j,k,l,m,n,o,p)
#define PINS_TO_CONVERT PARALLEL_PORT_PINS
#define CONVERT8(...) CONVERT_PINS(__VA_ARGS__)
#define CONVERT16(...) CONVERT_PINS_16(__VA_ARGS__)
#define SHIFT_BITS(data,a,b,c,d,e,f,g,h) \
    (data & 0x1) << (a) | \
    ((data & 0x2) << (b - 1)) | \
    ((data & 0x4) << (c - 2)) | \
    ((data & 0x8) << (d - 3)) | \
    ((data & 0x10) << (e - 4)) | \
    ((data & 0x20) << (f - 5)) | \
    ((data & 0x40) << (g - 6)) | \
    ((data & 0x80) << (h - 7))
#define SHIFT8(...) SHIFT_BITS(__VA_ARGS__)

static uint16_t bitmask = SHIFT8(0xFFFF, CONVERT8(PARALLEL_PORT_PINS));
static bool read_initialized = false;
static bool write_initialized = false;
static const pin_t pins[PARALLEL_PORT_WIDTH] = {PARALLEL_PORT_PINS};

ioportid_t port_group;
pin_t w_pin;
pin_t r_pin;
pin_t cs_pin;
const uint16_t nanoseconds_per_nop = 1000000000 / STM32_SYSCLK;

bool parallel_init() {
    static bool is_initialised = false;
    port_group = PAL_PORT(pins[0]);
    if (!is_initialised) {

#ifdef CONSOLE_ENABLE
        dprintf("bitmask: "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY((bitmask & 0xFF00) >>8), BYTE_TO_BINARY(bitmask & 0xFF));
#endif // CONSOLE_ENABLE

        is_initialised = true;
    }
}

bool parallel_start(pin_t write_pin, pin_t read_pin, pin_t chip_select_pin) {
    r_pin = read_pin;
    w_pin = write_pin;
    cs_pin = chip_select_pin;
    setPinOutput(r_pin);
    setPinOutput(w_pin);
    setPinOutput(cs_pin);
    writePinHigh(r_pin);
    writePinHigh(w_pin);
    writePinLow(cs_pin);
    return true;
}

uint16_t internal_write(const uint8_t *data, uint16_t offset, uint16_t data_length) {
    static uint8_t *d;
    d = (uint8_t *)data + offset;
    static uint16_t bits;
    uint16_t remaining_bytes = data_length - offset;

    for(uint16_t i = 0 ; i < remaining_bytes ; i++) {
        bits = SHIFT8(*d, CONVERT8(PARALLEL_PORT_PINS));

        writePinLow(w_pin); // prepare to write

#ifdef CONSOLE_ENABLE
#ifdef DEBUG_BYTE_CONVERSION
        dprintf("original bits "BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY((*d)));
        dprintf("shifted bits "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN"\n", BYTE_TO_BINARY((bits & 0xFF00) >>8), BYTE_TO_BINARY(bits & 0xFF));
#endif // DEBUG_BYTE_CONVERSION
#endif // CONSOLE_ENABLE

        palWriteGroup(port_group, bitmask, 0, bits);
        writePinHigh(w_pin); // Commit the write
        d++;
    }
    return data_length;
}

void internal_write_init(void) {
    if (write_initialized) return;
    writePinHigh(r_pin);
    writePinLow(w_pin);
    palSetGroupMode(port_group, bitmask, 0, PAL_MODE_OUTPUT_PUSHPULL);
    write_initialized = true;
    read_initialized = false;
}

bool parallel_write(uint16_t word) {
    internal_write_init();
    return internal_write(&byte, 0, sizeof(uint8_t));
}

void internal_read_init(void) {
    if (read_initialized) return;
    writePinHigh(w_pin);
    writePinLow(r_pin);
    palSetGroupMode(port_group, bitmask, 0, PAL_MODE_INPUT);
    write_initialized = true;
    read_initialized = false;
}

// TODO: write optimized read method with buffered reads
// uint16_t parallel_read() {
//     internal_read_init();

//     uint16_t bits = 0;
//     wait_ms(1);
//     uint16_t portbits = palReadGroup(port_group, bitmask, 0);
//     for (uint8_t i = 0; i < port_width ; i++) {
//         bits |= ((portbits >> pin_shifts[i]) & 0x1 ) << i;
//     }
//     writePinHigh(r_pin);
//     wait_ms(1);
//     return bits;
// }

void parallel_stop() {
    writePinHigh(cs_pin);
    writePinHigh(w_pin);
    writePinHigh(r_pin);
    read_initialized = false;
    write_initialized = false;
}

void parallel_transmit(const uint8_t *data, uint16_t length) {
    internal_write_init();
    return internal_write(data, 0, length);
}

