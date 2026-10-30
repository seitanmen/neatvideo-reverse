# UI を本家に合わせる — 実装報告 (2026-10-26、executor の最終応答から統括が要約して転記。この時点では独立検証は未実施)

## 結果 (実装者の測定)

- selftest は 64/64。長方形の互換試験 (`test_rect.py --aft`) は 129/129。OFX は 4/4。
- 表示の照合 (`test_display.py`):
  - 本家の RGB ペインから計算した YCrCb を `g_ycc2` と比べた平均絶対誤差は、Y 0.307 / Cr 0.438 / Cb 0.337。
  - 当てはめた直線: Cr は 1.719v+143.65 (式 1.736v+143.49)、Cb は 1.827v+117.50 (式 1.828v+117.47)。
- Enhanced (test_v02 フレーム 0 を 2 倍に拡大して計算) の平均 / sd:

  | ペイン | 本家 | repro |
  |---|---|---|
  | Y | 118.1 / 59.3 | 117.3 / 57.6 |
  | Cr | 143.9 / 17.9 | 143.8 / 15.3 |
  | Cb | 117.3 / 20.7 | 116.7 / 17.8 |

- 切り替えの時間 (1080p): Enhanced の初回 0.54〜0.57 秒、キャッシュ済みは 0.01〜0.02 秒。

## 変更したファイル

控えは `coord/backup/*.20261026_pre_genuineui`。

- `nvfilter.py` (差分 16 行): `area_band_sigma` と `build_profile_v5` が (y0, x0, h, w) を受け付ける。正方形の結果は変わらない。
- `nv_aft_v5.py` (差分 11 行): 4 要素の area に対応。
- `denoise_video_v5.py`: `--area` の解析だけを変更。
- `nv_ui_qt/`:
  - `display.py` (新規)
  - `widgets/viewer.py` (書き直し)
  - `main_window.py`
  - `menus.py`
  - `widgets/profile_panel.py`
  - `state.py`
  - `selftest.py`

## 本家との残る違い

- ディザは省いた。
- Cr の傾きに約 1% の差がある。
- Noise Level の後ろの (±n%) は出していない。
- Main Sample の色は本家の画面で測った値。
- 枠の太さは近似。
- 帯域のバーの値は repro の定義。
- Quality は n/a。
- イコライザは Y/Cr/Cb の 3 本のまま (RGB 化すると保存の意味が変わるため)。
- View・Filter・Tools メニューは撮影できていない。
- Fit ズームは連続的な倍率。
- ピンのアイコンは既存のまま。
- フレームのスライダーは残した。
- Adjust の表示範囲は、表示している画像ごとに求める。
