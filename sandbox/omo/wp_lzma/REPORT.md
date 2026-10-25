> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# wp_lzma 報告書 (REPORT.md)

日付: 2026-10-17。担当: wp_lzma 作業エージェント。
対象: Neat Video 6 (OFX Demo) のプロファイル LZMA 圧縮 (`FUN_009b41c4` → `FUN_009d61f4` →
LZMA SDK) の再現 `repro/nv_lzma.py` (本 wp では `base/nv_lzma.py` の写し) の追加検証。
INSTRUCTIONS.md の 4 項目すべてに決着がついた。観測と推定を区別して記す。数値はすべて実測
(出典は `logs/`・`notes/`・`data/` の各ファイル)。

## 結論 (要素ごと)

1. **SDK の版**: 動作は 9.20〜18.05 の全版と 228/228 ダンプでバイト一致、18.06 が初差。
   静的判別子 3 点も 9.20 形状。実装は 9.20〜18.05 のいずれか (これらの間は本入力群では区別不能)。
2. **未検出突然変異 2 種**: 両方とも差が出る入力を構成できた。`hash4_mask_minus` は 8 シード
   すべてで差、`extra_lit_rep_cond` は 2,000 入力の低エントロピー探索で 18 入力に差
   (18/18 で移植側が C 9.20 と一致し変異側のみが外れる)。感度試験の穴は閉じた。
3. **blob_size**: 本体 `blob`/`blob_size` を照合に組み込み、235 入力・容量 4 種で移植の判定が
   C の戻り値と完全一致 (不一致 0)。本家の容量式 (n + 0x90 → destLen = n + 0x80) を逆コンパイルで確認。
   非圧縮データで本家の -1 経路が実際に発火することも確認 (移植の None と一致)。
4. **境界**: 合成 87 入力 (4KiB 倍数 ±1) と本家 5 領域 (32×32・45×45・46×46・64×64・512×512) × 3 面
   で不一致 0。感度試験も 10/10・2/2 で検出。

以下、詳細。

## 1. SDK の版の絞り込み

### 取得とビルド (観測)
- `dl/` に 9.20〜18.06 の公式アーカイブ 27 点を取得、SHA256 を `notes/dl_sha256.txt` に記録。
  9.20 の SHA256 は公表値 `8ac221ac…445` と一致。
- `src/lzo_<ver>` (920〜1806 の 28 版) を clang -O2 -w -D_7ZIP_ST でビルド (`logs/build.txt`)。

### 動作スイープ (観測, `logs/version_sweep.txt`)
- 既存の本家ダンプ 228 件 (面バイト長 4,608〜131,072) を各版で圧縮しバイト比較。
- **9.20〜18.05: 228/228 全一致。18.06: 227/228 不一致 (1 件だけ偶然一致)。**
  最初の差の例: 面長 32,768 で len 3238 (18.05) vs 3244 (18.06)、差バイト位置 306。
- 18.05→18.06 の SDK 差分は `kNumOpts` 1<<12→1<<11 と `GetOptimum` の書き換え
  (`LzmaEnc.c` 差分 781 行)。本入力群 (辞書 ≤ 512KiB) では 18.05 まで出力が同一。

### 静的判別子 (観測, `notes/version_fingerprint.md`; Normalize は今回 disasm 全域を再確認)
- `LzmaEncProps_Init` = `0xb9c53c`: 触るフィールド/オフセットが 9.20 の `CLzmaEncProps` と完全一致
  (reduceSize なし)。
- `LzmaEncProps_Normalize` = `0xb9c5b4`〜`0xb9c7b8` (0x204 バイト): dictSize 既定分岐は
  `level==6 ? 1<<25 : 1<<26` (≤17.00 形)。**末尾に reduceSize の切り下げループがない** (=9.20。
  9.21+ は for(i=15..30)、9.35+ は for(i=11..30))。触るオフセットは
  0x0/0x4/0x8/0xc/0x10/0x14/0x18/0x1c/0x20/0x24/0x2c のみ。
- `MatchFinder_Create` = `0xb96220`: `hs = historySize - 1` 直書きで expectedDataSize
  キャップなし (17.01 未満)。sizeReserv は `> 2<<30 → >>2` の単一分岐 (9.38 以前の形)。
  `kMaxHistorySize` = 3<<30。
