"""This script automates the conversion of font files into a format QMK firmware understands.
"""

import qmk.path
import qmk.painter
from PIL import Image, ImageDraw, ImageFont
from milc import cli


@cli.argument('-f', '--font', required=True, help='Specify input font file.')
@cli.argument('-o', '--output', required=True, help='Specify output image path.')
@cli.argument('-s', '--size', default=12, help='Specify font size. Default 12.')
@cli.argument('-n', '--no-ascii', arg_only=True, action='store_true', help='Disables output of the full ASCII character set (0x20..0x7F), exporting only the glyphs specified.')
@cli.argument('-e', '--ext-ascii', arg_only=True, action='store_true', help='Enables the extended ASCII character set (0x80..0xFF).')
@cli.argument('-g', '--glyphs', default='', help='Also generate the specified glyphs.')
@cli.subcommand('Converts an input font to something QMK understands')
def painter_make_font_image(cli):
    # Load the font
    cli.args.font = qmk.path.normpath(cli.args.font)
    font = ImageFont.truetype(str(cli.args.font), int(cli.args.size))

    # The set of glyphs that we want to generate images for
    glyphs = qmk.painter.generate_font_glyphs_list(cli.args.no_ascii, cli.args.ext_ascii, cli.args.glyphs)

    # Work out the sizing of the generated text
    (ls_l, ls_t, ls_r, ls_b) = font.getbbox("".join(glyphs), anchor='ls')

    # The Y-offset of the text when drawing with the 'ls' anchor
    y_offset = ls_t

    # Create a new black-filled image with the specified geometry, but wider than required as we'll crop it once all the glyphs have been drawn
    # Note that the height is increased by 1 -- this allows for the first row to be used as markers for widths of each glyph
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
    img.save(cli.args.output)
    print(f"QMK font image exported to '{cli.args.output}'.")


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.argument('-n', '--no-ascii', arg_only=True, action='store_true', help='Disables output of the full ASCII character set (0x20..0x7F), exporting only the glyphs specified.')
@cli.argument('-e', '--ext-ascii', arg_only=True, action='store_true', help='Enables the extended ASCII character set (0x80..0xFF).')
@cli.argument('-g', '--glyphs', default='', help='Also generate the specified glyphs.')
@cli.argument('-f', '--format', required=True, help='Output format, valid types: %s' % (', '.join(qmk.painter.valid_formats.keys())))
@cli.subcommand('Converts an input font image to something QMK firmware understands')
def painter_convert_font_image(cli):

    # The set of glyphs that we want to generate images for
    glyphs = qmk.painter.generate_font_glyphs_list(cli.args.no_ascii, cli.args.ext_ascii, cli.args.glyphs)

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

    # Now we can get each glyph image and convert it to the intended pixel format
    glyph_data = []
    format = qmk.painter.valid_formats[cli.args.format]
    for n in range(len(glyph_pixel_offsets)):
        this_glyph_image = img.crop((glyph_pixel_offsets[n], 1, glyph_pixel_offsets[n] + glyph_pixel_widths[n], height))
        graphic_data = qmk.painter.image_to_rgb565(this_glyph_image) if cli.args.format == 'rgb565' else qmk.painter.palettize_image(this_glyph_image, ncolors=format['num_colors'], mono=(not format['has_palette']))

        glyph_data.append({"idx": n, "glyph": glyphs[n], "width": glyph_pixel_widths[n], "palette": graphic_data[0], "image_bytes": graphic_data[1]})

    # Print out some info for now
    for data in glyph_data:
        print(f'{data["idx"]}: glyph = \'{data["glyph"]}\' width = {data["width"]}, byte count = {len(data["image_bytes"])}')
