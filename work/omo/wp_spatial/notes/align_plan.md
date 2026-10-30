# 空間フィルタ移植を本家に合わせる計画 (align_plan, 2026-10-21)

作業者: spatial-align (wp_spatial の引き継ぎ)。観測 = 逆アセンブル (`tmp/disasm_full.txt`、arm64 スライス) と
__TEXT の静的データ (ファイルオフセット = アドレス) で確かめたこと。推定 = 意味づけ・未確認の仮定。
読まなかった範囲 (指示どおり迂回): FUN_00990f74 の入口 (0x990fdc〜0x991080 の難読化様コードと 0x991000〜0x991080)、
0x991500〜0x991554・0x9915c0〜0x991630・0x9916ec〜0x991744、0x1487000/0x1488000/0x1489000/0x1561000 のページと
image+0x156xxxx を参照する箇所 (各関数の入口・途中のハッシュ書き込み)、17−popcount 型の係数 (0x98f590〜0x98f718、
FUN_009aae90 の fVar54、FUN_009ae918 の iVar7)、トークン表 (FUN_0099c694 末尾、逆色変換 0x97b7a0〜0x97b81c)。

## 0. 全体の流れ (観測)

```
Filter 0x9ac2b0
  FUN_0099f79c(ctx)            プロファイルを ctx+0x114e8 へ複写
  FUN_0099e264(ctx, 0)         輝度別テーブル T (ctx+0xb8 / +0x14cc / +0x28e0、各 5 段 × 257 値) = (1.0·補間(profile+0xbf44))²
  FUN_0099ead0(ctx, 0)         cfg 3 面 (ctx+0x3d10 / +0x3de0 / +0x3eb0、各 0xd0 B) = 雛形 0x12445a8 × 設定 (ctx+0x3d00 が指す 0xb0c B の設定構造体)
  FUN_00addccc(…, 設定+0x104 (品質), (ctx[0x7f0] + 2·ctx[0x7f1])²·4, …)  チャンク分割
  スレッドプール → FUN_009ad8c8 → CPU FUN_009adc8c → チャンクごと FUN_009ae918
FUN_009ae918 (1 チャンク)
  段数 L = 設定+0x1c..+0x2c (帯域別 Amount) の最上位の非 0 帯域 + 1 (param_15&1 なら 5)
  σ マップ  FUN_009aae90(ctx, R, G, B, w, h, 追加面, σ0, σ1, σ2, 0, L)
  色変換    DAT_0147df50[色モード = ctx+0x11530 = profile+0x48](R, G, B → P0, P1, P2)
  各面 c    FUN_0098edb0(core, P_c, OUT_c, σ_c, w, h, ROI, cfg_c, model_c = profile 変換面 3+c のモデル (ctx+0x17330 + 0x1f54·c), var_c, flags)
            (param_19&1 なら面 1・2 は memcpy で素通し)
  混合      設定+0x10c が真で +0x110..+0x11c のどれかが 1 でないとき: 元の RGB を再変換し f_c = s[0x114+4c]·s[0x110] で
            OUT_c = (1−f)·元_c + f·OUT_c (FUN_009ee1a4)。var_c も同じ f で混合 (FUN_009987ac)
  逆色変換  DAT_0147df68[色モード]
```

旧ノートの「profile+0x110..0x11c」は誤りで、混合係数は**設定構造体** (ctx+0x3d00 → param_1[0x7a0]) の +0x110..+0x11c (観測)。

## 1. 対象ごとの計画

