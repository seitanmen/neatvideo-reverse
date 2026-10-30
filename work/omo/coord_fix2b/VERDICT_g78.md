# VERDICT G7・G8 (独立検証者 verify-g78、2026-10-30)

対象: worktree `.claude/worktrees/n3fix-b` の未コミット変更 (main df46dda から。作業者 fix2-b)。
検証用のスクリプトとログ: `sandbox/omo/coord_fix2b/independent/`。ビルド物と作業ファイルは scratchpad の `vg78/`
(`/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/vg78/`)。
ソースは変えていない。コミット・stash・ブランチ操作はしていない。本家バンドルは実行していない。
変更前の版は `git archive HEAD native` を scratchpad で別ビルドし (以下「HEAD 版」)、変更後の版も scratchpad でビルドした
(作業者の `native/libnvcore.dylib` とバイト一致 = `cmp` で一致を確認)。

## 判定: **REQUEST CHANGES** (軽微な修正 1 件。それ以外はすべて合格)

直す必要があるのは 1 件 (F1) だけで、修正は 1 行で済む。G7・G8 の中身 (ビット一致・取り消し・速度化・ABI) は、
作業者と別の入力で確かめて合格した。

| # | 項目 | 判定 |
|---|---|---|
| 1 | G8: ON (C) と OFF (Python) の出力 dict の全語ビット一致、フォールバックの条件 | 実入力はすべて合格。**ただし C に行くべきでない入力が 1 種類ある (F1)** |
| 2 | G8: 高速化 (分位点・`_dead`・ブロック取り出し) と `nvn::b3` NEON 化で結果が変わらないこと、フラグ、警告 0 | 合格 |
| 3 | G7: 取り消しの遅れが 1 秒未満 (Python・Qt C++)、取り消さないときの blob 不変、k 回目で停止、複数スレッドでの安全性 | 合格 |
| 4 | ABI 1→2、backend の公開 I/F、処理コアが無変更 | 合格 |
| 5 | 報告の数値・記述 | おおむね正確。誇張が 1 点、書かれていないことが 2 点 (下記) |

## 指摘

### F1 (重大度: 低〜中。修正必須): 輝度面の 1 辺が 1 px のフレームで `nvc_build_profile_v5` が SIGSEGV になり、Python UI (native ON) のプロセスごと落ちる
- 観測: `nv_native.build_profile_v5` は H = 1 か W = 1 (もう一方 ≥ 2) のフレームを C に渡す。変更後のライブラリは
  1×2・2×1・1×3・3×1・1×50・50×1 で、area=None と area=(0,0,8)、auto と spatial のどれでも終了コード 139 (SIGSEGV) になった
  (`independent/tiny2.py`、各ケースを別プロセスで実行。faulthandler で見ると `nv_native._p5_call` の ctypes 呼び出しの中で落ちている)。
  1×1 は落ちずに dict を返すが、Python (OFF) は `ValueError` (np.gradient) を送出するので、結果が一致しない。
  C 単体でも同じように落ちる (`independent/c.cpp`: 1×50 で `nvc_build_profile_v5` を直接呼ぶと終了コード 139)。
- 変更前: HEAD 版の `nv_native.build_profile_v5` (C 化は領域探索だけ) では、同じ入力は OFF と同じ `ValueError` だった
  (`independent/tiny.py old`。全 14 ケースが OFF と同じ)。`nvc_build_profile_v5` 自体は HEAD 版でも同じように落ちる
  (HEAD のライブラリで同じ結果)。つまり不具合は N3a からあり、Qt C++ UI では以前から起きる。G8 でそれが Python UI からも起きるようになった。
- 実際に起きる可能性は低い (幅か高さが 1 px の動画が要る)。それでも、例外で済んでいたものがプロセスの異常終了になるうえ、
  「C で同じ結果を保証できない入力は従来の経路に回す」という仕様 (nv_native の docstring と REPORT) にも反する。
- 修正案: `_p5_args` で `min(H, W) < 2` のとき None を返す (1×1 の不一致もこれで直る。輝度が 2 以上なら色差が 1×1 でも
  一致することは確認済み: 2×2・2×3・3×2・2×50・50×2 は OFF と一致)。Qt C++ を守るため、C 側の `nvc_build_profile_v5` でも
  `H < 2 || W < 2` を −2 にしたほうがよい (Qt は例外になる)。落ちる箇所は nv_prof5 の中だが、どの関数かは特定していない
  (lldb がこの環境では何も出力しなかった)。

