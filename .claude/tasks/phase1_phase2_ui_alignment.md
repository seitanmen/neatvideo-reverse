# タスク: 本家 UI 観測 (Phase 1) + 静的解析・移植 (Phase 2)

## 前提

- Resolve のタイムライン: **1920×1080** (変更済み)
- 逆アセンブル: `/tmp/nv_expand/disasm_full.txt` (OFX Demo) + `/tmp/nv6rs_disasm/disasm_full.txt` (RS)
- 処理コア (`nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`) は**変更しない**

## ⚠️ 保護区画の取り扱い

以下のアドレス範囲は**読まないこと** (難読化・トークン表を含む):

```
禁止アドレス (逆アセンブル検索の対象から除外すること):
  0x991000-0x991080   FUN_00990f74 入口の難読化
  0x991500-0x991554   縮小核のハッシュ
  0x9915c0-0x991630   縮小核のトークン読み
  0x9916ec-0x991744   縮小核のハッシュ
  0x991f80-0x991fe8   縮小核末尾のハッシュ
  0x992058 付近       縮小核のハッシュ
  0x99e6f4 の入口     輝度テーブルのトークン
  0x99f528            σ マップのトークン
  0x9a7828-0x9a79f0   AutoFineTune 入口ゲート
  0x9ab4b0-0x9ab5b0   σ 係数のハッシュ
  0x9b1dfc-0x9b1e7c   FindFlatArea のゲート
  0x1488000-0x1489000 保護大域ページ (読み取りは可、書き手の追跡はしない)
  0x1561000-0x1568000 トークン欄ページ (同上)
```

これらの範囲に触れる解析は**統括が実施する** (ClaudeCode は対象外)。

## Phase 1: 本家の画面を観測する (ClaudeCode が実行)

Resolve を起動して本家の Neat Video UI を開き、観察・記録する。

| # | 操作 | 記録する内容 |
|---|---|---|
| 1a | Auto Profile (Current Frame) をクリック | Noise Level の数値 (1080p 変更後) |
| 1b | Profile Check をクリック | 表示される内容 |
| 1c | Profile Check 中にフレームを替える | 表示の変化 |
| 1d | Profile Check 中にマウスを押す | Split ビューか |
| 1e | View メニュー | 項目一覧 |
| 1f | Filter メニュー | 項目一覧 |
| 1g | Tools メニュー | 項目一覧 |
| 1h | ウィンドウを小さくする | パネルのスクロール挙動 |
| 1i | プロファイル作成後の選択範囲の枠の色 | 赤/黄/緑 |
| 1j | ズームのメニュー | 段 |

スクリーンショット: `sandbox/omo/coord/genuine_ui/shots2/`
観察記録: `sandbox/omo/coord/genuine_ui/OBSERVED_1080P.md`

## Phase 2-A: 静的解析・移植 (ClaudeCode が実行 — 保護区画を含まない項目)

### 項目 6: 完了度 (本家 0%)

Manual Fine-Tune のイコライザで測った値の数から完了度を計算する。
UI コード内の単純なカウンタ。探索範囲: 表示側の関数のみ。

### 項目 7: 帯域の棒の値、イコライザの緑枠・赤枠の点の意味

プロファイルビューやイコライザの描画関数 (QPainter による描画)。
表示だけの差。処理ロジックを含まない。

### 項目 9: 表示のディザ

16384 個の定数表 (LCG 乱数、種 0x9)。__TEXT __const 内の静的データ。
逆アセンブルで "movk ..., #0x9" や "movz ..., #0x9" を検索してテーブルを特定する。

### 実装

特定した式を `repro/nv_ui_qt/` に実装する (表示のみなので処理コアは変更しない)。

## Phase 2-B: 静的解析 (統括が実行 — 保護区画に触れる可能性)

### 項目 5: Quality の計算式 (本家 23%)

プロファイルの品質から計算。探索範囲が σ マップ (FUN_009aae90) や
縮小核 (FUN_00990f74) の近くになる可能性があるため統括が実施。

### 項目 10: Assist Image の作り方

Profile Check 中にフィルタへ渡す設定をたどる必要がある。
フィルタパイプライン = FUN_00990f74 を含むため統括が実施。

## 報告

- Phase 1: `sandbox/omo/coord/genuine_ui/OBSERVED_1080P.md` + `shots2/`
- Phase 2-A: `sandbox/omo/coord/genuine_ui/STATIC_CLAUDE.md`
- Phase 2-B: `sandbox/omo/coord/genuine_ui/STATIC_LEAD.md` (統括)
