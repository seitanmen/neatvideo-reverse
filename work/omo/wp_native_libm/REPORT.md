# W2a: libm 感度ハーネス — 作業報告 (2026-10-29)

作業場所は worktree `/Users/seitan/Desktop/work/reverse/.claude/worktrees/w2a` (ブランチ `w2a-libm`、fea7964 から) です。仕様は `.claude/tasks/native_w2_libm.md` の W2a です。
「観測」は実行結果、「推定」は推論です。数値はすべて `sandbox/omo/wp_native_libm/runs/` の実測です。

## 0. 要点

- **C++ 移植が呼ぶ libm の関数は 5 種類あります。** タスク文の 4 種類に expf が加わります。
  - pow (nv_curve・nv_aft)
  - tanf (nv_linekernel)
  - atan2 と __sincos_stret (nv_model)
  - expf (nv_ndimage の np_expf)。使うのは nv_temporal.cpp:326 の v5 類似度 `exp(-e/thr)` だけです。
  - floor (nv_ndimage) は定義上厳密な関数なので、入れ替えていません。
  - quality・band・lzma の各バイナリは libm の関数を参照しません (`nm -u` で観測)。
- **alt1 (openlibm) と alt2 (musl) で既定と違う出力が出たのは、expf を通る経路だけです (観測)。**
  - 該当したのは、v5 の時間段 (temporal_v5 / process_frame_v5) の画素値と rv、それに spatial スイートの `np.exp` 部品試験です。
  - pow・tanf・atan2・sincos は、関数単体では 1 ULP の差があります (§5)。
  - そのうち部品の出力語まで届いたのは、musl の pow による合成補間器の 2 値 (curve の interp、2 ULP) だけでした。
  - 曲線・領域探索・1 面モデル・プロファイル・AFT・Quality/Completeness・FindFlatArea の領域と状態は、alt1・alt2 で全語一致しました。
- **処理済みフレームの差は小さいものでした。** 対象は UI の backend.process_frame で、test_v02 11 枚と test.mp4 7 枚です。
  - float32 の語が変わった割合は、1080p の Y で alt1 が 6.5%、alt2 が 0.8% です。
  - 差は最大 6 ULP で、0〜255 の尺度で 6.1e-5 です。
  - 8 ビットの書き出し符号が変わった画素は 36,633,600 画素中、alt1 で 26 (Y 21・Cb 2・Cr 3)、alt2 で 2 (Y のみ) です。いずれも ±1 でした。
- **alt3 (素朴な式) は、pow の差が曲線と AFT の色重みまで届きました。**
  - 曲線の ys は実フレームでも相対 6e-16 程度変わります。
  - FindFlatArea の領域・状態とプロファイルは変わりませんでした。
  - Quality % は変化なしでした。Completeness % は合成の AFT 後処理入力 2 件で 1% 下がりました。
- **既定ビルドは変更前とバイト単位で同一です。** 9 バイナリと libnvcore.dylib の SHA256 が一致しました。`make -C native verify JOBS=2` は変更後も 11 本すべて一致しました。

## 1. 変更・新規ファイル (worktree 内)

### 変更したファイル (既定ビルドの出力は不変、§3)
- **`native/nv_curve.cpp`・`nv_aft.cpp`・`nv_linekernel.cpp`・`nv_model.cpp`・`nv_ndimage.cpp`**
  - `#include "nv_libm.h"` を 1 行加えました。
  - volatile 関数ポインタの初期化子を `&::pow` → `NV_LIBM(pow, pow)` のように置き換えました (計 6 か所)。
- **`native/Makefile`**: 冒頭の使い方に 2 行足し、`libm-alt`・`libm-alt-clean` ターゲットを加えました。既定のルールは変えていません。
- **`.gitignore`**: 同梱ソースと W2a のソースだけを git に見せる 16 行を足しました。bin・obj・libm_ulp は除外のままです。

