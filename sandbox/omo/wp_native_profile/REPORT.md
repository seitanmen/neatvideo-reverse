# N1f: プロファイル構築 (nv_profile) の C++ 移植 — 作業報告

作業者: n1f-profile (2026-10-27)。REPORT.md は作業者が書けなかった (サブエージェントの .md 書き込みは止められる) ため、作業者の返信を統括が転記した。
タスク: `.claude/tasks/native_n1_split.md` の N1f、注意点は `native_n1_fixes.md` の「並行で着手できる本線」節。
作業ツリー: `/Users/seitan/Desktop/work/reverse` (main)。新規ファイルのみ作成し、既存ファイルは変更していない
(`sandbox/omo/readonly_snapshot_before.sha256` の照合は全件 OK)。コミットはしていない。

## 結果 (観測)

- `python3 native/verify_profile.py -j 3`: **39/39 のプロファイルで 0xc300 バイトすべてが Python とビット一致** (PASS)。
  - 実写 12/12: test_v02.mp4 n=100/312/576、test.mp4 n=0/300/450 × 128²・192² (色モード 1)
  - 実写の追加 9/9: 色モード 0 ×2・2 ×3・3 ×1、正方形でない領域 160×96・101×77・64×200、大きい領域 320×200
  - 合成 18/18: NaN の各種ペイロード (quiet/signalling・符号付き)、ブロックごとに別ペイロードの NaN (floor なし。tile_dev に NaN が届く)、±inf、
    飽和 (クリップの旗)、半面白、定数の面、1 面だけ定数、LZMA の溢れ (3 面とも / G だけ。後始末で 0x18..0x2b と 0xc2a4 が 0)、
    64×36 (mc_stats の 32 段が失敗して ×1.4 の分岐)、32×32 (最小)、48×40、31×64 (tile_dev で NotImplementedError、状態コード一致)、
    with_floor=False・lzma=False、−0 を含む 192²、巨大値 (σ1e6)、ランプ
  - 0x00..0x17 (ヒープポインタ) は双方 0 であることも確認している
  - `-j 1` でも 38/38 一致 (NaN グリッドの合成ケースを足す前の全件)
- 基準値: C++ で計算した reference_floor (51 語) が `data/ref_floor.npy` と **51/51 ビット一致**。白色ノイズ発生器 (FUN_009c3bc4) は
  `nv_refnoise.noise` と 3 通り (65536 語 seed 0x12345678、1000 語 seed 0xdeadbeef、37 語 seed 0) でビット一致
- 陰性対照 (verify_profile.py に組み込み): 領域内の G の 1 画素を 1 ulp 変えると 5〜11 バイトが変わる (モデル G の語 55 以降と tile_dev G)。
  領域外 (左の余白) の画素を +37 しても完全に同一 (領域の切り出し位置の確認を兼ねる)。4/4 期待どおり
- ビルド: `make -C native -f Makefile.profile` (`-std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra`) でクリーンビルド警告 0

## 新規ファイル

| ファイル | 内容 |
|---|---|
| `native/nv_profile.h` / `.cpp` | `nvp::build_profile` 本体と部品 (tile_dev・mc_stats・基準ノイズ・基準値の読み込みと計算・size_class・curve_bin) |
| `native/nv_profile_main.cpp` | テストドライバ `nv_profile_test` (一括照合・`time`・`floor`・`noise` のサブコマンド。使い方は先頭のコメント) |
| `native/Makefile.profile` | 単独 makefile (`Makefile.flat` と同じ形式。LZMA SDK のオブジェクトは `Makefile.lzma` と同じ規則) |
| `native/verify_profile.py` | 照合スクリプト (下記) |
| `sandbox/omo/wp_native_profile/bench.py` | 計時 (C++ -j1/-j3 と、Python のキャッシュに残る時間) |
| `sandbox/omo/wp_native_profile/tmp/` | Python の期待値キャッシュ (`pycache/`) と照合ログ (`quick.log`・`full.log`・`full2.log`・`mutants.log`・`mutants2.log`) |

**注意 (統括へ)**: `.gitignore` の 22 行目 `native/**` により、`native/Makefile.profile` と `native/nv_profile_test` は無視される
(`.cpp/.h/.py` は追跡対象として見えている)。Makefile.profile をコミットするなら `git add -f` か `.gitignore` の調整が必要 (私は .gitignore を触っていない)。

## 部品ごとの対応表

