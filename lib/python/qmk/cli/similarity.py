import contextlib
import json
from difflib import SequenceMatcher
import logging
from functools import partial
from milc import cli

from qmk.build_targets import BuildTarget
from qmk.keyboard import keyboard_completer, keyboard_folder
from qmk.search import search_keymap_targets
from qmk.util import parallel_map


def _set_log_level(level):
    cli.acquire_lock()
    old = cli.log_level
    cli.log_level = level
    cli.log.setLevel(level)
    logging.root.setLevel(level)
    cli.release_lock()
    return old


@contextlib.contextmanager
def ignore_logging():
    old = _set_log_level(logging.CRITICAL)
    yield
    _set_log_level(old)


def _generic_dotty(target: BuildTarget):
    t = target.dotty
    t.pop('manufacturer')
    t.pop('keyboard_name')
    t.pop('url')
    t.pop('maintainer')
    t.pop('keyboard_folder')
    t.pop('parse_errors')
    t.pop('parse_warnings')
    return t


def _generic_dump(target: BuildTarget):
    d = _generic_dotty(target).to_dict()
    return json.dumps(d, indent=None, sort_keys=True)


def _compare_targets(target_str: str, other: BuildTarget):
    with ignore_logging():
        o = _generic_dump(other)
        m = SequenceMatcher(None, target_str, o)
        return (other.keyboard, m.ratio())


@cli.argument('-n', '--count', type=int, default=10, help='The number of similar keyboards to display.')
@cli.argument('-kb', '--keyboard', type=keyboard_folder, completer=keyboard_completer, help='The keyboard to run a similarity check for.')
@cli.subcommand('Lists the similarity of one keyboard to all others')
def similarity(cli):
    all_targets = search_keymap_targets()

    target = next((t for t in all_targets if t.keyboard == cli.args.keyboard), None)
    others = [t for t in all_targets if t.keyboard != cli.args.keyboard]

    t = _generic_dump(target)
    f = partial(_compare_targets, t)

    cli.log.info(f'Comparing {target.keyboard} to all other keyboards...')
    ratios = parallel_map(f, others)
    top_ratios = sorted(ratios, key=lambda x: x[1], reverse=True)[:min(len(ratios), cli.args.count)]
    cli.log.info(f'Top {cli.args.count} similar keyboards to {target.keyboard}:')
    for keyboard, ratio in top_ratios:
        r = 100.0 * ratio
        cli.log.info(f'{r:4.0f}% {keyboard}')
