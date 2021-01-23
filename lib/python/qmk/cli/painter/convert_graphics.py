"""This script automates the conversion of graphics files into a format QMK firmware understands.
"""

import qmk.path
import qmk.painter
from milc import cli


@cli.argument('-i', '--input', help='Specify input graphic file.')
@cli.subcommand('Converts an input image to something QMK understands')
def painter_convert_graphics(cli):
    return
