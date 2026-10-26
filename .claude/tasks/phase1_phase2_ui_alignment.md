# タスク: 本家 UI 観測 (Phase 1) + 静的解析・移植 (Phase 2)

## 前提

- Resolve のタイムラインを **1920×1080** に変更済み (ユーザー対応済み)
- Neat Video v6 Pro (ライセンス済み) が Resolve で使用可能
- 再現 UI: `repro/nv_ui_qt/`
- 逆アセンブル: `/tmp/nv_expand/disasm_full.txt` (3,287,476 行、OFX Demo v6.1.4 の arm64 スライス)
- RS バイナリ逆アセンブル: `/tmp/nv6rs_disasm/disasm_full.txt` (3,705,842 行、v6.1.3 RS の arm64 スライス)
- 処理コア (`nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`) は**変更しない**

## Phase 1: 本家の画面を観測する (Resolve を操作)

Resolve を起動して本家の Neat Video UI を開き、以下を観察・記録する。
画面のキャプチャには `screencapture` コマンドまたは cliclick を使う。

### 観察手順

1. Resolve を起動する
2. test_v02.mp4 をメディアプールに読み込む (既に読み込まれている場合はスキップ)
3. クリップに Neat Video v6 を適用してプラグイン UI を開く
4. 以下を順番に操作・撮影する:

| # | 操作 | 記録する内容 |
|---|---|---|
| 1a | Auto Profile (Current Frame) をクリック | Noise Level の数値 (1080p 変更後) |
| 1b | Profile Check をクリック | 表示される内容 (グラフ・マスク・Enhanced) |
| 1c | Profile Check 中にフレームを替える | 表示の変化 |
| 1d | Profile Check 中にマウスを押す | 表示の変化 (Split ビューか) |
| 1e | View メニューをクリック | 項目一覧 |
| 1f | Filter メニューをクリック | 項目一覧 |
| 1g | Tools メニューをクリック | 項目一覧 |
| 1h | ウィンドウを小さくする | パネルのスクロール挙動 |
| 1i | プロファイル作成後の選択範囲 | 枠の色 (赤/黄/緑) |
| 1j | ズームのメニュー | 段 (33.3% など) |

5. スクリーンショットは `sandbox/omo/coord/genuine_ui/shots2/` に保存する

### 観察結果の記録

`sandbox/omo/coord/genuine_ui/OBSERVED_1080P.md` に書く:
- Noise Level の数値 (1080p 変更後)
- 各メニューの項目一覧
- 各ボタンの挙動の変化
- 選択範囲の枠の色

## Phase 2: 静的解析で式を読んで移植する

本家バイナリの該当関数を読んで、以下の計算式を特定し `repro/nv_ui_qt/` に実装する。

### 項目 5: Quality の計算式 (本家 23%)

本家のプロファイル欄に表示される "Overall Profile Quality" の値の計算式を特定する。

手掛かり:
- プロファイル欄に値を書き込む関数を探す (AGENTS.md §6.5 の profile 構造参照)
- 表示側は `QualityLabel` (`Ui::ProfilePageSidePanel` の一部)
- プロファイルの `tile_dev` (FUN_009b2f34) や `curve` の品質から計算すると推定
- `nvh profile` の出力と本家の画面の値で照合できる

### 項目 6: 完了度 (本家 0%)

Manual Fine-Tune のイコライザが測った値の数から完了度を計算する関数を読む。
AFT の移植 (aft_port) と一緒に実装する。

### 項目 7: 帯域の棒の値、イコライザの緑枠・赤枠の点の意味

プロファイルビューアやイコライザの描画関数を読む。表示だけの差。

### 項目 9: 表示のディザ

本家の表示用の乱数表 (16384 個、種 0x9) を再現する。見た目の違いは ±0.25 程度。
乱数表は __TEXT __const 内にある可能性が高い (シード 0x9 で LCG を探す)。

### 項目 10: Assist Image の作り方

本家が Profile Check のときにフィルタへ渡す設定を、呼び出し元からたどって読む。
Profile Check 中に立つ印 (page+0x1e0) からフィルタ設定をたどる。
画面の見た目は近づけられる (画素単位の一致は無理 — FUN_00990f74 は難読化されている)。

### 実装

特定した式を `repro/nv_ui_qt/` に実装する:
- Quality 計算 → `state.py` または新規 `quality.py`
- 完了度 → `profile_panel.py`
- 帯域の棒 → `noise_graph.py`
- ディザ → `viewer.py`
- Assist Image → `viewer.py` または `display.py`

## 報告

- Phase 1 の観察結果: `sandbox/omo/coord/genuine_ui/OBSERVED_1080P.md` + `shots2/`
- Phase 2 の解析結果: `sandbox/omo/coord/genuine_ui/STATIC_PHASE2.md`
- 実装の検証: `sandbox/omo/coord/genuine_ui/VERDICT_PHASE2.md`

## 制限

- 処理コア (`nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`) は**変更しない**
- 本家のアイコン画像・動画は著作物なので使用しない
- ハーネス (`nvh`) を使う場合は `nvh selftest` 終了コード 3 を確認してから
- 保護区画 (§2.2 の範囲) は読まない (ユーザーが統括に一任した範囲のみ)
