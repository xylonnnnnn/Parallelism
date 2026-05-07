# Задание 2

## Сборка

```bash
cmake -B build -S . -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
```

## Запуск

```bash
./build/task3_server 1000
```

После запуска появятся файлы:

```text
results/client_sin.csv
results/client_sqrt.csv
results/client_pow.csv
```

## Проверка результатов

```bash
./build/verify_results
```

Ожидаемый результат:

```text
results/client_sin.csv: OK, rows=1000
results/client_sqrt.csv: OK, rows=1000
results/client_pow.csv: OK, rows=1000
All checks passed. Total rows=3000
```
