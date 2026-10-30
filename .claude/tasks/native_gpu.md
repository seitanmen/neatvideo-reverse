# GPU 対応タスク (Metal → Windows) — ビット一致を壊さない GPU 化

作成: 2026-10-31 (統括)。目標: **出力を 1 ビットも変えずに** 処理の重い段を GPU に移す。
macOS は Metal、Windows は HLSL/DirectCompute (または最初は CPU 専用のまま)。

## 設計の柱 (重要)

1. **fp64 は Metal に存在しない** (Apple GPU)。double 累積を使う段
   (プロファイル構築の帯域統計、曲線ビンの double 和) は **GPU 化しない** (CPU のまま)。
   これらはフレーム 1 回・軽量なので速度への影響は小さい。
2. **超越関数 (pow/expf/tanf/atan2/sincos) は GPU で使わない**。
   表として事前計算できるもの (線カーネル表・gaussian 係数・補間器の節) は
   CPU で計算してバッファで渡す。そうすれば GPU カーネルは
   加算・減算・乗算・clamp・整数演算だけになる。
3. **除算・平方根は「定義済みアルゴリズム」で実装する**:
   ARM の FRECPE+FRECPS (除算) / VRSQRTE+VRSQRTS (平方根) の
   精度改良シーケンスは決定論的。N1 のポータブル版 (frecpe32/frecps32 など) と
   **同じアルゴリズムを Metal/HLSL で実装すれば、CPU と GPU でビット一致する**
   (乘算と減算だけの組み合わせであり、GPU ベンダー差が出ない)。
4. **整数演算 (SAD など) は順序自由** (整数加算は結合的で厳密)。GPU で安全。
5. 合格判定は今までどおり**ビット一致** (CPU 経路との比較)。
   「GPU なので誤差許容」にはしない。

## GPU-0: 実現可能性プローブ + 段ごとの適用性監査

1. `native/gpu/probe.{h,metal}`: Metal シェーダで次を試験し、CPU の結果とビット比較:
   - fp32 の加算・減算・乗算 (IEEE 完全一致か)
   - fp32 の除算・平方根 (正しく丸められるか。されない場合は 3 の手法を使う)
   - **非正規数 (denormal) が 0 にフラッシュされるか**
   - **NaN のペイロードが保持されるか** (Metal は正規化する可能性がある。
     保持されない場合、NaN を含む入力のビット一致は諦めて
     「NaN 入力は CPU 側で事前処理して GPU に渡さない」設計にする)
   - FMA 融合の強制有無 (Metal の `fma()` は明示呼び出しのみで、
     `a*b+c` が勝手に融合されないことを確認)
2. 段ごとの監査表を REPORT に作る:
   | 段 | 演算 | fp64 | 超越関数 | GPU 適用 |
   |---|---|---|---|---|
   | a-trous 分解 | 加減算のみ | なし | なし | **適用可** |
   | Wiener (分散・除算) | 乗算・除算・sqrt | なし | なし | 適用可 (定義済み div/sqrt) |
   | temporal 類似度 | 加減乗・clamp | なし | なし | 適用可 |
   | motion SAD | 整数 absdiff + 加算 | なし | なし | 適用可 (整数は順序自由) |
   | 曲線ビン集計 | **double 加算** | **あり** | なし | CPU のまま |
   | プロファイル構築 (band 統計) | **double** | あり | なし | CPU のまま |
   | 動きのサブ画素重心 | 加算・除算 | なし | なし | 適用可 (定義済み div) |
3. **成果物**: `native/gpu/probe/`、監査表 (REPORT)、
   「GPU 化できる段 / できない段」の最終リスト。

## GPU-1: 定義済み演算プリミティブ (Metal)

- frecpe/frecps・rsqrte/rsqrts のソフトウェアアルゴリズムを Metal 関数として実装
  (`native/gpu/defined_ops.metal`)。
- 検証: `native/verify_gpu_ops.py` — 乱数・境界・非正規・巨大・微小 100 万組で
  CPU (nv_portable のソフト版) と Metal をビット比較。不一致 0 が合格。

## GPU-2: 空間段 (spatial_v5) の Metal 化

- a-trous 5 帯域・局所 RMS・Wiener を Metal カーネル化
  (`native/gpu/spatial.metal`)。
- カーネルごとに CPU とビット比較 (Level/ステップ単位) →
  フレーム全体でビット比較 → 速度を測る (目標: 空間段を 0.57 秒のかなり下へ)。
- 端の扱い (0 詰め/複製) とステップ幅のパラメータを CPU 実装と完全に揃える。

## GPU-3: 時間段 + 動き (SAD) の Metal 化

- 類似度・画素重み・残留分散の算出を Metal 化。
- SAD は整数 absdiff の領域合計 (GPU の threadgroup reduction でよい。
  整数は順序自由)。
- 検証: verify_temporal 相当の全ケースを Metal で再実行してビット比較。

## GPU-4: ハイブリッド制御 + フレーム全体ビット一致

- CPU↔GPU のバッファ転送を最小化する構成 (フレームバッファは
  最初のフィルタ呼び出しで GPU に置きっぱなしにする)。
- フレーム全体 (temporal + spatial) を GPU 経路で処理し、
  CPU 経路とビット比較。速度を測る (目標: 1080p 1 フレーム 0.55 秒 → **0.2 秒以下**)。
- `native/verify_gpu_bridge.py` で N2a の verify_bridge と同じ形式で検証。

## GPU-5: Windows (HLSL) — 最後

- Metal カーネルを HLSL compute shader に移植 (同じ定義済み演算)。
- **同一入力 → Windows GPU 出力 vs macOS 出力のバイト比較** (CPU を介さない)。
- ベンダー差 (NVIDIA/AMD/Intel) の非正規・NaN 挙動は GPU-0 のプローブを
  Windows でも再実行して確認する。**一致しない場合は、Windows の GPU は
  「速度優先モード (出力が変わる・既定 OFF)」として分離し、
  ビット一致モードは CPU のまま**にする (文書化して提供)。

## 共通

- Metal シェーダのコンパイルは実行時 (ライブラリとして .metallib を同梱)。
- スナップショット・MIGRATION.md・AGENTS.md は統括が更新。
- コミットしない。REPORT: `sandbox/omo/wp_gpu/<段階>/REPORT.md`
- **N4 (OFX) と同じ native/ を触るため、N4 のマージ後にベースを main に
  変え直してから作業を開始すること。**

## 貼り付け用プロンプト

```
1. .claude/tasks/native_gpu.md の「GPU-0」を実行してください
   (プローブ + 監査表)。GPU-1 以降は GPU-0 の結果を見てから開始します。
```
(以降: 「GPU-1」「GPU-2」…を順に。各段階で REPORT を出し、統括が検証・マージ。)
