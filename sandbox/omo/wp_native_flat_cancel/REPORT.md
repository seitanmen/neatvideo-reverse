# F1: N1d の取り消し (状態 5) の移植 — REPORT

作業: 2026-10-27、worktree `.claude/worktrees/n1fix` (HEAD ca689d2)。タスク文は `.claude/tasks/native_n1_fixes.md` の F1。

## やったこと

- `nvc::genuine_noise_curve` (N1b) に、取り消しの問い合わせ `const nvc::Cancel& cancel` を追加した
  (`std::function<bool()>`。空なら問い合わせない)。
  - repro/nv_curve.py の `_cancelled` と同じく、タイルを取る前に毎回 1 回、全タイルの後にもう 1 回
    (スケジューラが空) 問い合わせる。真が返ったら、以後は問い合わせずに偽を返す (曲線の失敗)。
    取り消しがなければ、問い合わせは タイル数 + 1 回。
  - 複数スレッド (`threads > 1`) では、「問い合わせ → タイル番号の確保」を 1 つの mutex の中で行う。
    このため問い合わせ回数は 1 スレッドと同じになる (k 回目で取り消しなら k 回)。
    取り消しの後、他のスレッドは処理中のタイルを終えてから止まる (その結果は使わない)。
- `nvf::find_flat_area_genuine` (N1d) に、同じ `cancel` 引数を末尾に追加した。`noise == nullptr` のときは曲線にも渡す。
  - 曲線で取り消し → `(0, 0, 0, 0, 5)`。repro と同じ (本家は出力を書かず、repro はハーネスの初期値 0 を返す)。
  - 各サイズの先頭 (i0 以降のサイズごとに 1 回) で問い合わせる。取り消しなら、**それまでの最良の正方形
    (512 への切り詰め前)** と状態 5 を返す。最良の正方形がまだなければ 0 を返す。
  - 状態 1 で終わる経路 (縮小後の大きさが 0、合うサイズなし) は曲線の前に終わるので、問い合わせない (repro と同じ)。
  - C++ にキャッシュはない (repro のキャッシュは `nvfilter.find_flat_area` の側にある)。
- 照合用ドライバ `nv_flat_test cancel IN OUT [-j N]` を追加した。
  - 環境変数 `NV_FLAT_CANCEL_AT=k` を受け、k 回目以降の問い合わせに真を返す数え役で各ケースを実行する。
    数え役は neatvideo_ofx_614/analysis/noise/verify_cancel.py の `Counter` と同じ (k = 0 または未設定なら真を返さず数えるだけ)。
  - 出力はケースごとに `(x, y, w, h, status, 問い合わせ回数)` の i32 6 個。
- 照合スクリプト `native/verify_flat_cancel.py` を新規に作り、Makefile の `verify` にも追加した。

### タスク文と repro の食い違い (repro を優先した)

- タスク文:「取り消しのとき、領域は書かない (書きかけの最良値も返さない)」。
- repro/nv_flat.py: サイズ先頭の取り消しでは「outputs as written so far (the best square of the earlier sizes,
  before the cut of step 5; zeros if none yet)」を返す
  (`return (*(best if best is not None else (0, 0, 0, 0)), 5)`)。
- 統括の指示どおり repro に合わせた。曲線段の取り消しは repro も 0 を返すので、食い違いはない。
- AGENTS.md §6.5 の状態 5 の節が記す本家の観測 (「それまでの最良の正方形 (512 への切り詰め前。まだなければ未記入) のまま状態 5」) とも整合する。

### 曲線側の問い合わせ回数

C++ の nv_flat は曲線を内部で計算する (`noise == nullptr`)。そこで、曲線側の タイル数 + 1 回も含めて Python と一致させた
(タスク文の「FindFlatArea 側のみでよい」という簡略は使っていない)。
明示の補間器を渡したとき (`noise != nullptr`。repro に `noise_at` を直接渡す形) は、サイズ先頭の問い合わせだけになる。

## 変更したファイル

- `native/nv_curve.h` / `native/nv_curve.cpp`: `nvc::Cancel` と `genuine_noise_curve(..., threads, cancel)`。
- `native/nv_flat.h` / `native/nv_flat.cpp`: `find_flat_area_genuine(..., diag, cancel)`。
  ヘッダの「Not ported: the cancel query」を更新し、状態 5 の説明を「cancelled」にした。
- `native/nv_flat_main.cpp`: `cancel` サブコマンド。
- `native/verify_flat_cancel.py` (新規)、`native/Makefile` (`verify` に追加)。

