> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# WP aft — マージ計画 (統括用)

作成 2026-10-17。数値はすべて実測 (出典: `wp_aft/logs/`・本ファイル)。統括が適用する。

## 1. ファイル

| wp_aft 側 | 行き先 | 内容 |
|---|---|---|
| `src/nv_aft.py` | `repro/nv_aft.py` (新規) | `auto_fine_tune`/`auto_fine_tune_fast`/`auto_fine_tune_mode2`。v5 パイプラインは現状これを使わない — **入れるかは統括の判断** (REPORT §10.2) |
| `verify/verify_self.py` | `neatvideo_ofx_614/analysis/noise/aft_verify/verify_self.py` 等 | 自己一致試験 (2026-10-18: 178 PASS、誤り注入 12 種すべて検出)。本家不要 (本家ファイルの定数を読むだけ) |
| `verify/eval_v5_aft.py` | `neatvideo_ofx_614/analysis/blackbox/` (参考) | 区間 A/B の v5 変種測定。統括の再測定用 |
| `notes/AutoFineTune.md` | AGENTS.md §6.5 への追記原料 (下記 §2) | 命令単位解読 |
| `notes/asm/*.s` | `analysis/rtti/noise/` と同じ管理場所が望ましい | 抽出アセンブラ 18 ファイル |
| `harness_request.md` + `nvh_patch.diff` | 統括が審査 | aftshuffle・aftpoly (COMMON §4) |

## 2. AGENTS.md への追記案 (§6.5 AutoFineTune の項)

- 類似度 `FUN_009b6338` の R 語は **語 490/491 (byte 0x7a8/0x7ac)** (asm 0x9b64f4/0x9b655c)。
  許容語: stats[1+i] (L0)・stats[17+i] (L1)・stats[34+8l+m] (CV)・stats[49+l] (mean/q99 比)。
  s_lvl = DAT_012477dc = (3.0, 2.0, 3.0)、s_mult = DAT_012477d0 = (4.0, 3.0, 4.0) (mode 0/1/2)
- 曲線化 `FUN_009b4e08` の位相 2 はフラグ付きのビン 0..8 に分位点を書く (0x9b5238 の `cmp x9,#0x100` は本体の後)。
  位相 3 は**非フラグのビンそれぞれ**に avg×level を書く (フラグ付き = 新規分位点のビンは書き換えない)。
  どちらの位相も、また `FUN_009bdf30` も、書き込むビンの段係数 5 語を 1.0f にする (memset_pattern16 の模様 0xc962e0 = 1.0f×4、
  9b4e08 で 2 か所・9bdf30 で 8 か所)。2026-10-18 訂正: 旧案の「ビン 8 は書かない」「係数 = 0」は誤り
- 類似度の CV 鎖は、許容 0 の直後も含めて「次語 < 0 で終了、NaN は続行」(0x9b65fc の b.pl、他は b.mi)
- `FUN_009bdf30` (0x9be22c–0x9be29c) に難読化様の判定あり (語 image+0x16af38・カウンタ 0x1488404・
  表ポインタ 0x1561000 ページ・バイト +0x1a15)。逆アセンブル範囲では大域 (0x1488404) への
  書き込みのみで、ビン値への影響は観測されていない
- モード 2 のタイル順 `FUN_009d39ec`: LCG 風順列 (s0=0x7b)。外部参照は __TEXT 定数のみ
- worker の受入条件と image2 差分検査の詳細 (REPORT §2 / notes §1)
- 移植 `wp_aft/src/nv_aft.py` は静的解読に基づく再現で**本家との数値照合は未実施**
  (保護判定のため)。区間 A/B の v5 変種 (AFT 曲線で Y 曲線を再形成) は v5 既定と同等
  (Y +0.008 / +0.024 dB、0.05 dB 未満。2026-10-18 の修正後に再測定し、数値は修正前と同じ。変わったのは AFT 曲線のビン 8 だけ)

## 3. exp_results.txt への追記案

```
# wp_aft (2026-10-17): AutoFineTune 0x9a775c の静的移植 (本家照合なし — 保護判定のため実行不可)
# 自己一致 verify/verify_self.py: 178 PASS (2026-10-18 修正後。参照版==高速版、定数・ゲート・段係数の期待値、誤り注入 12 種すべて検出)
# 2026-10-18 修正 (独立検証の反証 2 点 + 軽微 1 点): 9b4e08 位相 2 はビン 0..8 (旧: 0..7)、段係数 5 語は 1.0f (旧: 0)、
#   類似度の CV 鎖で許容 0 直後の NaN は続行 (旧: 終了)
# 実装バグ 2 件を実フレーム測定で発見・修正: model_full の U32/F32、類似度 R 語 474→490 (byte 0x7a8)
# 区間 A/B の v5 変種 (AFT 曲線 R/G/B 平均で Y 曲線を再形成、area_level 錨止め):
#   A: base 47.117/50.985/51.179 → aft 47.125/50.986/51.179 (dB, Y/U/V)
#   B: base 47.222/50.920/51.189 → aft 47.246/50.921/51.189
#   受入 275/1980 (A)・213/1980 (B)、aft 曲線は両区間で同形。修正後の再測定でも PSNR・受入数は同じ、
#   aft 曲線のビン 8 だけが変わった (A 0.7637→0.7436、B 0.7648→0.7466)。
#   ベースラインは記録値 (A 47.12/50.98/51.18、B 47.22/50.92/51.19) を再現。
#   出典: sandbox/omo/wp_aft/logs/eval_v5_aft_20261018.json・verify_self_20261018.log・mut_20261018/ (修正前は eval_v5_aft.json)
```

## 4. ハーネス

`nvh_patch.diff` (aftshuffle N・aftpoly npts ncoef x.. y.. [q..])。統括の審査後、承認されれば
`wp_aft/verify/` に本家との照合スクリプトを追加する (その際の件数を REPORT に追記)。

## 5. 依存の変更有無

なし (repro 側は変更していない。v5 の既定・パラメータは不変)。
