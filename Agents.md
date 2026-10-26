# Agents.md — Neat Video v6 リバース＋デノイズ再現プロジェクト

作業ディレクトリ: `/Users/seitan/Desktop/work/reverse`
言語: 日本語で応答する。技術的正確性を優先し、推測と観測事実を区別する。

## 1. 概要

Neat Video v6 (OFX Demo) のバイナリを入手・解析し、そのデノイズ処理を
概念再現した Python パイプラインと調整 UI を構築した。
厳密な同一実装ではなく、本家の設計 (処理順・バンド構成・既定値) に準拠した再現である。
NeatLab は特許を公開しておらず詳細は営業秘密のため、内部定数の完全抽出は対象外。

## 2. 成果物一覧

### 入手バイナリ (workspace 直下)
- `NeatVideo6OFX.Demo.UB.dmg` (28MB, SHA256 `617a79e4…bedc4`) — OFX 用 Demo
- `NeatVideo6PR.Demo.UB.dmg` (27MB, SHA256 `993600b4…99600`) — Premiere 用 Demo
- Pro 版は公式サイトでログイン＋ライセンス必須のため未取得。必要なら要連携。
- `test.mp4` (640x360・901f・77MB) — テスト動画その1
- `test_v02.mp4` (1920x1080・721f・137MB) — テスト動画その2
- `test_v02_reduce.mov` (1080p・721f・101MB) — **本家 Neat Video 処理済みの正解データ**

### 展開・解析 (`neatvideo_ofx_614/`, 71MB)
- `NeatVideo6.ofx.bundle/` — OFX バンドル一式。主バイナリは `Contents/MacOS/NeatVideo6.ofx`
  (45MB, Mach-O Universal x86_64+arm64, SHA256 `6f0e9316…134e1`)
- `NeatVideo6.arm64.ofx` (22MB, SHA256 `95bef55f…8430`) — thin 化した arm64 スライス (解析用)
- `Distribution.xml`, `PackageInfo.xml`, `Info.plist`, `SHA256SUMS.txt`
- `analysis/` (4.6MB) — Ghidra デコンパイル成果物。`INDEX*.txt` が入口。
  `functions.csv` (39,734 関数), `rend_*.c` (レンダー経路), `prof_*.c` (設定・登録),
  `strfunc_*.c` (Demo 制限・GPU), `fixed_*.c` (OFX エントリ)
- `analysis/harness/` — 本家バンドルを dlopen して内部関数を直接呼ぶ照合用ハーネス (`nvh.c`・`Makefile`、使い方 §5、結果 §6.5)。
  `nvh_guard.h` は本家のアサーション (システムのダイアログ) を、初期化子より前に「標準エラー出力＋終了」へ差し替える安全装置
- `analysis/noise/` — ノイズ推定の照合スクリプト (共通部 `nvh_common.py`)
  - 帯域測度・FindFlatArea: `verify_levels.py` (帯域測度と分位点をビット単位で)・`verify_flat_exact.py` (FindFlatArea を中間値まで)・
    `verify_flat.py` (本番の入口)・`verify_cancel.py` (状態 5 = 取り消し)・`genuine_flat.py`・`parse_profile.py`
  - EstimateNoiseLevelCurve: `verify_curve.py` (曲線を端から端まで)・`verify_curve_shell.py` (タイル・IsClip・補間器)・
    `verify_pixnoise.py` (画素ノイズ)・`verify_lines.py` (線フィルタ)・`verify_curve_speed.py` (高速経路と参照経路の一致。本家不要)
  - 証拠の保存: `frozen_20261016/` (高速化前・軽微な修正前の曲線移植の凍結コピー。`verify_curve_speed.py --orig` が使い、`--mem` は `--mem-dirs` で指定したときに比べる)・
    `cpu_oracle/` (fmla・frecpe・frecps などの命令の C 版オラクル。本家は読み込まない)・
    `lzma_oracle/` (LZMA SDK 9.20 / 16.04 の C ソース (パブリックドメイン、取得元と SHA256 は README.md) と `LzmaCompress` のオラクル `lzo.c`、照合 `verify_lzma.py`、以前の感度試験で差が出なかった変異 2 種を検出する入力 `sensitivity_inputs/`。本家は読み込まない)
  - ノイズ推定まわりの逆コンパイルは `analysis/rtti/noise/` と `analysis/rtti/enlc/`
  - `aft_port/` — AutoFineTune の静的移植 (参考実装。本家と照合したのはモード 2 のタイル順だけ。§6.5)
- `analysis/blackbox/` — 本家出力との比較・較正 (§6.2〜6.4) と実験記録 `exp_results.txt`。`logs_20261016/` は 2026-10-16 の照合ログ
  (`INDEX.txt` が入口。EstimateNoiseLevelCurve 移植は `curve_p4/`・`fix5/`・`speed/`・`lowfix/`、FindFlatArea 移植は `ffa/`、統合後の回帰は `main_final/`)

### 作業用の隔離フォルダ (`sandbox/`)
- `sandbox/dirfilt/` — BuildNoiseModel の移植を別エージェントが行った作業場所 (指示書 `INSTRUCTIONS.md`)。2026-10-16 に `repro/` へマージ済み
  (マージ元 `sandbox/dirfilt/merge_stage/`)。報告・ログ・指示書の写しは `analysis/blackbox/logs_20261016/model/`、照合スクリプトは `analysis/noise/model_verify/`
- `sandbox/omo/` — 2026-10-17〜18 の作業パッケージ (`wp_*`: 各作業者の REPORT・notes・src) と独立検証 (`coord/verify_*/VERDICT.md`)、文書・コードの版の控え (`coord/backup/`)。
  マージしたのは speed (`repro/`) と aft の修正版 (`analysis/noise/aft_port/`) のコード、統括が審査した nvh のサブコマンド (ramp・wmul・pairw・down2・sad・b32・aftshuffle)、検証で確定した事実 (本書と `exp_results.txt` の対応する節) だけ。
  作業者の文書に残る誤りと正しい記述は、各 wp の `CORRECTIONS.md` にまとめた (2026-10-20。REPORT・MERGE_PLAN・notes・harness_request・FIX_REQUEST の先頭にその旨の注記。INSTRUCTIONS には注記なし)

### 引き継ぎ報告
- `REPORT_CLAUDE_20261021.md` — 2026-10-17〜21 の経緯、保護区画の観測と扱いの理由、作業途中のもの、引き継ぎ時の注意 (既存の成果物に手を加えるときは変更前を残す)
- `sandbox/omo/wp_spatial/notes/lead_static_20261021.md` — 2026-10-21〜22 に統括が保護区画まわりを読んだ記録 (ユーザーが統括に解析を一任)。
  先頭に検証結果を受けての訂正注記がある。確定した機構は本書 §6.4・§7 へ反映済み。検証は `coord/verify_protect/VERDICT.md` (A〜J の判定と
  「マージしてよい/書いてはいけない」の一覧)

### 共有データ (`data/`、workspace 直下)
- `data/ref_floor.npy` — `nv_profile.reference_floor` (白色ノイズの基準値 51 語) のキャッシュ。参照実装で計算した値とビット一致 (2026-10-17)

### 再現実装 (`repro/`)
- `nvfilter.py` — 本体。上半分が v2〜v4 関数 (旧・validate_v2/v4 用に不変)、その後が **v5**
  (ネイティブ YCbCr・輝度別ノイズ曲線・画素単位の時間重み＋残留ノイズ追跡・5 帯域 a-trous)、
  最後に本家のノイズ測定の移植 (`band_model`・`is_clip`・`find_flat_area_genuine` など、§6.5。中身は下の nv_* モジュール)
  2026-10-26: `build_profile_v5(area=...)` が (y0, x0, size) に加えて長方形 (y0, x0, h, w) を受け付ける (各辺を独立に [8, H]・[8, W] に切り詰め、
  h == w なら従来の 3 要素の area にまとめる。`prof["area"]` は正方形 3 要素・長方形 4 要素)。None と 3 要素は変更前とビット一致 (独立検証 186/186、
  `sandbox/omo/coord/genuine_ui/VERDICT.md`)。`nv_aft_v5.apply_aft` も 4 要素の area に対応 (正方形はビット一致)。CLI は `--area Y,X,SIZE` か `Y,X,H,W`
- `nv_curve.py`・`nv_pixnoise.py`・`nv_lines.py` — 本家 EstimateNoiseLevelCurve の移植 (本家とビット一致、§6.5)。
  `nv_curve.genuine_noise_curve(rgb)` が (輝度, ノイズ) の 8 点表を、`nv_curve.noise_at(表)` が本家の補間器を返す。
  `nv_curve.is_clip_genuine` は本家の IsClip。`nvfilter.find_flat_area` の正規化曲線とクリップ判定はこれを使う
- `nv_band.py`・`nv_flat.py` — 本家の帯域測度 (`FUN_009980e8`、`FUN_00995250` の水準部、分位点 `FUN_00997bcc`) と
  FindFlatArea (Auto Profile 経路) の移植 (本家とビット一致、§6.5)。`nvfilter` の `band_model`・`find_flat_area_genuine` などはこれに委譲する
- `nv_model.py`・`nv_model_special.py`・`nv_model_bm.py` — 本家 `FUN_00995250` (BuildNoiseModel の 1 面モデル) の全 4096 語の移植
  (本家とビット一致、§6.5)。`nv_model.model_full(plane, set_=-1, levels=5)`。語 0..54 は `nv_band.model_levels` をそのまま使う
- `nv_profile.py` — BuildNoiseModel (`FUN_009af718`) が書くプロファイル 0xc300 バイト (`build_profile(rgb, mode, area)`。ヒープのポインタ
  以外は本家とビット一致)。見本画像の LZMA 圧縮 (0x18〜0x2b) は `nv_lzma.py` (LZMA SDK 9.20 の符号器の移植) を使う。部品は `nv_mcstats.py` (`FUN_0098718c`)・`nv_refnoise.py` (基準モデルの白色ノイズ `FUN_009c3bc4`)・
  `nv_cov.py` (照合用の分岐カウンタ)。照合用で、ノイズ除去のパイプライン (v5) は使っていない (2026-10-17 の高速化後、負荷平均 5〜8 の中で 1 面 128² 2.1 秒・320² 6.6〜6.8 秒、プロファイル 128² 15.4〜15.7 秒 (高速化前は同条件で 2.7 / 9.2〜9.5 / 20.5〜21.0 秒))。白色ノイズの基準値 (`reference_floor`、51 語) は workspace 直下の `data/ref_floor.npy` に置いてあり (参照実装で計算した値とビット一致)、無ければ計算する (約 7 秒)
- `denoise_video_v5.py` — **実写用 v5 (推奨)**。ネイティブ yuv420p 入出力 (色タグ継承)・ストリーミング・フレーム並列。
  2026-10-26 に高ビット深度 (`--depth 10|12|16|32|source`、既定 8 = 従来と同じ yuv420p・libx264 で、控えとの出力のバイト一致を確認) と `--codec` を追加 (仕様・報告・検証は `sandbox/omo/coord/highbits/` の SPEC.md・REPORT.md・VERDICT.md、検証 Round 2 で APPROVE)
- `nv_depth.py` — 画素形式の表と変換 (2026-10-26)。高ビット深度の入力は読み込み時に処理コアの「8 ビット相当 0〜255 の float32」へ正規化し (整数は v/2^(b−8) で厳密、float の RGB は ×255 して `_KRB` の係数で full range の YCbCr 4:4:4 へ)、書き出しで戻す。
  `nvfilter.py` は無変更 (`CURVE_BINS` などは正規化後の尺度でそのまま働く)。色差はソースの形式のまま (4:2:0・4:4:4。4:2:2 は偶数列 = 元の標本・奇数列 = 左右の平均で 4:4:4 に広げて処理し、偶数列を戻す)。
  float の NaN/±inf は処理用に有限の値へ置き換え (NaN→0、+inf→面の有限の最大、−inf→最小)、書き出しで元の値を戻す
