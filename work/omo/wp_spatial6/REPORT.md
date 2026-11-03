# wp_spatial6 報告 — 本家 FUN_0098edb0 (面ごとの空間フィルタ) の C++ 移植

作業日 2026-11-03、worktree `.claude/worktrees/sp6` (ブランチ v6-spatial、main 8c14386 から)。コミットなし。
**観測** = 実行・逆アセンブル (arm64、`otool -tV`) で確かめたこと、**推定** = それ以外。
`work/` 以下は `.gitignore` の対象 (git status には出ない)。

> 筆記: 作業者 (sp6-worker) は harness の規則によりこのファイルを作成できなかったため、作業者が SendMessage で送った本文を統括が転記した (2026-11-03)。統括の確認・独立検証の結果は §7 に追記する。

## 1. 実装したもの

- `native/nv_spatial6.{h,cpp}` (名前空間 `nvs6`)
  - 葉 (照合対象の 9): recip FUN_009ed5f8・mul 9edc38・sqr 9ed2f0・sub 9ed710・sqrtk 9ed4e0・rstep 9ef260・robust 9ef0d0・scale 9ee304・clampmul 9ee7bc
  - 本体が使うその他の葉: add 9ed844・sqrtabs 9ed3e0・rinv 9eef5c・affine 9ee3fc・acc 9edeb8・blend 9ee1a4・bladd 9ed978・ratio 9ee04c・ramp 9ee93c・wmul 9eebf8・pairw 9efb1c・argmax 9ee5fc (best = 0.0 から始め、x ≥ best で更新するので同点は後の方向が選ばれ、NaN は選ばれない。2026-11-03 に注釈を訂正、L4)・fabs 9ee50c (2026-11-03 追加、L6)・gather 98ed34・sel2 98eba8・strip_box3/strip_rms3 (9ed014/9ed164 の 4 列帯)・down2 9f15b4
  - `plane_filter` (= FUN_0098edb0): 分解・白色化・縮小核 FUN_00990f74 の高速経路・方向経路 (組 1〜3、B 族、dc、cfg+0x28/+0x3c/+0x78)・cfg+0x90 の後段・量の 2 経路・段の切り出し (FUN_0099c694)・再構成
  - `Guard`: 改ざん検知の状態語に依存する 2 値を引数にしたもの (dir_popcount = 方向経路末尾の w25 の popcount、cfg_popcount = FUN_0098edb0 の [sp+0xbc] の元の popcount。どちらも既定 16 = 本家の静的初期値での値)
  - `Recorder`: 葉の呼び出しの記録 (再生照合用)
- `native/nv_spatial6_main.cpp`: CLI `nv_spatial6_test` (葉のサブコマンドは nvh と同じ引数形、`plane` は 1 面の処理)
- `native/Makefile`: 新ターゲット `nv_spatial6_test`・`nv_spatial6_mut` (MUT=1..9)。`all`・`verify`・既存ターゲットは変更なし
- `reference/genuine/analysis/harness/nvh.c`: `elem` に robust・ratio・bladd・ramp4・wmul4 (case 41)、サブコマンド `argmax` (FUN_009ee5fc)・`gather` (FUN_0098ed34) を追加。変更前の写しは `tools/nvh.c.before`
- `work/omo/wp_spatial6/tools/`: leafcheck.py・verify_leaves.py・compare_python.py・replay.py。ログは `logs/`

nvh に追加した関数はすべて葉 (観測、`tools/leafcheck.py`): 関数外への分岐 0、ページ参照は 0x14a67a8 (NEON 判定バイト) だけ (FUN_0098ed34 はページ参照なし)、スタックガードなし。
引数の並びは逆アセンブルで確定 (例: FUN_009ef0d0(s0=t, x0=a, x1=e, x2=out, x3=n)、FUN_009ee5fc(x0=maps, x1=idx, x2=max, x3=n, x4=stride, x5=count))。
`nvh selftest` は終了コード 3。NV_BUNDLE はこの worktree のバンドルに設定 (既定のパス `neatvideo_ofx_614/…` は存在しない)。本家のアサーションは一度も出ていない。

## 2. 葉の照合 (本家とビット一致、NaN のビット列まで)

`tools/verify_leaves.py --big 8 --small 500`、`logs/verify_leaves.log` (726 秒)。
- 葉ごとに 508 回 (n = 2000〜120000 を 8 回、n = 1〜40 を 500 回。端数経路は後者で出る)
- 入力: 正規乱数に ±0・±inf・qNaN/sNaN (両符号、複数ペイロード)・非正規化数・最大値・1 付近を 15% 混入。スカラーも 12% を特殊値

| 葉 | 要素数 | 不一致 |
|---|---|---|
| recip | 567,283 | 0 |
| mul | 671,518 | 0 |
| sqr | 545,539 | 0 |
| sub | 281,265 | 0 |
| sqrtk | 613,357 | 0 |
| rstep | 572,805 | 0 |
| robust | 286,272 | 0 |
| scale | 435,761 | 0 |
| clampmul | 431,636 | 0 |

その他も不一致 0: add 685k / sqrtabs 561k / rinv 626k / affine 506k / acc 672k / blend 612k / bladd 512k / ratio 451k / ramp4 391k / wmul4 457k / pairw 421k / argmax 666k / gather 153k / sel2 157k / strip3 4.49M (250 面) / down2 1.26M。合計不一致 0 語。

**感度試験** (`logs/verify_leaves_mutants.log`、`logs/mutant8_targeted.log`)
- 9 種中 8 種は乱数照合で検出 (1〜224 回目)
- 2026-11-03 の再実行 (M1・L1〜L6 の修正後、seed 9、葉ごとに n = 2000〜120000 の 3 呼び出し + n = 1〜40 の 150 呼び出し、`logs/verify_leaves_r2.log`): 26 種 (fabs を追加) すべて不一致 0。9 葉の要素数は recip 274,965 / mul 174,332 / sqr 117,367 / sub 190,644 / sqrtk 191,204 / rstep 205,037 / robust 169,289 / scale 125,684 / clampmul 78,356、fabs 237,741、strip3 75 面 942,787、down2 75 面 422,698。verify_leaves.py は各呼び出しの前に作業ディレクトリを空にし、前の呼び出しの出力を比べてしまうことがないようにした (L5)
- 変異 8 (scale の端数を `s·x` にする) は乱数 400 回で未検出だが**等価な変更ではない**。スカラーが NaN かつ端数の要素が別ペイロードの NaN のときだけ差が出る。狙った入力 (s = 0xffd54321、x[5] = 0x7fc12345) で本家と移植は 0x7fc12345、変異は 0xffd54321 → 検出

**途中で見つけた誤り** (観測): bladd の NEON 部の FMLA の乗数の順が逆で、B と c がともに NaN の要素で 4 語ずれた。0x9ed9b4 `fmla v5(d), v2(c), v4(B)` に合わせて修正。repro/nv_model.py と nv_model.cpp も同じ順だが c = 1.0 なので結果は変わらない (既存コードは変更なし)。

