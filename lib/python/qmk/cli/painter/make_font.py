"""This script automates the conversion of font files into a format QMK firmware understands.
"""

import codecs
import re
import datetime
import qmk.path
import qmk.painter
from string import Template
from PIL import Image, ImageDraw, ImageFont
from milc import cli

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
 * This file was auto-generated by `qmk painter-convert-font-image -i ${input_file} -f ${format}`
 */
"""

header_file_template = """\
${license}

#pragma once

#include <qp.h>

extern painter_font_t font_${sane_name} PROGMEM;
"""

palette_template = """\
static const uint8_t font_${sane_name}_palette[${palette_byte_size}] PROGMEM = {
${palette_lines}
};

"""

palette_line_template = """\
    0x${r}, 0x${g}, 0x${b}, // #${r}${g}${b} - ${idx}
"""

ascii_defs_template = """\
static const painter_font_ascii_glyph_offset_t font_${sane_name}_ascii_defs[${glyph_count}] PROGMEM = {
${ascii_defs_lines}
};

"""

ascii_defs_line_template = """\
    { ${offset}, ${width} }, // '${glyph}' / ${glyph_number_hex} / ${glyph_number}
"""

unicode_defs_template = """\
#ifdef UNICODE_ENABLE
static const painter_font_unicode_glyph_offset_t font_${sane_name}_unicode_defs[${glyph_count}] PROGMEM = {
${unicode_defs_lines}
};
#endif // UNICODE_ENABLE
"""

unicode_defs_line_template = """\
    { ${glyph_number_hex}, ${offset}, ${width} }, // '${glyph}' / ${glyph_number_unicode}
"""

uncompressed_source_file_template = """\
${license}

#include <progmem.h>
#include <stdint.h>
#include <qp.h>
#include <qp_internal.h>

// clang-format off

${palette}

static const uint8_t font_${sane_name}_data[${byte_count}] PROGMEM = {
${bytes_lines}
};

${ascii_glyphs}

${unicode_glyphs}

static const painter_raw_font_descriptor_t font_${sane_name}_raw PROGMEM = {
    .base = {
        .image_format = ${image_format},
        .image_bpp    = ${image_bpp},
        .compression  = IMAGE_UNCOMPRESSED,
        .glyph_height = ${glyph_height},
    },
    .image_palette             = ${palette_ptr},
    .image_data                = font_${sane_name}_data,
    .ascii_glyph_definitions   = ${ascii_glyph_definitions_ptr},
#ifdef UNICODE_ENABLE
    .unicode_glyph_definitions = ${unicode_glyph_definitions_ptr},
    .unicode_glyph_count       = ${unicode_glyph_count},
#endif // UNICODE_ENABLE
};

painter_font_t font_${sane_name} PROGMEM = (painter_font_t)&font_${sane_name}_raw;

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


def render_ascii_glyphs(glyph_data, subs):
    ascii_glyphs = [glyph for glyph in glyph_data if ord(glyph['glyph']) < 0x7F]
    lines = ''
    if len(ascii_glyphs) > 0:
        line_src = Template(ascii_defs_line_template)
        for glyph in ascii_glyphs:
            lines = lines + line_src.substitute({
                'offset': '{0:5d}'.format(glyph['offset']),
                'width': '{0:3d}'.format(glyph['width']),
                'glyph': glyph['glyph'],
                'glyph_number_hex': '0x{0:02X}'.format(ord(glyph["glyph"])),
                'glyph_number': ord(glyph["glyph"])
            })
    template = Template(ascii_defs_template)
    subs.update({
        'glyph_count': len(ascii_glyphs),
        'ascii_defs_lines': lines.rstrip(),
        'ascii_glyph_definitions_ptr': Template("font_${sane_name}_ascii_defs").substitute(subs) if len(ascii_glyphs) > 0 else 'NULL',
    })
    return template.substitute(subs).rstrip()


