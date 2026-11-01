## Verification Report — INT-3 Round 2（M-1・L-1・L-2 への対応）

### Verdict
**Status**: PASS（判定 **APPROVE**）
**Confidence**: high
**Blockers**: 0（新しい指摘は情報 3 件だけ。L-3 は INT-4 へ持ち越し）

M-1・L-1・L-2 はどれも直っていて、回帰で新たに壊れたものはありませんでした。

作業場所は `/private/tmp/claude-501/verify-int3r2/` です。
- qtui は worktree から同期し、ビルド物を消してクリーンビルドしました。
- worktree の `git status` は検証の前後とも 29 行（M 20・?? 9）で変わっていません。
- `~/.config/NeatRepro/nv_ui_qt.ini` の sha256 は、検証の前・selftest の後・verify_ofx の後・最後のいずれも `45d4258c…fcc6` です。
- 本家、Resolve、/Library/OFX、ユーザーの MeatVideoQt には触れていません。

### Evidence（どれも検証者が自分で実行した結果）

| Check | Result | Command/Source | Output |
|---|---|---|---|
| Round 1 からの差分 | 確認 | `diff -rq` で Round 1 の写しと比べた | 変わったのは qtui の 7 ファイルだけ（mainwindow.cpp、export.cpp、video.{h,cpp}、fieldprof.cpp、selftest.cpp、verify_interlace.py） |
| native・reference・ofx | 変更なし | `cmp`（native の全ソースと Makefile）、`git diff HEAD -- reference ofx` | 差 0 |
| make verify と verify_bridge | 再実行していない | 判断の根拠は右欄 | native は、Round 1 で `make verify` rc=0、INT-2 で verify_bridge ALL MATCH を出した版と 1 バイトも違わないため |
| qtui のクリーンビルド | pass | cmake → build | rc=0、warning 0 |
| qtui_selftest | pass | offscreen | 137 passed / 0 failed。int3UnknownOrderAsks と int3UnknownOrder420AutoProfile も PASS |
| verify_interlace | pass | 新しい qtui_export と qtui_fieldprof | ALL OK（詳細は基準 1・2） |
| compare_export（写し） | pass | REPRO を直した写し | 19/19 SAME、RESULT ALL SAME |
| 変更前の C++ との直接比較 | pass | INT-2 時点の qtui_export（main と同じ qtui）の出力と比べた | 82 ファイルで差 0（mp4・ts・mov・EXR はバイト、mkv は framemd5 とファイル長、json は seconds・src を除く） |
| OFX | pass | `build_ofx.sh` → `verify_ofx.py --quick` | warning 0。81 比較で 0/67,028,480 語の不一致。ini は unchanged |
| interlaced_vf の条件表 | pass | 検証者の C++ `qtui_vif` | 3 種の処理形式 × vf の有無 × ソースの形式 15 種、計 90 通り（詳細は基準 1） |
| 検証者の変異 8 種 | 下表 | `qmut.py`（毎回、関係するファイルを強制的に再コンパイル） | 下表 |

**検証者の変異 8 種**

| 変異 | 照合 | 結果 |
|---|---|---|
| NONE（変更なし） | verify_interlace | ALL OK |
| AREA_NOHALF（y0 を半分にしない） | verify_interlace | **検出**（領域ありのプロファイル 2 件が DIFFER） |
| AREA_H_NOHALF（h を半分にしない。検証者が追加） | verify_interlace | **検出**（2 件が DIFFER） |
| X264_NO_BFF（BFF でも tff=1 を渡す） | verify_interlace | **検出**（mp4・mov のストリームが tt で UNEXPECTED） |
| MARGIN（フレームの余白 = R/2） | verify_interlace | **検出**（R3 で 99,510 / 62,990 バイト差） |
| D32_DROP（`:interl=1` を足さない。M-1 を元に戻す） | verify_interlace | **検出**（depth 32 で 863,975 語差） |
| D32_CHAIN（`:interl=1` の代わりに `,scale=interl=1` を連ねる） | verify_interlace | 検出しない。等価な変異とみられる（情報 2） |
| PROG_EXPORT_IL（progressive でも interlaced_vf を通す） | compare_export | **検出**（e4_32float_mkv の出力とコマンドが DIFF） |

### Acceptance Criteria

| # | 基準 | Status | Evidence |
|---|---|---|---|
| 1 | M-1 の修正が正しい | VERIFIED | 下記 1 |
| 2 | L-1: 前回素通りした 3 種を検出する | VERIFIED | 下記 2 |
| 3 | L-2 の挙動 | VERIFIED | 下記 3 |
| 4 | 回帰がない | VERIFIED | 上の Evidence の表のとおり |

**1. M-1 の修正**
- 条件（`video.cpp`、読んで確認）: 「ソースが 4:2:0 か」と「処理の形式が 4:2:0 か」が食い違うときだけ interl=1 を付けます。
  - vf が空なら `scale=interl=1` にします。
  - vf が単独の scale なら `:interl=1` を足します。
  - それ以外は `,scale=interl=1` を連ねます。