- `nv_ui.py` — 調整 UI (tkinter, v5)。`python3 nv_ui.py` で起動。全編出力は `denoise_video_v5.py` を呼ぶ
- `nv_ui_qt/` — 調整 UI の PySide6 版 (2026-10-25。仕様 `ui_spec/UI_SPEC.md`・タスク `.claude/tasks/ui_qt_migration.md`)。2026-10-25 に配置を本家に寄せた (根拠は公式ユーザーガイド v6.1 の画像と、バイナリの UI 構築順の静的解析。`sandbox/omo/coord/verify_qtui_layout/` の LAYOUT_WEB.md・LAYOUT.md・LAYOUT_PLAN.md): ページ Prepare Noise Profile / Adjust and Preview、メニュー Effect / Edit / View / Profile / Filter / Tools / Help (本家の Task・Variants は省略) と Beginner / Advanced の切り替え、ビューアは既定 YCrCb+ (合成 + Y/Cr/Cb Enhanced。Enhanced と周波数表示は repro の定義)・「Filtered」表示と押している間だけ元画像・範囲を選ぶとその範囲だけ処理、最下段にサムネイル (青 = 現在、オレンジ = 時間窓) と Cancel / Apply (Apply = 全編の書き出し)、Filter Settings は Temporal / Spatial / General タブで成分は Luminance → Cr → Cb、帯域は High → Ultra Low。Frequencies 行は本家同様 3 成分共通 (成分別は「repro」欄)、Mix with Original は Enable 付きで本家の向き (表示 = 100 − 内部 mix)、プロファイルの Noise Level (Overall・成分別) は曲線の倍率 `tune`。Undo/Redo、JSON のプロファイル・プリセット (本家の .dnp / .nfp ではない)。
  バックエンドは変更せず `nvfilter.py` の v5 と `nv_aft_v5.py` (AFT。2026-10-25 に別途追加されたもの。RGB 各面の係数を Y/Cb/Cr の曲線に掛けている点は未確認) を使う。既定値は `V5_DEFAULTS` (UI_SPEC の Luminance 帯域の既定値ではない)。
  本家の Luminance / Cb / Cr 別の帯域スライダーは `nv_ui_qt/backend.py` が `spatial_v5` を成分ごとに呼んで実現し、全成分が同じ設定なら `process_frame_v5` とビット一致。4:2:0 では色差が 4 段なので Cb/Cr の High は無効表示。
  v5 に対応のない Sharpening・Jitter・Dust and Scratches・Flicker・Edge Smoothing・品質モード・Input Data Gain・Overall Profile Quality は無効表示 (n/a)。Overall Profile Quality の式は、輝度曲線の項 g を除いて静的に読めた (`sandbox/omo/coord/verify_quality/static/QUALITY.md`、独立検証 APPROVE。g は入口が難読化された関数なので読んでおらず、本家の 23% は未再現のため UI には入れていない)。Most Noisy Frame・Auto Complete・Assist・ガイド文は再現側の独自。
  全編出力は `nv_ui_qt/export.py` が `denoise_video_v5.main` を UI のプロファイル (AFT・手動調整込み) と成分別設定で動かす (CRF 0 で `backend.process_frame` とビット一致)。自己試験 `nv_ui_qt/selftest.py` 77 項目 (2026-10-26 に高ビット深度の 6 項目、本家合わせの 9 項目、Profile Check とパネルの 6 項目、Noise Level Check の 7 項目を追加)。Profile Check (2026-10-26、本家の観測と仕様・検証は `sandbox/omo/coord/genuine_ui2/` の SPEC.md・VERDICT.md で APPROVE): 本家同様ダイアログではなくビューアの Assist Image 表示に切り替える (パネルの Begin/Finish・ツールバー・メニューが 1 つの切り替え)。Assist Image は repro の定義 (現在のフレームに空間段だけを Luminance/Cr/Cb・全帯域 100% で掛けたもの。本家の処理は未解読) で、下段の Enhanced は強調なし。従来のダイアログは Profile Viewer… (⇧⌘I) になり、Repro 欄 (Profile mode など) もそこへ移した。Noise Level Check (2026-10-26、本家の観測・仕様・検証は `sandbox/omo/coord/verify_quality/` の REPORT.md・nlc/SPEC.md・nlc_verify/VERDICT.md で APPROVE): 本家同様、時間フィルタのマスクを Assist Image として出す (パネルの Begin/Finish・ツールバーの Filter Settings Check のメニュー・Tools のメニューが 1 つの切り替え)。色は本家の画面に当てはめた式 (I = (R+G+B)/3、(1 − m)·I·(1, 0.9, 0.75) + m·(I·(0.3, 0.45, 0.5) + (0, 0, 126.5))、残差 0.35〜1.3 階調)。マスク m は repro の定義 (時間段の Y の残留分散 rv から m = clamp((1/rv − 1)/(1/rv_min − 1), 0, 1)。本家の式は未解読で、test_v02 の −60% では本家 0.33 に対し 0.52)。Temporal の Noise Level (`t_noise`) を実装した (`nv_ui_qt/backend.py` が時間段だけ noise_level を ×t_noise。既定 +0% は変更前とビット一致)。`nv_aft_v5.assist_params` (2026-10-26 に別エージェントが処理コアに追加) は UI から使っていない。Device Noise Profile パネルは本家の順でスクロールなし、選択範囲の Noise Level は Main Sample 比の (±n%) 付き。
  本家合わせ (2026-10-26、DaVinci Resolve 上の本家 6.1.3 Pro の観測 `sandbox/omo/coord/genuine_ui/OBSERVED.md`・`shots/` と、バイナリの静的解析 `STATIC.md`。仕様 `SPEC.md`、独立検証 `VERDICT.md` で APPROVE):
  ビューアの表示は本家の式 (`nv_ui_qt/display.py`: 色変換 Y = 0.2989R+0.5866G+0.1145B・Cr = (R−Y)·0.71316504・Cb = (B−Y)·0.56465274、ペインごとの色付きの出力範囲、
  明るさ 0〜100 (50 で恒等)、YCrCb+ の Enhanced と Frequencies は BuildFreqImage (5 段の間引きラプラシアン、Prepare だけ強調 b/((Box3Box3 b²)^¼+0.01)・重み 1.25^k) と
  FindFreqLevs の表示範囲。本家の画面の YCrCb ペインとの平均絶対誤差は Y 0.29 / Cr 0.43 / Cb 0.33。ディザは省略)。表示モードはページごと (Prepare 既定 YCrCb+、Adjust 既定 Normal)。
  選択範囲は自由な長方形 (起点が角、各辺 ≤ 512、内側のドラッグで移動、クリックで消去、枠の色は min(w,h) で赤 ≤15 / 黄 ≤127 / 緑、上に本家の式の "Noise Level"
  = √(Σ_段 Σ_RGB 局所 RMS 平均²)、下に "Not Uniform" (平坦度 ≥ 2.0)・"Clipping"、▽ で Build Profile (Most Noisy / Current Frame))。選択の有無で Auto / Build Profile、
  32 未満は作成不可、32〜127 は確認。メニューは Effect / View / Profile / Filter / Variants / Tools / Help (Edit なし)、ツールバーと右パネルの並びも本家どおり。
  本家と違う点: イコライザは Y/Cr/Cb (本家は R/G/B)、Quality と帯域のバーの値は n/a か repro の定義、View・Filter・Tools の中身は未観測、Fit ズームは連続、フレームのスライダーは残す。
  Resolve のタイムラインが 3840×2160 だったので、本家の数値 (Noise Level など) は test_v02 を 2 倍に拡大した入力でのもの。高ビット深度 (2026-10-26): Preferences の Bit depth (既定 8-bit = 従来どおり) で読み込み・Batch Profiler・書き出しの深度を決め、高ビットのフレームは float32 で持つ (メモリ上限は実際のバイト数で計算)。ステータスバーに元の pix_fmt と処理の深度を出す。プロファイルの色差の間引き (`sub`) が書き出しの形式と違えば書き出しを拒否。OFX ブリッジは `--ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]` (16 = 4:2:0 の uint16 LE、32 = 4:2:0 の float32 LE で 8 ビット符号値/255。プロトコルは `ofx_bridge.py` の docstring)。OFX モードの既存の不具合 (`argv` 未定義、存在しないメソッドの呼び出しなど 6 件) は往復試験に必要な範囲で修正。独立検証で APPROVE (2026-10-25、D1〜D10 の指摘を修正済み。配置変更後は `coord/verify_qtui_layout/VERDICT.md` で L1〜L10・N1・N2 を修正して APPROVE。旧版の D6「ドラッグは Undo 1 件」は PySide6 6.12 の Enum 比較のため実際には成り立っておらず、L1 で修正)
- `denoise_video.py` — v1 実写用 (RGB・旧式。記録用)
- `denoise_video_v2.py` — 実写用 v2〜v4 (記録用。RGB 経由のため色ずれあり §6.2)
- `neat_repro.py` — v1 合成検証用。`validate_v2.py` / `validate_v4.py` / `validate_v5.py` — 各版の合成検証
- `out/` — 出力動画・比較画像・`results.json`

### 出力動画 (`repro/out/`, 代表のみ)
- `test_v02_denoised_v5.mp4` (+`.json`) — test_v02 の v5 処理結果 (**推奨**、既定値・auto プロファイル・CRF16)
- `test_v02_denoised_v5_crf12.mp4` (+`.json`) — 同じ処理を CRF12 で符号化 (本家比較用。55MB、本家は 106MB)
- `compare_v5_f312.png` — f312 拡大: 元 / v4c / v5 / 本家 の4分割
- `compare_v02_v5_3way.mp4` — 左から 元 / v5 / 本家 (中央 960x540 切り出し、全721f。ffmpeg に drawtext が無いためラベルなし。
  本家ファイルは色タグなしなので `setparams=colorspace=bt709:range=tv` で揃えて結合。検証は yuv420p 生データで行う (gray 変換は不正確))
- `test_denoised_v4.mp4` — test.mp4 の v4 処理結果
- `compare_3levels.mp4` — Original / Mild 0.4 / Standard 0.6 / Strong 0.9 の4分割
- `compare_v02_3way.mp4` — test_v02 の Original / 当再現(v4) / 本家 の3分割
- `test_v02_denoised_v4c.mp4` — test_v02 の v4 本家寄せ版 (`--sigma-scale 4`、色ずれあり)
- `thumb_*.png`, `compare_*.png` — 目視確認用

## 3. バイナリ解析の要点 (観測事実)

- 署名あり (`com.absoft.NeatVideo6-OFX`, NeatLab, hardened runtime, 2026-10-05)
- OFX export は3件のみ: `OfxGetPlugin` / `OfxGetNumberOfPlugins` (=1) / `FoundryOfxAllowPluginUnload`
- パイプライン構成要素 (RTTI・import・decompile で確認):
  `DeviceNoiseProfile`, `MotionCompensator::CompensateImageMotion` (MacroBlock＋SAD),
  `ImageWithLevels` (多層周波数分解), `NVFilterInstance(OFX)`, GPU タスク, Qt 5.15.2 カスタム
- 5 周波数バンド: ultra-low / very-low / low / medium / high × チャンネル (UI 文字列＋設定構築関数 `FUN_000b0934` で確定)
- `.nfp` 数値部の同定: バイト74 float = Noise Reduction Amount (0.2/0.8)、
  バイト222 = Temporal Radius (2/5)、バイト278 = フラグ、既定1.0 のバンドゲイン配列
- Demo 制限: 15分・1800x1000・透かしラベル＋GPU 1基 (ワンショット表示フラグ `DAT_01538ab0`)
- レンダー経路: `FUN_0000ae2c` (dispatch) → `FUN_000080a8` (fetch 時間窓→CPU/GPU 分岐→合成)
- 逆アセンブル全文: `/tmp/nv_expand/disasm_full.txt` (328万行・一時ファイル。必要なら `otool -tv` で再生成)
- 解析基盤: Ghidra 12.1.4 (`~/tools/ghidra_12.1.4_PUBLIC`)＋自前ビルド decompiler
  (`os/mac_arm_64/decompile`)、REA CLI (`npx rea-agents@6.4.0`)、Ghidra プロジェクト (`/tmp/ghproj`)

## 4. 再現パイプラインと本家の対応 (v5, 2026-10-15)

| 本家 (観測 §6.1 / 文献) | 再現 v5 (`nvfilter.py` 末尾) | 旧 v4 (本家と遠かった点) |
|---|---|---|
| FindFlatArea (Auto Profile 経路 §6.5): 縮小輝度で候補 (最大 64)、サイズ表 (1080p: 320〜48px・係数 1〜6) ごとに「粗い段ほど重い帯域エネルギー ÷ 輝度別ノイズ」に白色度・平坦度・クリップ・輝度の罰則を掛けたスコアの最小 | `find_flat_area` = 本家移植 (`nv_flat`・`nv_band`・`nv_curve`。帯域測度・候補・スコアまで本家とビット一致し、照合した全フレームで領域・状態コードが同一) | 32x32 の分散最小 (飽和部を拾い得る) |
| EstimateNoiseLevelCurve: 96px タイルごとに 16 方向×2 段の線フィルタで画素ノイズを測り、輝度8ビンに集計。W*H/1024 未満のビンは多項式補間、[1,2,1]/4 (FindFlatArea の正規化に使用) | FindFlatArea の正規化には本家とビット一致の移植 `nv_curve.genuine_noise_curve` (1080p 0.67 秒)。ノイズ除去の輝度別 σ は従来どおり `noise_curve_spatial` (16px ブロック・下位25%値を3フレーム中央値。本家曲線との比は test_v02 で 1.04〜1.20、test.mp4 で 0.73〜1.01。§6.5) | なし (輝度によらず単一σ) |
| BuildNoiseModel: 領域の RGB＋YCrCb の 6 面 × 5 段のラプラシアン帯域の局所 RMS (平均・99%・中央値・変動係数)＋方向フィルタ統計。輝度別係数は AutoFineTune (32px タイル、RGB 各面) (§6.5) | auto: 帯域比は静止画素の時間差分から (本家の領域モデルと段比 6% 以内で一致) / spatial: 本家と同じ領域のサブブロック | Haar HH1 の MAD のみ → 有色ノイズを 3〜10 倍過小評価 |
| 5バンド空間フィルタ (局所適応。間引きラプラシアン 5 段＋縮小核 `FUN_00990f74`、§6.1) | 非間引き a-trous 5帯域＋局所 Wiener (7x7)、Amount は1回だけ適用 (本家寄せの移植 `wp_spatial` は区間 A の Y が v5 未満で未採用、`exp_results.txt` の spatial 節) | 間引き Haar＋全域 BayesShrink、luma/chroma を2重適用 |
| Temporal→Spatial 固定順・Radius 2 | 同 | 同 |
| 動き補償 (MacroBlock＋SAD。静的な解読は §6.1 の decode_motion.md、本家と照合したのは SAD と 32×32 探索のサブ画素 (SAD 重心) の葉だけ) | 3段ピラミッド BM＋SSD 放物線サブ画素＋メディアン＋双線形の密フロー (葉の移植でフローを置き換えると区間 A の Y が下がるので不採用、§6.1) | half-pel 双線形 (雑音で半画素が選ばれやすい)、ブロック一定 |
| 時間重み: 類似度マップ・中心1、残留ノイズ Σf·w²/(Σw)² (f=0.64) | 同構造。類似度=5x5 二乗差/(2σ²)、`threshold` 4。残留ノイズを空間段へ渡す | 近傍重み ≈0.12 固定 (σ/20 由来)＋ブロック信頼度 |
| Noise Level / Cb・Cr Noise Level | `noise_level` / `channel_noise` (既定 1.0) | `--sigma-scale` (4 が必須だった) |
| Noise Reduction Amount (文献: Luma 40%/Chroma 100%/UltraLow 0%) | luma 0.7 / chroma 1.0 / ultra-low 0 (luma は本家出力に較正 §6.3) | 0.4 / 1.0 |
| Flicker (本家出力に全体補正なし) | 既定オフ (`--flicker R` で移動平均ゲイン) | 120f 区間平均ゲインが既定オン (継ぎ目で不連続) |
| 色処理 (YCbCr) | ネイティブ YUV、入出力両側に色タグ | rgb24 経由 BT.601・タグなし (Y −1.7 のずれ) |

