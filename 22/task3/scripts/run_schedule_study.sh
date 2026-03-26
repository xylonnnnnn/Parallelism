#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_CSV="${ROOT_DIR}/data/results_task3_schedule.csv"

N="${N:-100000}"
EPS="${EPS:-1e-5}"
MAX_ITERS="${MAX_ITERS:-5000}"
REPEATS="${REPEATS:-1}"
THREADS="${THREADS:-8}"
VARIANT="${VARIANT:-2}"

SCHEDULES="${SCHEDULES:-static dynamic guided}"
CHUNKS="${CHUNKS:-0 1 10 100 1000}"

mkdir -p "${ROOT_DIR}/data" "${ROOT_DIR}/reports"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j

rm -f "${RESULTS_CSV}"

for sched in ${SCHEDULES}; do
  for chunk in ${CHUNKS}; do
    if [[ "${sched}" == "static" && "${chunk}" == "0" ]]; then
      echo "Running schedule=${sched}, chunk=default, threads=${THREADS}, variant=${VARIANT}, n=${N}"
    else
      echo "Running schedule=${sched}, chunk=${chunk}, threads=${THREADS}, variant=${VARIANT}, n=${N}"
    fi

    OMP_NUM_THREADS="${THREADS}" "${BUILD_DIR}/task3_solver" \
      --n "${N}" \
      --threads "${THREADS}" \
      --variant "${VARIANT}" \
      --eps "${EPS}" \
      --max-iters "${MAX_ITERS}" \
      --schedule "${sched}" \
      --chunk "${chunk}" \
      --repeats "${REPEATS}" \
      --csv "${RESULTS_CSV}"
  done
done

echo "Schedule study results saved to ${RESULTS_CSV}"
