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

次 (N1): nv_band.model_levels の C++ 移植とビット一致検証 (同様の verify_*.py 形式)。

N1 の委託分割: `.claude/tasks/native_n1_split.md` (N1a 帯域測度 / N1b ノイズ曲線 /
N1c LZMA (SDK 9.20 の C ソースを流用) / N1d 領域探索 / N1e モデル / N1f プロファイル /
N1g 時間カーネル / N1h 空間カーネル / N1i AFT。依存グラフと共通規約・検証形式つき。
並列開始は N1a・N1b・N1c)。統括が各ワーカーの REPORT を受領後に verify 再実行・マージ・コミット。
