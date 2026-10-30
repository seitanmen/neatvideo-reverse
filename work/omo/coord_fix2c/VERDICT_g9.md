# VERDICT G9 (x86 検証の穴埋め) — 独立検証者 verify-g9 (2026-10-30)

対象: worktree `.claude/worktrees/n3fix-c` (main df46dda からの未コミット変更)。作業者 fix2-c の REPORT (0 件の再生の修正後の版) と、証拠 `sandbox/omo/wp_qtui_fix/x86_g9/`。
検証用のファイルは `sandbox/omo/coord_fix2c/independent/` に置いた。worktree のソースは変えていない。コミット・stash・ブランチ操作はしていない。

## 判定: **REQUEST CHANGES** (必須の修正は M1 の 1 件だけで、小さい)

主要な主張は確かめられた。x86 の差は temporal (expf) と nlc_image (既定 NaN の符号) だけで、分類器も既定の厳密判定も主張どおりに動く。arm64 の既定ビルドにも回帰はない。
ただし `--allow-known` に fail-open の経路が 1 つあり、x86 の再生が落ちても成功になる (M1)。これは直してほしい。L1〜L5 は直すことを勧めるが、合否には関わらない。

## 確認項目と判定

### 1. 記録・再生が本当に比較になっているか — 合格 (作業者が「未保証」とした出力サイズの穴は、quick の入力では塞がった)
- 観測 (捕捉の漏れ): libnvcore の C ABI のヘッダ 4 つで、エクスポートされた関数は 32 個。`nm -gU` の 32 個は SPEC のキー 32 個とちょうど一致した。
  - SPEC にない関数が呼ばれると `unspec` として数えられ、再生は失敗する。
  - libnvcore は 3 か所すべて `ctypes.CDLL(p)` (`repro/nv_native.py:72`・`verify_bridge_ui.py:44`・`verify_bridge_depth.py:42`) と `verify_prof5.py:52` で読み込まれていた。属性で引いても `CDLL.__getattr__` → `__getitem__` を通るので、置き換えたクラスが効く。`ctypes.cdll` や `np.ctypeslib` を使う経路はない。
  - 子プロセス (ProcessPool・spawn・export・fallback の subprocess) にも、`env` で PYTHONPATH が引き継がれる。
- 観測 (サイズの定義): 32 関数すべてについて、SPEC の引数の番号と大きさを `nvcore_c.h`・`nvcore_c_ui.h`・`nvcore_c_prof5.h`・`nvcore_c_depth.h` と 1 つずつ突き合わせた。食い違いはなかった。
  - 例: temporal の flow = (H/bs)(W/bs)・new_flow nf バイト、aft_combine の cw 9 個の double・flags 81、ofx の色差 (W/2)(H/2)、depth の 4:2:0/4:2:2/4:4:4 のバイト数、build_profile_v5 の out_i 16 個・out_d 72 個。
- 観測 (独立の動的確認、`independent/guard_replay.py`): 自分で `verify_bridge.py --quick` の全群を記録した (ALL MATCH、530 秒)。その 12,325 呼び出し (wn_band_std 以外の 31 関数) を arm64 の libnvcore.dylib で再生した。
  - 再生では各領域の前後に 4 KiB の番兵を置き、詰め物を 0x00 と 0xA5 の 2 通りで 2 回ずつ実行した。ポインタ配列にも、番兵だけのバッファを指す余分な要素を 8 個足した。
  - 結果: 番兵の書き換えは 0 件、記録との不一致は 0 件 (`independent/guard_replay.log`)。少なくともこの入力では、出力の見積もり不足 (書き過ぎ) も、入力の見積もり不足 (領域の外を読む) もない。
  - 感度: 出力の領域を 4〜8 バイト縮めると、minmax・flat_area・temporal_mask・depth_to_internal・noise_curve で番兵の書き換えを検出した。入力を縮めると、minmax・mean_f32 で結果の違いを検出した。
- 観測 (x86 で動いていること):
  - `libnvcore_x86.dylib` は `file`/`lipo` で x86_64 だけ (Non-fat) である。
  - `arch -x86_64 python3.12` は `platform.machine()` が x86_64、`sysctl.proc_translated` が 1 だった。この Python は arm64 の dylib の dlopen を拒否する。
  - 証拠の JSON の `machine` も x86_64 である。