- 文字列 71,960 件に LZMA/7-Zip/Pavlov/版数の痕跡 0 件。

### 結論
静的・動的の両面で 9.20 系。9.20 と 18.05 の間の版の特定は本入力群では不可能 (推定)。

## 2. 検出されなかった突然変異 2 種

記録元: `sandbox/lzma_verify/` (既存の感度試験)。差の出し方の記録は本 wp の
`logs/hash4_probe.txt`・`logs/extra_lit_*.txt`、入力は `data/`。

### 2a. `hash4_mask_minus` (`hs |= 0xFFFF` → `0x7FFF`)
- **差が出る入力を構成** (`data/hash4_plane_s1..s8.bin`、生成法と bucket 探索は
  `data/hash4_bucket.json`・`scripts/hash4_probe.py`)。
- 構成: hash4 の bucket 0x3ee3 (=16099) に、bit15 が 0 のグループ 69 個 (lo) と 1 のグループ
  100 個 (hi) を共存させる。代表 A=(14,68,248,148)、開始位置 P=0xbf23ee3 (bucket 探索 4,222,991 draw)。
- 結果 (観測, `logs/hash4_probe.txt`): 8 シードすべてで base≠mutant (例 s1: 2041 vs 2042 バイト、
  最初の差 2012 バイト目)。**C 9.20 オラクル: base の blob と一致、mutant は不一致。**
  対照実験: 辞書 69,632 (>64KiB、両者マスク 0xFFFF) では 8 シードすべて一致 → 差は
  bucket 16 進下 4 桁の bit15 が入力長内で選ばれるかどうかにのみ依存。
- スロット検証: base は lo=0x3ee3 / hi=0xbee3 を返す (mutant は hi 側が 0x7ee3)。

### 2b. `extra_lit_rep_cond` (`if not nextIsChar and matchByte != curByte:` → `if not nextIsChar:`)
- 第 1 段: 85 入力 (合成 + 本家 57 面の出力比較バッテリ、`logs/extra_lit_probe.txt`):
  分岐本体の到達 1,064,310 回、変異のみの到達 365,103 回、len2 候補 25,342 回、
  DP スロット獲得 326 回 — **出力差 0**。計測入りトレース版の出力は変異版と同一 (計測の影響なし)。
- 第 2 段: 128 入力の狙った探索 (低エントロピー・周期+摂動、`logs/extra_lit_search.txt`):
  変異のみの到達 230,896 回・スロット獲得 19 回、出力差 0。
- 第 3 段 (`scripts/extra_lit_search2.py`, `logs/extra_lit_search2.txt`): 低エントロピー
  5 種の生成器 × 2,000 入力で **18 入力に出力差**。増分カウンタ型
  (`v += choice((0,0,0,1))`、255 ラップ) と周期+摂動型で差が出た。
- 検証 (観測): 18 入力すべてで **base == C 9.20 (バイト一致)、mutant ≠ C**。
- 最小化例: `data/extra_lit_minimal.bin` = 3,113 バイト (c1038 の最小前方切り出し)。
  ストリーム 910 バイト、最初の差は 905 バイト目。トレース統計: 変異のみの到達 1,830 回・
  len2 1,055 回・スロット獲得 3 回。
- 解釈 (推定): 変異の追加候補 (cur のリテラル + cur+1 からの rep0) は DP のスロットを取れるが、
  同じ目標位置へ来る直接 rep0 候補 (長さ +1) に上書きされるのが常で、最終経路に残るには
  価格が同額以下かつ経路の末端付近で選ばれる必要がある。第 1・2 段の入力群はその条件を満たさず、
  低エントロピー (リテラルが安い) データで初めて満たした。
- これにより、既存感度試験で「検出されなかった」2 種は両方とも検出可能になった。

## 3. `blob_size` の照合 (移植の関数を照合に含める)