| # | 対象 | 本家の関数・アドレス・式 (観測) | 移植の現状 (`src/nv_spatial.py`) | 照合手段 |
|---|---|---|---|---|
| 1 | 白色化 | 0x98fae8〜0x98fb14 (非切り出し) / 0x98f86c〜0x98f8a4 (切り出し、行ごと): `FUN_009ed5f8(s0=0x38d1b717, σ_k, tmp, n)` = 先頭 n&~3 は d = ε+σ、FRECPE → FRECPS(d, e) → e·s、残りは 1/(σ+ε) の fdiv。続いて `FUN_009edc38(帯域_k, tmp, W, n)` = 帯域·tmp。切り出し経路では呼び出しごとの n が行の幅なので、NEON と端数の境界が行ごとに変わる | `bands[k] / (s + eps)` の真の除算 (不一致) | 葉 `nvh elem recip` / `elem mul` (要求) |
| 2a | 3×3 箱 (分解) | 0x98f378〜0x98f3b4: NEON かつ h ≥ 2・w ≥ 4 なら `FUN_009ed014(src+c, dst+c, W, h, mode 0, first, last)` を 4 列の帯ごと (最後の帯は w−4 に寄せて重ねる)。横 h = (x + 左) + 右、縦 ((h_r + h_{r−1}) + h_{r+1})、上端 (h0 + h0) + h1、下端 h_l + (h_l + h_{l−1})、× 0x3de38e39。端は複製。それ以外は FUN_0099ba54 → FUN_0099bf40 (分離型、mode 0) | 複製の分離型 (演算順が違う) | 葉 `nvh strip3 … box` (要求) |
| 2b | 2×2 縮小 | FUN_009f15b4 を行の組ごと | `halve` (2026-10-21 にビット一致) | 済 (`nvh down2`) |
| 2c | 帯域差 | `FUN_00990da8(粗, 現, 帯域, w_{k+1}, h_{k+1}, 奇数 w, 奇数 h)` (奇数フラグは FUN_0099c3a4 の +0x30/+0x68) | `cur − up2(coarse)` (拡大の式が違う) | `repro/nv_pixnoise.band_from` を写す (`nvh pixnoise` で全タイル一致済みの部品) |
| 2d | 拡大 | `FUN_00993df0(ACC, tmp, w_k, h_k, 奇数 w_{k−1}, 奇数 h_{k−1})` (0x98fcb4〜0x98fce8)。葉でない (FUN_009f173c・FUN_009f14e4 を呼ぶ) | (3,1)/4 の自前実装 (式・端が違う) | `nv_pixnoise.up2` を写す (照合済みの部品) |
| 2e | 局所 RMS | FUN_00990f74 内 0x9915a4: `FUN_009950f4(1.0, 箱文脈, W, &R, &tmp, r=1, mode=1)` → NEON 経路は `FUN_009ed164(1.0, src+c, dst+c, W, h, mode 1, first, last)` の 4 列帯: 横 (x² + 左²) + 右²、縦 ((h_r + h_{r−1}) + h_{r+1})、上端 (h0 + 0) + h1、下端 0 + (h_l + h_{l−1})、× (1/9·1)·1、fsqrt。**端は 0 詰め** | 複製の箱 → √max(·,0) (端・順が違う) | 葉 `nvh strip3 … sq` (要求) |
| 2f | 縮小核 (高速経路) | 0x99155c〜0x99168c: t = (cfg[k] + 1e-6)·(\|model[5+k]\|·1.1f)、2t = t + t。`FUN_009eebf8(t, 2t, 1, 0, R, W, X, n)`。0x991748〜: cfg+0xb8 ≤ 0 か amount = 1 なら `FUN_009ee304(−amount·(1−x20), X, dst, n)`、そうでなければ `FUN_009ee7bc(b8·(1−amt), −1, −1, −amt, P4, X, dst, n)` (P4 = 第 4 引数 = 98edb0 では tmp = 1/(σ+ε) の面とみられる。0x9916ec〜0x991744 は読んでいないので x20 = 0・x1 = X と仮定) | `wmul_genuine` は一致済み。乗数は v5 の amt | `nvh wmul` 済み、`elem scale` / `elem clampmul` (要求) |
| 2g | 累積と出力 | 0x98fc68: `FUN_009edeb8(1.0, dst, σ_k, ACC, ACC, n)` (NEON: ACC + 1·(dst·σ) を FMLA、端数: ((dst·σ)·1) + ACC。s = 1 なのでどちらも ACC + round(dst·σ))。拡大は k > 0 のとき次段の前に 2d。最後に `FUN_009ed844(入力, ACC, OUT, n)` | `acc − amt·X·s` (順・丸めが違う) | `elem acc` / `elem add` (要求) |
| 2h | 段の切り出し | `FUN_0099c694(rect[5], sizes, ROI, margin = DAT_012438d0[組] = (2,4,4,8,32))`: 段 k の矩形 = ROI を >>k して margin を足し面で切る。4 辺とも面の端に当たる段だけ「切り出しなし」(+0xa0+k = 0)。切り出しのある段は白色化・縮小・累積をその矩形だけで行い (行ごとの呼び出し)、縮小核の箱は矩形の端で 0 詰め | なし (全面) | 移植のみ (数値照合は 1 と 2a〜2g の葉) |
| 2i | 段数 0 の早退 | 0x98eeac〜0x98ef10: どの段も amount = 0 かつ cfg+0x90 = 0 で、適応 (b8 > 0) も var 出力もなければ OUT = 入力のコピー | なし | 移植のみ |
| 3a | cfg の構築 | `FUN_0099ead0(ctx, flag)`: 雛形 0x12445a8 (σ_cfg = 1.06,1,1,1,1、amount = 1×5、+0x50 = 1,1,1,0,0、+0xb8 = 0、+0xc0 = 4、+0xc8 = 2 …) を 3 面に複写し、設定 s で cfg[k] *= s[4]·s[8+4k]·s[0x30+4c]、amount[k] *= s[0x3c+4c]·s[0x1c+4k]·s[0] (s[0] = 0 なら amount = 0)、+0xc8 (方向組) = 表 0x1246c48[s+0x104 (品質)][色モード][c]、+0xb8 = s[0xcd] ? √(2·s[0xec]·s[0xd0]·s[0x128]² + 1)/(2·s[0x128]·s[0xec]·s[0xd0] + 1) : 0、+0xc0 = L | なし (v5 の P で代用) | 移植のみ (演算順は逆アセンブルで確定する) |
| 3b | 設定の既定値 | 設定構造体 0xb0c B (プリセット読み込み FUN_009bf2b8、正規化 FUN_009bf4d4)。品質 s+0x104 は「Normal (Faster)」=0 /「High (Slower)」=1、読み込み時に ≤ 1 に切り詰める (0x166548、0xb3460) | — | 既定値は未特定 (コンストラクタ未発見)。関数は設定を引数に取る。UI の名前との対応は推定 |
| 3c | σ マップ | `FUN_009aae90`: 段 k ごとに R,G,B を FUN_00990d2c で 1/2 縮小 (段 k+1 の大きさ)、a_j = profile 面 j のモデル語 5+k (q99)、行列 m = DAT_0147df80[色モード] (モード 1 は BT.601 係数の二乗)、c_cj = a_j²·m_cj / Σ_j a_j²·m_cj (·fVar54、正規時 1 と仮定)。σ_c = √(Σ_j c_cj·T_j[k][idx_j])、idx = 縮小 RGB の 8bit 化。NEON かつ n ≥ 5 は `FUN_009f1d8c` (fcvtnu → xtn → uzp1 で**下位 8 ビット** (256 以上は折り返し)、FMLA の順)、それ以外は FUN_009f02b4 (飽和) + 非融合の和 + FUN_009ed3e0。追加面があれば ×(1/255)·追加面 (FUN_009edd6c)。最後に FUN_00993df0 で段 k の大きさへ拡大して σ_c の段 k 位置へ | v5 の曲線 σ を縮小して代用 | 葉 `nvh sigfuse` / `elem u8` / `elem mulk` (要求)、残りは移植 |
| 3d | 輝度別テーブル | `FUN_0099e6f4(profile+0xbf44 の 9 ビン × 8 語, dst)`: 段 k の i = 0..256 で b = min((long)(i·0.03125), 7)、T = c_b·f_{b,k} + (i·0.03125 − b)·(c_{b+1}·f_{b+1,k} − c_b·f_{b,k}) (c = 語 0、f = 語 1+k)。`FUN_0099e264(ctx, 0)` が (1.0·T)² にする | なし | 移植のみ (FUN_0099e6f4 は入口にハッシュ書き込みがあり要求しない)。AutoFineTune 未実行のプロファイル (`nv_profile.build_profile`) では全ビン 1.0 → T = 1 |
| 3e | model・cfg の入力 | model_c = プロファイルの変換面 3+c (0x4c + 0x1f54·(3+c))。語 5+k = 段 k の q99。方向経路は語 80.. (A)、505.. (B)、905.. (C) | v5 の帯域比 | `repro/nv_profile.build_profile` (本家とビット一致) から作る |
| 4a | 色変換 | 前 DAT_0147df50 = (0x97b5a4 コピー, 0x97b5d0 YCrCb (NEON 部 FUN_009f1c7c)、0x97b718 平均系)。逆 DAT_0147df68 = (コピー, 0x97b76c (NEON 部 FUN_009f1d0c、端数は R = Y + Cr·1.4022 … の非融合)、0x97b924) | なし (YUV 面を直接処理) | 前: `nvh xform` 済み。逆: 葉 `nvh ixform` (FUN_009f1d0c、要求) |
| 4b | クロマ混合 | 上の「混合」(FUN_009ee1a4: NEON は FMLA(a·x, b, y)、端数は a·x + b·y) | なし | `elem blend` (要求) |
| 4c | チャンク分割 | FUN_00addccc (品質・面積 (ctx[0x7f0] + 2·ctx[0x7f1])²·4)。ctx[0x7f0] = max_c 表 0x1243740[組][L−1] (96〜192)、ctx[0x7f1] = max_c 表 0x1243808[組][L−1] (重なり 1〜128)。FUN_009adc8c がタスク (x0, y0, w, h) を切り出し (FUN_009c06cc)、ROI を渡し、FUN_009af2f8 で書き戻す | なし | 静的解読のみ (FUN_00addccc は未読。ROI は呼び出し側の計算を読んでから移植) |
| 5 | 極小面 | FUN_009f15b4 は H < 2 なら書かない、W < 2 なら 0 個書く。段の大きさ (FUN_0099c3a4) が 1 になる段が出るのは、チャンクが小さいとき (面の端のチャンク・48px 付近) | `halve` は 0 段のときコピー (本家と違う) | 呼び出し元のバッファ (FUN_009c80d4 の作業域) の前の内容を静的に追う。決まらなければ根拠付きで報告 |
| 6a | 方向組の選択 | 表 0x1246c48 (観測): 色モード 0 = (2,2,2)/(3,3,3)/(4,4,4)、モード 1 (既定) = (2,1,1)/(3,2,2)/(4,2,2)、モード 2 = モード 1 と同じ (品質 0/1/2 の順、面 Y,Cr,Cb)。品質は Demo の読み込みで ≤ 1 → **使われるのは組 1・2・3 (色モード 0 で 3)、組 4 (自己相似パターン保護 DAT_01243978 = (0,0,0,0,1)) は品質 2 のときだけ** | なし (組 0 相当の高速経路だけ) | — |
| 6b | 方向経路 | 0x991640〜: 方向数 > 0 (組の表 S+0x258) かつ model 語 A (0x140 + 0x140·組 + 0x40·k) ≠ −1e6 かつ 2·半径 A (S+0x1c8 = 4) < w, h なら 0x991aac の方向経路 (組 1: 4 方向・組 2: 8 方向・組 3: 16 方向 + 大口径 B (半径 6))。線カーネル FUN_00990448 (nv_lines 済み)、線フィルタ FUN_009944c0 (nv_lines 済み)、FUN_009ef0d0 (nv_model 済み)・FUN_009ef260・FUN_009eef5c・FUN_009ee5fc (nv_pixnoise 済み)・FUN_0098ed34 (済み)・FUN_0098eba8 (NEON 部 FUN_009f19cc)・FUN_009ee93c (済み)・FUN_009ed978 (nv_model 済み)・FUN_009efb1c (済み)・FUN_009f11fc (済み) など | なし | 0x991aac〜0x993960 を読んで移植。新しい葉は `elem rstep` (FUN_009ef260)・`elem rinv` (FUN_009eef5c)・`elem fabs`・`sel2` (FUN_009f19cc) を要求 |
| 6c | 組 0 の扱い | 組 0 (方向数 0) は既定の表では選ばれない。モデル語 A が −1e6 (BuildNoiseModel が測れなかった段) か、段が 2·4 以下の大きさのときだけ高速経路 | 現行の移植はこれだけ | — |

