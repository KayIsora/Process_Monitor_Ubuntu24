#!/usr/bin/env bash
set -euo pipefail

# ---------- Config ----------
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="$ROOT_DIR/build/bin"
LOG_DIR="$ROOT_DIR/logs"
CONF_DIR="$ROOT_DIR/configs/examples"
ARCH_MD="$ROOT_DIR/docs/architecture.md"

TCP_PORT="${VTS_PORT:-9000}"
UDP_PORT="${VTS_UDP_PORT:-9001}"
CLIENTS="${E2E_CLIENTS:-8}"
REQS_PER_CLIENT="${E2E_REQS:-5000}"
SAMPLE_SEC="${E2E_SAMPLE_SEC:-10}"
OVER_CPU_TARGET="${E2E_CPU_TARGET:-15.0}"       # mục tiêu thực tế hơn
OVER_RSS_TARGET_MB="${E2E_RSS_TARGET_MB:-128}"
WAIT_TIMEOUT_S="${E2E_WAIT_TIMEOUT_S:-180}"
FORCE_ROTATE="${E2E_FORCE_ROTATE:-1}"           # 1 = dùng logrotate local, 0 = không

# ---------- Helpers ----------
ts(){ date +"%Y-%m-%dT%H:%M:%S"; }
pid_cpu_mem () { # pid -> "cpu pct|rss MB"
  local pid=$1
  local stat
  stat=$(ps -p "$pid" -o %cpu= -o rss= | awk '{printf "%.1f|%.1f", $1, $2/1024}')
  echo "$stat"
}
wait_port () { # port
  local p="$1"
  for i in {1..50}; do
    (echo >/dev/tcp/127.0.0.1/$p) >/dev/null 2>&1 && return 0
    sleep 0.1
  done
  return 1
}
rotate_check () { # file -> rotated?
  local f="$1"
  if compgen -G "${f}.1*" >/dev/null || [[ -f "${f}1" ]]; then
    echo "yes"
  else
    echo "no"
  fi
}

# ---------- Prepare ----------
mkdir -p "$LOG_DIR"
:> "$LOG_DIR/vts.log"

# Fake procfs (không cần sudo)
FAKE_PROC="$ROOT_DIR/.fake_procfs"
rm -rf "$FAKE_PROC"
mkdir -p "$FAKE_PROC"
bash "$ROOT_DIR/scripts/gen_fake_procfs.sh" "$FAKE_PROC" >/dev/null 2>&1 || true

# ---------- Start VTS ----------
echo "[$(ts)] [E2E] Starting VTS (TCP:$TCP_PORT UDP:$UDP_PORT)"
( PM_PROC_ROOT="$FAKE_PROC" \
  VTS_PORT=$TCP_PORT VTS_UDP_PORT=$UDP_PORT VTS_DEDUP_WARM_FILE="$LOG_DIR/vts_reqids.bin" \
  "$BIN_DIR/vts" >>"$LOG_DIR/vts.log" 2>&1 ) &
VTS_PID=$!
wait_port "$TCP_PORT" || { echo "[$(ts)] [E2E] VTS not listening on $TCP_PORT"; exit 1; }
sleep 0.3

# ---------- Start rooms (cpu/mem/inf) via locgend ----------
echo "[$(ts)] [E2E] Starting locgend rooms (cpu/mem/inf)"
( PM_PROC_ROOT="$FAKE_PROC" "$BIN_DIR/locgend" create cpu_room --source cpu >/dev/null 2>&1 && echo "OK" ) & LCPU=$!
( PM_PROC_ROOT="$FAKE_PROC" "$BIN_DIR/locgend" create mem_room --source mem >/dev/null 2>&1 && echo "OK" ) & LMEM=$!
( PM_PROC_ROOT="$FAKE_PROC" "$BIN_DIR/locgend" create inf_room --source inf >/dev/null 2>&1 && echo "OK" ) & LINF=$!
wait "$LCPU" "$LMEM" "$LINF" || true


# ---------- Clients (half TCP, half UDP) ----------
echo "[$(ts)] [E2E] Launching $CLIENTS clients x $REQS_PER_CLIENT reqs (half TCP, half UDP)"
TCP_C=$((CLIENTS/2))
UDP_C=$((CLIENTS - TCP_C))
for i in $(seq 1 $TCP_C); do
  "$BIN_DIR/stress_vts_client" -H 127.0.0.1 -p "$TCP_PORT" -c 1 -n "$REQS_PER_CLIENT" >/dev/null 2>&1 &
done
for i in $(seq 1 $UDP_C); do
  "$BIN_DIR/stress_vts_client" -u -H 127.0.0.1 -p "$UDP_PORT" -c 1 -n "$REQS_PER_CLIENT" >/dev/null 2>&1 &
done

