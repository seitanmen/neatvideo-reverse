# Profile Quality (Device Noise Profile の "Quality" / Profile Viewer の "Overall Profile Quality") — 静的解析 2026-10-26

材料: RS (arm64 thin `/tmp/nv6rs_arm64.ofx`、全逆アセンブル `/tmp/nv6rs_disasm/disasm_full.txt`) と Demo
(`neatvideo_ofx_614/NeatVideo6.arm64.ofx`、llvm-objdump で 0x9bb3c0〜0x9bb760 だけを逆アセンブル)。
本家は実行していない (dlopen・nvh・デバッガとも使っていない)。アドレスには必ず RS / Demo を付ける。
「観測」= 逆アセンブルで実際に読んだこと、「推定」= そこからの推論。

## 1. 結論 (要約)

Quality は 1 つの関数 **RS 0xa4acf8 (Demo 0x9bb3e0)** の float 戻り値 q を百分率にしたもの。Simple / Advanced / Profile Viewer の 3 か所とも同じ関数を呼んでいる (観測)。

```
p = プロファイル (BuildNoiseModel が書く 0xc300 バイトの構造体。nv_profile.build_profile と同じ配置)
if u8 p[0xbf34] == 0: q = 0                                      # プロファイル有効フラグ
s11 = 0.5f * g(p + 0xbf40) + 0.5f                                # g = RS 0xa4b03c / Demo 0x9bb748 (未読・未確定、§5)
for i in 0,1,2:                                                  # 変換面 3,4,5 (色モード 1 では Y, Cr, Cb)
    luma_i = u8 DAT[mode*3 + i]                                  # RS 0x1622348 / Demo 0x147dfc8 = {1,1,1, 1,0,0, 1,0,0, 0,0,0}
    f_i = FUN_009b096c(p + 0x4c + (3+i)*0x1f54, luma_i)          # 平坦度 (= repro nv_flat.flatness、FindFlatArea と同じ関数)
    u_i = FUN_009d49b4(f_i, 2.0f, -0.25f)                        # = clamp(2.5 − f_i, 0, 1)
    if luma_i == 0: u_i = 0.5f*u_i + 0.5f                        # 色差は罰を半分に
P = (((u0+0.5f)*(u1+0.5f))*(u2+0.5f) + 0.25f) / 3.625f           # 3.625 = 1.5³ + 0.25 → 最大 1
q = s11 * P
W, H = i32 p[0xc2c0], i32 p[0xc2c4]                              # 見本領域の幅・高さ
if W != 0 and H != 0:
    m = min(W, H); T = (16, 32, 64, 128, 256)                    # RS 0x162c580 / Demo 0x1488128 (int64 ×5)
    S = 0 (m < 16) | k + (m − T[k−1])/(T[k] − T[k−1]) (T[k−1] ≤ m < T[k], k=1..4) | 5 (m ≥ 256)
else:
    S = (float) i32 p[0xbf38]                                    # サイズ区分 (BuildNoiseModel が書く整数)
q = q * (S / 5)
for c in 0,1,2: if u8 p[0xbf3c + c] != 0: q *= 0.5f              # R/G/B の IsClip > 0.33 フラグ 1 つにつき半分
```

表示:
- Profile Viewer (RS 0x1186b4〜0x118730): `qRound((double)q * 100.0)` → `QString::number(n, 10)` + `"%"` → `profileQualityLabel` の setText。クランプなし。
- Device Noise Profile パネル (Simple / Advanced、RS 0xdb25c〜0xdb2a8): `qRound(q * 100.0f)` (float で計算) → 自作ラベル RS 0x15ce0 が int を +0x50 に保存し `QString::number(n,10) + "%"` を setText。クランプなし。
- 違いは 100 倍と qRound を double でやるか float でやるかだけ (x.5 の境界で 1% ずれ得る。推定)。

照合 (§4): test_v02 では平坦度がすべて 1.5 未満なので P = 1、128×128 の領域では S/5 = 0.8 になり、q = 0.8·s11。**g の解読 (§8) により 23% も説明できた**: 新規プロファイル (AFT 前) は g = 1/81 で s11 = 0.5062、全面プロファイルなら S/5 = 1.0 で全面の Y 平坦度が P を決める (23% ↔ P = 0.454 ↔ Y 平坦度 2.38)。

## 2. 参照の連鎖 (観測)

