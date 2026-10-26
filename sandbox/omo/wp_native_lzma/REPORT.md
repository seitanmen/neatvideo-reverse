# wp_native_lzma 報告書 (N1c: LZMA 圧縮 nv_lzma ↔ SDK 9.20)

日付: 2026-10-26。担当: N1c 作業エージェント (サブエージェント)。報告ファイルは書けなかったため、最終応答を統括が転記した。
指示: `.claude/tasks/native_n1_split.md` の共通規約・合格ライン・N1c と統括の追加の約束。
観測 (実行して確かめたこと) と推定を分けて書く。コミット・git 操作はしていない。repro/・neatvideo_ofx_614/ は読んだだけ。本家バンドル・nvh は使っていない。

## 結論
- `make -C native -f Makefile.lzma && python3 native/verify_lzma.py` が **949/949 バイト一致、不一致 0** (観測、`verify_full.log`)。
  - Python (`repro/nv_lzma.py`) との比較 667/667 (+ 既知の相違 1 件: 長さ 0 の入力。下の「Python 側への不具合報告」)
  - 本家が保存した LZMA データとの比較 258/258 (重複を除いた全件) と、領域が分かっている切り出しの plane_int16 + blob 24/24
- 感度試験: ラッパに 18 種の変異を入れたうち 17 種を検出。残る 1 種は等価な変更。

## 作ったファイル
| ファイル | 内容 |
|---|---|
| `native/lzma/` | LZMA SDK 9.20 の C ソース 12 ファイル (無改変) + 配布物の `lzma.txt` (LICENSE 節) + `SOURCE.md` (出典・SHA256)。`obj/` はビルド物 |
| `native/nv_lzma.h`・`native/nv_lzma.cpp` | 薄いラッパ (名前空間 `nvlz`): `dict_size_for`・`plane_int16`・`compress`・`blob`・`blob_size` |
| `native/nv_lzma_main.cpp` | 試験用ドライバ `nv_lzma_test` (入力レコード列 → 出力レコード列。形式はファイル先頭のコメント) |
| `native/Makefile.lzma` | `make -C native -f Makefile.lzma` で `nv_lzma_test`。`clean`・`verify` もある |
| `native/verify_lzma.py` | 照合 (`--quick`・`--jobs J` (最大 4)・`--no-genuine`・`--verbose`。環境変数 `NV_LZMA_EXE` で別のバイナリ) |
| `sensitivity.py`・`sensitivity.log` | 感度試験と結果 |
| `verify_full.log` | 最終の全体実行のログ |
| `tmp/` | 一時ファイル (フレームのキャッシュ `*.npy`、変異体 `mut/`)。消してよい |

`native/Makefile` は編集していない。

## SDK の出典と SHA256 (詳細は `native/lzma/SOURCE.md`)
- LZMA SDK 9.20 (Igor Pavlov、パブリックドメイン)、https://www.7-zip.org/a/lzma920.tar.bz2
  - 配布物の SHA256 `8ac221acdca8b6f6dd110120763af42b3707363752fc04e63c7bbff76774a445`
  - ワークスペース内の写し 2 つ (`sandbox/omo/coord/verify_lzma/ref/`・`sandbox/omo/wp_lzma/dl/lzma920/arc/`) で再計算して一致 (観測)
- コピー元は `neatvideo_ofx_614/analysis/noise/lzma_oracle/sdk920/`。コピー前に `SHA256_sdk920.txt` で 12/12 OK、コピー後の `native/lzma/` でも 12/12 OK、配布物を展開した `C/` の同名ファイルと `cmp` で 12/12 同一 (観測)
- 写したファイル: Alloc.c/.h、LzFind.c/.h、LzHash.h、LzmaDec.c/.h (LzmaLib.c の `LzmaUncompress` が参照する)、LzmaEnc.c/.h、LzmaLib.c/.h、Types.h。`LzFindMt.*` は `-D_7ZIP_ST` で参照されないので写していない
- 各ファイルの SHA256 の全桁は SOURCE.md と SHA256_sdk920.txt

