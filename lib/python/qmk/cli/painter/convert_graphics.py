"""This script automates the conversion of graphics files into a format QMK firmware understands.
"""
import re
import datetime
import qmk.path
from qmk.painter import palettize_image, image_to_rgb565, compress_data
from string import Template
from PIL import Image
from milc import cli

#-----------------------------------------------------------------------------------------------------------------------

license_template = """\
/* Copyright ${year} QMK
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

/*
 * This file was auto-generated by `qmk painter-convert-graphics -i ${input_file} -f ${format}${compressed_flag}${chunk_size_flag}`
 */
"""

header_file_template = """\
${license}

#pragma once

#include <qp.h>

extern painter_image_t gfx_${sane_name} PROGMEM;
"""

palette_template = """\
static const uint8_t gfx_${sane_name}_palette[${palette_byte_size}] PROGMEM = {
${palette_lines}
};

"""

palette_line_template = """\
    0x${r}, 0x${g}, 0x${b}, // #${r}${g}${b} - ${idx}
"""

uncompressed_source_file_template = """\
${license}

#include <progmem.h>
#include <stdint.h>
#include <qp.h>
#include <qp_internal.h>

// clang-format off

${palette}

static const uint8_t gfx_${sane_name}_data[${byte_count}] PROGMEM = {
${bytes_lines}
};

static const painter_raw_image_descriptor_t gfx_${sane_name}_raw PROGMEM = {
    .base = {
        .image_format = ${image_format},
        .compression  = IMAGE_UNCOMPRESSED,
        .width        = ${image_width},
        .height       = ${image_height}
    },
    .image_bpp     = ${image_bpp},
    .image_palette = ${palette_ptr},
    .byte_count    = ${byte_count},
    .image_data    = gfx_${sane_name}_data,
};

painter_image_t gfx_${sane_name} PROGMEM = (painter_image_t)&gfx_${sane_name}_raw;

// clang-format on
"""

compressed_source_file_template = """\
${license}

#include <progmem.h>
#include <stdint.h>
#include <qp.h>
#include <qp_internal.h>

#ifndef QUANTUM_PAINTER_COMPRESSION_ENABLE
#    error Compression is not available on your selected platform. Please regenerate ${sane_name} without compression.
#endif

#if (QUANTUM_PAINTER_COMPRESSED_CHUNK_SIZE < ${chunk_size})
#    error Need to "#define QUANTUM_PAINTER_COMPRESSED_CHUNK_SIZE ${chunk_size}" or greater in your config.h
#endif

// clang-format off

${palette}

static const uint32_t gfx_${sane_name}_chunk_offsets[${chunk_count}] PROGMEM = {
${chunk_offsets_lines}
};

static const uint8_t gfx_${sane_name}_chunk_data[${byte_count}] PROGMEM = {
${bytes_lines}
};

static const painter_compressed_image_descriptor_t gfx_${sane_name}_compressed PROGMEM = {
    .base = {
        .image_format = ${image_format},
        .compression  = IMAGE_COMPRESSED_LZF,
        .width        = ${image_width},
        .height       = ${image_height}
    },
    .image_bpp       = ${image_bpp},
    .image_palette   = ${palette_ptr},
    .chunk_count     = ${chunk_count},
    .chunk_size      = ${chunk_size},
    .chunk_offsets   = gfx_${sane_name}_chunk_offsets,
    .compressed_data = gfx_${sane_name}_chunk_data,
    .compressed_size = ${compressed_size} // original = ${original_size} bytes (${image_bpp}bpp) / ${perc_of_uncompressed_size} of original // rgb24 = ${rgb24_size} bytes / ${perc_of_rgb24_size} of rgb24
};

painter_image_t gfx_${sane_name} PROGMEM = (painter_image_t)&gfx_${sane_name}_compressed;

// clang-format on
"""

compressed_chunk_offset_line_template = """\
    ${offset},  // chunk ${chunk_index} // compressed size: ${compressed_size} bytes / ${perc_of_uncompressed_size} of ${chunk_size} bytes
"""

#-----------------------------------------------------------------------------------------------------------------------

