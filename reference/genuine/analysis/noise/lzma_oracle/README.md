# LZMA オラクル (BuildNoiseModel のプロファイル 0x18..0x23)

本家 `FUN_009b41c4` は、領域を 16 ビット型 (`FUN_009f0554`: rint(x·16)、int16 に飽和、NaN → 0) に写し、R・G・B の各面
(W·h·2 バイト、行の詰め物なし) を `FUN_009d61f4` で圧縮して、`16 + 圧縮長` をプロファイル +0x18 / +0x1c / +0x20 に書く。
`FUN_009d61f4` は `LzmaCompress(level 5, dict = 長さを 4 KiB に切り上げ (最大 128 MiB), lc 3, lp 0, pb 2, fb 32, 1 スレッド)`
を呼び、出力は [元の長さ u32][圧縮長 u32][props 5 バイト][未記入 3 バイト][ストリーム]。出力の上限は 元の長さ + 0x80 バイト
(超えると SZ_ERROR_OUTPUT_EOF → -1 → 後始末 `FUN_009ba4cc` が 0x00..0x2b と 0xc2a4 を 0 にする)。

- `sdk920/` — LZMA SDK 9.20 の C ソース (必要なファイルのみ。パブリックドメイン)。入手元 https://www.7-zip.org/a/lzma920.tar.bz2
  (SHA256 `8ac221acdca8b6f6dd110120763af42b3707363752fc04e63c7bbff76774a445`)。各ファイルの SHA256 は `SHA256_sdk920.txt`
- `sdk1604/` — LZMA SDK 16.04 (同上、https://www.7-zip.org/a/lzma1604.7z、SHA256
  `edf8690e9d5ae0dbe80751e3df247bb4e1b497b100c4af5b95fc50a5c2e5b04f`)。版の比較用
- `lzo.c` — オラクル本体 (本家は読み込まない)。`make` で `lzo_920` / `lzo_1604` を作る
- `verify_lzma.py` — `repro/nv_lzma.py` (9.20 の Python 移植) をオラクルと本家 (`nvh profile` が書く out.bin.lzma0/1/2) に照合する

版の同定 (2026-10-16、観測): 本家の圧縮データ (test_v02 n=312 の 128² の 3 面) とバイト単位で一致したのは 9.20 と 16.04。
19.00 と 7-Zip 公式リポジトリ (github.com/ip7z/7zip) の 21.07・23.01・24.09・26.04 は別のストリーム (7 〜 118 バイト短い) を出す。
本家に入っている SDK の版そのものは特定していない (16.04 以前で 9.20 と同じ出力の版)。
