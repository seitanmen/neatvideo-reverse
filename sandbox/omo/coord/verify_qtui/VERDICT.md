# VERDICT: nv_ui_qt (PySide6 UI) 独立検証

総合判定: **REQUEST CHANGES** (主張 1〜7 の中核は確認できた。ただし AFT の二重適用・古い AFT 結果による上書き・終了時のハングと QThread 破棄は実害のある欠陥)

検証スクリプトは全て scratchpad 側 (`.../scratchpad/qt/verify/*.py`)。Qt は全て QT_QPA_PLATFORM=offscreen、`python3 -I`。プロジェクト内ファイルは変更していない。repro/out/ には書いていない。harness は実行していない。

## 主張ごとの判定

### 1. selftest 28/28: CONFIRMED
- `cd repro && PYTHONDONTWRITEBYTECODE=1 QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest ../test.mp4 $V/st` → 末尾 `28/28 passed`、18.6 秒 (selftest.log)。
- 観測: 28 件全て PASS。
- 推論: selftest の「export == backend.process_frame」は backend 自身との比較なので、対 nv.process_frame_v5 の照合は下の 5 で別途行った。

### 2. 既定で process_frame が nv.process_frame_v5 とビット一致、UI 既定 == V5_DEFAULTS: CONFIRMED
- t1.py: radius 0/1/2/3 × フレーム {0, 1, N/2, N-2, N-1} (test.mp4、901f) の 20 通りで、`nv.process_frame_v5`、`backend.process_frame`、`PreviewEngine.process` (キャッシュ経路、RGB 変換後) が全て一致。不一致 0。
- 時間側キャッシュの再利用 (空間系パラメータだけ変更) は、新規計算と一致 (`cached == fresh: True`)。noise_Cb を変えた場合も一致。
- selftest の `UI defaults map to nv.V5_DEFAULTS` も PASS。
- Temporal Radius 0: 上の 20 通りに含まれ、クラッシュも不一致もなし。UI 側の tkey/キャッシュも問題なし。

### 3. チャンネル別バンド設定 (チャンネルごとに spatial_v5 を呼び、そのチャンネルだけ採用): CONFIRMED
- t1.py (T2): Y のバンド量・Cb の noise・Cb/Cr の量・Cr の medium・Y の amt を変えると、変わるのは期待したチャンネルだけ。他は既定出力とビット一致。
  - noise_Y は全チャンネル (temporal の σ にも効くため期待どおり)。
  - bamt_Cr_high は変化なし (4:2:0 で High は未使用。主張 4 と整合)。
  - bnoise_Y_ultra-low は変化なし。既定の ultra-low 量が 0 だから (欠陥ではない)。
- 注意 (性能、推論): `spatial_ui` は c 番目の呼び出しで 0..c 面を全部計算して捨てるため、非一様のとき空間段の計算は約 2 倍 (Y 1 回 + Y,Cb + Y,Cb,Cr = 6 面評価 vs 3 面)。

### 4. 4:2:0 で Cb/Cr の High が無効: CONFIRMED
- test.mp4: `prof["levels"] = {'Y': 5, 'Cb': 4, 'Cr': 4}`、`bamt_Cb_high` は disabled、ツールチップは "Not used with this profile: Cb has 4 frequency bands …"。
- nvfilter.py:945 の `names = V5_BANDS[len-L:] if c else V5_BANDS[:L]` と整合 (chroma は L=4 で medium から使う)。

