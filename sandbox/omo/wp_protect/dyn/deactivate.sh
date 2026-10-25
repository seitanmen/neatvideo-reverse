#!/bin/zsh
# ライセンスを無効化し、元の状態に戻す
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
PLIST="$HOME/Library/Application Support/ABSoft/com.absoft.NeatVideo6-OFX.plist"
SNAP="$(cat "$DIR/.plist_snapshot_path" 2>/dev/null || echo "")"

echo "== ライセンス無効化 =="
if [ -z "$SNAP" ]; then
  echo "  ERROR: スナップショットが見つからない (.plist_snapshot_path がない)"
  exit 1
fi

if [ "$(cat "$SNAP")" = "登録前" ]; then
  # 初回登録だった場合: plist と ABSoft ディレクトリを消す
  if [ -f "$PLIST" ]; then
    rm "$PLIST"
    echo "  plist を削除 (初回登録前の状態に戻す)"
  fi
  rmdir "$HOME/Library/Application Support/ABSoft" 2>/dev/null || true
else
  cp "$SNAP" "$PLIST"
  echo "  plist をスナップショットから復元: $SNAP"
fi
rm -f "$DIR/.plist_snapshot_path"
echo "  完了"