## ラッパの設定 (本家 `FUN_009d61f4` と `repro/nv_lzma.py` に合わせた)
- 呼び出し: `LzmaCompress(dest, &destLen, src, n, props, &5, level 5, dict_size_for(n), lc 3, lp 0, pb 2, fb 32, numThreads 1)`。BT4・最適解析・mc 32 は 9.20 の `LzmaEncProps_Normalize` が level 5・fb 32 から決める。9.20 の構造体なので reduceSize はない
- `dict_size_for(n)` = `(n + 0xfff) & 0xfffff000`。`n + 0xfff >= 2^27` なら 128 MiB
- `blob`: 容量 n + 0x90 のバッファの +16 に、destLen = n + 0x80 で圧縮する。失敗 (0 以外) なら false (Python の None、blob_size −1)。ヘッダは [n u32][ストリーム長 u32][props 5][0 × 3] (バイト 13..15 は本家ではヒープの残骸、Python は 0)
- `compress`: 出力上限 2n + 0x90 (`lzo.c` と同じ) の制限なしの圧縮で、props とストリームを返す
- `plane_int16`: x·16 を float32 で計算 (溢れは inf)。NaN → 0、+inf → 32767、−inf → −32768、それ以外は `std::rint` (偶数への丸め、`np.rint` と同じ)。[−32768, 32767] に飽和、int16 LE

## 検証 (観測、`verify_full.log`)
### (1) Python との比較 — 667/667 一致
各入力で plane_int16 のバイト列、compress の props とストリーム、blob のバイト列 (または −1) をすべて比較。
- 面 572 件:
  - 境界 11 件 (0・−0・NaN・±inf・1e30・3.4e38・非正規数、飽和の縁 ±2047.9375・2047.96875・±2048・−2048.03125・−2048.0625、rint の中間値 (k+0.5)/16 を 2000 個とその次の float、NaN のビット列、全面 NaN / +inf / −0、±2049 の掃引)
  - 乱数 520 件 (1×1〜256×256、8 割は 96×96 以下)
  - 実写 40 件 (test_v02 n=0/312/576・test.mp4 n=0/300 の rgb48le/257 の float32。32²・48²・64²・97×71・128² (R/G/B)・200×150)
  - 512² の白色ノイズ 1 件
- バイト列 96 件 (1〜32・47〜257 バイトの乱数と 0、辞書の境界 4095〜49153 バイト、繰り返し・テキスト・乱数 (上限超え) など)
- 出力上限の境界 15 件: ストリーム長 − n が 0x7e〜0x82 になる入力を各 3 件。0x7e〜0x80 は収まり、0x81・0x82 は −1 で、両者一致 (SDK の `MyWrite` はちょうど destLen なら成功)
- blob が収まったのは 651 件、上限超えで −1 は 17 件

### (2) 本家の保存データとの比較 — 258/258 + 24/24 一致 (本家は新たに実行していない)
- 既存の `*.lzma0/1/2` 717 ファイル (`model_verify/tmp` 456、`wp_speed/tmp/mv` 228、`wp_lzma/data/boundary` 15、`coord/verify_lzma/nvh` 9、`coord/verify_integrate/nvh` 6、`wp_integrate/tmp` 3)。バイト 13..15 を除いて重複を除くと 258 件 (2,048〜524,288 バイト、計 10,250,646 バイト)
- Python 標準の `lzma` (FORMAT_ALONE) で復号し、`nv_lzma_test` で圧縮し直して、バイト 0..12 と 16.. を比較: 258/258 一致
- 領域が分かっている 8 領域 × 3 面 (f100.rgb.f32 の (700,200) 32²/45²/46²、frame312.rgb.f32 の (830,300) 32²/45²/46²/64²/512²): C++ の plane_int16 が本家データの復号と一致し、blob も一致 (24/24)。整数値のフレームなので rint の中間値はここでは試されていない
- 本家の −1 の実例は保存データになく、−1 の経路は Python とだけ照合