### F2 (情報): 速度の見出しの数字は最良値
- 観測 (`independent/bench.py`、test_v02 n=312・auto・キャッシュなし、HEAD 版の経路と交互に 3 回ずつ、負荷平均 8.5〜10.2):
  - 1 スレッド: 変更後 1.186 / 1.393 / 1.435 / 1.263 / 1.278 / 1.299 秒、HEAD 版 1.737〜1.916 秒
  - 4 スレッド: 変更後 0.522〜0.555 秒、HEAD 版 1.190〜1.245 秒
- 速くなったことは確かで、改善幅は報告の交互測定 (1.26〜1.50 秒 / 1.77〜2.02 秒) と合う。ただし報告の「1.174 秒で目標の 1.3 秒を下回った」は
  最良の 1 回の値で、負荷下では 6 回中 2 回が 1.3 秒を超えた。

### F3 (情報): 報告に書かれていないこと
- `nvn::b3` は `nv_spatial` (spatial_v5) と `nv_temporal` (時間段の atrous) も使う。そのためこの変更はフレーム処理と書き出しにも効く。
  ビットが変わらないことは下の 2 で確かめた。作業者の最終ログ (`fix2b/final_vb.log`、frame 72/72・export 4/4。11:47〜12:03 で最終のソースより後) でも一致している。
- 問い合わせの回数: 512² の AFT 全体で、私の測定は 62,616 回、報告は 62,621 回。数え方 (run_in_child・_apply_aft の外側の問い合わせを含めるか) の違いとみられる (推測)。

## 根拠

### 1. G8 のビット一致とフォールバック (`independent/g8.py`、ログは `independent/g8.log`。結果 131/133、外れた 2 件は F1 の 1×1)
比較のしかた: `nvfilter.build_profile_v5` (Python) と `nv_native.build_profile_v5` (C) の戻り値を、キーの順・型・float の 64 ビットのビット列まで比べた。
例外が出る場合は例外の型を比べた。`nv_native._p5_call` を包んで、C の呼び出しが実際に起きたかも記録した。
area=None のケースは、キャッシュなし (1 回目) とキャッシュに当たる 2 回目の両方で比べた。スレッド数は 1 と 3。
- 実写 (作業者の n とは別): test_v02 n=50・300・600、test.mp4 n=100・333・700。各フレームで「探索 + モードを巡回」と
  「手動の長方形 (H/3+7, W/5+3, 150, 90)」。pair = n+1、more = (n+1, n+2) で UI と同じ。**すべて一致**
- 変種 (test.mp4 n=333): float の area・フレーム外の area・負のサイズ・4 要素で h==w・np.int64 配列の area・pair ≠ more[0]・
  temporal で pair なし・pair も more もなし・full range + BT.2020・BT.601 の spatial・list の planes・負のストライドのビュー。**すべて一致**
- 合成: 2×2〜200×130 の小さいフレーム (64 未満は勾配による代替の方法を通る)、4:4:4、定数面 (noise-free)、全面 −0、非正規数、負の値、
  ±1e30・±3e38 の巨大な有限値 (Python 側は途中で inf/NaN になる)、勾配。**1×1 以外すべて一致**
- フォールバック (C を呼ばずに Python と一致するべきもの): uint8、輝度に NaN、色差に inf、pair に −inf、levels 4、levels が np.int64(5)、
  mode "foo"、matrix "smpte170m"、5 要素の area、float64 の色差、奇数の高さで色差が H//2。**いずれも C を呼ばず、結果も一致**
