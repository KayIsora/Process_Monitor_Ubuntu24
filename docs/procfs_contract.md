# /Memdata snapshot layout (fake procfs cho Pha 1a)

/Memdata/
  cpu/
    status      # text: "enabled=0|1 interval_ms=XYZ last_ts=unix_ms"
    data        # text: "cpu.usage=NNN\n"
  mem/
    status      # text: "enabled=0|1 ..."
    data        # text: "mem.free=NNN\n"
  inf/
    status      # optional, tương tự
    data        # optional

Semantics:
- status phản ánh ngay sau khi "config=1" (đối với fake là tức thời).
- data là snapshot chỉ-đọc.

# /proc/pm/{cpu|mem|inf}/{config,status,data} (định hướng)
- config: write "1 [interval_ms]" | "0"
- status: "enabled=0|1 interval_ms=XYZ last_ts=..."
- data: multi-line snapshots per-CPU, non-blocking; đọc dùng seqlock.

Final layout:
  /proc/pm/{cpu|mem|inf}/{config,status,data}

config:
  "1 [interval_ms]\n"  -> enable sampling with interval (default impl-specific)
  "0\n"                 -> disable
status (KV lines):
  enabled=0|1
  interval_ms=<uint>
  drops=<uint64>        # from driver if applicable
  ts_ns=<uint64>
data (KV/records, non-blocking):
  ts_ns=<uint64> cpu.usage=<float> ...
Semantics: Non-blocking read with seqlock + per-CPU buffers.
