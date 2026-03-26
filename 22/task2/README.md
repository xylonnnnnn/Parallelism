# Задание 2 — численное интегрирование (OpenMP)

В папке находится реализация численного интегрирования на C++ с OpenMP.
Функция `integrate_omp` написана через локальную переменную потока и `#pragma omp atomic`.

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Ручной запуск

```bash
OMP_NUM_THREADS=8 ./build/task2_integration --nsteps 40000000 --threads 8 --repeats 1 --csv data/results_task2.csv
```

## Массовый запуск на сервере

```bash
./scripts/run_benchmark.sh
```

Скрипт прогоняет потоки 1, 2, 4, 7, 8, 16, 20, 40 при `nsteps = 40 000 000`.

## Сбор сведений о вычислительном узле

```bash
lscpu
cat /sys/devices/virtual/dmi/id/product_name
numactl --hardware
cat /etc/os-release
```

## Отчёт

В `reports/` лежит PDF-шаблон и генератор итогового PDF.

```bash
python3 scripts/generate_report.py \
  --csv data/results_task2.csv \
  --out reports/task2_report_filled.pdf
```
