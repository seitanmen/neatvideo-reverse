# VERDICT: wp_aft (AutoFineTune 静的移植) 独立検証 (2026-10-17, 検証者)

総合: 部分承認。類似度・worker・親の後処理・定数・シャッフルは一致。**曲線化 FUN_009b4e08 / 補間 FUN_009bdf30 の 2 点で移植と MERGE_PLAN/REPORT の記述が逆アセンブルと食い違う** (REFUTED)。この 2 点を直すまで nv_aft.py の curve_post / curve_fill と MERGE_PLAN §2 の該当行はマージしない。
本家の AFT は実行していない。本家を呼んだのは `nvh selftest` (終了コード 3 を確認) と `nvh aftshuffle` だけ。

## 判定表
| # | 項目 | 判定 |
|---|---|---|
| 1 | 類似度 R 語 = 語 490/491、許容語の読み | CONFIRMED |
| 2 | 定数 (3,2,3) / (4,3,4) / Q66 | CONFIRMED |
| 3a | 9b4e08 位相 3 は非フラグのビンそれぞれに avg×level | CONFIRMED |
| 3b | 「ビン 8 は位相 2 が書かない」 | **REFUTED** |
| 3c | (追加) 段係数 5 語は 0 ではなく **1.0f** | **REFUTED** (移植は 0) |
| 4 | 親の後処理 (保護区間外) | CONFIRMED |
| 5 | worker の G→R→B、積 0.5、最後だけ b.le | CONFIRMED |
| 6 | verify_self 150 PASS の再現 | CONFIRMED。ただし空振り試験は PARTIAL (下記) |
| 7 | 区間 A の PSNR (variant +0.008) | CONFIRMED (区間 A のみ。B は未再測定) |
| 8 | nvh aftshuffle との照合 | CONFIRMED (110 件、不一致 0) |