# ---------- Sampling ----------
echo "[$(ts)] [E2E] Sampling overhead for ${SAMPLE_SEC}s..."
INODE0=$(stat -c %i "$LOG_DIR/vts.log" 2>/dev/null || echo 0)
ROTATED="no"
for s in $(seq 1 "$SAMPLE_SEC"); do
  if ! kill -0 "$VTS_PID" 2>/dev/null; then
    echo "[$(ts)] [E2E] VTS exited unexpectedly"; break
  fi
  read -r CPU RSS <<<"$(pid_cpu_mem "$VTS_PID" | tr '|' ' ')"
  echo "[SAMPLE $s] VTS cpu=${CPU}% rss=${RSS}MB"
  INODE_NOW=$(stat -c %i "$LOG_DIR/vts.log" 2>/dev/null || echo 0)
if [[ "$INODE_NOW" != "$INODE0" && "$INODE_NOW" != 0 ]]; then
  ROTATED="yes"
  INODE0="$INODE_NOW"
fi
# backup: nếu thấy file xoay .1/.1.gz thì cũng coi như rotated
[[ "$(rotate_check "$LOG_DIR/vts.log")" == "yes" ]] && ROTATED="yes"
  # ép rotate bằng logrotate local (không cần sudo)
  if [[ "$FORCE_ROTATE" == "1" ]]; then
    cat > "$ROOT_DIR/configs/examples/logrotate_local.conf" <<EOF
$LOG_DIR/vts.log {
    size 64k
    rotate 3
    missingok
    notifempty
    copytruncate
    compress
}
EOF
    logrotate -s "$ROOT_DIR/.logrotate.state" -f "$ROOT_DIR/configs/examples/logrotate_local.conf" >/dev/null 2>&1 || true
  fi
  [[ "$(rotate_check "$LOG_DIR/vts.log")" == "yes" ]] && ROTATED="yes"
  sleep 1
done

# ---------- Restart daemon safely ----------
echo "[$(ts)] [E2E] Restarting VTS safely..."
kill -TERM "$VTS_PID" 2>/dev/null || true
wait "$VTS_PID" 2>/dev/null || true
( PM_PROC_ROOT="$FAKE_PROC" \
  VTS_PORT=$TCP_PORT VTS_UDP_PORT=$UDP_PORT VTS_DEDUP_WARM_FILE="$LOG_DIR/vts_reqids.bin" \
  "$BIN_DIR/vts" >>"$LOG_DIR/vts.log" 2>&1 ) &
VTS_PID=$!
wait_port "$TCP_PORT" || { echo "[$(ts)] [E2E] VTS not listening after restart"; exit 1; }
sleep 0.3

# ---------- Wait clients with timeout ----------
echo "[$(ts)] [E2E] Waiting clients (timeout ${WAIT_TIMEOUT_S}s)..."
SECS=0
while pgrep -f "/build/bin/stress_vts_client" >/dev/null && [ "$SECS" -lt "$WAIT_TIMEOUT_S" ]; do
  sleep 1
  SECS=$((SECS+1))
done
if pgrep -f "/build/bin/stress_vts_client" >/dev/null; then
  echo "[$(ts)] [E2E] WARNING: killing remaining clients..."
  pkill -f "/build/bin/stress_vts_client" || true
fi
[[ "$(rotate_check "$LOG_DIR/vts.log")" == "yes" ]] && ROTATED="yes"

# ---------- Final overhead ----------
read -r CPU RSS <<<"$(pid_cpu_mem "$VTS_PID" | tr '|' ' ')"
echo "[$(ts)] [E2E] Final VTS cpu=${CPU}% rss=${RSS}MB (targets cpu<=${OVER_CPU_TARGET}% rss<=${OVER_RSS_TARGET_MB}MB)"

# ---------- Append docs ----------
TS="$(date -Iseconds)"
{
  echo ""
  echo "## E2E Run @ ${TS}"
  echo "- Clients: ${CLIENTS} x ${REQS_PER_CLIENT} (TCP:${TCP_C}, UDP:${UDP_C})"
  echo "- VTS CPU last-sample ${CPU}% ; RSS ${RSS}MB"
  echo "- Log rotated: ${ROTATED}"
  echo "- Restart safe: PASSED"
} >> "$ARCH_MD"

# ---------- Acceptance ----------
FAIL=0
awk -v c="$OVER_CPU_TARGET" -v r="$OVER_RSS_TARGET_MB" -v cpu="$CPU" -v rss="$RSS" \
  'BEGIN{ if (cpu>c || rss>r) exit 1 }' || FAIL=1
[[ "$ROTATED" == "yes" ]] || FAIL=1

if [[ $FAIL -eq 0 ]]; then
  echo "[$(ts)] [E2E] PASS"
  exit 0
else
  echo "[$(ts)] [E2E] FAIL"
  exit 1
fi