### Profile Viewer (RS)
- コンストラクタ RS 0x112cb0: `add x21, x19, #0x30` → `bl 0x1131e0` (setupUi。第 1 引数 = ダイアログ+0x30 の Ui 構造体)。+0x3a0 にホスト側オブジェクト、+0x3a8 はプロファイルの複写用オブジェクト (遅延生成、0x21170 バイト)。
- setupUi RS 0x1131e0 (オブジェクト名 "profileViewerDialog")。0x114dfc〜0x114e10 で QLabel (0x30 バイト) を作り、親を Ui+0x160 にして **Ui+0x170** に保存し、0x114e18 で "profileQualityLabel" と名付ける。→ ダイアログ+0x1a0。
- retranslateUi RS 0x11a530〜0x11a55c: Ui+0x160 (ラベルの親) に `0x8975a0(tr("profileViewerDialog", "Overall Profile Quality"))` を渡している。親は QGroupBox で、これはその題名とみられる (推定。0x8975a0 が setTitle であることは確認していない)。
- 更新関数 RS 0x117b98 (第 1 引数 = ダイアログ、w1 = 元か複写かの指定):
  - 0x117bd0〜0x117c2c: w1≠0 なら `x28 = [[dlg+0x3a0]+0x350]+0x28` (現在のプロファイル)。w1=0 なら `x28 = [dlg+0x3a8] + 0x114e8` (複写。AGENTS.md §6.4 の「インスタンスのプロファイル複写 +0x114e8」と同じオフセット)。
  - 0x1186b4: `ldr x21, [x19, #0x1a0]` (= profileQualityLabel)、0x1186bc `bl 0xa4acf8` (x0 = x28)、0x1186c0〜0x118704 は double の qRound(×100)、0x118710 `bl 0x382afc` (QString::number(int, base 10))、0x118718 "%" (0x14966f1) を連結 (0x15cee8)、0x118730 `bl 0x899328` (QLabel::setText とみられる。推定)。
- `bl 0xa4acf8` を呼ぶのは全逆アセンブル中 RS 0x1186bc と 0xdb25c の 2 か所だけ (grep。観測)。

### Device Noise Profile パネル (RS)
- RS 0xdf10c〜0xdf128 (Simple): 0x58 バイトのオブジェクトをコンストラクタ RS 0x15c04 で作って Ui+0x20 に保存し、"simpleQualityLabel" と名付ける。RS 0xe78bc〜0xe78d8 (Advanced) でも同じクラス (0x15c04、0x58 バイト) を Ui+0x28 に作り、"advancedQualityLabel" と名付ける。
- 共通の更新関数 RS 0xdb1b0 (呼び出し元は RS 0xdeccc と 0xe3d7c。Simple と Advanced のパネルとみられる。推定): `x19 = [[x1+0x350]+0x28]` (プロファイル)、`w25 = u8 [x19+0xbf34]`。w25 ≠ 0 のとき 0xdb25c `bl 0xa4acf8`、`×100.0f` (0x42c80000)、float の qRound、`bl 0x15ce0` (x0 = [obj+0xd6a0])。
- RS 0x15ce0: `str w1, [x0,#0x50]` → QString::number(w1, 10) + "%" → 0x899328 (setText)。[obj+0xd6a0] の先が simple/advancedQualityLabel かどうかは、メンバの対応までは追っていない (クラスが同じこと、"%" を付けること、Quality の関数を呼ぶ場所がここだけであることからの推定)。

