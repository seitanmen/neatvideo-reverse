# E9 のやり直し: 本家と同じプロファイルのフレーム (フレーム 5) (2026-11-02)

作成: e9-f5-worker。観測と推定は分けて書く (推定には「推定」と明記)。処理コア・main の道具・aft2 のコードは変えていない。
本家のバンドルは実行・dlopen・リンクしていない。Resolve は操作していない。`~/.config/NeatRepro/nv_ui_qt.ini` の sha256 は
作業の前後とも `45d4258c…80fcc6`。全数値は `summary_rerun_f5.txt`、プロファイルの記録は `profiles_rerun_f5.txt`、
AFT の曲線と本家 .dnp の比較は `aft_vs_dnp_rerun_f5.txt`、評価の JSON は `rerun_f5/<clip>.json`、ログは `rerun_f5/logs/`。

## 0. 結論 (観測)
- **フレームを 5 にしても、本家との差はほとんど縮まらない。** フレームの効果 (B−A、Y) はプログレッシブ 6 本で −0.81〜+0.00 dB
  (最大は park_joy p_y3 の −0.81 dB)、インターレース 4 本で −0.35〜+0.22 dB (old_town tff +0.04・park_joy tff +0.22・sintel tff_y3 −0.35・
  sintel tff_y6 +0.08)。
- **16 ビット化と積み方 (C−B) の画質への効きは、10 本中 9 本で |ΔY| ≤ 0.02 dB。** 例外は park_joy tff_y6 で、Y −0.97 dB・RGB −0.52 dB 悪くなる。
- ただし AFT の曲線そのものは本家に近づく。C の曲線は 10 本中 7 本で本家の .dnp と最大 |ln 比| ≤ 0.018、旗 (マーカー) は 9 本で 27/27 一致。
  インターレースは 4 本とも C で旗 27/27 (B では 22〜27/27)。park_joy tff_y6 は B 0.386 → C 0.017 で、このとき PSNR が下がった。
- mode auto の 9 本では AFT 曲線の出力への効きが小さく (§3.3)、C−B が 0.02 dB を超えたのは mode spatial の park_joy tff_y6 の 1 本だけ。
  本家との差 (Y で 0.4〜8.9 dB) の原因は、プロファイルのフレームや AFT の入力の作り方ではないとみられる (推定。§4)。

## 1. 条件
- A: 既存の結果 (フレーム 0・従来のコード)。プログレッシブ `runs/ours_frame/`、インターレース `runs/ours_field/` (既定の変種 same/frames)。再処理はしていない。
- B: フレーム 5・従来のコード。
  - プログレッシブ: 既存の `wp_e9/build` (e9_profile・qtui_export の sha256 は A の run.json と同じ。libnvcore は main の現在の
    `native/libnvcore.dylib` = `e084970d…`、A の記録は `4f0b6350…`)。
  - インターレース: `tools/cpp_int3` (main の道具、無変更) を main の作業ツリーのソースでビルドした `wp_e9/build_main_int3`
    (A は int2 worktree の写しのビルド `scratchpad/int3build`)。
  - 同じコードであることの確認 (観測): B のビルドでフレーム 0 を処理し直すと、10 本すべてで A の out.mp4 とバイト一致
    (インターレースは ui_profile.json も一致。`rerun_f5/logs/chk_f0_hashes.txt`)。確認用の出力は消した。
- C: フレーム 5・aft2 worktree のソース (コミット 61d566e と同じソース。ビルドはコミット前で、更新時刻で確認。`native/libnvcore.dylib` = `51983f1d…`)。
  - プログレッシブ: aft2 の `tools/cpp` (無変更) を `wp_e9/build_aft2` にビルド。e9_profile は `run_aft` を呼び、aft2 の `run_aft` は
    `run_aft_rgb` 経由で `nvc_aft_input16` を通す (16 ビット化)。
  - インターレース: aft2 の `tools/cpp_int3/e9_profile_fields.cpp` の AFT の呼び出しを、`profile_image` + `run_aft` から
    `run_aft_fields` (MainWindow::auto_fine_tune と同じ。Two fields は先のフィールドを上に積む) に替えて `wp_e9/build_aft2_int3` にビルド。
  - ビルドはどれも警告 0。RPATH は aft2 の native/ を指すことを otool で確認。