- 判断: arm64 の再生が記録と一致することは、同語反復ではない。新しいアドレスに作り直した入力から呼び出しが再現できることを示している。ただ、それだけでは出力の見積もり不足は検出できない。そこは上の番兵の試験で補った。
- 自分の x86 再生 (quick、12,325 呼び出し): 12,291 件が一致した。差は temporal 32 件と nlc_image 2 件だけで、`x86_bridge_known.py` で全件が既知の分類に入った。
  - temporal の ULP の分布は 1:311,439 / 2:108,321 / 3:13,617 / 4:2,525 / 5:406 / 6:48 / 7:12 / 8:5。NaN と inf はない。
  - nlc は 6 語すべて 0x7fc00000 (arm64) と 0xffc00000 (x86) の組だった。

### 2. `--allow-known` の分類器 — おおむね合格 (L1・L2 の穴あり)
`independent/known_probe.py` で境界の入力を合成して確かめた (観測)。
- temporal:
  - 1 ULP・8 ULP・−8 ULP は合格、9 ULP は不合格。
  - +0 と −0 は 0 ULP として不合格。片側だけ NaN・NaN の符号違いは不合格。大きさの違う領域は不合格。
- nlc_image: NaN の符号違い・ペイロード違いは合格。NaN と有限値、NaN と inf、有限値の 1 ULP 差、+0 と −0 は不合格。
- 他の関数 (spatial・temporal_mask) は不合格。
- 既定 (オプションなし) は厳密なまま。分類は `--allow-known` で、かつ x86 の再生の rc が 1 のときだけ行う。arm64 と portable の再生は常に厳密に判定する。
- 穴 (L1): temporal で **inf と FLT_MAX (0x7f800000 / 0x7f7fffff) が「1 ULP」として合格**する。docstring の「finite only」と食い違う。`np.isnan` しか見ていないため。
- 穴 (L2): 分類器は領域の esize と役割を見ず、4 バイト単位なら何でも float32 の ULP で比べる。そのため、**4 バイトの uint8 バッファ (nf = 4 の new_flow) の [0,1,1,1] と [1,1,1,1] が 1 ULP として合格**する。int32 の領域でも同じことが起きる。
  - 今回の差は out/rv の float32 面だけ (432 面すべて n が 4 の倍数で esize 4。流れ場と new_flow は一致) なので、結果には影響していない。
- 細部 (観測): 同一のバッファを渡すと `u.min()` が空配列の例外を出す。シェルの `&&` で失敗側に倒れるので、危険はない。

### 3. nv_depth の 208,737,760 語の根拠 — 数値の出どころが違う (L3)
- 観測: 208,737,760 は `verify_bridge_depth` が数える「Python と arm64 の C の比較語数」である (`vb_plain.log:270`・`allow_known/capture.log:274`)。x86 の比較の語数ではない。
- x86 側の根拠は、depth 群の 18,751 呼び出しの再生である (sanitize 1,296・to_internal 3,029・from_internal 13,829・ofx_pack 487・ofx_unpack 110)。全件一致しており、再生の数え方では出力 216,163,431 語になる (`summary_x86.md`)。
- 判断: すべての呼び出しで全バッファがバイト一致しているので、「208.7M 語が x86 でも一致する」は推移的には正しい。REPORT の「208,737,760 語は x86 で全件一致」は、x86 で直接数えた値のように読める。「x86 の depth 18,751 呼び出しが全件一致 (= Python との 208,737,760 語の比較も x86 で成り立つ)」と書き直すべき。

### 4. 「temporal の差は expf が原因」 — 合格
- 観測 (証拠ファイル):
  - `full_lalt_arm.json` (arm64 の openlibm 版と記録の比較) の不一致は 72 件。`full_lalt_x86.json` (x86 の openlibm 版と arm64 の openlibm 版の比較) は 178/178 一致した。
  - 不一致の 72 件の集合は、`full_x86.json` の temporal の不一致 72 件とまったく同じだった (pid:seq の集合で照合)。
  - 432 面の総語数 337,651,200 は、領域の大きさから計算した値と一致する。0.44% も合っている。
- 観測 (小さな再現、`independent/expf_blocks.c`): 自分の C で expf を両アーキの 10 ブロック (各 2^20 入力) にかけた。
  - 作業者のハッシュで差ありとされた 8 ブロックは、すべて 1 ULP の差だった (15〜10,491 個)。同じと判定された 2 ブロック (100・2000) は差 0。作業者の走査結果と整合する。
  - 0×inf は arm64 が 0x7fc00000、x86 が 0xffc00000 で、nlc_image の説明と一致する。inf−inf は両方とも 0x7fc00000 だった。
