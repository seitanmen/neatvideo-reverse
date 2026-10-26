# wp_native_flat 報告書 (N1d: FindFlatArea nv_flat の C++ 移植)

日付: 2026-10-27。担当: N1d 作業エージェント (サブエージェント)。REPORT.md は作業者が書かなかったため、最終応答を統括が転記した。
指示: `.claude/tasks/native_n1_split.md` の「共通規約」「検証の合格ライン」「N1d」。
変更していないもの (作業者の申告、`git diff --stat` は空): repro/・neatvideo_ofx_614/・処理コア・AGENTS.md・`native/Makefile`・既存の `native/nv_band.*`・`native/nv_curve.*`。
していないこと: git への書き込み、本家バンドルと nvh の実行、`repro/out/` への書き込み。

## 結果
`make -C native -f Makefile.flat && python3 native/verify_flat.py` の 1 回の実行で全件ビット一致、ビルドの警告 0 (観測)。

```
flat: 181 cases (60 real frame x mode, 81 synthetic, 40 fuzz; 32 deep)
flat: 362/362 case runs bit-exact (result + steps + curve + reduce + candidates + placed/IsClip + trace + structs)
  per run: 17730 candidates, 17109 scored candidates, 1293 x 3 structs (deep); statuses {0: 41, 1: 60, 2: 65, 3: 13, 4: 2}
total 146.9 s: ALL BIT-EXACT
```
クリーンビルドからの最終実行のログ: `sandbox/omo/wp_native_flat/tmp/full2.log`。

## 1. やったこと
`repro/nv_flat.py` の `find_flat_area_genuine` (cancel を除く) と、その部品を C++ に移植した。

| C++ (`namespace nvf`) | 移植元 (Python) |
|---|---|
| `reduce_planes` | `reduce_planes` (FUN_009b7e58: ブロックを行優先に float32 で累積し、最後に 1/(s·s) を掛ける) |
| `luma_601` | `nv_pixnoise.luma_601` / `luma_601_batch` (先頭 n&~3 個は FMA、残りは非融合) |
| `candidates` | `candidates` (FUN_009999cc。下の 3 点を含む) |
| `transform` | `transform` (色モード 0/3 は複写、1 は Y/Cr/Cb、2 は平均と差) |
| `flatness` / `level_terms` / `score` | `flatness` / `_level_terms` / `_score` |
| 内部の `place`・`fcvtzs`・`box` | `_place`・`_fcvtzs`・`_box` |
| `find_flat_area_genuine` | 同名の関数。縮小率 s の決め方、サイズ表 3 種、曲線を走らせる位置 (サイズ表の後)、IsClip による棄却 (2 面以上)、nz < 1e-6 の棄却、罰則 (double で > 0.1)、厳密な小なりでの選択、512 への切り詰め、状態 0〜4 |

`candidates` で写した点:
- 走査は右下から逆順で、最初に見つかった厳密な最小を選ぶ (NaN は inf として扱う)。
- 窓を MASKED で埋める。
- 同じ輝度ビンの候補が 4 個に達したら、そのビンの画素をすべて埋める。

再利用したもの:
- N1a: `nvb::band_levels`・`nvb::box_rows`・`nvb::box_cols` (半径 0 の複写は nv_flat 側で処理)。
- N1b: `nvc::genuine_noise_curve`・`nvc::is_clip_genuine`・`nvc::CurveInterp`。
- `nvc` の luma_601 は無名名前空間にあって呼べないため、同じ式を nv_flat.cpp に写した。

演算順と定数:
- float 定数は、Python の値をビットパターンで出力して写した (例: 罰則の係数 1.15 = 0x3f933333・1.33 = 0x3faa3d71、色差の平坦度の限度 0x402ccccc、MASKED 0x461c4000)。
- 被演算子の順:
  - numpy の配列演算は第 1 被演算子の NaN を返すので、AArch64 のインラインアセンブラ `A`/`M` で順を固定した。
  - numpy の float32 スカラーの + と * は第 2 被演算子の NaN を返すので、`AS`/`MS` を使った (`_level_terms`・`_score`・`flatness` の累積と閾値の列)。
  - − と / は C の演算子のまま (第 1 被演算子)。
- `threads > 1` では、曲線と、1 サイズ分の候補の帯域測度を並列にする。選択は逐次なので結果は変わらない。-j 1 と -j 3 の両方を全件で照合した。

## 2. 作ったファイル
- `native/nv_flat.h`・`native/nv_flat.cpp`: 移植本体。中間値を取り出す `Diag` (縮小面、各サイズの候補 64 個、配置と IsClip、採点した候補の trace、帯域測度の構造体) 付き。
- `native/nv_flat_main.cpp`: 試験ドライバ `nv_flat_test`。
  - `flat IN OUT [-j N]` と `time IN [-j N] [REPS]` (曲線だけの時間も出す)。
  - 入力は 1 ケースずつ読む。書式はファイル先頭のコメント。
