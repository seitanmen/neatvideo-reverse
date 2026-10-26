# タスク: Quality 計算式の特定 + Assist Image の本家整合 (統括の解析結果をもとに ClaudeCode が実装)

## 背景

統括が保護区画を含めて解析した結果:

### 項目 5 (Quality) — 統括の解析結果

**発見したデータ**:
- プロファイルの 0xBC00〜0xBC44 に -1 と正の float 値が混在する領域がある
  (帯域ごとの品質メトリックとみられる: 1.45, 1.50, 2.55, 2.21, 1.60, 1.67)
- tile_dev (0xBC44, 51 語) = タイルと全体モデルの RMS 偏差 (26/51 が非ゼロ)
- reference_floor (data/ref_floor.npy, 51 語) = 白色ノイズの基準偏差
- mc_stats (0xBEA8, 32 語) = 2 組 × 16 語のノイズレベル表

**試して一致しなかった式**:
- tile_dev/ref_floor の比 → 品質 = -5% (負)
- RMS 比 → 品質 = -5%
- 非ゼロ tile_dev の割合 → 51%
- curve の平坦度 → AFT 前は計算不可
- S/N 比 → 2.9-3.1 (パーセントにならない)

**結論**: 単純な tile_dev 統計からは 23% は導けない。以下の可能性:
1. tile_dev の前の領域 (0xBC00〜0xBC44) の品質メトリックを使用
2. 複数の指標を組み合わせた複合式
3. プロファイル以外の要因 (選択範囲の面積比等) が関与

**ClaudeCode に任せること**:
- RS バイナリで "Quality" 文字列 (0x14a8aea) を参照する関数を特定
- その関数の呼び出し元をたどって計算式を読む
- 保護区画に触れたら即座に停止して報告する

### 項目 10 (Assist Image) — 統括が実装済み → ClaudeCode が検証

統括が以下を実装した:
- `nv_aft_v5.assist_params(P, mode)`: 空間のみ・全帯域 100% のパラメータを返す
- `main_window.noise_level_check(on)`: Begin で assist_params を使ってプレビューを更新
- `main_window.request_preview()`: _nlc_params があればそちらを使う

**ClaudeCode に任せること**:
- 上記の実装がビット一致することを確認 (通常モードと Assist モードの出力の差を測定)
- 本家 Resolve で Noise Level Check を実行して見た目を比較
- 差があれば修正

## ClaudeCode への指示

### タスク A: Quality 計算式の特定

1. RS バイナリ (`/tmp/nv6rs_disasm/disasm_full.txt`) で以下を検索:
   - "Quality" 文字列 (0x14a8aea) を参照する adrp+add 命令
   - その参照を含む関数の名前・アドレスを特定
2. 該当関数の逆アセンブルを読んで計算式を特定する
3. ただし以下のアドレス範囲は**読まないこと**:
   - 0x991000-0x991080, 0x991500-0x991744, 0x991f80-0x991fe8
   - 0x992058 付近, 0x99e6f4 の入口, 0x99f528
   - 0x9a7828-0x9a79f0, 0x9ab4b0-0x9ab5b0, 0x9b1dfc-0x9b1e7c
4. 計算式を特定できたら `repro/nv_ui_qt/quality.py` に実装する
5. 特定できない場合は、その理由を報告する

### タスク B: Assist Image の検証

1. 通常モードと Assist モードの出力差を測定する:

from nv_aft_v5 import apply_aft, assist_params
prof = build_profile_v5(...)
prof = apply_aft(prof, rgb)
P_normal = v5_params()
P_assist = assist_params(P_normal, "profile_check")
# temporal_v5 + spatial_v5 を両方で実行して差を測定

2. Resolve で本家の Profile Check / Noise Level Check を実行して見た目を比較する
3. 差があれば修正する

### タスク C: 報告

結果を `sandbox/omo/coord/verify_quality/REPORT.md` に書く:
- Quality 計算式の特定結果 (成功/失敗と理由)
- Assist Image の検証結果
- 処理コアは変更しないこと
