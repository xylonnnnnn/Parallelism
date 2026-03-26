#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib.pyplot as plt
from reportlab.lib import colors
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import cm
from reportlab.platypus import Image, PageBreak, Paragraph, SimpleDocTemplate, Spacer, Table, TableStyle

THREADS_ORDER = [1, 2, 4, 7, 8, 16, 20, 40]

FONT_REGULAR = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def register_fonts() -> None:
    pdfmetrics.registerFont(TTFont("DejaVuSans", FONT_REGULAR))
    pdfmetrics.registerFont(TTFont("DejaVuSans-Bold", FONT_BOLD))



def load_rows(path: Path) -> List[dict]:
    if not path.exists():
        return []
    with path.open(newline='', encoding='utf-8') as fh:
        return list(csv.DictReader(fh))


def speedup_by_variant(rows: List[dict]) -> Dict[int, List[Tuple[int, float]]]:
    grouped: Dict[int, Dict[int, float]] = {}
    for row in rows:
        variant = int(row['variant'])
        threads = int(row['threads'])
        grouped.setdefault(variant, {})[threads] = float(row['seconds'])

    out: Dict[int, List[Tuple[int, float]]] = {}
    for variant, values in grouped.items():
        if 1 not in values:
            continue
        t1 = values[1]
        out[variant] = [(t, t1 / values[t]) for t in THREADS_ORDER if t in values]
    return out


def efficiency_by_variant(speedup: Dict[int, List[Tuple[int, float]]]) -> Dict[int, List[Tuple[int, float]]]:
    return {variant: [(t, s / t) for t, s in series] for variant, series in speedup.items()}


def chart_time(rows: List[dict], out_path: Path) -> bool:
    if not rows:
        return False
    plt.figure(figsize=(7.0, 4.2))
    grouped: Dict[int, List[Tuple[int, float]]] = {}
    for row in rows:
        grouped.setdefault(int(row['variant']), []).append((int(row['threads']), float(row['seconds'])))
    for variant, series in sorted(grouped.items()):
        series.sort()
        plt.plot([t for t, _ in series], [v for _, v in series], marker='o', label=f'Вариант {variant}')
    plt.xlabel('Количество потоков')
    plt.ylabel('Время, с')
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=180)
    plt.close()
    return True


def chart_metric(series_map: Dict[int, List[Tuple[int, float]]], out_path: Path, ylabel: str, linear: bool = False) -> bool:
    if not series_map:
        return False
    plt.figure(figsize=(7.0, 4.2))
    max_threads = max(max(t for t, _ in series) for series in series_map.values())
    if linear:
        plt.plot([1, max_threads], [1, max_threads], linestyle='--', marker='o', label='Линейное ускорение')
    for variant, series in sorted(series_map.items()):
        plt.plot([t for t, _ in series], [v for _, v in series], marker='o', label=f'Вариант {variant}')
    plt.xlabel('Количество потоков')
    plt.ylabel(ylabel)
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=180)
    plt.close()
    return True


def schedule_chart(rows: List[dict], out_path: Path) -> bool:
    if not rows:
        return False
    labels = [f"{r['schedule']}:{r['chunk']}" for r in rows]
    values = [float(r['seconds']) for r in rows]
    plt.figure(figsize=(7.4, 4.8))
    plt.bar(range(len(values)), values)
    plt.xticks(range(len(values)), labels, rotation=25, ha='right')
    plt.ylabel('Время, с')
    plt.grid(True, axis='y', alpha=0.3)
    plt.tight_layout()
    plt.savefig(out_path, dpi=180)
    plt.close()
    return True


def variants_table(rows: List[dict]) -> List[List[str]]:
    header = ['Потоки', 'V1 T, c', 'V1 S', 'V1 E', 'V2 T, c', 'V2 S', 'V2 E']
    if not rows:
        return [header] + [[str(t), '', '', '', '', '', ''] for t in THREADS_ORDER]

    time_by_variant: Dict[int, Dict[int, float]] = {1: {}, 2: {}}
    for row in rows:
        time_by_variant[int(row['variant'])][int(row['threads'])] = float(row['seconds'])

    t1_v1 = time_by_variant[1].get(1)
    t1_v2 = time_by_variant[2].get(1)
    data = [header]
    for t in THREADS_ORDER:
        v1 = time_by_variant[1].get(t)
        v2 = time_by_variant[2].get(t)
        data.append([
            str(t),
            '' if v1 is None else f'{v1:.3f}',
            '' if v1 is None or t1_v1 is None else f'{t1_v1 / v1:.3f}',
            '' if v1 is None or t1_v1 is None else f'{(t1_v1 / v1) / t:.3f}',
            '' if v2 is None else f'{v2:.3f}',
            '' if v2 is None or t1_v2 is None else f'{t1_v2 / v2:.3f}',
            '' if v2 is None or t1_v2 is None else f'{(t1_v2 / v2) / t:.3f}',
        ])
    return data


def schedule_table(rows: List[dict]) -> List[List[str]]:
    header = ['Schedule', 'Chunk', 'Время, с', 'Итерации', 'Невязка']
    if not rows:
        return [header, ['static', '0', '', '', ''], ['dynamic', '1', '', '', ''], ['guided', '100', '', '', '']]
    data = [header]
    for row in rows:
        data.append([
            row['schedule'],
            row['chunk'],
            f"{float(row['seconds']):.3f}",
            row['iterations'],
            f"{float(row['residual']):.3e}",
        ])
    return data