### 本体側の仕様の確認 (観測, 逆コンパイル `fn_0x9d61f4.c`・`fn_0x9b41c4.c`)
- 呼び出し元 `FUN_009b41c4`: バッファ容量 = `W * H * 2 + 0x90` (3 面で同一のバッファを使い回す)。
- `FUN_009d61f4`: `destLen = 容量 - 0x10 = n + 0x80`、`dictSize = (n+0xfff) & ~0xfff`
  (n+0xfff ≥ 2^27 なら 0x8000000)、lc3 lp0 pb2 fb32 level 5・1 スレッド。成功なら
  先頭 16 バイト = [n (u32)][streamLen (u32)][props 5][3 バイト未初期化] を書いて
  `16 + streamLen` を返す。`LzmaCompress` の返し 0 以外なら **-1**。
- `LzmaEnc_WriteProperties` は props の辞書を 2<<i / 3<<i (i≥11) のはしごで切り上げて書く。
  移植 `props_bytes` は既にこのはしごを実装 (36864 → 49152 など、本家ダンプとも一致)。

### 照合 (観測, `scripts/verify_blob.py` → `logs/verify_blob.txt`)
- 容量を引数に取れるオラクル `src/lzo_cap.c` を作成 (既定は本家と同じ n + 0x90)。
- 入力 235 (本家 228 面のデコード + 合成 7: const/rand4096/8192/16384/32768/per7/hash4) ×
  容量 {n+0x90 (本家と同値), 16+L-1, 16+L, 16+L+1} の全組で、移植の判定
  `len(stream) > cap - 0x10` と C の戻り値 (rc=0 / SZ_ERROR_OUTPUT_EOF=7) が一致。
  **不一致 0。** 本家と同じ容量では blob 全バイト一致 (13..15 を除く比較でなく今回のオラクルは
  calloc 済みなので全バイト)、`blob_size` == 16+L。
- 感度: `>=` に変えた変異は 235/235 入力で cap=16+L の判定を反転させる (検出可能)。
- **追加の観測**: 乱数 (非圧縮) データはこの設定で約 +1.3% 膨らむ (rand8192: +106、
  rand16384: +167、rand32768: +456)。n ≳ 10KiB の非圧縮面は本家の上限 n+0x80 を超え、
  `FUN_009d61f4` は -1 を返し、呼び出し元はその面を書かない (後始末 `FUN_009ba4cc` で
  プロファイル先頭を 0 化)。移植の `blob → None` / `blob_size → -1` と一致。
  既存の本家ダンプはすべて圧縮性の画像面のため、この経路は今まで実データで踏まれていなかった。
- なお、既存オラクル `src/lzo.c` の容量は n*2+0x90 (本家より広い)。出力は容量不足時以外は
  容量に依存しないため、過去のバイト比較には影響しない (観測: 今回の全入力で同一出力)。

## 4. 境界の照合

### 4a. 辞書サイズ切り上げ境界 (合成, `scripts/verify_dict_boundaries.py` → `logs/verify_dict_boundaries.txt`)
- 面長: 2048/4096/8192/12288/32768/65536/131072/524288 の ±1、极小長 (1,2,15,16,17,63,64,2047,2049)、
  内容 3 種 (rand/per17/mixed) の 87 入力。
- 移植と `src/lzo_920` の比較: props 5 バイト・ストリーム全体が全入力でバイト一致 (不一致 0)。
  props の辞書は両側とも はしご(roundup4k(n)) に一致 (例: 32769 → 36864 → props 49152)。
  `blob` の None 判定は `len(stream) > n + 0x80` と自己一致。
- 感度: 誤った辞書 (切り捨て、8KiB 丸め、4096 固定、+0x1000) 10/10 ケースで検出。
  (辞書 < 面長の組は移植が NotImplementedError を送出するので感度試験から除外 —
  本家が使う設定は常に 辞書 ≥ 面長 のため、これは移植の既知の適用範囲内。)

### 4b. 本家の最小・最大領域 (観測, `scripts/verify_region_boundaries.py` → `logs/verify_region_boundaries.txt`)
- 手順: `nvh selftest` の終了コード 3 を確認 → `nvh profile 1920 1080 <rgb.f32> 1 <out> x y w h`。
  入力は test_v02 n=312 のフレームから生成 (ffmpeg rgb24 → float 3 面連結、`data/boundary/frame312.rgb.f32`)。