## 3. 帯域ループ・方向経路の構造

### 3.1 確定したこと (観測)

**入口と早退** (FUN_0098edb0 の引数: x1 = P、x2 = OUT、x3 = σ マップ、x4/x5 = w/h、x6 = ROI、x7 = cfg、[x29+0x10] = model、[x29+0x18] = var 出力)
- 早退: var 出力が NULL・b8 ≤ 0 で、全段の amount (cfg+0x14+4k) と cfg+0x90+4k がともに 0 なら入力をコピーして終わる (0x98ee70〜0x98ef0c)
- 分解 (0x98f184〜0x98f368): 3×3 箱 = FUN_009ed014 の 4 列帯 (0x98f3a4)。W < 4 か H < 2 なら分離型 0x99ba54 → 0x99bf40 (0x98f264/0x98f280)。続いて FUN_009f15b4 (0x98f2c8)、FUN_00990da8 (0x98f314)
- 白色化 (切り出しあり 0x98f81c〜0x98f8a4 は行ごと、なし 0x98fad8〜0x98fb14 は面全体): FUN_009ed5f8(s0 = 0x38d1b717, x0 = σ マップ + 段・行のオフセット, x1 = inv, x2 = n) → FUN_009edc38(帯域, inv, W, n)。**σ は第 4 引数の σ マップ (FUN_009aae90 の出力) から読む。モデル語ではない**
- B 作業面: f984 (= [x22+0x284]、0x98f7b0) > 0 なら FUN_009ee3fc(−f984, f984, B) (0x98fb18〜0x98fb34)
- 縮小核: FUN_00990f74(core, W, dst = inv バッファ, P4 = inv, w, h, k, cfg, model, …) (0x98fb38〜0x98fb8c)。dst と P4 は同じバッファ
- 累積: FUN_009edeb8(1.0, dst, σ, ACC, ACC) (0x98fc4c 行ごと / 0x98fc88)。k > 0 なら FUN_00993df0 で拡大 (0x98fce8)
- 出力: 最後に FUN_009ed844(P, ACC, OUT) (0x99036c)
- 0x98fc8c〜0x98fca8 と 0x9900c0〜0x9902a0 (sub・sqr・FUN_009944c0×4・sqrtabs・ratio・拡大・scale・bladd) は var 出力 ([sp+0x130]) が非 NULL のときだけ通る (0x98fc90 / 0x9900c8 の cbz)。OUT には影響しないので未移植

**組の表**: x22 = 0x1243700 + 組·0x260 (0x990ff4〜0x991004、組 = cfg+0xc8)。組のデータは S = x22 + 0x280 (0x991af8)。Python の SET_TABLE (0x1243980) と Agents.md の DAT_01243988 (= S+8、角度) は同じ表。静的データ (__TEXT __const):

