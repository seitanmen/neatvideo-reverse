#!/bin/zsh
# ライセンスを一時的に有効化する ( plist のスナップショット → ユーザーがホストで登録 → 待ち )
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
PLIST="$HOME/Library/Application Support/ABSoft/com.absoft.NeatVideo6-OFX.plist"
SNAP="$DIR/plist_snapshot_$(date +%Y%m%d_%H%M%S)"

source "$DIR/.env" 2>/dev/null || { echo "ERROR: .env not found"; exit 1; }
if [ -z "${NV_LICENSE_CODE:-}" ]; then
  echo "ERROR: .env に NV_LICENSE_CODE が未記入です。記入してから再実行してください。"
  echo "  .env の場所: $DIR/.env"
  exit 1
fi

echo "== ライセンス一時有効化 =="
echo "  Name: $NV_LICENSE_NAME"
echo "  Type: $NV_LICENSE_TYPE"
echo "  Code: (非表示, ${#NV_LICENSE_CODE} 文字)"

# 既存の plist をスナップショット (ない場合は「無い」ことを記録)
if [ -f "$PLIST" ]; then
  cp "$PLIST" "$SNAP"
  echo "  既存 plist をスナップショット: $SNAP"
else
  echo "登録前" > "$SNAP"
  echo "  既存 plist は存在しない (初回登録)"
fi
echo "$SNAP" > "$DIR/.plist_snapshot_path"

cat << 'MSG'

=== 次のステップ (手動) ===
1. Neat Video がインストールされているホストアプリを開く
   (OFX: DaVinci Resolve / FCP / Vegas など。未インストールの場合は下記参照)
2. Neat Video を適用して RegistrationDialog を開く
3. .env の Name / Organization / Number of copies / Registration code を入力して OK
4. ホストを終了せずに、このターミナルに戻って Enter を押す

=== ホスト未インストールの場合 ===
OFX プラグインは bundle を
  /Library/OFX/Plugins/  (システム全体)
  または Resolve の OFX フォルダ
に置くとホストが認識する。bundle をコピーしてホストを起動すれば RegistrationDialog が出る。
詳細は activate_host.md を参照。

MSG

read "?登録が終わったら Enter を押してください..."

if [ -f "$PLIST" ]; then
  echo "  plist 確認: $(ls -la "$PLIST" | awk '{print $5}') bytes"
  echo "  登録キーの確認:"
  plutil -p "$PLIST" 2>/dev/null | grep -viE "code|name|key" | head -10
  echo "  (code/name のキーはログに出さない)"
else
  echo "  WARNING: plist がまだ存在しない。登録が完了していない可能性がある。"
fi