valid_formats = {
    'rgb565': {
        'bpp': 16,
        'uncompressed_source_template': None,
        'compressed_source_template': None,
    },
    'pal256': {
        'image_format': 'IMAGE_FORMAT_PALETTE',
        'bpp': 8,
        'has_palette': True,
        'num_colors': 256,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    },
    'pal16': {
        'image_format': 'IMAGE_FORMAT_PALETTE',
        'bpp': 4,
        'has_palette': True,
        'num_colors': 16,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    },
    'pal4': {
        'image_format': 'IMAGE_FORMAT_PALETTE',
        'bpp': 2,
        'has_palette': True,
        'num_colors': 4,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    },
    'pal2': {
        'image_format': 'IMAGE_FORMAT_PALETTE',
        'bpp': 1,
        'has_palette': True,
        'num_colors': 2,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    },
    'mono256': {
        'image_format': 'IMAGE_FORMAT_GRAYSCALE',
        'bpp': 8,
        'has_palette': False,
        'num_colors': 256,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    },
    'mono16': {
        'image_format': 'IMAGE_FORMAT_GRAYSCALE',
        'bpp': 4,
        'has_palette': False,
        'num_colors': 16,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    },
    'mono4': {
        'image_format': 'IMAGE_FORMAT_GRAYSCALE',
        'bpp': 2,
        'has_palette': False,
        'num_colors': 4,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    },
    'mono2': {
        'image_format': 'IMAGE_FORMAT_GRAYSCALE',
        'bpp': 1,
        'has_palette': False,
        'num_colors': 2,
        'uncompressed_source_template': uncompressed_source_file_template,
        'compressed_source_template': compressed_source_file_template,
    }
}

#-----------------------------------------------------------------------------------------------------------------------


def clean_output(str):
    str = re.sub(r'\r', '', str)
    str = re.sub(r'[\n]{3,}', r'\n\n', str)
    return str


#-----------------------------------------------------------------------------------------------------------------------


def render_license(subs):
    license_src = Template(license_template)
    return license_src.substitute(subs)


def render_header(format, subs):
    header_src = Template(header_file_template)
    return header_src.substitute(subs)


#-----------------------------------------------------------------------------------------------------------------------


def render_palette(palette, subs):
    palette_lines = ''
    palette_line_src = Template(palette_line_template)
    for n in range(len(palette)):
        rgb = palette[n]
        palette_lines = palette_lines + palette_line_src.substitute({'r': '{0:02X}'.format(rgb[0]), 'g': '{0:02X}'.format(rgb[1]), 'b': '{0:02X}'.format(rgb[2]), 'idx': n})
    palette_src = Template(palette_template)
    subs.update({'palette_byte_size': len(palette) * 3, 'palette_lines': palette_lines.rstrip()})
    return palette_src.substitute(subs).rstrip()


def render_bytes(bytes, subs):
    lines = ''
    for n in range(len(bytes)):
        if n % 32 == 0 and n > 0 and n != len(bytes):
            lines = lines + "\n   "
        elif n == 0:
            lines = lines + "   "
        lines = lines + " 0x{0:02X},".format(bytes[n])
    return lines.rstrip()