### 新規ファイル
- **`native/nv_libm.h`**: マクロ `NV_LIBM(sys, alt)` です。既定は `&::sys`、`-DNV_LIBM_ALT` のときだけ `&nvlibm::alt` になります。
- **`native/libm_alt.h`・`native/libm_alt.cpp`**: 実行時の切り替えと alt3 の実装です。
  - `NV_LIBM_ALT=0|1|2|3` で libm を選びます。
  - `NV_LIBM_ALT_FUNCS=pow,tanf,atan2,sincos,expf` で一部の関数だけを入れ替えます。
  - `NV_LIBM_LOG=dir` で、実際に渡された引数を記録します。
- **`native/Makefile.libm_alt`**: alt ビルドです。
  - ソースとフラグは既定と同じで、`-DNV_LIBM_ALT` を加えています。
  - 出力は `native/libm_alt/bin/` に既定と同じ名前で置きます (9 バイナリ + libnvcore.dylib)。ほかに `native/libm_alt/libm_ulp` を作ります。
- **`native/libm_alt/` の中身**
  - `openlibm_rename.h`・`musl_rename.h`: 公開名を `nv_ol_*`・`nv_mu_*` に変えます。
  - `openlibm.exp`・`musl.exp`: `ld -r` で外部に残す名前の一覧です。
  - `musl_shim/{features.h,endian.h}`: musl の内部マクロと `<endian.h>` の代用です。W2a で書いたものです。
  - `libm_ulp.cpp`: 関数単体の ULP 比較と、線カーネルのオフセット検査です。
  - `pyhook/sitecustomize.py`: verify スクリプトを書き換えずに、native 呼び出しを捕捉・差し替えるフックです。
  - `fields.py`: 各 driver の出力書式を読んで、語をフィールド名に対応させ、最終出力への影響を集計します。
  - `frames_e2e.py`: UI バックエンド経由で処理済みフレームを作って比べます。
- **`native/verify_libm_alt.py`**: ハーネス本体です。
- **`native/openlibm/`** (24 ファイル + SOURCE.md) と **`native/musl/`** (30 ファイル + SOURCE.md): 同梱ソースです。
- **`sandbox/omo/wp_native_libm/`**
  - `baseline/`: 変更前後のバイナリの SHA256、クリーンビルドのログ、変更前・変更後の verify ログです。
  - `runs/`: 全記録です。`summary.md`・`summary.json`・`impact.json`・`ulp/ulp.json`・`frames_e2e.json` と各段のログがあります。捕捉した入出力を含めて 15 GB です。
  - `dl/`: tarball と展開物です。信頼しない入力として扱い、中のビルドスクリプトは実行していません。
  - `tools/waitpid.py`: kqueue でプロセスの終了を待つ補助です。

AGENTS.md・MIGRATION.md・スナップショット・repro・処理コアの Python は触っていません。本家バイナリは読んでも動かしてもいません。`repro/out/` にも書いていません。

## 2. 同梱ソース (詳細は各 SOURCE.md)

各 SOURCE.md には、ファイルごとの SHA256 とライセンス全文を載せてあります。

| alt | 版 | 取得元 | tarball の SHA256 | 照合 |
|---|---|---|---|---|
| 1 openlibm | 0.8.8 (タグ v0.8.8、2026-09-02) | github.com/JuliaMath/openlibm/archive/refs/tags/v0.8.8.tar.gz | `721bab18…435d17` | Homebrew の formula の値と一致 |
| 2 musl | 1.2.6 (2026-03-20) | musl.libc.org/releases/musl-1.2.6.tar.gz | `d585fd3b…460512a` | Alpine aports の SHA512 と一致。PGP 署名は gpg が無いため**未検証** |

- **改変なし**: 同梱したファイルは展開物と SHA256 が一致します。必要な .c だけを自前の Makefile でコンパイルし、上流の configure や Makefile は実行していません。
- **記号の隔離**
  - 公開名は強制 include で改名し、`ld -r -exported_symbols_list` でそれ以外の記号をすべて局所化しました。
  - `nm -g` で外部に見えるのは、各ライブラリ 7 関数ずつです。
  - システム libm から参照するのは fabs・floor・scalbn・sqrt だけで、いずれも厳密な関数です。