- 共通: Auto Profile (Current Frame) → Auto Fine-Tune、既定のパラメータ、`--crf 0`・`--jobs 4`、設定は `runs/settings_e9.ini`。
  インターレースは Two fields・first 0 (TFF)・変種 same/frames (radius 4 フィールド) だけ。
- 評価: main の `tools/e9_eval.py` (無変更) を `--out` 付きで呼ぶ (aft2 の `tools/eval_rerun_f5.py`)。本家の書き出しは
  eval_all.py の find_export で選んだもの (progressive/Event_バージョン 1_0001_000N、interlaced_noRI/…_000N)。
- 評価器が同じであることの確認 (観測): 再評価した input・genuine・A の値を既存の `wp_e9/eval/<clip>.json` と、両方にある
  全キー (all・interior、10 本で 86〜240 値) で比べ、すべて一致。A~genuine の組も一致 (`summary_rerun_f5.txt` 冒頭)。

## 2. プロファイルの記録 (観測、`profiles_rerun_f5.txt`)
B・C とも profile_frame 5・aft_frame 5・aft_applied True・ui_frame 5。インターレースは 3 通りとも "Two fields"・field top。
B と C は領域・flat_status・mode が同じ (C で変わるのは AFT 由来のキー: aft_*・curve・sigma・ui_rgb_weights だけ)。

| クリップ | A の領域 | B・C の領域 | 本家の見本 | 一致 |
|---|---|---|---|---|
| old_town p_y3 | 280,248,128 | 0,1568,128 | 0,1568,128 (見本平均に最も近い位置) | 同じ |
| old_town p_y6 | 280,328,128 | 0,1568,128 | 0,1568,128 (同上) | 同じ |
| park_joy p_y3 | 736,1304,128 | 872,1064,128 | 720,968,128 (同上) | **違う** |
| park_joy p_y6 | 984,480,96 (status 2) | 984,248,96 (status 2) | 984,248,96 (同上) | 同じ |
| sintel p_y3 | 720,168,128 | 160,616,128 | 160,616,128 (同上) | 同じ |
| sintel p_y6 | 680,160,192 | 680,304,128 | 680,304,128 (同上) | 同じ |
| old_town tff_y6 | 上 72,1216,128 | 上 64,328,128 | 上 y64 x328 128² | 同じ |
| park_joy tff_y6 (spatial) | 上 368,1304,128 | 上 344,752,192 | **下** y348 x720 192² | **違う** (フィールドも) |
| sintel tff_y3 (status 2) | 上 88,1824,96 | 上 104,1792,96 | **下** y116 x1784 96² | **違う** (フィールドも) |
| sintel tff_y6 | 上 96,1448,128 | 上 88,1792,128 | 上 y88 x1792 128² | 同じ |

(領域は y, x, size。インターレースはフィールドの行。本家の見本はプログレッシブが aft2 の REPORT §4.1 (見本平均からの位置)、
インターレースが `wp_aft_runtime/out/logs/locate_i.log`。)
観測: フレーム 5 にすると 10 本中 7 本で本家と同じ領域を選ぶ (フレーム 0 では 0 本)。違う 3 本のうち 2 本は、本家が後の
(下の) フィールドで見本を取っているもの (当方は先のフィールドだけで探す。aft2 REPORT §2.1 の既知の違い)。

AFT の曲線 (3ch × 9 ビンのレベル) と本家 .dnp の比較 (`aft_vs_dnp_rerun_f5.txt`、最大 |ln(当方/本家)|・旗の一致):

| クリップ | A | B | C |
|---|---|---|---|
| old_town p_y3 | 0.271・25/27 | **0.001・27/27** | 0.185・26/27 |
| old_town p_y6 | 0.140・25/27 | 0.000・27/27 | 0.000・27/27 |
| park_joy p_y3 | 0.037・27/27 | 0.049・27/27 | 0.049・27/27 |
| park_joy p_y6 | 0.059・26/27 | 0.002・27/27 | 0.000・27/27 |
| sintel p_y3 | 0.313・23/27 | **0.007・27/27** | 0.258・27/27 |
| sintel p_y6 | 0.503・23/27 | 0.001・27/27 | 0.004・27/27 |
| old_town tff_y6 | 0.062・24/27 | 0.036・25/27 | **0.000・27/27** |
| park_joy tff_y6 | 0.378・26/27 | 0.386・26/27 | **0.017・27/27** |
| sintel tff_y3 | 0.575・25/27 | 0.022・27/27 | 0.018・27/27 |
| sintel tff_y6 | 0.445・23/27 | 0.017・22/27 | **0.000・27/27** |

