"""This script automates the conversion of font files into a format QMK firmware understands.
"""

import qmk.path
import qmk.painter
from PIL import Image, ImageDraw, ImageFont
from milc import cli


@cli.argument('-f', '--font', required=True, help='Specify input font file.')
@cli.argument('-o', '--output', required=True, help='Specify output image path.')
@cli.argument('-s', '--size', default=12, help='Specify font size. Default 12.')
@cli.argument('-a', '--all-ascii', arg_only=True, action='store_true', help='Exports ascii characters.')
@cli.argument('-g', '--glyphs', default='', help='Generate only the specified glyphs.')
@cli.subcommand('Converts an input font to something QMK understands')
def painter_make_font_image(cli):
    cli.args.font = qmk.path.normpath(cli.args.font)
    font = ImageFont.truetype(str(cli.args.font), int(cli.args.size))

    # Measure the whole text
    ascii_string = ''
    for c in range(0x20, 0x7F):
        ascii_string += chr(c)

    print(ascii_string)
    (ls_l, ls_t, ls_r, ls_b) = font.getbbox(ascii_string, anchor='ls')
    print(f"ls_l={ls_l}, t={ls_t}, r={ls_r}, b={ls_b}")

    y_offset = ls_t

    img = Image.new("RGB", (int(1.3 * (ls_r - ls_l)), ls_b - ls_t + 1), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)

    current_x = 0
    offsets = []
    widths = []
    for c in range(0x20, 0x7F):
        sc = chr(c)
        draw.text((current_x, 1 - y_offset), sc, font=font, fill=(255, 255, 255, 255), anchor='ls')
        (l, t, r, b) = font.getbbox(sc, anchor='ls')
        width = (r-l)
        offsets.append(current_x)
        widths.append(current_x)
        current_x += width

    img = img.crop((0, 0, current_x, ls_b - ls_t + 1))
    pixels = img.load()
    for i in range(len(offsets)):
        pixels[offsets[i],0] = (255,0,255)

    img.save(cli.args.output)

    return


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.subcommand('Converts an input font image to something QMK firmware understands')
def painter_convert_font_image(cli):
    return