- **警告**: 上流由来の警告が 2 件あり、名前を指定して抑止しました。openlibm の e_pow.c:134 (`-Wsign-compare`) と musl の libm.h:175 (`-Wunused-but-set-variable`) です。クリーンビルドの警告は既定・alt ともに 0 件です。
- **フラグ**
  - 上流の `-std=c99 -fno-builtin -fno-strict-aliasing` を使います。musl には `-frounding-math`、openlibm には `-D__BSD_VISIBLE -DASSEMBLER` を加えます。
  - これに規約の `-O2 -ffp-contract=off -fno-fast-math` を足しました。
  - clang は arm64 で `__FP_FAST_FMA` を定義するので、musl の pow は `__builtin_fma` の経路を通ります。aarch64 向けの musl ビルドと同じ経路です。
- **musl の CVE**: release ページは 1.2.6 までに CVE-2026-40200 と CVE-2026-6042 を挙げています。どちらも `src/math` の外の話で、同梱したファイルは対象外です。

### alt3 の実装について
arm64 の macOS では long double が double と同じ 53 ビット仮数なので、「long double 経由」の実装はできません。代わりに、double で教科書どおりの式を素朴に組みました。
- pow(x, y) = exp(y·log x)。特殊値だけ別に処理します。
- atan2(y, x) = atan(y/x) に象限補正を加えます。π は double の定数 1 個です。
- sin と cos は、π/2 を double の定数 1 個で引く単純な縮約と、テイラー級数で計算します。
- tanf は (float)(sin/cos)、expf は (float)exp で、double で計算してから float に 1 回丸めます。

誤差の上限の目安として使うもので、libm の候補ではありません。

## 3. 既定ビルドが変わっていない証拠 (観測)

- **ビルドの決定性**: `make -B all` を 2 回実行し、10 ファイルの SHA256 が同一でした (`baseline/bin_sha256_before.txt`)。
- **変更後のバイナリ**: ソース変更後の `make -B all` で、10 ファイルとも変更前と SHA256 が一致しました (`baseline/bin_sha256_after.txt`)。
- **変更後の verify**: `make -C native verify JOBS=2` は 11 分 45 秒で、全件一致でした (`baseline/make_verify_after.log`)。

  | スイート | 結果 |
  |---|---|
  | quality | 4000/4000 |
  | band | 5706/5706 |
  | curve | ALL BIT-EXACT |
  | lzma | 667/667 |
  | flat | ALL BIT-EXACT |
  | flat_cancel | 676/676 |
  | model | 1244/1244 (+ ub 28/28) |
  | temporal | 474/474 |
  | spatial | 1287/1287 |
  | profile | 39/39 PASS |
  | aft | ALL BIT-EXACT |

- **配管の確認**: alt ビルドを NV_LIBM_ALT=0 で動かすと、システム libm を libm_alt 経由で呼ぶことになります。この設定では、全スイートの全呼び出しと verify_bridge で既定と全語一致しました。

## 4. 実行方法と所要時間

```
make -C native libm-alt && python3 native/verify_libm_alt.py [--jobs 2] [--stages ...] [--suites ...] [--alts 0,1,2,3] [--budget S]
```

段の流れは次のとおりです (既定はすべて実行)。
1. **capture**: 11 本の verify を既定バイナリのまま実行し、native 呼び出しの入出力・NV_* 環境・stdout を保存します。各スクリプトは既定どおり合格します。
2. **replay**: 保存した各呼び出しを alt ビルドで NV_LIBM_ALT=0〜3 で再実行し、語単位で比べます。中断・再開でき、`--budget` で時間を区切れます。
3. **swap**: 差が出た (スイート, alt) の組だけ、verify スクリプトを alt ビルドで実行し、スクリプト自身の件数を取ります。
4. **bridge**: verify_bridge を `NV_NATIVE_LIB=libm_alt/bin/libnvcore.dylib` で実行します。
5. **frames → ulp → impact → report**: フレーム比較、関数単体の比較、フィールドへの対応付け、まとめの出力です。

所要時間 (M2 Max、負荷平均 4〜8、--jobs 2、観測):

| 段 | 所要時間 |
|---|---|
| capture | 計約 1,400 秒 (model は Python 側のキャッシュが効いて 60 秒。初回は約 520 秒) |
| replay | 計約 1,350 秒 |
| swap | 640 秒 |
| bridge | 1 alt あたり 410〜440 秒 (alt2 は他ジョブと重なって 1,305 秒) |
| frames / ulp / impact | 41 秒 / 72 秒 / 約 100 秒 |
| 通し | 約 1.5〜2 時間 |