- コードの確認 (`repro/nv_native.py` の `_p5_args`・`_p5_frame`): float32 の 2 次元・形・有限値・色差の形 (同じか (H+1)//2)・
  levels (厳密に int の 5)・mode と matrix の名前・area の要素数と int32 の範囲を確かめている。輝度の大きさの下限だけが抜けている (F1)
- キャッシュの整合 (36 件、2×2〜360×640 × 3 種類): C の経路がキャッシュに書いた値は、`nv_native.find_flat_area` が自分で計算した値、
  `nvfilter.find_flat_area` の値、prof の area・flat_status と一致
- 取り消し (`independent/p5cancel.py`): test.mp4 n=500 (問い合わせ 35 回) と test_v02 n=151 (247 回) で、k = 1, 2, 5, N/2, N−7, N−1, N, N+1。
  Python と C (1・3 スレッド) で、結果 (ProfileCancelled か dict か) と問い合わせ回数が一致。不一致 0

### 2. 高速化と b3 で結果が変わらないこと (`independent/unit.cpp`)
- HEAD 版と変更後のソースから同じプローブをビルドした。プローブは `nv_prof5.cpp` を取り込んで無名名前空間の関数を直接呼ぶ。
  ハッシュは 3 シードとも一致
  - `nvn::b3` (step 1〜32・axis 0/1、W 1〜129・H 1〜33) と `nvn::atrous` (5 段): 1 シードあたり 365 万語。入力は有限値・非負・非正規数 (±)・±0・±inf・
    qNaN (符号と払い出し値をランダムに)・sNaN・±3e38 を混ぜたもの
  - `percentile_f32` (q = 0・25・50・99・100) と `trim_rms`: n = 1〜300,001。8192 以上はヒストグラムの経路に入る。入力は非負のみ・−0 を含む・
    NaN を含む・+inf・重複の多い値・全部同じ値・非正規数だけ
- プローブが変化を見逃さないこと (変異を入れて試した): ① b3 の NEON 部で加算の被演算子を入れ替える (NaN の選び方が変わる)、
  ② 選択の順位を 1 つずらす、③ −0 も高速経路に通す。どれもハッシュが変わり、検出できた
- コードの確認: `select2_nonneg` は値のビット列が 0x7f800000 以下のとき (非負かつ NaN でない。−0 は外れる) だけ使う。このとき
  ビット列の大小は値の大小と同じで、ゼロは +0 だけなので、k 番目の値は nth_element の結果とビット単位で同じになる。
  `trim_rms` は写しで分位点を取るので、並べ替えの有無は後の和に影響しない。
  b3 の 4 レーン部は、スカラー部と同じ被演算子の順の FADD/FMUL (.4S) を使う (`nvp::add`/`mul` はインラインアセンブラ)。反射の要らない区間
  (x ≥ 2·step かつ x+3+2·step < W) だけで使い、`NVP_ASM` が 0 (portable) のときはスカラーのまま。ブロックの取り出し (`block_at`) は
  元の `blocks()` と同じ行優先の並びで、時間側の 1 回の走査も添字の順を保っている
- フラグと警告: `make -n lib` で `-O2 -ffp-contract=off -fno-fast-math -Wall -Wextra` (portable は加えて `-DNV_PORTABLE`) を確認。
  lib・lib_portable のクリーンビルドで警告 0 (HEAD 版も 0)

### 3. G7 (`independent/g7blob.py`・`g7lat.py`・`qtlat.cpp`)
- **取り消さないときの blob**: HEAD 版の `nvc_profile_build` と、変更後の `nvc_profile_build` / `nvc_profile_build_cancel`
  (cancel が NULL・常に偽) を比べた。領域は 9 種類: 32²・128²・200×96・512²・77×45 (色モード 0〜3)・300×216・フレーム全体 (状態 2)・10×10 (状態 1)、
  NaN・inf・−0 を混ぜた 300² の合成。1 スレッドと 4 スレッドで、全 54 件が状態と 0xc300 バイトの SHA-256 まで一致
- **k 回目で止まること**: 128² (問い合わせ 4806 回) と 300×216 (14,654 回) で、k = 1, 2, 3, 6, 7, 50, N/3, N/2, N−1, N。1・4 スレッドの 40 件すべてで、
  戻り値 3 かつ問い合わせがちょうど k 回。問い合わせの総数は 1 スレッドと 4 スレッドで同じ
- **遅れ (Python の UI 経路 `backend.run_in_child(task, run_aft, …)`)**: test_v02 n=444 (作業者とは別のフレーム) で、領域は 512²・480×256・160²、
  1・4 スレッド、取り消しを立てる時刻はランダム。取り消しを立てたその時刻 (立てたスレッドで記録) から戻るまでを測った。
  最大 0.58 秒 (4 スレッド・512²。負荷平均 41〜60)。問い合わせの間隔は最大 0.363 秒 (512²・1 スレッド、62,616 回)。最初の問い合わせまで 0.025 秒、
  最後の問い合わせから戻るまで 0.003 秒以下
- **遅れ (Qt C++ の `run_aft`)**: 自作のハーネス (`qtlat.cpp`。最終版の `libnvui.a` と変更後のライブラリをリンク) で、同じ条件で 15 回 × 2。
  最大 0.134 秒 (1 スレッド) / 0.139 秒 (4 スレッド)
- **スレッド安全性 (コード)**: `nvp::build_profile` の `stop_fn` は、mutex の中で利用者の問い合わせを呼び、一度真になったら以後は呼ばない (固定)。
  そのため利用者の関数が同時に呼ばれることはなく、真の後にもう 1 回呼ばれることもない。`model_full`・`tile_dev` はこの関数を通してしか問い合わせない。
  `parallel_for` は戻る前に join するので、ローカルの mutex とフラグへの参照が寿命より長く残ることはない。`mst[i]`・`tst[k]` はスレッドごとに別の要素。
  ctypes の関数呼び出しは GIL を手放し、コールバックは GIL を取ってから mutex を待たないので、デッドロックはしない。
  Qt の `CancelScope` は呼び出し中だけスロットを保持する。`nvm::CANCELLED` と `nvp::CANCELLED` はどちらも 3 で、既存の状態値 (0〜2) とぶつからない

### 4. ABI・I/F・処理コア
- `NV_NATIVE_LIB` に HEAD 版 (ABI 1) のライブラリを指定すると `AVAILABLE False`。このとき `backend.set_native(True)` の後も
  `native_enabled()` は False で、`backend.build_profile` は Python の経路で動いた。nvc_abi_version を見るのは nv_native.py だけ
  (Qt C++ はリンク時に解決する)
- `git diff` で `repro/nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py` の変更なし。`repro/nv_ui_qt/backend.py` の変更は docstring 2 か所だけ。
  `verify_bridge_prof5.py` の参照を OFF にしたのは妥当 (ON は C 自身になったため)

### 5. 報告の記述
- 正しいと確認したもの: 遅れの原因 (`profile_blob` に問い合わせがなかった)、問い合わせの場所、k 回目で止まること、取り消さないときに blob が不変であること、
  遅れが 1 秒未満であること、Qt 側の配線、ABI、フォールバックで列挙された条件、速度の改善 (HEAD 版の経路との交互測定で 1 スレッド約 1.4 倍、4 スレッド約 2.2 倍)
- 直してほしい点: F1 (フォールバックの条件の抜け)、F2 (最良値を見出しにしている)、F3 (b3 の影響範囲が書かれていない)
- 確かめていないもの: 変更前の遅れの値 (12.6 秒など) は測り直していない (HEAD 版の `profile_blob` に問い合わせがないことはコードで確認)。
  PySide6 の画面を通した取り消しは backend の階層で代わりに確かめた。自己試験・verify_bridge の全群・書き出しの一致は統括が再実行中なので、ここでは繰り返していない

---

# Round 2 (F1〜F3 の修正の再検証、2026-10-30)

## 判定: **APPROVE**

F1 は塞がった。H と W がどちらも 2 以上の入力では、変更前とビット単位で同じ結果になる。作業者が「残る差」とした件が UI から到達できないという主張も、コードで確かめた。F2・F3 の書き直しも正確である。

変更点の確認: Round 1 の後に native で変わったのは次の 4 ファイルで、変更は入口の判定とコメントだけ。
- `nv_prof5.cpp`: `f.H < 1 || f.W < 1` を `< 2` にした
- `nvcore_c_prof5.cpp`: `H < 1 || W < 1` を `H < 2 || W < 2` にした
- `nv_prof5.h`・`nvcore_c_prof5.h`: 注記だけ

`repro/nv_native.py` の `_p5_args` には `min(H, W) < 2` なら None を返す判定が加わった。現在のソースを scratchpad の `vg78/new2` でビルドし直すと、警告は 0 (lib・lib_portable)。できた dylib は worktree の `native/libnvcore.dylib` とバイト単位で同じだった。

## 1. F1 が塞がったか: 塞がった
- `independent/tiny2.py`: 1 ケースずつ別のプロセスで動かした。全 162 ケースで、ON (修正後) の結果が OFF と同じになった (落ちたものも、結果が違うものもない)。
  - 形 (18 種): 1×1・1×2・2×1・2×2・1×3・3×1・2×3・3×2・3×3・4×4・1×50・50×1・2×50・50×2・5×5・8×3・1×640・360×1
  - area (3 種): None・(0,0,8)・(0,0,1)
  - mode (3 種): auto・spatial・temporal
  - Round 1 では 26 ケースが SIGSEGV、2 ケースで結果が違っていた。1×1 で手動の area を渡すケースも、ON が Python の経路に回るので OFF と同じ dict になった。
- `independent/c.cpp` (1×50 で `nvc_build_profile_v5` を直接呼ぶ): 戻り値 −2 で正常に終了した (Round 1 では終了コード 139)。

## 2. H, W ≥ 2 の境界で、HEAD 版の C と結果が 1 ビットも変わらないか: 変わらない
- `independent/bound.py`: HEAD 版と修正版の `libnvcore.dylib` を別々のプロセスで動かし、`nvc_build_profile_v5` の戻り値と out_i・out_d の全バイト (SHA-256) を比べた。
- 入力
  - 合成の形 (13 種) × 色差 4:2:0 / 4:4:4: 2×2・2×3・3×2・2×50・50×2・3×3・4×4・5×5・8×3・2×640・360×2・64×64・65×97
  - 実写: test.mp4 n=610 の全面と、そこから切り出した 2×640・3×99
- 組み合わせ
  - area (5 種): 探索・(0,0,8)・(0,0,1)・長方形 (0,0,2,3)・(1,1,40,2)
  - mode 0〜2、pair の有無
- 結果: 870 ケースすべてで戻り値が 0、出力は 2 つのライブラリで同じ (出力ファイルが `cmp` で一致)。Round 1 の変更 (nv_prof5 の高速化) も含め、HEAD 版との一致が境界の形でも成り立つ。

## 3. 「残る差」(1×1 で手動の area を渡すと、Python は dict、Qt C++ は −2 のエラー) が UI から到達できないか: 主張どおり
- 差そのもの: Python (OFF、ON も Python の経路) は dict を返す (tiny.py・tiny2.py)。C は H < 2 で −2 を返し、`nvqt::build_profile` はそれを `runtime_error` にする (`qtui/src/core/nvcore.cpp`)。
- 手動の area が渡る経路
  - 両 UI とも、手動の area は Build Profile (`main_window.py` の `build_profile`、`mainwindow.cpp` の `MainWindow::build_profile`) からしか来ない。
  - ほかの呼び出し元は area を渡さない: PySide6 の `main.py:107` と `main_window.py:1787`、Qt の `ofx.cpp:167` と `mainwindow.cpp:2270`。
  - Build Profile は両方とも、選択の短い辺が 32 未満ならエラーを出して終わる。
  - 選択の矩形は、ビューアがフレームの内側に切り詰める (PySide6 `widgets/viewer.py` の `_gesture_rect`、Qt `viewer.cpp:791〜800`)。1×1 のフレームでは 32 以上の選択は作れない。
  - `on_area` を呼ぶのは、ビューアの選択と selftest だけである。
- area なしの経路: Auto Profile では、Python が ValueError (ON・OFF とも)、Qt が −2 の例外になり、どちらもエラーで終わる。
- 確かめていないこと: 1 px の動画を実際に開いて画面から操作することはしていない (コードの経路だけ確かめた)。

## 4. 入口の判定が他の呼び出し元に影響しないか: 影響しない
- 呼び出し元の一覧
  - `nvp5::build_profile_v5` を呼ぶのは `nvcore_c_prof5.cpp` だけ。
  - `nvc_build_profile_v5` を呼ぶのは次の 4 つだけ: `qtui/src/core/nvcore.cpp` の `build_profile`、`repro/nv_native.py`、`native/verify_prof5.py`、`native/verify_bridge_prof5.py`。
- 照合用の入力
  - `verify_prof5.py` の合成サイズは最小で 36×50。
  - `verify_bridge_prof5` の入力は実写のフレーム。
  - どちらにも 1 辺が 2 px 未満のフレームはないので、この判定には当たらない。
- 実際の UI で作られるフレームで H か W が 2 未満になるのは 1 px の動画だけで、以前はそこで落ちていた。
- `case_g8_tiny` (verify_bridge) は、次の 3 つを比べている。
  - 例外の型とメッセージ、または dict の全語
  - `_p5_args` から見て C の経路に回るはずか
  - `min(H, W) ≥ 2` という期待
- 中身は妥当。実行は統括の `verify_bridge --only g8` に任せた。

## 5. F2・F3 の書き直し: 正確
- F2 (速度の節): 次の 3 つを並べて書いている。数字は Round 1 の私の測定と合う。
  - 最良値 (1 スレッド 1.174 秒、4 スレッド 0.532 秒)
  - 私の測定 (1 スレッドは 1.19〜1.44 秒で、6 回中 2 回が 1.3 秒を超えた。4 スレッドは 0.52〜0.56 秒。HEAD 版の経路は 1 スレッド 1.74〜1.92 秒、4 スレッド 1.19〜1.25 秒)
  - 作業者の交互測定
- 冒頭の「目標の 1.3 秒を下回りました」という見出しは消えている。
- F3: `nvn::b3`・`atrous` を `nv_spatial`・`nv_temporal` も使うので、フレーム処理と書き出しにも効くことを明記している。根拠として verify_spatial・verify_temporal・frame 72/72・export 4/4 と私のプローブを挙げている。
- 問い合わせの回数は「約 6.26 万回」になり、Round 1 の小さな食い違いも解消している。
- F1 節の原因 (高さ 1 だと色差の切り出しが 0 行になり、`robust_sigma` が範囲外を読む) は ASan による作業者の観測で、私は追試していない。ただし、修正後に落ちないことと結果が変わらないことは、上の 1・2 で確かめた。

## 指摘
新しい指摘はない。
