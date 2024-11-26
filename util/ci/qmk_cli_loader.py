#!/usr/bin/env python3
from pathlib import Path
import os
import sys
import logging


def set_log_level(level):
    from milc import cli

    cli.acquire_lock()
    try:
        old = cli.log_level
        cli.log_level = level
    except AttributeError:
        old = cli.log.level
    cli.log.setLevel(level)
    logging.root.setLevel(level)
    cli.release_lock()
    return old


def import_qmk_cli(qmk_firmware: Path):
    oldcwd = os.getcwd()
    os.chdir(qmk_firmware)
    os.environ["QMK_HOME"] = str(qmk_firmware)
    os.environ["ORIG_CWD"] = str(oldcwd)
    try:
        # Import the QMK CLI
        import qmk_cli.helpers

        if not qmk_cli.helpers.is_qmk_firmware(qmk_firmware):
            raise ImportError("Failed to detect repository")
        import qmk_cli.subcommands

        lib_python = str(qmk_firmware / "lib/python")
        if lib_python not in sys.path:
            sys.path.append(lib_python)

        import qmk.cli
        from qmk.util import maybe_exit_config

        maybe_exit_config(should_exit=False, should_reraise=True)
    finally:
        os.chdir(oldcwd)


def unload_qmk_cli(qmk_firmware: Path):
    oldcwd = os.getcwd()
    os.chdir(qmk_firmware)
    os.environ["QMK_HOME"] = str(qmk_firmware)
    os.environ["ORIG_CWD"] = str(oldcwd)
    try:
        # Remove the QMK CLI from sys.path
        lib_python = str(qmk_firmware / "lib/python")
        if lib_python in sys.path:
            sys.path.remove(lib_python)

        # Unload the QMK CLI
        cycles = 0
        mods = [m for m in sys.modules if m.startswith("milc") or m.startswith("qmk")]
        while len(mods) > 0 and cycles < 25:
            for m in mods:
                try:
                    del sys.modules[m]
                except: # noqa: E722
                    pass
            mods = [
                m for m in sys.modules if m.startswith("milc") or m.startswith("qmk")
            ]
            cycles += 1
    finally:
        os.chdir(oldcwd)