def render_source(format, palette, image_data, width, height, compress, chunksize, subs):
    has_palette = True if 'has_palette' in format and format['has_palette'] == True else False

    subs.update({
        'palette': render_palette(palette, subs) if has_palette else '',
        'palette_ptr': Template("gfx_${sane_name}_palette").substitute(subs) if has_palette else 'NULL',
    })

    if compress:
        (compressed_data, offsets, sizes) = compress_data(chunksize, image_data)
        chunk_offsets_lines = ''
        chunk_offset_line_src = Template(compressed_chunk_offset_line_template)
        for n in range(len(offsets)):
            chunk_offsets_lines = chunk_offsets_lines + chunk_offset_line_src.substitute({
                'chunk_index': '{0:3d}'.format(n),
                'offset': '{0:6d}'.format(offsets[n]),
                'compressed_size': '{0:6d}'.format(sizes[n]),
                'perc_of_uncompressed_size': '{0:.2f}%'.format(100 * sizes[n] / chunksize),
                'chunk_size': chunksize,
            })
        subs.update({
            'byte_count': len(compressed_data),
            'chunk_count': len(sizes),
            'chunk_offsets_lines': chunk_offsets_lines.rstrip(),
            'bytes_lines': render_bytes(compressed_data, subs),
            'compressed_size': len(compressed_data),
            'original_size': len(image_data),
            'perc_of_uncompressed_size': '{0:.2f}%'.format(100 * len(compressed_data) / len(image_data)),
            'rgb24_size': 3 * width * height,
            'perc_of_rgb24_size': '{0:.2f}%'.format(100 * len(compressed_data) / (3*width*height))
        })
    else:
        subs.update({'byte_count': len(image_data), 'bytes_lines': render_bytes(image_data, subs)})

    source_src = Template(format['compressed_source_template'] if cli.args.compress else format['uncompressed_source_template'])
    return source_src.substitute(subs)


#-----------------------------------------------------------------------------------------------------------------------


@cli.argument('-i', '--input', required=True, help='Specify input graphic file.')
@cli.argument('-f', '--format', required=True, help='Output format, valid types: %s' % (', '.join(valid_formats.keys())))
@cli.argument('-c', '--compress', arg_only=True, action='store_true', help='Compress chunks to minimise flash size requirements.')
@cli.argument('-s', '--chunk-size', default=128, help='Compression chunk size. Equivalent amount of RAM is required on the MCU.')
@cli.subcommand('Converts an input image to something QMK understands')
def painter_convert_graphics(cli):
    """Converts an image file to a format that Quantum Painter understands.

    This command uses the `qmk.painter` module to generate a Quantum Painter image defintion from an image. The generated definitions are written to a files next to the input -- `INPUT.c` and `INPUT.h`.
    """
    if cli.args.input != '-':
        cli.args.input = qmk.path.normpath(cli.args.input)

        # Error checking
        if not cli.args.input.exists():
            cli.log.error('Input image file does not exist!')
            cli.print_usage()
            return False

    if cli.args.format not in valid_formats.keys():
        cli.log.error('Output format %s is invalid. Allowed values: %s' % (cli.args.format, ', '.join(valid_formats.keys())))
        cli.print_usage()
        return False

    cli.args.chunk_size = int(cli.args.chunk_size)

    parent_dir = cli.args.input.parent
    sane_name = re.sub(r"[^a-zA-Z0-9]", "_", cli.args.input.stem)

    format = valid_formats[cli.args.format]

    graphic_image = Image.open(cli.args.input)
    (width, height) = graphic_image.size
    graphic_data = image_to_rgb565(graphic_image) if cli.args.format == 'rgb565' else palettize_image(graphic_image, ncolors=format['num_colors'], mono=(not format['has_palette']))
    palette = graphic_data[0]
    image_data = graphic_data[1]

    subs = {
        'year': datetime.date.today().strftime("%Y"),
        'input_file': cli.args.input.name,
        'sane_name': sane_name,
        'format': cli.args.format,
        'compressed_flag': ' -c' if cli.args.compress else '',
        'chunk_size_flag': (' -s %d' % (cli.args.chunk_size)) if cli.args.compress else '',
        'image_format': format['image_format'],
        'image_bpp': format['bpp'],
        'image_width': width,
        'image_height': height,
        'chunk_size': cli.args.chunk_size,
    }

    subs.update({
        'license': render_license(subs),
    })

    header_text = clean_output(render_header(format, subs))
    source_text = clean_output(render_source(format, palette, image_data, width, height, cli.args.compress, cli.args.chunk_size, subs))

    header_file = cli.args.input.parent / (cli.args.input.stem + ".h")
    with open(header_file, 'w') as header:
        print(f"Writing {header_file}...")
        header.write(header_text)
        header.close()

    source_file = cli.args.input.parent / (cli.args.input.stem + ".c")
    with open(source_file, 'w') as source:
        print(f"Writing {source_file}...")
        source.write(source_text)
        source.close()

    #print(header_text)
    #print(source_text)
