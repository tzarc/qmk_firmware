// Copyright 2024 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

bool     prng_generate(void *buf, size_t n);
uint8_t  prng8(void);
uint16_t prng16(void);
uint32_t prng32(void);
uint64_t prng64(void);

int8_t  prng_int8_range(int8_t min, int8_t max);
int16_t prng_int16_range(int16_t min, int16_t max);
int32_t prng_int32_range(int32_t min, int32_t max);
int64_t prng_int64_range(int64_t min, int64_t max);

uint8_t  prng_uint8_range(uint8_t min, uint8_t max);
uint16_t prng_uint16_range(uint16_t min, uint16_t max);
uint32_t prng_uint32_range(uint32_t min, uint32_t max);
uint64_t prng_uint64_range(uint64_t min, uint64_t max);

void srand(unsigned int seed);
int rand(void);