- 3 か所の呼び出し（読んで確認）:
  - 書き出しは `if (il)` のときだけ。
  - open_video は `if (sc >= 0)` のときだけ。
  - set_scan の読み直しの判定は、progressive 側が read_filter のままの `base`、インターレース側が `interlaced_vf(base, …)` で、両者を比べます。
  - したがって、progressive の経路は interlaced_vf を通りません。変異 PROG_EXPORT_IL を compare_export が検出したので、この守りは試験でも押さえられています。
- 条件表（観測）:
  - 付くもの: 4:2:0 系（yuv420p・yuvj420p・yuv420p10le・nv12）から 4:2:2・4:4:4・RGB へ。4:2:2・4:4:4・RGB・411・440 から 4:2:0 へ。
  - 付かないもの: 4:2:0 から 4:2:0、4:2:2/4:4:4/RGB から 4:2:2/4:4:4。
  - read_filter の scale がある場合は `…:in_range=tv:interl=1` になります。
- 実データ:
  - depth 32 の 4:2:0 素材で、5 フレームの float 語の不一致は 0 でした。
  - interl=1 を付けないと、0 フレーム目の RGB 172,800 標本のうち 170,925 が異なります。
  - 4:2:2 を 8 ビットで読む経路と 4:2:2 素材の書き出しも、引き続きビット一致しました。

**2. L-1（照合の補強）**
- 前回素通りした 3 種（AREA_NOHALF・X264_NO_BFF・MARGIN）は、今回すべて verify_interlace が検出しました。
- 検証者が新しく足した AREA_H_NOHALF と、M-1 を元に戻す D32_DROP も検出しました。
- 加わった照合はすべて通っています。
  - 領域ありのプロファイル 2 件が全値一致。
  - 奇数の半径（plain R3、same R5、shift R3）の書き出しがビット一致。
  - ストリームの field_order: x264 の mp4・mov は TFF で tt、BFF で bb。mkv・ffv1・prores は tb/bt。

**3. L-2**
- 読んで確認したこと:
  - ask_scan のボタンは、`set_scan(want)` の後で、`scan == want`、プロファイルなし、Auto Profile の設定が有効、フレームあり、の条件がそろったときだけ auto_profile を呼びます。
  - 読み直す場合は、open_video が終わるまで scan が変わらないので、ここでは呼ばれません。二重には走りません。
  - Progressive と答えたとき（scan はもともと −1）も、ここで Auto Profile が走ります。
  - 読み直しを伴う切り替えでも、プロファイルがあれば本家の文言の箱を出します。
- 実行で確認したこと:
  - int3UnknownOrder420AutoProfile が PASS しました。読み直しをせず、Two fields の Auto Profile と AFT が走ります。
  - int3UnknownOrderAsks の中の文言の確認は `if (had)` の中にあるので、空振りしていないかを確かめました。写しに一時的に出力を入れると `had=1` で、確認は実際に実行されていました。写しは元に戻し、worktree のソースには手を加えていません。

### Gaps
- **[Low] L-3 Undo と走査方式の食い違い。** 統括の指示どおり INT-4 へ持ち越しです。変化はありません。
- **[情報 1] 440 と gray の扱い。**
  - 440 のソースは 4:4:4 として扱われるので、RGB で読むときに interl=1 が付きません（作業者が記録済み）。440 から 4:2:0 へは付きます。まれな形式です。
  - gray は 4:2:0 として扱われ、4:4:4・RGB で読むときに interl=1 が付きます。gray には色差がないので害はない、とみています（推測）。
- **[情報 2] D32_CHAIN を検出しないこと。** `:interl=1` の代わりに `,scale=interl=1` を連ねても、出力がビット一致しました。推測ですが、ffmpeg が形式の変換を後ろの scale で行うので、結果が同じになるのだと思います。照合の弱さとは考えていません。
- **[情報 3] progressive の守りを確かめる試験は 1 ケースだけ。** compare_export のうち変異 PROG_EXPORT_IL を検出したのは e4_32float_mkv の 1 件だけです。8 ビットの 4:2:2 を progressive で処理するケースがあれば、守りがもっと確実に確かめられます（任意）。

### Recommendation
**APPROVE。** M-1・L-1・L-2 はどれも正しく直っていて、検証者の変異でも照合の感度を確かめました。プログレッシブの経路は、変更前の C++ と 82 ファイルで同一、compare_export・selftest 137・verify_ofx もすべて一致しました。

検証者のプログラムとログは `/private/tmp/claude-501/verify-int3r2/` にあります。
- C++: `new/qtui/tests/vif.cpp`（`qtui_vif`）
- スクリプト: `qmut.py`
- ログ: `qmut.log`、`verify_interlace.log`、`selftest.log`、`cexp.log`、`verify_ofx.log`、`qtui_build.log`、`ofx_build.log`