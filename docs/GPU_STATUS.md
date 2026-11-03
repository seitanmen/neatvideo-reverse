# GPU 対応状況 (2026-10-31)

## 1. 本家の GPU 実装 (バイナリ解析から)

**リンクしている GPU フレームワーク (arm64 スライス)**:
- `Metal.framework` (32 か所の文字列) — macOS の主要 GPU API
- `OpenCL.framework` (4 か所)
- CUDA の文字列 3 件・Vulkan 1 件 (Windows 版の経路とみられる。arm64 macOS では未使用)

**RTTI で確認できる GPU クラス群**:
- `GPUProductsBridgeImpl / GPUProductsBridgeIntf` — GPU 製品 (デバイス) の列挙ブリッジ
- `GPUApiKind + GPUDeviceNumber` のタプルをキーにした map — **複数 GPU API × 複数デバイス**
  を管理 (`"MTL:8:"` のようなデバイス記述子文字列あり)
- `GPUBufPool` / `GPURenderMemEstimator` / `GPUPoolBooking` / `GrowingMemoryStack` —
  GPU バッファのプールとメモリ量の推定
- `SplitTuner` — CPU と GPU への作業分割の調整
- `NVBenchmark` — デバイス選定のベンチマーク:
  `setBookingDevices(EnabledDevicesT, NumFilterCPUs)` (有効デバイス + CPU 数から
  割り当てを決める)、`splitFrame / joinFrame` を実際に走らせて性能を測る
- `GPUError` / `GPULogger` / `LocalGPULogger`
- レンダー経路: `FUN_0000ae2c` → `FUN_000080a8` (fetch 時間窓 → **CPU/GPU 分岐** → 合成)
- GPU 経路の準備関数 `FUN_009eacb0` (0x9ebbbc) は保護ゲート付き
  (認証時に書かれるトークン表が絡む)

**Demo 版の制限**: GPU は 1 基のみ (ワンショット表示フラグ `DAT_01538ab0`)。

**未解読**: GPU 処理カーネル本体 (どの演算を GPU で計算するか、CPU 経路との
数値の差があるか) は未解読。保護コードと深く絡むため、CPU 経路との
出力差を比較する黒盒検証での接近が現実的。

## 2. Meat Video の状況

- **GPU 対応: 未実装**。C++ コア (libnvcore) は CPU のみ
  (arm64 は NEON/インラインアセンブラ、非 AArch64 は NV_PORTABLE)。
- Python の numpy も CPU のみ (macOS は Accelerate/OpenBLAS)。

## 3. 現状の評価

- 現在の速度で十分な用途は CPU で満たせる (1080p 1 フレームの処理 2.03 → 0.57 秒、
  AFT 46.8 → 0.61 秒、1080p 全編 721f を数分で処理)。
- GPU が効くのは 4K/8K・長尺・リアルタイムプレビュー。
- **注意**: 本家の正解データ (test_v02_reduce.mov など) が GPU 経路で処理されたものか
  CPU 経路かは不明。もし GPU 経路で CPU と出力が異なる場合、
  我々の較正 (PSNR 45.63 dB など) にはその差が混入している可能性がある
  (区間 A/B の検証は CPU/GPU どちらか不明のまま)。
- **CPU/GPU の A/B (観測、2026-11-03 追記)**: 本家 (導入済みの RS 版、6.1.3 Pro) で E9 のプログレッシブ 6 本を GPU 有効 (12CPU+1GPU auto) と CPU のみで書き出して比べた (2026-11-02 に Phase A で実施、`work/omo/wp_genuine_capture/REPORT.md` §3)。画素はビット一致しない (GPU vs CPU のみ 0/360 フレーム、GPU どうしも 2/360) が、正解に対する PSNR の差は最大 0.005 dB で、画質としては差がない。CPU のみの出力は 2 回で 360/360 一致 (決定的)。
  E9 の本家の数値は CPU 経路でも同じと扱ってよい。ただし本家とのビット一致を目指す照合では、
  GPU の出力は実行ごとに揺れる (推定) ので CPU のみの出力を基準にする。インターレースは A/B していない。
  test_v02 の旧い正解データ (区間 A/B) が CPU/GPU どちらかは不明のまま (ファイルは消失)

## 4. 残りの対応事項

1. **GPU API の確定**: Metal / OpenCL / CUDA のどれを本家が使うか
   (arm64 macOS は Metal が最有力。GPUProductsBridge の動作を黒盒で観察)。
2. ~~**CPU/GPU の出力差の有無の確認**~~ **済み (2026-11-03 追記)**: §3 の A/B。画素は異なるが画質の差は
   最大 0.005 dB (プログレッシブ 6 本)。GPU は本家の Preferences > Performance の Manual で GPU のチェックを外せばオフにできる。
3. **GPU 実装の要否判断**:
   - 出力差がなければ → GPU 対応は「速度のみ」の問題。4K/8K の実用速度が
     必要になった時点で Metal Performance Shaders / Metal を検討する。
   - 出力差がある場合 → 本家と同じ数値にするには GPU カーネルの解読が必要
     (コストが非常に高い。CPU 専用 + 差の文書化を推奨)。
4. **実装する場合の構成案**: libnvcore に Metal バックエンドを追加
   (`nvcore_metal.{h,mm}`)、nvcore_c にデバイス選択 API を追加、
   SplitTuner 相当の CPU/GPU 分割を実装。
5. **CI/検証**: GPU 有無で出力が変わらないことを gold 比較で保証する。

## 5. 結論

- 現状は **CPU 専用**で、1080p までの実用速度は確保済み。
- GPU は「本家の機能のうち未解読の最大の塊」。(2) の A/B (2026-11-03 追記) で画質の差は最大 0.005 dB だったので、
  GPU 対応は「速度のみ」の問題として扱う (§4 の 3)。本家との画質の差の原因を GPU に求める必要はない。
- Meat Video としての公開に GPU は不要 (CPU のみで本家と同等の数値再現を達成済み)。
