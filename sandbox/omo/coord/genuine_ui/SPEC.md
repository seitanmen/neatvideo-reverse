# UI を本家 (Resolve 上の Neat Video 6.1.3 Pro) に合わせる — 実装仕様 (統括、2026-10-26)

ユーザーの依頼: 「Resolve の Neat Video v6 有料版の UI を確認して、こちらのボタンの挙動やサンプリング範囲の自由化などを含め同じにしてください。YCrCb+ や YCrCb の表示が若干違うようです。test_v02.mp4 を使用してそれぞれを比較してください。」

根拠の資料 (必ず読むこと):
- `OBSERVED.md`: 統括が本家の画面で観測したこと。画像は `shots/`。
- `STATIC.md`: 本家バイナリの静的解析。表示の式・選択範囲の規則・Noise Level の式。
- `ours_before/`: 変更前のこちらの画面 (test_v02 フレーム 0)。
- 主な画像:
  - `shots/g_yccplus.png`・`g_ycc2.png`・`g_rgb.png` (Prepare の YCrCb+・YCrCb・RGB)
  - `m_yfreq.png` (Y Freq)
  - `sel1.png`・`sel_small.png`・`sel_big.png`・`tri.png` (選択範囲)
  - `bp_after.png` (プロファイル作成後)
  - `adj1.png`・`adj_sp.png`・`adj_gen.png` (Adjust の各タブ)
- 全体の規則: `/Users/seitan/Desktop/work/reverse/AGENTS.md` (§8)。

対象: `repro/nv_ui_qt/` のすべて。`repro/nvfilter.py` と `repro/nv_aft_v5.py` は §3 の互換を保つ最小限の変更だけ許す。

## 1. ビューアの表示 (最優先。STATIC.md §1 の式どおりに)

`widgets/viewer.py` の「repro の定義」(百分位の引き伸ばし、箱フィルタの帯域、TINT) を本家の式に置き換える。
計算は numpy で、新しいモジュール (例 `nv_ui_qt/display.py`) に分けてよい。分解の部品は `repro/nv_band.py` の既存の関数 (箱フィルタ・縮小・拡大) を使うこと。本家とビット一致している部品なので再利用する。

- **色変換**: 表示の RGB (0〜255) から Y = 0.2989R + 0.5866G + 0.1145B、Cr = (R−Y)·0.71316504、Cb = (B−Y)·0.56465274。ストリームの色行列 (bt709 など) は使わない (本家はホストが渡した RGB で計算する)。
- **ペインの写像**: STATIC.md §1c の表 (出力範囲・明るさ b・(A)(B)(C) の式) どおり。
  - こちらの明るさスライダーを 0〜100 (既定 50) にし、b = 50 で恒等にする。
  - ディザ (±0.25) は省いてよい。省いたら docstring に書く。
