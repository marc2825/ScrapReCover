import json
from pathlib import Path

from gui import SCALE_DIVISOR_DEFAULT


def default_config_path() -> str:
    return str(Path(__file__).resolve().parents[3] / "assets" / "config.json")


def load_scale_divisor(config_path: str) -> float:
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            data = json.load(f)
        polygon_config = data.get("polygon_config", {})
        value = polygon_config.get("texture_scale_divisor", SCALE_DIVISOR_DEFAULT)
        return float(value)
    except (OSError, json.JSONDecodeError, TypeError, ValueError):
        return SCALE_DIVISOR_DEFAULT
