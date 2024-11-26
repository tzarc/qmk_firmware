#!/usr/bin/env python3
from pathlib import Path
import os
import argparse
import logging
import qmk_cli_loader


def _get_all_keyboards_and_keymaps():
    from qmk.search import search_keymap_targets

    all_keyboards = set()
    all_keymaps = set()
    exception = None

    try:
        oldcwd = os.getcwd()
        os.chdir(os.environ["QMK_HOME"])
        oldlevel = qmk_cli_loader.set_log_level(logging.ERROR)
        all_targets = search_keymap_targets([("all", "all")])
    except Exception as exc:
        return (set(), set(), exc)
    finally:
        os.chdir(oldcwd)
        qmk_cli_loader.set_log_level(oldlevel)

    try:
        all_keymaps = set([(target.keyboard, target.keymap) for target in all_targets])
        all_keyboards = set([e[0] for e in all_keymaps])
    except Exception:
        pass

    return (all_keyboards, all_keymaps, exception)



# Using argparse, define arguments '--base-path=BASE_PATH', '--target-path=TARGET_PATH'
parser = argparse.ArgumentParser()
parser.add_argument("--base-path", type=str, required=True)
parser.add_argument("--target-path", type=str, required=True)
parser.add_argument("--base-ref", type=str, required=True)
parser.add_argument("--target-ref", type=str, required=True)
args = parser.parse_args()

# Import the QMK CLI for the base repo
base_path = Path(args.base_path).absolute()
os.chdir(base_path)
print(f"Importing QMK CLI from {base_path} (@ {args.base_ref})")
qmk_cli_loader.import_qmk_cli(base_path)

# Get all keyboards and keymaps for the base repo
print("Getting all keyboards and keymaps")
(_, kb_km_base, exc) = _get_all_keyboards_and_keymaps()

# Unload the QMK CLI
print("Unloading QMK CLI")
qmk_cli_loader.unload_qmk_cli(base_path)

# Throw the exception if it occurred
if exc is not None:
    raise exc

# Import the QMK CLI for the target repo
target_path = Path(args.target_path).absolute()
os.chdir(target_path)
print(f"Importing QMK CLI from {target_path} (@ {args.target_ref})")
qmk_cli_loader.import_qmk_cli(target_path)

# Get all keyboards and keymaps for the target repo
print("Getting all keyboards and keymaps")
(_, kb_km_target, exc) = _get_all_keyboards_and_keymaps()

# Unload the QMK CLI
print("Unloading QMK CLI")
qmk_cli_loader.unload_qmk_cli(target_path)

# Throw the exception if it occurred
if exc is not None:
    raise exc

# Find the combinations that exist in the target but not the base (we don't care if things are deleted)
removed = kb_km_base - kb_km_target
added = kb_km_target - kb_km_base

# Print out all the resulting keyboards and keymaps
if len(added) > 0:
    print("Build targets added:")
    for (kb, km) in added:
        print(f"  {kb}:{km}")

if len(removed) > 0:
    print("Build targets removed:")
    for (kb, km) in removed:
        print(f"  {kb}:{km}")
