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

#include "rle.h"

int16_t mem_get(rle_stream_t *in) {
    memory_rle_stream_t *s = (memory_rle_stream_t *)in;
    int16_t              c = (s->buffer[s->position]);
    if (s->position >= s->length) return RLE_EOF;
    s->position++;
    return c;
}

#ifdef RLE_ENCODER
bool mem_put(rle_stream_t *out, int16_t c) {
    memory_rle_stream_t *s = (memory_rle_stream_t *)out;
    if (c == RLE_EOF || s->position == s->length) {
        return false;
    }
    s->buffer[s->position++] = (uint16_t)c;
    return true;
}
#endif  // RLE_ENCODER

memory_rle_stream_t make_memory_rle_stream_t(void *buffer, size_t length) {
    memory_rle_stream_t stream = {
        .base =
            {
                .get = mem_get,
#ifdef RLE_ENCODER
                .put = mem_put,
#endif  // RLE_ENCODER
            },
        .buffer   = (uint8_t *)buffer,
        .length   = length,
        .position = 0,
    };
    return stream;
}

#ifdef RLE_HAS_FILE_IO
int16_t file_get(rle_stream_t *in_stream) {
    file_rle_stream_t *s = (file_rle_stream_t *)in_stream;
    int                c = fgetc(s->file);
    if (c < 0 || feof(s->file)) return RLE_EOF;
    return (uint16_t)c;
}

#    ifdef RLE_ENCODER
bool file_put(rle_stream_t *out_stream, int16_t c) {
    file_rle_stream_t *s = (file_rle_stream_t *)out_stream;
    if(c == RLE_EOF) return false;
    return fputc(c, s->file) == c;
}
#    endif  // RLE_ENCODER

file_rle_stream_t make_file_rle_stream_t(FILE *f) {
    file_rle_stream_t stream = {
        .base =
            {
                .get = file_get,
#    ifdef RLE_ENCODER
                .put = file_put,
#    endif  // RLE_ENCODER
            },
        .file = f,
    };
    return stream;
}
#endif  // RLE_HAS_FILE_IO

void rle_decode(rle_stream_t *in_stream, rle_stream_t *out_stream) {
    int16_t (*get)(rle_stream_t *)       = in_stream->get;
    bool (*put)(rle_stream_t *, int16_t) = out_stream->put;
    int16_t c;
    size_t  i, cnt;
    while (1) {
        c = get(in_stream);
        if (c == RLE_EOF) return;
        if (c >= 128) {
            cnt = c - 128;
            for (i = 0; i < cnt; i++) {
                if (!put(out_stream, get(in_stream))) return;
            }
        } else {
            cnt = c;
            c   = get(in_stream);
            for (i = 0; i < cnt; i++) {
                if (!put(out_stream, c)) return;
            }
        }
    }
}

#ifdef RLE_ENCODER
static bool append(rle_stream_t *out_stream, uint8_t *buf, size_t len) {
    bool (*put)(rle_stream_t *, int16_t) = out_stream->put;
    int i;
    if (!put(out_stream, (int16_t)(128 + len))) return false;
    for (i = 0; i < len; i++) {
        if (!put(out_stream, buf[i])) return false;
    }
    return true;
}

bool rle_encode(rle_stream_t *in_stream, rle_stream_t *out_stream) {
    int16_t (*get)(rle_stream_t *)       = in_stream->get;
    bool (*put)(rle_stream_t *, int16_t) = out_stream->put;
    uint8_t buf[256];
    size_t  len    = 0;
    bool    repeat = false;
    bool    end    = false;
    int16_t c;

    while (!end) {
        end = ((c = get(in_stream)) == RLE_EOF);
        if (!end) {
            buf[len++] = (uint8_t)c;
            if (len <= 1) continue;
        }

        if (repeat) {
            if (buf[len - 1] != buf[len - 2]) repeat = false;
            if (!repeat || len == 128 || end) {
                if (!put(out_stream, (int16_t)(end ? len : len - 1))) return false;
                if (!put(out_stream, buf[0])) return false;
                buf[0] = buf[len - 1];
                len    = 1;
            }
        } else {
            if (buf[len - 1] == buf[len - 2]) {
                repeat = true;
                if (len > 2) {
                    if (!append(out_stream, buf, (int16_t)(len - 2))) return false;
                    buf[0] = buf[1] = buf[len - 1];
                    len             = 2;
                }
                continue;
            }
            if (len == 127 || end) {
                if (end) {
                    int fg = 0;
                }
                if (!append(out_stream, buf, (int16_t)len)) return false;
                len    = 0;
                repeat = false;
            }
        }
    }
    put(out_stream, RLE_EOF);
    return true;
}
#endif  // RLE_ENCODER
