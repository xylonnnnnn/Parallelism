# Задание 1

В папке находятся:
- `src/main.cpp` — OpenMP-версия умножения матрицы на вектор с параллельной инициализацией.
- `CMakeLists.txt` — сборка через CMake.
- `gather_node_info.sh` — сбор сведений о вычислительном узле.
- `plot_results.py` — построение PDF-графика по CSV.
- `tables/` — шаблон CSV и место для результатов.
- `graphs/` — шаблон графика и место для итогового PDF.
- `report_task1.pdf` — шаблон отчёта.

## Сборка
```bash
cmake -S . -B build
cmake --build build -j
```

## Запуск
```bash
./gather_node_info.sh
./build/task1_matrix_vector --repeats 3
python3 plot_results.py
```

По условию задания расчёты нужно запускать на сервере. В архиве уже есть шаблоны CSV/графика/отчёта, но реальные числа нужно получить на целевом узле.