## 2. 進め方

1. (済み、本文書) 静的調査。
2. ハーネスなしでできる部分を先に移植する: 2a・2c〜2i の駆動部 (1 面の FUN_0098edb0 を、切り出しと NEON/端数の境界まで)、3a・3c・3d・3e、4a・4b。
   部品は `repro/nv_pixnoise` (up2・band_from・frecpe32・frecps32・fma32)、`nv_model` (sqrt_abs 等) をコピーして使う。
3. ハーネス追加後: 葉の照合 (`verify/verify_align_leaves.py`、本家の結果は `tmp/align_cache/`)。
4. 6b の方向経路を読み、組 1・2 から移植 (組 3 は B を含む)。
5. 区間 A/B の実測 (v5 互換の入口を残し、本家入力からの経路を別関数にする)。

## 3. 未確認の仮定 (正規のホストでの値は未確認)

> **統括注記 (2026-10-22)**: 下の 2 項目のうち popcount 型の機構については `coord/verify_protect/VERDICT.md` で
> 解読が進んだ。FUN_009ae918 内の「popcount − 16」の対象は**大域状態語 w22 の 32 ビット全体**で、保証語
> (0x1561dec) は「& 0x3fffff == 1」のゲートにだけ使う (popcount はしない)。0x1488030 (rebase 0xd26820) 経由で
> 読む w19 = *(u32*)(image+0x1489E68) の popcount は静的初期値で 16 → 減算値 0。いずれも「正規のホストで 0/1
> になる」ことの証明にはなっておらず、仮定のままで変わらない。FUN_0099afb4 の係数式の訂正は AGENTS.md §6.4 に反映済み。