### 合成ベンチ (PSNR dB, σ15 ノイズ, `validate_*.py`)
noisy 24.54 → v1 33.23 → v2 36.07 → v3 37.3 → v4 36.13 → **v5 38.70** (フリッカ補正込み 39.06)
※ v5 の合成クリップ (176x144) は FindFlatArea の最小 64px 未満のため、既知の 32x32 平坦部を手動領域として渡す。

### 本家比較 (test_v02、ネイティブ YUV の PSNR dB、`blackbox/exp_results.txt`)
- 区間 A (n=296〜327 を読み、出力 n=304〜319 の16f を評価、較正用): 入力 37.86 → v4c 設定 41.79 → **v5 47.12** (U 50.98 / V 51.18)
- 区間 B (n=560〜591 を読み、出力 n=568〜583、較正に未使用): 入力 37.94 → **v5 47.22** (U 50.92 / V 51.19)
- 全編 (30f 抽出・x264 符号化後): 入力 38.35 / v4c **38.44** (U 43.14, 平均 Y −1.73) / **v5 45.63** (U 49.72 / V 50.04, 平均 Y +0.12)
  / v5 を CRF12 で符号化 **46.70** (U 50.20 / V 50.41)。全編と区間の差の約 1.1 dB は当方の符号化損失 (§6.4 の測定下限も参照)
- 時間方向の残留 (フレーム差 RMS): 本家 1.07 / 1.01 に対し v5 1.02 / 0.91 (区間 A / B)
- 旧指標 (RGB・5f): v4 34.22 → v4c 35.07。高周波エネルギー 元 8.76 → v4c 5.43 → 本家 5.06 (別指標なので v5 とは直接比較しない)

## 5. 使い方

- 実写一括 (推奨 v5): `cd repro && python3 denoise_video_v5.py <in> <out> [--profile auto|spatial|temporal] [--area Y,X,SIZE|Y,X,H,W] [--radius 2] [--temporal 1.0] [--threshold 4] [--luma 0.7] [--chroma 1.0] [--spatial 2.0] [--noise-level 1.0] [--chroma-noise 1.0] [--start S --frames M] [--jobs 10] [--crf 16] [--flicker R] [--params P.json] [--audio copy|aac|none]`
  - 音声はトラックごとに判定: 出力コンテナの再生互換リスト (mp4: aac/alac/mp3/ac3/eac3/pcm、mov: +mp2 等) に入り試し書きが通ればコピー、
    だめなら AAC、それも無理なら警告して除外。部分範囲は同じ時刻範囲を切り出す。A/V のずれは最初にデコードされるフレームを基準に保ち、
    MPEG-TS/PS で ffmpeg が音声入力の基準を付け直す分も補正 (検証は `blackbox/av_sync/sync2.py`・`sync3.py`、結果は `exp_results.txt`)
  - 読み込みは最初のフレームを 0 とした r_frame_rate の等間隔格子 (`setpts=PTS-STARTPTS,fps=R`)。CFR 素材は passthrough とビット一致
  - 1080p・721f で 278 秒 (M2 Max・jobs 10)。出力に `<out>.json` (プロファイル・パラメータ) を併記
  - 高ビット深度: `--depth 8|10|12|16|32|source` (既定 8) と `--codec auto|x264|x265|ffv1|prores|exr`。auto は 10 ビット → libx264 (High 10 / 4:2:2 / 4:4:4)、
    12 ビット → libx265 (.mp4/.mov)、.mkv → FFV1 (可逆)、16 ビット → FFV1 (.mkv のみ)、32 ビット → gbrpf32le の FFV1 (.mkv) か EXR 連番 (`out_%05d.exr`、音声なし)。
    出力の pix_fmt は処理形式と同じで ffmpeg に変換させない。書けない組み合わせはプロファイルの前に 64×64 の試し書きで止め、途中で符号器が失敗したら書きかけを消す
- UI: `cd repro && python3 nv_ui.py` (Load→Auto Profile (auto/spatial/temporal)→タブで調整→Preview→Process full video)
- Qt UI: `cd repro && python3 -m nv_ui_qt.main [video]` (要 PySide6。2026-10-25 に 6.12.0 を導入)。自己試験 (画面を出さない): `QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest ../test.mp4 <出力先>` (出力先に設定・試験ファイル・短い可逆の書き出しだけを書く)。
  プレビューは全フレームをメモリに読む (上限は Preferences、既定 4 GB。1080p 721f で約 2.2 GB)。1080p 1 フレームのプレビューは約 2.4 秒、Preview Area で約 0.6 秒、AFT は約 48 秒 (検証者の測定)
- 検証: `python3 validate_v5.py` (合成)、`neatvideo_ofx_614/analysis/blackbox/calibrate_v5.py` (本家区間比較・較正)、
  `compare_full.py` (全編比較)。区間 yuv の抽出手順は calibrate_v5.py の docstring 参照。
  calibrate_v5.py の候補には `"area": [y0, x0, size]` (区間別は `{"296": [...], "560": [...]}`) でプロファイル領域を指定できる
- 本家との照合 (解析用): `make -C neatvideo_ofx_614/analysis/harness check` で `nvh` をビルドし、自己テストする。
  - サブコマンド: `nvh levels|model W H plane.f32 [set]` (帯域測度。各行「番号 %.9g 16進ビット」。`set` は model の方向フィルタの組)、`nvh curve|clip W H rgb.f32` (`curve` は %.17g)・`nvh flat W H rgb.f32 [mode] [sel]`、
    `nvh profile W H rgb.f32 mode out.bin [x y w h]` (本家の LZMA 圧縮データ 3 つも out.bin.lzma0/1/2 に書く。読み取りのみ)、読み取り専用の `nvh info` (定数・AutoFineTune の入口判定)・`nvh order`
  - EstimateNoiseLevelCurve の部品: `nvh tiles W H [tile margin]` (タイル配置)・`nvh kernel W H angle` (線カーネル表)・
    `nvh linefilt W H plane.f32 angle r mode [angle2 r2]` (線フィルタ)・`nvh pixnoise W H luma.f32 out` (画素ノイズ。1 辺 512px 以下)・
    `nvh refmodel out.bin` (白色ノイズの基準モデル)・`nvh interp n x.. y.. q..` (曲線の補間器)
  - FindFlatArea の部品: `nvh reduce W H rgb.f32 s out` (縮小と輝度)・`nvh cand W H luma.f32 rx ry step [nb per_bin]` (候補)・
    `nvh xform W H rgb.f32 mode out` (色変換)・`nvh flatness W H plane.f32` (帯域測度→平坦度)・`nvh quantile n v.f32 qbits flag div` (分位点。qbits は q の float32 ビットを 16 進で)
  - 曲線の照合: `cd neatvideo_ofx_614/analysis/noise && python3 verify_curve.py` (`--reference` で参照経路も、`--diagnose` でタイル単位、
    `--extra`・`--fuzz N` で追加の入力、`--sensitivity` で照合が空振りしないことの確認。加算順の集計は既定で行い `--no-order` で省く)。
    `verify_curve_speed.py --orig` は既定で `frozen_20261016/` と比べ、`--mem` でメモリを測る (`--mem-dirs frozen_20261016/pre_lowfix` で修正前と比較)
  - BuildNoiseModel の照合 (`analysis/noise/model_verify/`): `python3 verify_model_cases.py --sets --jobs 6` (1 面モデルの 4096 語、753 件。
    `--mutate NAME` で感度試験、`--every N` で間引き)、`python3 verify_profile.py [--quick]` (プロファイル。領域を指定して `nvh profile` を呼ぶので
    FindFlatArea は呼ばれない)、`independent/run_model.py`・`run_profile.py` (独立検証者の入力)、`bench.py`。本家の結果は `model_verify/tmp/` にキャッシュする
  - LZMA (プロファイル 0x18〜0x2b) の照合: `make -C neatvideo_ofx_614/analysis/noise/lzma_oracle` の後 `python3 verify_lzma.py [--oracle] [--genuine] [--fuzz N] [--quick]`
    (移植と C 9.20・16.04 の比較と行の網羅率、本家の圧縮データとのバイト単位の比較)
  - FindFlatArea の照合: `python3 verify_flat_exact.py --clip ../../../test_v02.mp4:24 --clip ../../../test.mp4:30 --modes 0,1,2 --inter`
    (`--production` で本番の入口も、`--scales`・`--small`・`--synthetic` で追加の入力)。帯域測度は `python3 verify_levels.py [--quick]`。
    本家の FindFlatArea は 1 プロセスで 1 回だけ呼ぶ (§6.5)。48px 未満のフレームは本家が落ちるので既定では渡さない (`--probe-crash` で 1 回だけ)
  - 状態 5 (取り消し) の照合: `python3 verify_cancel.py [--all-k]`。`nvh flat` は環境変数 `NVH_CANCEL_AT=k` で、ハーネス側の取り消しの
    問い合わせ (フィルタ核の vtable 0 番) が k 回目以降に「取り消し」と答える (ホスト側のコールバックで、本家には手を加えない)。問い合わせ回数は標準エラー出力に出る
  - 時間フィルタ・動き探索の葉 (2026-10-17 追加。いずれも外の関数を呼ばず、大域は高々 NEON 判定のバイト 0x14a67a8 (`curve` で既に動く `0x9ee04c` なども読むバイト) と __stack_chk_guard であることを逆アセンブルで確認。ramp・wmul・pairw・down2 は 0x14a67a8 だけでスタックガードも使わない (独立検証者が確認)):
    `nvh ramp t in.f32 n out` (`FUN_009ee93c`、clamp(2 − x/t, 0, 1))・`nvh wmul t r.f32 prev.f32 n out` (`FUN_009eebf8`、ramp(r)·prev)・`nvh pairw a b c n out` (`FUN_009efb1c`、c·(max(a,b) − b))・
    `nvh down2 W H plane out` (`FUN_009f15b4`、2×2 平均。1 回の呼び出しで 2 行 → 1 行)・`nvh sad W H a b ax ay bx by [elem]` (`FUN_009f862c` 16×16・`FUN_009f9290` 32×32)・
    `nvh b32 W H ref cur cx cy thr` (`FUN_0098bf18`、32×32 探索と SAD 重心のサブ画素。記述子は `FUN_0097eebc`/`FUN_0097eef8`、オフセット表は {int バイトオフセット, short dx, short dy} × 8)。AutoFineTune のモード 2 のタイル順 `nvh aftshuffle N` (`FUN_009d39ec`、0 ≤ N ≤ 4096。外の関数を呼ばず、大域は __TEXT の定数 1 件だけ)。
    空間フィルタの葉 (2026-10-21 追加、sandbox/omo/wp_spatial の要求を統括が逆アセンブルで確認: 22 関数とも外への分岐・呼び出しなし、大域は 0x14a67a8 か __TEXT `__const` の 0x12504a0 (値 0) だけ、または大域なし): `nvh elem NAME n out s0 s1 s2 s3 in1 [in2 [in3]]` (要素ごとの 17 関数。recip `0x9ed5f8`・mul `0x9edc38`・add `0x9ed844`・sub `0x9ed710`・acc `0x9edeb8`・scale `0x9ee304`・affine `0x9ee3fc`・clampmul `0x9ee7bc`・blend `0x9ee1a4`・mulk `0x9edd6c`・u8 `0x9f02b4`・sqr `0x9ed2f0`・sqrtabs `0x9ed3e0`・sqrtk `0x9ed4e0`・fabs `0x9ee50c`・rstep `0x9ef260`・rinv `0x9eef5c`。スカラーは float32 の 16 進ビット)・`nvh strip3 W H plane box|sq mode sbits out` (`FUN_009ed014`・`FUN_009ed164` を呼び出し元と同じ 4 列帯のループで)・`nvh sigfuse` (`FUN_009f1d8c`)・`nvh ixform` (`FUN_009f1d0c`)・`nvh sel2` (`FUN_009f19cc`。末尾に `full` を付けると呼び出し元 `FUN_0098eba8` を呼んで端数部まで処理する。`FUN_0098eba8` は葉ではないが、呼ぶのは `FUN_009f19cc` だけで大域は 0x14a67a8 だけであることを統括が確認)。
    `FUN_00990f74` (空間の縮小核)・`FUN_0099c3a4`・`FUN_0099afb4`/`FUN_0099a76c` は難読化された読み書きや係数を含むので、ハーネスでは呼ばない
    (2026-10-22 に統括が保護区画の解析をユーザーから一任されて機構を読んだ。確定した式は §6.4・§7 に反映、詳細は
    `sandbox/omo/wp_spatial/notes/lead_static_20261021.md` と `coord/verify_protect/VERDICT.md`。ハーネスで呼ばない方針は変わらない)
  - 入力は 0〜255 の float の平面 (RGB は R,G,B の順に連結)。照合は `analysis/noise/verify_*.py` で、
    共通部 `nvh_common.py` が最初に `nvh selftest` の終了コード 3 を確かめてから本家を呼ぶ
  - 安全装置 (`nvh_guard.h`): 本家のアサーションはシステムのダイアログを出し、出し続ける箇所もある。
    `nvh_guard_preload()` を dlopen の前に呼び、dyld の画像追加コールバックで LC_UUID が一致したバンドルだけ、
    静的初期化子より前に警告関数の呼び先を差し替える (呼び出しスタブの命令を解読して確認)。NSAlert も無効化する。
    初期化子が後に走ったことは `nvh order` で確認済み (コールバック後に `__DATA` が 6540〜6592 バイト変化。最初の変化は
    `__objc_const` の class_ro_t フラグで、objc ランタイムの登録 (map_images) によるもの。dyld はこの登録の後に初期化子を走らせる)。
    dlopen 後に差し替えと UUID を再確認し、失敗すれば本家の関数を呼ばずに終了する (終了コード 5 / 7)
  - 本家を読み込むバイナリは必ず `nvh_guard_preload()` を dlopen の前に呼ぶこと (Makefile は `nvh` だけをビルドする)。
    ガードなしの試作 (nvh_probe 等) は削除済み