### 感度試験 — 18 種中 17 種を検出
- 検出: destLen n+0x7f / n+0x81、辞書の丸めなし / +4 KiB、fb 64、lc 2、pb 0、level 4、level 6 (n=0 の props だけが変わる)、rint → trunc / round、NaN → 32767、±inf の入れ替え、上下の飽和値のずれ、ヘッダの残骸バイト、エンディアン
- 未検出 1 種: x·16 を double で計算して 1e-9 を足す変異。×16 は float32 でも double でも正確で、1e-9 は float に戻すときに消えるので、等価な変更とみられる (推定)

## 計測時間 (M2 Max、負荷平均 4〜8)
- 全体 28.4 秒 (clean からのビルド込み 29.7 秒)。`--quick` 約 15 秒、`--no-genuine` も付けると約 5.5 秒
- Python nv_lzma 8.8 µs/バイト。Python 照合部の壁時計 20.8 秒
- C++: 9,277,926 バイトを 2 回ずつ圧縮して 1.68 秒 (約 0.09 µs/バイト/回、Python の約 100 倍)。本家データ 10,250,646 バイトで 3.17 秒
- 感度試験 18 種: 3 分 10 秒

## Makefile.lzma のルール
- `CXX ?= clang++`、`CC = clang`、`CXXFLAGS ?= -std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra`
- `SDK_CFLAGS = -O2 -ffp-contract=off -fno-fast-math -D_7ZIP_ST -Wall -Wextra -Wno-self-assign -Wno-unused-but-set-variable -Wno-misleading-indentation`
- `lzma/obj/%.o`: SDK の C を `$(CC) $(SDK_CFLAGS) -c`。`nv_lzma_test`: `nv_lzma_main.cpp`・`nv_lzma.cpp` と SDK の .o 5 個 (LzmaLib・LzmaEnc・LzmaDec・LzFind・Alloc)。`verify`、`clean`
- 警告の抑制は SDK 内の無害な 5 件 (`-Wself-assign` 3 件: LzmaLib.c 11・12 行・LzmaEnc.c 2081 行の `p = p;`、`-Wunused-but-set-variable` 1 件: LzmaEnc.c 1900 行の `btMode`、`-Wmisleading-indentation` 1 件: LzmaEnc.c 1342 行) の 3 種だけを名前で。抑制後は警告 0
- `-D_7ZIP_ST`: 本家は 1 スレッドで呼ぶ。付けないと LzmaEnc.c が LzFindMt.h を要求する
- 注意: `CXX ?=` は make の組み込み既定値のため効かず、実際は `c++` (macOS では clang++)。既存の Makefile と同じ

## Python 側への不具合報告 (直していない)
1. **長さ 0 の入力の props** (観測): Python の `nv_lzma.compress(b"")` は `5d 00 10 00 00` (辞書 4 KiB)、SDK 9.20 の `LzmaCompress` は `5d 00 00 00 01` (16 MiB。`dict_size_for(0)=0` を `LzmaEncProps_Normalize` が level 5 の既定 `1<<24` に置き換える)。ストリームは同じ (`00 00 00 00 00`)。原因は `nv_lzma.props_bytes` が辞書 0 を既定値に置き換えないこと。本家は同じ SDK を同じ引数で呼ぶので SDK 側と同じとみられる (推定、本家では未確認)。本家で起きるのは幅か高さが 0 の面だけで、FindFlatArea (48px 以上) と nv_profile (32 未満は NotImplementedError) では起きない。C++ は SDK のままにし、照合ではこの 2 値でストリームが同じときだけ既知の相違として別に扱う
2. それ以外に Python 側の誤りは見つからなかった

## 残る懸念・未検証
- 128 MiB の辞書上限 (n+0xfff ≥ 2^27) は Python が遅いので未照合 (本家の領域は最大 512² で 524,288 バイト)
- 本家の −1 の実例は保存データがなく、Python とだけ照合した
- 照合は macOS arm64 (clang -O2) だけ。符号器は整数演算だけなので他の環境でも同じになるとみられる (推定)