ビルドは約 11 秒で、ディスクは約 15 GB 使います。
600 秒を超える処理 (profile・aft・bridge・make verify) は `nohup` で起動し、`tools/waitpid.py` (kqueue の NOTE_EXIT による待機で、ポーリングなし) で終了を待ちました。

## 5. 関数単体の差

実際に呼ばれた引数で、システム libm と比べた結果です (`runs/ulp/ulp.json`)。全スイートと verify_bridge で、NV_LIBM_ALT=0 のときに記録した「異なる引数」を数えています。

| 関数 | 呼び出し / 異なる引数 | 引数の範囲 | alt1 | alt2 | alt3 |
|---|---|---|---|---|---|
| pow | 611,863 / 20,466 | x は合成で ±inf を含む、y は 0〜4 の整数 | 1 件 (1 ULP) | 3 件 (1 ULP) | 8,940 件 (最大 160 ULP、相対 2.4e-14) |
| pow (実フレームのみ、bridge) | 20,320 / 160 | x ∈ [0, 245] | 0 | 0 | 79 件 (最大 9 ULP) |
| tanf | 130,320 / **8 (固定)** | [0.245, π/4] | 2 件 (1 ULP) | 2 件 (1 ULP) | 0 |
| atan2 | 9,684 / **860 (固定)** | 整数オフセット (dy, dx) | 126 件 (1 ULP) | 126 件 (1 ULP) | 218 件 (1 ULP) |
| sincos | 277,632 / **26 (固定)** | [−0.196, 3.04] | sin 2 件 (1 ULP)、cos 0 | sin 2 件、cos 0 | sin 9 件 (8 ULP)、cos 12 件 (5 ULP) |
| expf | 871,358,722 / 31,816,011 | 実フレームでは [−569, −0] | 3,180,165 件 (10.0%、1 ULP) | 215,973 件 (0.68%) | 215,081 件 (0.68%) |

- **全数掃引 (観測)**
  - tanf を記録範囲 [0x1.f5b76p-3, π/4] のすべての float (13,513,772 個) で調べると、alt1・alt2 は 12.4%、alt3 は 1.29% が 1 ULP 違いました。
  - expf をすべての float (約 42.8 億個) で調べると、1 ULP 違う割合は alt1 が 0.46%、alt2 と alt3 が 0.048% でした。差はすべて 1 ULP 以内です。
- **sincos の整合**: 各ライブラリの sincos は、そのライブラリの sin・cos と 26 引数すべてで一致しました。システムの `__sincos_stret` も、システムの sin・cos と一致しました。
- **macOS の expf (推定)**: alt3 の expf (double の exp を 1 回丸めたもので、ほぼ正しく丸めた値) と musl が、ほぼ同じ件数だけシステムと違います。ここから、macOS の expf はこの範囲の約 0.7% で、正しく丸めた値と 1 ULP 違うと推定します。openlibm の expf (FreeBSD 由来) はさらに不正確です。
- **tanf・atan2・sincos の引数は、入力に依存しない固定の集合です。** 方向表と半径だけで決まり、どのスイートでも同じ 8 / 860 / 26 個でした (観測)。
  - tanf: ポートは線カーネルのオフセットを frintn(f32(t·i) + 0.26f) で作ります。i < 512 (カーネル辺の上限) のすべての i について、alt1〜3 の全引数で調べ、オフセットは 1 つも変わりませんでした。
  - atan2 と sincos: 方向ビンの割り当てに使われます。1 面モデル 1244 件とプロファイル 39 件が全語一致したので、使われた (半径, ビン数) の組では割り当ては不変と推定します。割り当て表そのものは出力して比べていません。

## 6. スイート × alt の表

- 「呼び出し」は native バイナリを起動した回数、「語」は出力ファイルの 4 バイト単位の語です。
- 出力が既定とバイト一致した組は、スクリプトの結果も既定と同じ (全件一致) になるので、swap は実行していません。
- alt0 は全スイートで差 0 です。
- 表では alt1 列に既定の件数を書きました。alt2・alt3 の「差 0」も同じ件数で全件一致です。

