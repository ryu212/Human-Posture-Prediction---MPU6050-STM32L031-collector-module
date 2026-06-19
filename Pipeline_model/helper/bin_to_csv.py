import argparse
import re
import struct
from pathlib import Path

import pandas as pd

SENSOR1_COLS = ["Ax1", "Ay1", "Az1"]
SENSOR2_COLS = ["Ax2", "Ay2", "Az2"]
ACC_COLS = SENSOR1_COLS + SENSOR2_COLS
PROJECT_DIR = Path(__file__).resolve().parents[1]
RECORD_SIZE = 29
ACC_OFFSET = 17
FILENAME_RE = re.compile(r"^S(?P<session>\d+)_(?P<label>[01]{3})\.BIN$", re.IGNORECASE)


def parse_label(path: Path) -> int:
    match = FILENAME_RE.match(path.name)
    if not match:
        raise ValueError(f"Invalid BIN filename: {path.name}")
    return int(match.group("label"), 2)


def read_bin_file(path: Path) -> list[dict]:
    label = parse_label(path)
    data = path.read_bytes()
    remainder = len(data) % RECORD_SIZE
    if remainder:
        raise ValueError(f"{path.name} has {remainder} trailing bytes")

    rows = []
    for start in range(0, len(data), RECORD_SIZE):
        record = data[start : start + RECORD_SIZE]
        values = struct.unpack("<6h", record[ACC_OFFSET : ACC_OFFSET + 12])
        row = dict(zip(ACC_COLS, values))
        row["label"] = label
        rows.append(row)

    return rows


def convert_bin_folder(input_dir: Path) -> pd.DataFrame:
    files = sorted(input_dir.glob("*.BIN"))
    if not files:
        raise FileNotFoundError(f"No .BIN files found in {input_dir}")

    rows = []
    for path in files:
        rows.extend(read_bin_file(path))
    return pd.DataFrame(rows)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert posture BIN session files into the CSV format used by the pipeline."
    )
    parser.add_argument(
        "--input-dir",
        default=str(PROJECT_DIR / "dataset" / "posture_data"),
        help="Folder containing files named like S1_000.BIN.",
    )
    parser.add_argument(
        "--output",
        default=str(PROJECT_DIR / "dataset" / "posture_data.csv"),
        help="Output CSV path.",
    )
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output = Path(args.output)
    df = convert_bin_folder(input_dir)

    output.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(output, index=False)

    counts = df["label"].value_counts().sort_index().to_dict()
    print(f"Saved CSV: {output} ({len(df)} rows)")
    print(f"Label counts: {counts}")


if __name__ == "__main__":
    main()
