# Задание 2

В папке находятся:
- `src/main.cpp` — OpenMP-версия численного интегрирования.
- `CMakeLists.txt` — сборка через CMake.
- `gather_node_info.sh` — сбор сведений о вычислительном узле.
- `plot_results.py` — построение PDF-графика по CSV.
- `tables/` — шаблон CSV и место для результатов.
- `graphs/` — шаблон графика и место для итогового PDF.
- `report_task2.pdf` — шаблон отчёта.

## Сборка
```bash
cmake -S . -B build
cmake --build build -j
```

## Запуск
```bash
./gather_node_info.sh
./build/task2_integration --nsteps 40000000 --repeats 5
python3 plot_results.py
```
