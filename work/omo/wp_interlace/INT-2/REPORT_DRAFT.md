# INT-2 作業者報告 (int2-worker、SendMessage 3 通の転記。統括が転記、本文は無改変)

INT-2 最終報告 (1/3): INT-2 は完了。L-1 は main に修正済みで、再確認だけ行った。設計と変更ファイルはこの通に書く。コミットはしていない。worktree は .claude/worktrees/int2 (branch int2-fields)。

## 1. L-1 (コードは変更していない)
- 観測: L-1 の修正は main に入っている。INT-1 の Round 1 対応が 6fb424f でマージされ、VERDICT Round 2 で APPROVE になっている。
  - qtui の video.cpp の scan_of / first_field は、最初のフレームの interlaced_frame / top_field_first で順序を決める。旗が無いときは stream の値を使うが、tt / bb だけで、tb / bt は順序の根拠にしない。
  - 表示は判定の結果で、stream の値と食い違うときは "(stream field_order: tb)" を併記し、ツールチップにも生の値を出す。
  - L-2 (join の重なりは拒否する) と I-1 (weak は 0) も main で解消済み。今回の dylib でも `nm -m` の weak external は 0 だった。
- 再確認 (観測)
  - 素材: test.mp4 n=300..309 を 60p とみなし、tinterlace で作った (interleave_top/bottom + setfield)。
  - 確かめ方: qtui の libnvui.a にリンクした小さなプログラム (scratchpad/int2/scanprobe) で probe → first_field を出した。それとは別に、各フィールドが元のどちらのフレームから来たかを画素で調べた (`work/omo/wp_interlace/INT-2/l1_actual.py`)。
  - 結果:
    - ffv1 (mkv・mov) と prores +ildct は、stream が TFF で tb、BFF で bt と書かれているが、first_field は 0 / 1 で、実際の順序と一致した。
    - mpeg2 tt/bb と x264 tt/bb も一致した。
    - prores の +ildct なしは "field order unknown" で、first_field は −1。
    - progressive は −1。
    - 11 本のうち一致が 10 本、不明が 1 本で、食い違いは 0 本 (logs/l1_actual.log、l1_scanprobe.log)。
  - `verify_fields.py --video` は、ffv1 tb/bt・ffv1 mov・prores tb/bt・mpeg2 tt/bb の計 7 本で ALL BYTE-EXACT。tb の素材でも separatefields と 4/4 で一致した (logs/verify_fields_l1.log)。
- 順序を決められないとき (決定)
  - first_field = −1 (progressive / scan unknown / interlaced で順序不明) なら、フィールド経路には入らない。
  - API `nvfp::process_frame_fields` は first ∉ {TOP, BOTTOM} で std::invalid_argument を投げる。
  - 試験用 CLI (--order auto) は「--order tff/bff を指定せよ」と言って止まる。
  - INT-3 への提案: 本家と同じく Video Scan Type をユーザーが選べるようにし、自動判定の値を既定にする。不明のときはユーザーに聞く。

## 2. 設計
- **フィールド系列**: window のフレーム k のフィールド 2k+j の偶奇は j ^ first。つまり BFF なら下が先。
- **時間半径**: P.radius はフィールド数で、中心 s の近傍は s−R..s+R。
  - 端の扱い: 渡された window の中で切り詰める (process_frame_v5 と同じ)。
- **フィールドの処理**: 各フィールドに temporal_v5 → spatial_v5 (既存のものを高さ半分の画像にそのまま掛ける) → Mix をかけ、nv_fields の split/join (コピー) で再結合する。
- **半ラインのずれ**: 3 方式を実装し、効果の試験 (3/3 通目) で比べた。
  - PLAIN: 両方の偶奇をそのまま使い、ずれの推定は動き推定に任せる。
  - SHIFT: 逆の偶奇のフィールドに既知のずれを与える。
    - 量: dy = 0.5·(中心の偶奇 − 近傍の偶奇) フィールド行。
    - flow は warp_plane(近傍 Y, dy) を相手に推定し、輝度は FY+dy で 1 回だけ補間する。
  - SAME: 中心と同じ偶奇のフィールドだけを使う (±2, ±4… ≤ R)。
