/* Copyright 2021 Nick Brassel (@tzarc)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define RLE_EOF ((int16_t)(-1))

typedef struct rle_stream_t rle_stream_t;
struct rle_stream_t {
    int16_t (*get)(rle_stream_t *in_stream);
    bool (*put)(rle_stream_t *out_stream, int16_t c);
};

typedef struct memory_rle_stream_t {
    rle_stream_t base;
    uint8_t *    buffer;
    size_t       length;
    size_t       position;
} memory_rle_stream_t;

#ifdef RLE_HAS_FILE_IO
typedef struct file_rle_stream_t {
    rle_stream_t base;
    FILE *       file;
} file_rle_stream_t;
#endif  // RLE_HAS_FILE_IO

memory_rle_stream_t make_memory_rle_stream_t(void *buffer, size_t length);

#ifdef RLE_HAS_FILE_IO
file_rle_stream_t make_file_rle_stream_t(FILE *f);
#endif  // RLE_HAS_FILE_IO

bool rle_decode(rle_stream_t *in_stream, rle_stream_t *out_stream);

#ifdef RLE_ENCODER
bool rle_encode(rle_stream_t *in_stream, rle_stream_t *out_stream);
#endif  // RLE_ENCODER
