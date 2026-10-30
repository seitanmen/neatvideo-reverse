# N1i: AutoFineTune のネイティブ移植 (nv_aft) — REPORT

REPORT.md は作業者が書けなかった (サブエージェントの .md 書き込みは止められる) ため、作業者の最終応答を統括が転記した。
作業日: 2026-10-27〜28。作業者: n1i-aft (Claude)。作業ツリー: `/Users/seitan/Desktop/work/reverse` (main、コミットなし)。

## 1. やったこと

- 次の Python 実装を C++ (`native/nv_aft.{h,cpp}`) に写した。正 (ソース・オブ・トゥルース) はこの Python 実装。
  - `neatvideo_ofx_614/analysis/noise/aft_port/src/nv_aft.py`
  - `repro/nv_aft_v5.apply_aft` のうち、AFT 呼び出しより後の部分 (色の重み・面曲線の補間・√結合・σ・ビンの旗)
- 再利用した既存部品 (いずれも変更なし):
  - `nvb::mean_exact`・`nvb::quantile` (N1a)
  - `nvc::is_clip_genuine` (N1b)
  - `nvm::model_full` (N1e)
  - `nvn::Interp` (N1g、numpy.interp の再現)
- 照合用ドライバ `native/nv_aft_main.cpp` (`nv_aft_test`) と単独の makefile `native/Makefile.aft` を作った。
- 照合 `native/verify_aft.py` を作った (shuffle / sim / finish / aft / combine / --mutants)。
- 本家バンドル・nvh は実行していない。repro/・neatvideo_ofx_614/・既存の native ソースは変更していない。

## 2. 使われている入口と移植した範囲

- 観測: 実際の呼び出し経路は次のとおり。
  - `nv_ui_qt/backend.run_aft` → `nv_aft_v5.apply_aft(prof, rgb, mode=0)`
  - → `nv_profile.build_profile(rgb, 1, area)` → `nv_aft.auto_fine_tune(rgb, raw, mode=0)`
  - image2・cancel・fast・mode 1/2 は repro からは呼ばれていない (grep で確認)。
- 使われている経路 (必須): `auto_fine_tune` の mode 0、image2 なし、cancel なし。移植し、照合した。
- その他の経路も移植し、照合した。
  - mode 1。
  - mode 2: `shuffle_order` のスロット順、ビンが 32 個に達した場合のスキップ、`mode2_post` の戻り値 3 個。
  - image2 の差分判定。
  - cancel: k 回目の問い合わせで真を返す形。diag の計数も Python と一致させた。
  - W または H が 48 未満の経路 (False)。
- `auto_fine_tune_fast` は結果が `auto_fine_tune` と同じ (aft_port の自己試験による) なので、別の実装は作っていない。C++ は 1 本で両方に相当する。
- `curve_fill` の flag & 1 経路は C++ にも書いたが、`_finish` が常に flag 0 で呼ぶので到達せず、未照合。

## 3. 関数の対応表

| Python | C++ (`nva::`) | 備考 |
|---|---|---|
| `_penalty` | `penalty` | numpy float32 スカラーの a+b / a·b は (b, a) の順 (asm で固定) |
| `_dev` / `_ratio_dev` / `_cl` | `dev` / `ratio_dev` / `cl` (無名名前空間) | |
| `similarity` | `similarity` | 積の順、CV 鎖の打ち切り (順序付き < 0、NaN は続行) |
| `polyfit_aft` / `polyval_aft` | `polyfit_aft` / `polyval_aft` | double。CPython float と同じく (a, b) の順。pow は volatile ポインタ経由の libSystem pow |
| `nv_curve._det` / `_cramer` | `det` / `cramer` (複写) | nv_curve.cpp の同名関数は無名名前空間なので写した。積の順が `polyfit_genuine` と違うため、polyfit は別実装 |
| `shuffle_order` | `shuffle_order` | |
| `nv_profile.curve_bin` | `curve_bin` | |
| `curve_post` / `curve_fill` / `mode2_post` / `_finish` | 同名 (`finish` は公開) | |
| `_tiles` + `_run_tiles` | `run_tiles` | 段 1 (IsClip・平均・ビン) と段 2 (モデル・類似度・image2・比) に分けた。`-j N` で並列 |
| `auto_fine_tune` | `auto_fine_tune` | 状態: 0 OK / 1 False / 2 None (取り消し) / 3 モデル失敗 |
| `nv_aft_v5.apply_aft` の結合部 | `combine` | 下記の sum() を参照 |

