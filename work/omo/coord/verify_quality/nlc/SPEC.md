# Noise Level Check を本家に合わせる (2026-10-26)

## 本家の観測 (Resolve 上の 6.1.3 Pro、1920×1080、`../shots/`)
- O1 `g_adjust_normal.png` (通常) → Temporal Noise の Noise Level Check [▶ Begin] を押すと `g_nlc1.png`。
  - ビューア: 主画像が「Assist Image」(右上に白字)。上中央に黄色の "⚙ Noise Level Check"。表示モードは Normal のまま。
  - ツールバーの "Filter Settings Check ▾" が "Noise Level Check ▾" の表示になり、押された (青い) 状態。
  - パネル: Begin が "■ Finish" になる。この操作に関係する行 (Temporal Filter の見出し、Radius、Noise Level Check、Noise Level、Adapt to Changing Noise) が黄色の文字。
    それ以外の行 (Quality Mode、Local Flicker Check など) は無効の灰色。Spatial / General のタブは無効。
  - Finish を押すと元に戻る。
- O2 色 (画面の画素の当てはめ。I = (R+G+B)/3、R,G,B は元画像の表示値):
  - セピア S = I·(1, 0.9, 0.75)、青 B = I·(0.3, 0.45, 0.5) + (0, 0, 126.5)。
  - 出力 = (1 − m)·S + m·B。m は画素ごとのマスク (0〜1)。
  - 4 枚の画面で残差は平均 0.35〜1.33 階調。
- O3 マスク m (Temporal の Noise Level を変えた画面。test_v02 の静止したフレーム、Radius 2):
  - +0%: m の平均 0.996 (ほぼ全面が青)。
  - −60%: 0.33 (画素ごとの斑点。中央値 0.29、10〜90% 点 0.08〜0.64)。
  - −100%: 0.003 (セピアだけ)。
  - 撮影 `g_nlc_low.png`・`g_nlc_mid_1636.png` (−60%)・`g_nlc_mid_1648.png` (中間、m 0.95)。
- O4 文字列 (バイナリ): "Click Begin to show the Temporal Filter mask." / "The mask shows the areas where temporal noise reduction is applied." /
  "Adjust Noise Level while watching the mask to make it sufficiently cover the noisy areas of the frame."

## 現状 (別エージェントの追加分、観測)
- `main_window.noise_level_check` は UI の値を `nv_aft_v5.assist_params` に渡し、KeyError で落ちる (`../assist/nlc_probe.py`)。
- 仮に動いても、処理結果 (空間・全帯域 100%) を出すだけで、マスクではない。
- Temporal の Noise Level (`t_noise`) は n/a。

## 要件
- R1 **Temporal Noise Level を実装する** (UI 側のバックエンド `nv_ui_qt/backend.py` と `state.py` だけ。処理コア nvfilter.py・nv_aft_v5.py・nv_depth.py・denoise_video_v5.py は変更しない)。
  - `t_noise` (表示 +0% = 100) を `backend_params` に `t_noise` (倍率) として出す。
  - `backend.temporal` で、時間段だけ `noise_level` を `noise_level·t_noise` にして `nv.temporal_v5` を呼ぶ。
  - `_TEMPORAL_KEYS` (時間段のキャッシュの鍵) に入れる。
  - 既定 100% では従来とビット一致し、`nv.V5_DEFAULTS` の照合 (selftest) も通ること。
  - 全編出力 (`export.py` は `backend._work` を使う) にも同じ値が効くこと。
  - 範囲は既存の Spec (0〜250%) のままでよい。表示は相対 (+0%)。
- R2 **Noise Level Check = 時間フィルタのマスク** (repro の定義。本家のマスクの式は未解読)。
  - 現在のフレームの時間段 (`PreviewEngine` の時間段キャッシュと同じ値) の残留分散 rv (Y 面) から、m = clamp((1/rv − 1)/(1/rv_min − 1), 0, 1)。
    - rv_min は、窓の全近傍の重みが 1 のときの値 ((1−a) + a/n)² + a²·0.64·(n−1)/n² (n = 窓のフレーム数、a = Temporal Amount)。
    - a ≤ 0 か n = 1 なら m = 0。
  - 表示: O2 の式で、I は元フレームの RGB。主ペインだけに出し、表示モードは Normal にする。上中央に黄色の "⚙ Noise Level Check"、右上に "Assist Image"。
  - Begin 中に Noise Level・Radius・Temporal の Enable などを変えたら、マスクを計算し直す。
  - 古いフレームや古い設定の結果を出さない (Profile Check の `_pc_key` と同じ考え方)。
  - 入口は 3 つで、1 つの状態を共有する: パネルの Begin/Finish、ツールバーの "Filter Settings Check" メニューの "1. Noise Level Check" (有効にする)、メニューにあれば同じもの。
  - 状態の表示: ツールバーのボタンの文字を "Noise Level Check" にして押された状態に。Finish か、メニューの Finish で戻す。
  - Filter Tuning Assist (Spatial Filter Tuning) とは排他にする。Profile Check と同じく、ページ移動・再読み込み・プロファイルなしでは終了する。
- R3 **パネル**: Begin 中は関係する行の文字を黄色 (Profile Check の BeginRow の扱いと同じ部品) にし、関係しない行とタブ (Spatial / General) を無効にする。
  - 再現が難しい部分は、黄色と Finish の表示だけでもよい。その場合は REPORT に書く。
- R4 `main_window.noise_level_check` の現在の実装 (`nv_aft_v5.assist_params` を使うもの、`_nlc_params`、`request_preview` の分岐) を置き換える。
  - `nv_aft_v5.py` は変更しない (関数は残す。UI からは使わない)。
  - `viewer.py` の `src == "assist"` の分岐のコメント "genuine Noise Level Check mask…" は Filter Tuning Assist の表示なので、正しいコメントに直す。
- R5 **selftest に追加** (既存の 70 項目は変えず、件数を増やす):
  - (a) Begin で例外がなく、マスクの画像が出る。Finish で戻る。
  - (b) 静止したフレームの全近傍の重みが 1 になる合成のケース (または同じフレームの繰り返し) で m ≈ 1。t_noise = 0 で m = 0。
  - (c) t_noise 100% ではプレビューが変更前とビット一致。t_noise ≠ 100% では書き出しと `backend.process_frame` がビット一致 (CRF 0)。
  - (d) ツールバーのメニューとパネルの状態が同期する。
- R6 AGENTS.md の nv_ui_qt の段落は統括が更新するので、触れない。

## 作業の約束
- 編集前に `sandbox/omo/coord/backup/` へ `<ファイル名>.20261026_pre_nlc` で控えを取る (既存の控えを上書きしない)。
- 別エージェントも同じリポジトリで作業しているので、編集の直前にファイルを読み直す。
- 変更後は py_compile を通す。
- 自己試験は `QT_QPA_PLATFORM=offscreen` と `NV_UI_QT_SETTINGS=<自分の作業フォルダの ini>` で実行する。
- 書き込んでよいのは `repro/nv_ui_qt/` の対象ファイルと `sandbox/omo/coord/verify_quality/nlc/`。`repro/out/` には書かない。git に触れない。
- 報告は `nlc/REPORT.md` の内容として最後の応答に書く。項目は、変えたファイル、要件ごとの結果、自己試験の数、比較画像 (`nlc/` に本家と同じ構図の offscreen の画像)。
