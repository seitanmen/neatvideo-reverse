# ハーネス要求 (2026-10-21、spatial-align): 空間フィルタの葉 5 サブコマンド

wp_spatial の引き継ぎ (計画は `notes/align_plan.md`)。差分は `nvh_patch.diff` (`patch -p1` を作業ディレクトリ直下で。
`patch --dry-run` は通り、パッチ後の nvh.c は `clang -fsyntax-only -Wall` で警告なし。ビルドと make はしていない)。
以前の要求 (`nvh shrink`、却下済み) とその差分は `tmp/pre_align/` に移した。FUN_00990f74・FUN_0099c3a4・FUN_0099afb4・
FUN_0099a76c・FUN_00995250・AutoFineTune は要求しない。非葉 (FUN_0098edb0・FUN_009aae90・FUN_009950f4・FUN_00993df0・
FUN_00990da8・FUN_0098eba8・逆色変換 0x97b76c) も要求せず、その中の葉だけを呼ぶ。

## 要求するサブコマンド

| サブコマンド | 呼ぶ本家関数 | 用途 (align_plan の項) |
|---|---|---|
| `elem NAME n out s0 s1 s2 s3 in1 [in2 [in3]]` | 下表の 17 個 (要素ごとの関数。スカラーは float32 の 16 進ビット) | 白色化 1、縮小の乗数 2f、累積 2g、σ マップ 3c、混合 4b、方向経路 6b |
| `strip3 W H plane.f32 box\|sq mode sbits out` | FUN_009ed014 (box) / FUN_009ed164 (sq) を、呼び出し元 (0x98f378 / 0x995174) と同じ 4 列帯のループで | 3×3 箱 2a、局所 RMS 2e |
| `sigfuse n r g b tab coef out` | FUN_009f1d8c (σ マップの NEON 経路) | 3c |
| `ixform n y a b out` | FUN_009f1d0c (逆 YCrCb の NEON 部) | 4a |
| `sel2 n flag stride a b i3 i4 best src wt out` | FUN_009f19cc (FUN_0098eba8 の NEON 部、同じ引数) | 6b |

`elem` の NAME と関数: recip 0x9ed5f8 (s0; in, out, n)、scale 0x9ee304 (s0; in, out, n)、affine 0x9ee3fc (s0, s1; in, out, n)、
fabs 0x9ee50c、sqr 0x9ed2f0、sqrtabs 0x9ed3e0 (in, out, n)、sqrtk 0x9ed4e0 (s0; in, out, n)、u8 0x9f02b4 (in, out バイト, n)、
mul 0x9edc38・add 0x9ed844・sub 0x9ed710 (in1, in2, out, n)、mulk 0x9edd6c・rstep 0x9ef260・rinv 0x9eef5c (s0; in1, in2, out, n)、
blend 0x9ee1a4 (s0, s1; in1, in2, out, n)、clampmul 0x9ee7bc (s0..s3; in1, in2, out, n)、acc 0x9edeb8 (s0; in1, in2, in3, out, n)。
引数の役割は各関数の逆アセンブル (レジスタの使い方) と、空間段の呼び出しサイト (0x98f880・0x98f8a4・0x98fb14・0x98fc4c・
0x99036c・0x9916a4・0x991794・0x9917c0、FUN_009ae918 の FUN_009ee1a4、FUN_009aae90 の FUN_009edd6c) で確かめた。

## 安全性の根拠 (逆アセンブル `tmp/disasm_full.txt`、関数の範囲は `analysis/functions.csv`)

すべて: 範囲外への b/bl/blr/br なし、msr/mrs/svc/brk なし、ret は 1 つ。adrp は下表のものだけで、読むのは
NEON 判定のバイト 0x14a67a8 (ldrb) か、__TEXT 0x12504a0 の 1 語 (値 0x00000000、mode 1 の端の 0 として ld1.s で読むだけ)。
大域への書き込みなし (書き込みはすべて引数のポインタ経由)。__stack_chk_guard も使わない (スタック枠を作らない葉。9f19cc・9f1d8c は
callee-saved の退避のみ)。ハッシュ・トークン表・0x1487000/0x1488000/0x1561000 のページの参照はない。