## 4. 移植で見つかった落とし穴 (観測)

- **CPython 3.12 以降の `sum()` は float を Neumaier 補償で足す。** `_color_weights` の `denom = sum(...)` は単純な加算では一致しなかった (combine が 69 件中 16 件で cw だけ不一致)。
  - CPython の builtin_sum_impl を再現して一致させた: 0 + x0 → 補償ループ → c が 0 でなく有限なら f += c。
  - ndarray の sum() は通常の加算のままでよい。
- numpy float32 スカラーの NaN の選び方を実測した (numpy 2.1.3、arm64)。2 つの NaN の加減乗除で、add / mul は第 2 被演算子、sub / div は第 1 被演算子が残る。MIGRATION.md の記述と一致する。
- cancel 時の diag: Python は取り消しまでに処理したタイルを数えるので、C++ も cancel_at − 1 個のタイルを処理してから状態 2 を返す。
- mode 2 の並列化: スキップの判定がそれまでに受理されたタイルに依存する。そこで `-j > 1` では全タイルを投機的にモデル化し、スキップをスロット順に決め直す。結果は `-j 1` とバイト一致する (無駄な計算はスキップされた分だけ)。

## 5. 検証

コマンド:
```
make -C native -f Makefile.aft
python3 native/verify_aft.py [--quick] [--no-real] [--jobs 3] [--mutants]
```

最終実行 (2026-10-28、Python 側はキャッシュ済み、20.8 秒):
```
shuffle    4097/4097 OK
sim        3000/3000 OK
finish     3000/3000 OK
aft -j 1   67/67 OK
aft -j 3   67/67 OK
combine    69/69 OK
total 20.8 s -> ALL BIT-EXACT
```
初回 (期待値の計算込み) は `tmp/full_run1.log` で 193.2 秒、ALL BIT-EXACT。Python の AFT は合計 322.8 秒だった。

### 比較した内容 (いずれもビット比較)

- **shuffle**: n = 0..4096 の全件。全要素を比較した。
- **sim**: 乱数の 3000 件。
  - 内容: 現実的な比のモデル語に、CV 鎖の負値、0 の許容・0 の語、NaN (5 種のペイロード)・±inf、s_lvl / s_mult の 0・inf・NaN・負値、符号反転、非正規数・巨大値、同一モデルを混ぜた。
- **finish**: 3000 件、mode 0/1/2 を乱数で選んだ。比較は 0xc300 バイト全体と mode 2 の値 3 個。
  - 曲線の状態: 新規 (1 ビンだけ埋まっている)、乱数のレベル・count (−1..2)・旗バイト 0x1c〜0x1f (0/1/2)、または実プロファイル。
  - ビン: 長さ 0〜40。NaN・inf・0・1e30 を含むもの、定数 1.0 / 3.0 / 2.9999998 のものを混ぜた。
  - 被覆 (sys.settrace による行の被覆): similarity・curve_post・curve_fill・mode2_post・polyfit_aft の到達可能な行はすべて通った。通らなかったのは docstring、到達しない `ncoef < 1`・`npts < 1`、`flag & 1` の経路だけ。
