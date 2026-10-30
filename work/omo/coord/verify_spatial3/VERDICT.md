# VERDICT: wp_spatial / spatial-align 主張の独立検証 (verify_spatial3, 2026-10-21)

## 0. 検証した対象
- 写し: `src/nv_spatial.py` SHA-256 `f2ed47b325886f6583ea8d4c399c2bc323e1b3adf4f492963872a34870be56cc` (15:1x に wp_spatial/src から複写、以後これを検証)。
  `base/` は repro/ から直接コピー (nv_pixnoise・nv_lines・nv_band・nv_curve。nv_pixnoise は wp_spatial/base と cmp 一致)。
- **注意**: 検証の終わりに wp_spatial/src/nv_spatial.py は SHA `b356ab23…788a73` (15:29:48 更新) に変わっていた。差分は shrink/_directional/_detail 周り (組 3・cfg+0x90・A18 の追加)。
  葉 (leaf_*, strip_*, sigfuse_port, _sigfuse_small, sigma_level, ixform_neon, leaf_sel2, _fabs) の本体は写しと新版で**同一** (正規表現で関数ごとに比較: 全 24 関数 same)。
  よって (1) の結論は新版にも及ぶ。(3) の方向経路と (4) のログは新版・旧版のどちらに対するものか確定できない (ログは 13:32、写しは 15 時台)。
- nvh: `nvh selftest` 終了コード 3 を確認 (v3_leaves.py が毎回確認、ログ先頭 `selftest exit 3 OK`)。nvh 同時 3 本以内、面は最大 400×400、FindFlatArea は呼ばない。
- 書いたもの: `v3_leaves.py` (照合)、`v3_mutants.py` (感度試験)、`logs/v3_leaves_seed7770021.log`、`logs/v3_mutants.log`、`src_sha256.txt`、`file_sha256.txt`。
  キャッシュ `cache/` (本家出力。入力 `.in*` と `.out`)。

## 1. 主張 (1) 葉のビット一致 — CONFIRMED
実装者と別のシード (7770021)・別の入力・別のスカラー・別の寸法で、写しの関数を `nvh elem/strip3/sigfuse/ixform/sel2` の出力とビット単位 (float は 32 ビット、u8 は値) で比較した。
再実行: `cd coord/verify_spatial3 && python3 -I -B v3_leaves.py` →
```
elem 17 関数 (acc add affine blend clampmul fabs mul mulk recip rinv rstep scale sqr sqrtabs sqrtk sub u8): 各 77〜308 件 mismatched 0
ixform 30 / sel2 flag0 35 + flag1 35 / sigfuse 50 / strip3 box 108 + sq 132: mismatched 0
total cases 3316 mismatched 0
```
- 入力の種類: 正規乱数 (σ40)・一様 (−20〜280)・正の小値・特殊値 (±0, ±denormal, ±inf, 0x7f7fffff 近傍, 255.49998/255.50002, 4294967296, 1e10 など 26 種) ＋ NaN 6 種 (sNaN 0x7F800001・0xFFA00007・0xFF800001 を含む)・全ビット乱数・非正規化数・整数±2ulp。
  寸法 n = 1,4,6,10,11,19,32,35,100,257,1001 (NEON 部と端数部の両方)。strip3 は 2×4 〜 400×400、mode 0/1、sq は s = 1, 0.5, 2, −1.5, 0 も。sigfuse は n = 1〜3001、表に NaN を混ぜたもの含む。sel2 は stride = n+0〜4、flag 0/1、i3 = i4 の混在。
- rstep の NaN 扱い (fcmp; b.le が順序なしで成立、sNaN を静化しない): 端数部に sNaN を含む入力で一致。端数部を旧式 (NaN で除算) に変えた変異は 31 / 231 件で検出 → この主張は本家で裏付けられた。
- sel2 の flag・stride・n のスタック配置: 逆アセンブル 0x98eba8〜0x98ec08 で確認 (観測)。`ldrb w9,[x29,#0x10]` → `strb w9,[sp]`、`ldp x27,x28,[x29,#0x18]` → `stp x27,x28,[sp,#8]` (stride = +8、n = +0x10 相当の第 10・11 引数)。FUN_009f19cc は NEON 判定バイト 0x14a67a8 が非 0 のときだけ呼ばれる (既知の判定)。
- 感度試験 (`v3_mutants.py`、`logs/v3_mutants.log`): rstep NaN 旧式 31/231、rinv NEON→真の除算 167/308、recip 同 210/231、blend 係数取り違え 226/231、box の mode 反転 98/108、rms の s 無視 24/132、sel2 の flag 無視 35/70、ixform 出力入れ替え 30/30、u8 を floor に 51/77、sigfuse 1ulp ずらし 49/50。照合は空振りしていない。
- 未照合 (本検証の範囲外・制限): (a) ixform と sel2 は、移植が n&~3 (NEON 部) だけを返すので**端数 (n%4) の出力は比べていない** (elem は端数も比べた)。
  (b) strip3 は W<4 または H<2 (移植が NotImplementedError)・512 超は未試験。(c) 本家は 1 スレッド・この環境の NEON 経路のみ。
  (d) sigfuse の n<5 は本家が n を 4 の倍数に切り上げる経路との比較であり、sigma_level の実際の小 n 経路 (FUN_009f02b4 + 非融合) の本家照合ではない。