- 未確認: 全 2^32 の入力で 2,018,697 個という総数は、ダンプが消されているので証拠からは数え直せない (ブロック単位のハッシュだけが残っている)。

### 5. arm64 の既定ビルドに回帰がないか — 合格
- 観測: HEAD の native を `git archive` で 2 つ取り出し、片方だけ新しい Makefile にして `make -n -B` を比べた。空白をまとめた後の差は 0 だった。
  - 比べた目標: all・lib・lib_portable・portable・x86・verify・verify_bridge・nv_lzma_test・lzma/obj/LzmaEnc.o。
  - 違ったのは clean だけで、lzma/obj_x86 と libnvcore_x86.dylib を消す行が増えている。
- 観測: lib_x86 の 7 つのコンパイル行すべてに `-ffp-contract=off -fno-fast-math` と `-arch x86_64` が付いている。
- 観測: 取り出した側で `make -j4 all lib_portable lib_x86` のクリーンビルドを行い、警告は 0。できた libnvcore.dylib と libnvcore_x86.dylib は、worktree のものとバイト一致した。

### 6. 報告の正直さ・数値 — おおむね正確 (L3〜L5)
- 観測: 次の数値は証拠と一致した。
  - 38,667 / 38,593 / 74、72 / 432 / 1,472,151、2 / 2 / 6
  - 727 秒・29 分 26 秒、group 表 (`summary_x86.md`)
  - 最終行の後付け (ログでは「ALL IDENTICAL」だけ) は REPORT で開示されている。
- 未保証の範囲の書き方は正直である。出力サイズの穴は、本検証の番兵試験で quick の入力については塞がった。
- L4 (証拠の欠け): REPORT の次の主張には、保存された比較結果がない。
  - 主張: 「NaN を含む入力では spatial・temporal・aft に NaN の語だけ」「profile 39・LZMA 667 は一致」
  - 残っているのは、x86_exe_pair の `--report` の出力ではなく、arm64 側の verify のログ (pair_*.log) だけ。
  - `verify_spatial_x86_direct.log` は x86 の試験バイナリと Python の比較で、NaN を含むケースだけが外れている (1007/1287)。傍証にはなる。
  - なお profile_build (LZMA 込み) は、libnvcore の再生で x86 でも全件一致している (全群 16/16+、quick 11/11)。
- L5: MAX_ULP = 8 は、観測した最大値そのものである (自分の quick でも 8 ULP が 5 語あった)。入力によっては 9 ULP が出て `--allow-known` が誤って失敗し得る。こちらは安全側の誤りなので、記録だけでよい。

## 指摘 (重大度順)

- **M1 (中、必須)**: `--allow-known` が、古い `replay_x86.json` と `dump_x86` で成功を返す (fail-open)。
  - 再現 (観測): 自分の記録から temporal の差の 3 呼び出しだけを含む cap で作業ディレクトリを作った。
    1. `--no-capture --only frame --allow-known` を本物の Python で 1 回実行する (rc 0)。
    2. 同じ作業ディレクトリで、x86 の再生が「traceback を出して rc 1」で落ちる偽物の `--py86` (`independent/fakepy.sh`) を使って実行する。
  - 結果: 2 回目も前回の JSON とダンプが分類され、「ALL IDENTICAL (x86_64: known differences only, --allow-known)」・rc 0 になった。
  - 原因: 2 つが重なっている。
    - Python の未捕捉の例外も rc 1 になり、「差あり」の rc 1 と区別できない。
    - x86 の再生の前に `replay_x86.json` と `dump_x86/` を消していない。`--no-capture` は KEEP=1 なので、ダンプは常に残る。
  - 修正案: 再生の前に `replay_*.json`・`dump_x86`・`dump_arm64` を消す。さらに「差あり」の終了コードを例外と重ならない値 (例: 3) にし、シェルはその値のときだけ分類する。
- **L1 (低)**: 分類器 (a) が inf と有限値の差を「1 ULP」として通す。`np.isfinite` で両側とも有限であることを要求すべき。
- **L2 (低)**: 分類器が領域の esize と役割を見ない。temporal で許すのは、esize 4 で float32 の out/rv 面に限るべき (`diffs` にある `esize` と、領域の位置や role を使う)。
- **L3 (低)**: REPORT の「nv_depth の 208,737,760 語は x86 で全件一致」の出どころが違う (上の 3)。
- **L4 (低)**: x86_exe_pair の比較結果 (`--report` の出力) を証拠に残していない (上の 6)。
- **L5 (情報)**: MAX_ULP = 8 が観測した最大値そのもので、余裕がない (上の 6)。
- **L6 (低)**: `--require-groups` の `export` は、ラベル「?」の呼び出しで満たされる。
  - 自分の quick の記録 (`--export` なし) にも、「?」の呼び出しが 10 件あった (fallback の子プロセスや prof5 経由とみられる)。
  - そのため、`--export` を付けた実行で書き出しの呼び出しが記録されていなくても、export 群の要求は通り得る。0 件の再生の修正の目的からみて、詰めが甘い。