### Quality の関数 RS 0xa4acf8 / Demo 0x9bb3e0 (観測。難読化ブロックは除く)
- 対応の根拠: `fmov s1, #3.625` (0x1e21b001) は RS では 0xa4aeb0、Demo では 0x9bb5a0 の 1 か所だけにある。前後の命令列 (+0xbf34・+0xbf40・+0x5e48/+0x7d9c/+0x9cf0・(2.0, −0.25)・3.625・+0x38c/+0x390・5.0・+0x8/+0x9/+0xa) は両者で同じ。
- 0xa4ad14〜0xa4ad20: `x20 = p + 0xbf34`、`u8 [x20] == 0` なら 0 を返す (0xa4af18)。
- 0xa4ad88〜0xa4ad9c: `g = bl 0xa4b03c (p + 0xbf40)`、`s11 = g*0.5f + 0.5f` (fmul → fadd)。
- 0xa4ada0〜0xa4ae48: `w8 = i32 p[0x48]` (色モード)、`x22 = 0x1622348 + 3·mode` (RS。Demo 0x147dfc8)。面 3〜5 (p+0x5e48 = 0x4c + 3·0x1f54、+0x7d9c、+0x9cf0) ごとに `bl 0xa402f4 (面, u8 x22[i])` (Demo 0x9b096c = 平坦度) → `bl 0xa63938 (s0, 2.0f, −0.25f)` (Demo 0x9d49b4)。フラグが 0 なら `0.5f·v + 0.5f` (fcsel eq)。
- 0xa4ae94〜0xa4aeb8: `((u0+0.5)·(u1+0.5))·(u2+0.5) + 0.25 → /3.625 → ×s11`。
- 0xa4aebc〜0xa4af98: W = i32 [x20+0x38c] (= p+0xc2c0)、H = i32 [x20+0x390]。どちらかが 0 なら `S = scvtf(i32 [x20+4] = p+0xbf38)`。そうでなければ `m = (W > H) ? H : W`、表 0x162c580 (RS) の 5 つの int64 との比較で上の区分線形の S を作る (m < T0 は 0、m ≥ T4 は 5.0)。`q *= S / 5.0f`。
- 0xa4aff0〜0xa4b01c: u8 [x20+8]、[x20+9]、[x20+0xa] (= p+0xbf3c..0xbf3e) が 0 でなければ、それぞれ q·0.5f (fcsel eq で元の値を残す)。
- 静的データ (ファイルから読んだ値): RS 0x1622348 と Demo 0x147dfc8 はどちらも u8 {1,1,1, 1,0,0, 1,0,0, 0,0,0} (repro `nv_flat.LUMA_FLAGS` と同じ)。RS 0x162c580 と Demo 0x1488128 はどちらも int64 {16, 32, 64, 128, 256} (repro `nv_profile.SIZECLASS` と同じ)。

### 下位の関数
- 平坦度 Demo 0x9b096c: repro `nv_flat.flatness` が本家 (Demo) とビット一致する移植 (AGENTS.md §6.5)。読むのはモデル面の語 0..54 (q99・q50・cv) だけ。
- 罰 Demo 0x9d49b4: 先行作業 (aft_port/notes/AutoFineTune.md、VERDICT_verify1.md) の読み。s ≠ 0 では v = ((0.5/s)/c)·x + ((c−1)·0.5)/c、c = −0.25 なら v = −2x/s + 2.5、v < 0 → 0、v > 1 → 1。s = 2 なら clamp(2.5 − x, 0, 1)。(aft_port の nv_aft.py の docstring にある「−8x/s」は誤記で、式は −2x/s。)
- RS 0xa402f4 と RS 0xa63938 は読んでいない。Demo の版とバイト比較だけをした結果、どちらも異なっていた (RS 0xa63938 は +0x20 以降に大域語への書き込みらしい命令列が挿入されている)。RS でも数値の式が同じというのは推定。

## 3. 入力 (プロファイルのオフセット)

| オフセット | 内容 (nv_profile の注記) | Quality での使い方 |
|---|---|---|
| 0x48 | 色モード (初期値 1 = YCrCb) | luma フラグ表の行 |
| 0x4c + (3+i)·0x1f54 | 変換面 i (モード 1: Y, Cr, Cb) の FUN_00995250 モデル。語 0..54 = 5 段の mean/q99/q50/cv | 平坦度 → 罰 |
| 0xbf34 | 有効フラグ (BuildNoiseModel が 1 を書く) | 0 なら Quality 0 |
| 0xbf38 | 見本の大きさの区分 (int) | W/H が 0 のときだけ S |
| 0xbf3c..0xbf3e | R/G/B の IsClip > 0.33 | 1 つにつき ×0.5 |
| 0xbf40.. | 輝度曲線 (モード語 + 3ch × 9 ビン × 0x20 バイト) | g の引数 (未解読) |
| 0xc2c0, 0xc2c4 | 見本領域の W, H | 大きさの項 S |

意味の読み (推定): Quality = 曲線 (AFT) の完成度 × 見本の均一さ (Y は 1.5 で減り始め 2.5 で 0、色差は 0.5 が下限) × 見本の大きさ (256 px 以上で満点) × 白飛び・黒つぶれの罰。

## 4. 照合 (`quality_draft.py`、結果 `quality_draft_results.json`)

