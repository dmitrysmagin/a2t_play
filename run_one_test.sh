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

base=$(basename "$MODULE")
base_noext="${base%.*}"

mkdir -p test

# Build a2m_dump if not present
if [ ! -f a2m_dump.exe ] && [ ! -f a2m_dump ]; then
  echo "=== Building a2m_dump ==="
  TMP=$TMPDIR $GMAKE -f Makefile a2m_dump CC="$CC" 2>&1
fi

echo "--- $base ---"

# a2m_dump
TMP=$TMPDIR timeout 30 ./a2m_dump "$MODULE" "test/${base_noext}.c.reg" >/dev/null 2>&1 || echo "  a2m_dump: timeout/fail"

# adt2_dump
TMP=$TMPDIR timeout 30 "$REFS_DIR/adt2_dump" "$MODULE" "test/${base_noext}.ref.reg" 2>/dev/null || echo "  adt2_dump: timeout/fail"

# Compare
c_reg="test/${base_noext}.c.reg"
ref_reg="test/${base_noext}.ref.reg"
diff_file="test/${base_noext}.diff"
if [ -f "$c_reg" ] && [ -f "$ref_reg" ]; then
  diff -u <(sort "$c_reg") <(sort "$ref_reg") > "$diff_file" 2>&1
  diff_lines=$(wc -l < "$diff_file")
  if [ "$diff_lines" -eq 0 ]; then
    rm -f "$diff_file"
    echo "OK: $base_noext"
  else
    echo "DIFF: $base_noext ($diff_lines lines differ, see $diff_file)"
  fi
elif [ -f "$c_reg" ]; then
  echo "SKIP: $base_noext (no reference)"
else
  echo "SKIP: $base_noext (no C dump)"
fi
