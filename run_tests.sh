#!/bin/bash
# Testing script for a2t_play: compare a2m_dump (C) vs adt2_dump (Pascal reference)
set -e

CC=${CC:-/c/Users/user/msys64/ucrt64/bin/x86_64-w64-mingw32-gcc}
GMAKE=${GMAKE:-/c/Users/user/msys64/usr/bin/make}
MODULES_DIR="modules"
REFS_DIR="adt2play_sdl"
TMPDIR="${TMPDIR:-/tmp}"

# Step 1: Build a2m_dump
echo "=== Building a2m_dump ==="
TMP=$TMPDIR $GMAKE -f Makefile a2m_dump CC="$CC" 2>&1

# Step 2+3: Run a2m_dump and adt2_dump on all modules/*.a2m
echo "=== Running dump tools ==="
mkdir -p test

for f in "$MODULES_DIR"/*.a2m "$MODULES_DIR"/*.A2M "$MODULES_DIR"/*.a2t "$MODULES_DIR"/*.A2T; do
  [ -f "$f" ] || continue
  base=$(basename "$f")
  base_noext="${base%.*}"

  echo "--- $base ---"

  # a2m_dump (C version) - skip if reg exists and newer than the module
  if [ ! -f "test/${base_noext}.c.reg" ] || [ "$f" -nt "test/${base_noext}.c.reg" ]; then
    TMP=$TMPDIR timeout 30 ./a2m_dump "$f" >/dev/null 2>&1 && {
      [ -f "${base_noext}.reg" ] && mv "${base_noext}.reg" "test/${base_noext}.c.reg"
    } || echo "  a2m_dump: timeout/fail"
  fi

  # adt2_dump (Pascal reference)
  if [ ! -f "test/${base_noext}.ref.reg" ] || [ "$f" -nt "test/${base_noext}.ref.reg" ]; then
    TMP=$TMPDIR timeout 30 "$REFS_DIR/adt2_dump" "$f" 2>/dev/null && {
      [ -f "${f}.reg" ] && mv "${f}.reg" "test/${base_noext}.ref.reg"
    } || echo "  adt2_dump: timeout/fail"
  fi
done

# Step 4: Compare with diff
echo "=== Comparing outputs ==="
for f in test/*.c.reg; do
  [ -f "$f" ] || continue
  base=$(basename "$f" .c.reg)
  ref="test/${base}.ref.reg"
  if [ -f "$ref" ]; then
    diff_lines=$(diff <(sort "$f") <(sort "$ref") 2>&1 | wc -l)
    if [ "$diff_lines" -eq 0 ]; then
      echo "OK: $base"
    else
      echo "DIFF: $base ($diff_lines lines differ)"
    fi
  else
    echo "SKIP: $base (no reference)"
  fi
done

echo "=== Done ==="
