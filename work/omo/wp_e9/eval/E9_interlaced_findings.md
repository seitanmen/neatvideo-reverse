# E9 / INT-4 インターレース: 本家と当方 (フレーム処理・フィールド処理) の比較 (2026-11-02)

作成: e9-prep。観測と推測は分けて書く (推測には「推測」と明記)。処理コアの Python・clips/・genuine*/・ini は変えていない。

## 条件
- 素材: 1080i (TFF)。old_town_cross tff_y6、park_joy tff_y6、sintel tff_y3、sintel tff_y6 (各 60 フレーム = 120 フィールド)。
  正解は、組み合わせる前のフレーム (`clips/<src>/clean_tff.y4m`、フィールドごとは `clean_tff_fields.y4m`)。
- 本家 (統括が処理):
  - Resolve のインターレース処理は無効。Neat の Video Scan Type = Interlaced、Profiled with: Two fields (1920x540x2)、Auto Profile、既定のフィルタ。
  - 出力は `genuine/interlaced_noRI/Event_バージョン 1_0001_000N/` (N=1..4 の順に上の 4 組)。
  - Resolve のインターレース処理を有効にしたときの失敗した出力 (`genuine_failed/`) は使っていない。
- 当方フレーム: これまでの C++ 版 (プログレッシブとして処理、`runs/ours_frame/`)。
- 当方フィールド (INT-3):
  - 使ったもの: `.claude/worktrees/int2` の native/・qtui/ を scratch に写してビルドしたもの (worktree は無変更)。
    ドライバは `tools/cpp_int3/` (e9_profile_fields + 写しの export_cli)、実行は `tools/run_ours_fields.py`。
  - 走査方式: パラメータの `P["interlace"] = {first 0, mode, radius (フィールド数)}` で TFF を明示した
    (MainWindow::proc_params と同じ。y4m のフラグは使わない)。
  - プロファイル: build_profile_fields (Two fields、frame 0、最初のフィールド = top)。AFT はプロファイルのフィールド画像に掛けた
    (UI の auto_fine_tune と同じ)。
  - 4 通りの処理 (すべて同じプロファイル):
    1. same/frames: 既定。Temporal Radius 2 → 4 フィールド、同じパリティだけ。`runs/ours_field/`
    2. same/fields: R2 → 2 フィールド (本家式の数え方)。`runs/ours_field_same_fields/`
    3. shift/frames: 両パリティ、半ライン補正。`runs/ours_field_shift_frames/`
    4. plain/frames: 両パリティ、補正なし。`runs/ours_field_plain_frames/`
  - 書き出しは `--crf 0`。x264 に `+ilme+ildct tff=1` が付くが、同じ符号化コマンドで元に戻ることを cmp で確認済み (可逆)。
- プロファイル (当方フィールド): old [72,1216,128] auto、park [368,1304,128] spatial、sintel y3 [88,1824,96] auto (flat_status 2)、
  sintel y6 [96,1448,128] auto (領域はフィールドの行で表す)。
- フレームのずれ: 全出力でずれ 0、縦ずれ 0 (フィールドの入れ替わりなし)。

## 1. 画質 (正解に対する PSNR [dB]、全フレーム、YCbCr。本家は float)

| クリップ | 入力 | 当方フレーム | same/frames (既定) | same/fields | shift/frames | plain/frames | 本家 |
|---|---|---|---|---|---|---|---|
| old_town tff_y6 Y | 32.56 | 33.99 | 34.46 | 34.29 | 34.49 | **34.61** | 35.80 |
| park_joy tff_y6 Y | 32.64 | 23.80 | 30.93 | **31.90** | 30.21 | 30.34 | 34.17 |
| sintel tff_y3 Y | 39.01 | 33.14 | 40.39 | **41.32** | 40.58 | 40.64 | 44.83 |
| sintel tff_y6 Y | 33.08 | 32.55 | 36.37 | **37.24** | 36.62 | 36.67 | 39.81 |
| sintel tff_y3 Cb/Cr | 46.11/45.88 | 40.50/38.11 | 44.34/41.65 | 44.94/42.42 | 44.60/41.65 | 44.53/41.63 | 51.78/52.48 |
| sintel tff_y6 RGB | 31.10 | 30.21 | 33.17 | 33.94 | 33.40 | 33.42 | 37.86 |

フィールドごと (Y、top / bottom): どの出力も 2 つのフィールドの差は 0.2 dB 以内。
例 sintel tff_y3: 本家 44.78 / 44.87、same/fields 41.30 / 41.33、当方フレーム 33.24 / 33.05。

コーミング (大きく動く区分のエネルギー比。1 = 正解と同じ、< 1 = フィールドが混ざる) とコーミングの誤差 RMS:

