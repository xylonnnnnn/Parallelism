# Task 5: YOLOv8s-pose на CPU

## Что делает программа

Скрипт обрабатывает видео моделью `yolov8s-pose` на CPU и сохраняет новое видео с нанесенными keypoints.

Реализованы два режима:

1. `single` — последовательная обработка кадров в одном основном потоке.
2. `multi` — многопоточный pipeline:
   - кадры читаются из видео и кладутся во входную очередь;
   - рабочие потоки забирают кадры из очереди;
   - каждый рабочий поток имеет собственный экземпляр YOLO;
   - обработанные кадры кладутся в выходную очередь;
   - главный поток восстанавливает исходный порядок по индексу кадра;
   - кадры сразу записываются в выходное видео.

Также есть режим `--benchmark`, который запускает обработку с разным количеством потоков и помогает подобрать оптимальное число worker-потоков для конкретного CPU.

Дополнительно реализован режим камеры `--camera`, который показывает keypoints в реальном времени.

## Установка

Рекомендуется использовать виртуальное окружение.

```bash
python -m venv .venv
```

Windows:

```bash
.venv\Scripts\activate
```

Linux/macOS:

```bash
source .venv/bin/activate
```

Установить зависимости:

```bash
pip install ultralytics opencv-python numpy torch
```

При первом запуске Ultralytics автоматически скачает файл модели `yolov8s-pose.pt`, если его нет в рабочей папке.

## Запуск в однопоточном режиме

```bash
python main.py --video input.mp4 --mode single --output output_single.mp4
```

После завершения программа выведет:

- режим обработки;
- количество worker-потоков;
- количество потоков PyTorch на worker;
- количество кадров;
- время обработки всех кадров;
- FPS обработки;
- среднее время на кадр;
- путь к выходному видео.

## Запуск в многопоточном режиме

```bash
python main.py --video input.mp4 --mode multi --threads 4 --output output_multi_4.mp4
```

Параметр `--threads` задает количество worker-потоков. Каждый worker создает отдельный экземпляр модели YOLO, что соответствует thread-safe подходу для Ultralytics.

## Подбор оптимального количества потоков

```bash
python main.py --video input.mp4 --benchmark --benchmark-threads 1,2,3,4,6,8 --benchmark-dir benchmark_out
```

Скрипт сначала запускает `single`, затем `multi` для каждого значения из `--benchmark-threads`. В конце выводится таблица:

```text
mode    workers torch_threads frames elapsed_s fps ms_per_frame speedup
```

Оптимальным считается вариант с минимальным `elapsed_s`. На разных компьютерах лучший результат может быть разным. Часто максимум ускорения достигается не на самом большом числе потоков, потому что каждый worker запускает тяжелый CPU-инференс, а PyTorch тоже использует внутренние CPU-потоки.

## Настройка потоков PyTorch

По умолчанию программа задает число CPU-потоков PyTorch на worker так:

```text
os.cpu_count() // workers
```

Это уменьшает oversubscription, когда слишком много worker-потоков и слишком много внутренних потоков PyTorch конкурируют за одни и те же CPU-ядра.

Можно задать значение вручную:

```bash
python main.py --video input.mp4 --mode multi --threads 4 --torch-threads 1 --output output.mp4
```

## Режим камеры

```bash
python main.py --camera --camera-source 0 --threads 4
```

Выход из окна камеры — клавиша `q`.

Чтобы дополнительно записывать обработанный видеопоток:

```bash
python main.py --camera --camera-source 0 --threads 4 --record camera_output.mp4
```