- **ずれの符号と色差の量** (観測): きれいな静止フレームの下フィールドを上フィールドに合わせ、誤差の RMS を比べた。
  - 輝度: d=−0.5 で 3.75。d=0 で 5.66、+0.5 で 7.49、−0.25 で 4.29、−0.75 で 4.37 で、−0.5 が最小。
  - 色差 (4:2:0): −0.5 色差フィールド行で U 0.90。v5 の FY/2 流の −0.25 では 1.07、0 では 1.53 で、−0.5 が最小。
  - 理由: 4:2:0 の色差の行 c はフィールド c&1 に属するので、色差フィールドも 0.5 色差行 (= 1 輝度フィールド行) ずれている。
  - そこで SHIFT では、色差には「動き FY/2 + 0.5」を加える。輝度のずれをそのまま半分にした値 (0.25) にはしない。4:4:4 も 0.5。
- **プログレッシブ経路を変えないための形**
  - nvt::temporal_v5 に省略可能な引数 `const double* shift = nullptr` を足した。nullptr のとき、またはずれが 0 のフレームは、従来と同じ命令列を通る (分岐で避けている)。
  - process_frame_v5 と既存の呼び出し元は変えていない。
- **C ABI**: `nvc_temporal_shift` (nvc_temporal の引数 + shift[2·nf]) を追加した。NULL なら nvc_temporal と同じ。nvc_temporal は共通の内部関数を呼ぶ薄い包みにした (動作は同じ)。
  - **NVC_ABI_VERSION は 3 のまま**。根拠は N4e の前例どおり、追加だけで既存の意味は変わらないこと。加えて、repro/nv_native.py は版がちょうど 3 であることを要求するので、4 にすると Python ブリッジ (と凍結した PySide6 版) が libnvcore を読まなくなる。この判断は nvcore_c.h に注記した。
  - nv_fieldproc は dylib に入れていない (今は C ABI から呼ばない)。INT-3 は nvc_temporal_shift + 既存の nvc_spatial と、フィールドの分離・結合で組める。

## 3. 変更したファイル
- 新規 (native/)
  - nv_fieldproc.{h,cpp}: split_fields / join_fields / process_field_v5 / process_frame_fields
  - fields_ref.py: Python の参照。nvfilter は変更していない。temporal_shift は temporal_v5 の写しにずれを足したもの。
  - verify_fieldproc.py
  - fields_cli.py: 試験用 CLI
- 新規 (work/omo/wp_interlace/INT-2/)
  - effect_fields.py
  - l1_actual.py
  - backup/: 変更前のファイル 9 本。SHA256SUMS.txt を付けた。
  - logs/
- 変更
  - native/nv_temporal.{h,cpp}: shift 引数
  - native/nvcore_c.{h,cpp}: nvc_temporal_shift
  - native/nv_v5_main.cpp: kind 30 (temporal + shift) と kind 31 (process_frame_fields)
  - native/Makefile: V5_SRC に nv_fieldproc / nv_fields を追加し、verify に verify_fieldproc.py を追加
  - native/Makefile.v5・Makefile.libm_alt: V5_SRC (どちらも写しでビルドできることを確かめた)
- 変更していない: qtui/・ofx/・reference/ (nvfilter・nv_aft_v5・nv_depth・nv_native・PySide6 版)。AGENTS.md と MIGRATION.md も触っていない。
- 注意: work/omo/wp_interlace/INT-2/ 以下は .gitignore の対象なので、追跡するなら git add -f が要る。

INT-2 最終報告 (2/3): 回帰とビット一致は全件合格。ログはどれも work/omo/wp_interlace/INT-2/logs/ にある。照合は Python 3.13.5 / numpy 2.1.3 / scipy 1.15.3 で行った。