- 旧版: `python3 denoise_video_v2.py <in> <out> [--spatial ...] [--sigma-scale ...]` (v2〜v4、記録用)
- 要 ffmpeg (映像 IO)。Python 依存は numpy/scipy/pywt/Pillow のみ (要追記ならここに更新)

## 6. 追加解析 (2026-10-15)

### 6.1 RTTI→vtable 追跡で特定した本家関数 (arm64 スライス、Ghidra アドレス)
ラムダ型名 (`FuncPRun/FuncPFuncs<…$_N>`) の typeinfo → vtable → 参照元で親関数を特定。
成果物: `neatvideo_ofx_614/analysis/rtti/` (`INDEX_rtti.txt`, `*_parent_*.c`, `kern/fn_*.c`)、
スクリプト: `analysis/ghidra_scripts/{RttiLambdas,DecompAddrs}.java`
| 関数 | アドレス | 観測内容 |
|---|---|---|
| `astfqg::EstimateNoiseLevelCurve` | `0x9b6c8c` (worker `0x9b76b0`) | 輝度 **8 ビン** (`bin=8*Y/256`) ごとにノイズ和/画素数を集計。画素数 < `W*H/1024` のビンは無効→2〜3次多項式で補間/外挿→[1,2,1]/4 平滑。16px ブロック単位でクリップ率>0.33 を除外 |
| 画素ノイズ推定 | `0x998d98` | **16 方向 (整数格子 atan(1/4),atan(1/2),atan(3/4),45°… の 90°回転) × 2 スケール**のフィルタバンク。エネルギー比を**線形ランプ** clamp((r−0.8)/0.4), clamp((r−1.1)/0.4) (`FUN_009ee93c`) で重み化しエッジを除外 |
| `astfqg::FindFlatArea` | `0x9b1a5c` | 1/8 以下に縮小して探索。3ch 中 2ch 以上でクリップ率>0.33 なら棄却 (`IsClip.*` = `0x9c6648`)。Auto Profile は第3引数 0 でサイズ表から選ぶ (幅の **18%** の正方形は第3引数 1 の別経路)。詳細と移植は §6.5 |
| `astfqg::FilterTemporal` | `0x9a4800` → `0x9a815c` (CPU/GPU 振分) → CPU `0x9a84f8` | 近傍の重み = 類似度マップ (`FUN_0099a76c`、多重解像度、閾値 T² = (Threshold設定×0.16)²、モード1では 64)、中心の重み 1。`local_174` (中心 x、近傍 **0.64x**) は重みではなく**フレームごとのノイズ分散係数**で、Σw (`FUN_009ed844`: a+b) と Σf·w² (`FUN_009edadc`: acc+f·w²) を累積 → 残留ノイズ分散 Σf·w²/(Σw)² と推定。6 個の float を取る `FUN_009abbe4` は **3 チャンネル分の (ゲイン, オフセット)**: 中心フレームに (−1,0)×3 を渡して −中心 を作り、近傍には同じ組で `x·g+o` (`FUN_009aba00` / `FUN_009f08d4`) を掛けてから差を取る (フレーム単位の明るさ補正。§6.4) |
| 補助関数 (確定) | `0x9ed2f0` x², `0x9edc38` a·b, `0x9ed710` a−b, `0x9ee04c` a/(b+ε), `0x9ee3fc` a·x+b, `0x9ee93c` clamp 線形ランプ | `0x9ee93c` (a=t, b=2t, 1, 0) の本家とビット一致する式 (2026-10-17〜18、`nvh ramp|wmul|pairw|down2` と実装者 4,031 件・独立検証者 3,186 件で不一致 0。NEON 判定のバイトが非 0 の経路のみ。sandbox/omo/wp_temporal): b − a > b·1e-5 のとき (fcsel hi) そのまま、そうでなければ a ← a·0.99999 − 1e-5、b ← b·1.00001 + 1e-5 (t ≤ 0・±inf で効く。順序なしでは補正しないが slope が NaN になるので出力は同じ)。一般形 (a, b, c, d) では slope = f32((d − c)/(b − a))、ic = f32(c − a·slope) で、(t, 2t, 1, 0) では slope = f32(−1/(b − a))、ic = f32(1 − a·slope) (照合したのはこの組だけ)。先頭 n&~3 個は FMLA(x, slope, ic) → FMIN 1 → FMAX 0 (NaN 伝播)、残りは非融合の fmul・fadd → fcsel で clamp (NaN → 0、−0 → +0)。`0x9eebf8` はその結果 × prev (NEON 部は ramp·prev の順で NaN の選び方に効く)。`0x9efb1c` は c·(max(a,b) − b) で、NEON 部の max は FMAX (NaN 伝播)・端数部は a>b?a:b。2×2 平均 `0x9f15b4` は先頭 W&~7 列が (a+b)+(c+d)、残りの列が ((a+b)+c)+d (×0.25。奇数の最終列は捨てる。`repro/nv_pixnoise.down2` と同じ式) |
| ノイズ推定 (§6.5) | `BuildNoiseModel 0x9af718`, `AutoFineTune 0x9a775c`, `FindFreqLevs 0x9b2814`, `BuildFreqImage 0x9b0b68` | 2026-10-15 に解読 (FindFreqLevs / BuildFreqImage の呼び出し元は表示用ジョブ `FUN_0014f9a0` だけ。2026-10-17 に静的に確認) |
| 空間フィルタ (2026-10-17、sandbox/omo/wp_spatial を独立検証者が確認) | `Filter 0x9ac2b0` → CPU `FUN_009adc8c` → `FUN_009ae918` → 面ごと `FUN_0098edb0` → 縮小核 `FUN_00990f74` | 間引きラプラシアン 5 段 (3×3 箱平均 → 2×2 平均で縮小 → (3,1)/4 で拡大して差)。面ごとの `FUN_0098edb0` が白色化 W = B/(σ + 1e-4) (定数 0x38d1b717、0x98f86c〜0x98f880 で `FUN_009ed5f8` = 1/(x+ε) を呼ぶ) をして縮小核に渡し、縮小核は R = √(3×3 平均(W²)) (`FUN_009950f4`)、X = W·clamp((2t − R)/t, 0, 1)。通常経路の t = (cfg[k] + 1e-6)·\|model[+0x14+4k]\|·1.1 (0x99155c〜0x991574)、出力の乗数は −cfg[+0x14+4k] (Amount 系。0x991748〜0x991758 で読み、cfg+0xb8 ≤ 0 か値が 1 のとき 0x9917a0 の fneg で符号を反転。それ以外は `FUN_009ee7bc` の適応経路)。通常経路のランプと積は 0x9916a4 の `bl 0x9eebf8` 1 回 (s0 = t、s1 = t+t、R・W・X と要素数 n を渡す。n = 面の要素数というのは推定)、縮小は `FUN_0098edb0` の 0x98f2bc〜0x98f2dc で `FUN_009f15b4` を H>>1 回 (H < 2 なら呼ばない)。移植 (`wp_spatial/src/nv_spatial.py`) の halve と縮小式は 2026-10-21 に本家と同じ演算にした (実装者 1,785 件・独立検証者 2,484 件で `nvh down2|ramp|wmul` と不一致 0。区間 A/B の PSNR は変更前と 3 桁目まで同じ)。2026-10-21 の追加の観測 (wp_spatial の実装者の読みを独立検証者が抜き取り確認、`coord/verify_spatial3/VERDICT.md`): 局所 RMS (`FUN_009950f4` → `FUN_009ed164`) は端を 0 で埋め、3×3 箱 (`FUN_009ed014`) は端を複製する (どちらも `nvh strip3` で本家と一致)。方向フィルタの組は表 0x1246c48 (0x1246c48 + 色モード·0x48 + 品質·0x18 + 8·面) から選ばれ、既定の YCrCb では Y が組 2、Cr・Cb が組 1 (実装者の読み。本番は方向経路 0x991aac〜を通るとみられる)。方向経路の定数 4.5 (0x991ac8)・1.05 (0x3f866666) は確認、経路全体は本家と未照合。周期パターンの保護 (組 4) は品質 2 のときだけとみられる。`FUN_00990f74` の入口には難読化様のコードがあり、ハーネスでは呼ばない |
| 動き探索の葉 (2026-10-17〜18、sandbox/omo/wp_motion を独立検証者が確認) | SAD `0x9f862c` (16×16)・`0x9f9290` (32×32)、32×32 探索とサブ画素 `0x98bf18` | SAD は 8bit 面の Σ\|a−b\|。`0x98bf18` のサブ画素は SAD 重心で、すべて float32 の単独演算 (FMA・double・逆数近似なし): inv = 1/(thr + 0.01f)、d = inv·(float)(SAD − best) (64 ビット符号なしから変換)、w = (d·−0.66f) + 1.0f (2 回丸め、clamp なし。基準位置だけさらに +0.0f)、d < 1.5151515f (0x3fc1f07c) の近傍だけを表の順に num += (dx,dy)·w・den += w、出力 = num/den + (float)位置。`nvh sad`・`nvh b32` と、実装者 15,058 件・独立検証者 5,446 件 (実写切り出しを含む) で不一致 0。照合はハーネスで呼べる経路 (ブロックが面の内側) だけで、端に掛かる遅い経路・NaN の閾値・elem 4 は未照合。打切りの向き (< か <=) は出力から区別できず、逆アセンブル (b.pl) で確認。移植 `wp_motion/src/nv_motion.py` で v5 のフローを置き換えると区間 A の Y は 47.117 → 46.609 dB (v5 の既定は変えない)。パターン探索 `0x98b9a4`・`0x98adbc` (32×32 枝の入口に難読化様の判定があり未調査)・平滑化・回帰 `0x98c90c`・レベル間の伝播は未照合 |
| その他 | `FilterVideoField 0x9a17b8`, `MotionCompensator::{EstimateScaleMotion 0x98a368, CompensateImageMotion 0x989cd8, CompensateImageSet 0x987f88}` | 静的な解読は sandbox/omo/wp_motion/notes/decode_motion.md (本家との照合は上の葉だけ) |
※ 周辺に難読化 (ハッシュ値をグローバルに書き込む改ざん検知様コード) が混在。T² の単位は推定を含む。
※ 訂正 (2026-10-15): 初版の「近傍フレーム重み = 中心×0.64」「smoothstep」、第2版の「FUN_009abbe4 = 大域アフィン」は誤読。上記が補助関数を確定させた後の読み。