マージしてよい: 葉 17 関数 + strip_box3/strip_rms3 (mode 0/1) + sigfuse_port + ixform_neon の NEON 部 + leaf_sel2 の NEON 部は、本家 nvh とビット一致 (本検証 3316 件・不一致 0、実装者の 2566 件と合わせて独立に再現)。

## 2. 主張 (2) 静的な読み — PARTIAL
逆アセンブル (/tmp/nv_expand/disasm_full.txt) と __TEXT のバイト (`xxd` で NeatVideo6.arm64.ofx) で抜き取った結果:
- 組の選択表 0x1246c48: **CONFIRMED**。0x99f53c〜0x99f56c で アドレス = 0x1246c48 + 色モード(profile+0x48)·0x48 + 品質(s+0x104)·0x18 + 8·面。27 個の 8 バイト値 = モード 0: (2,2,2)(3,3,3)(4,4,4)、モード 1・2: (2,1,1)(3,2,2)(4,2,2)。
  写しの SET_SELECT と一致 (既定 YCrCb・品質 0 で Y=組 2、Cr/Cb=組 1)。差: 本家は品質をクランプしていない (移植は min(…,2))。範囲外の品質は未定義とみなしてよい。
- 段数 (levels_from_settings): CONFIRMED。0x99f580〜0x99f634 で flag&1→5、+0x2c>0→5、+0x28>0→4、それ以外は w12+1 = 3。
- cfg 雛形 0x12445a8: 先頭 = 0x3f87ae14 (1.06f)、続く語は 1.0f が並ぶ。写しの `sigma=(1.06,1,1,1,1)` と一致 (全 0xd0 バイトの各欄の割り当ては未照合)。
- σ 補間表 FUN_0099e6f4 の本体: CONFIRMED。b = min(fcvtzs(i·0.03125), 7)、frac = x − b、v_b = c_b·f_{b,k}、v_{b+1}、結果 = v_b + frac·(v_{b+1} − v_b) が非融合の fmul/fsub/fadd。257 点 × 面 3 本が 0x404 バイト刻み (k 主)。写しの `lum_tables` の補間部と一致。ゲイン (FUN_0099e264) と 2 乗は未確認。
- σ 係数 FUN_009aae90 (0x9ab484〜): a_j² (=モデル語²)、1/((a_R²·m0 + a_G²·m1) + a_B²·m2) の加算順は写しと一致。ただし**モデル語 5+k の添字そのもの (オフセット 0x349c, 0x53f0 など) は復号しておらず未確認**。
- 局所 RMS の 0 詰め (FUN_009950f4 → 9ed164) は呼び出しの引数 (w4=1, w5=1 at 0x991f54〜0x991f58) だけ確認。9ed164 の本体は読まず、strip3 sq の本家一致で代替 (mode の意味が写しと本家で一致すること)。
- 混合係数 (設定構造体 +0x10c..+0x11c)・設定構造体の既定値: 未確認 (実装者自身も「既定値は未特定」と書いている)。
- 保護箇所の取り扱い: 上の範囲を読む際、**表示範囲に入ってしまった**もの = 0x991f80〜0x991fe8、0x992058〜、0x99e6f4 の入口付近、0x99e7b8〜、0x99f528、0x9ab4b0〜0x9ab50c と 0x9ab4ec〜0x9ab508 (image+0x1561000 ページ参照)・0x9ab588〜0x9ab5b0 (ハッシュ書き込み様の命令列)。
  いずれも解析・追跡はせず、その先は読まずに止めた。結果に使ったのは非保護の演算命令だけ。指定の禁止範囲 (0x990f74 入口・0x991000〜0x991080 ほか) は読んでいない。

