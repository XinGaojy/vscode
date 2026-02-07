#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
OUT_DIR="${ROOT_DIR}/data/out"
INPUT_FILE="${ROOT_DIR}/data/large_input.txt"

SERIES_COUNT="${SERIES_COUNT:-200}"
POINTS_PER_SERIES="${POINTS_PER_SERIES:-1000}"
START_TS="${START_TS:-1769866000}"
STEP="${STEP:-1}"
PARALLEL="${PARALLEL:-4}"
ITERATIONS="${ITERATIONS:-50}"
QUERY_THREADS="${QUERY_THREADS:-1}"

echo "Building with CMake..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}"

echo "Generating data: series=${SERIES_COUNT}, points/series=${POINTS_PER_SERIES}"
"${BUILD_DIR}/tsdb_gen" "${INPUT_FILE}" "${SERIES_COUNT}" "${POINTS_PER_SERIES}" "${START_TS}" "${STEP}"

echo "Building storage..."
"${BUILD_DIR}/tsdb" build "${INPUT_FILE}" "${OUT_DIR}"

echo "Running stress queries: iterations=${ITERATIONS}, parallel=${PARALLEL}"
seq 1 "${ITERATIONS}" | xargs -P "${PARALLEL}" -I {} \
  "${BUILD_DIR}/tsdb" query "${OUT_DIR}" \
  metric=cpu tag=domain=beijing start="${START_TS}" end="$((START_TS + 100))" \
  fields=min,max,avg,sum,count threads="${QUERY_THREADS}" > /dev/null

echo "Done."