| スイート (呼び出し, 語) | alt1 openlibm | alt2 musl | alt3 素朴 |
|---|---|---|---|
| quality (1, 9,000) | 差 0 (4000/4000) | 差 0 | 差 0 |
| band (1, 14,528,736) | 差 0 (5706/5706) | 差 0 | 差 0 |
| lzma (3, 7,346,885) | 差 0 (667/667) | 差 0 | 差 0 |
| curve (5, 891,183) | 差 0 | 2 語 (0.0002%) | 7,839 語 (0.88%) |
| flat (32, 48,546,444) | 差 0 (362/362) | 差 0 | 352 語 |
| flat_cancel (462, 2,249,278) | 差 0 (676/676) | 差 0 | 24 語 |
| model (2, 5,096,752) | 差 0 (1244/1244) | 差 0 | 差 0 |
| temporal (1, 111,408,158) | 1,626,579 語 (1.46%) | 161,410 語 (0.14%) | 160,548 語 (0.14%) |
| spatial (1, 142,638,112) | 3,574 語 | 233 語 | 232 語 |
| profile (6, 603,307) | 差 0 (39/39) | 差 0 | 差 0 (39/39) |
| aft (6, 47,556,656) | 差 0 | 差 0 | 569 語 |

差が出た組の内訳です。

- **curve, alt2**
  - 差は `interp.values` の 2/20,554 値だけです (合成の補間器 1 表、最大 2 ULP、相対 2.5e-16)。
  - スクリプトの件数: curve 1094/1094、clip 4000/4000、**interp 20552/20554**、tile 25/25。
- **curve, alt3**
  - `curve.ys` が 804/8,752 値 (242 曲線、相対 3.7e-13 以下)、`interp.values` が 6,930/20,554 値 (巨大値の合成入力では overflow を含む) で変わりました。
  - スクリプトの件数: **curve 852/1094**、clip 4000/4000、**interp 13624/20554**、tile 25/25。
- **flat, alt3**
  - 差は `flat.curve.ys` の 352/2,304 値だけです (154 実行、相対 1.5e-13 以下)。
  - 領域・状態・候補・スコア・trace・縮小面は全語一致しました。
  - スクリプトの件数: **flat 208/362** (不一致はすべて曲線欄)。
- **flat_cancel, alt3**
  - 差は flat 呼び出しの曲線欄 24 語 (12 実行、6 ULP 以下) です。
  - スクリプトは領域・状態・問い合わせ回数だけを比べるので、cancel 676/676 で一致です。
- **temporal, alt1**
  - `temporal_v5.out`: Y 1.5%、Cb/Cr 1.1〜1.3% (5 ULP 以下、絶対差 7.6e-5 以下)。
  - `rv`: 7 ULP 以下。
  - `process_frame_v5.out`: Y 3.8% (6 ULP 以下)。
  - flow の面と、部品試験 (blocksum・dense・flow・pyramid・refine・warp) は差 0 です。
  - スクリプトの件数: **434/474** (rand frame 14/16、rand temporal 31/48、real frame 3/6、real temporal 18/36、comp はすべて一致)。
- **temporal, alt2 と alt3**: 5 ULP 以下の差で、スクリプトの件数はどちらも **436/474** です。
- **spatial**
  - 差は `np.exp` 部品の試験だけです (alt1 で 3,574/74,728 値、1 ULP)。spatial_v5 本体 (real 30/30・rand 60/60) とほかの部品は差 0 です。
  - スクリプトの件数: alt1 **1266/1287** (comp exp 39/60)、alt2・alt3 **1274/1287** (comp exp 47/60)。
- **aft, alt3**
  - AFT 本体の出力 67 件のうち 4 件 (test.mp4 n=750 と合成 3 件) で、`aft.prof.curve[c].level` 8 値が 1 ULP 変わりました。
  - `combine.cw` は 370/621 値 (9 ULP 以下、相対 1e-15 以下) が変わりました。
  - 合成の後処理 `finish` では `prof.curve.level` が 191/81,000 値変わり、うち 7 値は 16 ULP を超えます。確認した 2 値は、クランプの下限 0.2 と上限 3.0 の間で反転していました。
  - shuffle と sim は差 0 です。
  - スクリプトの件数: shuffle 4097/4097、sim 3000/3000、**finish 2827/3000、aft 63/67 (-j 1・-j 2 とも)、combine 5/69**。