| 名前 | アドレス | 大きさ | 外部分岐 | adrp (と直後の命令) |
|---|---|---|---|---|
| recip | 0x9ed5f8 | 280 B / 70 命令 | なし | 0x9ed5f8 adrp 0x14a6000 → add #0x7a8 → ldrb |
| scale | 0x9ee304 | 248 B / 62 | なし | 同 (0x9ee304) |
| affine | 0x9ee3fc | 272 B / 68 | なし | 同 (0x9ee3fc) |
| fabs | 0x9ee50c | 240 B / 60 | なし | 同 (0x9ee50c) |
| sqr | 0x9ed2f0 | 240 B / 60 | なし | 同 (0x9ed2f0) |
| sqrtabs | 0x9ed3e0 | 256 B / 64 | なし | 同 (0x9ed3e0) |
| sqrtk | 0x9ed4e0 | 280 B / 70 | なし | 同 (0x9ed4e0) |
| u8 | 0x9f02b4 | 188 B / 47 | なし | 同 (0x9f02b4) |
| mul | 0x9edc38 | 308 B / 77 | なし | 同 (0x9edc38) |
| add | 0x9ed844 | 308 B / 77 | なし | 同 (0x9ed844) |
| sub | 0x9ed710 | 308 B / 77 | なし | 同 (0x9ed710) |
| mulk | 0x9edd6c | 332 B / 83 | なし | 同 (0x9edd6c) |
| blend | 0x9ee1a4 | 352 B / 88 | なし | 同 (0x9ee1a4) |
| rstep | 0x9ef260 | 412 B / 103 | なし | 0x9ef264 adrp 0x14a6000 → add #0x7a8 → ldrb |
| rinv | 0x9eef5c | 372 B / 93 | なし | 0x9eef60 同 |
| clampmul | 0x9ee7bc | 384 B / 96 | なし | 0x9ee7bc 同 |
| acc | 0x9edeb8 | 404 B / 101 | なし | 0x9edeb8 同 |
| strip3 box | 0x9ed014 | 336 B / 84 | なし | 0x9ed02c adrp 0x1250000 → add #0x4a0 (0x12504a0、0 の 1 語) |
| strip3 sq | 0x9ed164 | 396 B / 99 | なし | 0x9ed178 adrp 0x1250000 → add #0x4a0 |
| sigfuse | 0x9f1d8c | 364 B / 91 | なし | なし |
| ixform | 0x9f1d0c | 128 B / 32 | なし | なし |
| sel2 | 0x9f19cc | 688 B / 172 | なし | なし |

## 入力の制限 (サブコマンド側で強制) と本家が落ちる入力

- `elem`: 1 ≤ n ≤ 4M。出力は n+4 要素を確保 (書くのは n 個)。
- `strip3`: 4 ≤ W ≤ 512、2 ≤ H ≤ 512 (呼び出し元が NEON 経路に入る条件 W ≥ 4・H ≥ 2 と、面 512px 以下の指示)、mode は 0 か 1。
  帯の左右の隣 (src[c−1]・src[c+4]) は first/last でないときだけ読むので、面の外は読まない。
- `sigfuse`: FUN_009f1d8c は 4 個ずつ n を切り上げて処理する (0x9f1ee4〜0x9f1eec の `cmp x9, x8; b.lt`)。入出力を 4 要素余分に確保し
  0 で埋める。表は 3 × 257 語 (添字は 8 ビットに切り詰められるので 0..255)。
- `ixform`: n & ~3 個だけ書く (0x9f1d0c の `and x8, x6, #~3`)。4 ≤ n。
- `sel2`: n & ~3 個 (0x9f19e0〜0x9f19e4)。src・wt は 256·stride 語 (添字 i + stride·byte)、n ≤ stride ≤ 65536。
- 既知の落ちる入力はない (上の制限内)。

## 照合の準備

照合スクリプト `verify/verify_align_leaves.py` を用意する (サブコマンドがなければ "harness pending" で終了。最初に
`nvh selftest` の終了コード 3 を確かめる。nvh の同時実行は 3 本まで)。本家の出力は `tmp/align_cache/` に置く。
