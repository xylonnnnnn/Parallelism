# Задание 3

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
./build/task3_linear_system --n 10000000 --eps 1e-5 --repeats 3
```