- FUN_009aae90 の fVar54 (17−popcount 型) = 1、FUN_0098edb0 の [sp+0xbc] = −0x1e0 (cfg[k] を 0 にしない)、
  FUN_00990f74 の x20 = 0 (乗数 1−x20 = 1)、FUN_0099c694・逆色変換のトークン表による加算 = 0。いずれもファイルの初期値で
  表が NULL のときの値ではなく、「改ざんがないときは効かない」という推定。
- FUN_00990f74 の方向数 [sp+0x188] は 0x991000〜0x991080 (読まない範囲) で書かれる。組の表 S+0x258 の方向数 (BuildNoiseModel と同じ表) と仮定。

## 4. 進捗 (2026-10-21、ハーネス待ちで停止した時点)

- 要求: `harness_request.md` + `nvh_patch.diff` (elem 17 関数・strip3・sigfuse・ixform・sel2)。照合スクリプト
  `verify/verify_align_leaves.py` は用意済み (現状 "harness pending")。
- `src/nv_spatial.py` に移植済み (ハーネスなしでできた部分。本家との数値照合はまだ 0 件):
  葉 (leaf_recip・mul・add・sub・scale・affine・mulk・acc・blend・clampmul・u8・rstep・rinv・sqr・sqrtabs・sqrtk、strip_box3・strip_rms3、
  sigfuse_port、ixform_neon / ixform)、halve (H < 2 は None)、level_rects (FUN_0099c694)、shrink_fast (FUN_00990f74 高速経路、
  方向経路の条件では NeedsDirectional)、plane_filter (FUN_0098edb0、切り出しと行ごとの NEON/端数境界まで)、build_cfg (FUN_0099ead0 の
  cfg[k]・amount・b8・levels・set・c90)、lum_tables (FUN_0099e6f4 + FUN_0099e264)、sigma_coefs / sigma_level / sigma_maps (FUN_009aae90)。
  up2・band_from は `base/nv_pixnoise.py` (repro のコピー) を使う。v5 互換の入口 `spatial_genuine` も同じ部品に切り替えた。