### 6.2 ブラックボックス比較 (test_v02 vs 本家出力, n=296〜327, ネイティブ YUV)
成果物: `neatvideo_ofx_614/analysis/blackbox/` (`bb_analyze.py`, `bb_out.txt`, `exp_compare.py`, `exp_results.txt`)
- フレームずれ 0、全体輝度/色の変化なし (±0.04) → **全体フリッカ補正は掛かっていない**。1800x1000 外も一様に処理。
- 除去成分の帯域別 RMS ≈ 入力ノイズの帯域別 σ (L1 2.25/2.18, L2 1.10/1.15, L3 0.54/0.57) → 全帯域でノイズ分を除去し、信号は保存。
- ノイズは強く有色: 白色換算 σ が L1=2.4 に対し L2〜L6=5〜7。nvfilter の HH1-MAD 推定 (0.74) は 3〜10 倍過小 → `--sigma-scale 4` が必要だった原因。
- ノイズは輝度依存 (σ_t 2.2〜3.7)、本家残留比は全輝度で一定 0.21〜0.25 → 輝度別ノイズ曲線で適応。
- 時間方向の残留 (フレーム差 RMS) 0.22 倍 ≈ 5 フレーム一様平均の理論値 0.2 → 静止部はほぼ一様重み。
- 局所適応: 平坦部の高域ゲイン 0.20、テクスチャ部 0.98。
- 実験 (16 フレーム, Y PSNR vs 本家): 入力 37.86 / 現行 v4c 設定 41.79 / 改善案 時間のみ 44.41 / **改善案 時間+空間 46.45 dB**
  (改善案 = 輝度別ノイズ曲線 + 一様 0.64 重み×画素単位類似度 + 平滑化した密フロー + 局所 Wiener。0.64 の解釈は §6.1 で訂正済み)
- 既存出力 `test_v02_denoised_v4c.mp4` は色ずれあり (Y −1.7, U −1.0, V +0.8, 彩度 +5%)。原因は色行列の不一致。
  v5 の CLI で検証: 出力側だけに `-colorspace bt709` を付けても ffmpeg 8 が raw 入力を BT.601 とみなして変換し
  Y −1.9 ずれる → 入力側にも同じタグを付けて解消 (`denoise_video_v5.py` の `encoder()`)。

### 6.3 v5 実装と較正 (2026-10-15)
- 構成: §4 の表のとおり。本家に合わせて変えた点は、ネイティブ YUV、輝度別ノイズ曲線、画素単位の時間重みと残留ノイズの空間段への受け渡し、
  5 帯域の非間引き分解、全体フリッカ補正の既定オフ、色タグの保持。
- プロファイル既定 `auto` (本家との差分あり): 曲線は本家と同じ単一フレーム推定、帯域形状は静止画素 (≥10%) の時間差分から。理由は次の2点 (実測)。
  ① 幅18%の領域は粗い帯域で景色の構造を含む (test_v02 では空＋雲＋丘。L3〜L5 の帯域比が時間差分の
     2〜20 倍 (領域全体の MAD)、サブブロックの 25% 値にしても 1.6〜4.6 倍)。
  ② test.mp4 は GOP 12 で I フレームのノイズが P より大きい (静止画素の L1 std: f300=20.9 / f301=16.5) → 3 フレームの中央値を使う。
- 時間差分推定は量子化に強いトリム RMS (上位1%除外)。MAD は 1LSB 未満の色差ノイズで 0 近くに潰れるため使わない。
- 較正 (`blackbox/calibrate_v5.py`、区間 A で座標降下 → 区間 B で確認、ログは `calib_search_296.txt` / `candidates_v5c_results.txt`):
  `threshold` 4.0、`luma` 0.7、`spatial` 2.0、`CHROMA_GAIN` Cb 3.0 / Cr 4.0 (単一フレームの色差推定に掛ける)。
  本家出力の設定は不明なので、これらは「test_v02 の本家出力に合わせた推定値」。文献の Luma 40% 相当 (luma 0.4) は Y で −0.45 / −0.30 dB (区間 A / B)。
  プロファイルモード比較 (A / B の Y・U・V): auto 47.12・50.99・51.18 / 47.22・50.92・51.19、temporal 47.09・50.86・50.76 / 47.28・50.75・50.68、
  spatial 46.92・48.28・47.60 / 46.85・50.38・49.72 → 色差を含めて auto が最良 (`candidates_v5d.json`)。
  spatial はレビュー修正後 (旧領域) 47.01・51.00・51.20 / 47.07・50.92・51.20、領域選択を本家移植に替えた後 (§6.5) は
  47.11・51.00・51.20 / 47.19・50.92・51.20。auto は不変 (test_v02 では処理が領域に依存しない。条件は §6.5)。
  Cr のみ係数 4 にすると両区間で V が +0.3 dB 良くなる (U は 3 が最良)。
- 独立レビュー (code-reviewer エージェント) の指摘への対応 (数値は `exp_results.txt` の post-review 節):
  - ノイズのない領域 (黒帯・ベタ塗り) で帯域比が発散 → IsClip を RGB 換算に変更 (黒帯は YCbCr だと Y しか限界に達しない)、
    5x5 近傍が完全に一定の画素を全統計から除外 (領域は 5% 超で棄却)、帯域比に減衰の上限 (時間差分 0.9 倍/段・領域 0.6 倍/段)、
    σ1<0.02 なら白色形状。黒帯の合成ケースで auto の Cb 誤差 2.04→9.94 が 2.04→2.23 に改善 (Y は 3.42→1.95)。
  - 回転タグ付き入力が崩れる → `-noautorotate` で符号化時の向きのまま処理し、`-display_rotation` で出力に引き継ぐ (エンコーダ入力側にも `-noautorotate`)。
  - VFR で全編と区間の番号が食い違う → r_frame_rate の CFR タイムライン (`setpts=PTS-STARTPTS,fps` → `select`) で番号付け。
    CFR 素材は passthrough とビット一致 (test_v02 全721f のハッシュ一致)、VFR は尺を保持。奇数サイズ → 偶数に切り詰めて符号化 (注記を出力)。
  - nb_frames の過大/不明 → `--frames` を切り詰めず、プロファイル用フレームが読めなければ先頭へ戻る。`-nostdin`、`-frames:v` で早期終了、
    フリッカは黒レベル (16) を保持。UI: 古い設定のプレビューを破棄 (世代番号)、モード切替で再プロファイル、smpte170m/bt470bg を BT.601 として扱う。
  - test_v02 では処理に使うプロファイル値 (曲線・帯域比・total 比) が修正前後で同一 → 全編・区間の数値はそのまま有効 (区間は再測定で一致を確認)。
  - 再レビューで 14 件すべて FIXED・APPROVE。追加対応: 無ノイズ画素が画面の 50% 超ならノイズなしとして σ=0 (本家由来ではない安全策。
    testsrc2 で出力≒入力、test_v02 / test.mp4 は 0% で影響なし)。ロスレス出力で窓処理のビット一致を 4 条件で確認。
- 本家とビット一致したノイズ推定を v5 に入れる試験 (2026-10-17、sandbox/omo/wp_integrate、独立検証者が区間 A を再測定。数値は `exp_results.txt` の「integrate」節):
  ノイズ曲線を本家の EstimateNoiseLevelCurve (輝度のみを測る) にすると、区間 A の Y は 47.117 → 46.984 (B 47.222 → 47.199) で、色差は U 41.35・V 40.98 と大きく悪い
  (輝度曲線 × CHROMA_GAIN を色差に流用するため σ が過大)。帯域比を本家の領域モデル (`nv_profile` の q99 語) にする案は、尺度を揃えると区間 A で 47.118 / 50.996 / 51.203 で v5 既定と同等。
  既定は変えない (コードはマージしていない)
- 未再現・今後 (2026-10-17〜18 の作業後。数値は `exp_results.txt` の対応する節):
  - AutoFineTune の輝度別係数: 静的移植 (`analysis/noise/aft_port/`、本家と照合したのはタイル順だけ。保護判定があるためハーネスでは本体を動かさない) のみ。
    v5 の Y 曲線を移植の曲線で整形した変種は区間 A / B の Y で +0.008 / +0.024 dB (同等。既定は変えない)
  - 空間フィルタ: 本家寄せの移植 (間引きラプラシアン＋縮小核、方向フィルタの組なし) は区間 A の Y が t=1.0 で 46.314、t=1.5 で 46.997 (v5 47.117。誤った ε 9.4433e-4 での測定で、ε = 1e-4 では検証者の再測定で 46.311 / 46.996)。本家と未照合
  - 動き: 葉 (SAD・サブ画素) のみ本家と照合。移植でフローを置き換えると区間 A の Y は 46.609。パターン探索・平滑化・回帰・EstimateScaleMotion の本体は本家と未照合
  - 16 方向の構造検出 (v5 は帯域比テストで代用)。類似度関数は §6.4、ノイズ推定は §6.5 で解読済み。残差の過半は本家出力自体の圧縮ノイズ (§6.4)。

### 6.4 時間フィルタの類似度関数の解読 (2026-10-15)
逆アセンブルで浮動小数点レジスタの受け渡しを追跡し、補助関数と定数表 (`kern/fn_*.c`) を読んで確定した。
- 差分の作り方 (`FUN_009a84f8`): 本家の内部プレーンは **RGB**。各チャンネルにフレームごとの (ゲイン, オフセット) を掛けて
  中心との差を取り、関数表 `DAT_0147dfb0[モード]` で 1 枚に合成する: モード0 = 第2プレーン (G)、モード1 = 0.2989R+0.5866G+0.1145B
  (`0x97bb88`、BT.601 輝度)、モード2 = 3 プレーン使用 (`0x97bbb0`)。
- `FUN_0099a76c`: 差分を 4 段のピラミッドに分解 (`FUN_0099c3a4` が段のサイズ、`FUN_009f15b4` が 1/2 縮小、`FUN_00990da8` が
  「縮小→拡大して差」= 帯域分割)。粗い段 (3) から細かい段 (0) へ `FUN_0099afb4` を呼び、段の重みを `FUN_00993df0` で拡大して次段に掛ける。
- `FUN_0099afb4` (段 k): X = 差 / (ノイズ分散ピラミッド + 1e-4) → R = √(3×3 平均(X²)) (`FUN_009950f4`)。
  閾値 t_k = s_k × C_k × 1.1 × 0.9 × **1.2806249 (=√(1+0.64))** × σ_k (+ 最粗段のみ T²)。C = `DAT_012445a8` = (1.06, 1.0, 1.0, 1.0)、
  s_k = 段の係数 `param_8[k]` (呼び出し元で 100% か設定値 `0xd4`。最粗段だけ ×1.3)、σ_k = ノイズ構造体の段ごとの値 (実行時に決まり数値は未確定)。2026-10-17 に逆アセンブルで確認: 構造体は `FUN_009a84f8` が 0x9a8a20〜0x9a8a2c で作るインスタンス +0x17330 で、`FUN_0099a76c` の第 9 引数を経て 4 か所の呼び出し (0x99aca0 / 0x99ad64 / 0x99ae0c / 0x99aeb4) に渡り、0x99b3c4 で +0x14+4k を fabs で読む。+0x17330 はインスタンスのプロファイル複写 (+0x114e8、`FUN_0099f79c`) の変換面 3 (色モード 1 では Y) のモデル面ブロックで、σ_k はその語 5+k (帯域の 99% 値) に当たる (同じクラスであることはオフセットと呼び出しの連鎖からの推定)。時間側は Y 面のモデルだけを使う。
  重み = clamp((2t_k − R)/t_k, 0, 1) を前段の重みに乗算 (`FUN_009eebf8`)。√1.64 は「中心(分散1)と近傍(分散0.64)の差」の標準偏差係数で、
  0.64 = 近傍の分散係数という読みを裏付ける。FUN_0099afb4 内の段係数 s15 = ((DAT_012445a8[x27] × 1.1f) × s8 × 1.2806249f) × 0.9f ×
  (x22 − (popcount(w19) − 16)) (0x99b364〜0x99b3b8。1.2806249 = √1.64、0.9 = 0x3f666666。w19 = *(u32*)(*(u64*)0x1488030 + 0x763648)
  = *(u32*)(image+0x1489E68)、静的初期値 0xbc5368a5 で popcount 16 → 減算値 0。実行時に書き替わる可能性あり)。s8 は FUN_0099a76c が
  ×1.3 ×[x24+0xc] して v0 で渡す値で、「w22 (大域状態語) の count 型鎖 + w10(raw)>>1 レーン」の 64bit 値 (popcount(w22) の単純関数ではない。
  2026-10-22 に独立検証者が命令列のネイティブ再実行で確認。coord/verify_protect/VERDICT.md)。旧記載の「17 − popcount(改ざん検知値)、正常時 1」は
  近似で誤り (訂正)。正常時の値は依然未確定。
- 方向フィルタによる構造的な差の検出 (`DAT_01243988` 等): モード0 = なし、モード1 = 0°/90°/45°/135° の 4 方向、
  モード2 = atan(1/2) 系を加えた 8 方向。差を方向別にフィルタして二乗し、平滑化して正規化 (`FUN_009ed4e0`: √|x|·k)、最大値で統合 (`FUN_009efc94`)。
