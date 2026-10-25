> **統括注記 (2026-10-20)**: この文書には独立検証で否定・訂正された記述がある。正しい記述は同じフォルダの `CORRECTIONS.md` を参照。

# MERGE_PLAN — wp_spatial の統合手順 (統括用)

前提: すべての判断は統括。本 wp は `sandbox/omo/wp_spatial/` の外に書いていない。

## 0. そのまま使えるもの (変更不要)

- `REPORT.md`・`notes/spatial_analysis.md` — 解析記録。Agents.md §6 への引用源。
- `verify/run_ab.py`・`candidates_ab.json`・`data/`・`logs/run_ab_*.log` — 再現済みの実測。
  v5 基準値を両区間で 0.003 dB 以内で再現しているため、数値はそのまま引用可。

## 1. ハーネス (先に確定させるべき)

1. `harness_request.md` と `nvh_patch.diff` を審査する (`nvh shrink`、FUN_00990f74 の組 0 限定)。
2. 採用するなら統括が nvh.c に適用して make で再ビルド (作業者は make 禁止のため未ビルド・未検証)。
3. `python3 sandbox/omo/wp_spatial/verify/verify_shrink.py` を実行 (base/nvh_common.py の NVH は
   絶対パスを上書き済み)。サブコマンドが無ければ "harness pending" で終了する。
4. 不一致が出たら `nv_spatial.py` の `local_rms`/ランプ/t の式を修正して再実行
   (verify_shrink は最初の不一致で dst/X 比を診断出力する)。
   ビット一致後、REPORT §2 の「ビット照合 0 件」を更新する。

## 2. コードの統合 (repro/ へ)

- `src/nv_spatial.py` → `repro/nv_spatial.py` として新規追加。
  - `nvfilter.py` は変更しない (依存は `import nvfilter` の局所参照のみ) → 他スクリプトへの波及なし。
  - `python3 -m py_compile repro/nv_spatial.py` を通す。
- 差し替えの呼び方は実験のまま (モンキーパッチ) でよい。本線に入れるなら
  `v5_params` に `spatial_engine: "v5"|"genuine"` を足し、`process_frame_v5` 内で分岐する形を提案
  (REPORT §6-2 のとおり `genuine_t` の既定は 1.0 のまま)。
- `verify/sanity.py` は nvfilter のパスを `base/` に向けているため、統合後は
  sys.path の挿入行を削って使う。

## 3. Agents.md の更新 (統括が§4 表と §6 に)

- §4 表の「5バンド空間フィルタ (局所適応)」行に本家の実態 (観測) を追記:
  間引きラプラシアン金字塔 5 段 (box3×3 → ½↓ → (3,1)/4 拡大差) + 白色化
  (ε=9.4433e-4) + 局所 RMS しきい値ランプ `clamp((2t−R)/t)`、t = 1.1·cfg·model、
  通常経路の出力スカラーは model[+0x14+4k]、方向フィルタ組 1..4 あり (未解読)。
- §6 に空間段の節を追加する場合、`notes/spatial_analysis.md` §2・§5 が原文。
  区間 A/B の数値 (REPORT §4) も実測として引用可。
- §6.5 の FUN_00990f74 に触れる箇所があれば §2.5 訂正 (通常経路の scale は model 側) を反映。

## 4. ログ・ノートの帰属 (統括の判断、REPORT §6-4)

- 提案: `logs/run_ab_296.log`・`run_ab_560.log`・`readonly_sha_*.txt` と
  `notes/spatial_analysis.md` の写しを `neatvideo_ofx_614/analysis/blackbox/logs_20261017_spatial/` へ。
- `notes/dec/` (逆コンパイル一式) は既存 `analysis/` の対応物と重複するため、
  新規分 (`fn_0x990f74.c` 等) だけ `analysis/rtti/` 側に置くか判断。

## 5. 競合・後片付け

- `nvfilter.py` を変更していないため `denoise_video_v2.py`/`nv_ui.py`/`validate_*.py` への影響なし。
- 読み取り専用ファイル 28 件の SHA 前後比較: 全件不変 (`logs/readonly_sha_after.txt`)。
- `data/` の YUV (約 400MB) は再生成可 (REPORT の ffmpeg コマンド) — 統合後に削除してよい。

## 6. 今後の延伸 (任意・未実施)

- `nvh shrink` 追加後: 方向フィルタ組 1..4 の照合用サブコマンド (要 core 状態の再現のため、
  別の切り出し方になる可能性あり)。
- FUN_009aae90 の σ テーブル (+0x11548 系・T_j テーブル) の実数値取得 —
  AutoFineTune と同じ保護判定の壁があるため、統括判断。
- タイリング (FUN_0099c694) と GPU 経路 (FUN_009e9940) の解析。
