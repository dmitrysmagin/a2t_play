#!/bin/bash
# Run tests for all modules in modules/ directory
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODULES_DIR="$SCRIPT_DIR/modules"

count=0
ok=0
diff=0
skip=0

for f in $(find "$SCRIPT_DIR/modules" -type f -iname '*.a2m' 2>/dev/null | sort); do
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
