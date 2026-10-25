#!/bin/zsh
# 動的解析プローブ (ライセンス有効化状態と無効化状態の両方で実行して比較する)
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
NVH="/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/analysis/harness/nvh"
OUT="$DIR/results"

mkdir -p "$OUT"
MODE="${1:-both}"

if [ "$MODE" = "unlicensed" ] || [ "$MODE" = "both" ]; then
  echo "== unlicensed probe =="
  "$NVH" selftest >/dev/null 2>&1
  echo "  selftest exit=$? (3 = OK)"
  "$NVH" probe 2>/dev/null | tee "$OUT/probe_unlicensed.txt"
fi

if [ "$MODE" = "licensed" ] || [ "$MODE" = "both" ]; then
  echo ""
  echo "== licensed probe =="
  echo "  (activate.sh を先に実行してホストで登録しておくこと)"
  "$NVH" selftest >/dev/null 2>&1
  echo "  selftest exit=$? (3 = OK)"
  "$NVH" probe 2>/dev/null | tee "$OUT/probe_licensed.txt"
fi

if [ "$MODE" = "both" ] && [ -f "$OUT/probe_unlicensed.txt" ] && [ -f "$OUT/probe_licensed.txt" ]; then
  echo ""
  echo "== diff (unlicensed vs licensed) =="
  diff "$OUT/probe_unlicensed.txt" "$OUT/probe_licensed.txt" || true
fi