- 自己試験 `verify/sanity_align.py` 全項目 ok (4 列帯の手順どおりの素朴な実装と strip_box3 / strip_rms3 がビット一致、
  プロファイル (nv_profile) から cfg・σ・1 面が動く、Y の組 2 で方向経路の条件が成り立つこと、全面 ROI = 切り出しなし)。
  旧 `verify/sanity.py` も PASS。
- 観測: AutoFineTune なしのプロファイルでは輝度別テーブルが全て 1 → σ マップはほぼ 1 (Y 段 0 の平均 0.99999988)。
- 未着手: 方向経路 (6b、本番で必ず通る)、極小面 (5)、チャンク分割 (4c)、混合 (4b) の駆動部、モード 2 の逆色変換。

## 5. 照合の再開後 (2026-10-21、ハーネス追加後)

- 葉の照合 `verify/verify_align_leaves.py` (ログ `logs/verify_align_leaves_20261021.log`): 2,566 件、不一致 0 (観測)。
  途中で rstep (FUN_009ef260) の端数部が NaN のとき 11 件ずれた → 本家は `fcmp; b.le` が順序なしでも成立して x0 を (sNaN も静化せず) そのまま返す。移植を直した。
  sel2 の flag (スタック渡しの unsigned char) は flag 1 の 16 件で b・i4 の書き戻しまで一致 → Apple arm64 の詰め方 ([sp] 1 B、stride [sp+8]、n [sp+0x10]) は本家の呼び出し側 (0x98ec04〜0x98ec08) と同じ (観測)。
  感度 (`verify/sensitivity_align.py`): 旧式・取り違えの式は recip 60/75、acc 49/225、blend 43/150、affine 47/225、rinv 176/225、局所 RMS の端の複製 24/54、箱の端の取り違え 51/54 件で本家とずれる。
