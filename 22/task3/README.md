# Задание 3 — лабораторная работа №2 (OpenMP, метод простой итерации)

В папке находится OpenMP-реализация метода простой итерации для системы
`Ax = b`, где на главной диагонали матрицы `A` стоят `2`, а вне диагонали — `1`.
Для тестовой задачи из условия вектор `b` заполнен значениями `N + 1`, поэтому точное решение равно вектору из единиц.

## Что реализовано

- **Вариант 1** — для каждого цикла используется отдельный `#pragma omp parallel for`.
- **Вариант 2** — одна внешняя параллельная область `#pragma omp parallel`, которая охватывает весь итерационный процесс.
- Поддержка `schedule(runtime)` для исследования `static / dynamic / guided / auto`.
- Матрица не хранится явно: действие `A` на вектор вычисляется аналитически, что позволяет исследовать большие размеры задачи без выделения `N^2` памяти.

## Сборка

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Ручной запуск

```bash
OMP_NUM_THREADS=8 ./build/task3_solver \
  --n 1000000 \
  --threads 8 \
  --variant 1 \
  --eps 1e-5 \
  --max-iters 1000000 \
  --schedule static \
  --chunk 0 \
  --csv data/results_task3_variants.csv
```

Параметр `tau` по умолчанию вычисляется автоматически как `2 / (N + 2)`.

## Массовый запуск двух вариантов

```bash
./scripts/run_variants.sh
```

## Исследование schedule

```bash
THREADS=8 VARIANT=2 ./scripts/run_schedule_study.sh
```

## Подбор размера задачи

В методичке требуется, чтобы на одном ядре задача считалась не менее 30 секунд.
Так как это зависит от сервера, размер `N` подберите эмпирически и затем зафиксируйте его для всех сравнений.

## Сбор сведений о вычислительном узле

```bash
lscpu
cat /sys/devices/virtual/dmi/id/product_name
numactl --hardware
cat /etc/os-release
```

## Отчёт

В `reports/` лежит PDF-шаблон и генератор итогового отчёта.

```bash
python3 scripts/generate_report.py \
  --variants data/results_task3_variants.csv \
  --schedule data/results_task3_schedule.csv \
  --out reports/task3_report_filled.pdf
```
