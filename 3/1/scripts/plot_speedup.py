#!/usr/bin/env python3
import csv
import sys
from pathlib import Path

import matplotlib.pyplot as plt


def read_rows(csv_path: Path):
    with csv_path.open(newline="") as f:
        reader = csv.DictReader(f)
        return list(reader)


def main():
    csv_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("results/task1_matrix_vector.csv")
    output_path = Path(sys.argv[2]) if len(sys.argv) > 2 else Path("results/speedup.png")

    rows = read_rows(csv_path)
    if not rows:
        raise SystemExit(f"No rows in {csv_path}")

    by_size = {}
    for row in rows:
        n = int(row["matrix_size"])
        by_size.setdefault(n, []).append(row)

    plt.figure()
    for n, size_rows in sorted(by_size.items()):
        size_rows.sort(key=lambda r: int(r["threads"]))
        threads = [int(r["threads"]) for r in size_rows]
        speedups = [float(r["multiply_speedup"]) for r in size_rows]
        plt.plot(threads, speedups, marker="o", label=f"M = {n}")

    all_threads = sorted({int(r["threads"]) for r in rows})
    plt.plot(all_threads, all_threads, linestyle="--", label="Linear")
    plt.xlabel("Количество потоков")
    plt.ylabel("Ускорение Sp")
    plt.title("Ускорение умножения матрицы на вектор")
    plt.grid(True)
    plt.legend()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(output_path, dpi=200, bbox_inches="tight")
    print(f"Saved plot to {output_path}")


if __name__ == "__main__":
    main()
