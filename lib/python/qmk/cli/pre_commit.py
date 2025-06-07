"""Routes to the `pre-commit` command.
"""
from milc import cli

@cli.argument('pc_args', nargs='+', arg_only=True, help='Arguments to pass to pre-commit. You\'ll generally want to use a double dash (--) to separate the `pre-commit` arguments from the qmk cli arguments, e.g. `qmk pre-commit -- --help`.')
@cli.subcommand('Runs pre-commit hooks')
def pre_commit(cli):
    """Runs pre-commit hooks in the QMK firmware directory.
    """
    # Run pre-commit in the QMK firmware directory
    args = ['pre-commit'] + cli.args.pc_args
    result = cli.run(args, capture_output=False)
    return result.returncode == 0