## プログレッシブ経路が変わっていないこと (観測)
- **`make -C native verify`** (14 本、verify_fieldproc を含む): rc=0 (make_verify.log)。
  - quality 4000/4000
  - band (real 108/108)
  - curve ALL BIT-EXACT
  - lzma 667/667
  - flat ALL BIT-EXACT
  - flat_cancel ALL IDENTICAL
  - model (real 56 + 280)
  - temporal (real frame 6/6・temporal 36/36、flow を含む)
  - spatial 1287/1287
  - profile (real 12/12・real2 9/9)
  - aft ALL BIT-EXACT
  - match 1107 ALL MATCH
  - fields ALL BYTE-EXACT
  - fieldproc ALL BIT-EXACT
- **`cd native && python3 verify_bridge.py`** (full): ALL MATCH、769 秒 (verify_bridge_full.log)。
  - curve 20・profile 9・aft 26・frame 72 (306,334,381 語)・cancel 49・fallback 9・g8 152・prof5・ui・depth (208,737,760 語)。
- **qtui_selftest**: 130 passed / 0 failed (qtui_selftest.log)。
- **`ofx/tools/verify_ofx.py --quick`**: 81 比較で 0/67,028,480 語の不一致、ALL MATCH。ini は変わっていない (verify_ofx_quick.log)。
  - 注意: このスクリプトは qtui/build/qtui_ofx_session_drv を前提にしている。そのため build_main とは別に qtui/build も作った。
- **書き出しのバイト一致** (compare_export、ROOT/REPRO だけを直した写し、--exe は worktree の build_main/qtui_export)
  - (i) 新しい lib 同士: 19/19 SAME、RESULT ALL SAME (compare_export.log)。
  - (ii) Python 側だけを**変更前の libnvcore.dylib** に向けたもの (NV_NATIVE_LIB で、ロードされたことを確認): 19/19 SAME (compare_export_vs_baseline_lib.log)。これで、新旧の lib を跨いでも出力がバイト単位で同じであることを示した。
- **dylib の公開シンボル** (旧 → 新)
  - 増えたのは `_nvc_temporal_shift` だけ。`nvt::temporal_v5` は引数が 1 つ増えたのでマングル名が変わった (C++ 内部のシンボルで、C ABI ではない)。
  - weak は 0。
- **クリーンビルドの警告**
  - native (`make clean` → `make all`): warning 0。
  - qtui (新しい build_main) と ofx/build_ofx.sh: warning 0。
  - `make clean` の後、nv_quality_test は git checkout で戻した。
- **ini**: ~/.config/NeatRepro/nv_ui_qt.ini の sha256 は前後とも 45d4258c…fcc6。

## フィールド経路のビット一致 (verify_fieldproc.py の全件、verify_fieldproc_full.log)
- **参照の写しが元と同じであること**
  - fields_ref.temporal_shift を、ずれなしとずれ 0 の両方で nvfilter.temporal_v5 と比べた: 24/24、631,604 語が一致。
  - PLAIN モードの process_field を、フィールド系列に対する nvfilter.process_frame_v5 と比べた: 一致 (追加後の quick で 12/12 フィールド)。
- **kind 30 (temporal + shift)**: 参照と比べて 48/48、630,486 語が一致。
  - 範囲: 4:2:0 と 4:4:4、ずれ 0 / ±0.5 / 輝度だけ / 色差だけ / 任意、flow を与えた場合、v5 と pyramid。
  - ずれ 0 の kind 30 は、C++ の kind 2 (従来の temporal_v5) と 12/12、98,152 語が一致。
- **kind 31 (process_frame_fields)**: 96/96、759,048 語が一致。
  - 範囲: H は奇数・4 で割って 2 余る・4 の倍数、4:2:0 と 4:4:4、TFF と BFF、PLAIN / SHIFT / SAME、R 0..5、窓の端、Amount 0・負、Mix。
