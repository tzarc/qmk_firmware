// Copyright 2021 Nick Brassel (@tzarc)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <qp_stream.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stream API

uint32_t qp_stream_read_impl(void QP_RESIDENT_FLASH_OR_RAM *output_buf, uint32_t member_size, uint32_t num_members, qp_stream_t *stream) {
    uint8_t *output_ptr = (uint8_t *)data;

    uint32_t i;
    for (i = 0; i < (num_members * member_size); ++i) {
        int16_t c = qp_stream_get(stream);
        if (c == STREAM_EOF) {
            break;
        }

        output_ptr[i] = (uint8_t)(c & 0xFF);
    }

    return i;
}

uint32_t qp_stream_write_impl(const void *input_buf, uint32_t member_size, uint32_t num_members, qp_stream_t *stream) {
    uint8_t *input_ptr = (uint8_t *)data;

    uint32_t i;
    for (i = 0; i < (num_members * member_size); ++i) {
        if (!qp_stream_put(stream, input_ptr[i])) {
            break;
        }
    }

    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory streams

int16_t mem_get(qp_stream_t *stream) {
    qp_memory_stream_t *s = (qp_memory_stream_t *)stream;
    if (s->position >= s->length) return STREAM_EOF;
    return s->buffer[s->position++];
}

bool mem_put(qp_stream_t *stream, uint8_t c) {
    qp_memory_stream_t *s = (qp_memory_stream_t *)stream;
    if (s->position >= s->length) return false;
    s->buffer[s->position++] = c;
    return true;
}

int mem_seek(qp_stream_t *stream, int32_t offset, int origin) {
    qp_memory_stream_t *s = (qp_memory_stream_t *)stream;
    switch (origin) {
        case SEEK_SET:
            s->position = offset;
            break;
        case SEEK_CUR:
            s->position += offset;
            break;
        case SEEK_END:
            s->position = s->length + offset;
            break;
    }
    if (s->position < 0) s->position = 0;
    if (s->position >= s->length) s->position = s->length;
    return 0;
}

int32_t mem_tell(qp_stream_t *stream) {
    qp_memory_stream_t *s = (qp_memory_stream_t *)stream;
    return s->position;
}

bool mem_is_eof(qp_stream_t *stream) {
    qp_memory_stream_t *s = (qp_memory_stream_t *)stream;
    return s->position >= s->length;
}

qp_memory_stream_t qp_make_memory_stream(void QP_RESIDENT_FLASH_OR_RAM *buffer, uint32_t length) {
    qp_memory_stream_t stream = {
        .base =
            {
                .get    = mem_get,
                .put    = mem_put,
                .seek   = mem_seek,
                .tell   = mem_tell,
                .is_eof = mem_is_eof,
            },
        .buffer   = (uint8_t QP_RESIDENT_FLASH_OR_RAM *)buffer,
        .length   = length,
        .position = 0,
    };
    return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FILE streams

#ifdef QP_STREAM_HAS_FILE_IO

int16_t file_get(qp_stream_t *stream) {
    qp_file_stream_t *s = (qp_file_stream_t *)stream;
    int               c = fgetc(s->file);
    if (c < 0 || feof(s->file)) return STREAM_EOF;
    return (uint16_t)c;
}

bool file_put(qp_stream_t *stream, uint8_t c) {
    qp_file_stream_t *s = (qp_file_stream_t *)stream;
    return fputc(c, s->file) == c;
}

int file_seek(qp_stream_t *stream, int32_t offset, int origin) {
    qp_file_stream_t *s = (qp_file_stream_t *)stream;
    return fseek(s->file, offset, origin);
}

int32_t file_tell(qp_stream_t *stream) {
    qp_file_stream_t *s = (qp_file_stream_t *)stream;
    return (int32_t)ftell(s->file);
}

bool file_is_eof(qp_stream_t *stream) {
    qp_file_stream_t *s = (qp_file_stream_t *)stream;
    return (bool)feof(s->file);
}

qp_file_stream_t make_file_stream(FILE *f) {
    qp_file_stream_t stream = {
        .base =
            {
                .get    = file_get,
                .put    = file_put,
                .seek   = file_seek,
                .tell   = file_tell,
                .is_eof = file_is_eof,
            },
        .file = f,
    };
    return stream;
}
#endif  // QP_STREAM_HAS_FILE_IO