`quality_from_profile(blob, g=None)` は g を除いた部分 R = P·S/5·(0.5^クリップ数) を返す (g を渡すと本家と同じ演算順で q を返す)。test_v02 で計算した結果:

| 入力 | 領域 (x, y, w, h) | 平坦度 Y / Cr / Cb | P | S | R |
|---|---|---|---|---|---|
| 1080p n=312、nv_profile.build_profile の完全なプロファイル (13 秒) | 832, 0, 128, 128 | 1.070 / 1.151 / 1.000 | 1.0 | 4.0 | 0.80 |
| 1080p n=0、移植の FindFlatArea | 832, 0, 128, 128 | 1.039 / 1.145 / 1.004 | 1.0 | 4.0 | 0.80 |
| 2 倍 (bicubic) n=0、移植の FindFlatArea | 1752, 1800, 128, 128 | 1.066 / 1.000 / 1.223 | 1.0 | 4.0 | 0.80 |
| 2 倍 (bicubic) n=312、移植の FindFlatArea | 2856, 1792, 192, 192 | 1.191 / 1.241 / 1.182 | 1.0 | 4.5 | 0.90 |

- 簡易版 (面 3〜5 の語 0..54 を nv_band.model_levels で作る `_fast_blob`) の R は、完全なプロファイルの R と一致した (n=312、1080p)。
- 平坦度はすべて 1.5 未満なので u = 1、P = 1、クリップなし。R は領域の大きさだけで決まる (128 → 0.8、192 → 0.9、256 以上 → 1.0)。
- 本家の 23% (genuine_ui2/shots2/g0.png。Resolve で 3840×2160 に拡大した test_v02、"Profiled with: Full frame"、イコライザは平ら、完成度 0%) にするには s11 = 0.23 / R が必要: 128 px なら 0.2875、192 px なら 0.256、256 px なら 0.23。s11 = 0.5g + 0.5 なので、**g ∈ [0, 1] では 23% にならない** (q は 40〜100% の範囲になる)。g は負の値も取る関数か、Resolve 上の領域がもっと小さい (m = 32 なら S/5 = 0.4 で s11 = 0.575) かのどちらか。これは g を読まなければ決められない。
- 参考: genuine_ui/OBSERVED.md の別の作業では Quality 43% (Build Profile の後、AFT 後とみられる曲線)。R = 0.8 とすると s11 = 0.5375。曲線の状態だけで 23% → 43% に変わったのなら、g は AFT の完成度に応じて変わる項とみられる (推定)。
- 本家が Resolve で選んだ領域も、拡大の方法も分かっていない。上の 2 倍の行は bicubic で近似したもの。

## 5. 未確定点
1. ~~**g = RS 0xa4b03c / Demo 0x9bb748 (引数 p+0xbf40 = 輝度曲線)**~~ → **2026-10-26 に統括が解読した** (下記 §8)。入口と末尾の難読化ブロックは大域語の記帳だけで、浮動小数点の累積 s0 には触れない。RS と Demo で数値経路が命令単位で一致することを確認済み。
2. RS 0xa402f4 (平坦度) と RS 0xa63938 (罰) が Demo の版と同じ式かどうか (バイト列は違う。推定では同じ式に難読化のブロックを挿入したもの)。
3. Profile Viewer の 0x899328 が QLabel::setText、0x8975a0 が QGroupBox::setTitle であること (Qt は静的リンクでシンボルがない。引数と使い方からの推定)。
4. Simple と Advanced のパネルのうち、どちらが [obj+0xd6a0] のラベルを使うか (メンバの対応は追っていない)。
5. 本家の 23% の条件 (Resolve 上で選ばれた領域の大きさ、AFT を実行したか)。