観測:
- インターレースでは C (16 ビット化と積み方。インターレースでは両者を分けていない) で 4 本とも本家の旗と一致し、曲線も 2 本で ≤ 5e-4、残る 2 本で ≤ 0.018。
- プログレッシブの old_town p_y3 と sintel p_y3 では、C のほうが B より本家から離れる。
  - old_town p_y3: マーカーが入れ替わったのは B (青) 面のビン 1 の 1 ビン (本家・B では測定 0.979、C ではマーカー = 補間 1.103) だが、
    B (青) 面の曲線の値はビン 0・1・2・4 で違う (最大 |ln 比| 0.185 はビン 0: 本家 0.917、C 1.103)。
  - sintel p_y3: マーカーは 27/27 で一致しており、|ln 比| 0.258 はマーカーの入れ替わりによるものではない (値のずれ)。
- 推定: こちらの入力は 8 ビット 4:2:0 の y4m を RGB に戻したもので、本家に渡した 16 ビット TIFF とは 1/514 程度の差がある。16 ビット化
  (1/16 刻みへの丸め) は丸めの境目でこの差を拡大し、タイルの採否 (類似度の積 > 0.5) か標本数 4 の境目にかかったビンが入れ替わったとみられる。
  aft2 の受け入れ照合 (TIFF を入力) では 14 本とも ≤5 ulp なので、16 ビット化そのものの誤りではない。

## 3. 画質 (正解に対する PSNR [dB]、全フレーム。Y・Cb・Cr は YCbCr float、RGB は RGB)

### 3.1 プログレッシブ
| クリップ | 入力 Y | A Y | B Y | C Y | 本家 Y | A/B/C Cb | 本家 Cb | A/B/C Cr | 本家 Cr | A/B/C RGB | 本家 RGB |
|---|---|---|---|---|---|---|---|---|---|---|---|
| old_town p_y3 | 38.55 | 36.75 | 36.69 | 36.69 | 37.42 | 38.42/38.40/38.39 | 39.93 | 39.80/39.76/39.76 | 42.97 | 32.35/32.31/32.31 | 33.60 |
| old_town p_y6 | 32.56 | 35.94 | 35.93 | 35.93 | 36.36 | 38.22/38.21/38.21 | 39.78 | 39.19/39.20/39.20 | 42.55 | 31.80/31.79/31.79 | 32.92 |
| park_joy p_y3 | 38.57 | 32.15 | 31.34 | 31.34 | 40.28 | 33.80/33.44/33.44 | 44.05 | 37.23/37.11/37.11 | 45.13 | 28.48/27.95/27.95 | 37.09 |
| park_joy p_y6 | 32.71 | 31.47 | 31.30 | 31.30 | 36.46 | 33.60/33.45/33.45 | 41.19 | 36.92/36.92/36.92 | 42.46 | 28.04/27.90/27.90 | 33.55 |
| sintel p_y3 | 39.03 | 43.59 | 43.59 | 43.59 | 45.48 | 47.83/47.80/47.80 | 53.05 | 43.72/43.72/43.72 | 53.74 | 39.00/39.00/39.00 | 43.34 |
| sintel p_y6 | 33.09 | 39.27 | 39.25 | 39.25 | 40.41 | 45.67/45.63/45.63 | 50.90 | 41.53/41.51/41.51 | 49.88 | 35.72/35.70/35.70 | 38.50 |