差分語の位置とフィールド名の対応は次のとおりです。
- 語番号は `runs/capture/<suite>/calls/<call>/diff_alt<a>_<arg>.npz` にあります (idx = 語番号、a = 既定の語、b = alt の語)。先頭 8 個は `replay.jsonl` の `first` にもあります。
- フィールド名への対応は `fields.py` が行います。各 `nv_*_main.cpp` の書式どおりに解釈しています。
- プロファイルは 0xc300 バイトを ptr / lzma / mode / model[面].{levels, dc, A, R, B, C, ad319c, ad1ae0} / tile_dev / mcstats / means / curve[ch].{level, scale, valid, flags} / lzma_ok / area に分けています。

## 7. verify_bridge (libnvcore.dylib を alt ビルドに差し替え)

ログは `runs/bridge/alt*.log` です。

| 群 | alt0 | alt1 | alt2 | alt3 |
|---|---|---|---|---|
| curve | 20/20 | 20/20 | 20/20 | **10/20** |
| profile | 9/9 | 9/9 | 9/9 | 9/9 |
| aft | 26/26 (999,732 語) | 26/26 | 26/26 | **18/26** |
| frame | 72/72 (306,334,381 語) | **12/72** (語の差 4.5%) | **12/72** (0.45%) | **12/72** (0.45%) |
| cancel | 23/23 | 23/23 | 23/23 | 23/23 |
| fallback | 9/9 | 9/9 | 9/9 | 9/9 |

- **alt3 の curve**: 違ったのは noise_curve の ys で、1〜2 語、相対 6e-16 程度です。find_flat_area の領域・状態は 10/10 で一致しました。
- **alt3 の aft**: 999,698/999,732 語が一致しました。違ったのは run_aft の `aft_color_weights` と `aft_face_curves`、それに t n=302 の AFT blob 1 語です。
- **frame で一致した 12 件**: pyramid 類似度 (expf を使わない) と temporal 0 の変種でした (観測)。
- **fallback の注意**: 「default (library found)」の 1 件は NV_NATIVE_LIB を外すので、既定の dylib を使います。この 1 件は alt の比較になっていません。

## 8. 差の伝播と最終出力

### 関数 → 部品 → 最終出力 (観測)

| 関数 | alt1・alt2 での行き先 | alt3 での行き先 |
|---|---|---|
| tanf (固定 8 角、1 ULP) | 線カーネルが不変なので、どこにも届かない | 同じ (alt3 は関数単体でも差 0) |
| atan2 / sincos (固定、1 ULP) | 1 面モデル・プロファイルは全語一致で、届かない | 同じ (最大 8 ULP の差でもモデルは全語一致) |
| pow (1 ULP、データに依存) | 実フレームでは差なし。合成の補間器 2 値 (musl) だけ | 曲線の ys (相対 1e-13 以下)、flat の曲線欄、AFT の色重み・面曲線・ビンのレベル 1 ULP、合成の後処理でクランプの反転。**領域・状態・候補・スコアには届かない** |
| expf (1 ULP、画素ごと) | 時間段の重み → out と rv → 空間段 → 処理済みフレーム | 同じ (件数は musl とほぼ同じ) |