- 実装: `nvfilter.pyramid_weight` / `similarity="pyramid"` + `pyramid_scale` (既定 1.5 = 区間 B の最良。a-trous 帯域で代用、最粗段は T²≈64 でほぼ棄却しないため省略)。
  本家出力との一致 (`candidates_v5e/f.json`、Y / 動き領域 Ymov): 区間 A は v5 既定 47.12/46.98 に対し pyramid s1.5 47.08/46.94・s2.0 47.10/46.95、
  区間 B は v5 既定 47.22/47.05 に対し s1.5 47.22/47.05 → **同等** (動き画素は全体の約 23.5%)。既定は v5 のまま、選択肢として残す。
- 測定の下限: 本家出力を同じ 28Mbps で x264 再圧縮すると本家自身との PSNR は Y 49.69 dB (CRF16 では 46.71)。
  区間の誤差 (47.12 dB = MSE 1.26) の約 55% は本家出力自体の圧縮ノイズで、アルゴリズム由来の差は 50.6 dB 相当。

### 6.5 ノイズ推定の解読 (2026-10-15)
静的に読んだ内容を、本家バンドルの内部関数をハーネス (`analysis/harness/nvh`、§5) で直接呼んで数値で確かめた。
照合スクリプトは `analysis/noise/*.py`、数値は `exp_results.txt` の「noise-estimation decode」節。
- **Auto Profile の流れ** (`FUN_0014e4a4`): FindFlatArea (`0x9b1a5c`、第3引数 0) → 領域の切り出し (`FUN_009c7318`)
  → BuildNoiseModel (`0x9af718`) → (指定時) AutoFineTune (`0x9a775c`)。FindFreqLevs / BuildFreqImage は別ジョブ (`FUN_0014f9a0`) で、
  周波数画像 3 枚の表示範囲 (30/70% 分位から中心 ±4 倍、1/99% 分位で制限) を決める。ノイズ値には関わらない表示用とみられる (推定)。2026-10-17 に静的に確認 (観測): 全逆アセンブルの bl/b 参照で、BuildFreqImage の呼び出しは `0x14fa3c`、FindFreqLevs は `0x14fa9c`・`0x14facc` の 2 回で、どちらも `FUN_0014f9a0` の中。`FUN_0014f9a0` の呼び出しは `0x14fc94` (`FUN_0014fbb0`) だけで、`FUN_0014fbb0` はジョブ表 (データ参照) からのみ参照される
- **内部の尺度**は RGB の float 0〜255 (16bit 型は ×16 の固定小数点)。IsClip (`FUN_009c6f88`) は値域を 1/8 刻みのヒストグラムにし、
  最頻ビンが過半数を占めるか分布の端にある (片側が 10 倍以上多い) ときだけ、そのビンの割合を返す (孤立した 255 は数えない)。
- **帯域ノイズ測度** (`FUN_009980e8`。BuildNoiseModel 内の `FUN_00995250` もほぼ同じ段統計で、違いは BuildNoiseModel の項):
  - ラプラシアン・ピラミッド 5 段: 3×3 箱平均 (端は複製) → 2×2 平均で縮小 → (3,1)/4 で拡大して差を取る
  - 局所 RMS = √(3×3 箱平均(帯域²)) (端は 0 詰め)
  - 端 2px (BuildNoiseModel は 4px) を除いて、平均・99%・中央値と 8 オクターブの変動係数を取る。小さすぎる段は −0.5×|前段| (負 = 無効)
  - 99%・中央値は本家の分位点関数 (`FUN_00997bcc`) による: 平均から ±σ 刻みで閾値を動かし、刻みを −1/2 倍しながら最大 8 往復、
    標本 49 以下は前後の値で補間、n/4 が max(1/(1−q), 10) 未満なら正規分布近似 (平均 + Φ⁻¹(q)·σ·小標本係数) を下限にする
  - 箱フィルタは横 → 縦の順 (移動和)。白色ノイズ σ10 で 9.07 / 2.20 / 0.97 / 0.45 / 0.22
  - 2026-10-16 に演算順まで移植した (`repro/nv_band.py`)。出力構造体の 4096 語すべてが本家とビット一致する
    (初期化パターン、小さすぎる段の外挿、-0・NaN のビット列まで。`verify_levels.py` の 161 平面と、独立検証者の敵対的入力・乱数の計 2,000 平面以上)
    - 平均 (`FUN_009ef3fc`) は 2048 値ごとのブロックで 4 レーン×2 組を順に累積し、ブロック和を double に足す。標準偏差 (`FUN_009ef520`) は
      レーン部が FMLA、端数部が非融合。値の下限 1e-6 は FMAXNM (NaN は 1e-6 になる)
    - 分位点 (`FUN_00997bcc`) は制御の流れをすべて解読した (`nvh quantile` で 933 件一致)。構造体の初期値は格納命令の静的な再生で得た
    - 以前の近似移植の相対誤差は、ケースごとの最大の中央値で 2.3e-6、直流 1e6 の入力では最大 0.18 (変動係数) だった
- **FindFlatArea** (Auto Profile 経路):
  - 縮小率 s≥8 (縮小後 2^18 画素以下) で箱平均した輝度から候補を作る。高域の局所 RMS が小さい位置を、16 輝度ビン × 4 個まで取る (`FUN_009999cc`)
  - 画素数別のサイズ表 (`0x1246d30` 等。1080p: 320/192/128/96/64/48、係数 1/1.15/1.33/2/4/6) の各サイズで、全候補を次のスコアで評価し、最小を選ぶ:
    (2·max(2 − 段比の平均, 0) + 1) × サイズ係数 × (4·クリップ ch 数 + 1) × Π平坦度 × (Σ_k 2^k·L_k / 段数) / 予想ノイズ(輝度) × (|輝度 − 96|/512 + 1) × (どれかのクリップ率 > 0.1 なら 2)
  - 記号: L_k は 3 面 (色モード 1 は Y・(R−Y)·0.713・(B−Y)·0.565) の帯域測度の和。平坦度 (`FUN_009b096c`) は、粗いスケールの変動係数と 99%/中央値比がノイズより大きいほど増える。予想ノイズは EstimateNoiseLevelCurve から取る
  - 粗い段ほど重く数え、白色より低域寄りのスペクトルを罰するので、雲・丘のような大きな構造を強く避ける
  - test_v02 では画面上端の空の 128×128 を選ぶ (例 n=312: x832・y0。n=576 だけ下側の 64×64)。v5 の旧実装は雲・丘を含む 346px を選んでいた
  - 幅 18% の正方形を返すのは第3引数 1 の別経路で、n=312 では猫の顔を含む中央を選ぶ (状態 3 = 平坦でない)
  - 2026-10-16 に縮小 (`FUN_009b7e58`)・候補 (`FUN_009999cc`)・色変換・平坦度 (`FUN_009b096c`)・スコア・選択の演算順まで移植した (`repro/nv_flat.py`)。
    最終コードで、実装者と独立検証者 2 人・修正後の再検証者 1 人が照合した (計 4,000 件以上、不一致 0)。領域・状態コードと、縮小面・候補 (計 3 万個以上) が本家とビット一致した。
    入力は実写 (実装者の 67 フレームと、検証者が選んだ 120 フレーム (うち 105 は実装者が使っていないもの)。test_v02・test.mp4) × 色モード 0〜2、1440p〜8K、
    48px からの切り出し、合成 (黒帯・白飛び・定数・NaN/inf・同点の候補・サイズ表の境界)、乱数。
    本番の入口 `nvfilter.find_flat_area` は、実装者の 32 フレームで 32/32、再検証者の別の 16 フレームで 16/16 (移植とは別に集計)、検証者の `adv_prod` で 27/27
    - 解読した細部: 候補の走査は右下から逆順、厳密な小なりで最小を選ぶ (NaN は選ばれない)。半径 0 の箱フィルタは本家ではコピー。
      平坦度の色差の限度値は 2.7f ではなく 0x402ccccc。サイズ表の選択は画素数 ≤ 0x280000 / > 0xA00000
    - 本家の不具合 (ハーネスでの観測。実ホストで同様かは推定): 48px 未満のフレームでは合う正方形がなく、`FUN_00a22868(NULL, 100)` を呼んで SIGSEGV になる。
      移植は (0,0,0,0,1) を返す。本番は 64px 未満で代替の方法を使うので、この経路に入らない
    - FindFlatArea 内の難読化された判定 (0x9b1dfc〜0x9b1e7c) は、呼ぶたびにカウンタ (0x1488348) を更新し、上位 2 ビットが 0 のとき
      トークン表由来の値を縮小後の高さに足す。ファイルの初期値では 1 回目は足さないので、照合は 1 プロセス 1 回に限った。
      正規のホストで加算が 0 になるというのは推定。判定の回避・パッチはしていない
    - 状態 5 (取り消し。2026-10-16 にハーネスで観測): 取り消しの問い合わせはフィルタ核の vtable 0 番。EstimateNoiseLevelCurve のワーカー
      (`FUN_009b76b0`) はタイルを取る前に毎回、さらにスケジューラが空になったときにもう 1 回問い合わせ、取り消しなら偽を返す
      (FindFlatArea は状態 5。出力の領域は書かない)。FindFlatArea も各サイズの最初に問い合わせ、取り消しならそれまでの最良の正方形
      (512 への切り詰め前。まだなければ未記入) のまま状態 5 を返す。この経路は作業バッファ 7 個の解放を通らない (2026-10-17 に逆アセンブルで確認。7 個は `0x9b1cc0〜0x9b1d3c` の `FUN_009d7200` で確保し、解放は `0x9b2620〜0x9b2658` の `FUN_009d7910` だが、状態 5 の入口 `0x9b1d88` (曲線が偽)・`0x9b1f5c` (取り消し) はどちらもそこを飛ばして `0x9b2734` に入る。`0x9b2734` で呼ぶ `0xaed9b4` は曲線に渡した一時オブジェクトの配列 2 本を delete[] するだけ。取り消しのたびに数 MB 未満を漏らすとみられる (推定。アロケータ側で別途回収される可能性は静的には排除していない))。Auto Profile (`FUN_0014e4a4`) は状態 5 では BuildNoiseModel を呼ばない。
      問い合わせ回数は 1 スレッドで タイル数 + 1 + サイズ数 (1080p 240+1+6 = 247)。移植 (`nv_curve`・`nv_flat` の `cancel`) は回数と結果が本家と一致
      (`verify_cancel.py`。数値は `exp_results.txt` の「status 5」節)
    - 速度 (M2 Max、負荷平均 5.5〜6.7、新しいプロセスで 3 回): FindFlatArea 部分 (`find_flat_area_genuine`) は 1080p 0.79〜0.86 秒、4K 1.81〜2.02 秒、
      8K 2.39〜2.66 秒 (以前の近似移植は 2.47〜2.63 / 4.73〜4.75 / 5.92〜6.03 秒)。本番の入口 (曲線を含む) は 1080p 1.62〜1.76 秒、4K 4.98〜5.14 秒
      (以前 3.19〜3.24 / 7.80〜7.93 秒、1080p で約 1.9 倍・4K で約 1.6 倍速)。本番の入口の曲線は 1 回目の呼び出しで負荷下のため、
      単体の最良値 (0.67 秒) より遅い。本家の `nvh flat` (1 スレッド、曲線込み) は並列負荷下の参考値で 1080p 0.63〜0.71 秒、4K 1.8 秒
    - メモリ: FindFlatArea 部分の RSS 増分は 1080p +100〜134 MB、4K +111〜151 MB、8K +123〜166 MB (以前の近似移植は +24〜104 MB)。
      `nv_flat.BATCH_FLOATS` (既定 3M 値) を 1M にすると 1080p で +43 MB・0.91 秒 (1 回の測定。実装者の別の測定では 1.09〜1.22 秒)。
      本番の入口の tracemalloc のピークは 1080p 272 MB・4K 413 MB で以前と同じ。ピークは RGB 変換と曲線の段にあるとみられる (tracemalloc が以前と同一であることからの推定)
