#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

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



def load_rows(csv_path: Path):
    if not csv_path.exists():
        return []
    with csv_path.open(newline='', encoding='utf-8') as fh:
        return list(csv.DictReader(fh))


def chart(rows, out_path: Path) -> bool:
    rows = sorted(rows, key=lambda r: int(r['threads']))
    if not rows:
        return False
    threads = [int(r['threads']) for r in rows]
    speedup = [float(r['speedup']) for r in rows]
    plt.figure(figsize=(7.0, 4.2))
    plt.plot(threads, threads, linestyle='--', marker='o', label='Линейное ускорение')
    plt.plot(threads, speedup, marker='o', label='Измеренное ускорение')
    plt.xlabel('Количество потоков')
    plt.ylabel('Ускорение')
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=180)
    plt.close()
    return True


def table_data(rows):
    if not rows:
        return [['Потоки', 'T1', 'Tp', 'Sp'], *[[str(t), '', '', ''] for t in THREADS_ORDER]]
    by_thread = {int(r['threads']): r for r in rows}
    data = [['Потоки', 'T1', 'Tp', 'Sp']]
    for t in THREADS_ORDER:
        r = by_thread.get(t)
        if not r:
            data.append([str(t), '', '', ''])
            continue
        data.append([
            str(t),
            f"{float(r['serial_seconds']):.3f}",
            f"{float(r['omp_seconds']):.3f}",
            f"{float(r['speedup']):.3f}",
        ])
    return data


def build_pdf(csv_path: Path, out_path: Path):
    rows = load_rows(csv_path)
    chart_path = out_path.with_suffix('.png')
    has_chart = chart(rows, chart_path)

    register_fonts()
    styles = getSampleStyleSheet()
    styles.add(ParagraphStyle(name='BodyRu', parent=styles['BodyText'], fontName='DejaVuSans', fontSize=10.5, leading=14))
    styles.add(ParagraphStyle(name='TitleRu', parent=styles['Title'], fontName='DejaVuSans-Bold', fontSize=18, leading=22, textColor=colors.HexColor('#183153')))
    styles.add(ParagraphStyle(name='HeadingRu', parent=styles['Heading2'], fontName='DejaVuSans-Bold', fontSize=13, leading=16, textColor=colors.HexColor('#234B7D')))

    doc = SimpleDocTemplate(str(out_path), pagesize=A4, leftMargin=1.7 * cm, rightMargin=1.7 * cm, topMargin=1.6 * cm, bottomMargin=1.5 * cm)
    story = [
        Paragraph('Задание 2. Отчёт по OpenMP-версии численного интегрирования', styles['TitleRu']),
        Spacer(1, 0.25 * cm),
        Paragraph(
            'Этот шаблон предназначен для оформления замеров ускорения программы численного интегрирования при '
            'использовании 1, 2, 4, 7, 8, 16, 20 и 40 потоков.',
            styles['BodyRu']),
        Spacer(1, 0.25 * cm),
        Paragraph('1. Экспериментальная установка', styles['HeadingRu']),
        Paragraph(
            'Добавьте описание CPU, сервера, NUMA-конфигурации и операционной системы. Зафиксируйте число шагов: '
            '`nsteps = 40 000 000`.',
            styles['BodyRu']),
        Spacer(1, 0.2 * cm),
        Paragraph('2. Таблица результатов', styles['HeadingRu']),
    ]

    table = Table(table_data(rows), repeatRows=1)
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
    story.append(table)
    story.extend([
        Spacer(1, 0.3 * cm),
        Paragraph('3. График ускорения', styles['HeadingRu']),
    ])

    if has_chart:
        story.append(Image(str(chart_path), width=15.5 * cm, height=9.0 * cm))
    else:
        story.append(Paragraph('График будет автоматически построен после заполнения CSV-файла.', styles['BodyRu']))

    story.extend([
        Spacer(1, 0.2 * cm),
        Paragraph('4. Вывод', styles['HeadingRu']),
        Paragraph(
            'В выводе отметьте, насколько близко ускорение к линейному, в какой момент начинается насыщение и как '
            'влияют накладные расходы на синхронизацию и доступ к памяти.',
            styles['BodyRu']),
    ])

    doc.build(story)
    if has_chart and chart_path.exists():
        chart_path.unlink()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--csv', type=Path, default=Path('data/results_task2.csv'))
    parser.add_argument('--out', type=Path, default=Path('reports/task2_report_template.pdf'))
    args = parser.parse_args()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    build_pdf(args.csv, args.out)
