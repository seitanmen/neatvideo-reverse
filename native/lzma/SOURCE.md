# native/lzma/ — LZMA SDK 9.20 の C ソース (無改変の写し)

`native/nv_lzma.{h,cpp}` が使う LZMA 符号器。本家 (`FUN_009d61f4` → `LzmaCompress`) と同じ出力になる版として、
既存の照合 (AGENTS.md §6.5、`reference/genuine/analysis/noise/lzma_oracle/README.md`) で使われた 9.20 を使う。

## 出典

- 配布物: LZMA SDK 9.20 (Igor Pavlov)、`lzma920.tar.bz2`、取得元 https://www.7-zip.org/a/lzma920.tar.bz2
- 配布物の SHA256: `8ac221acdca8b6f6dd110120763af42b3707363752fc04e63c7bbff76774a445` (7-zip.org の公表値と一致。
  ワークスペース内の写し `work/omo/coord/verify_lzma/ref/lzma920.tar.bz2` と `work/omo/wp_lzma/dl/lzma920/arc/lzma920.tar.bz2`
  の両方で 2026-10-26 に再計算して一致)
- コピー元: `reference/genuine/analysis/noise/lzma_oracle/sdk920/` (既存のオラクル `lzo_920` の入力。`SHA256_sdk920.txt` に各ファイルの値)。
  コピーの前に同フォルダで `shasum -a 256 -c ../SHA256_sdk920.txt` が 12/12 OK、コピー後に `native/lzma/` で同じ一覧が 12/12 OK。
  さらに配布物を展開した `C/` の同名ファイルと `cmp` で 12/12 同一 (2026-10-26)
- `lzma.txt` は配布物の最上位にある説明書 (LICENSE 節を含む) を `cmp` で同一のまま写したもの

## ライセンス

配布物 `lzma.txt` の LICENSE 節:

> LZMA SDK is written and placed in the public domain by Igor Pavlov.

各ソースの先頭にも `Igor Pavlov : Public domain` とある。

## ファイルと SHA256 (コピー後の native/lzma/ で計算、コピー元と一致)

```
48dfee686e1b419ab9eb04e9335b587bff86f48766372c8729a9248ac750a6b5  Alloc.c
5b4a1af6a9e2c7d25748e89c9979112e3d43293cd1a7f4788b57e68ca0db0585  Alloc.h
34707623ca12f9091c9bc64b521c29a06236deba60427dd1413f9454c0d45209  LzFind.c
f66e314322c4f1e092ad6d694cced62a4548071746db46e00fb3ca9f31fa09c2  LzFind.h
8da146558014fb3c5a18ebe5744fc73e4f8da68777d8efe397a25e5c9d55d8dd  LzHash.h
b63e1c33d273cf4460da8bb94b148dfdf067e6ad3c4dd1725e33c5ceb1aefe3d  LzmaDec.c
f43966ab7858974a38f7a6c3c7dade35bcfb8763811e9cfa8690f416e7efb819  LzmaDec.h
e8793c1064c3b712a63957e2ee91553c383ac9d3784add3d7f165f038aea7d5d  LzmaEnc.c
5013a8d5db9010ec347e22ed265045e1f1497deb68102c641b4853c71d911fa0  LzmaEnc.h
17ff1a1c050e15b882994fd5c8aab134e55e43bcf553074c11dafc2d71bbe3d6  LzmaLib.c
ff1a71dfed8f6082c86ab07c08bfc873cceeee8fceff9d4ba0f6b5e82dd4b6d4  LzmaLib.h
1e367ddb4e774cabcc8f44321215d6593fb02dd799a4c7cb2a83b7b456768154  Types.h
ecf55f6328ffab36ec2702904e92128a9a2c237a9576922b347d129bb793e521  lzma.txt
```

確認: `cd native/lzma && shasum -a 256 -c ../../reference/genuine/analysis/noise/lzma_oracle/SHA256_sdk920.txt`

## 必要なファイルの理由

- `LzmaLib.c/.h` — `LzmaCompress` (本家が呼ぶ入口)。`LzmaUncompress` も同じファイルにあるので `LzmaDec.c/.h` も要る
- `LzmaEnc.c/.h` — 符号器。`LzFind.c/.h`・`LzHash.h` — BT4 の一致検索。`Alloc.c/.h` — `g_Alloc` (malloc/free)。`Types.h` — 共通型
- `LzFindMt.*` (複数スレッドの一致検索) は `-D_7ZIP_ST` で参照されないので写していない (本家は 1 スレッドで呼ぶ)

## ビルド

`native/Makefile.lzma` が `clang -O2 -ffp-contract=off -fno-fast-math -D_7ZIP_ST -Wall -Wextra` でビルドし、SDK 側の無害な警告 3 種
(`-Wself-assign`・`-Wunused-but-set-variable`・`-Wmisleading-indentation`) だけを名前で抑制する。ソースは編集しない。
