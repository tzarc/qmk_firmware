#!/usr/bin/env python3
from pathlib import Path
import argparse
import qmk_cli_loader


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

# Unload the QMK CLI
print("Unloading QMK CLI")
qmk_cli_loader.unload_qmk_cli(base_path)

# Import the QMK CLI for the target repo
target_path = Path(args.target_path).absolute()
print(f"Importing QMK CLI from {target_path}")
qmk_cli_loader.import_qmk_cli(target_path)

# Unload the QMK CLI
print("Unloading QMK CLI")
qmk_cli_loader.unload_qmk_cli(base_path)