## 1. 類似度 FUN_009b6338 (CONFIRMED)
`python3 -I ext.py 9b6338 9b69dc` で逆アセンブルを再抽出して読んだ (`a_9b6338.s`)。
- x1→x21 (タイルのモデル、分子)、x2→x20 (プロファイル面)、x3→x19 (許容 51 語)。s0=s_lvl、s1=s_mult。
- R 語: 0x9b64f4 `ldr s2,[x21,#0x7a8]` / 0x9b64f8 `[x20,#0x7a8]` = 語 490、0x9b655c/0x9b6560 `#0x7ac` = 語 491。ループ変数 i に依存しない (毎回同じ語)。語 474 (byte 0x768) は読まない。
- 方向語: `[x26-0x40]` (x26=x21+0x400) = byte 0x3c0 = 語 240+i、`[x26]` = 0x400 = 語 256+i。
- 許容語: x24=x19+0x44 で `[x24-0x40]` = stats[1+i]、`ldr [x24],#4` 後置 = stats[17+i]。CV 鎖は `x19+(l<<5)+0x88+4m` = stats[34+8l+m] (m=0..6)、入力語は `x21+(l<<5)+0x40+4m` = 語 16+8l+m。最後の `[x21+4l]/[x21+4l+0x14]` = 語 l / 語 5+l、許容は `x19+4l+0xc4` = stats[49+l]。ループ数 `[0x1244098]`=8 (項目 2 の読みで確認)。
- p5/p6 は語 5/6 の |t/p|、三つ目は語 6/5 の比 (ldp s0,s1,[x21,#0x14]; fdiv s1,s1,s0 = t6/t5)。nv_aft.similarity と一致。
- ペナルティ関数 9d49b4 も読んで nv_aft._penalty と一致 (s==0 分岐、v = (0.5/s/c)*x + ((c-1)*0.5)/c、b.mi→0、b.le→v、NaN は v のまま)。
- 細かい食い違い (軽微、実フレームに影響しない見込み): CV 鎖で許容 0 の直後の次語検査は asm では `b.pl` (0x9b65fc 他)。NaN では N=0 なので pl が成立して続行するが、移植は `not (wt >= 0)` で NaN を break する (コメントの「NaN でも break」は誤り)。
- 参考: 0x9b636c〜0x9b63c8 の入口にカウンタ書き込み様の命令列がある。ゲートではない。調査していない (対象外)。

## 2. 定数 (CONFIRMED)
`otool -l` で __const の addr 0xc93780 ↔ file offset 13186944 を確認し、`ファイルオフセット = addr - 0xc93780 + 13186944` で読んだ (arm64 スライス、本家は実行せず)。
- 0x12477dc (file 19167196) = (3.0, 2.0, 3.0)、0x12477d0 (19167184) = (4.0, 3.0, 4.0)、0x1244098 (19153048) = int64 8。
- worker の 0x9b5d14〜0x9b5d28: `0x7d0+mode*4` を s8 (=s_mult)、`0x7dc+mode*4` を s9 (=s_lvl) に読み、9b6338 へ s0=s9, s1=s8 で渡す。移植の S_MULT/S_LVL の対応と一致。
- Q66: 0x9b4e7c `mov w8,#0xf5c3; movk #0x3f28` = 0x3f28f5c3 = float32(0.66) (Python で 0x3f28f5c3 と確認)。クリップ閾値 0x3ea8f5c3 (0.33) も一致。

## 3. 曲線化 FUN_009b4e08
`a_9b4e08.s` を読み、制御の流れを Python で模擬した (模擬は `python3 -I` の 10 行。9 ビンの旗を与えて位相 2 の本体が走るビンを列挙)。
- 3a CONFIRMED: 位相 3 の走査 (0x9b5294〜0x9b5308) は 0x52f0 の `x22==8` で終了し、旗 `flag[x22+1]` を読んで x22 を進めてから本体へ入る。非フラグのビンそれぞれ (ビン 0 は入口、ビン 1..8) で本体が走る。本体の条件 (count>0 または +0x1c≠0)、level=avg×level、count=0、+0x1c=1、+0x1e=(3.0<値) は移植と一致。
- 3b REFUTED: 位相 2 (0x9b5180〜0x9b5258) の `cmp x9,#0x100; b.eq` は**本体の後**にある。x9=0xe0 (ビン 7) の後で旗[8] を読み x9=0x100 にして本体 (0x9b5188) へ入り、その後で終了する。模擬: 旗=全て 1 → 本体はビン [0..8]、旗=ビン 8 のみ → [8]。q8 は sp+0x44 に保存され本体の x12 がそこを指す。Ghidra の逆コンパイル (`rtti/noise/fn_0x9b4e08.c` の LAB_009b5238: `if (lVar7 == 0x100)` は本体の後) も同じ。よって**ビン 8 は位相 2 が書く** (旗があるとき)。移植は `for k in range(8)` で書かない。REPORT §2・MERGE_PLAN §2 の「ビン 8 は位相 2 が書かない (本家の癖)」は誤り。
- 3c REFUTED: 位相 2 の 0x9b510c の `_memset_pattern16(sp, 0xc962e0, 0x14)` と位相 3 の 0x9b52d0〜0x9b52d8、9bdf30 の全 memset_pattern16 7 か所 (すべて adrp 0xc96000+0x2e0 を渡す) の模様は、ファイルから読むと `0000803f ×4` = 1.0f ×4。つまり段係数 5 語は **1.0f** に書かれる。移植は 0 (curve_post の 2 か所と curve_fill.write)。AGENTS.md §6.5 の「段係数は 1.0 のまま」とも矛盾する。MERGE_PLAN/notes の「係数 = 0」も誤り。

## 4. 親 FUN_009a775c の後処理 (CONFIRMED、0x9a7828〜0x9a79f0 以外)
`a_9a775c_tail.s` (0x9a79f0〜0x9a80d8)。
- 成功 (w23≠0) かつ mode≤1 (`cmp w3,#1; b.hi`): 9b4e08 を (ctx+0x259f8 / +0x25a88 / +0x25b18, profile+0xbf44 / +0xc064 / +0xc184, w3=mode) で 3 回 → 9bdf30 を s0=0x3e4ccccd (0.2)、s1=3.0、x1=同 3 曲線、w2=0 で 3 回。
- mode==2: 出力ポインタ 3 個が非 NULL のときだけ 9b5408 を 3 回 (結果を各ポインタへ)。
- 0x9a8004 の `strb wzr,[prof,#0x2d]` は、成功・mode≥3・失敗 (`cbz w23` → 0x9a8004) のいずれの経路もここを通る。ただし保護区間 0x9a7828〜0x9a79f0 の手前で抜ける経路があるかは見ていない (対象外)。
- 注意: 0x9a7d94〜0x9a7dac に 0x1561000 ページの読みがある行を、抽出の確認中に見かけた。解析していない (対象外)。

## 5. worker FUN_009b56f4 (CONFIRMED)
- 面の対応: 0x9b5a18 の 9c3de4 の引数 (5f38, 5f30, 5f28) がタイル構造の面 0,1,2 になる (+0xb0/+0xb8/+0xc0)。類似度の呼び出しは 1 回目が x26+0x5f30 (プロファイル面 1 `+0x1fa0`、許容 `+0xbd10`) = G、2 回目が 5f38 (面 0 `+0x4c`、許容 `+0xbc44`) = R、3 回目が 5f28 (面 2 `+0x3ef4`、許容 `+0xbddc`) = B。順序は G→R→B。
- ゲート: G 後 `fcmp s0,s12(0.5); b.mi` (0x9b5d70)、R 後 `fmul; fcmp; b.pl` (0x9b5dc0〜)、B 後も b.pl。最後の 0x9b60b0 `fcmp s10,s12; b.le` だけが厳密 (NaN も拾う)。
- 追加は R→bins ch0 (モデル +0x3eb0=R モデル、面 0 の語 0)、G→ch1、B→ch2 の順で `|model.語0|/|face.語0|`。移植と一致。クリップは `b.gt` (NaN 素通り)、ビン計算 ((m+16)*0.03125, fcvtzs, 8 で飽和) も一致。
- 差分検査 (image2) は asm の b.mi / b.gt の並びを確認したのみ (完全な突き合わせはしていない)。

## 6. 自己一致試験 (CONFIRMED / 空振り PARTIAL)
- `wp_aft` の src/・verify/・base/ を `cp/` に複写し (base の 14 ファイルは repro/ と cmp 一致)、`cd cp/verify && PYTHONDONTWRITEBYTECODE=1 python3 -I verify_self.py`: **150 PASS、FAIL 0、exit 0** (`log_verify_self.txt`)。t_readonly は logs が無いコピーでは走らない。
- 誤り注入 7 種 (`mut/`、各 150 試験を完走、`log_mut_*.txt`):
  | 注入 | 結果 |
  |---|---|
  | R 語 490→474 に戻す | 検出 (self-accept 6 件 FAIL) |
  | シャッフル初期値 0x7b→0x7a | 検出 (shuffle n=3 の手計算) |
  | 位相 2 の閾値 4→5 | 検出 (7 件) |
  | 位相 2 でビン 8 を書く (**本家の正しい挙動**) | 検出される = 試験が誤った信念「ビン 8 は書かない」を固定している |
  | 段係数 0→1 (**本家の正しい挙動**) | **未検出** (試験が係数を見ていない) |
  | S_LVL mode1 の 2.0→3.0 | **未検出** |
  | G ゲート 0.5→0.6 | **未検出** |
- 結論: 試験は「実装と移植者の読みの整合」を見るもので、本家との一致証拠ではない (REPORT も未照合と明記)。定数・ゲート閾値・係数は試験の盲点。

## 7. PSNR (CONFIRMED、区間 A のみ)
`cp/verify/eval_A_only.py` (eval_v5_aft.py のコピー。ROOT 固定、並列 8→3、区間 A のみ) を実行 (`log_eval_A.txt`, `cp/logs/eval_A_only.json`):
baseline Y/U/V = 47.117 / 50.985 / 51.179、variant = 47.125 / 50.986 / 51.179 (Y +0.008 dB)。受入 275/1980、aft_curve [0.8078 … 0.7637]。REPORT と数値一致。区間 B は再測定していない。
注意: この測定は上記 3b/3c の食い違いを直す前の移植で行われている。曲線の水準 (ビン 8 の値) は修正で変わり得る (ビン 8 は補間でなく分位点になる場合がある)。係数は PSNR 経路 (level のみ使用) に影響しない。

## 8. aftshuffle (CONFIRMED)
`nvh selftest` 終了コード 3 を確認後、`shuf_cmp.py` で N = 乱数 100 個 (1..4096, seed 20261017) + 境界 10 個 (0,1,2,3,1023,1024,1025,2048,4095,4096) の計 110 件を `nvh aftshuffle N` と `nv_aft.shuffle_order(N)` で比較 (出力は「i 値」行、i も検査): **不一致 0**。統括の 84 件と合わせ、FUN_009d39ec の移植は本家とビット一致とみなせる。

## マージしてよい
- `shuffle_order` (本家照合済み、194 件)。
- `similarity` / `_penalty` / `_dev` の構造 (R 語 490/491、許容語、定数)。CV 鎖の NaN 分岐は軽微な差として注記するか修正。
- worker の順序・ゲート (G→R→B、0.5、最後だけ厳密)、親の後処理の呼び出し構造 (保護区間外)。
- 定数 S_LVL/S_MULT/Q66/NDIR (静的読みで確認)。AGENTS.md への追記案のうち、R 語・許容語・定数の行。
- 「AFT 全体は本家未照合」「v5 変種は同等 (+0.008 dB)」の記述 (数値は再現)。

## マージしてはいけない (直すまで)
- `curve_post` の位相 2 から「ビン 8 を除く」処理 (正: ビン 8 も旗があれば書く) と、MERGE_PLAN §2 の「ビン 8 は位相 2 が書かない」行。
- 段係数を 0 にする 4 か所 (`curve_post` 位相 2・位相 3、`curve_fill.write`)。正: 1.0f。MERGE_PLAN/notes の「係数 = 0」も訂正。
- verify_self の「post bin8 never written」試験 (誤った信念を固定している)。係数・定数・ゲート閾値の期待値を足す (逆アセンブル由来であることを明記)。
- 区間 A/B の PSNR は修正後に再測定してから exp_results.txt に載せる。
- `curve_fill` (9bdf30) は本検証では PSNR 経路の挙動 (結び目、外挿の個数) の突き合わせをしていない。係数 1.0 だけ全 7 か所の書き込みで確認した。

## 対象外として触れなかったもの
0x9be22c〜0x9be29c (9bdf30 内)、FUN_009d41b0 / FUN_009d4708 内の検査、0x1488404、0x1561000 ページ、image+0x16af38。(抽出ファイル `a_9bdf30.s` には 0x9be22c〜0x9be29c の行が含まれるが読んでいない。0x9a7d94〜0x9a7dac の 0x1561000 ページ参照行は別件の確認中に目に入っただけで、解析していない。)

## 成果物
`/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/verify_aft/` : VERDICT.md、ext.py (逆アセンブル抽出)、a_*.s、shuf_cmp.py、run_mut.sh、mut/、cp/、log_*.txt。