- `native/Makefile.flat`: `nv_flat_test` を nv_flat_main・nv_flat・nv_band・nv_curve から `-lpthread` 付きでビルドする。
  - フラグは `native/Makefile` と同じ。
  - 感度試験用に `MUTFLAGS` の入口がある (未使用)。
- `native/verify_flat.py`: 照合スクリプト。
  - オプション: `--quick`・`--jobs` (最大 3)・`--fuzz N`・`--chunk`・`--cache` (Python の期待値を tmp に保存して再利用する。感度試験用)。
  - 環境変数 `NV_FLAT_EXE`・`NV_FLAT_TMP` で別のバイナリ・作業場所を指定できる。
- `sandbox/omo/wp_native_flat/mutants/run_mutants.py`・`mutants.log`: 感度試験。nv_flat.cpp の写しを変異させて sandbox 内でビルドし、native/ には書かない。
- `sandbox/omo/wp_native_flat/bench.py`・`tmp/bench.log`: 速度の計測。
- `sandbox/omo/wp_native_flat/tmp/`: フレームのキャッシュ 20 枚と期待値の pkl (約 396 MB)、ログ `full1.log`・`full2.log`。

## 3. 検証 (観測)

### 比較の方法
1 ケースごとに次をすべてビット列で比べた。許容誤差は使っていない。
- 結果の (x, y, w, h, 状態)
- s・縮小後の大きさ・最初に収まるサイズの番号 i0・曲線を走らせたか
- C++ が自分で計算した曲線 8 点 (f64)
- 縮小した R・G・B と輝度
- 各サイズの r・step と候補 64 個 (x, y, 平均, スコア)
- 各候補の配置と R・G・B の IsClip
- 採点した全候補の trace (サイズ・番号・正方形・クリップ数・罰則・平均・nz・平坦度 3 つ・E・R・スコア)
- deep のケースだけ、採点した候補の帯域測度 3×4096 語。Python 側は `transform` + `nv_band.band_levels` を 1 面ずつ呼んで期待値を作るので、`find_flat_area_genuine` が使うバッチ経路と単面経路が一致することも同時に確かめている。

C++ は全ケースを -j 1 と -j 3 の両方で走らせて比較した。

### 入力
**実写 20 フレーム × 色モード 0/1/2 = 60 ケース**
- test_v02 の n = 0, 48, 100, 150, 200, 260, 312, 380, 450, 520, 576, 700
- test.mp4 の n = 0, 120, 240, 300, 450, 600, 750, 870
- ffmpeg の rgb48le を 257 で割ったもの (quality_draft._frame と同じ)。ffmpeg は動画 1 本につき 1 回の実行で切り出した。
- test_v02 n=312 モード 1 の結果は (832, 0, 128, 128, 0) で、AGENTS.md §6.5 の記述 (x832・y0 の 128²) と同じ。

**合成 81 ケース**
- 黒帯と飽和:
  - 黒帯 (上下)、ピラーボックス、白飛び。
  - R が飽和 (クリップ数 1 → 状態 4)、R と G が飽和 (全候補を棄却 → 状態 1)。
- 同点:
  - 定数 128、量子化した平坦ノイズ (同点)、32px 周期のノイズ (同点の候補)。
- NaN・inf・巨大値:
  - NaN・±inf のブロック、NaN の列帯、ペイロードの違う NaN が 30% 混じる領域。
  - 暗部、3e38 / −1e30。
- 罰則の境界: IsClip がちょうど 0.1f になる 10px 周期の柄 (比較 0.1f > 0.1 (double) の境界)。
- サイズ表の境界:
  - 2048×1280 (= 0x280000) と 2048×1281。
  - 4K (n=312 を 2 倍に拡大)、4224×2496 (> 0xA00000 で表 3)。
  - 4608×3712 (s = 9、step = 1)。
- 小さい画像:
  - 64×64、63×64、65×97、49×50、96×96、100×60、30×200、127×129、129×128。
  - 48×48、47×47 と 40×40 (収まるサイズなし)、7×7 (w = 0)。
- 400² 以下の合成画像は色モード 0〜3 のすべて (モード 3 = 複写も含む) で試した。
- ノイズ表を明示したもの 8 種:
  - 0、NaN、負、1e39。
  - 1.0000001e-6、ちょうど 1e-6f (nz の閾値の境界)。
  - 2 点の表、節点の値に NaN ペイロード。

**乱数 40 ケース**
- 生成器は `verify_curve.fuzz_image` (48〜260px、NaN・inf・巨大値・NaN ペイロード・非正規数)。
- 色モードは乱数で選んだ。

### 集計
- 1 回の実行あたり: 候補 17,730 個、採点した候補 17,109 個、構造体 1,293 × 3 面。
- 状態: 0 が 41 件、1 が 60 件、2 が 65 件、3 が 13 件、4 が 2 件。
- 状態 5 は出ない (cancel は未移植)。

