#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
RESULTS_CSV="${ROOT_DIR}/data/results_task2.csv"
THREADS=(1 2 4 7 8 16 20 40)
NSTEPS="${NSTEPS:-40000000}"
REPEATS="${REPEATS:-1}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j

rm -f "${RESULTS_CSV}"
for t in "${THREADS[@]}"; do
  echo "Running nsteps=${NSTEPS}, threads=${t}"
  OMP_NUM_THREADS="${t}" "${BUILD_DIR}/task2_integration" \
    --nsteps "${NSTEPS}" \
    --threads "${t}" \
    --repeats "${REPEATS}" \
    --csv "${RESULTS_CSV}"
done

echo "Results saved to ${RESULTS_CSV}"
