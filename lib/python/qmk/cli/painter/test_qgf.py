"""This script tests QGF functionality.
"""
import qmk.painter
import qmk.qgf_pil_plugin
from milc import cli
from PIL import Image


@cli.argument('-v', '--verbose', arg_only=True, action='store_true', help='Turns on verbose output.')
@cli.argument('-i', '--input', required=True, help='Specify input graphic file.')
@cli.argument('-o', '--output', required=True, help='Specify output graphic file.')
@cli.argument('-f', '--format', required=True, help='Output format, valid types: %s' % (', '.join(qmk.painter.valid_formats.keys())))
@cli.argument('-r', '--no-rle', arg_only=True, action='store_true', help='Disables the use of RLE when encoding images.')
@cli.argument('-d', '--no-deltas', arg_only=True, action='store_true', help='Disables the use of delta frames when encoding images.')
@cli.subcommand('Tests the QGF exporter')
def test_qgf_convert(cli):
    """Tests the QGF converter.
    """
    if cli.args.input != '-':
        cli.args.input = qmk.path.normpath(cli.args.input)
    if cli.args.output != '-':
        cli.args.output = qmk.path.normpath(cli.args.output)

    if cli.args.format not in qmk.painter.valid_formats.keys():
        cli.log.error('Output format %s is invalid. Allowed values: %s' % (cli.args.format, ', '.join(qmk.painter.valid_formats.keys())))
        cli.print_usage()
        return False

    format = qmk.painter.valid_formats[cli.args.format]

    input_img = Image.open(cli.args.input)
    input_img.save(cli.args.output, "QGF", use_deltas=(not cli.args.no_deltas), use_rle=(not cli.args.no_rle), qmk_format=format, verbose=cli.args.verbose)
