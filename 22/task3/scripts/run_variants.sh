#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_CSV="${ROOT_DIR}/data/results_task3_variants.csv"

THREADS_STR="${THREADS:-1 2 4 7 8 16 20 40}"
VARIANTS_STR="${VARIANTS:-1 2}"

N="${N:-100000}"
EPS="${EPS:-1e-5}"
MAX_ITERS="${MAX_ITERS:-5000}"
REPEATS="${REPEATS:-1}"
SCHEDULE="${SCHEDULE:-static}"
CHUNK="${CHUNK:-0}"

mkdir -p "${ROOT_DIR}/data" "${ROOT_DIR}/reports"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j

rm -f "${RESULTS_CSV}"

for variant in ${VARIANTS_STR}; do
  for t in ${THREADS_STR}; do
    echo "Running variant=${variant}, threads=${t}, n=${N}, eps=${EPS}, max_iters=${MAX_ITERS}"
    OMP_NUM_THREADS="${t}" "${BUILD_DIR}/task3_solver" \
      --n "${N}" \
      --threads "${t}" \
      --variant "${variant}" \
      --eps "${EPS}" \
      --max-iters "${MAX_ITERS}" \
      --schedule "${SCHEDULE}" \
      --chunk "${CHUNK}" \
      --repeats "${REPEATS}" \
      --csv "${RESULTS_CSV}"
  done
done

echo "Results saved to ${RESULTS_CSV}"