- 方向経路 (組 1・2) を移植 (0x991aac〜0x993d18 を読んだ。組 3・4 と cfg+0x78・+0x90 の任意機能は NotImplementedError)。
  cfg+0x28 / +0x3c は段 3・4 で雛形の 1.0 が残るので、既定でも粗い段で通る (移植済み)。B・I の作業面 (98edb0 の [x29−0xb8]・[x29−0xd0]) の初期化・段ごとの変換・拡大も移植。
  本家との照合は葉だけ (FUN_00990f74・FUN_0098edb0 は呼べない)。自己試験 `verify/sanity_align.py` は全項目 ok。
- 区間 A/B (v5 互換の入口、`logs/run_ab_{296,560}_align.log`): 葉を本家どおりにしても Y の差は 0.004 dB 以内 (46.315/46.727、t=1.5 で 46.998/47.158)。
- 読まずに迂回・仮定した点の追加: 方向経路の末尾 0x993be0〜0x993cd8 の popcount 型の係数 (x20、乗数 1 − x20) は 0 と仮定。方向経路は 0x9916ec (読まない範囲) に戻ってから乗数に入る。
- チャンク分割: FUN_009adc8c はスケジューラのタスク (x, y, w, h, 核の x, y, w, h, 余白 lx, ty) から ROI = (ty, lx, 核 w, 核 h) を作る (nv_curve.tile_layout と同じ記述子の並び。観測)。核と余白の大きさを決める FUN_00ac2088 (面積 (ctx[0x7f0] + 2·ctx[0x7f1])²·4 を受け取る) は未読。

## 6. 第 3 区切り (2026-10-21)

- 組 3 の方向経路 (フラグ S+0 = 1 の方向ごとの処理、Mb、作業面 A18 = 第 10 引数 [x29−0xc0]、B 族 0x992cac〜0x993284) を移植。
  組 4 (C 族 0x9932a8〜・自己相似パターン保護 [sp+0xe8]・cfg+0x50) は未移植: 品質 2 (Highest) でしか選ばれず、Demo は読み込み時に
  品質を ≤ 1 に切り詰める (0x166548・0xb3460) ので Demo では通らない (観測は切り詰めのコード。実ホストで品質 2 が渡らないことは推定)。
- cfg+0x78 (Rx·(1 − c78·m)、B 族の閾値係数) と cfg+0x90 (残差の高域を戻す後段 0x99185c〜0x992608、箱の回数 cfg+0xa4、
  cfg+0x8c のバイト) を移植。cfg の構築に c28・c3c・c78・ca4・c8c を追加 (演算順は 0x99f0d0〜0x99f480 の逆アセンブル)。
  注意 (観測): cfg+0x28 / +0x3c の段 3・4 は雛形の 1.0 のまま (設定が掛からない)。
- チャンク: Filter は FUN_00ac14d4(sched, 0, 0, W, H, 核, 核, 余白, 0x30, −W, −H, …) (0x9ac880〜0x9ac8c0) → FUN_00abf6a8。
  核 = ctx[0x3f80]、余白 = ctx[0x3f88]。格子は nv_curve.tile_layout と同じ (`verify/verify_chunks.py`: `nvh tiles` と 204 配置で一致、
  ログ `logs/verify_chunks_20261021.log`)。FUN_00ac2088 は面の確保とキャッシュで、配置には関わらない (静的に読んだだけ)。
  → 2026-10-22 に統括が解読し検証者が確認 (coord/verify_protect/VERDICT.md I): キー照合型の作業面プール
  (0x9f7c54 複製 / 0x9f7c9c 比較 / alloc は FUN_00ac2764 経由で 0x9d3278)。エントリ初期化は [+8] = 100、
  100.0f/[x19+0x2c8] を面に、+0xa8 へ 0x1279770 をパターンとした 0x88 バイトの pattern16 埋め、0xac1a88 を 2 回。
  呼び出しは 6 か所 (0x9a2600/0x9a2658/0x9a6b00/0x9a6b50/0x9ac87c/0x9ad5a0)。チャンク配置の計算は含まない。移植不要。
  FUN_00ac14d4 は大域 0x14de194.. (ファイルの値 0x5c, 0x135, 0x3a0, 0x2a, 0x2c) で処理範囲を幅 1856・高さ 1024 に中央で切り詰める
  (Demo の制限とみられる)。ところが本家出力 test_v02_reduce (1920×1080) は端の列・行も同じ強さで処理している (除去成分の RMS が
  列 0〜39 で 3.3〜4.0、中央と同程度。観測) → この出力は制限のない条件で作られたとみられる (推定)。`filter_frame(demo_limit=False)` が既定。
