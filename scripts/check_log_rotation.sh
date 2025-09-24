#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="$ROOT_DIR/build/bin"
LOG_DIR="$ROOT_DIR/logs"
LOG_FILE="$LOG_DIR/vts.log"

mkdir -p "$LOG_DIR"
:> "$LOG_FILE"

# Start/Restart VTS
pkill -f "/build/bin/vts" >/dev/null 2>&1 || true
VTS_PORT=9000 VTS_UDP_PORT=9001 VTS_DEDUP_WARM_FILE="$LOG_DIR/vts_reqids.bin" \
  nohup "$BIN_DIR/vts" >>"$LOG_FILE" 2>&1 & echo $! > "$ROOT_DIR/vts.pid"
sleep 1

inode_of() { stat -c %i "$1" 2>/dev/null || echo 0; }
size_of()  { stat -c %s "$1" 2>/dev/null || echo 0; }

inode0="$(inode_of "$LOG_FILE")"
echo "[CHK] start inode=$inode0 size=$(size_of "$LOG_FILE")"

# Tạo tải TCP PING lớn (để log do VTS tự viết, không dùng dd)
("$BIN_DIR/stress_vts_client" -H 127.0.0.1 -p 9000 -c 16 -n 40000 >/dev/null 2>&1) &
CLIENT_PID=$!

rotations=0
max_wait=90
prev_ok=0
for s in $(seq 1 $max_wait); do
  sz=$(size_of "$LOG_FILE")
  ok=$(grep -c "result=OK" "$LOG_FILE" 2>/dev/null || echo 0)
  echo "[CHK] t=${s}s size=${sz} bytes ok_lines=${ok}"

  # phát hiện file xoay theo 2 kiểu tên
  if [[ -f "$LOG_DIR/vts.log1" || -f "$LOG_DIR/vts.log.1" ]]; then
    inode_now="$(inode_of "$LOG_FILE")"
    if [[ "$inode_now" != "$inode0" ]]; then
      rotations=$((rotations+1))
      echo "[CHK] rotation #$rotations detected: inode $inode0 -> $inode_now"
      inode0="$inode_now"
      [[ $rotations -ge 1 ]] && break
    fi
  fi
  sleep 1
done

wait "$CLIENT_PID" || true
echo "[CHK] files:"
ls -l "$LOG_DIR"/vts.log* || true

# Đánh giá
FAIL=0
[[ $rotations -ge 1 ]] || { echo "[FAIL] No rotation detected."; FAIL=1; }
ok_after=$(grep -c "result=OK" "$LOG_FILE" 2>/dev/null || echo 0)
if [[ "$ok_after" -le "$prev_ok" ]]; then
  echo "[FAIL] Log did not advance after rotation."
  FAIL=1
fi

if [[ $FAIL -eq 0 ]]; then
  echo "[PASS] Log rotation ổn định (đã xoay >=1 lần và tiếp tục ghi sau khi xoay)."
  exit 0
else
  echo "[FAIL] Log rotation chưa ổn. Xem $LOG_FILE để chẩn đoán."
  exit 1
fi
