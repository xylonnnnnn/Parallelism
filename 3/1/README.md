# Задание 1: анализ эффективности умножения матрицы на вектор

## Сборка

```bash
cmake -B build -S . -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
```

## Запуск

```bash
./build/matrix_vector_benchmark \
  --sizes 20000,40000 \
  --threads 1,2,4,7,8,16,20,40 \
  --output results/task1_matrix_vector.csv
```

## Построение графика

```bash
python3 scripts/plot_speedup.py results/task1_matrix_vector.csv results/speedup.png
```
