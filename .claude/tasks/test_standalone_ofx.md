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