def styled_table(data):
    table = Table(data, repeatRows=1)
    table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#DDEAF7')),
        ('TEXTCOLOR', (0, 0), (-1, 0), colors.HexColor('#183153')),
        ('FONTNAME', (0, 0), (-1, 0), 'DejaVuSans-Bold'),
        ('FONTNAME', (0, 1), (-1, -1), 'DejaVuSans'),
        ('GRID', (0, 0), (-1, -1), 0.4, colors.HexColor('#A9B7C6')),
        ('ALIGN', (0, 0), (-1, -1), 'CENTER'),
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
        ('PADDING', (0, 0), (-1, -1), 6),
        ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, colors.HexColor('#F7FAFD')]),
    ]))
    return table


def build_pdf(variants_csv: Path, schedule_csv: Path, out_path: Path):
    variant_rows = load_rows(variants_csv)
    schedule_rows = load_rows(schedule_csv)

    speedup = speedup_by_variant(variant_rows)
    efficiency = efficiency_by_variant(speedup)

    time_png = out_path.with_name(out_path.stem + '_time.png')
    speedup_png = out_path.with_name(out_path.stem + '_speedup.png')
    efficiency_png = out_path.with_name(out_path.stem + '_efficiency.png')
    schedule_png = out_path.with_name(out_path.stem + '_schedule.png')

    has_time = chart_time(variant_rows, time_png)
    has_speedup = chart_metric(speedup, speedup_png, 'Ускорение', linear=True)
    has_eff = chart_metric(efficiency, efficiency_png, 'Эффективность')
    has_schedule = schedule_chart(schedule_rows, schedule_png)

    register_fonts()
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name='BodyRu', parent=styles['BodyText'], fontName='DejaVuSans', fontSize=10.2, leading=14))
    styles.add(ParagraphStyle(name='TitleRu', parent=styles['Title'], fontName='DejaVuSans-Bold', fontSize=18, leading=22, textColor=colors.HexColor('#183153')))
    styles.add(ParagraphStyle(name='HeadingRu', parent=styles['Heading2'], fontName='DejaVuSans-Bold', fontSize=13, leading=16, textColor=colors.HexColor('#234B7D')))

    doc = SimpleDocTemplate(str(out_path), pagesize=A4, leftMargin=1.6 * cm, rightMargin=1.6 * cm, topMargin=1.5 * cm, bottomMargin=1.4 * cm)
    story = [
        Paragraph('Задание 3. Лабораторная работа №2 — OpenMP-реализация метода простой итерации', styles['TitleRu']),
        Spacer(1, 0.25 * cm),
        Paragraph(
            'Шаблон отчёта для сравнения двух вариантов OpenMP-программы: с отдельными `parallel for` и с одной '
            'внешней параллельной областью. Здесь же оформляется исследование параметров schedule.',
            styles['BodyRu']),
        Spacer(1, 0.25 * cm),
        Paragraph('1. Методика эксперимента', styles['HeadingRu']),
        Paragraph(
            'Укажите размер задачи N, точность eps, выбранное значение tau, число потоков и характеристики вычислительного узла. '
            'Размер N нужно подобрать так, чтобы запуск на одном ядре занимал не менее 30 секунд.',
            styles['BodyRu']),
        Spacer(1, 0.2 * cm),
        Paragraph('2. Сравнение двух вариантов программы', styles['HeadingRu']),
        styled_table(variants_table(variant_rows)),
        Spacer(1, 0.25 * cm),
    ]

    if has_time:
        story.append(Image(str(time_png), width=15.8 * cm, height=9.0 * cm))
        story.append(Spacer(1, 0.15 * cm))
    else:
        story.append(Paragraph('График времени появится после заполнения `results_task3_variants.csv`.', styles['BodyRu']))

    if has_speedup:
        story.append(Image(str(speedup_png), width=15.8 * cm, height=9.0 * cm))
        story.append(Spacer(1, 0.15 * cm))
    else:
        story.append(Paragraph('График ускорения появится после заполнения `results_task3_variants.csv`.', styles['BodyRu']))

    if has_eff:
        story.append(Image(str(efficiency_png), width=15.8 * cm, height=9.0 * cm))
    else:
        story.append(Paragraph('График эффективности появится после заполнения `results_task3_variants.csv`.', styles['BodyRu']))

    story.extend([
        Spacer(1, 0.2 * cm),
        Paragraph('3. Исследование schedule', styles['HeadingRu']),
        styled_table(schedule_table(schedule_rows)),
        Spacer(1, 0.2 * cm),
    ])

    if has_schedule:
        story.append(Image(str(schedule_png), width=15.8 * cm, height=9.6 * cm))
    else:
        story.append(Paragraph('График по schedule появится после заполнения `results_task3_schedule.csv`.', styles['BodyRu']))

    story.extend([
        Spacer(1, 0.2 * cm),
        Paragraph('4. Вывод', styles['HeadingRu']),
        Paragraph(
            'В выводе сравните накладные расходы обоих вариантов, укажите оптимальную стратегию распараллеливания и '
            'сделайте вывод о наиболее выгодном типе schedule и размере chunk для выбранной задачи.',
            styles['BodyRu']),
    ])

    doc.build(story)
    for path, flag in [(time_png, has_time), (speedup_png, has_speedup), (efficiency_png, has_eff), (schedule_png, has_schedule)]:
        if flag and path.exists():
            path.unlink()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--variants', type=Path, default=Path('data/results_task3_variants.csv'))
    parser.add_argument('--schedule', type=Path, default=Path('data/results_task3_schedule.csv'))
    parser.add_argument('--out', type=Path, default=Path('reports/task3_report_template.pdf'))
    args = parser.parse_args()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    build_pdf(args.variants, args.schedule, args.out)