## 0 件の再生の修正 (作業者の 2 回目の修正) の確認 — 合格
- 観測 (`x86_bridge_replay.py` で直接確認):
  - 空の cap と存在しない cap は、どちらも「FAIL: no capture」・rc 2。
  - 群の不足 (`--only nvc_noise_curve --require-groups curve,frame`) は「FAIL: no recorded call of the group(s) frame」。
- 観測 (`verify_x86_bridge.sh` で確認): `--quick --only frame --no-capture --allow-known` を空の作業ディレクトリで実行すると、「FAIL: no capture」・rc 1。
- 統括が先に実行した `rerun_quick_strict.log` (0/0 で ALL IDENTICAL、exit 0) は空の比較で、修正前のスクリプトによるもの。厳密モードの確認として数えてはいけない。

## 再実行・作業の記録
- 重い実行は 1 本ずつ行った。
  - 記録: `verify_bridge.py --quick` の全群 (530 秒、記録 3.5 GB は削除済み)。
  - 番兵の再生: 107 秒。x86 の再生: 148 秒。
  - スクラッチでのクリーンビルド。
- 誤って並行して起動した 2 本目の記録は、途中で止めて消した (`repro/out/` には触れていない)。
- 本家のバンドルは実行していない。新しいソフトウェアは入れていない。画面は出していない。
- 残したファイル (`sandbox/omo/coord_fix2c/independent/`):
  - `guard_replay.py`・`guard_replay.log` (番兵の再生)
  - `known_probe.py` (分類器の境界)
  - `expf_blocks.c` (expf と 0×inf)
  - `fakepy.sh` (M1 の再現用)
  - `replay_x86_quick.json` (自分の x86 再生)
  - `capture_quick_tail.log`

---

# Round 2 (2026-10-31、M1・L1〜L6 の修正後) — 判定: **APPROVE**

対象は作業者 fix2-c の 3 回目の修正。変わったファイルは `native/verify_x86_bridge.sh`・`x86_bridge_replay.py`・`x86_bridge_known.py`・`x86_bridge_cap.py` で、証拠は `x86_g9/fix_m1/`・`x86_g9/exe_pair/`。
`native/Makefile` と `.gitignore` の差分は Round 1 と同じ (Makefile の増減 23 行)。新しいファイルはすべて `py_compile` と `bash -n` を通った。
検証用のファイルは `independent/` の `r2_*`・`known_probe2.*` に置いた。重い実行は 1 本ずつ行い、作業ディレクトリは scratchpad に作った (`coord_fix2c/work*` には触れていない)。

## M1 (fail-open) — 解消
- 静的に確認した点:
  - 再生の前に、その回の `replay_*.json`・`dump_x86`・`dump_arm64` を消している。
  - 再生の終了コードは 0 一致 / 3 差あり / 2 記録なし・0 件・群の不足に分かれた。Python の未捕捉の例外は 1、シグナルで落ちれば 128+ になる。
  - 分類は rc が 3 で、かつその回の `replay_x86.json` が空でないときだけ行う。arm64 と portable の再生は 0 以外なら失敗。分類用の arm64 の `--select` 再生が 0 以外なら失敗。
- 動的に確認した点: 自分で記録した `--quick --only frame --keep` の作業ディレクトリを使った。
  - (A) 本物の Python: x86 は 62/93 一致で、temporal 31 件・186 面・435,256 語が分類された。rc 0。
  - そのうえで、前回の JSON とダンプが残った状態で `--no-capture --only frame --allow-known --py86 <壊れた x86 の Python>` を 5 通り試した (`independent/r2_breakage.log`):

    | 壊し方 | 結果 |
    |---|---|
    | `fakepy.sh` (traceback を出して rc 1、Round 1 で M1 を再現したもの) | 分類されず「1 REPLAYS FAILED」・**rc 1** |
    | `r2_py_midcrash.sh` (本物の再生を 20 秒後に SIGSEGV) | Segmentation fault → **rc 1** |
    | `r2_py_corrupt.sh` (本物の再生の後に JSON を半分に切って rc 3 で返す) | JSONDecodeError → **rc 1** |
    | `r2_py_rc3_nojson.sh` (JSON を書かずに rc 3) | 分類されず **rc 1** |
    | `r2_py_rc3_nodump.sh` (本物の再生の後に dump_x86 を消して rc 3) | 31 件すべて「dump missing」で分類外 → **rc 1** |

  - (C) 同じディレクトリで本物の Python に戻すと rc 0。(D) 厳密判定のまま `--no-capture` は rc 1。
