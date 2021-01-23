"""This script automates the conversion of font files into a format QMK firmware understands.
"""

import qmk.path
import qmk.painter
from milc import cli


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.subcommand('Converts an input font to something QMK understands')
def painter_make_font_image(cli):
    return


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.subcommand('Converts an input font image to something QMK firmware understands')
def painter_convert_font_image(cli):
    return
