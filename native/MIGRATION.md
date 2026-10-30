Migration of the PySide6 UI (reference/python/nv_ui_qt) to a native binary - staged plan
=============================================================================

作成: 2026-10-26 (統括)。背景: 本家は C++ / Qt 5.15.2 / NEON float32 / 独自スレッドプールの
ネイティブ .ofx。repro はアルゴリズム確定済み (v5 + AFT + ノイズ推定は本家とビット一致の部品あり)。
出力品質はfloat32 の演算順が同じなら C++ でも同一になる (IEEE 754)。

方針: コア (演算) と UI を分けて段階的に移植する。各段階で「Python の実装とビット一致」を
自動検証してから次へ進む。Fast-math は禁止 (-ffp-contract=off、FMA 融合も禁止)。

段階
----
N0. 検証パイプラインの確立 (このリポジトリ、native/)
    軽い部品 (Quality 計算一式: genuine_g / flatness / penalty / size 項) を C++ へ移植し、
    Python 実装とビット一致することをランダム入力で検証する。ツールチェーン (clang++ arm64)
    と検証手順の証明が目的。

N1. コア部品の C++ 移植 (算術のみ、UI なし)
    順序 (依存が浅い順):
      1. nv_band.model_levels (5 段ラプラシアン帯域測度) - 本家とビット一致済みの移植があるので
         C++ 化して同じテストで検証
      2. nv_curve.genuine_noise_curve (EstimateNoiseLevelCurve) - 96px タイル、16 方向線フィルタ
      3. nv_flat.find_flat_area_genuine (Auto Profile の領域探索)
      4. nv_model.model_full / nv_profile.build_profile (方向フィルタ・LZMA)
      5. nvfilter v5 の temporal / spatial カーネル (a-trous + Wiener + 類似度)
      6. nv_aft_v5 / aft_port (AutoFineTune)
    各段階で (a) 合成入力のビット一致、(b) test_v02 の区間 A/B の PSNR が Python 版と同一、を確認。

N2. ネイティブアプリの梱包 (ユーザーが早く使える形)
    PyInstaller で repro UI を .app 化 (Python 旁边) - N1 の C++ 部品を ctypes から呼ぶ速度版に
    差し替えられるよう、コアの呼び出し界面 (process_frame / build_profile) は backend 経由で一本化済み。

N3. C++ Qt UI (Qt 6, LGPL)
    nv_ui_qt のページ構成 (Prepare Noise Profile / Adjust and Preview、本家配置 2026-10-25 版) を
    Qt C++ で再構築。ビューアの表示式は display.py 済み。処理は N1 の C++ コアを直呼び。

N4. OFX プラグイン化
    N3 の UI と N1 のコアを NeatVideo6.ofx 互換のバンドルにする。OFX のエクスポートは 3 関数
    (OfxGetPlugin / OfxGetNumberOfPlugins / FoundryOfxAllowPluginUnload - AGENTS.md §3)。
    Resolve での動作確認 (Demo と同じ 15 分/透かし制限はかけない)。

規約
----
- 浮動小数点: float32 明示、-ffp-contract=off、-ffast-math 禁止。numpy の演算 (加算順・Broadcast)
  と同じ順序を C++ のループで再現する。
- 検証: native/verify_*.py が乱数 + 境界ケースを生成し、Python と C++ の出力をビット比較する。
  一致は「同一ビット列」で判断する (誤差 1e-7 では合格にしない)。
- 保護区画: 本家バイナリの難読化部品を C++ に写すことはない (移植は repro 側の解読済み実装が源)。

現状
----
**N1 完了 (2026-10-28)。** N1a〜N1i の 9 部品 + 残件解決 F1〜F4 すべてが Python とビット一致。
`make -C native verify` (11 本) 全件一致、クリーンビルド警告 0。独立検証 9 件 APPROVE
(coord/native_n1/ と native_n1b/ と native_n1c/)。速度目安 (arm64 1 スレッド):
プロファイル 128² 17→1.3 秒、AFT 1080p 22〜37→0.5〜0.8 秒、曲線 1080p 0.49 秒 (4 スレッド 0.13)、
領域探索 0.81 秒 (F4 後に約 +3%)、process_frame_v5 全体 2.1→0.55 秒。LZMA は約 100 倍。