- 残る前提 (情報): x86 の Python 自体が rc 0 を偽って返す場合は検出できない。これは再生そのものを信頼する前提で、欠陥ではない。

## L1・L2 (分類器) — 解消
`independent/known_probe2.py` で 24 件を試し、24/24 が期待どおりだった (`known_probe2.log`)。
- 合格になるもの: temporal の引数 16 (out) と 17 (rv) の 1 ULP、FLT_MAX とその 1 ULP 下、nlc_image の引数 4 の NaN の符号違い。
- 不合格になるもの:
  - ±inf と ±FLT_MAX、+inf と −inf、9 ULP、大きさの不一致、同一のバッファ
  - flow_out (引数 14)、uint8 の new_flow (引数 15。esize を 4 と偽っても不合格)
  - 入力の planes (引数 6)、int の par_i (引数 9)
  - role が io のもの、esize 8 のもの、`arg` の無い古い JSON の形、arg が None のもの
  - nlc_image の入力 m (引数 1) と引数 0、spatial、temporal_mask
- 引数番号は、再生が記録の `args` から機械的に引いている。領域が別の引数と共有されている場合は、最初に現れた引数の番号が付くので、分類外 (安全側) になる。
- 実際の実行 (A・C) では、186 面すべてが引数 16 と 17 として分類された。

## L6 (export の判定) — 解消
- 自分で `--quick --only curve --export --allow-known --keep` を実行した。rc 0 で、x86 の差は export の temporal 8 件、すべて既知の分類だった (`independent/r2_export.log`)。
- 記録をプロセスごとに数えると、次のとおり。
  - verify_bridge 側の 3 プロセスは curve と aft だけ。
  - export 側の 16 プロセスは export だけ。中身は `_load` の abi_version 16、`_temporal_v5` 8、`_spatial_v5` 24。temporal の 8 = 4 フレーム × 2 本で、NV_NATIVE=1 の書き出しに当たる。
  - 両者のプロセスは重ならず、unlabelled は 0。
- `--export` なしの記録 (A) には export も unlabelled も 0 件で、export 以外の呼び出しは紛れ込んでいない。
- 群があるかどうかを数えるとき、nvc_abi_version は数えない (fallback だけは数える)。そのため、書き出しが native を呼ばなければ export 群の要求は失敗する。

## L3・L4・L5 — 解消
- L3: 書き直しは正確。18,751 呼び出しの内訳 (1,296 + 3,029 + 13,829 + 487 + 110) と 216,163,431 語は、Round 1 で `summary_x86.md` から計算した値と一致する。「Python との 208,737,760 語の比較が x86 でも成り立つのは、この一致を経由して」という書き方も正しい。
- L4: `exe_pair/report_*.md` は REPORT の主張と合っている。
  - spatial: 668,088 語のうち 667,858 語が NaN。有限値の差 230 語は exp の 133 + 97 で、すべて exp のケース。
  - temporal: NaN 24,187 語、有限値の差 160,083 語 (= 127,602 + 32,474 + 7)。片側だけの NaN は 0。
  - aft: finish 835 語・sim 66 語がすべて両側 NaN。
  - profile 6/6 (verify_profile_pair.log の ok は 39 件)。LZMA 3/3・667/667 バイト。
  - なお spatial では、入力の面が有限の「finite input」のケースにも NaN のビット違いがある (19,107 語と 29,515 語)。レポートの注のとおり、プロファイルや引数が inf を含むためで、主張とは矛盾しない。
- L5: REPORT と分類器の docstring に「余裕なし」と記録された。

## Round 2 の判定理由
Round 1 の必須の M1 は、壊し方 5 通りすべてで失敗になることを確かめた。L1・L2・L6 は境界の試験と自分の実行で、L3・L4・L5 は証拠との照合で解消を確かめた。
新しい不具合は見つからなかった。arm64 の既定ビルドは Round 1 から変わっていない (Makefile の差分は同じ)。以上から **APPROVE** とする。