- 領域: (830,300) に 32×32・45×45・46×46・64×64・512×512 の 5 種 (領域指定のため FindFlatArea
  は不呼び出し、flat_status は -1)。面長は 2048 / 4050 / 4232 / 8192 / 524288 バイト。
  45×45 は 4KiB 境界の下 (4096)、46×46 は上 (8192) — 実経路で切り上げ境界の両側を通過。
- 結果: 15 面 (5 領域 × 3) すべてで移植 `blob` と本家 `out.bin.lzmaN` が [0:13] と [16:] で
  バイト一致 (不一致 0)。props 辞書 = はしご(roundup4k(w·h·2))、`blob_size` = 16+L も一致。
  32×32 (2,048 バイト) は既存本家 corpus の最小面長 4,608 を下回る新境地。
- 感度: r46 の面 1 で辞書 ±0x1000 の 2/2 ケースを検出。

## 本家との比較件数と不一致数 (今回の分)

| 照合 | 件数 | 不一致 |
|---|---|---|
| 版スイープ (本家ダンプ × 28 版) | 228 × 28 | 9.20〜18.05: 0 / 18.06+: 227/228 が不一致 (18.06 が初差) |
| blob_size・容量境界 (本家 228 面デコード + 合成) | 235 入力 × 4 容量 | 0 |
| 辞書境界 ±1 (移植 vs C 9.20) | 87 | 0 |
| 本家領域 32×32〜512×512 | 15 面 | 0 |
| 変異検証 (base vs C 9.20、差分入力) | hash4 8 + extra_lit 18 | 0 (base が常に C と一致) |

## 要相談 (統括の判断)

1. **感度試験への入力追加の推奨**: `data/extra_lit_minimal.bin` (または 18 差分入力) と
   `data/hash4_plane_s1.bin` を既存の感度試験 (`sandbox/lzma_verify/`) の標準入力群に加えるか。
   加えれば 14 変異すべてが自動検出される (推定: 本 wp の実測に基づく)。
2. **AGENTS.md §6.5 の版記述の更新**: 現在「SDK 9.20 と 16.04 が本家と同じ出力」だが、
   実測では 9.20〜18.05 が一致・18.06 が初差。反映するか (本 wp からは書けないため統括作業)。
3. **非圧縮面で -1 を返す事実**の AGENTS §6.5 (LZMA sizes 節) への追記も候補
   (実用上は画像面が非圧縮になることは稀という推定を付ける)。
4. ハーネスの新サブコマンドは不要だった (領域指定 `nvh profile` で足りた)。
   `harness_request.md` / `nvh_patch.diff` は作成していない。

## 成果物一覧 (wp_lzma 内)

- `REPORT.md` (本文書), `MERGE_PLAN.md`
- `dl/` 27 アーカイブ + `notes/dl_sha256.txt`
- `src/` 28 版オラクル + `lzo_cap.c` (容量指定) + `dec.c` + 変異 `m_hash4_mask_minus.py`・`m_extra_lit_rep_cond.py`
- `scripts/` `hash4_probe.py`・`extra_lit_probe.py`・`extra_lit_search.py`・`extra_lit_search2.py`・
  `trace_extra_lit.py`・`verify_blob.py`・`verify_dict_boundaries.py`・`verify_region_boundaries.py`・`readonly_check.py`
- `data/` 本家 228 面のデコード (`genuine_planes/`)、差分入力 (`extra_lit_diff2_*.bin`・`extra_lit_minimal.bin`・
  `hash4_plane_s*.bin`)、`hash4_bucket.json`、`boundary/` (フレームと本家プロファイル断片)、`blob_sizes.json`
- `logs/` 各種実行ログと `readonly_before.txt`/`readonly_after.txt`
- `notes/` `version_fingerprint.md`・`dl_sha256.txt`

## 読み取り専用領域の無変更確認

`scripts/readonly_check.py`: `logs/readonly_before.txt` の 1,505 エントリ (Agents.md・AGENTS.md・
repro/・neatvideo_ofx_614/・sandbox/lzma_verify/ など) を mtime (整数秒)・サイズで照合。
**missing 0 / changed 0** (`logs/readonly_after.txt`)。