### F1〜F4 (2026-10-28 完了。REPORT は wp_native_{flat_cancel,model_ub,linekernel,portable}/、独立検証は coord/native_n1c/VERDICT_f1f4.md)

- **F1 取り消し**: `NV_FLAT_CANCEL_AT` 方式のホストで 676/676 (状態 5・問い合わせ回数・部分的な
  正方形も一致)。タスク文の「領域は書かない」は誤りで、**repro と本家の観測どおりそれまでの
  最良の正方形を返す**のが正 (AGENTS.md §6.5、独立検証者も妥当と判断)。
- **F2 未到達経路**: 両経路とも Python と同じ明示的エラーに統一 (追加 28/28 + 88 件)。
  本家では未定義動作 (<32px のタイル外読み・方向ビン不在のリスト未初期化) なので一致対象は repro。
- **F3 線カーネル共通化**: `native/nv_linekernel.{h,cpp}` に抜き出し。変更前後で全照合一致、
  変更前のバイナリともバイト一致。
- **F4 アーキ非依存**: `native/nv_portable.{h,cpp}` (NaN 優先の明示選択 + ソフト FRECPE/FRECPS) に
  統一。`make portable` (-DNV_PORTABLE) も Python と全件一致。**x86_64 (Rosetta 2) の出力は
  arm64 とバイト一致** (帯域・曲線・モデル・領域探索。nv_flat.cpp の 2 行が portable 経由から
  もれていたのを修正 — inf−inf の NaN 符号が x86 のみ食い違い、修正で解消)。arm64 の既定の
  速度は F4 前とほぼ同じ。
- **既知の範囲・前提**:
  - x86 照合済みは N1a/N1b/N1d/N1e。nv_ndimage (N1g/h) にはアーキ依存が残り、N1f/N1i の
    独自部分は x86 未照合。
  - N1f はエラー時に出力バッファへ途中書き込みが残る (状態 OK 以外ではバッファを読まない前提)。
  - N1i の源の Python 実装は本家との照合がタイル順のみ (C++ はその Python とビット一致)。
    色結合は Python 3.12+ の sum() の挙動を再現。
  - N1f/N1i/N1g/N1h の照合は numpy 2.1.3 / scipy 1.15.3 / Python 3.13 (arm64) が前提
    (更新時は照合やり直し)。

### N1 の履歴

N1 の委託分割: `.claude/tasks/native_n1_split.md` (N1a 帯域測度 / N1b ノイズ曲線 /
N1c LZMA (SDK 9.20 の C ソースを流用) / N1d 領域探索 / N1e モデル / N1f プロファイル /
N1g 時間カーネル / N1h 空間カーネル / N1i AFT。依存グラフと共通規約・検証形式つき)。

### N1 の進捗 (2026-10-26、N1a/b/c 完了。REPORT は work/omo/wp_native_*/、独立検証は coord/native_n1/)

- **N1a 帯域測度** `native/nv_band.{h,cpp}`: 5706/5706 ビット一致 (乱数 + 実写)。128² で約 22 倍。
- **N1b ノイズ曲線** `native/nv_curve.{h,cpp}`: 曲線 1094・IsClip 4000・補間器 20554・タイル 25、
  すべてビット一致。1080p 1 スレッド 0.49 秒 / 4 スレッド 0.13 秒 (Python 0.7 秒)。
  FRECPE/FRECPS の再現は必須 (真の除算で代用すると感度試験で 454 曲線中 92 が不一致)。
- **N1c LZMA** `native/nv_lzma.{h,cpp}` + `native/lzma/` (SDK 9.20 無改変、出典と SHA256 は lzma/SOURCE.md):
  949/949 バイト一致 (本家が保存した圧縮データ 258/258 を含む)。約 100 倍。
