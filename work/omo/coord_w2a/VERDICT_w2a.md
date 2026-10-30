# W2a (libm 感度ハーネス) 独立検証 — VERDICT

検証者: verify-w2a (作業者 w2a-libm とは別エージェント)。日付: 2026-10-29。
作業場所: worktree `/Users/seitan/Desktop/work/reverse/.claude/worktrees/w2a`。コードは変更していない。
自分のスクリプトとログは `sandbox/omo/coord_w2a/independent/` (番号付きログ `01_…`〜`12_…`、`replay_one.py`、
`finish_quality.py`、`frames_indep.py`)。本家バイナリ・repro・処理コア・`repro/out/` には触れていない。
「観測」= 自分の実行結果、「推定」= 推論。作業者の数値は `sandbox/omo/wp_native_libm/REPORT.md` と `runs/` のもの。

## 判定: **APPROVE**

W2a の成果物 (alt ビルド、同梱ソース、ハーネス、REPORT の表) は、抜き取りで再現した範囲で作業者の主張と一致し、
既定ビルドは変更前とバイト単位で同一である。W2b/W2c の判定材料としては十分 (所見は §8)。重大な指摘はない。
軽微な指摘 (§7) は判定を変えない。

---

## 1. 既定ビルドが不変であること (観測)

| 検査 | 結果 |
|---|---|
| `make -C native clean && make -C native JOBS=2` (クリーンビルド、警告 0) → 9 バイナリ + libnvcore.dylib の SHA256 | **10/10 が `baseline/bin_sha256_before.txt` と一致** (`01_bin_sha256_indep.txt`) |
| 追加: W2a の変更を含まない `git archive HEAD native` をスクラッチ領域でビルド (`01b_pristine_HEAD_sha256.txt`) | **10/10 が上と一致** → 作業者の「変更前」基準値は、自分で作った変更前バイナリとも一致 |
| 既定ビルドでの verify (libm を使う 8 本、`--jobs 2`) | curve ALL BIT-EXACT (1094/4000/20554/25)、model 1244/1244 (+ub 28/28)、temporal 474/474、aft ALL BIT-EXACT (4097/3000/3000/67/67/69)、spatial 1287/1287、profile 39/39 PASS、flat 362/362、flat_cancel 676/676 (`02_verify_default_A.log`・`_B.log`) |
| libm を使わない 3 本 (quality・band・lzma) | quality 4000/4000、band 5706/5706、lzma 667/667 (`02_verify_default_C.log`) → **11 本すべて全件一致** |

`nv_libm.h` のマクロは `-DNV_LIBM_ALT` がないとき `(&::sys)` に展開されるので、変更後のソースから変更前と同一のオブジェクトが出るのは
自然であり、ハッシュ一致はそれを裏付ける。

## 2. 同梱ソースの出典と改変の有無 (観測、`03_source_provenance.log`)

- tarball の実ハッシュ: openlibm `721bab18…435d17`、musl SHA256 `d585fd3b…460512a` / SHA512 `1adad96e…732555c` — いずれも SOURCE.md の記載と一致。
- tarball を自分で新しいディレクトリに展開し (中のスクリプトは実行していない)、同梱ファイルを `cmp`: **openlibm 25/25、musl 31/31 が同名ファイルとバイト一致**
  (SOURCE.md 以外の全ファイル。LICENSE.md・COPYRIGHT を含む)。
- SOURCE.md の SHA256 表 (25 行・31 行) は実ファイルのハッシュと全行一致。
- ライセンス全文: `native/openlibm/LICENSE.md` (5,857 バイト)・`native/musl/COPYRIGHT` (6,196 バイト) が同梱され、各 SOURCE.md にも全文が転記されている。
- 改名ヘッダ (`openlibm_rename.h`・`musl_rename.h`) と shim (`musl_shim/features.h`・`endian.h`) は W2a 作成物で、`#define` と属性マクロだけ。上流ソースへの編集はない。
- Homebrew formula / Alpine aports との照合、musl の PGP 署名は**私も未検証** (この機械に gpg がない。作業者の記載どおり)。

## 3. alt ビルドの正しさ (観測、`04_libm_alt_*.log`・`12_gitignore_log_demo.log`)