| 欄 (S 基準 / x22 基準) | 意味 | 根拠 | 組 0/1/2/3/4 |
|---|---|---|---|
| S+0 / +0x280 (バイト) | 0 = rstep、1 = 頑健化 | 0x991bbc〜0x991bc8 (ldrb w25; cbz → 0x991c24 bl 0x9ef260) | 0/0/0/1/1 |
| S+4 / +0x284 | f984 | 0x9916bc, 0x992020, 0x98f7b0 | 1/1/1/0.5/0.5 |
| S+8 / +0x288 | 方向の角度 | 0x991354 → FUN_00990448 | なし/4/8/16/16 |
| S+0xc8 / +0x348 (+8d) | 2 本目のカーネルの番号 (−1 = なし) | 0x991c4c (cmn x27,#1)、0x991c7c (madd x0, x27, #0x3830) | —/−1×4/−1×4,0,1,1,0/0,0,0,4,4,4,8,8,8,8,8,12,12,12,0,0 |
| S+0x148 / +0x3c8 (+8d) | 2 本目の半径 | 0x991c98 | —/1/1/2,2,1,2,2,2,1,2,2,2,1,2,2,2,1,2 |
| S+0x1c8 / +0x448 | rA | 0x991648 | 0/4/4/4/4 |
| S+0x1d0 / +0x450 | E の平滑の箱の半径 | 0x991cdc | 0/1/1/1/1 |
| S+0x1e8 / +0x468 | B 族の半径 | 0x991450 | 0/0/0/6/6 |
| S+0x208 / +0x488 | C 族の半径 | 0x991454 | 0/0/0/0/12 |
| S+0x258 / +0x4d8 | 方向数 | 0x99105c | 0/4/8/16/16 |
| 0x12438d0[組] | 段の切り出しの余白 | FUN_0099c694 の引数 | 2/4/4/8/32 |
| 0x12438f8[組] | dc の半径 | 0x993a1c | 0/0/4/4/4 |
| 0x1243920[組] (バイト) | dc の箱を 2 回 | 0x993adc | 0/0/0/1/1 |
| 0x1243978[組] (バイト) | パターン保護 | 0x98ef30 | 0/0/0/0/1 |

0x1243700〜0x124397f の値 (0.99, 1.1, 1.05, …) は組 0 のデータではない。

**縮小核 (FUN_00990f74)**
- 基本量: R = FUN_009950f4(1.0, …, W, r = 1, mode = 1) (0x991578〜0x9915a4)、s13 = cfg[k] + 1e-6 (0x991638)、t = s13·(|model[5+k]|·1.1) (0x99167c〜0x99168c)
- 方向経路の条件 (0x99163c〜0x991670): 方向数 ≠ 0 かつ model 語 80+80·組+16k ≠ −1e6 (順序なしも ≠ 扱い) かつ 2·rA < w かつ 2·rA < h
- 0x9915c0〜0x991630 のハッシュブロックは幅 ≥ 513 のときだけ通る。中身は大域への書き込みと w23 の算出で、w23 は記帳 (0x9916ec・0x991ae4) にしか使わない。0x1487aac のバイトはこのブロックに入るかどうかだけを決める
- 高速経路: FUN_009eebf8(t, 2t, 1, 0, R, W, X) (0x9916a4)、f984 > 0 なら B = 1.0 (0x9916a8〜0x9916dc)、x20 = 0 (0x9916e0)
- 量 (0x991748〜0x9917c0): b8 > 0 かつ amt ≠ 1 なら適応経路 FUN_009ee7bc(b8·(1−amt), −1, −1, −amt, x = [sp+0x118] = P4 = inv, y = X, out = dst)。そうでなければ FUN_009ee304((float)(int64)(1 − x20)·(−amt), X, dst)
- cfg+0x90+4k ≠ 0 なら後段 (0x991860〜0x99260c)
- 方向経路 (0x991aac〜0x993d18):
  - Wc = FUN_009eef5c(4.5t, W, R) (s0 = 4.5t を渡し、逆数は葉の中)
  - 方向ごと: FUN_009944c0(kernel_d, Wc, tmp, rA, 1) (0x991ba4〜0x991bb8。引数は kernel, src, dst, r, mode)
  - S+0 で分岐: rstep (0x991c34) または頑健化 (差 → 局所 RMS → FUN_009ef0d0(0.75t)、0x991bcc〜0x991c1c)。続いて 2 本目のカーネル
  - 組 3・4 は 2 乗 → 箱 2 回 → FUN_009ed4e0(1/|model[80+80·組+16k+d]|) (0x991e70〜0x991ea0)。組 1・2 は FUN_009950f4(1/|A|) (0x991f28〜)
  - argmax FUN_009ee5fc (0x992000)。f984 > 0 なら ramp → sel2、そうでなければ FUN_0098ed34 (0x992680)
  - 末尾 0x993be0〜0x993cd8: w20 = popcount(w25) − 16 (`sub w20, w8, #0x10`。32 ビットの結果で、x20 にはゼロ拡張された値が入る)。w25 は 0x9926a0〜0x9926c0 で大域の状態語から読む値。0x9916ec に戻ってから、量の乗数は 0x9917a4〜0x9917ac (`mov w8, #1; sub x8, x8, x20; scvtf s1, x8`) で (float)(int64)(1 − x20)。popcount ≥ 16 なら 1 − (popcount − 16)、popcount < 16 なら 1 − (2³² − (16 − popcount)) (2026-11-03 訂正: 初版の C++ は符号付き 1 − (int)(popcount − 16) で、popcount < 16 のとき本家と違っていた。L1)
  - x19 (0x993ce0) は組 4 のパターン保護のときだけ非 0 (0x993944 で 0、0x9939f8 で設定)
- FUN_0098edb0 の popcount 判定 (2026-11-03 訂正、独立検証の M1): [sp+0xbc] = popcount(w25) − 0x1f0 は段のループの外の 0x98f590〜0x98f718 で 1 回だけ作る。0x98f77c〜0x98f788 (`ldr w8, [sp,#0xbc]; cmn w8, #0x1e0; b.eq; str wzr, [x22, x11, lsl #2]`) は段のループの先頭 0x98f774 の中にあり (x22 = cfg = [sp+0x128]、x11 = 段 k。戻りの分岐は 0x98f754 と 0x9902c4 の b.gt)、popcount ≠ 16 なら**全段 k = L−1…0 について、その段の処理の前に cfg[k] (cfg+4k、+0x00 の σ_cfg) に 0 を書く**。書き先は呼び出し側の cfg (ctx+0x3d10 / +0x3de0 / +0x3eb0 の面ごとの cfg) なので、同じ cfg を使う後の呼び出しにも 0 が残る。初版の「cfg[L−1] だけに 0」は誤り

### 3.2 推定
- 各段・各方向で「どのバッファをどの呼び出しに渡すか」の全体は、前任の Python 移植の読みと本作業の抜き取り確認による。本体 (FUN_0098edb0 / FUN_00990f74) は本家で呼んでいない
- 正規のホストで popcount = 16 (x20 = 0、cfg を変えない) というのは推定。`nvs6::Guard` の 2 つの popcount (`dir_popcount` = 方向経路の w25、`cfg_popcount` = FUN_0098edb0 の w25、既定はどちらも 16) で任意の値で動く。`cfg_popcount ≠ 16` のときは全段の cfg[k] を段ごとに 0 にし、`plane_filter` の引数 `cfg_after` に本家が書き換えた後の cfg を返す (入力の cfg は変えない。呼び出し側の cfg が書き換わることはこの引数で表す)
- σ マップ (FUN_009aae90)、cfg (FUN_0099ead0)、組の選択表 0x1246c48 は移植の外 (入力)。前任の読みでは既定 YCrCb の組は品質 0 で (2,1,1)、品質 1 で (3,2,2)。「組 3 = Y の本番設定」は品質 1 のときだけ

### 3.3 仕様書 (`.claude/tasks/native_v6_spatial.md`) と PHASE_B_PROGRESS.md の矛盾の判定
- op (+0x348) = −1 スキップ / 0 rstep / 1 頑健化、および「何もしない」: 誤り。+0x348+8d は 2 本目のカーネルの番号、+0x3c8+8d はその半径。rstep か頑健化かは S+0 のバイトで選ぶ。「何もしない」経路はない
- w25 = 組+0x450: 誤り。w25 は x22+0x280 のバイト。+0x450 は平滑の半径
- 表の基底 0x1243700 / 0x1243988: 同じ表。仕様 §5 の「組 0 の帯域量」は表の前にある別のデータ
- +0x284 が方向経路または適応経路のゲート: どちらでもない (f984)。適応経路の条件は cfg+0xb8 > 0 かつ amt ≠ 1、方向経路の条件は §3.1 の 4 つ
- FUN_009ba54 / 9bf40: 実在するのは 0x99ba54 / 0x99bf40 で、分離型の箱 (行と列)。コピー・合成ではない
- 仕様 §8「難読化は出力に影響しない」: 根拠にしていない。方向経路の量の乗数 (x20) と、全段の cfg[k] の 0 書きが状態語に依存する
- 白色化の σ はモデル語: 誤り。σ マップから読む。PROGRESS の「x1 = σ のソース」は誤読で、x1 は出力
- 0x1487aac の旗がゲート: 幅 ≥ 513 のハッシュブロックに入るかどうかだけを決める
- 方向経路を通ると量 = 0: 誤り。乗数は (float)(int64)(1 − x20) (x20 は 32 ビットの popcount − 16 のゼロ拡張) で、popcount = 16 なら 1
- 適応経路の x = W: 誤り。x = inv = 1/(σ+1e-4)
- FUN_009944c0 の引数 (out, src, kernel, …): 誤り。(kernel, src, dst, r, mode)
- rinv の s0 = 1/(4.5t): s0 = 4.5t を渡す
- 「X を B に書き戻す」: 実際は ACC += dst·σ、拡大、最後に OUT = P + ACC
- 依頼文の「Python 移植は通常経路のみ」: 事実と違う。`wp_spatial/src/nv_spatial.py` は組 1〜3 の方向経路・B 族・後段・切り出しまで移植済み (組 4 は NotImplementedError)。同ファイルは ../base (存在しない) から読むので、reference/python を sys.path に入れて使った
- bladd の Python 移植 (nv_model.blend_add) は c90 が NaN のときだけ本家と違い得る (FMLA の乗数の順)

## 4. 帯域ループ・方向経路の照合

### 4.1 Python 移植との比較 (`tools/compare_python.py`)
- 入力: test_v02 (n=100/312/576)・test.mp4 (n=0/300/450) の Y・Cr・Cb の切り出し 48〜263 px、2 割は合成の面 (段差 + 傾斜 + 雑音)。model は同じ面の 128² の切り出しに native/nv_model_test。σ は正の合成値 (1、1+0.2·雑音、定数 0.3〜3)。cfg は nv_spatial.build_cfg (品質 0/1、段数 3〜5、Amount、適応 b8、cfg+0x90 の後段と箱の回数、cfg+0x28/3c/78)。25〜40% は組 3 に強制、35% は ROI で段を切り出し
- 結果:
  - `logs/compare_python.log`: 100 件中、実装範囲の 98 件が全語一致 (1,943,161 語、方向経路を通った段 328)。2 件は両方 NOT_IMPLEMENTED
  - `logs/compare_python_rec.log` (再生用): 23/23 一致、1 件は両方 NOT_IMPLEMENTED
  - `logs/compare_python_portable.log` (-DNV_PORTABLE、arm64): 37/37 一致、3 件は両方 NOT_IMPLEMENTED
  - NOT_IMPLEMENTED は幅 52〜54・段数 5 (拡大の幅 < 2)
- 2026-11-03 の再実行 (M1・L1〜L6 の修正後、既定の Guard): `logs/compare_python_r2.log` 100 件中、実装範囲の 94 件が全語一致 (1,823,799 語、方向経路の段 323)、6 件は両方 NOT_IMPLEMENTED。再生用 `logs/compare_python_rec2.log` 24/24 一致
- Guard を既定以外にした比較 (seed 303、20 件。Python 移植は popcount = 16 の仮定のままなので、差は C++ 側だけゲートの値を変えたためのもの):
  - `--popcfg 15` (全段の cfg[k] = 0。M1 の修正後の読み): Python の参照も全段の σ_cfg を 0 にした cfg で動かすと **20/20 全語一致** (`logs/compare_python_guard_popcfg15_mirror.log`) → 0 にする段の範囲 (全段) と時点 (各段の処理の前) が Python の参照と合う。既定の cfg の Python と比べると 20 件中 18 件が不一致 (`logs/compare_python_guard_popcfg15.log`)。一致した 2 件は Amount が全段 0・b8 = 0・cfg+0x90 の後段なしで、出力が σ_cfg (t) に依存しない。初版の --zero (最粗段だけ 0) で一致していた case 1・17 は cfg+0x90 の後段がある件で、後段は X (各段の t に依存) を使うため、全段を 0 にすると不一致になる
  - `--popdir 17` (乗数 0) と `--popdir 15` (乗数 1 − (2³² − 1) = −4294967294) は、どちらも 20 件中 13 件が不一致 (`logs/compare_python_guard_popdir17.log` / `popdir15.log`)。一致した 7 件のうち 6 件は Amount が全段 0 (乗数が効かない)、case 19 は b8 = 0.353・amt = 0.4 で適応経路 clampmul を通り、x20 を使わない。不一致の 13 件は、方向経路の段が非 0 の Amount で scale 経路を通るもの。popdir の値そのものは Python に対応する参照がなく、乗数の式は逆アセンブルの読み (§3.1) による
  - 初版の `--x20 1` / `--zero` のログは、ゲートの意味を誤っていたので削除した (上の popdir17 は初版の x20 = 1 と同じ乗数 0 で、数字も同じ 13/20)

### 4.2 本家の葉での再生 (`tools/replay.py`、`logs/replay.log`)
- 方法: 上の 24 件で C++ が呼んだ葉をすべて記録し、記録した入力で本家の葉を nvh で呼んで出力をビット比較
- 結果: **29,997 呼び出しで不一致 0**
- 内訳: linefilt 13,088 / sub 2,669 / strip3-box 2,135 / sqr 1,806 / add 1,722 / sqrtk 1,664 / strip3-sq 1,091 / robust 985 / acc 972 / mul 844 / affine 805 / recip 739 / ramp4 329 / wmul4 216 / sqrtabs 142 / rstep 136 / argmax 125 / down2 96 / rinv 87 / sel2 75 / scale 71 / gather 50 / ratio 45 / pairw 36 / bladd 33 / clampmul 20 / blend 16 (計 29,997。初版の内訳には acc・add・affine が抜けていた。L2)。B 族 (gather) は 16 件で通過
- linefilt は nvh linefilt の %.9g テキスト出力との比較 (有限値は厳密、NaN は NaN 同士として比較)
- 再生していないもの: FUN_00993df0 (拡大)・FUN_00990da8 (帯域差)・B/I/A18 作業面のバイト移動。cfg+0x90 の後段の絶対値 (本家は葉 FUN_009ee50c を 0x992214・0x992264 で呼ぶ) は初版では C++ 内の符号ビット消去で、記録していなかった (L6)。2026-11-03 の修正で `nvs6::fabs` (FUN_009ee50c) として記録・再生の対象にした (下の再実行と §2 の照合に含む)。前 2 つは既存の移植 (nv_model.cpp の up2、nvb::band_from) の写しで、nvh pixnoise / nvh model 経由で本家と一致済みの部品
- 2026-11-03 追記 (統括の再実行で見つかった replay.py の不具合の修正):
  - 不具合: 257×257 の面 (統括の記録 case018) で `sel2 66049 0 66049` が `nvh sel2` の上限 (stride ≤ 65536) を超え、nvh が終了コード 1 → replay.py が RuntimeError で全体を中断していた。`gather` も同じ上限 (同じ case018 の `gather 66049 66049`)。
  - 修正 1: nvh の範囲外の呼び出しは「<種類> SKIPPED (理由)」、予期しない nvh の失敗は「<種類> ERROR」として数え、フォルダごとの行 (`… calls, … mismatching, … skipped`) とログの合計 (`TOTAL calls …, mismatching …, errors …, skipped …`) に出す。全体は止めない。終了コード 1 は不一致・ERROR・SKIP (SKIP は `--allow-skip` で 0 にできる) のとき (2026-11-03 の L3 で SKIP も 1 に変更)。n < 1 または stride < n の塊照合は ok にせず SKIP にする (L3)。上限は `limits()` に nvh.c の値 (elem/pairw n ≤ 4M、strip3/linefilt 辺 ≤ 512、strip3 W ≥ 4・H ≥ 2、down2 辺 2〜4096、argmax count ≤ 64・stride ≤ 4M) を写した。
  - 修正 2: stride > 65536 の sel2 / gather は、nvh を変えず 65536 要素ずつの塊で照合する (`sel2-chunked` / `gather-chunked`)。FUN_0098eba8 / FUN_0098ed34 の要素 i が読むのは a, b, i3, i4, best の i 番目と src / wt の j·stride + i 番目だけなので、塊 [a0, a1) は src / wt を 256 行 × (a1 − a0) に詰め直した入力で同じ値になる。塊の先頭は 4 の倍数で、最後の塊が元の呼び出しの端数 n & 3 を持つので、各要素が通る NEON / スカラーの式も元の呼び出しと同じ。nvh の sel2 の上限は変更していない。
  - 陰性対照: case018 の 2 呼び出し (sel2・gather、n = 66049) の記録出力の要素 65537 (2 つ目の塊) の 1 ビットを反転すると、2 件とも不一致として検出された。
  - 範囲外の確認: 作業者の 24 件 (`rec1`、29,997 呼び出し) には SKIP も塊の照合が必要な呼び出しもなかった (最大の面 140² = 19,600 要素)。統括の 30 件 (30,291 呼び出し) では、塊の照合が必要なのは case018 の sel2・gather 各 1 件だけで、SKIP はない。
  - 統括の記録 case018 の再生 (`logs/replay_lead_case018.log`): **1,525 呼び出しで不一致 0、ERROR 0、SKIP 0** (sel2-chunked 1・gather-chunked 1 を含む)。

- 2026-11-03 の再実行 (M1・L1〜L6 の修正後、既定の Guard): `compare_python.py --cases 24 --seed 808 --set3 0.4 --maxside 140 --rec` (24/24 が Python と全語一致、`logs/compare_python_rec2.log`) の記録を `replay.py --jobs 1` で再生 (`logs/replay_r2.log`): **20,683 呼び出しで不一致 0、ERROR 0、SKIP 0** (24 フォルダとも 0 / 0、終了コード 0)。内訳: linefilt 6,812 / acc 1,586 / mul 1,546 / affine 1,516 / recip 1,439 / sub 1,290 / strip3-box 1,171 / sqr 898 / add 835 / strip3-sq 805 / sqrtk 768 / robust 481 / rstep 320 / ramp4 270 / wmul4 194 / sqrtabs 130 / argmax 97 / rinv 92 / down2 85 / sel2 74 / scale 73 / fabs 48 (新規) / pairw 44 / ratio 42 / bladd 24 / gather 23 / clampmul 12 / blend 8
- 変更したのは新規ファイル (nv_spatial6.{h,cpp}・nv_spatial6_main.cpp) と tools だけで、native/ の既存ファイルは今回変えていないので、make -C native verify は再実行していない (前回 14 本合格)

## 5. 未確定・未実施と次の作業
- **統合照合** (仕様 §6.4、本家の FUN_0098edb0 / FUN_00990f74 を直接呼ぶ) は未実施・**要判断**。バッファの受け渡しを含む本体全体が本家と同じかは未確認
- 組 4 (C 族・自己相似パターン保護) と var 出力は未移植。段の高さ < 2、拡大の幅 < 2、幅 ≥ 513 は NOT_IMPLEMENTED
- Guard の 2 つの popcount (= 16 の仮定) は正規のホストでの値が未確定。popcount ≠ 16 のとき本家が呼び出し側の cfg を書き換えて後の呼び出しに残す点は `cfg_after` で返すが、それを次のチャンク・面に引き継ぐ駆動部は移植の外
- σ マップ・輝度別テーブル・cfg・色変換・チャンク分割は移植の外
- 提案: (1) 統合照合を行うかの判断 (ユーザー)、(2) 外側の部品 (nv_spatial.sigma_maps / build_cfg / filter_chunk / filter_frame) の C++ 化と Python の filter_frame との比較、(3) E9 素材での PSNR 測定、(4) x86 ビルドの確認 (portable ビルドは arm64 上で一致を確認済み)

## 6. make -C native verify (`logs/make_verify.log`、EXIT 0): 14 本すべて合格
verify_quality (本家画面 13/13、ビン対応 13/13)・verify_band 5706/5706・verify_curve ALL BIT-EXACT・verify_lzma 667/667・verify_flat ALL BIT-EXACT・verify_flat_cancel ALL IDENTICAL・verify_model 1244/1244・verify_temporal 474/474・verify_spatial 1287/1287・verify_profile PASS・verify_aft ALL BIT-EXACT・verify_match ALL MATCH・verify_fields ALL BYTE-EXACT・verify_fieldproc ALL BIT-EXACT

## 7. 統括の確認・独立検証 (2026-11-03)

### 7.1 統括の再実行 (担当者と別の乱数の種)
- 修正前: 葉 (seed 777) 不一致 0。compare_python (seed 4242) 全件一致、replay は 20,198 + 8,568 + 1,525 呼び出しで不一致 0
- M1・L1〜L6 の修正後: compare_python (seed 31337) 12/12 一致 (206,665 語、方向経路の段 39)、replay 9,606 呼び出し (fabs 24 を含む) で不一致 0・ERROR 0・SKIP 0
- `git diff` は native/Makefile と nvh.c だけ (nv_spatial6.{h,cpp}・nv_spatial6_main.cpp は新規)。既存の native のソースは変えていないので、§6 の make verify (14 本) はそのまま有効

### 7.2 独立検証 Round 1 (REQUEST_CHANGES)
- 葉 (seed 777001) 不一致 0、compare_python 14/14、replay 18,364 呼び出しで不一致 0。逆アセンブルの読み (a)〜(g) と組の表の値は確認
- M1: popcount ≠ 16 のとき全段の cfg[k] を呼び出し側の cfg に 0 で書く (初版は cfg[L−1] だけを写しに) → 修正
- L1: x20 は uint32 のゼロ拡張 → 修正。L2: §4.2 の内訳の漏れ → 修正。L3: replay の SKIP で終了コード 0 → 修正。
  L4: argmax の注釈 → 修正。L5: verify_leaves の出力ファイルの使い回し → 修正。L6: fabs が葉の再生の対象外 → 修正

### 7.3 独立検証 Round 2: APPROVE
- M1 は 0x98f774〜0x98f788 と、L1 は 0x993cd8・0x9917a4〜0x9917ac と一致 (p = 15 で −4294967294)。L2〜L6 は解消
- 既定の Guard では修正前と出力がバイト一致 (seed 4242 の 14 件、最後の Add(P, ACC) の出力)
- 検証者の実行: 葉 (seed 5150、7 種 × 82 呼び出し) 不一致 0、compare_python 14/14 (+ 両方 NOT_IMPLEMENTED 1)、replay 18,424 呼び出し (fabs 60) で不一致 0・ERROR 0・SKIP 0
- Low (新規): a. §1 の Guard の古い名前、b. §3.1 の量の乗数の型の書き方 → どちらも統括が本文を訂正 (変更前は scratchpad に控え)。
  c. `cfg_after` は CLI からも照合からも使われず未試験 (読んだ範囲では正しい) → 記録のみ
- ログ: 統括 `scratchpad/sp6_lead/`、検証者 `scratchpad/v6/`・`scratchpad/v6r2/` (揮発性)

### 7.4 残る制限 (推定を含む)
- C++ と Python 移植は同じ読みにもとづくので、両者の一致は本家の FUN_0098edb0 / FUN_00990f74 の本体との一致を示さない (本体はハーネスで呼ばない方針)
- 組 4 (パターン保護・C 族) と var 出力は未移植。段の高さ < 2・拡大の幅 < 2・幅 ≥ 513 は NOT_IMPLEMENTED
- 実ホストでの Guard の値は未確認 (既定は静的初期値の popcount 16)


## 8. 本家本体との照合 (2026-11-03、未実施)

- 経緯: ユーザーが nvh 経由で本家の FUN_0098edb0 を呼ぶことを承認した (2026-11-03)。統括の指示に従い、呼ぶ前に静的確認を行った。
- 静的確認 (観測、`tools/callgraph.py` → `logs/integ_static_callgraph.txt`。FUN_0098edb0 から推移的に到達する関数は 100 個):
  1. 外部呼び出し: _pow・_tanf、メモリの確保・解放・複写 (malloc / free / new / delete / memcpy / bzero / memset。確保器 FUN_009c80d4 → FUN_009d7f6c → FUN_009d7200 / FUN_009d7910)、スピンロック (sched_yield / gettimeofday / pthread_self)、sysctl (FUN_009ec008)、cxa_guard、stack_chk、chkstk。スレッドの生成・ファイル・ネットワーク系の呼び出しは見つからなかった
  2. ダイアログ: 警告関数 FUN_009d95a4 (CFUserNotificationDisplayAlert) が確保器の異常経路から到達する。nvh_guard の差し替えでダイアログにはならない見込み (推定)
  3. 改ざん検知様のブロック (例: 0x98f100〜0x98f110、0x991044〜0x991054、0x9926c8〜0x9926d8) は、__DATA のポインタ欄の値 + 即値の番地に書き込む
  4. host のコールバック: 取り消しの問い合わせ (core の先頭語の vtable 0 番。先頭語が NULL なら呼ばない)、確保器の関数ポインタ、組 4 のパターン保護の間接呼び出し (組 0〜3 では通らない)
  5. Guard の 2 つの popcount の元 (0x98f0d8〜0x98f0f8、0x9926a0〜0x9926c0) も、同じポインタ欄を経た大域の語
- 止めた理由:
  - (3) の書き込み先と (5) の状態語は、静的な読みだけでは確定できなかった。
  - それを確定させる手順 (ポインタ欄の解決) は、作業者側の安全性の判定で止められた。
- 結果:
  - **統合照合は実施していない。** 本家の本体は 1 回も呼んでおらず、nvh.c も変更していない。
  - 推定: C++ の移植と Python 移植の一致 (§4) は、本家の本体との一致を示さない。バッファの受け渡しを含む本体全体と、実ホストでの Guard の値は未確認のまま。

## 9. E9 素材での比較: v6 空間 / v5 / 本家 (2026-11-03)

作業者 (sp6-worker) が実行・評価し、§9 の本文は統括が書いた (作業者の書き込みは止められた)。数字は `logs/e9_table.txt`・`logs/e9_eval.log` から写した。

### 9.1 条件 (観測)
- 素材: プログレッシブ 3 素材 (old_town_cross・park_joy・sintel) × σ3/σ6 = 6 本、各 60 フレーム (依頼文の「4 素材」「150 フレーム」とは違う。インターレースの本家出力 `genuine/interlaced_noRI/` 4 本はフィールド処理が要るので対象外)。
- 評価: `wp_e9/tools/e9_eval.py` (無改変)、正解 = きれいな元画像、内部フレーム 2..57 の PSNR。本家は `genuine/progressive/Event_バージョン 1_0001_000N` (genuine_map.json の名前と違うが、ファイル名の検索で 1 本ずつ一意)。
- 処理: `tools/e9_v6.py` (nv_ui_qt.export と同じ流れ、プロファイルとパラメータは `runs/ours_frame_f5` (フレーム 5 で Auto Profile) と同じ、`--crf 0`)。時間段・プロファイルは v5 のまま、空間段だけを差し替えた:
  - **v5**: 差し替えなし。6 本とも全 60 フレームが `runs/ours_frame_f5` の出力とバイト一致 (`logs/e9_check_a.log`)
  - **v6-set0**: `wp_spatial/src/nv_spatial.spatial_genuine` と同じ入力 (σ は v5 の成分、cfg は v5 の量、genuine_t = 1.0、組 0、モデル語 5+k = 1.0) を C++ の `nvs6::plane_filter` に渡す。面全体 (幅 1920) を処理するため `Guard.allow_wide` (既定 false) を使う
  - **v6-set0m**: v6-set0 のモデル語 5+k を v6-dir と同じ値にしたもの (しきい値の変化だけを見るため)
  - **v6-dir†** (実験的): 方向経路あり。組は表 0x1246c48 の YCrCb・品質 0 (Y = 組 2、Cb/Cr = 組 1)、本家の分割 (核・余白、ROI = 核)。
    **モデル語は作業者の仮定**: プロファイルのフレーム 5 の雑音入り面の領域 (Y 128²、色差 64²) に `native/nv_model_test` (BuildNoiseModel の 1 面モデル、本家とビット一致) を掛け、段 k の語 (5+k、55+5s+k、80+80s+16k+d、480+5s+k、505+80s+16k+d) を、その領域での段 k の v5 σ の平均 (rv = 1) で割った。本家の値ではない
- 事前確認 (観測): C++ と Python 移植は 1080p の 6 本 × 3 フレーム × 2 変種 = 36 件で 3 面ともビット一致 (`logs/e9_check_b.log`)。`allow_wide` の追加後も既定の Guard で compare_python 14/14 (+ 両方 NOT_IMPLEMENTED 1)・verify_leaves 不一致 0。分割だけの影響 (v6-set0 の面全体と分割、2 フレーム) は Y が 8 ビットで同一、Cb/Cr が最大 0.10・8 ビット MSE < 1e-4 (`logs/e9_chunk_check.log`)。`~/.config/NeatRepro/nv_ui_qt.ini` の sha256 は前後で不変
- 統括の再実行 (観測): park_joy σ3 の v6-dir を処理し直した出力は、復号後の画素で作業者の出力とバイト一致。評価のやり直しで内部フレームの Y は v5 31.389・v6-dir 32.29・本家 40.296 dB (表と一致)
- v6-dir の正規化後の語 5+k (段 0..; 負の値は小さすぎる段の外挿で、しきい値は絶対値で使う):

| 素材 | Y | Cb | Cr |
|---|---|---|---|
| old_town_cross σ3 | 1.46/1.13/1.04/2.09/−2.93 | 0.53/0.60/0.53/−0.55 | 0.38/0.53/0.42/−0.42 |
| old_town_cross σ6 | 1.54/1.32/1.17/1.92/−2.14 | 0.52/0.62/0.56/−0.59 | 0.40/0.55/0.37/−0.37 |
| park_joy σ3 | 1.47/0.68/0.66/0.98/−0.79 | 0.47/0.50/0.43/−0.24 | 0.39/0.41/0.36/−0.20 |
| park_joy σ6 | 1.47/0.79/0.78/−0.88/−0.54 | 0.49/0.50/−0.50/−0.28 | 0.38/0.37/−0.43/−0.25 |
| sintel σ3 | 1.61/1.34/1.08/1.35/−1.38 | 0.53/0.55/0.81/−0.76 | 0.41/0.43/0.44/−0.26 |
| sintel σ6 | 1.72/1.86/1.53/1.74/−1.48 | 0.59/0.78/0.75/−0.61 | 0.46/0.48/0.54/−0.37 |

### 9.2 結果 (観測、PSNR dB、内部フレーム。Y静/小/大 = 正解の動きの区分ごとの Y、秒 = 60 フレームの処理時間 (jobs 4))
| 素材 | 出力 | Y | Cb | Cr | RGB | Y静 | Y小 | Y大 | 秒 (60f) |
|---|---|---|---|---|---|---|---|---|---|
| old_town_cross/p_y3 | 入力 | 38.55 | 45.56 | 45.11 | 36.40 | 38.55 | 38.55 | 38.55 |  |
| old_town_cross/p_y3 | v5 | 36.69 | 38.41 | 39.77 | 32.32 | 37.43 | 36.40 | 35.78 | 11.4 |
| old_town_cross/p_y3 | v6-set0 | 37.16 | 38.58 | 40.15 | 32.62 | 37.61 | 37.01 | 36.52 | 11.5 |
| old_town_cross/p_y3 | v6-set0m | 36.88 | 39.29 | 42.21 | 33.09 | 37.47 | 36.59 | 36.21 | 11.6 |
| old_town_cross/p_y3 | v6-dir† | 36.97 | 39.47 | 42.47 | 33.23 | 37.55 | 36.75 | 36.18 | 27.4 |
| old_town_cross/p_y3 | 本家 | 37.41 | 39.93 | 42.98 | 33.59 | 37.86 | 37.15 | 36.97 |  |
| old_town_cross/p_y6 | 入力 | 32.56 | 39.70 | 39.24 | 30.46 | 32.56 | 32.56 | 32.57 |  |
| old_town_cross/p_y6 | v5 | 35.94 | 38.22 | 39.22 | 31.81 | 37.11 | 35.52 | 34.61 | 11.4 |
| old_town_cross/p_y6 | v6-set0 | 36.53 | 38.37 | 39.63 | 32.16 | 37.30 | 36.23 | 35.58 | 11.5 |
| old_town_cross/p_y6 | v6-set0m | 36.01 | 39.01 | 41.33 | 32.43 | 37.10 | 35.53 | 34.87 | 11.7 |
| old_town_cross/p_y6 | v6-dir† | 36.37 | 39.27 | 41.89 | 32.77 | 37.27 | 36.06 | 35.24 | 26.2 |
| old_town_cross/p_y6 | 本家 | 36.37 | 39.78 | 42.55 | 32.93 | 37.14 | 36.08 | 35.42 |  |
| park_joy/p_y3 | 入力 | 38.57 | 45.12 | 45.13 | 36.46 | 38.59 | 38.59 | 38.57 |  |
| park_joy/p_y3 | v5 | 31.39 | 33.49 | 37.15 | 28.00 | 39.32 | 37.57 | 30.32 | 11.4 |
| park_joy/p_y3 | v6-set0 | 32.36 | 35.18 | 37.66 | 29.02 | 40.11 | 38.58 | 31.28 | 11.6 |
| park_joy/p_y3 | v6-set0m | 32.05 | 38.01 | 40.66 | 29.88 | 39.76 | 38.08 | 31.00 | 11.8 |
| park_joy/p_y3 | v6-dir† | 32.29 | 38.26 | 41.24 | 30.14 | 40.26 | 38.61 | 31.21 | 28.4 |
| park_joy/p_y3 | 本家 | 40.30 | 44.06 | 45.15 | 37.09 | 41.94 | 41.48 | 39.92 |  |
| park_joy/p_y6 | 入力 | 32.71 | 39.89 | 39.40 | 30.73 | 32.73 | 32.80 | 32.68 |  |
| park_joy/p_y6 | v5 | 31.35 | 33.49 | 36.96 | 27.95 | 38.75 | 36.96 | 30.32 | 11.7 |
| park_joy/p_y6 | v6-set0 | 32.24 | 35.02 | 37.34 | 28.86 | 39.38 | 37.84 | 31.22 | 11.8 |
| park_joy/p_y6 | v6-set0m | 31.89 | 37.74 | 39.96 | 29.66 | 39.08 | 37.33 | 30.87 | 11.7 |
| park_joy/p_y6 | v6-dir† | 32.18 | 38.07 | 40.50 | 29.97 | 39.54 | 37.92 | 31.14 | 26.0 |
| park_joy/p_y6 | 本家 | 36.50 | 41.21 | 42.50 | 33.58 | 39.74 | 38.98 | 35.85 |  |
| sintel/p_y3 | 入力 | 39.02 | 46.17 | 45.95 | 36.99 | 40.03 | 38.62 | 38.62 |  |
| sintel/p_y3 | v5 | 43.58 | 47.78 | 43.68 | 38.97 | 44.40 | 45.34 | 42.43 | 11.5 |
| sintel/p_y3 | v6-set0 | 43.83 | 48.42 | 44.11 | 39.33 | 44.43 | 45.66 | 42.75 | 11.6 |
| sintel/p_y3 | v6-set0m | 43.62 | 49.36 | 44.84 | 39.67 | 44.42 | 45.39 | 42.48 | 11.8 |
| sintel/p_y3 | v6-dir† | 43.97 | 49.65 | 44.93 | 39.91 | 44.50 | 45.87 | 42.92 | 25.7 |
| sintel/p_y3 | 本家 | 45.48 | 53.15 | 53.82 | 43.34 | 44.29 | 47.29 | 45.67 |  |
| sintel/p_y6 | 入力 | 33.08 | 40.51 | 40.18 | 31.12 | 34.07 | 32.68 | 32.69 |  |
| sintel/p_y6 | v5 | 39.26 | 45.63 | 41.52 | 35.70 | 38.89 | 42.69 | 38.36 | 11.7 |
| sintel/p_y6 | v6-set0 | 39.36 | 46.25 | 41.77 | 35.89 | 38.85 | 42.76 | 38.55 | 11.7 |
| sintel/p_y6 | v6-set0m | 39.20 | 46.88 | 42.33 | 36.06 | 38.92 | 42.59 | 38.24 | 11.7 |
| sintel/p_y6 | v6-dir† | 39.57 | 47.41 | 42.52 | 36.37 | 38.97 | 43.13 | 38.79 | 27.2 |
| sintel/p_y6 | 本家 | 40.43 | 51.00 | 50.00 | 38.52 | 38.35 | 42.87 | 41.46 |  |

| 素材 | v6-set0−v5 (Y/RGB) | v6-set0m−v6-set0 | v6-dir†−v6-set0m | v6-dir†−v5 | 本家−v5 | 本家−max(v6) (Y/RGB) | 本家 yuv8 Y |
|---|---|---|---|---|---|---|---|
| old_town_cross/p_y3 | +0.47/+0.31 | -0.28/+0.47 | +0.09/+0.13 | +0.28/+0.91 | +0.72/+1.28 | +0.25/+0.37 | 37.38 |
| old_town_cross/p_y6 | +0.59/+0.35 | -0.52/+0.28 | +0.36/+0.34 | +0.43/+0.96 | +0.43/+1.12 | -0.16/+0.16 | 36.35 |
| park_joy/p_y3 | +0.97/+1.02 | -0.30/+0.86 | +0.23/+0.26 | +0.90/+2.15 | +8.91/+9.10 | +7.94/+6.95 | 40.24 |
| park_joy/p_y6 | +0.90/+0.91 | -0.36/+0.80 | +0.29/+0.31 | +0.84/+2.03 | +5.15/+5.63 | +4.26/+3.61 | 36.48 |
| sintel/p_y3 | +0.24/+0.36 | -0.20/+0.34 | +0.35/+0.24 | +0.39/+0.94 | +1.90/+4.38 | +1.51/+3.44 | 45.28 |
| sintel/p_y6 | +0.09/+0.19 | -0.16/+0.17 | +0.37/+0.31 | +0.30/+0.67 | +1.17/+2.82 | +0.86/+2.15 | 40.37 |

(差の表の「本家 yuv8 Y」は本家の出力を 8 ビット 4:2:0 に丸めた Y。当方の出力は 8 ビットなので、厳密にはこちらと比べる。差は 0.02〜0.20 dB (sintel σ3 が最大))

### 9.3 v6 は本家との差を縮めたか (観測、縮めた割合 = (v6 の最良 − v5) / (本家 − v5))

| 素材 | Y | RGB | 一言 |
|---|---|---|---|
| old_town_cross σ3 | 65% (v6-set0) | 71% (v6-dir†) | 大半を縮めた |
| old_town_cross σ6 | 本家を上回る (v6-set0 36.53 > 36.37) | 86% (v6-dir†) | ほぼ追いついた |
| park_joy σ3 | 11% | 24% | 差 7.9 dB が残る |
| park_joy σ6 | 17% | 36% | 差 4.3 dB が残る |
| sintel σ3 | 21% | 21% | 差 1.5 dB (Y)・3.4 dB (RGB) が残る |
| sintel σ6 | 26% | 24% | 差 0.9 dB (Y)・2.2 dB (RGB) が残る |

- v6-set0 と v6-dir† はどの素材でも v5 より良い (v6-set0 は Y +0.09〜+0.97、v6-dir† は Y +0.28〜+0.90・RGB +0.67〜+2.15)。v6-set0m は RGB ではすべて v5 より良いが、Y では sintel σ6 で v5 より 0.07 dB 低い (39.20 / 39.26。2026-11-03 訂正、独立検証の L1)。
- v6-set0m − v6-set0 は Y で −0.16〜−0.52、RGB で +0.17〜+0.86 (色差のしきい値が下がり、色差は良く輝度は悪くなる)。v6-dir† − v6-set0m は Y +0.09〜+0.37・RGB +0.13〜+0.34 で、方向経路の効果はどの素材でも正。
- park_joy は v5 も v6 も入力より Y が悪い (入力 38.57 → v5 31.39・v6 32.36、本家 40.30)。動きの区分では、静止部は v5 39.32 で入力より良く、大きな動きの部分が v5 30.32 (入力 38.57) と悪い。old_town_cross σ3 も v5 が入力より低い。
- sintel の Cr は v5 43.68 < 入力 45.95 (本家 53.82)。静止部の Y は v6 が本家を上回る (σ3: v6-dir† 44.50 / 本家 44.29、σ6: 38.97 / 38.35)。

### 9.4 推定
- 残る差の大部分は空間段ではない: park_joy の悪化は大きな動きの部分に集中しており、時間段・動き補償 (v5 のまま) が原因とみられる。sintel の色差の差は、v5 の色差の較正 (CHROMA_GAIN Cb 3 / Cr 4、test_v02 1 本で決めたもの) が強すぎることとみられる。
- v6 の空間段の効果は 0.1〜1 dB (Y) 程度で、時間段・較正の見直しに比べて小さいとみられる。

### 9.5 解釈の限界
- **本家の出力の GPU 経路** (2026-11-03 訂正。初版は「GPU 有効/無効の A/B は未実施」と書いたが誤り。独立検証の M1): E9 の本家は Resolve 上で 12CPU+1GPU auto で処理した。同じプログレッシブ 6 本の GPU 有効/無効の A/B は実施済み (`work/omo/wp_genuine_capture/REPORT.md` §3、`eval/gpu_ab_*.json`・`logs/cmp_*.txt`)。画素はビット一致しない (GPU vs CPU のみ 0/360 フレーム一致、GPU どうしも 2/360) が、正解に対する PSNR の差は最大 0.005 dB で、§9.2 の本家の数値は CPU 経路でも同じと扱ってよい (観測)。したがって本家との差 (例: park_joy の 4〜8 dB) は CPU/GPU の経路差では説明できない。ただし本家とのビット一致の照合では GPU の出力は使えない (CPU のみの出力が基準)。GPU の処理本体は未解読のまま (`docs/GPU_STATUS.md`。同書と Agents.md §9 の「A/B は最優先・未実施」は古い記述)。インターレースは A/B していない
- **本家は改変されたバイナリ**: E9 の本家は導入済みの RS 版 (ad hoc 署名、配布物から改変、`wp_genuine_capture/REPORT.md` §0)。保護の状態語に依存する値 (Guard の popcount など) が正規のホストと同じとは限らない (推定)
- v6 の空間段は本家の本体と照合していない (§8)。C++ と Python 移植は同じ読み。
- σ と cfg は v5 の代用 (本家の sigma_maps / build_cfg ではない)。組の選択・分割・Guard.allow_wide (幅 ≥ 513 の分岐は帳簿付けだけという §3.1 の読みに依る) も本家と照合していない。
- v6-dir† のモデル語の正規化は作業者の仮定。v6-set0m との差で方向経路の効果を分けたが、正規化そのものが妥当かは未確認。
- 時間段・プロファイルは v5、較正は test_v02 の 1 本。プロファイルの見本位置はフレーム 5 (本家と同じ位置になるのは 10 本中 7 本、Agents.md §10)。
- 評価は各 60 フレームの内部 56 フレームだけ。

### 9.6 ファイル
- 道具: `tools/e9_v6.py`・`tools/e9_v6_spatial.py`・`tools/e9_check_b.py`、C ABI `native/nv_spatial6_c.cpp` と Makefile のターゲット `libnvspatial6.dylib` (`all`・`verify` は変更なし)、`nvs6::Guard.allow_wide` (変更前は `backup/nv_spatial6.{cpp,h}.before_e9`)。いずれも main の作業ツリーで未コミット
- ログ: `logs/e9_check_a.log`・`e9_check_b.log`・`e9_allow_wide_compare_python.log`・`e9_allow_wide_verify_leaves.log`・`e9_chunk_check.log`・`e9_runs.log`・`e9_eval.log`・`e9_table.txt`
- 出力 (追跡外): `wp_e9/runs/sp6_{v5,v6set0,v6set0m,v6dir}/<src>/p_y{3,6}/` (out.mp4・run.json・v6dir は models.npz)、評価 `wp_e9/runs/sp6_eval/*.json`