- `make -C native verify` で 4 本を通しで照合できる。
- **移植の重要な知見** (以降の N1d〜N1i も従う):
  - clang -O2 は fast-math なしでも可換演算の被演算子順を入れ替える。NaN のビット列が
    Python と変わるため、画素処理の演算は `native/nv_portable.h` (F4) 経由にする。AArch64 の既定は
    1 演算 1 命令のインラインアセンブラ、`-DNV_PORTABLE` と非 AArch64 は FPProcessNaNs・既定 NaN
    0x7fc00000・ソフトウェア FRECPE/FRECPS を明示的に書いた版 (旧 `-DNVC_PORTABLE` も別名で受ける)。
    N1a/b/d/e は x86_64 (Rosetta 2) の出力が arm64 とバイト一致する (`native/verify_x86.sh`)
  - x86 では素の C++ の 0/0・inf−inf・std::fma (libm の fmaf) が符号付きの既定 NaN 0xffc00000 になる。
    NaN が生じ得る 2 変数の演算は、std::fma も含めてすべて nvp 経由にする (F4 で 2 件見つかった)
  - numpy のスカラー演算は NaN の優先被演算子が演算と型で違う (add は配列=第 1/スカラー第 2 など)。
    Python 側の型を読んで C++ 側で明示的に選ぶ
  - libm の pow/tan は volatile 関数ポインタ経由 (定数畳み込み防止)
- **既知の差**: `repro/nv_lzma.py` は長さ 0 の入力で props が SDK と違う (C++ は SDK どおり。
  本家の経路は 32px 以上なので不発生。N1f の照合では長さ 0 を除外)。
### N1d/e/g/h (2026-10-27 完了。REPORT は work/omo/wp_native_{flat,model,spatial,temporal}/、独立検証は coord/native_n1b/VERDICT.md で 4 件 APPROVE)

- **N1d 領域探索** `native/nv_flat.{h,cpp}` (N1a・N1b を利用): 181 ケース × (-j 1, -j 3) = 362/362 ビット一致
  (実写 20 フレーム × 色モード 0..2、合成 81、乱数 40。縮小面・候補・trace・帯域測度の構造体まで比較)。
  1080p 0.81 秒 / 3 スレッド 0.34 秒 (Python 1.58 秒)。取り消し (状態 5) は未移植。→ F1 で移植 (下記)。
- **N1e 1 面モデル** `native/nv_model.{h,cpp}` (4096 語すべて、族 A/R/dc/B/C・FUN_00ad319c・FUN_00ad1ae0): 1244/1244 ビット一致。
  本家の nvh model ダンプ (キャッシュ) とも 753/753 一致。8〜13 倍 (スカラー asm、未ベクトル化)。
  線カーネルは nv_curve.cpp と重複 (共通化は未了)。非 AArch64 は未対応 (FRECPE のソフト版なし)。→ どちらも F3/F4 で解消 (下記)。
- **N1h 空間カーネル** `native/nv_spatial.{h,cpp}` + 共通部品 `native/nv_ndimage.{h,cpp}`: 1287/1287 ビット一致。1080p 約 1.7 倍。
- **N1g 時間カーネル** `native/nv_temporal.{h,cpp}` (動き推定・dense_flow・warp・類似度 v5/pyramid・rv、process_frame_v5 全体も):
  474/474 ビット一致。1080p の process_frame_v5 2.1 秒 → 0.55 秒 (1 スレッド)。
- **照合の前提 (N1g/N1h)**: 期待値は scipy.ndimage / numpy の演算順に依存し、照合はこの環境 (numpy 2.1.3・scipy 1.15.3・arm64) に固定。
  scipy の gaussian_filter と np.interp は FMA を使い、numpy の加算リダクションは形状で順が変わる (1 列幅・nbx=1 の特例)。
  numpy/scipy を更新したら照合をやり直す。
- **API の前提**: パラメータは Python の float と同じく演算前に float32 にする (NEP 50。np.float64 を混ぜると Python 側が倍精度になる)。
  pyramid は Y の帯域が 3 段以上を前提 (Python は 3 段未満で IndexError、C++ は 0 埋め)。
- `make -C native verify` で 8 本 (N0 + N1a〜e・g・h) を通しで照合できる (2026-10-27: クリーンビルド警告 0、全件一致)。
- 残件 (取り消し未移植・N1e の未到達経路・線カーネル重複・非 AArch64) は F1〜F4 で解決 (下記)。

### N1f/N1i と F1〜F4 (2026-10-28 完了。REPORT は work/omo/wp_native_{profile,aft,flat_cancel,model_ub,linekernel,portable}/、独立検証は coord/native_n1c/VERDICT_{n1f,n1i,f1f4}.md で 6 件 APPROVE)

