// Copyright 2024 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <hal.h>

#if HAL_USE_TRNG

#    include "hal_trng.h"

static void prng_init(void) {
    static bool prng_initialized = false;
    if (!prng_initialized) {
        prng_initialized = true;
        trngStart(&TRNGD1, NULL);
    }
}

bool prng_generate(void *buf, size_t n) {
    prng_init();
    bool err = trngGenerate(&TRNGD1, n, (uint8_t *)buf);
    return err;
}

// Signal to `prng.c` that it doesn't need to define its own `prng_generate`
#    define PRNG_GENERATE_DEFINED

#endif
