# Noise Level Check の実装報告 (SPEC R1〜R5、2026-10-26)

実装者 (サブエージェント) の最終応答を、統括がここへ転記した。

## 変えたファイル
控えは `sandbox/omo/coord/backup/*.20261026_pre_nlc`。

| ファイル | 内容 |
|---|---|
| `state.py` | `t_noise` を実装済みにし、ツールチップを本家の文にした。`backend_params` が `P["t_noise"] = ui/100` を出す |
| `backend.py` | `temporal()` は t_noise ≠ 1 のときだけ時間段の noise_level を ×t_noise にする。ほかに `temporal_mask(rv, n, a)`、`temporal_sig(P)` (`_TEMPORAL_KEYS` + t_noise)、`PreviewEngine._temporal_stage` / `mask()` (process と同じ時間段キャッシュ) |
| `display.py` | `nlc_image(rgb, m)` (本家の色の当てはめの式) |
| `main_window.py` | `set_noise_level_check`・`_sync_nlc_widgets`・`_nlc_key`・`request_nlc` (worker 枠 "nlc"、キャッシュ 4、100 ms)。旧 `noise_level_check`・`_nlc_params`・`request_preview` の分岐・`GUIDES["noise"]` を削除。ツールバーのメニュー "1. Noise Level Check" を有効にし、Finish で両方を終える |
| `menus.py` | Tools > Noise Level Check を切り替え (`actions["nlc"]`) にした |
| `widgets/filter_panel.py` | `set_nlc_active` (R3)。Begin の説明に本家の文字列 |
| `widgets/viewer.py` | `set_nlc` / `showing_nlc`、表示元 "nlc"、上中央と右上の文字。`src == "assist"` のコメントを訂正 |
| `selftest.py` | `nlc_checks` 7 項目 |

## 結果 (実装者の測定)
- **R1:**
  - t_noise = 1 は、変更前の控えとの比較 (`verify_backup.py`) で 5 設定 × 3 フレーム、15/15 ビット一致した。
  - −40% では出力が異なり、書き出し (CRF 0) と `backend.process_frame` はビット一致した。
- **R2:** m の値。
  - 同じフレームを 5 回並べると m の最小は 1.000000。−100% では m の最大は 0。
  - test_v02 (こちらはフレーム 17。本家のフレームは不明) の平均 m は、+0% で 0.984 (本家 0.996)、−60% で 0.50 (本家 0.33)、−100% で 4e-7 (本家 0.003)。
- **R3:** 関係する行を黄色にし、関係しない行とタブ (Spatial / General) を無効にした。Begin で Temporal タブに移り、Temporal Noise を開く。
- **R4:** `nv_aft_v5.assist_params` は UI から使わなくなった (nv_aft_v5.py は変更なし)。
- **R5:** 自己試験 77/77 (`st_run1.log`)。

比較画像は `cmp_adjust_normal.png`・`cmp_nlc_0.png`・`cmp_nlc_m60.png`・`cmp_nlc_m100.png` (左が本家、右が repro)。

## 未対応・注意 (実装者の申告)
1. マスクの式は repro の定義。−60% では本家より平均 m が約 0.17 高い (同じフレームではない可能性がある)。
2. 本家で観測していない repro 側の選択が 2 つある。Begin 中にマウスを押している間は元画像を出す。Begin 中は表示モードを変えられる。
3. Noise Reduction Amount は、本家では閉じた状態だが、こちらは開いたまま灰色にしている。
4. Tools > Noise Level Check は repro 独自の項目 (本家のメニューにはない)。
5. Filter Tuning Assist のツールチップ (除去成分を ×4) と実際の動作 (左が元画像・右が処理後) が食い違っている。これは以前からの問題。
6. 再読み込みで終了する点は、コードの経路を確かめただけ。
7. AGENTS.md は未更新 (統括が担当)。
