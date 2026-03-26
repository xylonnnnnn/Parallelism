# Задание 1 — умножение матрицы на вектор (OpenMP)

В папке находится реализация плотного умножения матрицы на вектор на C++ с OpenMP и параллельной инициализацией массивов.

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Ручной запуск

```bash
OMP_NUM_THREADS=8 ./build/task1_matvec --n 20000 --threads 8 --repeats 1 --csv data/results_task1.csv
```

Программа выводит:
- объём памяти,
- время инициализации,
- время умножения,
- полное время,
- checksum для проверки корректности.

## Массовый запуск на сервере

```bash
./scripts/run_benchmark.sh
```

По умолчанию скрипт прогоняет размеры 20000 и 40000 для потоков 1, 2, 4, 7, 8, 16, 20, 40.

## Привязка потоков

Примеры команд:

```bash
taskset -c 1,2,3 ./build/task1_matvec --n 20000 --threads 3
numactl -N 0 ./build/task1_matvec --n 20000 --threads 8
numactl -C 4,5,6 ./build/task1_matvec --n 20000 --threads 3
```

## Описание вычислительного узла

Соберите информацию на сервере командами:

```bash
lscpu
cat /sys/devices/virtual/dmi/id/product_name
numactl --hardware
cat /etc/os-release
```

## Отчёт

В `reports/` лежит PDF-шаблон и скрипт генерации итогового отчёта по CSV.

```bash
python3 scripts/generate_report.py \
  --csv data/results_task1.csv \
  --out reports/task1_report_filled.pdf
```