- **N1f プロファイル** `native/nv_profile.{h,cpp}` (N1a〜N1e を利用): 39/39 のプロファイルで 0xc300 バイト全部がビット一致
  (実写 21・合成 18。ヒープポインタ 0x00..0x17 は双方 0)。独立検証者の新しい入力 33/33 + エラー状態 6/6。
  reference_floor は `data/ref_floor.npy` を読む (計算版 `nvp::compute_reference_floor` も npy と 51/51 一致)。
  128² で 1 スレッド 1.27 秒 / 3 スレッド 0.45 秒 (Python 17 秒)。時間のほぼすべてが N1e の model_full。
  状態が OK 以外のとき出力バッファは途中まで書かれている (読まないこと)。
- **N1i AutoFineTune** `native/nv_aft.{h,cpp}` (aft_port の nv_aft.py + nv_aft_v5.apply_aft の結合部): タイル順 4097・類似度 3000・
  後処理 3000・AFT 67 (-j 1/-j 3)・結合 69、すべてビット一致。独立検証者の新しい入力 16 + 追加 4 + 結合 204 + タイル順 41。
  1080p で 1 スレッド 0.5〜0.8 秒 (Python 22〜37 秒)。正は aft_port の Python (本家と照合したのはタイル順だけ)。
  結合部は CPython 3.12 以降の `sum()` の Neumaier 補償加算を再現している (Python を下げると期待値が変わる)。
- **F1 取り消し**: `nvf::find_flat_area_genuine(..., diag, cancel)` と `nvc::genuine_noise_curve(..., threads, cancel)`
  (`nvc::Cancel = std::function<bool()>`)。問い合わせは曲線がタイル数 + 1 回、各サイズの先頭に 1 回で、複数スレッドでも回数は 1 スレッドと同じ。
  サイズ段の取り消しはそれまでの最良の正方形 (切り詰め前) を返す (repro どおり。タスク文の「書かない」とは異なり、本家の観測とは整合)。
  `verify_flat_cancel.py` 676/676。
- **F2**: nv_model_bm の 2 経路 (pad ≥ 辺の端複製・条件を満たす方向ビンなし) は model_full から到達しない (ad1ae0 の辺 ≥ 84。幾何の全数検査)。
  pad ≥ 辺は本家では定義済みの分岐 (未移植)、方向ビンなしは本家ではリスト未初期化。C++ は両方 NOT_IMPLEMENTED で Python の NotImplementedError と一致
  (`nvm::bm_quantiles` の直接照合 28/28、verify_model.py の ub_checks)。
- **F3**: 線カーネルを `native/nv_linekernel.{h,cpp}` に一本化 (挙動不変、全照合一致)。
- **F4**: `native/nv_portable.{h,cpp}` に浮動小数点演算を集約 (N1a/b/d/e)。arm64 の portable 版も Python と全件一致、
  x86_64 は arm64 とバイト一致 (band・curve・model・flat・flat_cancel の全コマンド)。asm 経路の速さは不変 (flat は +3%)、
  portable は curve で約 3.6 倍遅い。残るアーキ依存: nv_ndimage (N1g/N1h) の asm とその非 AArch64 代替 (NaN の規則なし)、libm の expf。
  N1f・N1i は N1a/b/e と同じヘルパを使う部分のほかに独自の asm (numpy スカラーの NaN 優先) を持ち、x86 は未照合。
- **照合の前提 (再掲)**: N1g/N1h/N1f/N1i の期待値は numpy 2.1.3・scipy 1.15.3・Python 3.13 (arm64) の演算順に依存する。
  これは検証ハーネス側の性質で、C++ の移植の性質ではない。numpy/scipy/Python を更新したら照合をやり直す。
- `make -C native verify` で 11 本 (N0 + N1a〜i + F1) を通しで照合できる。x86 との比較は `make -C native portable x86 && native/verify_x86.sh`
  (Rosetta 2 で数時間かかる)。
### N2 完了 (2026-10-28。REPORT は wp_native_bridge/ と wp_native_app/)