### 途中の誤り
- 最初の --quick で、構造体だけが不一致になった。
- 原因は照合スクリプト側の誤り。Python の `band_levels` が float32 のビューを返すのを、uint32 で比べていなかった。
- 修正後は全件一致した。移植本体は一度も直していない。

## 4. 感度試験 (`mutants/run_mutants.py`、--quick 106 ケース、-j 1)

**検出 (13 種)**

| 注入した誤り | 不一致のケース数 |
|---|---|
| 同点で後ろの候補を選ぶ (<=) | 6 |
| NaN を inf に置き換えない | 7 |
| 縮小の列の順を逆に | 64 |
| 縮小の累積の NaN の順 | 4 |
| 輝度を非融合に | 89 |
| 色差の限度を 2.7f に | 21 |
| E/n を逆数の積に | 64 |
| t·(E/nz) を (t·E)/nz に | 54 |
| 罰則を float の 0.1f との比較に | 1 (IsClip = 0.1f の柄で検出) |
| nz の閾値を <= に | 1 (ちょうど 1e-6f の表で検出) |
| ビンの満杯判定を > に | 51 |
| モード 2 の /3 を ×(1/3) に | 16 |
| 状態 2 の判定を > 0x80 に | 14 |

**未検出 (4 種)**

いずれも、試験の入力では出力が変わらない変更とみている (推定。証明はしていない)。

| 注入した誤り | 出力が変わらないとみる理由 |
|---|---|
| `cv_chain_continue` (平坦度の CV の列で、負の語を break でなく continue) | 負の CV は列の末尾 (未測定の印) にしか現れないとみられる |
| `score_nan_order`・`level_terms_nan_order` (スカラーの NaN の順) | 帯域の平均は fmaxnm で NaN にならず、平坦度は 1 以上か inf。両方の被演算子が同時に NaN になる経路がほぼない |
| `flat_ok_le` (平坦度の判定を <= 2 に) | 平坦度がちょうど 2.0 になる入力を作れていない |

## 5. 計測時間 (`bench.py`、M2 Max)
条件: 負荷平均 6.6〜8.3、色モード 1、各 3 回の最良値、曲線込み。

| 入力 | C++ -j 1 | C++ -j 3 | Python |
|---|---|---|---|
| 1080p (test_v02 n=312) | 0.814 秒 (曲線だけ 0.507) | 0.340 秒 (曲線だけ 0.169) | 1.579 秒 (曲線 0.715、FindFlatArea 部分 0.864) |
| 4K (3840×2160) | 2.916 秒 (曲線だけ 1.938) | 1.184 秒 (曲線だけ 0.665) | 5.097 秒 (曲線 2.649、FindFlatArea 部分 2.369) |

- FindFlatArea 部分 (曲線を除く) は C++ -j 1 で 1080p 約 0.31 秒・4K 約 0.98 秒。Python の約 2.4〜2.8 倍速。
- 残りの時間の大半は、320²・192² の候補ごとの帯域測度 (分位点のソート) とみている。N1a の報告 §3 と同じ傾向 (推定。内訳は測っていない)。
- 照合の全実行での C++ の所要時間は、-j 1 が 56.7 秒、-j 3 が 22.6 秒 (入出力込み)。

## 6. Python 側への不具合報告 (直していない)
- 不具合と、docstring と実装の食い違いは見つからなかった。
- 注記 1: `_level_terms`・`_score`・`flatness` は numpy の float32 スカラーで書かれているので、+ と * では第 2 被演算子の NaN が残る。
  - C++ もそれに合わせた。
  - 本家の命令の被演算子の順と一致する保証はない。ただし §4 のとおり、実際には両方が NaN になる経路がほぼない。
- 注記 2: trace の float は `float()` で double に変換されている。NaN を含めて float32 に戻すとビットが元に戻るので、照合に使える。

## 7. 未移植・未検証
- **取り消し (cancel) は未移植。**
  - C++ は取り消しの問い合わせをせず、状態 5 は曲線が失敗したときだけ返す。
  - `nvc::genuine_noise_curve` が失敗するのは 48px 未満の画像だけで、FindFlatArea はその経路に入らない。そのため実際には 5 を返さない。
- 難読化カウンタ (縮小後の高さへの加算) は、1 回目 (加算なし) の経路に固定した。Python と同じ。
- 色モードが 0〜3 の外: Python は ValueError、C++ は状態 −1 を返す (照合していない)。
- 本家との直接照合はしていない。一致の相手はこの機械の Python 移植 (numpy 2.1.3)。
- NaN のビット列の一致は AArch64 でだけ保証する。x86 ではインラインアセンブラのない経路になり、未確認。
- 8K (s = 12) は照合していない。s ≥ 9 は 4608×3712 の s = 9 で確認した。
- 本番の入口 `nvfilter.find_flat_area` は移植の対象外。YUV から RGB への変換・キャッシュ・64px 未満の代替方法を含む。
- 統合時の注意: `.gitignore` は許可リスト式なので、新しいファイルは今のままでは git に載らない (`native/Makefile.flat` の例外が要る)。
