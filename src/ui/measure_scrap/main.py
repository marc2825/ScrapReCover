#!/usr/bin/env python3

import argparse
import shlex
import sys
import tkinter as tk

from config_loader import default_config_path, load_scale_divisor
from gui import AppConfig, MAX_DISPLAY_SIZE_DEFAULT, PolygonController


def parse_args() -> AppConfig:
    parser = argparse.ArgumentParser(description='Scrap extraction tool for ScrapReCover')
    parser.add_argument('--mode', type=str, default='new', choices=['new', 'edit'],
                        help='Operation mode (new or edit)')
    parser.add_argument('--index', type=int, default=-1,
                        help='Index of polygon to edit (only used in edit mode)')
    parser.add_argument('--temp-dir', type=str, default='../input_scraps/cropper_temp',
                        help='Temporary directory for edit mode')
    parser.add_argument('--outputs-dir', type=str, default='../input_scraps/cropper_outputs',
                        help='Output directory for new mode')
    parser.add_argument('--max-display-size', type=int, default=MAX_DISPLAY_SIZE_DEFAULT,
                        help='Maximum display size for preview')
    raw_args = sys.argv[1:]
    if len(raw_args) == 1 and " " in raw_args[0]:
        raw_args = shlex.split(raw_args[0])
    args = parser.parse_args(raw_args)

    args.scale_divisor = load_scale_divisor(default_config_path())

    return AppConfig(
        mode=args.mode,
        index=args.index,
        temp_dir=args.temp_dir,
        outputs_dir=args.outputs_dir,
        scale_divisor=args.scale_divisor,
        max_display_size=args.max_display_size
    )


def main() -> None:
    try:
        sys.stdout.reconfigure(line_buffering=True)
        sys.stderr.reconfigure(line_buffering=True)
    except AttributeError:
        pass
    config = parse_args()
    root = tk.Tk()
    PolygonController(root, config)
    root.mainloop()


if __name__ == "__main__":
    main()
