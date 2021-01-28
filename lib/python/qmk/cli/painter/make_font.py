"""This script automates the conversion of font files into a format QMK firmware understands.
"""

import qmk.path
from qmk.painter import generate_font_glyphs_list
from PIL import Image, ImageDraw, ImageFont
from milc import cli


@cli.argument('-f', '--font', required=True, help='Specify input font file.')
@cli.argument('-o', '--output', required=True, help='Specify output image path.')
@cli.argument('-s', '--size', default=12, help='Specify font size. Default 12.')
@cli.argument('-n', '--no-ascii', arg_only=True, action='store_true', help='Disables output of the full ASCII character set (0x20..0x7F), exporting only the glyphs specified.')
@cli.argument('-e', '--ext-ascii', arg_only=True, action='store_true', help='Enables the extended ASCII character set (0x80..0xFF).')
@cli.argument('-g', '--glyphs', default='', help='Generate only the specified glyphs.')
@cli.subcommand('Converts an input font to something QMK understands')
def painter_make_font_image(cli):
    cli.args.font = qmk.path.normpath(cli.args.font)
    font = ImageFont.truetype(str(cli.args.font), int(cli.args.size))

    # The set of glyphs that we want to generate images for
    glyphs = generate_font_glyphs_list(cli.args.no_ascii, cli.args.ext_ascii, cli.args.glyphs)

    # Work out the sizing of the generated text
    (ls_l, ls_t, ls_r, ls_b) = font.getbbox("".join(glyphs), anchor='ls')

    # The Y-offset of the text when drawing with the 'ls' anchor
    y_offset = ls_t

    # Create a new black-filled image with the specified geometry, but wider than required as we'll crop it once all the glyphs have been drawn
    img = Image.new("RGB", (int(1.3 * (ls_r - ls_l)), ls_b - ls_t + 1), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    # Keep track of the drawing position, each glyph's pixel offset, and each glyph's width
    current_x = 0
    offsets = []
    widths = []

    # Draw each glyph after one another, keeping track of the locations
    for c in glyphs:
        draw.text((current_x, 1 - y_offset), c, font=font, fill=(255, 255, 255, 255), anchor='ls')
        (l, t, r, b) = font.getbbox(c, anchor='ls')
        width = (r - l)
        offsets.append(current_x)
        widths.append(current_x)
        current_x += width

    # Shrink the final image now that we know how wide it truly is
    img = img.crop((0, 0, current_x, ls_b - ls_t + 1))

    # Insert magenta pixels at in the first row, at the start of each glyph's location
    pixels = img.load()
    for i in range(len(offsets)):
        pixels[offsets[i], 0] = (255, 0, 255)

    # Save to the output file specified
    img.save(cli.args.output)
    print(f"QMK font image exported to '{cli.args.output}'.")

    return


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.subcommand('Converts an input font image to something QMK firmware understands')
def painter_convert_font_image(cli):
    return
