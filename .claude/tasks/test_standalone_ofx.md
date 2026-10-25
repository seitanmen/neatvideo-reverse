# タスク: スタンドアロンモードと OFX モードの経路テスト

## 概要

`repro/nv_ui_qt/` の 2 つの起動経路 (スタンドアロン + OFX ホストモード) を
実行して動作を確認し、不具合があれば修正する。

- スタンドアロン: `cd repro && python3 -m nv_ui_qt.main ../test.mp4`
- OFX モード: `ffmpeg ... -f rawvideo - | python3 -m nv_ui_qt.main --ofx 640 360 24`

## 前提

- 作業ディレクトリ: `/Users/seitan/Desktop/work/reverse/repro`
- テスト動画: `../test.mp4` (640×360, 901 フレーム, yuv420p)
- PySide6 6.12.0 インストール済み (anaconda)
- 既存の自己試験 55/55 PASS (参考: `nv_ui_qt/selftest.py`)
- OFX ブリッジ: `nv_ui_qt/ofx_bridge.py` (stdin/stdout で raw yuv420p フレーム交換)
- OFX モードの `_set_ofx_mode()` は `main_window.py` にある (Open/Export/Generic を無効化)

## テスト項目

### 1. スタンドアロンモード

```bash
cd /Users/seitan/Desktop/work/reverse/repro
QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.main ../test.mp4
```

offscreen では画面が出ないので、selftest か個別のスクリプトで以下を確認:

1.1. 動画が読み込まれる (frames > 0、info["W"] > 0)
1.2. Auto Profile が実行できる (プロファイルが生成される、flat_status が返る)
1.3. Auto Profile 後に AFT が自動実行される (prof["aft_applied"] == True)
1.4. スライダーを変更するとプレビューが再計算される
1.5. Apply ボタンでプレビューに現在の設定が反映される
1.6. Effect > Export Full Video で mp4 が書き出せる (5 フレーム程度で良い)

### 2. OFX モード — ブリッジの単体テスト

```python
# ofx_bridge.py の OfxFrameSource を stdin なしでテスト
from nv_ui_qt.ofx_bridge import OfxFrameSource, unpack_yuv420p, pack_yuv420p, frame_size
import numpy as np

W, H = 640, 360
# ダミーフレーム生成
y = np.full((H, W), 128, np.uint8)
cb = np.full((H//2, W//2), 128, np.uint8)
cr = np.full((H//2, W//2), 128, np.uint8)
data = y.tobytes() + cb.tobytes() + cr.tobytes()

# unpack → pack ラウンドトリップ
planes = unpack_yuv420p(data, W, H)
packed = pack_yuv420p(planes)
assert packed == data, "roundtrip mismatch"
assert frame_size(W, H) == len(data)
```

### 3. OFX モード — パイプ経由の起動テスト

ffmpeg をホストの代役にして、raw フレームを OFX モードの stdin に流す:

```bash
cd /Users/seitan/Desktop/work/reverse

# 5 フレーム (192×108 に縮小して高速化) をパイプで送信
ffmpeg -y -nostdin -i test.mp4 -vf "scale=192:108" -pix_fmt yuv420p \
  -frames:v 5 -f rawvideo - 2>/dev/null | \
python3 -c "
import sys
sys.path.insert(0, 'repro')
from nv_ui_qt.ofx_bridge import OfxFrameSource
W, H = 192, 108
src = OfxFrameSource(W, H)
frames = src.read_all()
print(f'frames: {len(frames)}')
assert len(frames) == 5
print('OFX bridge: stdin → frames OK')
"
```

### 4. OFX モード — UI 起動テスト (offscreen)

offscreen モードで OFX UI を起動し、以下を確認:

4.1. フレームが stdin から読み込まれる
4.2. Open / Export / Generic Profile が無効化されている
4.3. Apply を押すと処理済みフレームが stdout に書き出される
4.4. Auto Profile → AFT が動作する

offscreen で Qt の stdin/stdout パイプを扱うのは難しいため、
必要に応じてテスト用のスクリプトを書く (frames を直接注入する)。

### 5. 不具合が見つかった場合

- 修正して再テストする
- 修正内容を REPORT に記録する
- 既存の処理コア (`nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`) は変更しない

### 6. 報告