| クリップ | 当方フレーム | same/frames | same/fields | shift/frames | plain/frames | 本家 |
|---|---|---|---|---|---|---|
| old_town | 0.22 / 7.03 | 0.60 / 5.26 | 0.60 / 5.24 | 0.60 / 5.29 | 0.61 / 5.24 | 0.84 / 4.87 |
| park_joy | 0.06 / 24.3 | 0.72 / 8.68 | 0.76 / 7.77 | 0.69 / 9.35 | 0.69 / 9.24 | 0.92 / 6.08 |
| sintel y3 | 0.63 / 8.11 | 0.95 / 2.85 | 0.96 / 2.63 | 0.94 / 2.77 | 0.94 / 2.75 | 0.99 / 1.48 |
| sintel y6 | 0.59 / 8.71 | 0.89 / 4.28 | 0.91 / 3.95 | 0.88 / 4.15 | 0.88 / 4.13 | 0.98 / 2.45 |

本家との近さ (出力どうしの Y PSNR [dB]、高いほど本家に近い):

| クリップ | 当方フレーム | same/frames | same/fields | shift/frames | plain/frames |
|---|---|---|---|---|---|
| old_town | 36.97 | **39.00** | 38.14 | 38.32 | 38.47 |
| park_joy | 24.40 | 33.38 | **34.78** | 31.92 | 32.05 |
| sintel y3 | 33.48 | 41.93 | **43.28** | 42.02 | 42.06 |
| sintel y6 | 33.34 | 38.15 | **39.62** | 38.22 | 38.26 |

観測:
- フィールド処理で、当方フレームの大きな悪化は解消した (park +7.1、sintel y3 +7.3、y6 +3.8、old +0.5 dB。既定どうしの比較)。
- それでも本家との差は 1.2〜3.5 dB 残る (最良の変種どうしで、old 1.19、park 2.27、sintel y3 3.51、y6 2.57)。
- 変種の間では、same/fields が 4 組中 3 組で最良で、既定より 0.87〜0.97 dB 高い。old_town だけは plain/frames が最良で、
  same/fields は既定より 0.17 dB 低い。

## 2. 静止部の並び

| クリップ | 当方フレーム | same/frames | same/fields | shift/frames | plain/frames | 本家 |
|---|---|---|---|---|---|---|
| old_town (静止 43%) Y static | 37.09 | 36.85 | 36.91 | 36.76 | 36.78 | 37.06 |
| park_joy (静止 1%) | 35.42 | 37.27 | 37.45 | 37.37 | 37.43 | 38.11 |
| sintel y3 (静止 26%) | 43.54 | 43.68 | **43.85** | 43.44 | 43.45 | 43.65 |
| sintel y6 | 38.21 | 38.12 | 37.99 | 38.28 | 38.27 | 37.60 |

静止区分のフレーム差と、残す割合 (`tools/diag_temporal.py`、フレーム t と t−1 = 同じパリティのフィールドで 2 フィールド間隔):

| クリップ | 指標 | 当方フレーム | same/frames | same/fields | shift/frames | plain/frames | 本家 |
|---|---|---|---|---|---|---|---|
| old_town | 出力のフレーム差 RMS (正解 5.43) | 1.88 | 1.79 | 2.00 | 1.65 | 1.68 | 2.76 |
| old_town | 足した雑音を残す割合 | 0.062 | 0.062 | 0.098 | 0.037 | 0.037 | 0.075 |
| park_joy | 足した雑音を残す割合 | 0.070 | 0.093 | 0.138 | 0.056 | 0.057 | 0.166 |
| sintel y3 | 足した雑音を残す割合 | 0.099 | 0.104 | 0.142 | 0.074 | 0.072 | 0.168 |
| sintel y6 | 足した雑音を残す割合 | 0.062 | 0.068 | 0.109 | 0.040 | 0.039 | 0.151 |
| sintel y6 | 正解の揺らぎを残す割合 | 0.21 | 0.25 | 0.32 | 0.27 | 0.27 | 0.52 |

観測:
- 静止部の PSNR では、本家・当方フィールド・当方フレームの差は小さい (old・sintel で全体の幅 0.3〜0.7 dB)。sintel y6 では当方のほうが
  本家より高い (当方 38.0〜38.3、本家 37.60)。当方の差がつくのは動く区分である (sintel y3 で大きく動く区分は、
  same/fields 40.32、本家 45.11、当方フレーム 31.18)。
- 静止部の時間方向の平均の強さ (足した雑音を残す割合が小さいほど強い) は、強い順に shift・plain/frames、当方フレーム・same/frames、
  same/fields、本家と並ぶ。本家に最も近いのは same/fields である。
