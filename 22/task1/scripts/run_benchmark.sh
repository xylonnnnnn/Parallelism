#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_CSV="${ROOT_DIR}/data/results_task1.csv"
THREADS=(1 2 4 7 8 16 20 40)
SIZES=(20000 40000)
REPEATS="${REPEATS:-1}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j

rm -f "${RESULTS_CSV}"
for n in "${SIZES[@]}"; do
  for t in "${THREADS[@]}"; do
    echo "Running n=${n}, threads=${t}"
    OMP_NUM_THREADS="${t}" "${BUILD_DIR}/task1_matvec" \
      --n "${n}" \
      --threads "${t}" \
      --repeats "${REPEATS}" \
      --csv "${RESULTS_CSV}"
  done
done

echo "Results saved to ${RESULTS_CSV}"