- `make -C native libm-alt-clean && make -C native libm-alt`: 成功、警告 0、約 29 秒。`libm_alt/bin/` に 9 バイナリ + libnvcore.dylib、`libm_alt/libm_ulp`。
- 記号の隔離: `nm -g` で `openlibm.o`・`musl.o` の外部定義はそれぞれ `nv_ol_*`・`nv_mu_*` の 7 個のみ。未定義は openlibm が
  `fabs floor scalbn sqrt` (+ stack_chk)、musl が `fabs floor scalbn` (+ stack_chk)。alt の dylib は `nv_ol_/nv_mu_` 記号 16 個を含み、
  既定の dylib は 0 個。既定 dylib の libm import は `__sincos_stret atan2 expf floor pow tanf` の 6 個 (REPORT の「5 種 + floor」と一致)。
- 代替関数が実際に呼ばれること:
  - `NV_LIBM_LOG` を付けて alt ビルドの `nv_curve_test interp` を実行 → `libm_args_<pid>.bin` が生成され、`libm_ulp args` で pow 19,886 個の引数
    (呼び出し 36,356 回) を読める。同じ引数集合で alt1/2/3 の結果が異なる (bitdiff 1 / 3 / 8,675)。
  - 同じ入力を既定バイナリに `NV_LIBM_ALT=2` で与えると既定出力と一致 (環境変数は既定ビルドに効かない)、alt ビルド alt2 では 2 語異なる。
  - `NV_LIBM_ALT_FUNCS` の部分入れ替え: curve interp 呼び出し (23807_4) で alt2 は 2 語差、`NV_LIBM_ALT_FUNCS=tanf,atan2,sincos,expf` (pow 以外) では **0 語**、
    `NV_LIBM_ALT_FUNCS=pow` では同じ 2 語 (語番号 25888・25898) → 部分入れ替えが効き、差が pow 由来であることも確定 (`05_replay_…log`)。
- 自作の replay (`replay_one.py`、作業者の replay コードは使っていない) で alt0 は 0 語差 (配管の確認)。

## 4. 主張の再現 (抜き取り、観測)