- 本家は静止部で雑音を当方より多く残す (old で出力のフレーム差 2.76、当方 1.65〜2.00)。そのうえで、静止部の PSNR は同等である。

## 3. 除いた量の中身 (`tools/diag_removed.py`、全表は `eval/diag_tables_interlaced.md`)

Y 全体の「他」(除いた量のうち雑音以外の RMS)。足した雑音は 2.86〜6.0:

| クリップ | 当方フレーム | same/frames | same/fields | shift/frames | plain/frames | 本家 |
|---|---|---|---|---|---|---|
| old_town | 5.06 | 4.78 | 4.82 | 4.79 | 4.73 | 4.00 |
| park_joy | 16.44 | 7.08 | **6.19** | 7.80 | 7.69 | 4.42 |
| sintel y3 | 5.58 | 2.34 | **2.05** | 2.33 | 2.31 | 1.12 |
| sintel y6 | 5.89 | 3.71 | **3.25** | 3.66 | 3.64 | 2.02 |

低域 (帯域 5 より下) で除いた量の RMS (sintel y6、足した雑音 1.55 は黒帯の直流のずれ):
当方フレーム 2.48、same/frames 1.47、same/fields 1.03、shift・plain 1.36、本家 0.17。

観測: フィールド処理でも、プログレッシブで見た性質 (粗い帯域・低域で画を削る、`E9_progressive_findings.md`) は残る。
変種の中では same/fields が最も少なく削る。

## 4. INT-4 の既定の提案と根拠

提案: **mode = same (同じパリティ)、Temporal Radius はフィールドで数える (R2 → 2 フィールド)**。
すなわち、現在の same/frames を same/fields に変える。

根拠 (観測):
1. 画質: 4 組中 3 組で最良。既定 (same/frames) より park +0.97、sintel y3 +0.93、y6 +0.87 dB。
   残る old_town では −0.17 dB (最良の plain/frames より −0.32 dB) で、損は小さい。
2. 本家との近さ: 出力どうしの PSNR で 3 組が最も本家に近い (park 34.78、sintel 43.28 / 39.62。既定より +1.3〜1.5 dB)。
   old_town だけは既定のほうが近い (39.00 対 38.14)。
3. 静止部の時間平均の強さが本家に最も近い (足した雑音を残す割合: same/fields 0.10〜0.14、本家 0.075〜0.17、
   既定 0.06〜0.10、shift・plain 0.04〜0.06)。本家の「R2」が 2 フィールドを意味するという読みと整合する
   (推測。本家の半径の定義は未解読で、出力からの当てはめである)。
4. コーミングの保存 (比 0.60〜0.96) と除く「他」の少なさが、当方の変種の中で最良か同等。
5. 処理時間が最も短い (同じ条件の書き出しで 6 秒。same/frames 9 秒、shift・plain 15〜16 秒)。

mode について:
- shift と plain は、ほぼ同じ結果になる (互いの PSNR 51〜54 dB)。same より強く平均するので、静止部の雑音は減る。
  一方で、動く区分の画を削り、コーミングを弱める。本家に近づく方向ではない。
- 推測: 両パリティを使う場合、半ラインの補正は、この素材と半径ではほとんど効いていない。

注意:
- 素材は 4 組 (うち sintel は 1 本の 2 レベル) だけ。old_town のように静止部が多く粒状が強い素材では、半径が長いほうが
  わずかに良い (plain/frames が最良)。
- 本家との差 (1.2〜3.5 dB) の大部分は mode や半径の差ではなく、プログレッシブと同じ問題 (雑音の見積もり過ぎ、粗い帯域・低域を
  削ること、色差の強すぎ) とみられる (推測。どの変種でも本家との差がほぼ同じ形で残るため)。

## 道具と出力
- 当方フィールドの処理: `tools/run_ours_fields.py --build <scratch>/int3build CLIP...`
  (`tools/cpp_int3/` を scratch の int2 の写しに対してビルド: 写しで `make -C native lib` → cmake)
- 評価: `tools/eval_all.py` (runs/ours_field と runs/ours_field_* を自動で拾う)、`tools/summarize.py`、
  `tools/diag_all.py` (同)、`tools/diag_table.py --names ...`
- 出力: `eval/<src>_tff_<lv>.json`、`eval/diag_*_tff_*`、`eval/diag_tables_interlaced.md`、`eval/summary_*.txt`、
  `logs/run_ours_fields.log`・`eval_interlaced_fields.log`・`diag_all_interlaced.log`
- ビルドの写し (scratch): int2 の native/・qtui/ と data/ref_floor.npy。libnvcore と e9_profile_fields・qtui_export は警告 0 でビルドした。
