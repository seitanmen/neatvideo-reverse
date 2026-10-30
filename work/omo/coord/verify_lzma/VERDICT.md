# VERDICT wp_lzma (独立検証, 2026-10-17)
作業場所: sandbox/omo/coord/verify_lzma/ (scripts/ = 私のスクリプト、bin/ = 私のビルド、py/ = repro/nv_lzma.py ほかのコピー、nvh/ = nvh profile の出力)。
読んだだけ: wp_lzma の REPORT/MERGE_PLAN/src/m_*.py/data、repro/nv_lzma.py。作業者のビルド物 (src/lzo_*, sdk*) と dl/x は使っていない。

## 1. SDK 版: CONFIRMED (中間版は部分的)
- アーカイブ SHA256 (私が取得): 9.20 tar.bz2 = 7-zip.org 取得分 `8ac221ac…a445` = 作業者の dl と一致。16.04 (7-zip.org) `edf8690e…b04f` = 既存 README と一致。
  18.05 `d4ad3820…4732`・18.06 `01f8f7af…a3f6`: 7-zip.org は 404 のため SourceForge と Wayback (7-zip.org の過去版) の 2 経路から取得し、2 経路とも作業者の値と一致。
  追加で 17.01・18.01・18.03 も SourceForge から取得し作業者の値と一致 (作業者の dl/ が改竄されていない証拠)。
- 私が bsdtar で自分のフォルダに展開し、自作 `src/lzo_cap.c` (容量 n+0x90 = 本家と同じ) を clang -O2 -D_7ZIP_ST でビルド (libSystem のみ、otool -L で確認)。
- 本家ダンプ: `neatvideo_ofx_614/analysis/noise/model_verify/tmp/*.prof.lzma0/1/2` 228 個 (76 回の `nvh profile` の出力、本家の直接出力)。
  私が Python lzma (FORMAT_ALONE) で面を復号し (n は 4,608〜131,072、8 種)、各版で再圧縮して [0:13] と [16:] を比較 (`scripts/sweep.py`):
  9.20 228/228、16.04 228/228、17.01 228/228、18.01 228/228、18.03 228/228、18.05 228/228、**18.06 は 1/228 だけ一致 (227 不一致, 例 n=8192: 3244 vs 本家 3238)**。
- 未検証: 9.21〜16.03・17.00 など残りの中間版は自分では回していない (作業者の主張のまま。ただし 9.20 と 16.04 と 17.01 が一致、18.05 まで一致なので区間としては整合)。
- 制約: ダンプは 8 種の面長だけ (辞書 ≤ 128KiB)。大辞書での版差は未検証。
- 静的判別子は項目 5 参照。

## 2. 未検出変異 2 種: CONFIRMED
- 変異版が 1 箇所だけの変更か: `diff repro/nv_lzma.py src/m_*.py` → hash4: `hs |= 0xFFFF`→`0x7FFF` の 158 行のみ、extra_lit: 821 行 `and matchByte != curByte` 削除のみ (他に差なし。sandbox/lzma_verify/mutants 版とは 1157〜1158 行の errstate だけ異なり、これは repro 側の新しい修正)。
- 3 者比較 (`scripts/mut.py`, 移植=repro/nv_lzma.py のコピー、C=私のビルドの 9.20):
  - hash4_plane_s1..s8 (2048B): 8/8 で 移植==C 9.20、変異≠C (例 s1 長さ 2057/2058/2057)。
  - extra_lit_minimal.bin (3113B) と diff2 4 件: 5/5 で 移植==C、変異≠C (例 minimal は長さ同じ 926 だが内容が違う)。
  - 対照: 本家の面 (n=8192) では変異==移植 (従来どおり未検出) → 新入力でだけ検出される。
- 制約: hash4 は面長 2048 (辞書 4KiB、64KiB 以下限定の変異) なので 32×32 領域相当の入力でのみ実戦的に効く。

## 3. 容量式と -1 経路: CONFIRMED
- 逆アセンブル (`/tmp/nv_expand/disasm_full.txt`):
  - FUN_009b41c4 0x9b4290〜0x9b4298: `x8 = W*H; x8 <<= 1; x21 = x8 + 0x90` を `0x9d61f4` の x3 (容量) に渡す (0x9b4310)。
  - FUN_009d61f4 0x9d6204〜0x9d620c: `n+0x90 <= cap` のアサーション、0x9d6254: `destLen = cap - 0x10` (= W*H*2+0x80 = n+0x80)。辞書 0x9d625c〜0x9d6270: `(n+0xfff)&~0xfff`、`>>27 != 0` なら 0x8000000。
    引数: level 5、lc 3 / lp 0 / pb 2 / fb 32 / threads 1 (スタック引数を解読)。出力 [n u32][destLen u32] を +0, +4 に書き、props は +8 (5 バイト)、+13..15 は書かない。戻り値 destLen+0x10。
  - LzmaCompress (0xba3704) が非 0 → `mov x0,#-1; ret` (0x9d62a8〜0x9d62ac)。呼び出し元は負値で 0x9b43d4 (後始末 0x9ba4cc) へ。