- 駆動部: `filter_chunk` (FUN_009ae918: σ → 色変換 → 面ごと → 混合 (設定 +0x10c..+0x11c、FUN_009ee1a4) → 逆変換 (モード 0・1・2)) と
  `filter_frame` (チャンク格子、ROI、核の書き戻し)。逆変換モード 2 (0x97b924) を移植。
  σ マップの追加面 (FUN_009adc8c の第 5 引数) と混合の元画像 (第 3 引数側) の正体は未確認 (既定はなし / 入力と同じ)。
  → 2026-10-22 に統括が解読し検証者が確認 (coord/verify_protect/VERDICT.md、`notes/lead_static_20261021.md` §1-§2):
  追加面 = FUN_009adc8c 第 7 引数 → FUN_009ae918 第 17 引数 ([x29−0xb0] の下位語)。その素はジョブ記述子オブジェクト
  (vtable+0x18 の戻り値) の [0x18] 語で、Auto Profile では StartAdaptive 第 3 引数まで遡れる。既定経路では NULL。
  混合の元画像 = FUN_009ee1a4 の x0 は FUN_009ae918 第 17〜19 引数 = 呼び出し側が保持した元の変換面 (面 2 だけ混合先が
  第 10 引数 = var 面絡み)。実装の filter_chunk の構造 (元の変換面を保持) で正しい。移植への変更は不要。
- 極小面 (5): 48px 以上のフレームでは FUN_009f15b4 が H < 2 で呼ばれることはない (段の高さ ≥ 3)。W か H が 48〜63 で段数 5 のとき
  (a) 段 4 の幅 3 で箱が分離型 (FUN_0099ba54 / FUN_0099bf40) になり (移植済み: nv_lines.box_rows / box_cols)、(b) σ マップの段 5 の幅 1 を
  FUN_00993df0 のスカラー経路で拡大する。(b) は行の外 (幅 1 なので次の行の値、最後の行は面の外の作業域) を読むので、値は作業域の
  前の内容に依存し決められない (静的読み。移植は NotImplementedError)。核 ≥ 96・余白付きのチャンクでは起きない。
- 自己試験 `verify/sanity_align.py` 全項目 ok (組 3・任意機能・チャンク駆動部・色モード 0/2 の往復・混合)。葉の照合は再実行で不一致 0。

## 7. 検証 (coord/verify_spatial3) への対応 (2026-10-21)

- 品質のクランプ: 移植の min(品質, 2) は本家にない (0x99f5xx は 0x1246c48 + (long)(int)s[0x104]·0x18 + 色モード·0x48 を範囲の確認なしで読む)。
  移植は品質 0〜2・色モード 0〜2 の外を ValueError にした (本家は表の外を読む)。
