# W2 の判定 (統括、2026-10-29)

根拠: `sandbox/omo/wp_native_libm/REPORT.md` (W2a 作業者) と `sandbox/omo/coord_w2a/VERDICT_w2a.md` (独立検証、APPROVE)。
統括の再確認: 既定ビルド 10 ファイルの SHA256 が変更前の記録と一致 (`coord_w2a/bin_sha256_now.txt`)。

## 判定: W2c (文書化)。W2b (ベンダー libm への統一) は行わない

観測 (作業者と検証者の両方で再現):
- 実在の 2 つの libm (openlibm・musl) で、FindFlatArea の領域と状態、プロファイル blob (0xc300 バイト)、AFT 後の曲線、
  Quality / Completeness の % は 1 語も変わらない。
- 変わるのは時間段の `expf` (v5 類似度 `exp(-e/thr)`) を通る画素値だけ。float32 で最大 6 ULP (0〜255 尺度で 6e-5)、
  8 ビットの書き出し値では ±1 が約 100 万画素に 1 個 (作業者 36.6M 画素中 26 / 2、検証者 6.9M 画素中 8 / 0)。
- 素朴実装 (alt3) でも、届くのは曲線 (相対 1e-13 以下)・AFT の色重み・合成入力 2 件の Completeness 1% まで。

推定: Windows の UCRT も 1 ULP 級の差と仮定すれば、同じ範囲に収まる。本家もプラットフォーム間の一致を保証していないので、
この範囲は「本家と同じ性質」として文書化で足りる。

## W2c で行うこと (統括がマージ時に実施)
1. MIGRATION.md に、libm 依存 5 関数 (pow・tanf・atan2・sincos・**expf**) と、差が届く語 (時間段の out・rv → 空間段 → 処理済みフレーム。
   musl の pow で合成の補間器 2 値)、届かない語 (領域・状態・プロファイル・AFT 曲線・% 表示) を記す。基準は「macOS libm (Python と同じ)」。
2. Windows runbook §3 の分類に、「temporal・spatial (exp 部品)・verify_bridge の frame 群は libm 差で不一致になり得る (8 ビットで ±1)、
   それ以外の 9 本と bridge の curve/profile/aft/cancel は一致が必須」を追記。
3. ハーネス (`make -C native libm-alt && python3 native/verify_libm_alt.py`) は残し、UCRT の実測が得られたら alt として追加できる旨を記す。

## 未解決 (判定を変えない)
- UCRT そのものは未測定 (runbook 待ち)。
- verify_bridge の frame 群での 8 ビット集計は未実施 (temporal スイートと frames 段で同種の集計あり)。
- `runs/` 15 GB は .gitignore 済み。`capture/*/calls` を消すと replay の再実行はできない。