### 5. export が UI プロファイルを使い、CRF 0 で backend と一致: CONFIRMED (範囲つき)
- t7.py A: UI で Auto Fine-Tune + Manual Fine-Tune (Y 係数編集) 後に実 QProcess で export (CRF 0, jobs 3, 3 フレーム)。出力プロファイル JSON は `aft_applied: True`、`ui_factors` あり。出力は **素の `nv.process_frame_v5(その JSON プロファイル, v5_params())` と 3/3 フレームでビット一致**。つまり AFT/手動調整が反映され、既定設定では本家 denoise_video_v5 の処理と同一。
- t7.py B: Spatial off + Radius 0 の出力 == 入力フレーム (3/3 ビット一致)。ui_spatial / radius が JSON 経由で子プロセスに渡る。
- spawn 方式でも `backend._work` が子プロセスで使われる (selftest の非一様チャンネル export が PASS)。
- 未検証: AFT 後に UI の profile と、dv が自前で作る profile が別物なのは仕様どおり (パッチは本プロセス内のみ。変更確認: nvfilter.py 10/16 15:34、denoise_video_v5.py 10/16 00:02、どちらも未変更)。

### 6. バックエンドに対応のないコントロールの無効化・表示: PARTIAL
- 観測: Sharpening / Jitter / Dust / Flicker / Edge Smoothing / Quality Mode 2 種 / Input Data Gain / Overall Profile Quality は全て disabled (t1.py の 20 件が `enabled=False`)。Overall Profile Quality は "n/a" 表示。
- 観測: 「使えない」旨はツールチップ (子ウィジェット) にだけあり、可視ラベル・セクション見出しには出ない。主張の "labelled as not available" は、可視の表示としては満たしていない (ツールチップは無効ウィジェットでも出る、は推論)。
- 観測: Most Noisy Frame・Auto Complete・Assist・各ガイドは、ツールチップ/本文末尾に "repro heuristic / written for the reproduction" の表記あり (profile_panel.py, preview.py, main_window.py GUIDES)。Overall Profile Quality は未計算と明記。
- 「Quality Mode」「Temporal Quality」等のうち `t_quality`/`s_quality` はツールチップ表記のみ。

### 7. Undo/redo、プリセット/プロファイル JSON、QSettings 既定: CONFIRMED (異常入力に難あり)
- selftest で undo/redo、preset/profile の保存・読込、Reset、既定プリセットの割り当て・Reset 反映が PASS。
- t3.py T5: assign_default_preset / assign_default_profile 後に別 MainWindow を作ると `noise_Cr` = 150 が復元され、動画を開くと既定プロファイルが適用される。
- 異常入力 (欠陥 D5 参照): JSON の最上位がリスト/数値、`params` がリストの場合に AttributeError が未捕捉。

## 検収基準
| # | 基準 | 判定 | 根拠 |
|---|---|---|---|
| 1 | `python -m nv_ui_qt.main` で起動 | 満たす | offscreen で 10 秒間エラー出力なしで動作継続 (perl alarm で打ち切り、終了コード 142) |
| 2 | 動画を開いてプレビュー表示 | 満たす | selftest「load + auto profile」「preview filtered」、1080p も下記 |
| 3 | Auto Profile 実行 | 満たす | selftest PASS。1080p でも area [0, 856, 128] |
| 4 | AFT で曲線が変わる | 満たす (ただし D1・D2) | 1 回目 係数約 1.03〜1.05。2 回目で二重適用 |
| 5 | Filter Settings の全スライダーが機能 | 一部 | 実装済みは全て backend 辞書に反映 (selftest)。20 個は設計上 無効 (バックエンドに対応なし) |
| 6 | スライダー変更でプレビュー自動更新 | 満たす | selftest、300ms デバウンス。1080p で空間のみ変更 0.3 秒 |
| 7 | Process full video で mp4 | 満たす | t7.py A/B、selftest |
| 8 | Undo/Redo | 満たす | selftest。スライダーのホイール/キー操作は直前の同キー操作に併合される (D6) |
| 9 | プリセット保存・読込 | 満たす | selftest、t3.py |