def render_unicode_glyphs(glyph_data, subs):
    unicode_glyphs = [glyph for glyph in glyph_data if ord(glyph['glyph']) > 0x7F]
    lines = ''
    if len(unicode_glyphs) > 0:
        line_src = Template(unicode_defs_line_template)
        for glyph in unicode_glyphs:
            lines = lines + line_src.substitute({
                'offset': '{0:5d}'.format(glyph['offset']),
                'width': '{0:3d}'.format(glyph['width']),
                'glyph': glyph['glyph'],
                'glyph_number_hex': '0x{0:05X}'.format(ord(glyph["glyph"])),
                'glyph_number_unicode': 'U+{0:04X}'.format(ord(glyph["glyph"]))
            })
    template = Template(unicode_defs_template)
    subs.update({
        'glyph_count': len(unicode_glyphs),
        'unicode_defs_lines': lines.rstrip(),
        'unicode_glyph_definitions_ptr': Template("font_${sane_name}_unicode_defs").substitute(subs) if len(unicode_glyphs) > 0 else 'NULL',
        'unicode_glyph_count': len(unicode_glyphs),
    })
    return template.substitute(subs).rstrip()


def render_source(format, palette, glyph_data, subs):
    # Concatenate all the bytes from each of the glyphs
    image_data = []
    for glyph in glyph_data:
        glyph["offset"] = len(image_data)
        image_data.extend(glyph["image_bytes"])

    has_palette = True if 'has_palette' in format and format['has_palette'] is True else False
    subs.update({
        'palette': render_palette(palette, subs) if has_palette else '',
        'palette_ptr': Template("font_${sane_name}_palette").substitute(subs) if has_palette else 'NULL',
        'byte_count': len(image_data),
        'bytes_lines': render_bytes(image_data, subs),
        'ascii_glyphs': render_ascii_glyphs(glyph_data, subs),
        'unicode_glyphs': render_unicode_glyphs(glyph_data, subs),
    })

    source_src = Template(uncompressed_source_file_template)
    return source_src.substitute(subs)


