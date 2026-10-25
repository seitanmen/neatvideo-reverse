# 動的解析検証 (ライセンス一時有効化)

## 目的
ライセンス成立時にトークン表欄・状態語・保証語がどう変わるかを実測し、
静的解析 (`notes/lead_static_20261021.md`) の結論を検証する。

## 手順

### 0. .env にライセンス情報を記入
```
cp .env.example .env
# .env を編集して NV_LICENSE_NAME / NV_LICENSE_CODE を記入
```

### 1. ライセンスを有効化
```
./activate.sh
```
スクリプトが既存の plist をスナップショットした後、ホストアプリで登録するよう案内する。
ホストアプリ (Resolve 等) で Neat Video の RegistrationDialog を開き、.env の値を入力して登録する。
登録が済んだらターミナルに戻って Enter。

### 2. プローブ実行
```
./probe_run.sh both
```
無効化状態と有効化状態の両方でプローブを実行し、差分を出す。

### 3. ライセンスを無効化 (元の状態に戻す)
```
./deactivate.sh
```
スナップショットから plist を復元 (または初回登録だった場合は plist を削除)。

## ファイル

| ファイル | 用途 |
|---|---|
| .env | ライセンス情報 (コミットしない) |
| .env.example | .env のテンプレート |
| activate.sh | plist スナップショット + 登録案内 |
| deactivate.sh | plist 復元 (スナップショットから) |
| probe_run.sh | nvh probe を実行して差分 |
| results/ | プローブ出力の保存先 |

## 検証ポイント (nvh probe の出力)

| 項目 | 無効化時 (予測) | 有効化時 (観測したい値) |
|---|---|---|
| 0x1566B80 (AFT entry table) | NULL (0) | 非 NULL (表の先頭アドレス) |
| AFT entry condition | ≠ 0 (body SKIPPED) | **0** (body RUNS) |
| 0x1566BC0 (shrink w19 table) | NULL | 非 NULL |
| shrink table[0x3997] | (読めない) | バイト値 |
| 0x1561DEC (& 0x3fffff) | ≠ 1 (gate FAIL) | **1** (gate OK)? |
| 0x1561DF0 (16bit gate) | ≠ 1 (gate FAIL) | **1** (gate OK)? |
| 0x1561D58 (kernel shift) | 0 | 0 or 1 (チャンク実行後に変わるか) |
| 0x1489E68 popcount | 16 (subtrahend 0) | 変わるか? |
| AFT body condition | FAIL (table NULL) | **PASS?** |