| Python (repro/) | C++ | 備考 |
|---|---|---|
| `nv_profile.build_profile` | `nvp::build_profile` | rgb は H×W×stride_c、area は (x, y, w, h)。領域が枠外のときとモード 0..3 以外は BAD_ARGUMENT (Python は検査しない。numpy のスライスの切り詰めは再現しない) |
| `nv_flat.transform` | `nvf::transform` (N1d) | 切り出した面のコピーに適用 |
| `nv_model.model_full` (6 面、set −1、5 段) | `nvm::model_full` (N1e) | 先頭 0x1f54 バイトを 0x4c + i·0x1f54 に置く。状態は面 0..5 の順で最初の非 0 を返す (Python の例外と同じ順) |
| `nv_profile.tile_dev` (FUN_009b2f34) | `nvp::tile_dev` | 32×32 タイル (16 px 間隔) のモデル (set 2、2 段) は N1e。加算は Python と同じ tx 外側・ty 内側の順。W/H < 32 は NOT_IMPLEMENTED |
| `nv_profile.reference_floor` | `nvp::load_reference_floor` (npy の読み込み、必須の経路) / `nvp::compute_reference_floor` (計算、任意の経路) | ドライバは既定で `<バイナリの場所>/../data/ref_floor.npy` を読む (`--floor` で変更)。計算版は npy と 51/51 一致、約 0.47 秒 |
| `nv_refnoise.noise` (FUN_009c3bc4) | `nvp::reference_noise` | `_gen_src` と同じ機械的な変換で、命令列を C++ の文に写した (反復をまたぐのは w8 だけ) |
| `nv_mcstats.mc_stats` (FUN_0098718c) | `nvp::mc_stats` | grey16・down2 (先頭 W&~7 列は算術シフト + 飽和、残りの列は u32 の論理シフト)・17 点の箱 (reflect 詰め、×⌊2³²/17⌋ >> 32)・int16 への折り返し・高域・PRNG・SAD・最小の探索はすべて整数。最後の割り算だけ float |
| `nv_band.mean_exact` | `nvb::mean_exact` (N1a) | |
| `nv_curve.is_clip_genuine` | `nvc::is_clip_genuine` (N1b) | 旗は > F32(0.33) (0x3ea8f5c3) |
| `nv_profile.size_class` / `curve_bin` | `nvp::size_class` / `curve_bin` | |
| `nv_profile.lzma_part` | `nvlz::plane_int16` / `nvlz::blob` (N1c、SDK 9.20) | 3 面すべて成功したときだけ、長さ・W・h・0xc2a4 を書く (Python は最初の失敗で打ち切るが、出力は同じ) |
| 曲線の初期化・0xbf34・0xc2c0・0xc2c8.. の定数 | `build_profile` の中 | 定数はすべて Python 側の float32 のビットから写した |

### tile_dev の float 演算 (numpy のスカラー演算の意味論)
tile_dev は numpy の float32 **スカラー**演算で書かれている。numpy 2.1.3 / arm64 で探った結果 (観測):
- 両方が NaN のとき、`a + b` と `a * b` は **b** のペイロードになる (fadd/fmul が (b, a) の順で実行される)。`a / b` は a のペイロード
- signalling NaN が優先されて quiet 化される (AArch64 の規則)
- `abs()` は sNaN を quiet 化しない。`np.sqrt` は quiet 化する

C++ ではこれをインラインアセンブラではなく明示的な分岐 (`nan2`・`np_add` など) で再現し、NaN の入力がないときだけ通常の演算をする (NaN 以外の結果は被演算子の順に影響されない)。
演算の結果として生じる NaN は AArch64 の既定 NaN 0x7fc00000 に揃えるので、この部分はアーキテクチャに依存しない。ただし再利用した N1a/N1b/N1e が AArch64 前提なので、全体の照合は AArch64 でだけ行った。
補助の確認 (使い捨てのスクラッチ試験): 14 種の値 (NaN 各種・±inf・±0・非正規数・最大値) の全 196 組で、add/mul/div/sqrt が numpy のスカラーとビット一致。

## 検証コマンドと件数

```
make -C native -f Makefile.profile
python3 native/verify_profile.py -j 3          # 39 件 + 基準値 4 件 + 陰性対照 4 件。Python の期待値はキャッシュ (初回は 3 プロセスで約 4 分)
python3 native/verify_profile.py --quick       # 19 件 (実写 4 + 追加 2 + 合成 13)
python3 sandbox/omo/wp_native_profile/bench.py # 計時
```
- 期待値のキャッシュのキー: 「窓の画素バイト・W・H・モード・領域・旗」の SHA-1 と、`build_profile` が使う repro のソース 12 本
  (nv_profile・nv_mcstats・nv_refnoise・nv_flat・nv_model・nv_model_special・nv_model_bm・nv_band・nv_curve・nv_lzma・nv_pixnoise・nv_lines) と
  `data/ref_floor.npy` の SHA-1。各ケースはフレーム全体ではなく、領域に最大 7 px の余白を付けた窓を渡すので、領域のオフセットも試験される
- LZMA の長さ 0 の入力: 領域は 1 px 以上なので発生しない (辺が 32 未満なら、LZMA の前の tile_dev で止まる)。照合から除外した件は 0
- 不一致のときは、違うバイト範囲を部品名で示す (`part_of`: ヒープポインタ・LZMA の長さ・モデル面 R/G/B/T0/T1/T2 の語 0..54 / 55 以降・tile_dev R/G/B・mc_stats・means・
  サイズ区分・クリップの旗・曲線・LZMA の旗・W/h・定数)

### 感度試験 (変異体、`tmp/mutants.log`・`mutants2.log`)
nv_profile.cpp をスクラッチにコピーして 1 か所だけ変え、verify を回した (本体のソースに変異用のフックは置いていない):