- **YCrCb**: Cr/Cb ペインは (C) の式 (out = m_c + s'·h_c·v/64.5)。Y は (A)。
- **RGB**: R/G/B ペインは (A) で、出力範囲は表の色付き。
- **Frequencies (High/Mid/Low)** と **YCrCb+ (Enhanced)** は STATIC.md §1d の BuildFreqImage/FindFreqLevs。
  - Prepare ページの YCrCb+ は強調あり (b̃ = b/((Box3(Box3(b²)))^{1/4} + 0.01)、w_k = 1.25^k、5 段の和)。Adjust ページの YCrCb+ は強調なし (帯域の単純な和)。
  - 表示範囲は q30/q70 と正規近似 ±4d (Enhanced の Cr/Cb は ±8d、Y は 1/99% で切る)、写像は (B)。
  - 「up / 縮小 / 3×3 箱」は nv_band と同じ間引きラプラシアンにする。
  - Enhanced と Freq の色変換はプロファイルの色モード 1 (= 上の式) で行う。
- **ペインの配置**: 1+3 Bottom は上段 (主ペイン) と下段の高さの比を 2:1 にする。1+3 Right は幅の比 2:1。
  - 下段は主ペインと同じ倍率。選択範囲があればその中心 ((x0+x1+1)/2, (y0+y1+1)/2) を下段の表示の中心にし、なければ主ペインの中心に合わせる。下段どうしは同じ位置。
- **表示モードはページごとに持つ**: Prepare の既定は YCrCb+、Adjust の既定は Normal (本家の観測)。
- **計算の速さ**: 1080p で切り替えが重くなりすぎないようにする。キャッシュを使い、計算は可能なら worker で。切り替えにかかる秒数を報告する。

## 2. 選択範囲 (サンプリング範囲の自由化。STATIC.md §2・§4)

- Prepare ページの選択は**長方形**にする。
  - ドラッグの起点を角とし、各辺は最大 512 px、画像の内側に制限する。下限は設けない。
  - 選択範囲の内側をドラッグすると、大きさを保って移動する。合成画面をドラッグせずにクリックすると選択を消す。
  - いまの正方形の制限と、プレビュー範囲 (Adjust) の扱いは見直す。Adjust の preview 範囲は辺 47〜1280。
- **枠の色** (min(w,h)):
  - ≤ 15: 赤 (255,15,15)、外側 (128,0,0)、破線
  - 16〜127: 黄 (255,208,15)、外側 (100,97,2)
  - ≥ 128: 緑 (15,192,15)、外側 (0,94,0)
  - プロファイルに使った領域 (Main Sample) は青系 (観測。静的な値は未特定なので (15,15,255) 系でよい)
- **枠の上の "Noise Level: x.x"**: STATIC.md §2 の NL = √(Σ_k Σ_c mean_{c,k}²)。`nv_band.model_levels` の語 0..4 を、選択範囲の RGB 3 面 (0〜255) について計算する。
  - min(w,h) ≥ 16 のときだけ表示する。ドラッグ中もその場で更新する (重ければ間引く)。
  - 右パネルの "Noise Levels: Main Sample / Selection" も同じ式で出す (Main Sample = プロファイルに使った領域、Selection = いまの選択)。
- **枠の下の "Not Uniform: Y, Cr, Cb"**: 選択範囲の色変換 (色モード 1) の各面で `nv_band.band_levels` → `nv_flat.flatness` (本家とビット一致の移植) を計算し、2.0 以上の成分を並べる。IsClip が 0.33 を超える R/G/B があれば "Clipping: …"。どちらも黄色の文字。
- **ボタンの名前と動作**:
  - 選択がないとき "Auto Profile ▾"、あるとき "Build Profile ▾"。
  - 枠の右下に ▽ を描き、クリックで "Build Profile (Most Noisy Frame)" / "Build Profile (Current Frame)" のメニューを出す。
- **作成の制限** (STATIC.md §4):
  - min(w,h) < 32 はエラー ("Too small area for building a noise profile; it should be at least 32x32 pixels." 相当)。
  - 32〜127 は確認 ("Use the small area" / "Cancel")。
  - Fine-Tune は 16 以上。

## 3. 処理側の長方形対応 (互換を保つ最小限の変更)

- `nvfilter.build_profile_v5(area=...)` が (y0, x0, size) に加えて (y0, x0, h, w) を受け付けるようにする。
  - `area_band_sigma`・領域の平均輝度 `lvl`・範囲の切り詰めを長方形で扱う。
  - **(y0, x0, size) と FindFlatArea の結果ではビット一致を保つ** (変更前の控えと比べて試験する)。
  - `prof["area"]` は、正方形なら従来どおり 3 要素、長方形なら 4 要素 [y0, x0, h, w]。
- `nv_aft_v5.apply_aft` (`prof["area"][:3]` を読む) と、`nv_ui_qt` の `area[2]` を読む箇所 (`main_window.py`・`profile_panel.py`・`selftest.py`) を 4 要素に対応させる。
- `denoise_video_v5.py --area` は Y,X,SIZE と Y,X,H,W の両方を受け付ける。`nv_ui.py` (tk) は正方形のままでよいが、壊さないこと。
- 編集の前に、変更前の控えを `sandbox/omo/coord/backup/<name>.20261026_pre_genuineui` に置く (既存の控えは上書きしない)。
- 変更したら `py_compile`。AGENTS.md §4 の数値には影響しないはず (確認すること)。

## 4. ボタンの並びと挙動 (OBSERVED.md と画像に合わせる)

画像を見比べて、違うところを本家に合わせる。少なくとも次の点:

- **メニュー**: Effect View Profile Filter Variants Tools Help (こちらの "Edit" は無くし、Undo/Redo などは本家どおり Profile/Filter メニューへ)。
  - Effect と Profile と Help の項目と順序は OBSERVED.md のとおり (実装のない項目は無効表示)。
  - Variants メニューは無効表示の項目でよい。
- **Prepare のツールバー**: 左に [Auto/Build Profile ▾] [Generic Profile] [Load Profile]、右に [Profile Check] (プロファイル作成後) [Variants (無効)]。
  - こちらの "Auto Match (n/a)" は、本家のこの位置にはないので外す。機能はメニューに残してよい。
- **Prepare の右パネル** (`bp_after.png`):
  - 上のアイコン: 開く・保存・Undo▾・Redo▾・×・ピン・(i)。
  - プロファイル名: 作成前は赤の "profile not ready"。
  - "Device Name and Mode:" と ▸≡、名前の欄 ("Unspecified device")。
  - 情報の欄: Frame size / Scan type / Gamma / Gain / Profiled with。Profiled with は本家では "Full frame (WxH)" と出る。
  - Quality: 値を計算できないなら n/a のまま。
  - Noise Levels: Main Sample / Selection の数値 (§2 の NL)。
  - 帯域 High/Medium/Low/Very Low/Ultra Low の棒: プロファイルの帯域ごとのノイズから。repro の定義なら docstring と報告に書く。
  - イコライザ: AFT の係数が RGB 面ごとに得られるなら、本家どおり R/G/B の 3 本で表示し、[RGB][R][G][B] の選択にする。得られないなら現状維持で、理由を報告する。
  - "Profile Check and Tuning": Profile Check (?) [Begin]、"▸ Noise Level" は畳んだ状態が既定。
- **Adjust のツールバー**: [範囲選択] [Preview] [Play All ▾] の順。右に [Filter Settings Check ▾] [Variants (無効)]。
- **ビューア下のバー**:
  - ズームは本家のようにパーセントで表示する。"Fit" のときも、実際の倍率に近い段のパーセントを出してよい。
  - 表示モードのメニューは本家の見出しどおりにする (Primary Modes / Channel Components / Frequency Components / Viewer Split)。Normal では Viewer Split を無効にする。
  - "W: H:" (選択の大きさ)、"R: G: B:" (カーソル下の値、小数 2 桁)、"Frame: 1920x1080, 8-bit"。
- **最下段**: P▾ γ▾ x1.0▾ (左)、サムネイル、Cancel/Apply。こちらにあるフレームのスライダーは単体アプリに必要なので残してよい (報告に書く)。

**ノイズ除去の既定値** (Luminance 40% など) は変えない。v5 の既定値は本家の出力に合わせて較正したもので、変えると処理結果が変わる。

## 5. 試験 (数値で報告。すべて `sandbox/omo/coord/genuine_ui/impl/` に書く。`repro/out/` 禁止)

1. **表示の照合**:
   - 本家の画像 `shots/g_rgb.png` と `shots/g_ycc2.png` から、ペイン内の画素 (R/G/B ペインの各チャンネル = 元の RGB) を取り出す。新しい display モジュールで YCrCb ペインを計算し、本家の YCrCb ペインとの平均絶対誤差を報告する。
     - 統括の照合では Y が 0.39 だった。ペインごとの位置合わせは STATIC.md 末尾を参照。
   - Enhanced と Freq は、同じ RGB 入力から計算して、本家の `g_yccplus.png` / `m_yfreq.png` と見た目と統計 (平均・標準偏差) を比べる。表示範囲の倍率が 3.6 倍の縮小表示である点に注意。数値の一致は求めないが、目視で同じ質感であること。
2. **画面の比較**: test_v02 フレーム 0 の Prepare (YCrCb+・YCrCb・RGB・Y Freq)・プロファイル作成後・Adjust の 3 タブを offscreen の grab で撮り、`shots/` の同じ画面と並べた比較画像を作って `impl/` に保存する。
3. **長方形のプロファイル**: (y0, x0, size) と FindFlatArea で、build_profile_v5 の結果が変更前の控えとビット一致すること (test.mp4・test_v02 の数フレーム)。長方形 (例 216×512) のプロファイルができ、UI から書き出しまで通ること。
4. **既存の試験**: `QT_QPA_PLATFORM=offscreen NV_UI_QT_SETTINGS=<impl>/st.ini python3 -m nv_ui_qt.selftest ../test.mp4 <impl>/selftest` (repro/ から) がすべて PASS。項目を変えたら理由を書く。新しい振る舞い (長方形の選択・NL・ボタン名の切り替え・表示の式) の項目を追加する。
5. `py_compile`。

## 6. 規則

- Qt は offscreen だけ。NV_UI_QT_SETTINGS は必ず自分のフォルダのファイルにする。
- 本家バンドルの読み込み・nvh の実行はしない。Resolve の画面は操作しない (統括が担当)。
- git の index・履歴を変える操作はしない。重いジョブは同時に 3 つまで。
- 周りのコードの書き方に合わせる。
- 報告は最終応答に書く (REPORT ファイルは統括が保存する)。内容: 変更点、各試験の結果 (数値)、本家との残る違い、判断したこと。