- 再確認した根拠アドレス (いずれも逆アセンブル、観測):
  - モデルと cfg の渡し方: FUN_009ae918 0x9aebf8〜0x9aec24 (面 0: cfg = ctx+0x3d10 → x7、model = ctx+0x17330 → [sp])、0x9aecf0〜0x9aed00
    (面 1: ctx+0x3de0、ctx+0x19284)、0x9aee88〜0x9aee98 (面 2: ctx+0x3eb0、ctx+0x1b1d8)。FUN_00990f74 は第 9 引数 ([x29+0x10]) を
    0x991300〜0x991304 で [sp+0x178] に置く。
  - モデル語 5+k: 0x99155c〜0x991574 (x8 = k·4、`ldr s10, [model + x8 + 0x14]`)。
  - 方向経路の語: A 0x991e70〜0x991e88 (model + 組·0x140 + k·0x40 + d·4 + 0x140 → 語 80 + 80·組 + 16k + d)、
    R 0x992b08〜0x992b1c (model + 組·0x14 + k·4 + 0x780 → 語 480 + 5·組 + k)、dc 0x993a28〜0x993a3c (+0xdc → 語 55 + 5·組 + k)、
    B 0x992e80〜0x992e9c と 0x992f98 (+0x7e4 → 語 505 + 80·組 + 16k + d)、組 = cfg+0xc8 (0x990ff4 → [sp+0x198])。
  - Rx と wR: 0x9928b8 (D = Wc − S)、0x9928c0〜0x9928cc (Xd²)、0x9928d0〜0x992a38 (0 詰めの 3×3 を 2 回)、0x992a48 (√|·|)、
    0x992b04〜0x992b50 (t4 = s13·(|R|·1.1)、FUN_009ee93c(t4, t4 + t4, 1, 0, Rx))、0x992c14〜0x992c24 (X = wR·Xd)。
  - dc: 0x993a10〜0x993a6c (半径 DAT_012438f8[組]、語 ≠ −1e6、2r < W・H)、0x993a70〜0x993a88 (FUN_009ef0d0(1.25t, D, R))、
    0x993a8c〜0x993acc (q²、行 → 列の箱)、0x993ad0〜0x993b14 (DAT_01243920[組] なら 2 回目)、0x993b18〜0x993b24 (√|·|)、
    0x993b90〜0x993bbc (FUN_009eebf8(s13·|dc|, 1.5·that, 1, 0, Qd, X))。
  - 分岐: cfg+0x28 0x992b5c〜0x992b68 (`fcmp; b.le` → 0 以下か NaN で飛ばす)、cfg+0x78 0x992a78〜0x992a8c (同じ)、
    cfg+0x3c 0x9927a4〜0x9927ac (同じ)、cfg+0x90 0x991860〜0x991874 (`b.eq` → 0 のときだけ飛ばす。NaN は通る)。移植の条件 (> 0 / ≠ 0) と一致。
  - 混合: 0x9aefcc〜0x9af008 (設定 = ctx+0x3d00、+0x10c のバイト、+0x110..+0x11c のどれかが 1 でない)、0x9af048〜0x9af07c
    (f = s[0x114]·s[0x110]、FUN_009ee1a4(1 − f, f, 元の変換面, OUT, OUT))、面 1・2 は 0x9af0d8〜・0x9af16c〜 (s[0x118]・s[0x11c])。
  - ゲイン FUN_0099e264: 0x99e2c4 `cbz w1` → 第 2 引数 0 なら 0x99e2f0〜0x99e2fc で 3 つとも 1.0、そうでなければ ctx+0x114d0/d4/d8。
    0x99e300〜0x99e34c で ctx+0x1d42c / 0x1d54c / 0x1d66c → ctx+0xb8 / 0x14cc / 0x28e0。0x99e3a8〜0x99e404: (g·T)·(g·T)。
    Filter は 0x9ac534〜0x9ac538 で第 2 引数 0。補間 FUN_0099e6f4 の本体 0x99e75c〜0x99e7b4: x = (float)i·0.03125、b = min(fcvtzs(x), 7)、
    T = c_b·f_b + (x − b)·(c_{b+1}·f_{b+1} − c_b·f_b) (非融合)。
- 追加の照合: W < 4 か H < 2 の分離型の箱と局所 RMS を、本家の部品の組み合わせ (`nvh linefilt` の横 → 縦、`elem sqr`・`sqrtk`) と比べ
  104 件不一致 0 (`verify/verify_small.py`)。sel2 の端数部は FUN_0098eba8 そのものを呼ぶ要求を出した (harness_request2.md)。
  ixform の端数部は関数の入口にトークン表の処理があるので要求せず、静的な読みのまま。
  → 2026-10-22 に統括が入口 0x97b7a0〜0x97b830 を読んだ (`notes/lead_static_20261021.md` §6): カウンタ
  0x147e0e0 (__data、ファイル値 0x228a8d04) の更新と 0x147ee44/0x147ee4c のポインタ・カウンタ下位 16 ビットに
  よるバイト値、`add x24, x24, x10, lsl #10` (x24 = 第 6 引数) の列がある。NEON 部 (0x97b824〜、FUN_009f1d0c) は
  通常どおりで `nvh ixform` の照合には影響しない。入口ブロックが本番の数値に効くか (スカラー部のどの経路か) は
  未解読。1920×1080 (n = H = 1080 は 4 の倍数) ではスカラー部は走らないので test_v02 では無関係。
- 読まない範囲への追加 (検証者の指摘): 0x991f80〜0x991fe8、0x992058 付近、0x99e6f4 の入口、0x99f528、0x9ab4b0〜0x9ab5b0。
  正直な記録: このうち 0x991f80〜0x991fe8・0x992058 付近・0x9ab4b0〜0x9ab5b0 は、指摘の前に方向経路と σ マップを読む途中で表示していた
  (ハッシュの書き込みとみて値は使っていない。σ マップの scale は「正規時 1」と仮定したまま)。0x99e6f4 の入口 (0x99e71c〜0x99e758) は
  今回、指摘を受けた後に本体を確かめる表示に含めてしまった。内容は使っていない。0x99f528 は読んでいない。
