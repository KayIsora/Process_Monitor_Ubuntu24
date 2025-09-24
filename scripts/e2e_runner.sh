#!/usr/bin/env bash
set -euo pipefail
mkdir -p logs
# 1) chạy vts offline
build/bin/vts -f logs/vts_input.fifo -c 65536 -m 10 &
VTS_PID=$!
sleep 1
# 2) stress 100k msg/s trong 60s
build/bin/stress_writer logs/vts_input.fifo 100000
# 3) dừng và kiểm tra shutdown <1s
t0=$(date +%s%3N); kill -TERM $VTS_PID; wait $VTS_PID || true
t1=$(date +%s%3N); dt=$((t1-t0))
echo "Shutdown ms=$dt"
# 4) in metrics (đã in stderr khi exit), và kiểm tra log rotate tồn tại
ls -lh logs/vts.log*