- **aft**: 67 件。それぞれ `-j 1` と `-j 3` で実行した。
  - 比較: 状態、返る 0xc300 バイト全体、mode 2 の値、diag 7 項目、`_run_tiles` のビン 27 列。ビンは `nv_aft._run_tiles` を包んで取り出した。
  - 実写 24 件:
    - test_v02.mp4 n=312 / 100 / 576 (1080p 全面) の mode 0。プロファイルの領域は FindFlatArea の結果 (832,0,128² / 800,0,128² / 488,864,64²)。
    - n=312 の mode 1・mode 2、n=312 の 128² 固定領域。
    - test.mp4 n=0 / 300 / 750 (640×360 全面) × {FindFlatArea 領域 (48²〜64²), 128² 領域 (256,96)} × mode 0/1/2。
    - 1080p の受理タイルは 260〜545。mode 2 の 1080p では 689 タイルが「32 個に達した」スキップを通った。
  - 合成 43 件:
    - 内容: 勾配に白色・有色・RGB 別のノイズを足したもの、白飛び、定数ブロック、NaN/inf (ペイロード混在)、階段、48×48、47×120 (W < 48)、512×96、W / H が 32 の倍数でないもの。
    - 各ケースを mode 0/1/2 で回し、image2 は 2 種、cancel は 1 / 7 / 48 / 49 回目。
    - 状態の内訳: OK 61、False 3、取り消し 3。
- **combine**: 69 件。aft の mode 0 の 23 組 × 3 種。
  - 条件: v5 の曲線と tot_ratio・area_level を乱数で与えた。area_level は範囲外の −5 と 300 を含む。band は 0..4、q99 = 0 (分母 0 → 1/3) も入れた。
  - 比較: aft_color_weights・aft_face_curves・curve Y/Cb/Cr・sigma・aft_bin_flags。
  - Python は `apply_aft` そのものを、`build_profile` / `auto_fine_tune` をキャッシュ済みのバイトに差し替えて実行した。

### 陰性対照 (`--mutants`、`tmp/mutants_run.log`)

4 種すべてを検出した。後の 3 種は、aft_port の自己試験で検出されなかった誤りの型である。

| 変異 | 検出 (不一致件数) |
|---|---|
| NVA_MUT_Q66 (分位点 0.66 → 0.67) | finish 2792、aft 46 |
| NVA_MUT_UPPER (上側外挿の last > 5 → > 4) | finish 219、aft 9 |
| NVA_MUT_CVTOL (CV 許容に s_lvl を掛ける) | sim 97、aft 54 |
| NVA_MUT_GT3 (> 3.0 の旗を >= に) | finish 449 |

### キャッシュ

- 置き場所: `sandbox/omo/wp_native_aft/tmp/` の `profile_*.pkl` と `aft_*.pkl`。
- キー: 入力画像 (と image2) の SHA-256、領域、モード、cancel、移植元ソースのハッシュ (repro/nv_*.py と aft_port/src/nv_aft.py。nv_ui.py・nv_depth.py・nv_cov.py は除く)。
- 実フレーム: `sandbox/omo/wp_native_band/tmp/*.npy` を読み取りのみで使う。無ければ ffmpeg で rgb48le / 257 に切り出す。

### ビルド

フラグは `-std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra`。クリーンビルド (`-B`) で警告 0。

## 6. 計時 (M2 Max、負荷平均 5.5、best of 3)

| 入力 | Python `auto_fine_tune` (初回計算時) | C++ -j 1 | C++ -j 3 |
|---|---|---|---|
| test_v02 n=312 1080p mode 0 | 33.5 s | 0.756 s | 0.282 s |
| test_v02 n=100 mode 0 | 36.8 s | 0.804 s | 0.298 s |
| test_v02 n=576 mode 0 | 30.9 s | 0.689 s | 0.258 s |
| test_v02 n=312 mode 1 | 29.9 s | 0.672 s | 0.251 s |
| test_v02 n=312 mode 2 | 21.9 s | 0.494 s | 0.286 s |
| test.mp4 n=0 640×360 mode 0 | 4.4 s | 0.097 s | 0.039 s |

