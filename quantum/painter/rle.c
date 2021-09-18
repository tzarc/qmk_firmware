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

bool rle_decode(stream_t *in_stream, stream_t *out_stream) {
    int16_t (*get)(stream_t * stream)         = in_stream->get;
    bool (*put)(stream_t * stream, int16_t c) = out_stream->put;
    int16_t c;
    size_t  i, cnt;
    while (1) {
        c = get(in_stream);
        if (c == RLE_EOF) break;
        if (c >= 128) {
            cnt = c - 127;
            for (i = 0; i < cnt; i++) {
                c = get(in_stream);
                if (c == RLE_EOF) return false;
                if (!put(out_stream, c)) return false;
            }
        } else {
            cnt = c;
            c   = get(in_stream);
            if (c == RLE_EOF) return false;
            for (i = 0; i < cnt; i++) {
                if (!put(out_stream, c)) return false;
            }
        }
    }
    return true;
}

#ifdef RLE_ENCODER
static bool rle_append_bytes(stream_t *out_stream, uint8_t *buf, size_t len) {
    bool (*put)(stream_t * stream, int16_t c) = out_stream->put;
    int i;
    if (!put(out_stream, (int16_t)(127 + len))) return false;
    for (i = 0; i < len; i++) {
        if (!put(out_stream, buf[i])) return false;
    }
    return true;
}

bool rle_encode(stream_t *in_stream, stream_t *out_stream) {
    int16_t (*get)(stream_t * stream)         = in_stream->get;
    bool (*put)(stream_t * stream, int16_t c) = out_stream->put;
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
                repeat = false;
            }
        } else {
            if (buf[len - 1] == buf[len - 2]) {
                repeat = true;
                if (len > 2) {
                    if (!rle_append_bytes(out_stream, buf, (len - 2))) return false;
                    buf[0] = buf[1] = buf[len - 1];
                    len             = 2;
                }
                continue;
            }
            if (len == 128 || end) {
                if (end) {
                    int fg = 0;
                }
                if (!rle_append_bytes(out_stream, buf, len)) return false;
                len    = 0;
                repeat = false;
            }
        }
    }
    return true;
}
#endif  // RLE_ENCODER
