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
    test_string = ''
    for c in range(0x20, 0x7F):
        test_string += chr(c)

    print(test_string)
    (l, t, r, b) = font.getbbox(test_string, anchor='lt')
    print(f"l={l}, t={t}, r={r}, b={b}")

    img = Image.new("RGB", (r-l, b-t+1), (255, 255, 255))
    draw = ImageDraw.Draw(img)
    draw.text((0,1), test_string, font=font, fill=(0,0,0,255), anchor='lt')
    img.save(cli.args.output)

    return


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.subcommand('Converts an input font image to something QMK firmware understands')
def painter_convert_font_image(cli):
    return