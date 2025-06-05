"""Routes to the `pre-commit` command.
"""
from milc import cli

@cli.subcommand('Runs pre-commit hooks')
def pre_commit(cli):
    """Runs pre-commit hooks in the QMK firmware directory.
    """
    # Run pre-commit in the QMK firmware directory
    result = cli.run(['pre-commit', 'run', '--all-files'], capture_output=False)
    return result.returncode == 0