## 検証

`make -C native && python3 native/verify_flat_cancel.py --jobs 3`
(ログ: `sandbox/omo/wp_native_portable/final_verify_all.txt` の該当部分)

### 入力

- ケース 17 個。
  - 実写 8 フレームの色モード 1: test_v02.mp4 の n = 0 / 312 / 576 / 700 (1080p)、test.mp4 の n = 0 / 300 / 450 / 750 (640x360)。
  - うち 2 フレームは色モード 0 / 2 も照合。
  - うち 2 フレームは明示の補間器 (曲線の問い合わせなし)。
  - 合成 3 種: verify_cancel.py の 200x150 / 64x64 / 96x400 のノイズ。
- k の値:
  - 実写: 0 (取り消しなし)、1、2、曲線の中間、タイル数 − 1、タイル数、タイル数 + 1 (曲線の最後の問い合わせ)、
    各サイズの問い合わせ、総数 − 1、総数、総数 + 1。
  - 合成: 0〜総数 + 1 のすべての k。

### 判定

各 (ケース, k) で C++ を -j 1 と -j 3 で実行し、次をすべて確かめた。

1. (x, y, w, h, status) と問い合わせ回数が Python と一致すること。Python 側の呼び方:
   `nv_flat.find_flat_area_genuine(rgb, None, mode, cancel=c, curve=lambda: noise_at(genuine_noise_curve(rgb, cancel=c)))`
2. Python 側が次の性質を満たすこと:
   1 ≤ k ≤ 総数 のときちょうど状態 5、それ以外は取り消しなしの結果で、問い合わせ回数 = 総数。
3. 取り消しなしの従来の入口 `nv_flat_test flat` の結果が Python と一致すること。

### 結果

- **676/676 一致**。F1 直後の実行、F4 後の通し (`make verify`)、F4 の portable build (`nv_flat_test_p`) のいずれでも同じ。
- 問い合わせの総数:

  | 入力 | 総数 | 内訳 |
  |---|---|---|
  | 1080p | 247 | 曲線 241 (240 タイル + 1) + サイズ 6 (AGENTS.md §6.5 の本家の観測と同じ) |
  | 640x360 | 35 | 28 + 1 + 6 |
  | 200x150 | 11 | |
  | 64x64 | 4 | |
  | 400x96 | 9 | |

- 各ケースに、部分正方形つきの状態 5 (サイズ段での取り消し) が含まれる (1080p では 5 個の k)。

### 感度試験 (照合が空振りしないことの確認)

scratchpad で作った変異体を `NV_FLAT_EXE` で照合した。

- サイズ段の取り消しで、最良値ではなく 0 を返す変異: 152/676 不一致 (検出)。
- 曲線の最後の問い合わせを省く変異: 190/676 不一致 (検出。回数が 1 少ない)。

### 回帰

`python3 native/verify_flat.py --jobs 3` は **362/362 一致**。
F1 + F3 の後 (ログ `verify_flat_regress.txt`)、F4 の後 (`final_verify_all.txt`)、portable build (`final_verify_portable.txt`) で確認した。

## 計時

取り消しを渡さない従来の経路で増えたのは、問い合わせの判定と、複数スレッド時のタイル確保の mutex だけ
(1080p で 240 タイルに 1 回ずつ)。

1080p の `nv_flat_test time -j 1` (M2 Max、負荷平均 3.2〜4.2) は、変更前 0.811〜0.814 秒、変更後 0.837〜0.841 秒 (+3%)。
曲線単体は 0.486〜0.489 秒で変わらないので、差は F4 で nv_band の減算を命令固定にした分とみられる (推定)。
詳細は portable の REPORT の計時表にある。

verify_flat_cancel.py 全体は約 156〜160 秒 (Python 60 秒、C++ 95 秒)。C++ は 1 ケースを 1 プロセスで実行するので、1080p の読み込みを含む。

## 未解決点・注意

- `nvc::genuine_noise_curve` の偽は、「48px 未満」と「取り消し」の両方を表す。
  FindFlatArea からは 48px 以上でしか呼ばないので、区別は不要。単体で使う呼び出し元が区別したいときは、
  `cancel` を自分で数えるか、事前にサイズを確かめる。
- 本家を複数スレッドで動かすホストではワーカーごとに問い合わせるので、回数は一致しない (AGENTS.md §7)。
  C++ を複数スレッドで動かしたときは、1 スレッドの本家・repro と同じ回数にしてある。

## Python 側への不具合報告

なし。
