#!/usr/bin/env python3
from pathlib import Path
import csv
import matplotlib.pyplot as plt

base = Path(__file__).resolve().parent
summary = base / "tables" / "matrix_vector_summary.csv"
plots_dir = base / "graphs"
plots_dir.mkdir(exist_ok=True, parents=True)

data = {}
with summary.open("r", encoding="utf-8") as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            n = int(row["matrix_size"])
            t = int(row["threads"])
            s = float(row["speedup"])
        except Exception:
            continue
        data.setdefault(n, {"threads": [], "speedup": []})
        data[n]["threads"].append(t)
        data[n]["speedup"].append(s)

all_threads = sorted({t for v in data.values() for t in v["threads"]})
plt.figure(figsize=(8, 5))
if all_threads:
    plt.plot(all_threads, all_threads, marker='o', linestyle='--', label='Линейное ускорение')
for n, series in sorted(data.items()):
    plt.plot(series["threads"], series["speedup"], marker='o', label=f'N={n}')
plt.xlabel("Число потоков")
plt.ylabel("Ускорение")
plt.title("Задание 1: ускорение умножения матрицы на вектор")
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig(plots_dir / "task1_speedup.pdf")
