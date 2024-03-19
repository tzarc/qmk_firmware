// Copyright 2024 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later
#include "prng.h"

#if __has_include("_prng_impl.h")
#    include "_prng_impl.h"
#endif // __has_include("_prng_impl.h")

#ifndef PRNG_GENERATE_DEFINED

// https://www.pcg-random.org/posts/bob-jenkins-small-prng-passes-practrand.html + https://filterpaper.github.io/prng.html
#    define rot8(x, k) (((x) << (k)) | ((x) >> (8 - (k))))
static uint8_t jsf8(void) {
    static uint8_t a = 0xf1;
    static uint8_t b = 0xee, c = 0xee, d = 0xee;

    uint8_t e = a - rot8(b, 1);
    a         = b ^ rot8(c, 4);
    b         = c + d;
    c         = d + e;
    return d  = e + a;
}

bool prng_generate(void *buf, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        ((uint8_t *)buf)[i] = jsf8();
    }
    return true;
}

#endif // PRNG_GENERATE_DEFINED

uint8_t prng8(void) {
    uint8_t ret;
    prng_generate(&ret, sizeof(ret));
    return ret;
}

uint16_t prng16(void) {
    uint16_t r;
    prng_generate(&r, sizeof(r));
    return r;
}

uint32_t prng32(void) {
    uint32_t r;
    prng_generate(&r, sizeof(r));
    return r;
}

uint64_t prng64(void) {
    uint64_t r;
    prng_generate(&r, sizeof(r));
    return r;
}

#define PRNG_IMPLEMENT_RANGE_API(type_prefix, nbits)                                                                   \
    type_prefix##nbits##_t prng_##type_prefix##nbits##_range(type_prefix##nbits##_t min, type_prefix##nbits##_t max) { \
        if (min > max) {                                                                                               \
            type_prefix##nbits##_t tmp = min;                                                                          \
            min                        = max;                                                                          \
            max                        = tmp;                                                                          \
        }                                                                                                              \
        type_prefix##nbits##_t range = max - min;                                                                      \
        if (range == 0) {                                                                                              \
            return min;                                                                                                \
        }                                                                                                              \
        return min + (prng##nbits() % range);                                                                          \
    }

// int variants
PRNG_IMPLEMENT_RANGE_API(int, 8)
PRNG_IMPLEMENT_RANGE_API(int, 16)
PRNG_IMPLEMENT_RANGE_API(int, 32)
PRNG_IMPLEMENT_RANGE_API(int, 64)

// uint variants
PRNG_IMPLEMENT_RANGE_API(uint, 8)
PRNG_IMPLEMENT_RANGE_API(uint, 16)
PRNG_IMPLEMENT_RANGE_API(uint, 32)
PRNG_IMPLEMENT_RANGE_API(uint, 64)

void srand(unsigned int seed) {
    /* No-op. */
}

int rand(void) {
    return (int)prng32();
}
