# Задание 3

В папке находятся:
- `src/main.cpp` — OpenMP-решение СЛАУ методом простой итерации.
- `CMakeLists.txt` — сборка через CMake.
- `gather_node_info.sh` — сбор сведений о вычислительном узле.
- `plot_results.py` — построение PDF-графика по CSV.
- `tables/` — шаблон CSV и место для результатов.
- `graphs/` — шаблон графика и место для итогового PDF.
- `report_task3.pdf` — шаблон отчёта.

## Особенности
Реализованы оба варианта из лабораторной работы №2:
1. отдельный `#pragma omp parallel for` для каждого цикла;
2. одна общая `#pragma omp parallel`.

Также добавлено исследование `schedule(...)`.

## Сборка
```bash
cmake -S . -B build
cmake --build build -j
```

## Запуск
```bash
./gather_node_info.sh
./build/task3_linear_system --n 10000000 --eps 1e-5 --repeats 3
python3 plot_results.py
```
