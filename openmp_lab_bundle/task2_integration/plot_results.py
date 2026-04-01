#!/usr/bin/env python3
from pathlib import Path
import csv
import matplotlib.pyplot as plt

base = Path(__file__).resolve().parent
summary = base / "tables" / "integration_summary.csv"
plots_dir = base / "graphs"
plots_dir.mkdir(exist_ok=True, parents=True)

threads, speedup = [], []
with summary.open("r", encoding="utf-8") as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            threads.append(int(row["threads"]))
            speedup.append(float(row["speedup"]))
        except Exception:
            continue

plt.figure(figsize=(8, 5))
if threads:
    plt.plot(threads, threads, marker='o', linestyle='--', label='Линейное ускорение')
    plt.plot(threads, speedup, marker='o', label='integrate_omp')
plt.xlabel("Число потоков")
plt.ylabel("Ускорение")
plt.title("Задание 2: ускорение численного интегрирования")
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig(plots_dir / "task2_speedup.pdf")