## 6. 触れずに止めた箇所 (難読化の兆候。中身の解釈はしていない)
数値の計算 (浮動小数点レジスタ) は下の各ブロックの出力 (w8〜w10・w21・__common への書き込み) を読んでいない。これはレジスタの流れを見て確かめたことで、ブロックの中身は解釈していない。
- RS 0x117b98 (Profile Viewer の更新): 0x117c74〜0x117cc8 (大域語のビット混合・バイトの和 → 比較 → ハッシュ化したアドレスへの書き込み。結果 w25 は [sp+0x1c])、0x118640〜0x1186b0 (同じ型のもの。0x16f0190+0x24a8 の表を引く。[sp+0x1c] が 0 でないときだけ実行される)。
- RS 0xa4acf8: 0xa4ad28〜0xa4ad84 (ror/csel の大域語、ハッシュ化したアドレスへの書き込み、0x16f0190+0x1cc0 の表を 0x502e で引く)、0xa4ae4c〜0xa4ae90、0xa4aed0〜0xa4aed4 (ダミーの `ldr xzr`)、0xa4af9c〜0xa4afec (バイトの和の型)。
- Demo 0x9bb3e0: 0x9bb410〜0x9bb458、0x9bb520〜0x9bb580、0x9bb5c0〜0x9bb624、0x9bb6ec〜0x9bb6f8 (同じ型のもの。ROOT 領域 0x1488xxx・0x1489xxx への書き込みを含む)。
- RS 0xa4b03c / Demo 0x9bb748 (g): 入口が難読化のブロックなので入らなかった (§5-1)。
- RS 0xa402f4 / 0xa63938: バイト比較だけをして、逆アセンブルは読んでいない。
- 指定された Demo の禁止範囲には入っていない。

## 7. ファイル
- `quality_draft.py`: `quality_from_profile(blob, g=None)`、`genuine_g(blob)` (統括の解読 §8)、`penalty`、`size_term`、`_fast_blob` (Quality が読む語だけを作る。**曲線の語を書かないので g には使えない**。g が必要なら `nv_ui_qt/quality._curve_blob` か、曲線つきで作り直すこと)。`python3 quality_draft.py` で §4 と §8 の数値を再計算する (ffmpeg で test_v02 を読む。約 1 分)。repro は import するだけで、変更していない。
- `quality_draft_results.json`、`run.log`: §4 と §8 の出力。
- `repro/nv_ui_qt/quality.py`: UI 用の実装 (`genuine_g` は §8 の同じ解読の複製。同期を保つこと)。

## 8. g の解読 (2026-10-26、統括。保護区画を含む読みは統括が実施)

材料: RS 0xa4b03c〜0xa4b380 (全逆アセンブルから抽出) と Demo 0x9bb748〜 (Demo の全逆アセンブル `sandbox/omo/wp_spatial/tmp/disasm_full.txt`)。両者で数値経路 (読みのオフセット・定数・分岐構造) が命令単位で一致することを確認した (難読化ブロックの配置とレジスタ割当は異なる)。

### 8.1 難読化ブロックの内容 (解読済み。数値に無関係)

- **入口 (RS 0xa4b03c〜0xa4b0f0)**: ROOT スロット (`0x162d000` 経由の大域語) 2 個を読み、1 個目は eor/and/orr 型の語の更新 (`w8' = (w8 ^ w8>>1) & 0x11054005` を orr シフトで戻して xor) とハッシュ化アドレスへの書き戻し、2 個目はバイト和 (`add w9, w9, w9 lsr #16; add w9, w9, w9 lsl #8; add w9, w9, w9 lsr #24; extr`) で鍵を作り、`cmp w9, #1 / b.eq` で記帳の要否を決める。3 個目は ror+csel (比較語 0 なら原語) の後 cbz で書き戻しを飛ばす。**どれも s0 に触れない。** w9 (0 or 1) だけが以後の記帳スキップの分岐に使われる。
- **ループ内 (RS 0xa4b154〜0xa4b188 など)**: 同型のバイト和鍵の確認と、表 (`0x16f0190+0x750` 経由) への語の書き戻し。s0 に触れない。
- **末尾 (RS 0xa4b308〜0xa4b374)**: 同型の鍵計算の後、`0x16f0190+0x7a40` の表語 (トークン表) が非 NULL ならその表から 1 バイト (`w11 = 0xf43e`) を読んでカウンタ (`0x162c000+0x8a8`) に足す。**s0 には触れない** (`movi v1, #0` → `fmax s0, s0, s1` → ret)。
- 結論: 難読化ブロックは改ざん検知型の記帳で、ライセンス状態で鍵が変わっても g の戻り値は変わらない (記帳の分岐は s0 の計算には影響しない)。ハーネスで実行せずに静的解読だけで確定した。

### 8.2 数値経路 (確定)