- **N2a ctypes 速度ブリッジ**: `native/nvcore_c.{h,cpp}` → `libnvcore.dylib`、
  `repro/nv_native.py` (ctypes ラッパ、dylib なしで Python に自動フォールバック)、
  backend に Preferences「native core」フラグ (既定 ON)。verify_bridge: curve 20/20・
  profile 9/9・AFT 26/26 (999,732 語)・frame 72/72 (306,334,381 語)・cancel 23/23・
  fallback 9/9、**全語ビット一致**。自己試験 96/96 (ON/OFF)、書き出しバイト一致。
  速度 (1080p 1 スレッド): AFT 46.8→2.05 秒 (4 スレッド 0.61)、フレーム 2.03→0.57 秒、
  プロファイル構築 2.38→1.70 秒 (C 化は領域探索のみ。残りは N1 部品がまだ無い)。
  N1 コードの変更: AFT に取り消し問い合わせを追加、空間段から面ごとの関数を抽出
  (11 本で回帰なし)。
- **N2b PyInstaller .app**: `packaging/` (spec・build_app.sh・test_app.sh・ランチャ)。
  arm64・187 MB・最低 macOS 26 (同梱 PySide6 6.12 に合わせる)。ffmpeg は同梱せず
  NV_FFMPEG → prefs/ffmpeg → PATH の順で探索。test_app.sh: バンドル内自己試験 96/96
  (NV_NATIVE=0 でも 96/96)、書き出しバイト一致 (10 ビット mkv は復号フレーム一致、
  UID+CRC 44 バイトのみ差)、バンドル無変更。起動 1.0〜1.1 秒。
  nv_ui.py と v1〜v4 の旧スクリプトは同梱しない。
- **既知の範囲**: AFT の取り消しは前段のプロファイル構築の間 (128² で 0.5〜1.6 秒) 効かない。
  x86_64 の dylib は未作成。verify_bridge は動画・ffmpeg・PySide6 が必要なため make verify には
  入れず別ターゲット (`make -C native lib` 後に `python3 native/verify_bridge.py`)。
- **アプリ名**: 2026-10-29 に **MeatVideo** に改名 (bundle・launcher・scripts・docs)。
  ウィンドウ内の題字は reproduction UI のまま。内部モジュール名 (nv_*) は不変。
### N3 完了 + W2 判定 (2026-10-29)

- **N3 完了**: `qtui/` (CMake Qt 6 アプリ) — Prepare/Adjust 両ページ・ビューア・選択範囲・
  パネル (Quality/Completeness 含む)・フィルタ設定・プレビュー/サムネイル・書き出し・
  Preferences・Profile Viewer/Check・Variants・OFX ブリッジモード。**処理は libnvcore の
  C ABI 直呼びのみ** (UI に numpy を持ち込まない)。`nvcore_c_{ui,depth,prof5}` +
  `nv_depth/nv_prof5/nv_lstsq/nv_ui` を追加。nv_lstsq は np.linalg の OpenBLAS 0.3.29 の
  演算順を再現。PyObj (順序付き JSON、Python の意味論) でプロファイル dict を扱う。
  自己試験 105/105 (Qt Test)。書き出し 23/23 バイト一致。L-1 (OFX の 2^63/2^64 フレーム
  オーバーフロー) を修正 — 0.44 秒/226 MB で終了し Python 版と同じ終わり方
  (修正前は数秒で 10 GB)。マージ後の main ツリーで clean ビルド警告 0・
  make verify 11 本全件一致・verify_bridge 全語一致・自己試験 105/105 を統括が再確認
  (自己試験は `NV_TEST_VIDEO=<動画>` または qtui から見た ../test.mp4 が必要)。
- **N3 の既知差 (記録のみ)**: L-2 OFX 引数の端値 (1e400・Unicode 数字・0x10) の解釈が
  Python 版と違う / L-3 C++ 自己試験 3 項目が弱く項目 20 が未実装 (Python との一致は
  比較スクリプトで確認) / I-5 自己試験の単項目実行は前項の動画読み込み前提で落ちる /
  I-6 極端なサイズのエラー文が違う (traceback vs no frames) / nv_depth の x86 未照合 /
  実画面確認は未実施 (offscreen のみ。手順は REPORT)。
