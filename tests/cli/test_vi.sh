#!/usr/bin/env bash
set -euo pipefail
BIN=./build/bin/system-monitor

# ensure VTS is running
pgrep -x vts >/dev/null || ( ./build/bin/vts & sleep 0.2 )

pass() { echo "[PASS] $1"; }
fail() { echo "[FAIL] $1"; exit 1; }
# ... phần còn lại giữ nguyên



$BIN help >/dev/null && pass help || fail help


# Bad args
if $BIN create >/dev/null 2>&1; then fail badargs; else pass badargs; fi


# Show non-existing should return JSON with error code
OUT=$($BIN show room NOPE || true)
[[ "$OUT" == *"\"code\":1002"* ]] && pass notfound || fail notfound


# Basic flow (server must map to idempotent ops)
$BIN create room cpu_room | grep '"code":0' && pass create || fail create
$BIN start room cpu_room | grep '"code":0' && pass start || fail start
$BIN show room cpu_room | grep '"code":0' && pass show || fail show
$BIN edit room cpu_room --set threshold 80 | grep '"code":0' && pass edit || fail edit
$BIN stop room cpu_room | grep '"code":0' && pass stop || fail stop
$BIN delete room cpu_room | grep '"code":0' && pass delete || fail delete