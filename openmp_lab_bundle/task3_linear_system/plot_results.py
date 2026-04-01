#!/usr/bin/env python3
from pathlib import Path
import csv
import matplotlib.pyplot as plt

base = Path(__file__).resolve().parent
plots_dir = base / "graphs"
plots_dir.mkdir(exist_ok=True, parents=True)

variants_csv = base / "tables" / "variants_summary.csv"

series = {}
with variants_csv.open("r", encoding="utf-8") as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            key = row["variant"]
            t = int(row["threads"])
            s = float(row["speedup"])
        except Exception:
            continue
        series.setdefault(key, {"threads": [], "speedup": []})
        series[key]["threads"].append(t)
        series[key]["speedup"].append(s)

all_threads = sorted({t for s in series.values() for t in s["threads"]})

plt.figure(figsize=(8, 5))
if all_threads:
    plt.plot(all_threads, all_threads, marker='o', linestyle='--', label='Линейное ускорение')
for key, values in series.items():
    plt.plot(values["threads"], values["speedup"], marker='o', label=key)
plt.xlabel("Число потоков")
plt.ylabel("Ускорение")
plt.title("Задание 3: ускорение решения СЛАУ")
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig(plots_dir / "task3_speedup.pdf")