- **W2 判定: W2c (文書化) で確定**。W2a のハーネス
  (`native/libm_alt.{h,cpp}` + openlibm/musl 同梱、NV_LIBM_ALT で切替、
  `verify_libm_alt.py`) が測定基盤として残る。libm の ULP 差は本家自身の
  プラットフォーム別バイナリと同じ性質であり、macOS libm 基準を文書化する。
  numpy/scipy/Python/OpenBLAS の版を上げたら照合をやり直す
  (nv_lstsq は OpenBLAS 0.3.29 の演算順に固定)。
- **N2/N3 運用決定 (5 問題への回答)**: (1) 設定ファイルは Python 版と共有
  (C++ 固有の 2 キーを追加、既存キーは削除しない)。(2) polyfit は OpenBLAS 0.3.29 固定。
  (3) W2 は W2c。(4) 1080p 比較用の生画像は検証のたびに再生成 (コミットしない)。
  (5) 書き出し CLI の UI 設定依存は当面許容 (N4 でスタンドアロン化)。

### C++ ファースト方針 (2026-10-30、ユーザー決定)

本プロジェクトの成果物は C++ (本家と同じ形) に一本化する。Python (repro/) は
**参照実装・検証オラクル**として維持するが、以後の機能追加・修正・最適化は C++ 側のみ行う。
- 両 UI 同期の義務は廃止: Qt C++ UI (qtui/) が公式の UI。PySide6 版 (nv_ui_qt/) は
  参照・照合用に残す (動かし続ける義務はない。壊れても修正しない)。
- 検証基準: (1) C++ の決定論 (同一入力 → 同一出力)、(2) 既存のビット一致スイートの維持
  (gold は Python 由来の固定フィクスチャとして使う。numpy/scipy 版固定の前提は同じ)、
  (3) 本家の観測挙動との整合。
- Python 依存の解消が作業の中心になる: 書き出し CLI の UI 設定依存の解消 (N4 で予定済み)、
  G8 済みのプロファイル構築 C 化を全 UI 経路に接続、Python なしで完結する .app/.exe。

### N3 fixes2 完了 (2026-10-30、PR #1/#2/#3 をマージ。独立検証 3 件 APPROVE)

- **G3 (L-2 解消)**: `qtui/src/core/pynum.{h,cpp}` — OFX 引数の数値解析を CPython 3.13 の
  int()/float() に準拠 (0x10 は ValueError、1e400 は inf、Unicode Nd 数字 670 文字対応)。
  edge 16/16・extra 20/20・size 14/14。traceback の 5 フレームまで一致。
- **G4/G5 (L-3・I-5 解消)**: C++ 自己試験の弱い 3 項目を Python 版同等に強化、項目 20 を追加、
  動画を要する項目を自己完結化 (`qtui/tools/selftest_each.py` で単項目実行を確認)。
  自己試験は 105 件 → 110 項目。
- **G6 (I-6 解消)**: 異常終了時に Python 版と同じ形の traceback と最終行を出力。
  極端サイズ 4810 ジョブが一致。
- **G2 (E3 解消)**: 両 UI の Preferences に ffmpeg 欄を追加
  (`prefs/ffmpeg`。探索順 NV_FFMPEG → prefs → PATH)。新規 `reference/python/nv_ui_qt/ffmpeg_path.py`。
  Python 自己試験は 96 → 97 項目。
- **G7 (E1 解消)**: `nvc_profile_build_cancel` (戻り値 3 = 取り消し) を追加。
  取り消しは構築の全段で効く。verify_bridge の cancel は 23 → 49 ケース。
- **G8 (E2 解消)**: `build_profile_v5` の UI 出力全部 (曲線・band_ratio・tot_ratio・
  sigma・levels・flat_status) を C (nv_prof5) で計算、Python とビット一致。
  `nvn::b3` を NEON 化。`NVC_ABI_VERSION` を 2 に更新。
  verify_bridge に g8 152/152 (1px 辺を含む) を追加。
- **G9 (x86 検証)**: 記録/再生ブリッジ (`native/x86cap/`・`verify_x86_bridge.sh`) で
  arm64 3577/3577・portable 3577/3577・x86 3546/3577 (差 31 件は nvc_temporal で
  W2c 既知の差のみ)。nv_depth も x86 で照合済み。nv_ndimage の asm 部分のみ
  「x86 未保証」が残る。