- **real**: test.mp4 を tinterlace した 640×360 (TFF と BFF)、フィールドのプロファイル、3 モード: 18/18、6,220,800 語が一致。
- **bridge** (ctypes で nvc_temporal_shift): NULL のとき nvc_temporal と、ずれを与えたとき参照と比べて 24/24、712,208 語が一致。
- **空振りしないこと**: C++ のソースを書き換えた変異 8 種を、一時ディレクトリでビルドして照合した。8 種とも検出した。
  - SIGN 20/144
  - CHROMA_HALF 20
  - SAME_OFF 18
  - RADIUS 18
  - ORDER 48
  - NO_PRESHIFT 54
  - CHROMA420_BEFORE 34
  - LUMA_SHIFT_OFF 55
- **再現手順**: `make -C native nv_v5_test lib && python3 native/verify_fieldproc.py [--quick] [--sensitivity]`
  - --sensitivity なしで約 30 秒。make verify に入れた。

## 速度 (参考。負荷平均 15〜22 の中で測った、1080p の 5 フレームの窓)
- フレーム単位 (kind 3): 0.57 秒
- SHIFT R2 (kind 31): 0.59 秒
- SAME R2: 0.34 秒

INT-2 最終報告 (3/3): フィールド単位にすると動き部は大幅に減ったが、**静止部は減らず、逆に増えた**。タスク文の想定とは違うので、正直に書く。効果の表と残るリスクをこの通にまとめる。

## 効果の確認 (観測。work/omo/wp_interlace/INT-2/effect_fields.py、logs/effect_s6.log・effect_fields.json)
- **素材**: 合成の 60p 640×360 (静止したテクスチャ、横じま・縦の細部、縦にも動くテクスチャ付きの円)。ノイズは σ=6 (色差 3) で、yuv420p にしてから ffmpeg の tinterlace で 30i にした。TFF は interleave_top、BFF は interleave_bottom。
- **比べ方**: きれいな元画像を同じく tinterlace したものとの RMS。26 フレームで評価した。
  - 静止部: 円が一度も通らない画素。
  - 動き部: 両フィールドの時刻の円を 2px 広げた範囲。
  - comb: 輝度の誤差の縦方向の高域 e[y] − (e[y−1]+e[y+1])/2 の RMS。
- **処理**: (a) は今のフレーム単位で、フレームから作ったプロファイル。(b) はフィールド単位で、フィールドから作ったプロファイル。どちらも v5 の既定値。

TFF (BFF もほぼ同じ値):
| 処理 | Y 静止 | Y 動き | U 動き | V 動き | comb 動き | PSNR Y 全体 |
|---|---|---|---|---|---|---|
| 入力 | 6.01 | 6.02 | 1.53 | 1.54 | 7.38 | 32.55 |
| (a) フレーム R2 | **1.24** | 6.40 | 2.81 | 4.01 | 11.23 | 43.12 |
| (b) PLAIN R2 | 1.97 | 3.81 | 2.57 | 3.96 | 4.69 | 42.22 |
| (b) SHIFT R2 | 1.93 | 3.92 | 2.53 | 3.87 | 4.84 | 42.35 |
| (b) SAME R2 | 1.64 | **3.81** | 2.77 | 4.38 | 4.78 | 43.28 |
| (b) SHIFT R4 | 1.84 | 4.23 | **2.37** | **3.60** | 5.29 | 42.51 |
| (b) SAME R4 | 1.35 | 4.05 | 2.59 | 4.03 | 5.13 | **44.30** |

- **動き部**: (b) で大幅に減る。
  - Y の RMS は 6.40 → 3.81 (−40%)。
  - comb は 11.2 → 4.7 (−58%)。
  - (a) の動き部は入力より悪い (comb 11.2 > 入力 7.4)。フレーム単位の処理では、2 つの時刻が混ざったフレームに残像が出ている。
  - σ=3 では (a) の Y 動き 6.08 に対し (b) は 2.1〜2.2、σ=10 では 8.66 に対し 5.8〜6.3 で、傾向は同じ (logs/effect_s3.log、effect_s10.log)。
