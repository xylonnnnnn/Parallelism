#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_CSV="${ROOT_DIR}/data/results_task3_schedule.csv"
N="${N:-1000000}"
THREADS="${THREADS:-8}"
VARIANT="${VARIANT:-2}"
EPS="${EPS:-1e-5}"
MAX_ITERS="${MAX_ITERS:-1000000}"
REPEATS="${REPEATS:-1}"

SCHEDULES=("static:0" "static:1" "dynamic:1" "dynamic:100" "guided:1" "guided:100" "auto:0")

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j

rm -f "${RESULTS_CSV}"
for item in "${SCHEDULES[@]}"; do
  schedule="${item%%:*}"
  chunk="${item##*:}"
  echo "Running schedule=${schedule}, chunk=${chunk}"
  OMP_NUM_THREADS="${THREADS}" "${BUILD_DIR}/task3_solver" \
    --n "${N}" \
    --threads "${THREADS}" \
    --variant "${VARIANT}" \
    --eps "${EPS}" \
    --max-iters "${MAX_ITERS}" \
    --schedule "${schedule}" \
    --chunk "${chunk}" \
    --repeats "${REPEATS}" \
    --csv "${RESULTS_CSV}"
done

echo "Schedule study saved to ${RESULTS_CSV}"
