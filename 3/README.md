# Решение Task3, задание 2

## Что реализовано

- `TaskServer<T>` — шаблонный класс сервера.
- Сервер работает в отдельном потоке.
- Задачи хранятся в `std::queue`.
- Результаты хранятся в `std::unordered_map<size_t, T>`.
- Синхронизация: `std::mutex` + `std::condition_variable`.
- Есть интерфейс:
  - `void start()`
  - `void stop()`
  - `size_t add_task(task)`
  - `T request_result(id_res)`
- Запускаются 3 клиента, каждый в отдельном потоке:
  - клиент `sin` считает `std::sin(x)`;
  - клиент `sqrt` считает `std::sqrt(x)`;
  - клиент `pow` считает `std::pow(x, y)`.
- Каждый клиент пишет результаты в отдельный CSV-файл.
- `verify_results` читает CSV-файлы и проверяет результаты.

## Почему выбраны эти контейнеры и синхронизация

- `std::queue` подходит для очереди задач, потому что сервер обрабатывает задачи в порядке FIFO: добавить в конец и взять из начала.
- `std::unordered_map<size_t, T>` подходит для контейнера результатов, потому что по id задачи нужен быстрый поиск, добавление и удаление.
- `std::condition_variable` лучше постоянного опроса очереди в цикле, потому что поток сервера спит, пока нет задач, и не расходует CPU впустую.
- `std::promise/std::future` здесь не обязательны: задание требует сохранять результат в контейнере и выдавать его через `request_result`. Поэтому ожидание результата реализовано через `condition_variable` и `unordered_map`.

## Сборка

```bash
cmake -B build -S . -DCMAKE_CXX_COMPILER=g++
cmake --build build -j
```

## Запуск

`N` — количество задач на каждого клиента, должно быть `5 < N < 10000`.

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