```
x0 = p + 0xbf40。k = 0..8 (x8 = 0x20k、9 回)。チャンネル c = 0,1,2 (+0x120c) を各 k で順に処理。
ビン (c,k) の基点 o = 0xbf44 + 0x120c + 0x20k (x2 = x0 + x8 からの相対オフセット −0x40 を含めて一致):
    ldrb [x2+0x22]  = o+0x1e   (レベル > 3.0 の旗)
    ldrb [x2+0x21]  = o+0x1d   (クリップ旗。BuildNoiseModel が filled ビンに書き、AFT の curve_post が 0 に戻す)
    ldr  [x2+0x4]   = o+0x4    (語 1 = 段係数の先頭。init で 1.0)
    ldr  [x2+0x1c]  = o+0x1c   (i32。AFT のマーカー旗 o+0x1c を含む。o+0x1d/o+0x1e はこの時点で 0)
    ldrb [x2+0x20]  = o+0x20   (次のビンの語 0 の下位バイト。ch0/ch1 のビン 8 は次チャンネルの
                                ビン 0、ch2 のビン 8 は 0xc2a4 = LZMA 成功旗)
分岐 (この順):
    o+0x1e != 0            → s0 += -1/27   (fmov w13 = 0xBD17B426)
    elif o+0x1d != 0       → s0 += -1/27
    else:
        f32 o+0x4 == 0     → s0 += -1/81   (fmov w10 = 0xBC4A4588)
        elif i32 o+0x1c!=0 → s0 += +1/27   (fmov w12 = 0x3D17B426)
        elif o+0x20 != 0   → s0 += +1/81   (fmov w11 = 0x3C4A4588、fcsel eq でバイト比較)
        else               → +0
戻り値: fmax s0, 0.0 (NaN は 0 側に倒す。ARM の fmax は非 NaN オペランドを選ぶ)
```

加算順は (k, c) の二重ループの通り (k 内で c = 0,1,2)。float32 の累積なので 27×(1/27) は 1.0 ではなく 0.999999762 になる。

### 8.3 状態ごとの値 (確定 + 検証)

| プロファイルの状態 | 各ビンの分岐 | g |
|---|---|---|
| 新規 (BuildNoiseModel 直後、クリップなし) | 全ビン +0、ただし ch2 のビン 8 が 0xc2a4=1 で +1/81 | **1/81 ≈ 0.0123** |
| 新規 + クリップ ch (filled ビンの +0x1d = 1) | そのビン −1/27 | 1/81 − クリップ数/27 (多くは負 → 0 にクランプ) |
| AFT 後 | **未測定ビン (AFT が補間で埋めたもの) だけ +1/27**。測定ビンはマーカー 0 のまま (curve_post が明示的に 0 にし、curve_fill の write() は fillable な未測定ビンにだけ +0x1c = 1 を書く)。測定ビンの後ろに非 1.0 レベルが来ると +1/81 の連鎖も付くが、観測した 2 例では連鎖は 0 だった | 測定されなかったビン数/27 (Full frame の猫 = 23/27 → 完成度 85%。test.mp4 の auto 領域 = 19/27 → 完成度 70%) |
| AFT 後 + レベル > 3.0 のビン | そのビン −1/27 (マーカーより旗が先) | 上から (レベル>3.0 数)×2/27 を引く |

`quality_draft.py` の単体試験 (合成状態 7 ケース) と test_v02 の完全プロファイル (n=312、128²、クリップなし) で g = 1/81・q = 0.40494・表示 40% を確認 (§4 の表に追記済み)。_fast_blob は曲線を書かないので g には使えない (q が 40.0% になる)。

### 8.4 本家観測との整合 (全部説明できる。2026-10-26 夜に完成度=g×100 の発見で確定)

g の呼び出し元は Quality (RS 0xa4acf8) の他に 4 か所あり、そのうち Simple/Advanced パネル
(RS 0xe2e20 / 0xe437c) と Profile Viewer (RS 0x118ef8 / 0x119038) の表示は **g×100 を
qRound して "%" を付ける** (同じ丸めと `bl 0x15ce0` のラベル書式)。これがパネルの
**Completeness** 表示である (ラベル文字列 "Completeness" RS 0x14a92de)。完成度は
Quality と独立した g の直接表示なので、g の値を丸め誤差 ±0.5% で直接読める。

