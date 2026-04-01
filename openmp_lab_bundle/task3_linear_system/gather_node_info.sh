#!/usr/bin/env bash
set -euo pipefail

mkdir -p tables
{
  echo "===== product_name ====="
  cat /sys/devices/virtual/dmi/id/product_name || true
  echo
  echo "===== lscpu ====="
  lscpu || true
  echo
  echo "===== numactl --hardware ====="
  numactl --hardware || true
  echo
  echo "===== /etc/os-release ====="
  cat /etc/os-release || true
} > tables/node_description.txt

echo "Saved node description to tables/node_description.txt"