| 変異 | 結果 | 解釈 |
|---|---|---|
| タイルの加算順を ty 外側にする | 検出 (全実写で tile_dev R/G/B が不一致、基準値も不一致) | |
| mc_stats の最後の割り算を変える | 検出 (mc_stats の 128 バイト) | |
| np_add の NaN の優先を (a, b) にする | 不検出 | 到達できる入力では等価とみられる (推定。下記) |
| sNaN の優先を外す | 不検出 | 同上 (モデルの語に届く NaN は既に quiet。`0x7fe00001` などを観測) |
| floor の比較 `>` を `>=` にする | 不検出 | 等価 (等しいとき置き換えても同じ値) |
| size_class の分母に +1 | 不検出 | 等価 (区間内の割合は 1 未満なので、切り捨ては常に c) |

NaN の優先の変異が出力に現れない理由 (推定): tile_dev に NaN が届くのは CV の語 (15..30) だけで、タイルの CV が NaN なら、同じ面全体のモデル pm の CV も NaN になる。
`_dev(x, y)` では |x| > |y| が偽なので |y|/|x| を計算し、割り算の第 1 被演算子である pm 側のペイロードが選ばれる。累積で出会う NaN はすべて同じ pm の語に由来するので、
足し算の優先がどちらでも結果は同じになる。また floor ありの経路では、NaN は floor に置き換わる。数値の再現としては numpy の意味論どおりに実装してあり、上の 196 組の単体照合で確かめた。

## 計時 (観測。M2 Max、負荷平均 7.9〜8.6 の中で、他の作業者と CPU を共有)

| 領域 | Python `build_profile` (1 回、3 プロセス並列の中) | C++ -j1 (3 回の最良) | C++ -j3 |
|---|---|---|---|
| 128² (test_v02 n=100) | 17.1 秒 | 1.27 秒 (14 倍) | 0.45 秒 (38 倍) |
| 192² (test_v02 n=100) | 28.1 秒 | 1.94 秒 (15 倍) | 0.68 秒 (42 倍) |
| 128² / 192² (test_v02 n=312) | 18.3 / 28.2 秒 | 1.34 / 1.87 秒 | 0.49 / 0.66 秒 |

C++ の内訳 (乱数の面、-j1、スクラッチの計時):
- 128²: モデル 6 面 0.94 秒、tile_dev 3 面 0.04 秒、mc_stats 0.01 秒、LZMA 3 面 0.01 秒
- 192²: それぞれ 1.43 / 0.09 / 0.015 / 0.02 秒

**時間のほぼすべては N1e の `nvm::model_full` (1 面モデル)**。
`-j N` は 6 面のモデル、tile_dev のタイルのモデル、LZMA の 3 面を並列にする。加算と選択は逐次のままなので、出力は -j によらない (-j1 と -j3 の両方で全件一致を確認)。

## 未解決点・制限

- 非 AArch64 は未対応 (N1a/N1b/N1e の部品が AArch64 前提のため。nv_profile.cpp 自身の float 演算はアーキテクチャ非依存に書いてある)
- 照合は Python の移植に対して行った。本家バンドル・nvh は実行していない。本家との一致は、Python 側の既存の照合 (AGENTS.md §6.5: `nvh profile` 76 件で、ポインタ 6 語以外は一致) に依存する
- 領域が枠外のとき、Python は numpy のスライスで黙って切り詰め、w/h の欄には指定値を書く。C++ はこの場合 BAD_ARGUMENT を返す (本家の経路では FindFlatArea が枠内の領域を返すので起きない)。
  モード 4 以上も、Python は transform を恒等として通すが、C++ は BAD_ARGUMENT を返す
- 1 辺が 512 を超えるか 2 未満のときは、nvm::model_full が BAD_ARGUMENT を返す (Python の ValueError と同じ)。照合のケースには含めていない (512 を超える面は Python が非常に遅い)
- `curve_bin` の +inf/NaN の分岐と `size_class` の m < 16 の分岐は、tile_dev が辺 ≥ 32 を要求するので (m ≥ 32)、build_profile からは到達しない

## Python 側への不具合報告

不具合は見つからなかった。気づいた点のみ:
- `nv_profile.build_profile` は領域の範囲を検査しない (枠外では切り詰めた crop に対して計算し、W/h の欄だけ指定値になる)
- `lzma_part` は最初に失敗した面で打ち切るが、後始末で全部 0 にするので、出力は打ち切らない場合と同じ

## 既存部品への要望

- N1f の速度は `nvm::model_full` の速さでほぼ決まる (128² で 1 面 0.16 秒)。N1e の REPORT にあるとおり、未ベクトル化なので高速化してほしい
- `nv_flat.h` の `transform` は mode 0..3 以外を黙って恒等にする。プロファイル側で範囲を検査しているので変更は不要だが、API に注記があると良い
- 統合時: `native/Makefile` に N1f の規則 (Makefile.profile の `nv_profile_test`) を加え、`make verify` に `verify_profile.py` を入れてほしい
  (Python の期待値の初回計算に約 4 分かかるので、キャッシュを前提にするか `--quick` を推奨。Makefile.profile は .gitignore の `native/**` に掛かる点に注意)