- **残った記録**: Qt C++ の aftCancellable 試験は弱い (マージ後の強化候補)。
  取り消し ABI は int(*)(void) 型。1×1 の手動領域で Python と結果が違う
  (UI からは作れない)。Python 自己試験の終了時まれな停止は別タスク。

### G1 + G9 + スタイル修正マージ完了 (2026-10-31、PR #5/#6 マージ。独立検証 Round 4/5・Round 2 APPROVE)

- **G1 (R/G/B イコライザ、C++ ファースト版)**: qtui のイコライザを本家どおり R/G/B の
  9 ビンに。Y/Cb/Cr 曲線は `nvcore_c_rgb` の色行列結合 (`nvc_rgb_weights` /
  `nvc_rgb_effective_curve` / `nvc_rgb_combine`。nva::combine を color_weights と
  combine_levels に分離。nvc_aft_combine の動作は不変 = 結合 69/69) で作る。
  新形式 `nv_repro_profile_rgb` v2・Reset Fine-Tuning は AFT 前の曲線に戻る。
  旧形式 (Y/Cb/Cr) の JSON は**従来の意味のまま読む** (統括決定。71 件の互換確認済み)。
  `NVC_ABI_VERSION = 3` (main 取り込み後、nv_native.py も 3)。自己試験に
  g1EditMatchesAftCombine (4 種編集 × 曲線/sigma/フレームハッシュをビット比較、
  1 ulp 変異で FAIL) を追加。
- **G9 (x86 検証)**: 記録/再生ブリッジ (`native/x86cap/`・`x86_bridge_{cap,replay,diff,known}.py`・
  `verify_x86_bridge.sh`・`x86_exe_pair.py`) で arm64 3577/3577・portable 3577/3577・
  x86 3546/3577 (差 31 件は nvc_temporal で W2c 既知の expf 差のみ)。
  nv_depth も照合済み。nv_ndimage の asm 部分のみ「x86 未保証」が残る。
- **スタイル修正 (PR #6)**: qtui がスタイル未指定で macOS ネイティブ表示になっていたのを、
  `qtui/src/app/look.{h,cpp}` の `apply_look()` で Fusion ベース + 本家の灰色パレットに統一
  (中央タブ・ボタン内右下の ▾・選択欄の ▾ 1 本化・プロファイル名欄の ▼・
  ウィンドウ内メニューバー + 右端の「Advanced Mode」)。イコライザ行の n/a を
  Completeness (g×100) 表示に変更 (自己試験 equalizerCompleteness 追加)。
  フレーム情報に "32-bit float" 表示を追加。文字コントラスト比 1.1〜1.5 → 2.4〜3.4。
  処理・書き出しは不変 (g1_compare ALL SAME・compare_export ALL SAME)。
- **統括の実画面確認**: マージ後の main を実ビルドして実画面で起動、
  ウィンドウ内メニュー・中央タブ・Advanced Mode・灰色パレット・R/G/B イコライザを確認
  (オブジェクト `native/nv_portable` などはすべて libnvcore に統合済み)。
- **MIGRATION.md の訂正**: ABI 版数は 2 ではなく **3** (PR #5 が main を取り込んで更新。
  repro/nv_native.py も ABI_VERSION = 3)。
  Python Preferences に Auto Fine-Tune 行を追加して両 UI の行を揃えた。
- **Windows**: `.claude/tasks/native_windows_runbook.md` (Windows ホストの Claude が実行中) と
  native_windows.md (W1〜W5)。

### Windows 対応 (計画 2026-10-28。詳細と委託タスクは .claude/tasks/native_windows.md)

基準点の整理: 「Python とビット一致」の基準点は arm64 macOS の numpy/scipy/Python の環境。
Windows の numpy/scipy は演算順が違うため、Windows numpy との一致は目標にしない。
Windows の目標は (1) C++ のクロスプラットフォーム決定論 (同一入力 → macOS と Windows で
同じ出力。F4 の手法の延長) と (2) libm 差異 (pow/tanf/atan2/sincos) の解消か文書化。
タスク: W1 CMake 化 + CI (N2 と並行可) → W2 libm 対策 → W3 Windows 検証 → W4 .exe 梱包 →
W5 Windows OFX (N3 の後)。