- **BuildNoiseModel**:
  - 領域の RGB 3 面と色変換後 3 面 (色モード 0 RGB・1 YCrCb・2 平均系) の計 6 面について、5 段の帯域測度と方向フィルタ (`DAT_01243988` の 5 組) の 99% 値をプロファイル (+0x4c から 1 面 0x1f54 バイト) に保存する
  - 段統計 (`FUN_00995250` の語 0..54) は `nv_band.model_levels` が本家とビット一致する (155 平面)。測るのは W, H ≥ 9 かつ
    (W−8)(H−8) ≥ 32 の段だけで、小さすぎる段の外挿の係数は前段の |q99_k / q99_k−1| (0.5〜1.0)。以前の `band_model(border=4)` は
    最大 87% (極端な入力。実フレームの切り出しでも 75%) ずれていた。外挿の係数が −0.5 固定だったことが原因とみられる (本番では未使用)
  - 2026-10-16 に語 55 以降 (方向フィルタ) とプロファイル全体も移植した (`repro/nv_model*.py`・`nv_profile.py`。別エージェントが `sandbox/dirfilt/` で実装し、
    統括が独立検証の後にマージ)。数値は `exp_results.txt` の「BuildNoiseModel full port」節
    - 方向フィルタの組 `DAT_01243988` (5 組 × 0x260 バイト、静的に読み出し): 組 0 なし、組 1 は 4 方向、組 2 は 8 方向、組 3・4 は 16 方向 (FUN_00998d98 と同じ表)。
      族 A (半径 4: 帯域から線フィルタを引いて `FUN_009ef0d0` で頑健化した帯域の方向エネルギー)、R (argmax 方向の応答を引いた残差)、dc (残差を帯域の局所 RMS で重み付け)、
      族 B (組 3・4、半径 6) と族 C (組 4、半径 12) は 2 帯域画像の方向エネルギー。いずれも 99% 値。配置は `analysis/noise/model_notes/layout.md`
    - 組 4 は `FUN_00ad319c` も呼ぶ: 帯域画像の自己相似パターン (32×32 の SAD で 12+12 個のオフセットを選ぶ) のスコアを最大 5 回、続く `FUN_00ad1ae0` は
      2 種のブロック照合の比の分位点 0.5 / 0.85 (自己相似のスコアはバイト 0x1464 = 語 1305 以降、分位点はバイト 0x1f04〜0x1f53 = 語 1985〜2004。`model_notes/layout.md`)。周期的な固定パターンのノイズの検出とみられる (推定)
    - 照合 (観測): `nvh model` は実装者の 753 件 (463 面 × 組 −1 と組 0..4 の 290 件、3,084,288 語) と独立検証者の 361 件 (195 面、新しい入力) で不一致 0。
      `nvh refmodel` 2005 語一致。`nvh profile` (領域指定) は実装者 76 件と検証者 29 件で、除外 9 語 (ポインタ 6・LZMA の長さ 3) 以外の不一致 0。
      LZMA の長さも移植した後は、`verify_profile.py` 76/76 でポインタ 6 語以外の不一致 0
      感度試験は 14 種の注入のうち 13 種で検出 (残る 1 種は呼び出しが ×1+0 だけの等価な変更)
    - 再現した本家の癖 (観測): 無順序比較 (NaN) で b.le・b.lt が成立する、NEON の 2 次元箱 (`FUN_009ed014`) と分離型の丸めの違い、
      `FUN_009ef0d0` は NEON 部が FRECPE+FRECPS・端数部が真の除算、方向ビン (`FUN_00acf420`) は libm の atan2 / __sincos_stret の結果に依存
      (移植も ctypes で libSystem の同じ関数を呼ぶ)、`FUN_009b2f34` の下限は NaN を下限に置き換える、曲線のビンは fcvtzs で飽和 (+inf → 8)
  - 見本画像の LZMA 圧縮 (`FUN_009b41c4`、2026-10-16 に移植。数値は `exp_results.txt` の「LZMA sizes」節): 領域を 16 ビット型 (`FUN_009f0554`:
    rint(x·16)、int16 に飽和、NaN → 0) に写し、R・G・B 各面 (W·h·2 バイト) を `LzmaCompress` (level 5、辞書 = 長さを 4KiB に切り上げ、
    lc3 lp0 pb2 fb32、1 スレッド) で圧縮して、16 + 圧縮長を +0x18/+0x1c/+0x20 に書く。出力の上限 (長さ + 0x80) を超えると後始末
    (`FUN_009ba4cc`) が 0x00〜0x2b と 0xc2a4 を 0 にする。本家の圧縮データとバイト単位で一致したのは SDK 9.20 と 16.04 (19.00・21.07・
    23.01・24.09・26.04 と Python の lzma は異なる)。2026-10-17 の追加照合 (本家の圧縮データ 228 件): 作業者の 28 版のスイープでは 9.20〜18.05 がすべて一致し 18.06 が初めて異なる
    (独立検証者が 9.20・16.04・17.01・18.01・18.03・18.05 の一致と 18.06 の不一致 (227/228) を再確認。辞書は 128KiB 以下の範囲)。静的には `LzmaEncProps_Normalize` (0xb9c5b4) に reduceSize の切り下げがなく `LzmaEncProps_Init` の構造体も 9.20 の形で、ソースは 9.21 より前とみられる。
    呼び出し元は容量 W·H·2 + 0x90 を渡し、`FUN_009d61f4` は destLen = 長さ + 0x80 で `LzmaCompress` を呼び、0 以外なら −1 (乱数の面は 16KiB 以上でこの上限を超える。C のオラクルで観測、本家での実行は未確認)。移植 `nv_lzma` は C 9.20 と 366/366 入力、本家と 88/88 プロファイルで一致 (観測)
  - 32px タイル (16px 間隔) と全体モデルのずれの RMS (`FUN_009b2f34`) も保存し、後段の類似度判定の許容幅に使う。下限は 256×256 の白色ノイズ (シード 0x12345678) で作る基準値
  - 輝度曲線 (+0xbf40) は 3ch × 9 ビン (中心 0,32,…,256) × {レベル, 段係数 5, 有効, フラグ}。作成直後は、領域の平均輝度のビンだけが 1.0
  - test_v02 (n=312) の Y の局所 RMS は 3.21 / 1.65 / 0.84 / 0.48 (段比 0.514 / 0.510 / 0.577。白色ノイズは本家の σ10・256px の測定で
    0.242 / 0.439 / 0.468 なので低域寄りの有色)。色差は 0.10〜0.21
  - 同じ領域の時間差分ノイズの段比は 0.515 / 0.496 / 0.610 で、差は 6% 以内。v5 auto の「帯域形状は時間差分から」は
    本家の領域モデルと整合する (レベルは時間差分が 15〜22% 高い)
- **AutoFineTune** (`0x9a775c`、タイル処理 `0x9b56f4`):
  - 全画面を 32×32 のタイルに分け、飽和のないタイルの RGB 各面で 2 段のモデルを作る
  - プロファイルとの類似度 (`FUN_009b6338`) は、帯域レベル比・段比・方向フィルタ・変動係数・99%/平均の各ずれ x を、許容幅 s で clamp(2.5 − 2x/s, 0, 1) にした積。3 面の積が 0.5 を超えたタイルだけを使う
  - 使うタイルについて、最細段の比 (タイル ÷ プロファイル) を各面の平均輝度のビンに集める。ビンごとの係数は 66% 分位 (`FUN_009b4e08`。標本 4 未満のビンは既存値を平均比で拡大縮小)
  - 有効なビンを多項式で補間・外挿し、[0.2, 3.0] に制限する (`FUN_009bdf30`)。係数は RGB 各面の自チャンネル輝度に対するもので、段係数は 1.0 のまま
  - 入口に難読化された判定がある (逆アセンブル 0x9a7828〜0x9a79f0)。静的な 32 ビット語 w = `0x1488496` (ファイルの値 0x822068fe) と、
    `__common` (ゼロ初期化) にあるポインタ image+0x1566b80 (トークン表) を使う。表が NULL なら x = w、あれば x = −表[w & 0xffff] とし、
    (x + w の第 2 バイト)·2 + w の第 3 バイト = 0 のときだけ本体を実行する。この値では、表の 0x68fe 番目のバイトが 97 のときだけ成り立ち、
    NULL では成り立たない (`nvh info` の条件値 4366324414)。判定の前に、この語のアドレスを image+0x1569fb0 に書く。
    本体の中にも同じ型の判定 (語 `0x148849c`、表 image+0x15636b8、カウンタ `0x1488318`) があり、32px タイルの行数に (…)·0x40 を足す
    (正しい表なら 0 になるとみられる)。表を書くコードは特定していない (ホストによる初期化か認証の過程で作られると推定)。
    ハーネスでは表が NULL なので本体は実行されず 0 を返す。判定の回避・パッチはしていない。判定の前 (`0x9a775c〜0x9a7828`) にプロファイルへの書き込みはない (2026-10-17 に逆アセンブルで確認) ので、`nvh profile` のダンプは BuildNoiseModel 直後の状態
  - 2026-10-17 の静的な追加読み (sandbox/omo/wp_aft の作業者の解読を独立検証者と統括が逆アセンブルで確認。AutoFineTune は本家で実行していない):
    - 類似度 `FUN_009b6338` の R 語は語 490/491 (byte 0x7a8/0x7ac、0x9b64f4/0x9b655c)。許容幅は stats[1+i]・stats[17+i]・stats[34+8l+m]・stats[49+l]。定数 `DAT_012477dc` = (3, 2, 3)・`DAT_012477d0` = (4, 3, 4) (色モード 0/1/2)、66% は 0x3f28f5c3
    - worker `FUN_009b56f4` のゲートは G (面 1) → R → B の順で、類似度の積を 0.5 と比べる (最後の 0x9b60b0 だけ b.le)。親は色モード 0/1 で `FUN_009b4e08`×3 → `FUN_009bdf30`(0.2, 3.0, 0)×3、モード 2 で `FUN_009b5408`×3 を呼び、profile+0x2d = 0 は成否によらず書く (0x9a8004)
    - `FUN_009b4e08` はビン 0〜8 の 9 個すべてを処理する (0x9b5238 の `cmp x9,#0x100` は書き込みの後)。段係数の欄は `memset_pattern16` の模様 0xc962e0 = 1.0f×4 で埋める (0 ではない)
    - モード 2 のタイル順 `FUN_009d39ec` は LCG 風の順列 (初期値 0x7b) で、`nvh aftshuffle` と移植が計 194 件で一致
    - 類似度の CV 鎖は次の語が順序付きで負のときだけ打ち切る (NaN なら続ける。b.pl / b.mi)
    - 移植は `analysis/noise/aft_port/` (参考実装。本家と照合したのはタイル順だけ。自己試験は移植者の読みとの整合で、検出できない誤りが 3 種ある。README.md)。repro/ の v5 は使わない
- **EstimateNoiseLevelCurve** (`0x9b6c8c`、ワーカー `0x9b76b0`。FindFlatArea の正規化用) は、本家と全精度でビット一致する移植にした
  (`repro/nv_curve.py`・`nv_pixnoise.py`・`nv_lines.py`。数値は `exp_results.txt` の「EstimateNoiseLevelCurve exact port」以降の節)。
  - タイル: 96px (余白 12px、最小 48px) をスケジューラ (`FUN_00ac1194`) と同じ配置で並べる。16×16 ブロックごとに R か B の IsClip が 0.33 を超えれば重み 0 (G は見ない)
  - 輝度は BT.601 の fma の連鎖 (先頭 n&~3 画素は NEON、残りはスカラー)。画素ノイズ (`FUN_00998d98`) は 2 段 × 16 方向の線フィルタ:
    線カーネル (`FUN_00990448`、オフセット frintn(t·i+0.26f)) に沿って `FUN_009944c0` で半径 4、続けて 2 回目の平滑化。
    方向別エネルギー比をランプ 0.8→1.2・1.1→1.5 で重みにしてエッジを除く。モード 0 で対の 2 本目の線の除数が r..1 になる本家の不具合も再現した
  - 演算の癖も再現した: NEON の 4 画素単位の累積和、割り算は frecpe+frecps (`FUN_009ee04c`)、ランプは FMA、2×2 縮小は faddp の順、NaN・inf の扱い (命令単位で確認)
  - 集計: ビン = int(8/256·輝度)、画素数が (W·H)>>10 以上のビンだけ有効。無効なビンは 2〜3 次の多項式 (クラメルの公式) で補間・外挿し、[1,2,1]/4 で平滑化する。
    補間器 (`FUN_00aedb18`) は内側を線形、外側を端の 2 点の直線で外挿する
  - 照合: 本家 (`nvh curve`、%.17g) と 23 件・184 値すべてがビット一致した。独立検証者 2 人が新たに作った入力 494 件と 140 枚
    (4K・8K、48px、奇数サイズ、閾値ちょうどのビン、外挿の各分岐、NaN・inf・巨大値) もすべて一致した。
    部品も一致した: 線カーネル 184/184、線フィルタ 4544/4544、画素ノイズは全タイル、補間器は 2232 問い合わせ、タイル配置は 185 サイズ
  - 速度: 1080p 1 フレーム 0.67〜0.68 秒 (本家 0.44 秒。同じ大きさのタイルをまとめて numpy で処理。演算順は参照経路と同一で、
    `batch=False` が 11.7 秒の参照経路)。4096×2160 は 2.63〜2.64 秒。tracemalloc の最大は 1080p 224 MiB、4K 249 MiB、8K 239 MiB
    (タイルごとに足しては捨てる。入力の面をフレーム全体で複製しない)
  - 加算順: 実写と通常の合成画像では、ビンの double の和がどの順でも厳密になる (test_v02 n=100/312/576 で確認)。
    照合した範囲で加算順により ys が変わるのは、少数の巨大な値 (1e13〜1e20) を含む入力だけ。そうした入力での本家との一致は、移植の加算順
    (スケジューラ順のタイルごと、タイル内は行優先) と矛盾しないことを示す。順番待ちのタイルを足す順は本家との比較では区別できず、
    殻どうしの比較 (`verify_curve_shell.py`) で確かめる
  - test_v02 n=312 の曲線: 2.3839 / 2.5597 / 2.7485 / 2.8578 / 2.8228 / 2.5533 / 2.0492 / 1.4962 (全範囲輝度 16〜240)。
    v5 の `noise_curve_spatial` と軸・単位を揃えると比 1.04〜1.20 (端ほど本家が高い)。test.mp4 (n=0/300/450/750) では 0.73〜1.01 (v5 は暗部ほど高い)
