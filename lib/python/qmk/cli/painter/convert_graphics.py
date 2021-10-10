"""This script automates the conversion of graphics files into a format QMK firmware understands.
"""
import re
import datetime
import qmk.path
import qmk.painter
from string import Template
from PIL import Image
from milc import cli
from colorsys import rgb_to_hsv

license_template = """\
// Copyright ${year} QMK -- generated source code only, image retains original copyright
// SPDX-License-Identifier: GPL-2.0-or-later

// This file was auto-generated by `qmk painter-convert-graphics -i ${input_file} -f ${format}`
"""

header_file_template = """\
${license}

#pragma once

#include <qp.h>

extern painter_image_t gfx_${sane_name} QP_RESIDENT_FLASH;
"""

palette_template = """\
static const uint8_t gfx_${sane_name}_palette[${palette_byte_size}] QP_RESIDENT_FLASH = {
${palette_lines}
};

"""

palette_line_template = """\
    0x${h255}, 0x${s255}, 0x${v255}, // ${idx} - #${r}${g}${b} - H: ${h360}°, S: ${s100}%, V: ${v100}%
"""

source_file_template = """\
${license}

#include <stdint.h>
#include <qp.h>
#include <qp_internal.h>

// clang-format off

${palette}

static const uint8_t gfx_${sane_name}_data[${byte_count}] QP_RESIDENT_FLASH = {
${bytes_lines}
};

static const painter_raw_image_descriptor_t gfx_${sane_name}_raw QP_RESIDENT_FLASH = {
    .base = {
        .image_type   = IMAGE_TYPE_LOCATION_FLASH,
        .image_format = ${image_format},
        .image_bpp    = ${image_bpp},
        .compression  = ${compression},
        .width        = ${image_width},
        .height       = ${image_height},
        .version      = IMAGE_VERSION_0,
    },
    .image_palette = ${palette_ptr},
    .byte_count    = ${byte_count},
    .image_data    = gfx_${sane_name}_data,
};

painter_image_t gfx_${sane_name} QP_RESIDENT_FLASH = (painter_image_t)&gfx_${sane_name}_raw;

// clang-format on
"""


def render_license(subs):
    license_src = Template(license_template)
    return license_src.substitute(subs)


def render_header(format, subs):
    header_src = Template(header_file_template)
    return header_src.substitute(subs)


def render_palette(palette, subs):
    palette_lines = ''
    palette_line_src = Template(palette_line_template)
    for n in range(len(palette)):
        rgb = palette[n]
        hsv = rgb_to_hsv(rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0)
        palette_lines = palette_lines + palette_line_src.substitute({
            'r': '{0:02X}'.format(rgb[0]),
            'g': '{0:02X}'.format(rgb[1]),
            'b': '{0:02X}'.format(rgb[2]),
            'h255': '{0:02X}'.format(int(hsv[0] * 255.0)),
            's255': '{0:02X}'.format(int(hsv[1] * 255.0)),
            'v255': '{0:02X}'.format(int(hsv[2] * 255.0)),
            'h360': '{0:3d}'.format(int(hsv[0] * 360.0)),
            's100': '{0:3d}'.format(int(hsv[1] * 100.0)),
            'v100': '{0:3d}'.format(int(hsv[2] * 100.0)),
            'idx': '{0:3d}'.format(n)
        })
    palette_src = Template(palette_template)
    subs.update({'palette_byte_size': len(palette) * 3, 'palette_lines': palette_lines.rstrip()})
    return palette_src.substitute(subs).rstrip()


def render_bytes(bytes, subs):
    lines = ''
    for n in range(len(bytes)):
        if n % 16 == 0 and n > 0 and n != len(bytes):
            lines = lines + "\n   "
        elif n == 0:
            lines = lines + "   "
        lines = lines + " 0x{0:02X},".format(bytes[n])
    return lines.rstrip()


def render_source(format, palette, image_data, width, height, subs):
    has_palette = True if 'has_palette' in format and format['has_palette'] is True else False

    subs.update({
        'palette': render_palette(palette, subs) if has_palette else '',
        'palette_ptr': Template("gfx_${sane_name}_palette").substitute(subs) if has_palette else 'NULL',
        'byte_count': len(image_data),
        'bytes_lines': render_bytes(image_data, subs),
    })

    source_src = Template(source_file_template)
    return source_src.substitute(subs)


@cli.argument('-i', '--input', required=True, help='Specify input graphic file.')
@cli.argument('-o', '--output', default='', help='Specify output directory. Defaults to same directory as input.')
@cli.argument('-f', '--format', required=True, help='Output format, valid types: %s' % (', '.join(qmk.painter.valid_formats.keys())))
@cli.argument('-r', '--rle', arg_only=True, action='store_true', help='Uses RLE to minimise converted image size.')
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

    if cli.args.format not in qmk.painter.valid_formats.keys():
        cli.log.error('Output format %s is invalid. Allowed values: %s' % (cli.args.format, ', '.join(qmk.painter.valid_formats.keys())))
        cli.print_usage()
        return False

    # Work out the output directory
    if len(cli.args.output) == 0:
        cli.args.output = cli.args.input.parent
    cli.args.output = qmk.path.normpath(cli.args.output)

    sane_name = re.sub(r"[^a-zA-Z0-9]", "_", cli.args.input.stem)

    format = qmk.painter.valid_formats[cli.args.format]

    graphic_image = Image.open(cli.args.input)
    graphic_image = qmk.painter.convert_requested_format(graphic_image, format)
    (width, height) = graphic_image.size
    graphic_data = qmk.painter.convert_image_bytes(graphic_image, format)
    palette = graphic_data[0]
    image_data = graphic_data[1] if not cli.args.rle else qmk.painter.compress_bytes_qmk_rle(graphic_data[1])

    subs = {
        'year': datetime.date.today().strftime("%Y"),
        'input_file': cli.args.input.name,
        'sane_name': sane_name,
        'format': cli.args.format,
        'image_format': format['image_format'],
        'image_bpp': format['bpp'],
        'image_width': width,
        'image_height': height,
        'compression': 'IMAGE_UNCOMPRESSED' if not cli.args.rle else 'IMAGE_COMPRESSED_RLE',
    }

    subs.update({
        'license': render_license(subs),
    })

    header_text = qmk.painter.clean_output(render_header(format, subs))
    source_text = qmk.painter.clean_output(render_source(format, palette, image_data, width, height, subs))

    header_file = cli.args.output / (cli.args.input.stem + ".h")
    with open(header_file, 'w') as header:
        print(f"Writing {header_file}...")
        header.write(header_text)
        header.close()

    source_file = cli.args.output / (cli.args.input.stem + ".c")
    with open(source_file, 'w') as source:
        print(f"Writing {source_file}...")
        source.write(source_text)
        source.close()