## 追加の独立検証
- **1080p (test_v02.mp4, 1920x1080, 721f)**, mem_gb=4.0: 読込 + Auto Profile 4.9 秒 (721 フレーム全部)、最大 RSS 約 3.1 GB (フレームは uint8 で約 2.2 GB)。Auto Profile: frame 240, area [0, 856, 128]。1 フレームのプレビュー: Preview Area なし 2.4 秒、あり (中央 960x540) 0.6 秒、空間のみ変更 0.3 秒。
- メモリ上限設定 (mem_gb=0.25): 読込フレーム数 86 (= 0.25·2^30 / 3,110,400 の計算どおり)、最大 RSS 約 1.1 GB。上限は効く。
- ただし上限で切り詰めたときの通知 "(preview holds the first 86 of 721 frames …)" は、直後の Auto Profile の状態表示で上書きされて見えない (D7)。
- **領域選択の回転対応** (rot_test.py): `_display_rect` と `_dragged` を (H,W) = (60,100), (64,64), (100,60) × rot 0..3 × 4 領域 (四隅含む) で、`np.rot90` したマスクの外接矩形と照合。不一致 0。往復 (表示矩形 → 符号化座標) も一致。回転が反時計回りという前提は denoise_video_v5.py:46 の docstring に従う (推論)。ズーム 100% で検証。
- **スレッド**:
  - プレビュー: `gen` が違う結果は破棄、同スロットの要求は最新 1 件に畳まれる。コードと selftest の範囲では古い結果の表示なし。
  - プロファイル: `req` で古い結果を破棄するが、破棄時も `set_busy(False)` するので、AFT 実行中にボタンが再有効化される (t3.py T3b 観測: プロファイル完了後 `b_aft` enabled: True、AFT はまだ実行中)。
  - **AFT: 古い結果が新しいプロファイルを上書き (D2)**。
  - **終了処理 (D3)**。
- 編集ファイル確認: nvfilter.py 10/16 15:34:46、denoise_video_v5.py 10/16 00:02:32、nv_aft_v5.py 10/25 07:35:07 (記載どおり、変更なし)。作業木で nv_aft_v5.py より新しいのは nv_ui_qt/、ui_spec/UI_SPEC.md、`__pycache__`、.omc 状態ファイルのみ。

## 欠陥
| ID | 重大度 | 場所 | 内容 (観測) | 提案 |
|---|---|---|---|---|
| D1 | 中 | backend.py:181-190, main_window.py:403-422 | Auto Fine-Tune を 2 回押すと係数が二乗される。test.mp4 で Y 係数 1回目 [1.044 1.044 1.044 1.031 1.034 1.05 1.05 1.05]、2回目 [1.09 1.09 1.09 1.063 1.069 1.102 1.102 1.102]。`apply_aft` が現在の (AFT 済み) 曲線に乗算するため。冪等でない。 | AFT は AFT 前の base を保持してそこから適用する (`aft_applied` なら元 base を使う)、または押下時に確認 |
| D2 | 中 | main_window.py:416-421 | AFT 実行中に別のプロファイルを読み込む/構築すると、AFT 完了時に `base` が古いプロファイルから作った AFT 結果で置き換わる (t3.py T3a: device は OTHER のままだが area が [8,8,64] から [288,416,48] に戻る)。世代チェックがない。 | AFT の開始時の `prof_token` を保存し、完了時に一致しなければ破棄。AFT 中は他のプロファイル操作を無効化 |
| D3 | 中〜高 | worker.py:78-82, main_window.py:744-748 | `closeEvent` が `wait_all(30000)` で最大 30 秒 UI を止め、AFT はキャンセル不能なので 1080p の AFT 中に閉じると 32.8 秒かかった後 `QThread: Destroyed while thread '' is still running` (t9.py)。 | AFT/プレビューにも `cancelled` チェック点を入れる、または終了時はスレッドを放棄せず `app.aboutToQuit` で待つ/確認ダイアログを出す |
| D4 | 低 | main_window.py:373-376 | 古いプロファイル結果の破棄時も `set_busy(False)` するため、別ジョブ実行中にボタンが有効になる | 現在の `req` のときだけ `set_busy(False)` |
| D5 | 低 | state.py:202-214, main_window.py:532 ほか | 最上位が JSON のリスト/数値、`params` がリストのファイルで AttributeError が未捕捉 (load_preset/load_profile の except に AttributeError がない)。`{"format":"nv_repro_profile","base":{"x":1}}` は受理され、`on_profile` が `KeyError: 'area'` を出して不正な base が状態に残る (profile_panel.py:135) | except に AttributeError を追加、`base` の必須キー検証 |
| D6 | 低 | sliders.py:48-62 | スライダーのホイール/キー操作は `editStarted` を出さないため、同キーの直前の編集に併合され Undo が粗くなる | `valueChanged` で `sliderDown()` でないときは editStarted を出す |
| D7 | 低 | main_window.py:228-231 | メモリ上限で切り詰めた旨のメッセージが Auto Profile の状態表示で消える | 状態バーのラベルを分けるか、最後に再表示 |
| D8 | 低 | main_window.py (cancel_export / process_video) | export 中止後に部分出力 mp4 と `*.ui_params.json` / `*.ui_profile.json` が残る (観測: c.mp4 9.6 MB)。孤児プロセスは残らない (観測: なし) | 中止時に削除するか通知 |
| D9 | 低 | 表示 | 無効コントロールの「未実装」は可視ラベルに出ない (主張 6 の PARTIAL) | ラベルに "(n/a)" 等を付ける、セクション見出しに表記 |