- **実装**: `nvfilter.find_flat_area` を本家移植 (`find_flat_area_genuine` と `band_model`・`is_clip`・`_flatness` など) に置き換えた。
  中身は 2026-10-16 から `nv_flat`・`nv_band`・`nv_curve` (いずれも本家とビット一致)
  - 正規化曲線は本家と同一の移植 (`nv_curve`)。`curve="v5"` で以前の代用曲線 (v5 の単一フレーム曲線) も選べる
  - 本番の入口 (`find_flat_area`) 経由で本家と照合し、領域と状態コードが 20/20 で一致した (test_v02 8/8、test.mp4 7/7、
    合成 5/5 (黒帯・白飛び・RGB 有色ノイズを含む))。代用曲線では test.mp4 n=450 が逆転していた。48px の候補 2 つが僅差で並んでいたためで
    (曲線で割る前のスコア比は本家の帯域測度でも移植でも 0.99881)、本家の曲線では本家と同じ (400,288) を選ぶ
  - 状態コード (`FLAT_STATUS`: 0 良好・1 見つからず (代替の方法)・2 小さい・3 平坦でない・4 飽和・5 取り消し) を `prof["flat_status"]` に残し、
    CLI と UI に表示する (1/3/4 は CLI で警告)。64px 未満のフレームは本家同様に探索せず代替の方法を使う。同じフレームの再探索はキャッシュ (スレッド安全)
  - 取り消し: `find_flat_area(..., cancel=f)` は本家と同じ箇所で f() を問い合わせ、取り消しなら状態 5 (代替の方法は使わず、キャッシュもしない)。
    `build_profile_v5(..., cancel=f)` は状態 5 で `ProfileCancelled` を送出する (本家同様ノイズモデルを作らない)。UI は新しいプロファイル要求で古い要求を取り消す。CLI は取り消さない
  - 区間 A / B (Y): spatial は 47.01 → **47.11** / 47.07 → **47.19**。auto は 47.12 / 47.22 のまま。auto の処理は、静止画素が 10% 以上で
    単一フレームの曲線が全面で得られるとき領域に依存しない (曲線が得られない面は領域の σ に切り替わるので、その場合は依存する)
  - `build_profile_v5` に `matrix` 引数を追加した。CLI・UI・照合は `nv.matrix_from_tags` (タグ: bt709 / smpte170m・bt470bg / bt2020。
    なければ 720p 以上を BT.709) で決めた行列を渡し、本家と同じくホストが変換した RGB で探す (クリップ判定・曲線も同じ行列)

## 7. 制限・注意

- Pro バイナリ未取得。Demo と Pro の差分、本家レンダーカーネル定数の完全抽出は未了。
- 後方フローは順方向の否定近似ではなく方向別に推定している (v4 以降。v5 も中心→各近傍を個別推定)。
- 4K・長尺は未検証。1080p・721f で v4 は約40分 (bs32・並列なし)、v5 は 278 秒 (jobs 10)。
- v5 CLI は既定 (`--depth 8`) では 8bit 4:2:0 で入出力する (10bit 入力も 8bit に変換)。高ビット深度 (`--depth`、2026-10-26) の制限 (検証で観測):
  FFV1 の float は ffmpeg 8.0.1 で `-level 4 -strict experimental` が必要 (可逆は確認、他のツールでの読み込みは未確認。ほぼ全面 NaN のフレームでは ffmpeg 側が可逆に符号化できない例あり)。
  x264 10 ビットの `--crf 0` は可逆ではない。full range の高ビットの白 (2^b−1) は内部で 255.75 になる (limited range は 8 ビット ×4 で厳密に一致)。
  4:2:2 の色差は奇数列が補間なので σ が約 20% 低く出る。yuv ソースを `--depth 32` で読むと swscale の変換誤差 (BT.709 の定義式との差が最大 0.0038、0〜1 尺度) が入る。
  float の有限の巨大値は壊れる (1e9 以上で値が失われ、1e17 以上で出力の大半が NaN。+inf の置き換え先が巨大でも同様)。
  EXR 連番の後始末はパターンに合う既存の番号のフレームも消す。`vyu444` は 4:2:0 と誤判定する (まれな形式)。worker が落ちた場合 (BrokenProcessPool) は書きかけが残る。画素アスペクト比・字幕・データストリームはコピーしない
  (音声はトラックごとにコピー/AAC/除外。ストリームコピーの部分範囲は先頭が音声パケット 1 個分 (≤約20ms) 遅れて始まり得る。
  レビュー素材 (TS/PS/GOP 途中開始/音声先行/VFR/FLV/複数トラック) で A/V のずれは全トラック ±1.6ms 以内)。
  VFR 入力は r_frame_rate の CFR タイムラインに載せて処理・出力する (NLE と同様に隙間は重複フレームになり、
  重複フレーム同士も独立ノイズとして平均される点は近似)。奇数サイズは 1 行/列落とす。
- v5 の較正値は test_v02 の 1 本 (設定不明の本家出力) に合わせたもの。別素材の本家出力が手に入れば再較正する。
  `CHROMA_GAIN` は temporal モードの曲線にも掛かる (較正は auto モードで実施)。白色の色差ノイズの合成素材では色差をやや強く平滑化する。
- 平坦部が全くなく静止画素も 10% 未満の素材では、帯域形状が単一フレームの事前分布頼みになる (本家も「詳細のない領域で再プロファイル」と警告する状況)。
- 本家がノイズ除去に使う輝度別係数 (AutoFineTune) は、保護判定のためハーネスで実行できず、数値は未取得 (アルゴリズムは §6.5)。
  v5 の曲線と本家の EstimateNoiseLevelCurve の比は test_v02 で 1.04〜1.20、test.mp4 で 0.73〜1.01 (§6.5)。AutoFineTune の係数との一致は未確認 (静的移植の曲線を使う変種は区間 A / B で v5 と同等、§6.3)。
- `find_flat_area` は、曲線・IsClip・帯域測度・候補・スコアまで、1 スレッドで動かした本家と演算順までビット一致する。照合は本家が完了する入力に限る
  (48px 未満のフレームでは本家が落ちる。§6.5)。本家の照合は FindFlatArea を 1 プロセス 1 回だけ呼んで行った
  (2 回目以降は難読化された判定が縮小後の高さを変え得る。正規のホストでの値は未確認)。
  状態 5 (取り消し) は 1 スレッドの本家と問い合わせ回数まで一致させた。複数スレッドのホストではワーカーごとに問い合わせるので、回数は一致しない。
- BuildNoiseModel の移植 (`nv_model`・`nv_profile`) の制限: プロファイルの 0x00〜0x17 (ヒープのポインタ) は再現できない (圧縮が失敗したときは本家も 0)。
  LZMA の移植 (`nv_lzma`) は本家が使う設定 (Bt4・最適解析・1 スレッド・辞書 ≥ 入力長) だけで、1 バイトあたり約 10µs (128² の 3 面で約 1 秒)。
  本家の SDK の版は 1 つに絞れていない (本家と同じ出力になるのは 9.20〜18.05、18.06 以降は異なる。§6.5)。幅か高さが 32 未満の領域は本家がタイルの外を
  読むので移植は NotImplementedError (FindFlatArea が選ぶ領域は 48px 以上)。`FUN_00ad11b0` で条件を満たす方向ビンがない場合は本家がリストを初期化しないので
  NotImplementedError (1 辺 512px 以下では起きないことを検証者が幾何から確認)。1 スレッドの本家との一致。移植は本家より 1 面で約 21〜23 倍、プロファイルで約 17 倍遅い (2026-10-17 の高速化後の移植の時間を、高速化前に別の負荷で測った本家の時間 (128² 0.10 秒・320² 0.30 秒・プロファイル 0.91 秒) で割った目安。高速化前は 28〜33 倍。高速化は約 1.3 倍で、LZMA の純 Python 部 (約 10 µs/バイト) は変えていない)。
  1 行の面を `nvh model` に並列で渡すと本家が SIGBUS で落ちることがある (原因は不明。`FUN_00995250` 経由では `FUN_009f15b4` が 2 行読みにならないことを呼び出し元 0x99576c〜0x995770 の判定から静的に確認 (ほかの呼び出し元と負のオフセットの読みは未確認)。照合の対象から外した)。
- EstimateNoiseLevelCurve の移植がビット一致するのは、1 スレッドで動かした本家 (ハーネスの条件) に対してである。ホストが本家を複数スレッドで動かすと、
  ビンごとの double の部分和をスレッドの順に足すので、最下位ビットが異なり得る (実行のたびにも変わり得る)。
  ただし和が厳密になる入力 (test_v02 の 3 フレームで確認。実写では一般的とみられる) では順によらず同じになる。
  線カーネルのシフト量 `DAT_01561d58` は難読化コードが実行時に書く値で、移植は 0 と仮定している (カーネルのダンプは全件一致)。
  2026-10-22 に機構を解読 (coord/verify_protect/VERDICT.md): `__DATA __common` のゼロ初期化 (新規プロセスでは 0)。u64 読みは
  FUN_00990448 の 0x9904c8 のみ (u64 だが実質 shift 量で、浅い角度 flag=1 の count 格納 0x9907a0 で `lsl x15, x15, x22` に使う。
  急な角度は lsl なし)。書き手は 3 箇所: 0x45114 (BresenhamDrawer slot3、popcount−16 = 正常時 0) / 0x9af244 (FUN_009ae918 チャンク末尾、
  「(保証語 0x1561dec & 0x3fffff) == 1 かつ popcount(大域状態語 w22) ≠ 16」のとき 1) / 0x9ebbbc (FUN_009eacb0、GPU 経路の準備。
  0x1561df0 (16bit) == 1 のゲート + 別の状態語の 32bit popcount ≠ 16 のとき 1)。保証語 0 の新規プロセスでは書かれないので
  shift 0 の仮定はハーネス照合と整合する。shift ≠ 0 の実ホストでは浅い角度カーネルが idx テーブル (スタック 512 語) の
  未初期化部 [n..511] を読み得る (≤120px では start+2·count−1 ≤ 359 ≤ 511 で範囲内、SIGSEGV ではない)。中身はスタック履歴依存で
  決定的再現不可 (未定義動作)。下限试算 (残骸 0・面外無視) では test_v02 n=312 の暗部ビン (xs=16) が +3% 程度、明部は ≈1。
  トークン表の書き手 (2026-10-22 追加調査、lead_static_20261021.md §11): トークン欄 (0x1566B80 等、__common) への
  アクセスは「ROOT スロット (0x1488xxx、__data、rebase/bind 対象外) の**負のオフセット定数** + 32 ビット即値」の
  64 ビット加算で解決される (例: *(0x1488CE8) = −0x18B18000、+0x1A07EB80 → image+0x1566B80。watch-list の
  0x1569FB0 も同様に検算一致)。このバイナリの静的コードに表欄への str は存在しない (3 経路で全件検索、0 件)。
  書き手はホスト側コンポーネントか自己書き換え (静的解析の外)。書かれない場合 AFT 本体は cbz で代替パス →
  本体不成立 (既知の観測どおり)。追跡はここで打ち切り。
  1 辺 512px を超えるタイルは本家の表に収まらないので、移植は ValueError にする。
- 本家のプロファイルの色モード (profile+0x48) の初期値は 1 (YCrCb)。移植と照合の前提と同じ (2026-10-17 に逆アセンブルで確認: 初期化 `FUN_009b9e80` が
  `0x9b9efc add x0,x19,#0x48` → `bl 0x9ba3d0`、その先頭 `0x9ba3ec`/`0x9ba3f0` で 1 を書く)。.nfp の読み込みや UI がこの値を書き換える経路は未確認。
  モード 0 なら平坦度の判定対象 (3 面とも輝度扱い) が変わる。
- 移植は本家に渡る RGB を「色差を繰り返しで拡大し 0〜255 に収めた値」で模している。実際のホストの補間・クリップとの差は照合の範囲外。
- ハーネスは本家バイナリを実行する。2026-10-15、安全装置の導入前に本家のダイアログが 2 回画面に出た。現在は初期化子より前に差し替える (§5)。
- `/tmp` 下の成果物 (disasm・Ghidra プロジェクト・ビルド物) は揮発性。重要物は workspace 側に保存済み。

## 8. 共同作業ルール (複数エージェント共有)

- `nvfilter.py` の関数シグネチャ変更は `denoise_video_v2.py` / `nv_ui.py` /
  `validate_*.py` に波及する。変更後は `python3 -m py_compile` を通す。
- パイプラインの意味を変えたら (順序・既定値・式)、§4 対応表とベンチ数値も同時更新する。
  数値は `validate_*.py` / `blackbox/exp_results.txt` の実測のみ記載 (推測禁止)。
- 長時間ジョブ (実写処理は 1080p で約40分) はバックグラウンド実行し、
  `repro/out/` の既存成果物を上書きしない (別名保存)。
- 揮発物 (`/tmp/nv_expand`, `/tmp/ghproj`, `/tmp/nv_out*`) の再生成手順は §3 参照。
- 競合回避: 同一ファイルの同時編集を避ける。UI (`nv_ui.py`・`nv_ui_qt/`) と core (`nvfilter.py`)
  は担当を分け、結合部は `params()` / `process_frame()` の I/F とする。
