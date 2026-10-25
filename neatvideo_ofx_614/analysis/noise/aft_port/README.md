# AutoFineTune (FUN_009a775c) の静的移植 — 参考実装 (2026-10-18)

出どころ: sandbox/omo/wp_aft (GLM の作業者 → Claude の実装者が修正)。独立検証 2 回 (notes/VERDICT_verify1.md・VERDICT_verify2.md)。

- **本家との照合はしていない**。AutoFineTune は保護判定 (0x9a7828〜0x9a79f0) があるのでハーネスで実行しない。
  本家と照合できたのはモード 2 のタイル順 `shuffle_order` だけ (`nvh aftshuffle`、計 194 件で一致)。
- 逆アセンブルで確認した点: 類似度の R 語 490/491、定数 (3,2,3)/(4,3,4)・Q66 0x3f28f5c3、ゲート G→R→B、
  FUN_009b4e08 の 9 ビン処理と段係数 1.0f、CV 鎖の NaN で続行。保護判定・ハッシュ書き込み様のコードは読まず、移植もしていない
  (入口と本体内の判定は「正しい表なら成立 / 加算 0」と仮定)。
- `verify/verify_self.py` は移植者の読みとの自己整合の試験で、本家との一致の証拠ではない (177 PASS。sandbox 用の読み取り専用チェック 1 件はここでは省かれる)。
  第 2 の検証者が入れた誤りのうち 3 種 (上側外挿の閾値 last>5→>4、位相 2 の旗の < → <=、CV 許容に s_lvl を掛ける) は検出されなかった。
- repro/ の v5 はこれを使わない。v5 の Y 曲線を AFT 曲線で整形した変種は区間 A/B で Y +0.008 / +0.024 dB (同等。exp_results.txt)。
- 実行: `PYTHONDONTWRITEBYTECODE=1 python3 verify/verify_self.py`。nv_aft は既定で `repro/` の nv_band・nv_curve・nv_model・nv_profile を import する (`NV_AFT_BASE` で差し替え可)。
