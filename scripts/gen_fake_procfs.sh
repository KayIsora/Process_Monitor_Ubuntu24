#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:-/tmp/fake_pm}"
rm -rf "$ROOT"; mkdir -p "$ROOT"/{cpu,mem,inf}
for n in cpu mem inf; do
  echo "ok=1 retries=0 ts=$(date +%s)" > "$ROOT/$n/status"
  echo "sample-$n-data" > "$ROOT/$n/data"
  printf "" > "$ROOT/$n/config"
done
echo "[fake procfs] at $ROOT"
