#!/usr/bin/env python3
from pathlib import Path
import argparse
import logging
import qmk_cli_loader


def _get_all_keyboards_and_keymaps():
    from qmk.search import search_keymap_targets

    all_keyboards = None
    all_keymaps = None
    exception = None

    try:
        oldlevel = qmk_cli_loader.set_log_level(logging.ERROR)
        all_targets = search_keymap_targets([("all", "all")])
    except Exception as exc:
        return (None, None, exc)
    finally:
        qmk_cli_loader.set_log_level(oldlevel)

    try:
        all_keymaps = set([(target.keyboard, target.keymap) for target in all_targets])
        all_keyboards = set([e[0] for e in all_keymaps])
    except Exception:
        pass

    return (all_keyboards, all_keymaps, exception)



# Using argparse, define arguments '--base-path=BASE_PATH', '--target-path=TARGET_PATH', '--merge-sha=MERGE_SHA', '--base-sha=BASE_SHA'
parser = argparse.ArgumentParser()
parser.add_argument("--base-path", type=str, required=True)
parser.add_argument("--target-path", type=str, required=True)
parser.add_argument("--merge-sha", type=str, required=True)
parser.add_argument("--base-sha", type=str, required=True)
args = parser.parse_args()

# Import the QMK CLI for the base repo
base_path = Path(args.base_path).absolute()
print(f"Importing QMK CLI from {base_path}")
qmk_cli_loader.import_qmk_cli(base_path)

# Get all keyboards and keymaps for the base repo
(_, kb_km_base, _) = _get_all_keyboards_and_keymaps()

# Unload the QMK CLI
print("Unloading QMK CLI")
qmk_cli_loader.unload_qmk_cli(base_path)

# Import the QMK CLI for the target repo
target_path = Path(args.target_path).absolute()
print(f"Importing QMK CLI from {target_path}")
qmk_cli_loader.import_qmk_cli(target_path)

# Get all keyboards and keymaps for the target repo
(_, kb_km_target, _) = _get_all_keyboards_and_keymaps()

# Unload the QMK CLI
print("Unloading QMK CLI")
qmk_cli_loader.unload_qmk_cli(base_path)

# Find the symmetric difference between the base and target keymap sets
diff = kb_km_base.symmetric_difference(kb_km_target)

# Print out all the resulting keyboards and keymaps
print("Build targets:")
for (kb, km) in diff:
    print(f"  {kb}:{km}")