## 3. 主張 (3) 方向経路 — PARTIAL (抜き取りで一致した部分は確認、全体は未確認)
確認できた (逆アセンブル、写しの `_directional` と突き合わせ):
- Wc = rinv(4.5·t, W, R): 0x991ac4〜0x991adc で s8 = s13·(s0·1.1)、`fmov s0,#4.5; fmul s0,s8,s0`、`bl 0x9eef5c`。**定数 4.5 CONFIRMED**。
- 方向ごとの流れ: 0x9944c0 (線フィルタ、半径 4、mode 1) → フラグ(組表の byte)≠0 なら sub・rms(1.0)・0x9ef0d0(0.75t)、0 なら **0x9ef260 (rstep, 引数 s8=t)** → 2 段目の線フィルタ (表 0x1243700+0x260·組+0x348/0x3c8 の添字、−1 なら 1 段) → 2 乗 (9ed2f0) → 0x9950f4 に s = 1/|A| (`fabs; fdiv s0,s11,s0` 0x991f30〜0x991f38、A = [組·0x140 +…+4d] の語)。写しの E_d の式と一致。
- argmax = 0x9ee5fc: CONFIRMED (呼び出し 0x992000)。
- **定数 1.05 CONFIRMED** (0x3f866666: 0x992034 と 0x992770)。w = ramp(1.05·s13, 2·, c=1, d=0; best) (0x9ee93c の s2=1.0・s3=0)、m = ramp(1.05·s13·s0, 2·, c=0, d=1; best) (s2=0・s3=1)。写しの `ramp_ab(...,1,0)` / `(…,0,1)` と一致。f984>0 (表 +0x284) の分岐も一致。
- 構造マスクの平滑 (c3c>0): 0x9927a4〜0x992838: a = (S+0x460 の語)·c3c·0.5、wmul(a, 2a, 0, 1; sm, m) (0x9eebf8)。写しの 0.08·c3c·0.5 と一致 (S+0x1e0 = 0.08 は表の値、バイトは確認していない)。
- Xd = acc(−1, m, S, Wc) (0x9edeb8 の呼び出し、0x992844〜0x992854): 呼び出しの形は一致。
- 2 択 FUN_0098eba8: 葉は本家とビット一致 (1 章)。呼び出し側の flag = (k≠0) の根拠 (0x98ec04〜08 の配置は確認したが、k≠0 の出所は未確認)。
未確認: Rx 以降 (0x9928b8〜0x993d18 の大半: Rx の箱 2 回・wR の t4 = s13·|R|·1.1・dc 項 0x993a10〜0x993bbc・Pc/0x993bc0)、dfilt の第 2 段、モデル語の添字 `80+80·sid+16k+d`・`480+5·sid+k`・`55+5·sid+k` (逆アセンブルで復号していない)、cfg+0x28/+0x78/+0x90 の分岐。**方向経路全体を本家と数値で照合した記録はない** (本家の経路にはハーネスで呼べない部分が含まれる)。
注意: 写し (f2ed…) は組 3・4 と c78>0・c90≠0 を NotImplementedError にする。新版は組 3 と cfg+0x90 を追加済みだが、本検証は新版の追加部分を見ていない。

マージしてよい: 4.5・1.05・ramp の引数・argmax・rstep/rinv/acc の呼び出し形、組選択表、段数、σ 補間の式。
マージしてよくない (「確認済み」と書かない): 方向経路の dc 項・Rx/wR・モデル語の添字・混合係数・設定構造体の既定値。これらは「静的に読んだが本検証では未確認」と書く。

## 4. 主張 (4) 区間 PSNR の変化 ≤ 0.004 dB — CONFIRMED (数値の読み取りのみ。解釈に制限)
`logs/run_ab_{296,560}_{bitexact,align}.log` の値 (Y):
| 行 | A bitexact→align | B bitexact→align |
|---|---|---|
| genuine-default | 46.311 → 46.315 (+0.004) | 46.724 → 46.727 (+0.003) |
| genuine-t1.5 | 46.996 → 46.998 (+0.002) | 47.157 → 47.158 (+0.001) |
| U / V (default) | 50.426→50.427 / 50.041→50.042 (A) | 50.393→50.394 / 50.063→50.063 (B) |
v5-default は同一 (47.117 / 47.222)。変化はすべて 0.004 dB 以内で、主張どおり。制限:
- ログの時刻 (13:32) は src の最終更新 (15:29) より前で、どの版のコードで測ったかはログから確定できない。
- 本検証では PSNR を再測定していない (数値の確認のみ)。
- "genuine" 行 (移植) は v5 既定より低い (A: 46.31 対 47.12、約 0.8 dB。t1.5 で 0.12 dB 差)。0.004 dB の変化は「葉の修正が全体の結果をほとんど変えない」ことを示すだけで、方向経路や移植全体が正しい証拠ではない。

## 5. まとめ
| 項目 | 判定 |
|---|---|
| (1) 葉のビット一致 | CONFIRMED (3316 件・不一致 0、感度試験 10 種で検出、端数 n%4 の ixform/sel2 は未比較) |
| (2) 静的な読み | PARTIAL (組選択表・段数・σ 補間・雛形先頭は確認。モデル語の添字・混合係数・0 詰め本体・設定既定値は未確認) |
| (3) 方向経路 | PARTIAL (4.5・1.05・ramp 引数・argmax・流れの前半は確認。後半・dc 項・添字は未確認、全体の数値照合なし) |
| (4) 区間 PSNR | CONFIRMED (ログ値のみ。版の対応は不明) |

仮定: 設定構造体の既定値 (実装者が「未特定」と記述)、トークン表が加算 0 になるという仮定 (nv_spatial の ixform のコメント)、17−popcount 係数 = 1。いずれも本検証では触れていない。