### 3.2 インターレース (TFF、フィールド処理 same/frames)
| クリップ | 入力 Y | A Y | B Y | C Y | 本家 Y | A/B/C Cb/Cr | 本家 Cb/Cr | A/B/C RGB | 本家 RGB | コーミング比 A/B/C | 本家 | コーミング誤差 A/B/C | 本家 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| old_town tff_y6 | 32.56 | 34.46 | 34.51 | 34.53 | 35.80 | 37.66/38.25, 37.68/38.31, 37.68/38.32 | 39.56/42.07 | 30.62/30.66/30.67 | 32.38 | 0.601/0.605/0.607 | 0.842 | 5.26/5.24/5.23 | 4.87 |
| park_joy tff_y6 | 32.64 | 30.93 | 31.16 | 30.18 | 34.17 | 31.75/35.61, 32.04/35.61, 31.89/35.55 | 38.43/39.94 | 26.81/27.03/26.51 | 31.32 | 0.721/0.733/0.684 | 0.923 | 8.68/8.46/9.47 | 6.08 |
| sintel tff_y3 | 39.01 | 40.39 | 40.04 | 40.03 | 44.83 | 44.34/41.65, 44.45/41.37, 44.44/41.36 | 51.78/52.48 | 36.26/36.00/35.99 | 42.61 | 0.949/0.947/0.947 | 0.990 | 2.85/2.99/2.99 | 1.48 |
| sintel tff_y6 | 33.08 | 36.37 | 36.45 | 36.45 | 39.81 | 42.20/39.69, 42.52/39.71, 42.52/39.71 | 49.67/48.94 | 33.17/33.25/33.25 | 37.86 | 0.893/0.898/0.898 | 0.978 | 4.28/4.26/4.26 | 2.45 |

フィールドごとの Y (top/bottom) は、どの出力も差 0.04 dB 以内 (全表に記載)。静止・小・大の動きの区分、時間方向の残差も全表にある。

### 3.3 分解 (Y、dB。gap = 本家 − 当方。~本家 = 当方と本家の出力どうしの Y PSNR)
| クリップ | B−A (フレーム) | C−B (16 ビット・積み方) | gap A | gap B | gap C | 縮んだ量 (A→C) | ~本家 A / B / C |
|---|---|---|---|---|---|---|---|
| old_town p_y3 | −0.05 | −0.00 | 0.67 | 0.73 | 0.73 | −0.05 | 43.42 / 43.27 / 43.26 |
| old_town p_y6 | −0.01 | +0.00 | 0.42 | 0.43 | 0.43 | −0.01 | 41.19 / 41.17 / 41.17 |
| park_joy p_y3 | −0.81 | −0.00 | 8.13 | 8.94 | 8.94 | −0.81 | 32.58 / 31.71 / 31.71 |
| park_joy p_y6 | −0.17 | +0.00 | 4.99 | 5.16 | 5.16 | −0.17 | 32.46 / 32.26 / 32.26 |
| sintel p_y3 | +0.00 | +0.00 | 1.89 | 1.89 | 1.89 | +0.00 | 46.05 / 46.06 / 46.06 |
| sintel p_y6 | −0.03 | +0.00 | 1.13 | 1.16 | 1.16 | −0.03 | 41.98 / 41.93 / 41.93 |
| old_town tff_y6 | +0.04 | +0.02 | 1.33 | 1.29 | 1.27 | +0.07 | 39.00 / 39.09 / 39.13 |
| park_joy tff_y6 | +0.22 | −0.97 | 3.23 | 3.01 | 3.98 | −0.75 | 33.38 / 33.72 / 32.27 |
| sintel tff_y3 | −0.35 | −0.01 | 4.43 | 4.78 | 4.80 | −0.36 | 41.93 / 41.45 / 41.43 |
| sintel tff_y6 | +0.08 | −0.00 | 3.45 | 3.37 | 3.37 | +0.08 | 38.15 / 38.26 / 38.25 |

Cb・Cr・RGB の分解も `summary_rerun_f5.txt` にある (傾向は Y と同じ。最大は sintel tff_y6 の Cb +0.33 (B−A)、park_joy p_y3 の Cb −0.35 (B−A)、
park_joy tff_y6 の RGB −0.52 (C−B))。

観測:
- フレームの効果 (B−A) は −0.81〜+0.22 dB。本家と同じ領域・ほぼ同じ AFT 曲線になった sintel p_y3 でも Y は ±0.00、本家との出力の近さも 46.05 → 46.06。
- 16 ビット化と積み方の効果 (C−B) は park_joy tff_y6 を除いて ±0.02 dB 以内。park_joy tff_y6 は 10 本中ただ 1 本の mode spatial
  (ほかは auto) で、C で G の係数がビン 2〜8 で 1.043 → 1.509 (本家 1.534) になり (B ではビン 2 がマーカー、C では本家どおり測定)、
  Y −0.97 dB・コーミング比 0.733 → 0.684。
