#!/bin/bash
# Run tests for all modules in modules/ directory
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODULES_DIR="$SCRIPT_DIR/modules"

count=0
ok=0
diff=0
skip=0

for f in "$MODULES_DIR"/*.a2m "$MODULES_DIR"/*.A2M "$MODULES_DIR"/*.a2t "$MODULES_DIR"/*.A2T; do
  [ -f "$f" ] || continue
  echo ""
  "$SCRIPT_DIR/run_one_test.sh" "$f"
  result=$?
  count=$((count + 1))
  case $result in
    0) ok=$((ok + 1)) ;;
    1) diff=$((diff + 1)) ;;
    *) skip=$((skip + 1)) ;;
  esac
done

echo ""
echo "=== Summary ==="
echo "Total: $count | OK: $ok | DIFF: $diff | Skip: $skip"