結果を `sandbox/omo/coord/verify_qtui_paths/REPORT.md` に書く:
- 各テスト項目の PASS/FAIL
- 不具合があれば修正内容
- スタンドアロンと OFX の両モードの起動確認の証跡

## 注意

- `QT_QPA_PLATFORM=offscreen` を使って画面を出さずに実行する
- ただしスタンドアロンの起動確認 (1) は `QT_QPA_PLATFORM` を付けずに一度実行して
  ユーザーが画面で確認できるようにすること (起動して即座にスクリーンショットを取るか、
  数秒待ってから終了する)
- 既存の自己試験 (`nv_ui_qt/selftest.py`) も最後に実行して 55/55 を再確認する

---

## 追加依頼 (ユーザー指示): 本家 (Resolve の有料版) との機能比較・修正

### 前提

- Resolve に Neat Video v6 の**有料版** (NeatVideo6RS) がインストールされている
  - プラグイン: `/Library/OFX/Plugins/NeatVideo6RS.ofx.bundle/`
  - アプリ: `/Applications/Neat Video v6 for Resolve/`
- ライセンスは有効 (ユーザーが登録済み)
- 再現 UI: `repro/nv_ui_qt/`
- 比較用動画: `test_v02.mp4` (1920×1080, 721 フレーム)

### 7. 本家の UI を Resolve で開いて比較する

7.1. Resolve を起動し、test_v02.mp4 をメディアプールに読み込む
7.2. クリップに Neat Video v6 を適用してプラグインの UI を開く
7.3. 以下を観察してスクリーンショットまたはメモを取る:

- **各ボタンの挙動**:
  - Auto Profile (Current Frame) → 何が起きるか (AFT も自動で走るか)
  - Auto Fine-Tune → 曲線グラフの表示、Base Profile と Tuned Profile のオーバーレイ
  - Profile Check → 何が表示されるか (ノイズグラフ、品質インジケーター)
  - Noise Level Check → マスクプレビューの見た目
  - Filter Settings Check → マスクプレビューの見た目
  - Apply → 何が起きるか

- **ノイズサンプルの選択範囲**:
  - 長方形の選択ができるか (正方形のみか)
  - 選択範囲のサイズ制限
  - 選択範囲のドラッグ操作

- **ビューアの表示モード**:
  - YCrCb+ の表示内容 (4 ペーンの構成)
  - YCrCb の表示内容
  - Composite の表示
  - Enhanced 表示の違い
  - Original / Filtered / Split の切替方法

- **スライダーの範囲と刻み**:
  - Noise Reduction Amount: 0-100% か、刻みは何か
  - Noise Level: 0-200% か、relative 表示か
  - Radius: ドロップダウンかスライダーか
  - Sharpening: 0-100% の範囲か

### 8. 再現 UI との差分を修正する

7 で観察した内容をもとに、`repro/nv_ui_qt/` を修正する:

8.1. **ボタンの挙動**: 本家と同じ動作にする (特に Auto Profile 後の AFT 自動実行、Profile Check の表示内容)

8.2. **ノイズサンプルの選択範囲**: 正方形のみではなく長方形も選択できるようにする
     - `find_flat_area()` の戻り値を長方形 `(y0, x0, h, w)` に変更
     - UI のドラッグ選択を長方形に対応
     - `nv_profile.build_profile()` の crop を長方形で切る

8.3. **YCrCb+ / YCrCb の表示**: 本家の 4 ペーン構成・Enhanced 表示を正確に再現する

8.4. **その他の差分**: 7 で観察した差分をすべて修正

### 9. test_v02.mp4 での比較検証

9.1. Resolve の Neat Video で test_v02.mp4 をデノイズ (デフォルト設定) → 出力を保存
9.2. 再現 UI で test_v02.mp4 をデノイズ (同じ設定) → 出力を保存
9.3. フレームごとの PSNR を計算 (Y/U/V)
9.4. 差分が大きい場合、原因を特定して修正する

### 報告

- 追加の結果も `sandbox/omo/coord/verify_qtui_paths/REPORT.md` に追記する
- 修正したファイルのリストと変更内容を記録する
- 処理コア (`nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`) は変更しない (UI 層のみ)
  ただし `find_flat_area()` の長方形対応は `nv_flat.py` / `nvfilter.py` の変更が必要な可能性がある
  (その場合は変更前に backup を取り、変更理由を REPORT に明記する)