- auto では AFT 曲線の出力への効きが小さい: 独立検証者の観測では、old_town p_y3・sintel p_y3 で C の曲線が B から |ln 比| 0.185・0.258 動いても、
  実効の sigma の変化は 1e-4 程度だった。AFT 曲線の違いに出力が反応したのは spatial の 1 本だけである。
- 本家との差が合計 (A→C) で縮んだのは 2 本 (old_town tff +0.07、sintel tff_y6 +0.08 dB) だけ。sintel p_y3 は ±0.00、残る 7 本は広がった (0.01〜0.81 dB)。

## 4. 推定 (差の原因について)
- 推定: E9 の本家との差 (Y 0.4〜8.9 dB、色差 1.5〜11 dB) は、プロファイルのフレームや AFT の入力の作り方によるものではない。本家と同じ領域・
  ほぼ同じ AFT 曲線 (|ln 比| ≤ 0.007) になった sintel p_y3・p_y6・old_town p_y6・park_joy p_y6 でも、差は A とほとんど変わらないため。
- 推定 (spatial の 1 本、park_joy tff_y6 での観測に限る): 曲線が本家に近づくと PSNR が下がったのは、当方が曲線を使う側 (RGB 面の係数を
  Y/Cb/Cr の曲線へ掛ける写像 (aft_color_weights)、CHROMA_GAIN、空間・時間の各段での σ の使い方) が本家と違うためとみられる。
  「σ が上がって除き過ぎた」というのは解釈であり、確かめていない。この 1 本から auto の本や処理コア一般には広げられない。
- 注意: 曲線が本家とほぼ同じになった auto の本は、auto では AFT 曲線が出力にほとんど効かないため、「本家どおりの曲線を正しく使えば差が
  縮むか」の試験になっていない。上の 1 つ目の推定 (差の原因はフレームや AFT の入力ではない) も、この範囲でのものである。
- 推定: 残る領域の違い 3 本のうち 2 本 (park_joy tff_y6・sintel tff_y3) は、Two fields の見本を両フィールドから探す本家の動作 (aft2 REPORT §2.1) を
  当方が再現していないことによる。park_joy p_y3 の違いの原因は調べていない (aft2 REPORT §7 にも同じ記録)。

## 5. 実行したコマンド (再実行用。cwd は任意、W = `/Users/seitan/Desktop/work/reverse/work/omo/wp_e9`、
   T = `/Users/seitan/Desktop/work/reverse/.claude/worktrees/aft2/work/omo/wp_e9/tools`、R = `/Users/seitan/Desktop/work/reverse`)
ビルド (C は aft2 のコードが変わったらこの 2 つと `make -C $R/.claude/worktrees/aft2/native lib` をやり直す):
```
cmake -S $R/.claude/worktrees/aft2/work/omo/wp_e9/tools/cpp -B $W/build_aft2 -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build $W/build_aft2 -j 2
cmake -S $R/.claude/worktrees/aft2/work/omo/wp_e9/tools/cpp_int3 -B $W/build_aft2_int3 -DSRC_ROOT=$R/.claude/worktrees/aft2 -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build $W/build_aft2_int3 -j 2
cmake -S $W/tools/cpp_int3 -B $W/build_main_int3 -DSRC_ROOT=$R -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos && cmake --build $W/build_main_int3 -j 2   # B (インターレース)
```
処理 (全体は `rerun_f5/logs/run_all.sh`、各 10 秒前後):
```
P="old_town_cross/p_y3 old_town_cross/p_y6 park_joy/p_y3 park_joy/p_y6 sintel/p_y3 sintel/p_y6"
I="old_town_cross/tff_y6 park_joy/tff_y6 sintel/tff_y3 sintel/tff_y6"
python3 $T/run_ours.py $P --root $W --build $W/build --nvcore $R/native/libnvcore.dylib --profile-frame 5 --tag f5 --jobs 4                                  # B
python3 $T/run_ours_fields.py $I --root $W --build $W/build_main_int3 --variants same_frames:same:4 --profile-frame 5 --tag f5 --jobs 4                     # B
python3 $T/run_ours.py $P --root $W --build $W/build_aft2 --nvcore $R/.claude/worktrees/aft2/native/libnvcore.dylib --profile-frame 5 --tag f5q16 --jobs 4  # C
python3 $T/run_ours_fields.py $I --root $W --build $W/build_aft2_int3 --variants same_frames:same:4 --profile-frame 5 --tag f5q16 --jobs 4                  # C
```
評価と表 (評価は 2 本並列で約 13 分):
```
python3 $T/eval_rerun_f5.py --root $W --jobs 2
python3 $T/summarize_rerun_f5.py --root $W
```
AFT 曲線と .dnp の比較は `wp_aft_runtime/tools/aftlib.py` の `curve(image(dnp))` と nvprof.json の `base.aft_levels`・`aft_bin_flags.marker`
を比べる小さなスクリプトで行った (出力 `aft_vs_dnp_rerun_f5.txt`。スクリプトは残していない。必要なら書き直す)。