| 主張 | 自分の実行 | 結果 |
|---|---|---|
| (a) alt1・alt2 でプロファイル・AFT・FindFlatArea の領域と状態が 1 語も変わらない | profile 34992_5 (486,759 語)・34992_6 (49,924 語)、aft 38631_4 (852,989 語)・38631_6 combine (8,022 語)・38631_3 finish (37,449,000 語)、model 32883_1 (5,096,668 語)、flat 6 呼び出し (最大 9,906,178 語)、flat_cancel 3 呼び出し、いずれも alt1・alt2 | **すべて 0 語差** (`05_…log`・`06_…log`) |
| (a') Quality/Completeness % が alt1・alt2 で不変 | blob が同一なので自明 (上の 0 語差) | 成立 |
| (b) 処理済みフレームの 8 ビット値の差 (作業者: alt1 26 画素・alt2 2 画素、±1) | 作業者が使っていないフレーム test_v02 n=200・500、test.mp4 n=100・700 (各 ±2 の窓を自分でデコード、AFT プロファイルは bridge キャッシュのもの、UI 既定値) を自作 `frames_indep.py` で処理 (`08_frames_indep.log`) | 6,912,000 画素中、**alt1 8 画素 (すべて ±1、1080p のみ)、alt2 0、alt3 0**。最大 6 ULP (6.1e-5)。float32 語の変化率は 1080p の Y で alt1 9.7〜9.9%、alt2 1.1% (作業者の 11 枚平均 6.5% / 0.8% より高めだが同じ桁。フレーム依存と推定)。10 ビットは alt1 36・alt2 8、16 ビットは alt1 1,848・alt2 190 |
| (c) alt3 で Completeness % が合成 2 件で 1% 下がる | finish 38631_3 を alt3 で再実行し、自作 `finish_quality.py` (finish.out を自分で切り分け、`repro/nv_ui_qt/quality.py` の `quality_from_blob`・`genuine_g` を直接呼ぶ) で既定と比較 | 3,000 ケース中 0xbf34≠0 の 903 blob で **Quality % 変化 0、Completeness % 変化 2 (ケース 668: 59→58、853: 60→59)** — 作業者の例と同じケース・同じ値 (`07_finish_quality_alt3.log`)。語差 191 も REPORT の「191/81,000」と一致 |
| (d) musl の pow で curve の補間器 2 値が 2 ULP 違う | 上の §3 の replay | 2 語 (25888・25898)、alt1 は 0 語。作業者のスイート結果 (swap: interp 20552/20554) は `10_swap_…log` (§9) |
| 追加: alt3 の aft 本体 4 語 (38631_4)・combine 370 語・curve 23807_1 443 語・flat_cancel 27559_300 の 2 語 (語 27・29 = 曲線 ys 欄、領域・状態の語 0〜4 は不変) | replay | REPORT・summary の内訳と整合 |
| 追加: 作業者のスクリプトによる swap (temporal alt1、curve alt2) | `10_swap_temporal_alt1_curve_alt2.log` | §9 |

## 5. 関数単体の ULP 計測の妥当性 (評価)

- **引数の範囲**: `libm_ulp args` は、alt0 の replay と bridge で `NV_LIBM_LOG` が記録した「実際に渡された引数」の和集合を使う (`ulp.json` の per_suite で確認)。
  tanf 8 個・atan2 860 個・sincos 26 個は全スイートで同じ固定集合 (方向表と半径で決まる) なので網羅的。pow は 20,466 個 (合成の ±inf を含む)、
  実フレーム (bridge) は 160 個。expf は 31.8M 個の異なる引数 (実フレームは [−569, −0])、さらに全 float の掃引もある。
  自分で model+aft のログに `libm_ulp` を掛けて同じ数 (tanf 8・atan2 860・sincos 26) と同じ bitdiff を得た (`09_ulp_indep.log`)。
- **expf が主因という主張**: 根拠は (1) alt1・alt2 で差が出た部品が expf を通る temporal_v5 / process_frame_v5 と spatial の `np.exp` 部品だけ、
  (2) `NV_LIBM_ALT_FUNCS` で関数ごとに切り分けられること、(3) expf の呼び出しが 8.7 億回で引数の 10% (alt1) / 0.7% (alt2) が 1 ULP 違うこと。
  私の replay でも pow 以外を alt にした curve は 0 語差で、frames の差の性質 (±1 の 8 ビット、≤6 ULP) も一致。妥当。
- **pow/tanf/atan2/sincos が出力に届かない理由の示し方**:
  - tanf: 唯一の用途 (線カーネルのオフセット `frintn(f32(t·i) + 0.26f)`、i < 512) を `libm_ulp` が直接検査し、変化 0。これは構造的な根拠で妥当。
    ただし **`runs/ulp/ulp.json` にはこの `kernel_offsets_changed` 欄がない** (`libm_ulp.cpp` が ulp 段の実行後に更新されたと推定)。私の再実行では
    alt1〜3 とも `kernel_offsets_changed 0 / kernel_args_changed 0` (`09_ulp_indep.log`) なので REPORT の記述は正しいが、成果物が古い。
  - atan2/sincos: 方向ビンの割り当て表は直接比べておらず、1 面モデル 1,244 件・プロファイル 39 件の全語一致からの推定 (作業者も明記)。
    固定 860/26 引数での差が 1 ULP (alt3 でも ≤8 ULP) で、ビン境界に乗らなかったという経験的根拠。入力に依存しない引数集合なので、
    この環境で使う (半径, ビン数) の組については十分。
  - pow: alt1/alt2 の差は 20,466 引数中 1/3 個 (合成入力)、実フレームの 160 引数では 0 → 実フレームで届かないのは当然で、合成の補間器 2 値だけ届いた。
    alt3 (素朴 pow、最大 160 ULP) で曲線 ys・AFT のレベル・色重みまで届くが、領域・状態・プロファイル・Quality % には届かず、
    Completeness % のみ合成 2 件で 1% 変化、という「上限」が示されている。
- **限界** (作業者も記載): Windows UCRT 自体は測っていないので、本ハーネスは「1 ULP 級の libm 差がどこまで届くか」の代理。
  frames の集計は UI 既定パラメータのみ (bridge の変種 noise+/noise− 等は語単位で比べているが 8 ビット符号の集計はない)。

## 6. REPORT の記述と実際の差 (観測)

一致を確認した項目: 同梱ファイル数 (24+SOURCE / 30+SOURCE)、tarball ハッシュ、`nm -g` の 7 関数、システム libm 参照 (fabs/floor/scalbn/sqrt)、
クリーンビルド警告 0、既定 verify 11 本の件数、`make verify` 11 分 45 秒 (`make_verify_after.log`)、§5 の呼び出し回数 (pow 611,863・expf 871,358,722 など)、
§6 の語数、§7 の bridge 表 (`runs/bridge/alt*.json`)、§8 の frames 表 (36,633,600 画素、alt1 26 = Y21+Cb2+Cr3、alt2 2)、swap の件数
(curve alt2 20552/20554、temporal 434/436/436、spatial 1266/1274/1274、flat alt3 208/362 (=154 不一致)、aft alt3 63/67)。

差・注意点:
1. `ulp.json` に `kernel_offsets_changed` がない (上記 §5)。軽微 (再実行で確認済み)。
2. frames の float32 変化率はフレーム依存で、私の 4 枚では alt1 の Y が 9.7〜9.9% (REPORT 6.5%)。8 ビットの結論 (±1、ごく少数) は同じ。
3. `Makefile.libm_alt` の冒頭コメントはシステム libm の参照に `rint` を挙げるが、実際の未定義記号にはない (openlibm は sqrt を含む)。文書上の些細な差。
4. `runs/` 15 GB と `dl/` 16 MB は `.gitignore` の `sandbox/omo/wp_*/**` で除外され (`*.py`・`*.md` だけ可視)、`native/libm_alt/{bin,obj,libm_ulp}` も除外されている
   (`git check-ignore` で確認)。コミット対象に巨大物が混ざる恐れはない。

## 7. 指摘 (重大度つき)

| # | 重大度 | 内容 | 対応 |
|---|---|---|---|
| 1 | 低 | `runs/ulp/ulp.json` が `libm_ulp` の最終版の出力ではない (線カーネルのオフセット検査の欄がない) | 任意: `python3 native/verify_libm_alt.py --stages ulp` で再生成 (72 秒)。私の再実行結果は `09_ulp_indep.log` |
| 2 | 低 | `Makefile.libm_alt` コメントの `rint` | 任意: 文言修正 |
| 3 | 情報 | musl の PGP 署名は未検証 (gpg なし)。SHA256/SHA512 は上流の値と一致 | W2b で musl を採る場合に検証を推奨 (openlibm を採る場合は不要) |
| 4 | 情報 | 方向ビン割り当て表の直接比較はない (出力一致からの推定) | W2b/W2c の判定には影響しない (固定引数集合で全語一致) |

必須の修正: なし。

## 8. W2b/W2c の判定材料として十分か (所見)

十分と判断する。理由:
- 「どの語が動くか」が関数ごと・部品ごとに切り分けられている (pow→曲線/AFT のみ、expf→時間段の画素値のみ、tanf/atan2/sincos→届かない)。
- 最終出力への影響が数値で出ている: alt1/alt2 (実在する 2 つの libm) で、プロファイル blob・領域・状態・Quality/Completeness % は 0 件の変化、
  処理済み画素は 8 ビットで 1e-6 以下の割合で ±1 (私の追加 4 枚でも同じ性質)。alt3 (素朴実装) でも領域・プロファイル・Quality % は不変で、
  Completeness % が合成入力 2 件で 1% 動くのが上限。
- 欠けているもの (判定時に意識すべき点): UCRT そのものの計測はない (Windows runbook 側の結果待ち)。bridge 変種の 8 ビット集計はない。
  W2b を選ぶなら、gold の移行手順 (F4 方式) と、expf を含めて 5 関数すべてをベンダー libm にするかの決定が別途必要
  (expf は openlibm より musl の方がシステム libm に近い: 全 float の 1 ULP 差率 0.46% vs 0.048%)。

## 9. 作業者のスクリプトによる swap の再現 (観測、`10_swap_temporal_alt1_curve_alt2.log`)

作業者のフック (`libm_alt/pyhook`) を swap モードで使い、verify スクリプト自身の件数を取った:
- `verify_temporal.py --jobs 2` を alt1 (openlibm) で: **434/474** (rand frame 14/16、rand temporal 31/48、real frame 3/6、real temporal 18/36、comp 6 群はすべて一致)
  — REPORT §6 の「434/474」と内訳まで一致。
- `verify_curve.py --jobs 2` を alt2 (musl) で: curve 1094/1094、clip 4000/4000、**interp 20552/20554**、tile 25/25 — REPORT と一致。
