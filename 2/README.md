# Решения по заданиям

Сделано по присланным файлам:
- `Tasks.pdf`
- `Лабораторная работа.pdf`
- семинар 1 OpenMP
- семинар 2 OpenMP
- правила замера времени

## Что входит

- `src/task1_matvec.c` - задание 1, C, умножение матрицы на вектор, есть параллельная инициализация.
- `src/task2_integrate.c` - задание 2, C, численное интегрирование.
  - `atomic`
  - `atomic_local` (вариант из задания: `#pragma omp atomic` + локальная переменная)
- `src/task3_slae.cpp` - задание 3, C++, метод простой итерации (Якоби), два варианта OpenMP:
  - `v1`: отдельный `#pragma omp parallel for` для каждого цикла
  - `v2`: одна общая `#pragma omp parallel` на весь итерационный алгоритм
- `Makefile` - сборка через `make`
- `scripts/run_task1.sh`, `scripts/run_task2.sh`, `scripts/run_task3.sh` - шаблоны массовых замеров

## Сборка

```bash
make
```

## Задание 1

Сборка:

```bash
make bin/task1_matvec
```

Запуск:

```bash
./bin/task1_matvec 20000 20000 8 omp
./bin/task1_matvec 20000 20000 1 serial
```

Формат:

```bash
./bin/task1_matvec <m> <n> <threads> <mode>
mode = serial | omp
```

Для задания использовать:
- размеры `20000x20000`, `40000x40000`
- потоки `1 2 4 7 8 16 20 40`

Массовые замеры:

```bash
./scripts/run_task1.sh
```

## Задание 2

Сборка:

```bash
make bin/task2_integrate
```

Запуск:

```bash
./bin/task2_integrate 8 40000000 atomic_local
./bin/task2_integrate 1 40000000 serial
```

Формат:

```bash
./bin/task2_integrate <threads> <nsteps> <mode>
mode = serial | atomic | atomic_local
```

По заданию основной вариант:
- `atomic_local`
- `nsteps = 40000000`
- потоки `1 2 4 7 8 16 20 40`

Массовые замеры:

```bash
./scripts/run_task2.sh
```

## Задание 3

Сборка:

```bash
make bin/task3_slae
```

Запуск:

```bash
./bin/task3_slae 12000 8 v1 1e-5 10000 static
./bin/task3_slae 12000 8 v2 1e-5 10000 dynamic:64
```

Формат:

```bash
./bin/task3_slae <n> <threads> <variant> <eps> <max_iters> <schedule[:chunk]>
variant = v1 | v2
schedule = static | dynamic | guided | auto
```

Что исследовать по лабораторной:
- сравнение `v1` и `v2`
- графики времени, ускорения, эффективности
- исследование `schedule(...)` при фиксированных `n` и числе потоков

Массовые замеры:

```bash
VARIANT=v1 SCHEDULE=static ./scripts/run_task3.sh
VARIANT=v2 SCHEDULE=guided:64 ./scripts/run_task3.sh
```

## Сведения об узле

Команды для отчета:

```bash
lscpu
cat /sys/devices/virtual/dmi/id/product_name
numactl --hardware
cat /etc/os-release
```

## Привязка потоков/процесса

Как в `Tasks.pdf`:

```bash
taskset -c 1,2,3 ./bin/task2_integrate 3 40000000 atomic_local
numactl -N 0 ./bin/task2_integrate 8 40000000 atomic_local
numactl -C 4,5,6 ./bin/task2_integrate 3 40000000 atomic_local
```

## Что еще нужно сделать на сервере

Я подготовил код и шаблоны запусков, но фактические значения времени, таблицы, графики и итоговый PDF-отчет надо получать на вашем сервере, потому что в задании прямо сказано проводить вычисления на сервере.

Для отчета:
1. собрать программы;
2. прогнать серии замеров несколько раз;
3. усреднить время, отбросив выбросы;
4. посчитать ускорение `S(p) = T(1) / T(p)`;
5. посчитать эффективность `E(p) = S(p) / p`;
6. построить графики и написать вывод.
