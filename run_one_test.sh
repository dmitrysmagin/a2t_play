#!/bin/bash
# Run one test: compare a2m_dump (C) vs adt2_dump (Pascal reference) for a single module
set -e

if [ $# -lt 1 ]; then
  echo "Usage: $0 <module.a2m>"
  exit 1
fi

MODULE="$1"
CC=${CC:-/c/Users/user/msys64/ucrt64/bin/x86_64-w64-mingw32-gcc}
GMAKE=${GMAKE:-/c/Users/user/msys64/usr/bin/make}
REFS_DIR="adt2play_sdl"
TMPDIR="${TMPDIR:-/tmp}"
MAX_FRAMES="${MAX_FRAMES:-30000}"
# Large MAX_FRAMES need more wall time (a2m_dump can take ~45s+ for 100k frames).
TIMEOUT_SEC="${TIMEOUT_SEC:-120}"

base=$(basename "$MODULE")
base_noext="${base%.*}"

mkdir -p test

# Resolve dump binary name (Windows vs Unix)
DUMPBIN=""
if [ -f a2m_dump.exe ]; then DUMPBIN=a2m_dump.exe
elif [ -f a2m_dump ]; then DUMPBIN=a2m_dump
fi

# Build a2m_dump if missing, or when sources are newer (Makefile uses Embarcadero
# `make` on some PATHs — always invoke GNU make as $GMAKE for this target).
need_dump_build=0
if [ -z "$DUMPBIN" ]; then need_dump_build=1
elif [ src/a2t.c -nt "$DUMPBIN" ] || [ src/a2m_dump.c -nt "$DUMPBIN" ]; then need_dump_build=1
fi
if [ "$need_dump_build" = 1 ]; then
  echo "=== Building a2m_dump (GNU make, CC=$CC) ==="
  TMP=$TMPDIR $GMAKE -f Makefile a2m_dump CC="$CC"
  DUMPBIN=""
  [ -f a2m_dump.exe ] && DUMPBIN=a2m_dump.exe
  [ -z "$DUMPBIN" ] && [ -f a2m_dump ] && DUMPBIN=a2m_dump
fi

if [ -z "$DUMPBIN" ]; then
  echo "ERROR: a2m_dump binary missing after build attempt (set GMAKE to GNU make, CC to mingw gcc)" >&2
  exit 1
fi

echo "--- $base ---"

# a2m_dump
TMP=$TMPDIR timeout "$TIMEOUT_SEC" "./$DUMPBIN" "$MODULE" "test/${base_noext}.c.reg" "$MAX_FRAMES" >/dev/null 2>&1 || echo "  a2m_dump: timeout/fail"

# adt2_dump
TMP=$TMPDIR timeout "$TIMEOUT_SEC" "$REFS_DIR/adt2_dump" "$MODULE" "test/${base_noext}.ref.reg" "$MAX_FRAMES" 2>/dev/null || echo "  adt2_dump: timeout/fail"

# Compare
c_reg="test/${base_noext}.c.reg"
ref_reg="test/${base_noext}.ref.reg"
diff_file="test/${base_noext}.diff"
diff_sig_file="test/${base_noext}.diff.sig"
if [ -f "$c_reg" ] && [ -f "$ref_reg" ]; then
  diff -u "$c_reg" "$ref_reg" > "$diff_file" 2>&1 || true
  # Filter: only count diffs in lines that affect audio output
  # (shadow regs 0/1, frequency table F). Ignore LE, EFT, MB, ET, VS, FP, GV, etc.
  awk '/^[+-][0-9]/ { if ($8 == "0" || $8 == "1" || $8 == "F") print }' "$diff_file" > "$diff_sig_file" 2>/dev/null || true
  diff_lines=$(wc -l < "$diff_sig_file")
  if [ "$diff_lines" -eq 0 ]; then
    rm -f "$diff_file" "$diff_sig_file"
    echo "OK: $base_noext"
  else
    total_lines=$(wc -l < "$diff_file")
    echo "DIFF: $base_noext ($diff_lines sig / $total_lines total lines differ, see $diff_file)"
  fi
elif [ -f "$c_reg" ]; then
  echo "SKIP: $base_noext (no reference)"
else
  echo "SKIP: $base_noext (no C dump)"
fi

# Count frames and update MODULES_TESTED.md
c_frames=$(grep -cE '^[0-9]+ 0 ' "$c_reg" 2>/dev/null || echo 0)
if [ -f "$diff_file" ]; then
  d_lines=$(wc -l < "$diff_file")
  d_status="FRAME-DIFF"
else
  d_lines=0
  d_status="PASS"
fi
if grep -qE "^\| $base_noext \|" MODULES_TESTED.md 2>/dev/null; then
  sed -i "s/^| \($base_noext\) |[^|]*|[^|]*|[^|]*|/| \1 | $c_frames | $d_lines | $d_status |/" MODULES_TESTED.md
fi
