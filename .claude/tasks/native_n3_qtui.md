# N3: Qt 6 C++ UI への移植 (委託タスク、3 段階)

作成: 2026-10-29 (統括)。前提: N1 (コア 9 部品の C++ 化、ビット一致済み) と N2a
(`native/nvcore_c.{h,cpp}` = C ABI `libnvcore.dylib`) 完了。N3 は PySide6 の
`repro/nv_ui_qt/` を Qt 6 (C++17/20) で再構築し、処理は **libnvcore の C ABI を直呼び**する。

## 最重要原則

1. **処理は必ず libnvcore 経路のみ**。C++ UI に numpy を持ち込まない
   (Python 版 UI との一致は N2a の verify_bridge が保証済みなので、同じ C ABI を呼べば
   結果も同じ)。numpy を使いたくなったら nvcore_c に関数を足す (その場合は
   `native/verify_bridge.py` に Python 対応ケースを追加してビット一致を取る)。
2. **UI の仕様は repro/nv_ui_qt が源**。ページ構成・パネルの並び・表示式は
   `repro/nv_ui_qt/` と `sandbox/omo/coord/genuine_ui*/` の観測記録に従う
   (本家合わせ済み: ページ Prepare Noise Profile / Adjust and Preview、メニュー
   Effect / View / Profile / Filter / Variants / Tools / Help、Device Noise Profile パネルの
   Quality / Completeness、Noise Level 表示、選択範囲の色分け、Fit ズーム等)。
  表示式の実装リファレンス: `repro/nv_ui_qt/display.py` (色変換・ペイン・BuildFreqImage)。
3. 規約: float32 明示・fast-math 禁止・`-ffp-contract=off`・C++20・Qt 6.8+ (LGPL)。
   本家の真似で Qt 5.15 にしない (LGPL 6系でよいかはユーザー確認済みの方向で進める)。
4. **コミットしない**。REPORT は `sandbox/omo/wp_qtui_<段階>/REPORT.md`。
   処理コア Python・nvcore_c の既存関数の挙動は変えない (追加のみ)。
5. 各段階の合格は「Python 版 UI と**画面の見た目・操作・出力が同じ**」:
   - 出力: 同じ入力・操作列で書き出した mp4 がバイト一致 (N2b の test_app.sh 方式)。
   - 見た目: スクリーンショット比較 (画像は REPORT に添付。差分の数値化は任意)。
   - 自己試験: C++ 版の selftest (Qt Test) を作り、段階ごとに増やす。

## N3a: 骨格 + Prepare ページ

- `qtui/` (新規ディレクトリ) + `qtui/CMakeLists.txt`。libnvcore をリンク
  (FindNativeCore.cmake か単純パス)。Python に依存しない。
- ウィンドウ枠・2 ページ切替・メニュー/ツールバーの並び・ステータスバー・
  ビューア (Original / YCrCb+ 合成 / Y・Cr・Cb Enhanced / Filtered 表示、選択範囲の
  ドラッグ・移動・消去・色分け・Noise Level ラベル)。
- Device Noise Profile パネル (Quality / Completeness / Noise Levels / バー / イコライザ表示)。
  Quality は N0 の `nv_quality` を直呼び。
- 動画オープン: ffmpeg でデコードして自前で YCbCr に (N2b の探索順を流用)。
  プレビューのメモリ上限・フレーム並列は Python 版と同じ仕様。
- 合格: 動画オープン → 選択 → Build Profile (dylib 経路) → パネル表示が Python 版と同じ。
  自己試験 20 項目以上。

## N3b: Adjust ページ + 書き出し

- Filter Settings (Temporal / Spatial / General タブ、Luminance → Cr → Cb、
  帯域 High → Ultra Low、Frequencies 行、Mix with Original、Noise Level Check の
  マスク表示)、プレビュー (Preview Area、キャッシュ、世代管理)、サムネイル
  (時間窓表示)、Undo/Redo、プロファイル/プリセットの JSON 入出力
  (Python 版と同じファイル形式)。
- 書き出し: N2 の export 経路 (CRF・音声・高ビット深度・codecs) を C++ から
  ffmpeg プロセスで (Python 版と同じコマンド列を生成)。
- 合格: 同じ操作列で .mp4 がバイト一致。自己試験 30 項目以上。

## N3c: 残り (Preferences / Profile Viewer / Profile Check / Variants / OFX モード)

- Preferences (Bit depth・native core・jobs・CRF・音声・auto AFT・メモリ上限・ffmpeg パス)、
  Profile Viewer (Quality/Completeness 表示)、Profile Check / Noise Level Check、
  Variants、Generic Profile、OFX モード (`--ofx W H [FPS] [MAX_FRAMES]`。
  プロトコルは `native/...` ではなく `repro/nv_ui_qt/ofx_bridge.py` の docstring)。
- 合格: Python 版の自己試験 96 項目に対応する C++ 版 selftest (項目表を
  REPORT に対応付けて載せる) + 書き出しバイト一致。

## 納め方・その他

- REPORT・スクリーンショット・対応表は `sandbox/omo/wp_qtui_a|b|c/`。
- スナップショット・AGENTS.md・MIGRATION.md は触らない (統括が更新)。
- Qt のライセンス表記 (LGPL) を about に載せる。
- 不明点は REPORT の「質問」節に書いて停止せず仮実装してよい (統括が判定する)。

## 貼り付け用プロンプト

```
1. .claude/tasks/native_n3_qtui.md の「N3a」を実行してください。
2. 同ファイルの「N3b」を実行してください。
3. 同ファイルの「N3c」を実行してください。
```

(1〜3 は別セッション推奨。N3a の完了を統括が検証してから N3b に進むこと。)
