# 本家 UI に合わせる変更 — 独立検証 (2026-10-26。検証者の最終応答から統括が要約。ログは verify/)

## Round 1: APPROVE (ブロッカー 0)

- **処理側の互換**: `v_core.py --aft` は 161/161。
  - area が None、正方形、端からはみ出す範囲のいずれも、build_profile_v5 と apply_aft が控えとビット一致した (float は 64 ビットのビット列で比較)。
  - 長方形の切り詰め・area_level・area_sigma は、独立に計算した値と一致した。
  - denoise_video_v5 の 8 ビットの既定出力 (test.mp4 --start 300 --frames 8 --crf 0) は、mp4 がバイト単位で一致した。
- **表示の式**:
  - コードの読み: `display.py` を STATIC.md §1 と逆コンパイル結果に照らして、off-by-one はなかった。
  - 独立実装との差は最大 1 LSB (Enhanced と Freq は 1/16)。
  - 本家の画面 (g_rgb → g_ycc2) との平均絶対誤差は Y 0.293 / Cr 0.427 / Cb 0.334。
  - Enhanced と Freq は本家と同じ質感だが、標準偏差が本家より 1〜17% 低い (Resolve の拡大の違いと推定)。
  - Noise Level の本家値: 24.5 に対して 24.2〜26.0、15.1 に対して 15.4〜16.4 (拡大した画像で計算)。
- **選択範囲** (QTest で 24/24): 角・最大 512・画像内への制限・移動・クリックで消去・枠の色・NL (独立計算と一致)・Not Uniform・▽ メニュー・Auto/Build の名前の切り替え・32 未満の無効・32〜127 の確認、のすべてを確かめた。
  - 216×512 のプロファイルで、CRF 0 の書き出しが `backend.process_frame` とビット一致した。
- **メニューとツールバー**: OBSERVED.md のとおり。
- **回帰**: selftest は 64/64。OFX は 6/6。py_compile と pyflakes は問題なし。
- **切り替えの時間** (1080p): Enhanced 0.47〜0.52 秒、Freq 0.27〜0.31 秒、キャッシュ済み 0.007 秒。
- **指摘 (Low)**:
  1. `--area a,b,c` でトレースバックが出る。
  2. 4 要素で h==w のとき、正方形の経路で切り詰められる。
  3. メニューの区切り線・⌘C/⌘V・Undo の文言。
- **参考**:
  - Cr の傾きの約 1% の差は、当てはめ方による見かけの差 (端を除くと 1.7349、式は 1.7364)。
  - NL の計算は GUI スレッドで行う (ドラッグ中は 100 ms に 1 回)。
  - 目に見える残りの違い: Fit ズーム、イコライザが RGB でない、Quality が n/a、Repro の欄、枠の太さ、Adjust の既定 70%、Spatial の節が展開されている、Quality Mode が n/a。

統括が指摘 1〜3 を直した (控えは `coord/backup/*.20261026_pre_lowfix`)。

## Round 2 (統括の Low の修正の再検証): REJECT → 修正 → APPROVE

- 再検証では (a)〜(d) が PASS した。
  - `v_core.py --aft`: 186/186。
  - `--area` の誤った形 5 種は 0.35〜0.38 秒で rc 2 になる。正しい形の出力は控えとバイト一致。
  - メニュー: 6/6。⌘C/⌘V は入力欄の操作を奪わない。
  - selftest: 64/64。
- ただし Medium が 1 件あった。Undo の文言を固定するために入れた接続が、終了時に「破棄済みの QAction」の RuntimeError を stderr に出していた (OFX で設定を変えて閉じたとき)。
- 統括の対応: `menus.py` の関数 relabel で `shiboken6.isValid` を確かめるようにした。
- 再々検証: APPROVE。
  - OFX 6/6 で、フレーム数は前回と同じ。stderr は Qt の offscreen の通知だけで、トレースバックは 0。
  - メニュー: 6/6。
  - selftest: 64/64。