- 動作: ランダム n=4096/8192 は rc=0、16384/32768 は rc=7 (OUTPUT_EOF) を 9.20・18.05 の両方で確認。移植 blob_size は 4169/8317/-1/-1、blob は 16384・32768 で None (一致)。
- 推定のまま: 「この経路が本家実機で発火する」ことは本家に非圧縮面を渡して確かめていない (逆アセンブルと C オラクルから)。

## 4. 境界 (32×32 / 45×45 / 46×46): CONFIRMED
- `nvh selftest` 終了コード 3 を確認後、`nvh profile 1920 1080 f100.rgb.f32 1 p<S>.bin 700 200 S S` を S=32,45,46 で 1 回ずつ (作業者とは別のフレーム n=100・別座標 (700,200); FindFlatArea 不呼出, flat_status -1)。
- 移植は本家出力の復号に頼らず、入力の RGB から `nv_lzma.plane_int16` → `blob` で独立に作成 (`scripts/region.py`):
  9 面すべて 移植==本家 ([0:13], [16:] と長さ)、C 9.20・18.05 とも一致。ヘッダ語 0x18..0x20 = 16+L も一致。props の辞書: 32×32 (2048B) と 45×45 (4050B) は 4096、46×46 (4232B) は 8192 = 4KiB 切り上げ境界の両側を実経路で通過。
- 制約: 領域は 1 箇所のみ (各サイズ 1 回)。512×512 は私は取っていない (作業者の主張のまま)。

## 5. 静的判別子: CONFIRMED (+ 補足)
- `LzmaEncProps_Normalize` 0xb9c5b4〜0xb9c7b8 を全域読んだ: level<0→5、dictSize==0 → level≤5 で 1<<(2*level+14)、level==6 で 1<<25、他は 1<<26、lc/lp/pb/algo/fb/btMode/numHashBytes/mc/numThreads の既定。触るオフセットは 0x0〜0x2c のみで **reduceSize の切り下げループなし**。私の 9.20 ソースの Normalize と同形。16.04 の LzmaEnc.h には reduceSize (UInt64) がある。
- `LzmaEncProps_Init` 0xb9c53c: level=5、dictSize=0(+4)、mc=0(+0x24)、writeEndMark=0(+0x28)、他は -1 (+8..+0x20, +0x2c) = 9.20 の構造体形。
- 補足 (推定): この静的形は 9.21 以降 (reduceSize 導入) では成り立たないため、**ソース上の版は 9.20 系 (≤9.20)**、動作上は 9.20〜18.05 が区別不能、という二層の書き方が正確。作業者の「9.20 形状」はここまで言える。MatchFinder_Create は私は読んでいない (作業者の主張のまま)。

## マージしてよい事実
- 動作: 9.20 / 16.04 / 17.01 / 18.01 / 18.03 / 18.05 が本家ダンプ 228/228 とバイト一致、18.06 は不一致 (227/228)。
- 変異 2 種は hash4_plane_s1..8 / extra_lit_minimal.bin で検出可能 (移植==C、変異≠C)。
- 容量式 W*H*2+0x90 / destLen=cap-0x10=n+0x80 / 辞書切り上げ / lc3 lp0 pb2 fb32 / 失敗時 -1。非圧縮 16KiB 以上で -1 (C オラクルと移植)。
- 32/45/46 の領域で移植と本家 .lzma0/1/2 が一致 (9/9 面)。
- 静的: Normalize に reduceSize ループなし、Init の構造体形が 9.20 と一致。

## まだマージしない方がよい事実
- 「9.20〜18.05 の全版が一致」は 9.20, 16.04, 17.01, 18.01, 18.03, 18.05 のみ私が確認。残りの中間版は作業者の報告のみ。
- 非圧縮面で本家が実際に -1 を返すこと (実機では未観測)。
- 512×512 の一致と MatchFinder_Create の静的所見 (私は再確認していない)。
- 大辞書 (>128KiB) での版差・hash4 型の変異が大面で効くか (範囲外)。