## 仕様・表示の乖離 (欠陥ではなく開示済みの設計判断として記録)
- UI_SPEC の Luminance バンド量既定 (0/60/80/90/100%) ではなく V5_DEFAULTS (0/100/100/100/100%) を採用。state.py の冒頭と task ファイルの表に整合。
- Mix のツールチップは v5 の意味 (0% = フィルタ結果のみ) で、UI_SPEC §5 の文言 ("high values -> more of filter effect") とは逆向き。UI 側が明記している。
- UI_SPEC の Auto Gain、"Select Preview Area Here / Double Click"、PreviewCache 設定は未実装 (検収基準には含まれない)。

## 最終推奨
**REQUEST CHANGES**: 主張 1〜5、7 は確認、6 は部分的。欠陥 D1・D2・D3 は修正を求める (バックエンドの変更は不要)。他は低リスク。

---
# 再検証 (D1〜D9 修正後)

総合判定: **REQUEST CHANGES を APPROVE に変更可能 (条件: 新規 D10 は低〜中。承認後の修正でよい)**。D1〜D9 は全て FIXED。ただし再検証中に、初回の検証の誤りと新規欠陥 D10 を見つけた。

スクリプト: `r_d3.py` `r_d8.py` `r_misc.py` `r_d6.py` (同ディレクトリ、spawn のため `if __name__` ガード付き)。