@cli.argument('-f', '--font', required=True, help='Specify input font file.')
@cli.argument('-o', '--output', required=True, help='Specify output image path.')
@cli.argument('-s', '--size', default=12, help='Specify font size. Default 12.')
@cli.argument('-n', '--no-ascii', arg_only=True, action='store_true', help='Disables output of the full ASCII character set (0x20..0x7F), exporting only the glyphs specified.')
@cli.argument('-u', '--unicode-glyphs', default='', help='Also generate the specified unicode glyphs.')
@cli.subcommand('Converts an input font to something QMK understands')
def painter_make_font_image(cli):
    # Load the font
    cli.args.font = qmk.path.normpath(cli.args.font)
    font = ImageFont.truetype(str(cli.args.font), int(cli.args.size))

    # The set of glyphs that we want to generate images for
    glyphs = qmk.painter.generate_font_glyphs_list(cli.args.no_ascii, cli.args.unicode_glyphs)

    # Work out the sizing of the generated text
    (ls_l, ls_t, ls_r, ls_b) = font.getbbox("".join(glyphs), anchor='ls')

    # The Y-offset of the text when drawing with the 'ls' anchor
    y_offset = ls_t

    # Create a new black-filled image with the specified geometry, but wider than required as we'll crop it once all the glyphs have been drawn
    # Note that the height is increased by 1 -- this allows for the first row to be used as markers for widths of each glyph
    img = Image.new("RGB", (int(1.5 * (ls_r - ls_l)), ls_b - ls_t + 1), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    # Keep track of the drawing position, each glyph's pixel offset, and each glyph's width
    current_x = 0
    offsets = []
    widths = []

    # Draw each glyph after one another, keeping track of the locations
    for c in glyphs:
        draw.text((current_x, 1 - y_offset), c, font=font, fill=(255, 255, 255, 255), anchor='ls')
        (l, t, r, b) = font.getbbox(c, anchor='ls')
        glyph_width = (r - l)
        offsets.append(current_x)
        widths.append(glyph_width)
        current_x += glyph_width

    # Shrink the final image now that we know how wide it truly is
    img = img.crop((0, 0, current_x, ls_b - ls_t + 1))

    # Insert magenta pixels in the first row, at the start of each glyph's location
    pixels = img.load()
    for i in range(len(offsets)):
        pixels[offsets[i], 0] = (255, 0, 255)

    # Save to the output file specified
    cli.args.output = qmk.path.normpath(cli.args.output)
    img.save(cli.args.output)
    print(f"QMK font image exported to '{cli.args.output}'.")


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.argument('-n', '--no-ascii', arg_only=True, action='store_true', help='Disables output of the full ASCII character set (0x20..0x7F), exporting only the glyphs specified.')
@cli.argument('-u', '--unicode-glyphs', default='', help='Also generate the specified unicode glyphs.')
@cli.argument('-f', '--format', required=True, help='Output format, valid types: %s' % (', '.join(qmk.painter.valid_formats.keys())))
@cli.subcommand('Converts an input font image to something QMK firmware understands')
def painter_convert_font_image(cli):

    # The set of glyphs that we want to generate images for
    glyphs = qmk.painter.generate_font_glyphs_list(cli.args.no_ascii, cli.args.unicode_glyphs)

    # Work out the format
    format = qmk.painter.valid_formats[cli.args.format]

    # Load the image
    cli.args.input = qmk.path.normpath(cli.args.input)
    img = Image.open(cli.args.input)

    # Work out the geometry
    (width, height) = img.size

    # Work out the glyph height -- we assume that the first row of pixels is the marker
    glyph_height = height - 1

    # Work out the glyph offsets/widths
    glyph_pixel_offsets = []
    glyph_pixel_widths = []
    pixels = img.load()

    # Run through the markers and work out where each glyph starts/stops
    glyph_split_color = pixels[0, 0]  # top left pixel is the marker color we're going to use to split each glyph
    glyph_pixel_offsets.append(0)
    last_offset = 0
    for x in range(1, width):
        if pixels[x, 0] == glyph_split_color:
            glyph_pixel_offsets.append(x)
            glyph_pixel_widths.append(x - last_offset)
            last_offset = x
    glyph_pixel_widths.append(width - last_offset)

    # Make sure the number of glyphs we're attempting to generate matches the input image
    if len(glyph_pixel_offsets) != len(glyphs):
        cli.log.error('The number of glyphs to generate doesn\'t match the number of detected glyphs in the input image.')
        return False

    # Convert the overall image so that it's already in the correct format, allows us to split it up
    converted_img = img.crop((0, 1, width, height))
    converted_img = qmk.painter.convert_requested_format(converted_img, format)
    (palette, converted_img_bytes) = qmk.painter.convert_image_bytes(converted_img, format)

    # Now we can get each glyph image and convert it to the intended pixel format
    glyph_data = []
    for n in range(len(glyph_pixel_offsets)):
        this_glyph_image = converted_img.crop((glyph_pixel_offsets[n], 0, glyph_pixel_offsets[n] + glyph_pixel_widths[n], height))
        (this_glyph_image_palette, this_glyph_image_bytes) = qmk.painter.convert_image_bytes(this_glyph_image, format)
        glyph_data.append({"idx": n, "glyph": glyphs[n], "width": glyph_pixel_widths[n], "image_bytes": this_glyph_image_bytes})

    sane_name = re.sub(r"[^a-zA-Z0-9]", "_", cli.args.input.stem)

    subs = {
        'year': datetime.date.today().strftime("%Y"),
        'input_file': cli.args.input.name,
        'sane_name': sane_name,
        'format': cli.args.format,
        'image_format': format['image_format'],
        'image_bpp': format['bpp'],
        'glyph_height': glyph_height,
    }

    subs.update({
        'license': render_license(subs),
    })

    # Generate the output
    header_text = qmk.painter.clean_output(render_header(format, subs))
    source_text = qmk.painter.clean_output(render_source(format, palette, glyph_data, subs))

    # Render out the text files
    header_file = cli.args.input.parent / (cli.args.input.stem + ".h")
    with codecs.open(header_file, 'w', 'utf-8-sig') as header:
        print(f"Writing {header_file}...")
        header.write(header_text)
        header.close()

    source_file = cli.args.input.parent / (cli.args.input.stem + ".c")
    with codecs.open(source_file, 'w', 'utf-8-sig') as source:
        print(f"Writing {source_file}...")
        source.write(source_text)
        source.close()