| 観測 | 条件 | 式での値 |
|---|---|---|
| 23% (genuine_ui2/shots2/g0.png) | 3840×2160 に 2 倍拡大した test_v02、全面プロファイル、AFT 前 (イコライザ平ら・完成度 0%) | g=1/81 → s11=0.5062、S/5=1.0 (m=2160)、クリップなし → P = 0.454 → Y 平坦度 2.38。全面プロファイルの平坦度として妥当 |
| 43% (genuine_ui/OBSERVED.md) | Build Profile の後 | 128² (S/5=0.8) で Y 平坦度 2.25 (P=0.5375)、または最小辺 ~160px (S/5=0.85) で平坦度 < 1.5 の領域。どちらも整合 |
| 65% + 完成度 85% (検証者の Resolve 観測、shots/manual_ft_*.png) | 全面プロファイル + AFT | g = 23/27 = 0.85185 (85.19% → 85) → s11 = 0.92593。Quality 65% から P ∈ [0.704, 0.707)。Y 平坦度 ≈ 2.06。整合 |
| Reset 直後 35% + 完成度 0% (同上) | Reset Fine-Tuning Results の直後 | g = 0 (完成度 0% → g < 0.005。新規の 1/81 = 1.23% → 1% とは違うので **Reset ≠ 新規ビルド**)。q = 0.5×P ≈ 0.352 → 35%。整合 |
| Reset 後の赤 4 ビン編集 (同上) | ビン 4・5・3・6 を約 +50% へ | 各編集がちょうど +1/81 (完成度 1→2→4→5% = 1.23→1、2.47→2、3.70→4、4.94→5。Quality 35→36→36→37→37% も P ∈ [0.704, 0.707) で全段一致)。マーカーあり (+1/27/編集) なら初段で完成度 4%/Quality 37% になり不整合 |
| test_v02 1080p 128² (移植で再計算) | AFT 前 | **40%** (g=1/81、P=1、S/5=0.8、クリップなし) |
| test.mp4 auto 領域 (repro UI の表示) | AFT 前 / AFT 後 | 25%/1% (g=1/81) → 43%/70% (g=19/27=0.7037→70)。repro の AFT ポートが 19 ビンを未測定としてマークした結果と一致 |

### 8.5 再現側の写像 (repro の状態 → 本家の曲線バイト)

`repro/nv_ui_qt/quality.py` の `_curve_blob`:
- ビンのレベル語 (語 0) = `factors[c][k] × tune_all × tune_ch` (未編集なら 1.0 ちょうど → 下位バイト 0 で +1/81 の連鎖なし)
- AFT 後: `base["aft_bin_flags"]` (nv_aft_v5.apply_aft が AFT ポート自身のバイトから読んで保存) の
  マーカー/クリップ/>3.0 旗をそのまま書く。測定ビンはマーカー 0、未測定ビンは 1 (本家と同じ)。
  フラグなしの旧形式の保存プロファイルは全マーカーフォールバック (従来の近似)。
- AFT 前: filled ビン (チャンネル面の平均輝度のビン) に +0x18 = 1 と +0x1d = クリップ旗。マーカーなし。
  レベル > 3.0 の手動編集には +0x1e を立てる (新規ビルドの 1.0 には付かない)。
- 手動 Fine-Tune のマーカーなし書き込みは**観測で確定** (2026-10-26、§8.4 の Reset 後 4 ビン編集。
  ちょうど +1/81/ビンで、マーカー型は初段から不整合)。
- 0xc2a4 = 1 (repro の LZMA 部は常に成功扱い)
- UI は Quality (q×100) と Completeness (g×100) の両方を Device Noise Profile パネルと
  Profile Viewer に出す (本家と同じ 2 つの表示)。

### 8.6 未解決: Reset 直後の g = 0 の機構

Reset Fine-Tuning Results の直後は完成度 0% (g < 0.005) で、新規ビルドの 1/81 (完成度 1%)
とは違う。段係数は init/AFT とも 1.0 を書くので −1/81 の相殺はなく、レベルも (完成度が
 Manual 編集にだけ反応することから) ほぼ 1.0 に戻っているとみられる。残る説明は
 **0xc2a4 = 0 (Reset が LZMA 旗を消すか、復元しない)** である。確かめる方法:
- 本家で新しく Build Profile (Full frame) を作り Quality/完成度を見る (Copy Profile の控え +
  Undo で復旧できることは確認済み)。完成度 1% なら Reset 固有の差、0% なら Reset ≡ 新規。
- Reset Fine-Tuning Results の関数を静的に読む (UI コードとみられ、保護区画の外の可能性)。
repro は Reset を新規同様 (1/81) に扱うので、その状態では本家より最大 0.5 ポイント高く出る
(丸め境界でのみ見える差)。