- **静止部**: (b) で減らず、逆に増える (Y 1.24 → 1.35〜1.97)。
  - 切り分け (観測、scratchpad の diag_static.py): 時間段だけ (空間の量 0) にすると、(a) 2.24 に対し SAME R4 2.33 で、ほぼ同じだった。差の大部分は空間段で出ている (全体で 1.20 対 1.32)。プロファイルをフレームのものに替えても 1.319 で、変わらなかった。
  - 推測: 高さが半分のフィールドでは縦の細部の周波数が 2 倍になり、空間フィルタがそれをノイズとして削らない。静止部では、フレームの空間フィルタが両フィールドをまたいだ縦の相関を使えている。
  - SHIFT が SAME より静止部で悪い理由 (観測): きれいな静止画像でも、逆の偶奇のフィールドを双線形の 0.5 行補間したものと中心フィールドとの差は RMS 3.75 あり、σ=6 に近い。細部のある場所では、補間した近傍が「似ているが偏った」標本になる。
- **どれを既定にするかの提案** (INT-3 で決めてほしい)
  - 輝度だけを見るなら SAME が最良。同じ R で SHIFT・PLAIN より、静止部も動き部も良い。
  - 色差の動き部は SHIFT の方が良い。
  - R をフィールド数で数えると、本家と同じ R=2 では SAME の標本は 3 つしかない。R4 にすると、静止部はフレーム処理に近づく (1.35)。
  - API はモードを引数で受け取る。既定は呼び出し側 (INT-3) が決める。

## 残るリスクと、気づいたこと
1. 静止部はフレーム単位の方が良い (上の表)。本家がどうしているかは解読されていない。INT-4 の本家比較で確かめるべきである。
2. SHIFT の補償は双線形補間で、補間の誤差は縦の細部で大きい。もっと良い補間 (縦の多タップや、動き適応のデインターレース) は、今回の範囲の外とした。
3. フィールド用のプロファイル (Two fields / One field) は INT-3 で作る。試験では build_profile_v5 をフィールド画像に掛けた。σ=3 ではフレームのプロファイルを使うと悪くなった (Y 静止 1.03 → 1.24)。
4. qtui の read_frames は `-pix_fmt yuv420p` で変換している。4:2:2・4:4:4 のインターレース素材では、swscale がフィールドを混ぜて色差を作る (INT-1 の verifier の観測どおり)。INT-3 では `scale=interl=1` などが要る。fields_cli.py は yuv420p 以外を interl=1 で変換している。
5. 順序の判定に使うのは最初のフレームだけで、PAFF や途中で順序が変わる素材は対象外 (INT-1 の I-9 と同じ)。
6. SHIFT の flow (nvc_temporal_shift の flow_in / flow_out) は、ずれを除いた残りの動きの意味になる。shift なしの flow と混ぜてキャッシュしてはいけない (ヘッダに書いた)。
7. 既存の問題 (どれも今回は直していない)
   - `make -C native verify JOBS=N` は verify_fields.py が --jobs を受け付けないので止まる (INT-1 から)。今回は JOBS なしで回した。
   - qtui/tools/compare_export.py は今も ROOT/repro を参照している (写しで実行した)。
   - verify_ofx.py は qtui/build を前提にしている。
8. verify_fieldproc.py の一時ディレクトリの既定は work/omo/wp_interlace/INT-2/tmp (ignore されている)。--tmp で変えられる。

## 試験用 CLI
- `python3 native/fields_cli.py IN OUT.mkv [--order auto|tff|bff] [--mode same|shift|plain|frame] [--radius R] [--start S --frames N]`
  - --order auto は qtui と同じ規則で順序を決める。決められなければ止まる (prores の +ildct なしと progressive で、止まることを確かめた)。
  - 出力は setfield 付きの FFV1 (x264_bb の入力から作った出力は、frame の top_field_first=0)。
  - 音声・色タグ・高ビット深度は扱わない (INT-3 の範囲)。

## 守ったこと
- 本家バンドルは実行も dlopen もリンクもしていない。
- Resolve、/Library/OFX/Plugins、ユーザーの MeatVideoQt、reference/python/out には触れていない。
- PySide6 版、AGENTS.md、MIGRATION.md は変更していない。
- コミットはしていない。