### 最終出力ごとの結果
- **FindFlatArea の領域と状態**: flat 362 実行、flat_cancel 462 実行、bridge 10 件で、alt1〜3 とも変化は 0 件です。
- **プロファイル blob (0xc300 バイト)**: profile スイート 39 件と bridge 9 件で、alt1〜3 とも全バイト一致しました。
- **AFT 後の曲線**: alt1・alt2 は全語一致です。alt3 はビンのレベル 8 値が 1 ULP、色重み (cw) が相対 1e-15 以下で変わりました。
- **Quality / Completeness の % 表示**
  - 本家形式の blob から計算した場合 (`nv_ui_qt/quality.py` の quality_from_blob と genuine_g):
    - alt1・alt2 は blob が同一なので、変化は 0 件です。
    - alt3 は、Quality を計算できる (0xbf34 ≠ 0 の) blob 1,037 個で、Quality % の変化は 0 件、Completeness % の変化は **2 件** でした (合成の後処理入力で、59→58% と 60→59%)。
    - 原因 (観測): 多項式で埋めたビンのレベルが 0.2 (0x3e4ccccd) から 3.0 (0x40400000) に反転しました。その結果、前のビンで「次のビンの語 0 の下位バイト ≠ 0 なら +1/81」の加点が消えました。
  - UI の表示 (repro の写像 `_curve_blob`):
    - 入力は、領域、領域の帯域測度 (libm を使わない)、IsClip、平均、aft_bin_flags (combine の marker/clip/gt3) です。
    - alt1〜3 とも領域は変わらず、combine の marker/clip/gt3 は 69/69 件で全バイト一致しました。
    - このため UI の % も変わらないと推定します。UI の % そのものは計算していません。
- **処理済みフレーム** (frames 段): 条件は UI のバックエンドの process_frame、UI の既定パラメータ、AFT 済みプロファイルです。対象は test_v02 n=308〜318 の 1080p 11 枚と、test.mp4 n=300〜306 の 7 枚で、合計 36,633,600 画素です。

| | alt1 openlibm | alt2 musl | alt3 素朴 |
|---|---|---|---|
| float32 語の差 (1080p の Y / Cb / Cr) | 6.53% / 4.80% / 3.89% | 0.79% / 0.63% / 0.49% | 0.79% / 0.62% / 0.49% |
| float32 語の差 (640×360 の Y / Cb / Cr) | 1.60% / 1.34% / 1.23% | 0.32% / 0.29% / 0.27% | 0.31% / 0.29% / 0.28% |
| 最大の差 | 6 ULP (6.1e-5、0〜255 尺度) | 6 ULP | 6 ULP |
| 8 ビット符号が変わった画素 | **26** (v02 の Y 21・Cb 2・Cr 3。Y は 11 枚中 10 枚で発生)。すべて ±1 | **2** (v02 の Y) | **2** (v02 の Y) |
| 10 ビット (×4) で変わった画素 | 100 | 10 | 11 |
| 16 ビット (×256) で変わった画素 | 6,634 | 768 | 770 |

  temporal スイートでも傾向は同じで、8 ビット符号の変化は alt1 で Y 3・Cb 1 (30.7M 画素中)、alt2・alt3 で Y 1 でした。

## 9. 未解決と注意

- **Windows UCRT は未測定です。** 代替 libm は UCRT ではないので、UCRT の pow/tanf/atan2/sincos/expf の誤差分布はわかっていません。今回の結果は「1 ULP 級の差がどこまで届くか」を見る代理です。
- **Python 側はシステム libm のままです。** そのため、alt ビルドの不一致は「macOS の Python との不一致」という意味です。numpy の float32 の exp が libm の expf と一致することは、MIGRATION.md に記録済みです。
- **arm64 だけで実施しました。** x86_64 の alt ビルドは作っていません。
- **musl の PGP 署名は未検証です。** gpg が無いためです。SHA512 は Alpine の値と一致しています。
- **方向ビンの割り当て表は直接比べていません。** atan2・sincos の割り当てが不変というのは、出力の一致からの推定です。
- **大きな ULP 値は合成入力によるものです。** ULP は IEEE 順の整数差で数えています。巨大値や overflow を含む合成入力では、alt3 の差が inf ULP や巨大な相対差になります。curve の interp と flat の curve.ys に出た大きな ULP はこの種の入力です。
- **capture の判定方法**: 呼び出しの引数のうち実在するファイルを入力、呼び出しで作られたか変わったファイルを出力とみなしています。alt0 の再実行が全語一致したので、出力の取り違えはありません。
- **runs/ は 15 GB あります。** impact と report をやり直すには capture の入出力が要るので残しました。`runs/capture/*/calls` を消せば 1 GB 未満になりますが、replay・swap・impact はやり直せなくなります。
