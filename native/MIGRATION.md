Migration of the PySide6 UI (repro/nv_ui_qt) to a native binary - staged plan
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
- N0 完了: native/nv_quality.{h,cpp} (Quality 一式: genuine_g / flatness / penalty / size 項) +
  native/verify_quality.py (乱数・境界・NaN の 4000〜20000 ブロック)。
  `make -C native && python3 native/verify_quality.py [N]` で Python と**ビット一致**を確認
  (2026-10-26: 20000/20000)。移植時に見つかった落とし穴を MIGRATION.md の規約に反映済み:
  float 定数は必ず Python 側のビットパターンから持ってくる (手計算の 16 進は誤る: −0.25 は
  0xBE800000)、fmaxnm の NaN 経路は C の比較では書き足さない (isnan を明示)、
  most vexing parse に注意。

N1 の委託分割: `.claude/tasks/native_n1_split.md` (N1a 帯域測度 / N1b ノイズ曲線 /
N1c LZMA (SDK 9.20 の C ソースを流用) / N1d 領域探索 / N1e モデル / N1f プロファイル /
N1g 時間カーネル / N1h 空間カーネル / N1i AFT。依存グラフと共通規約・検証形式つき)。

### N1 の進捗 (2026-10-26、N1a/b/c 完了。REPORT は sandbox/omo/wp_native_*/、独立検証は coord/native_n1/)

- **N1a 帯域測度** `native/nv_band.{h,cpp}`: 5706/5706 ビット一致 (乱数 + 実写)。128² で約 22 倍。
- **N1b ノイズ曲線** `native/nv_curve.{h,cpp}`: 曲線 1094・IsClip 4000・補間器 20554・タイル 25、
  すべてビット一致。1080p 1 スレッド 0.49 秒 / 4 スレッド 0.13 秒 (Python 0.7 秒)。
  FRECPE/FRECPS の再現は必須 (真の除算で代用すると感度試験で 454 曲線中 92 が不一致)。
- **N1c LZMA** `native/nv_lzma.{h,cpp}` + `native/lzma/` (SDK 9.20 無改変、出典と SHA256 は lzma/SOURCE.md):
  949/949 バイト一致 (本家が保存した圧縮データ 258/258 を含む)。約 100 倍。
- `make -C native verify` で 4 本を通しで照合できる。
- **移植の重要な知見** (以降の N1d〜N1i も従う):
  - clang -O2 は fast-math なしでも可換演算の被演算子順を入れ替える。NaN のビット列が
    Python と変わるため、N1a/N1b は画素処理の加減乗算をインラインアセンブラで順固定
    (AArch64 前提。x86 では NaN を含む入力のビット一致は保証しない。`-DNVC_PORTABLE` で
    ソフトウェア版に落ちる経路あり)
  - numpy のスカラー演算は NaN の優先被演算子が演算と型で違う (add は配列=第 1/スカラー第 2 など)。
    Python 側の型を読んで C++ 側で明示的に選ぶ
  - libm の pow/tan は volatile 関数ポインタ経由 (定数畳み込み防止)
- **既知の差**: `repro/nv_lzma.py` は長さ 0 の入力で props が SDK と違う (C++ は SDK どおり。
  本家の経路は 32px 以上なので不発生。N1f の照合では長さ 0 を除外)。
### N1d/e/g/h (2026-10-27 完了。REPORT は sandbox/omo/wp_native_{flat,model,spatial,temporal}/、独立検証は coord/native_n1b/VERDICT.md で 4 件 APPROVE)

- **N1d 領域探索** `native/nv_flat.{h,cpp}` (N1a・N1b を利用): 181 ケース × (-j 1, -j 3) = 362/362 ビット一致
  (実写 20 フレーム × 色モード 0..2、合成 81、乱数 40。縮小面・候補・trace・帯域測度の構造体まで比較)。
  1080p 0.81 秒 / 3 スレッド 0.34 秒 (Python 1.58 秒)。取り消し (状態 5) は未移植。
- **N1e 1 面モデル** `native/nv_model.{h,cpp}` (4096 語すべて、族 A/R/dc/B/C・FUN_00ad319c・FUN_00ad1ae0): 1244/1244 ビット一致。
  本家の nvh model ダンプ (キャッシュ) とも 753/753 一致。8〜13 倍 (スカラー asm、未ベクトル化)。
  線カーネルは nv_curve.cpp と重複 (共通化は未了)。非 AArch64 は未対応 (FRECPE のソフト版なし)。
- **N1h 空間カーネル** `native/nv_spatial.{h,cpp}` + 共通部品 `native/nv_ndimage.{h,cpp}`: 1287/1287 ビット一致。1080p 約 1.7 倍。
- **N1g 時間カーネル** `native/nv_temporal.{h,cpp}` (動き推定・dense_flow・warp・類似度 v5/pyramid・rv、process_frame_v5 全体も):
  474/474 ビット一致。1080p の process_frame_v5 2.1 秒 → 0.55 秒 (1 スレッド)。
- **照合の前提 (N1g/N1h)**: 期待値は scipy.ndimage / numpy の演算順に依存し、照合はこの環境 (numpy 2.1.3・scipy 1.15.3・arm64) に固定。
  scipy の gaussian_filter と np.interp は FMA を使い、numpy の加算リダクションは形状で順が変わる (1 列幅・nbx=1 の特例)。
  numpy/scipy を更新したら照合をやり直す。
- **API の前提**: パラメータは Python の float と同じく演算前に float32 にする (NEP 50。np.float64 を混ぜると Python 側が倍精度になる)。
  pyramid は Y の帯域が 3 段以上を前提 (Python は 3 段未満で IndexError、C++ は 0 埋め)。
- `make -C native verify` で 8 本 (N0 + N1a〜e・g・h) を通しで照合できる (2026-10-27: クリーンビルド警告 0、全件一致)。
- **次**: N1f (プロファイル: N1c・N1d・N1e 済み) と N1i (AFT: N1a・N1e 済み) が解放済み。