出力: `$W/runs/ours_frame_f5/`・`ours_frame_f5q16/`・`ours_field_f5/`・`ours_field_f5q16/` (各 190〜235 MB、計約 850 MB)。
注意: main の `tools/eval_all.py` は `runs/ours_field_*` を変種として拾うので、main で eval_all を流し直すと `ours_field_f5`・`ours_field_f5q16` も
既存の eval/*.json に入る。

## 6. 道具の変更 (aft2 worktree の `work/omo/wp_e9/tools/`、main の道具は無変更)
- `run_ours.py`: `--root` (clips/・runs/ のある wp_e9)、`--build`、`--nvcore` (run.json に記録する libnvcore) を追加。既定は従来どおり。
- `run_ours_fields.py`: `--tag` (出力を `runs/ours_field_<tag>/`・`ours_field_<tag>_<変種>/` に分ける) と `--root` を追加。既定は従来どおり。
- `cpp_int3/e9_profile_fields.cpp`: AFT を `run_aft_fields` で呼ぶ (aft2 の MainWindow::auto_fine_tune と同じ)。aft2 のソース専用 (main の qtui には `run_aft_fields` がない)。
  これは引数の追加ではなく既定の動作の置き換えである (`run_aft_fields` は aft2 の qtui にしかない)。このため、マージ後に A と B のインターレースを
  再現するには、A と B のコミットのツール (`tools/cpp_int3/e9_profile_fields.cpp` の変更前、main の 1c953a1 のもの) を使う必要がある。
- 新規: `eval_rerun_f5.py` (評価の呼び出し)、`summarize_rerun_f5.py` (表と評価器の確認)。

## 7. 未確認の点
- 本家の 16 ビット TIFF そのものを入力にした E9 の処理はしていない (当方の入力は 8 ビット 4:2:0 の y4m。§2 の C の 2 本の曲線のずれの原因の推定は未検証)。
- park_joy p_y3 の領域の違い、Two fields の見本を後のフィールドから取る動作 (§4)。
- インターレースの変種は same/frames だけ (same/fields など他の 3 変種はフレーム 5 で流していない)。
- プロファイルを同じにしたうえで差がどの処理段 (時間・空間・色差) で生じるかの分解は、この作業の範囲外。

## 7. 独立検証 (2026-11-02、統括が転記)
- 判定 (e9f5-verifier): **APPROVE** (ブロッカー 0)。文言の修正 M-1・L-1〜L-4 を作業者が反映済み (この版)。
- 検証者の独立な再現 (観測):
  - B のビルドでフレーム 0 を処理すると A とバイト一致 (sintel p_y3・park_joy tff_y6)。
  - B・C のフレーム 5 を再処理すると、runs/ の out.mp4・nvprof.json と一致 (park_joy tff_y6)。
  - 自作スクリプトの PSNR (2 本 × A・B・C・本家)、AFT 曲線と本家 .dnp の比較 (10 本 × A・B・C) が表と一致。
  - ビルドの由来: B は main の libnvcore (`nvc_aft_input16` なし)、C は aft2 の libnvcore (import あり)。
- 統括の再計算: park_joy tff_y6 の Y の PSNR を、8 ビットの Y 面から別スクリプトで計算。A 30.93・B 31.15・C 30.18 dB で、表と小数第 1 位まで一致 (表は float の YCbCr で計算)。