| ID | 判定 | 根拠 (観測) |
|---|---|---|
| D1 | FIXED | test.mp4 で AFT 2 回 == 1 回 (曲線 JSON 一致)、元と異なる。undo 2 回で元の曲線に戻る。1080p (test_v02) でも 2 回 == 1 回 (`r_d3.py`)。1080p の AFT は 48.1 秒 |
| D2 | FIXED | AFT 実行中の 2 回目は "Auto Fine-Tune is already running" で拒否。実行中に別プロファイル (device OTHER, area [8,8,64]) を設定 → 完了後も device/area は OTHER のまま、"result discarded: the profile changed while it ran" |
| D3 | FIXED | test_v02.mp4 (mem 0.25GB) で AFT 開始 0.5 秒後に `close()` → **2.20 秒**で戻り、`QThread: Destroyed…` は出力に無し (stderr を含め確認)。AFT は spawn した子プロセスを terminate |
| D4 | FIXED | AFT 実行中にプロファイル作業が終わっても `b_aft` は無効のまま。重ねたプロファイル要求の後、アイドル時にボタンは有効 |
| D5 | FIXED | list/数値/params がリスト/base が不完全/base 無し/factors の形不正/文字列値/クリップボードの不正内容、全て捕捉され状態は変わらない。正常なプロファイルは読める |
| D6 | FIXED | キー操作 3 回 → undo 3 件。実マウス (QTest で press/move/release) のドラッグ → 1 件、グルーブのクリック → 1 件。(合成の `setSliderPosition` 4 回は 4 件になったが、これは実マウス経路ではないテスト方法の問題で、欠陥ではない) |
| D7 | FIXED | ステータスバー常設ラベル "640x360 · preview 776/901 frames (memory cap)"。776 は 0.25·2^30/345600 と一致 |
| D8 | FIXED (ファイル) | キャンセル後、c.mp4・c.ui_params.json・c.ui_profile.json が削除され settings.ini のみ残る。"export cancelled" 表示。ただし**ウィンドウを閉じて export を止めた場合は d.mp4 と d.ui_*.json が残る** (D10b) |
| D9 | FIXED | 実装されていない 20 コントロールの可視ラベルは全て "(n/a)"。5 セクション見出しに "— not available in the reproduction"。Input Data Gain は "x1.0 (n/a)"、品質バーは "n/a (not computed)" |

selftest: `33/33 passed` (selftest2.log)。

## 新規・訂正
- **訂正 (初回の誤り)**: 初回の検証で「export 中止後に孤児プロセスは残らない」と書いたが、確認の grep が `t7/c.mp4|nv_ui_qt.export` だけで、`multiprocessing.spawn` のワーカーを見ていなかった。今回 ps を前後比較したところ、**export を中止する/ウィンドウを閉じると、ProcessPoolExecutor のワーカー 3 個と resource_tracker が ppid=1 の孤児として残る** (CPU 0%、待機)。初回テストで残ったものも含め、私が作ったものは kill した (96481/96489 は 8 日前からの別物で触っていない)。
- **D10a (低〜中)**: `worker.py` `ExportProcess.kill()` (QProcess.kill は親だけ SIGKILL) が原因。プロセスグループごと終了する、または export.py 側で親の死活を監視する。
- **D10b (低)**: closeEvent で export を止めたときは、部分出力と `*.ui_*.json` の削除が行われない (cancel_export の経路のみ削除)。

## 最終推奨
D1〜D9 は全て修正を確認。D3 の目標 (1080p で AFT 中に閉じて 2.2 秒、QThread 破棄メッセージなし) は達成。**APPROVE** (D10a/D10b は後続で修正)。

---
# D10 再検証

| 項目 | 判定 | 観測 |
|---|---|---|
| D10a 孤児プロセス (キャンセル) | FIXED | `r_d10_ui.py cancel`: キャンセル後、export のプロセスグループ 0、ワーカー/ffmpeg/resource_tracker の残存なし (ps) |
| D10a (ウィンドウを閉じる) | FIXED | `close`: グループ 0、残存なし |
| D10a (UI 自体を SIGKILL) | FIXED | export 実行中 (グループ 7 プロセス: export・ffmpeg 2・resource_tracker・ワーカー 3) に UI を `kill -9` → 約 1〜2 秒でグループが 0 になる (ウォッチドッグ)。残存プロセスなし |
| D10b ファイル削除 (close) | FIXED | close 後のディレクトリは settings.ini のみ (mp4・*.json 全て削除) |
| キャンセル時のファイル | FIXED | 同上 |
| (b) 正常終了時のファイル | OK | `done` (3 フレーム): o.mp4・o.json・o.ui_params.json・o.ui_profile.json が残り、export_ok = True |
| 補足 | 観測 | UI を SIGKILL した場合は部分出力 o.mp4 と o.ui_*.json が残る (UI が死んでいるため削除できない。許容範囲) |

触っていない既存の孤児 2 つ (ppid 1、約 8 日、PID 96481/96489) は残っている。

D10 は全て FIXED。総合判定: **APPROVE**。
