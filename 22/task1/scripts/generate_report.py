#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib.pyplot as plt
from reportlab.lib import colors
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import cm
from reportlab.platypus import Image, Paragraph, SimpleDocTemplate, Spacer, Table, TableStyle

THREADS_ORDER = [1, 2, 4, 7, 8, 16, 20, 40]

FONT_REGULAR = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FONT_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def register_fonts() -> None:
    pdfmetrics.registerFont(TTFont("DejaVuSans", FONT_REGULAR))
    pdfmetrics.registerFont(TTFont("DejaVuSans-Bold", FONT_BOLD))



def load_rows(csv_path: Path) -> List[dict]:
    if not csv_path.exists():
        return []
    with csv_path.open(newline='', encoding='utf-8') as fh:
        reader = csv.DictReader(fh)
        return list(reader)


def build_speedup(rows: List[dict]) -> Dict[int, List[Tuple[int, float]]]:
    grouped: Dict[int, Dict[int, float]] = defaultdict(dict)
    for row in rows:
        n = int(row['n'])
        t = int(row['threads'])
        total = float(row['total_seconds'])
        grouped[n][t] = total

    speedups: Dict[int, List[Tuple[int, float]]] = {}
    for n, by_threads in grouped.items():
        if 1 not in by_threads:
            continue
        t1 = by_threads[1]
        series = []
        for threads in THREADS_ORDER:
            if threads in by_threads:
                series.append((threads, t1 / by_threads[threads]))
        speedups[n] = series
    return speedups


def draw_chart(speedups: Dict[int, List[Tuple[int, float]]], out_path: Path) -> bool:
    if not speedups:
        return False
    plt.figure(figsize=(7.0, 4.2))
    max_threads = max(max(t for t, _ in series) for series in speedups.values())
    plt.plot([1, max_threads], [1, max_threads], linestyle='--', marker='o', label='Линейное ускорение')
    for n, series in sorted(speedups.items()):
        plt.plot([t for t, _ in series], [s for _, s in series], marker='o', label=f'N={n}')
    plt.xlabel('Количество потоков')
    plt.ylabel('Ускорение')
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=180)
    plt.close()
    return True


def build_table(rows: List[dict]) -> List[List[str]]:
    if not rows:
        header = ['N', 'T1'] + [f'T{t}' for t in THREADS_ORDER[1:]]
        return [header, ['20000'] + [''] * len(THREADS_ORDER), ['40000'] + [''] * len(THREADS_ORDER)]

    grouped: Dict[int, Dict[int, float]] = defaultdict(dict)
    for row in rows:
        grouped[int(row['n'])][int(row['threads'])] = float(row['total_seconds'])

    header = ['N'] + [f'T{t}' for t in THREADS_ORDER]
    table = [header]
    for n in sorted(grouped):
        row = [str(n)]
        for threads in THREADS_ORDER:
            value = grouped[n].get(threads)
            row.append('' if value is None else f'{value:.3f}')
        table.append(row)
    return table


def build_pdf(csv_path: Path, out_path: Path) -> None:
    rows = load_rows(csv_path)
    speedups = build_speedup(rows)
    chart_path = out_path.with_suffix('.png')
    chart_exists = draw_chart(speedups, chart_path)

    register_fonts()
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name='BodyRu', parent=styles['BodyText'], fontName='DejaVuSans', fontSize=10.5, leading=14))
    styles.add(ParagraphStyle(name='TitleRu', parent=styles['Title'], fontName='DejaVuSans-Bold', fontSize=18, leading=22, textColor=colors.HexColor('#183153')))
    styles.add(ParagraphStyle(name='HeadingRu', parent=styles['Heading2'], fontName='DejaVuSans-Bold', fontSize=13, leading=16, textColor=colors.HexColor('#234B7D')))

    doc = SimpleDocTemplate(str(out_path), pagesize=A4, leftMargin=1.7 * cm, rightMargin=1.7 * cm, topMargin=1.6 * cm, bottomMargin=1.5 * cm)
    story = []
    story.append(Paragraph('Задание 1. Отчёт по OpenMP-версии умножения матрицы на вектор', styles['TitleRu']))
    story.append(Spacer(1, 0.25 * cm))
    story.append(Paragraph(
        'Шаблон предназначен для быстрого оформления результата после запуска экспериментов на сервере. '
        'Сюда следует добавить описание вычислительного узла, таблицу замеров, график ускорения и вывод о масштабируемости.',
        styles['BodyRu']))

    story.append(Spacer(1, 0.3 * cm))
    story.append(Paragraph('1. Что включить в итоговый отчёт', styles['HeadingRu']))
    story.append(Paragraph(
        'Укажите CPU и сервер, число NUMA-узлов, объём памяти по узлам, операционную систему, '
        'а также режим запуска без привязки и, при желании, результаты с taskset/numactl.',
        styles['BodyRu']))

    story.append(Spacer(1, 0.2 * cm))
    story.append(Paragraph('2. Таблица времени выполнения', styles['HeadingRu']))
    table_data = build_table(rows)
    table = Table(table_data, repeatRows=1)
    table.setStyle(TableStyle([
        ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#DDEAF7')),
        ('TEXTCOLOR', (0, 0), (-1, 0), colors.HexColor('#183153')),
        ('FONTNAME', (0, 0), (-1, 0), 'DejaVuSans-Bold'),
        ('FONTNAME', (0, 1), (-1, -1), 'DejaVuSans'),
        ('GRID', (0, 0), (-1, -1), 0.4, colors.HexColor('#A9B7C6')),
        ('ALIGN', (1, 1), (-1, -1), 'CENTER'),
        ('VALIGN', (0, 0), (-1, -1), 'MIDDLE'),
        ('PADDING', (0, 0), (-1, -1), 6),
        ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, colors.HexColor('#F7FAFD')]),
    ]))
    story.append(table)

    story.append(Spacer(1, 0.3 * cm))
    story.append(Paragraph('3. График ускорения', styles['HeadingRu']))
    if chart_exists:
        story.append(Image(str(chart_path), width=15.5 * cm, height=9.0 * cm))
    else:
        story.append(Paragraph(
            'График появится автоматически после заполнения CSV-файла `data/results_task1.csv` и повторного запуска генератора отчёта.',
            styles['BodyRu']))

    story.append(Spacer(1, 0.2 * cm))
    story.append(Paragraph('4. Вывод', styles['HeadingRu']))
    story.append(Paragraph(
        'В заключении стоит сравнить фактическое ускорение с линейным, отметить насыщение по потокам и влияние памяти/NUMA. '
        'Если вы проводили привязку потоков к ядрам или NUMA-узлам, отдельно сравните её с базовым запуском.',
        styles['BodyRu']))

    doc.build(story)
    if chart_exists and chart_path.exists():
        chart_path.unlink()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--csv', type=Path, default=Path('data/results_task1.csv'))
    parser.add_argument('--out', type=Path, default=Path('reports/task1_report_template.pdf'))
    args = parser.parse_args()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    build_pdf(args.csv, args.out)