- 1 スレッドで約 44〜55 倍速い。
- Python の時間は期待値を作ったときの 1 回の測定で、3 並列の負荷下。
- 負荷平均 102 の時に測った値 (3 s 前後) は捨てた。
- プロファイル構築 (Python `build_profile`) は 128² で 14.7〜17.8 秒。今回の計時には含めていない (C++ 化は N1f)。

## 7. 未解決点・注意

- 正は aft_port の Python 実装で、本家と照合したのはタイル順だけ (README)。今回の一致は「Python と同じ」ことの証明で、本家との一致の証拠ではない。
- 2 つの NaN の被演算子順が問題になる箇所の扱い:
  - AFT 本体: numpy の規則どおり asm で順を固定した。
  - 色結合部 (`combine`) と CPython の Neumaier ループ内: 被演算子順を確かめていない。そこを通る入力は照合していない。CPython の C コードは clang が可換演算を入れ替え得る。
- `np.maximum(ws, 0)` の −0 の扱いは確かめていない。C++ は ws >= 0 ? ws : 0 で、−0 はそのまま返す。重みも曲線も非負なので、実際には ws = −0 にならないとみられる (推定)。
- x86 / `NVC_PORTABLE` 相当の経路は未検証。AArch64 以外では asm の代わりに素の演算になり、NaN のビット列の一致は保証しない。nv_model の非 AArch64 未対応 (N1e) にも依存する。
- `curve_fill` の flag & 1 経路は未照合 (到達しない)。
- `MODEL_ERROR` (`nvm::model_full` の失敗) は 32×32 では起きない。Python は例外になる経路で、照合していない。
- 照合はこの環境 (numpy 2.1.3・Python 3.13.5・arm64) に固定。sum() の補償加算は Python 3.12 以降の挙動なので、Python を下げると combine の期待値が変わる。

## 8. Python 側への不具合報告 (直していない)

- `nv_aft_v5._color_weights` は Python の `sum()` (3.12 以降は補償加算) に依存している。本家の FUN_009aae90 が float32 か double の単純加算なら、ここだけ本家と違い得る (推定。本家とは未照合)。
- `apply_aft` は AFT の 9 ビン語を `np.interp` で v5 の 8 中心 (16, 48, …, 240) に補間している。両端のビン 0 と 8 (輝度 0 と 256) は、中心の範囲外なので使われない。意図どおりかは不明 (観察のみ)。
- `nv_aft.auto_fine_tune` の戻り値の型は場合によって変わる。mode 2 では成功時だけ tuple で、W < 48 と取り消しでは bytes になる。呼び出し側が mode 2 を使うなら注意 (現状 repro は mode 0 だけ)。

## 9. 既存部品への要望 (統合時に統括が判断)

- `nv_curve.cpp` の `det` / `cramer` / `g_pow` は無名名前空間にあるので、`nv_aft.cpp` に複写した (重複)。共通ヘッダに出せば重複は消える。polyfit は積の順が違うので、別のままでよい。
- 無名名前空間の asm ヘルパ (A/M/AD/AS/MS) は `nv_band.cpp` と重複している。F3 (共通化) の対象に加えてほしい。
- `Makefile.aft` は既存の makefile と同じ形式で、`$(MUTFLAGS)` を持つ。`native/Makefile` の all / verify への追加は統括側で行う。`.gitignore` の `native/**` の例外に `nv_aft*`・`Makefile.aft`・`verify_aft.py` を足す必要がある。

## 10. 新規ファイル

- `native/nv_aft.h`
- `native/nv_aft.cpp`
- `native/nv_aft_main.cpp` (ドライバ `nv_aft_test`: aft / sim / finish / shuffle / combine / time)
- `native/Makefile.aft`
- `native/verify_aft.py`
- `sandbox/omo/wp_native_aft/tmp/` (キャッシュ、照合の入出力、`full_run1.log`・`mutants_run.log`、計時入力 `time.in`)
- `sandbox/omo/wp_native_aft/REPORT.md` (この本文。統括が転記)

