# N3 残件の解決: 本家 UI ギャップの縮小 + 既知問題の修正 (G1〜G9)

作成: 2026-10-29 (統括)。N3 マージ後の既知差 (L-2/L-3/I-5/I-6・x86 未照合・実画面未確認) と、
本家との UI ギャップ (Agents.md「本家と違う点」+ N2 の既知項目) を解決する。

## 共通規約

- 処理コア Python (`repro/nvfilter.py`・`repro/nv_aft_v5.py`・`repro/nv_depth.py`) は変更禁止。
  `repro/nv_ui_qt/`・`qtui/`・`native/` は可 (backend の公開 I/F は不変)。
- **両 UI (PySide6 と Qt C++) は常に同じ出力**でなければならない。片方だけ変えて
  バイト一致が崩れたら失敗。変更後は必ず:
  1. `QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest ../test.mp4 <out>` (96/96 以上)
  2. `QT_QPA_PLATFORM=offscreen NV_TEST_VIDEO=<動画> ./qtui/build_main/qtui_selftest` (105/105 以上)
  3. `make -C native verify` (11 本) + `make -C native lib && python3 native/verify_bridge.py` (全語一致)
  4. 書き出しバイト一致 (ON/OFF、PySide6 版と Qt C++ 版)
- 合格はビット一致。fast-math 禁止。コミットしない (統括がマージ)。
- REPORT: `sandbox/omo/wp_qtui_fix/REPORT.md` (全タスク共通 1 ファイルに節を分ける)。

---

## G1: イコライザの R/G/B 化 (本家最大のギャップ。設計レビュー後に実装)

- 本家: イコライザは R/G/B の 3 面 (AFT の係数も RGB 各面の自チャンネル輝度に対するもの)。
  repro: Y/Cr/Cb の 3 面。出力の意味論が変わるため **最後まで温存してきた差**。
- **手順**:
  1. 設計メモを先に書く (`REPORT.md` の冒頭): 本家の RGB 面係数は
     `analysis/noise/aft_port/` の実装 (面 0..2 = R,G,B の自面輝度ビン) と同じ。
     v5 の処理は YCbCr なので、R/G/B の係数を Y/Cb/Cr に写す方法が 2 案:
     (a) AFT と同じ厳密な色マトリクス結合 (`nv_aft_v5.apply_aft` の `f_c = √(Σ c_cj·f_j)`,
        c_cj = a_j²·m_cj / Σ(a_j²·m_cj)。a_j = 各面のモデル q99) を毎フレーム前に適用する
     (b) RGB 面の曲線自体を処理に使う (v5 の改造になる — 処理コア Python は変更禁止なので不可)
     → **(a) を採用**。UI のイコライザ編集は R/G/B の 9 ビン係数に変わり、
     backend/qtui は apply_aft と同じ結合式で Y/Cb/Cr の曲線倍率を作る。
  2. **実装は Qt C++ 版 (qtui/) のみ** (2026-10-30 の C++ ファースト方針。
     PySide6 版は参照実装のまま変更しない)。イコライザの表示・state を R/G/B の
     9 ビン係数にし、結合式 (apply_aft と同じ a_j²·m_cj の重み) を C++ で実装する
     (N1i の nv_aft に既存の実装があれば流用)。
  3. 互換性: 旧形式の Y/Cr/Cb 係数 JSON は読み込み時に R/G/B への読み替えをするか、
     明示エラーにする (どちらかを REPORT で判断して記す)。
  4. 検証: R/G/B イコライザ +20% 編集の処理フレームが、変更前の Y/Cr/Cb 結合実装と
     ビット一致することを C++ 内部で証明 (同じ R/G/B 係数 → 同じ Y/Cb/Cr 倍率 →
     同じ処理結果)。書き出しは変更前 C++ ビルドとバイト一致。自己試験を更新。
- **注意**: この変更で v5 の既定出力は変わらない (イコライザ既定 = 係数 1.0 のまま)。
  既存の検証 11 本・verify_bridge は影響を受けないこと。

## G2: ffmpeg 欄の Preferences 追加 (両 UI)

- Qt C++ 版 (qtui): Preferences に「ffmpeg の場所」行 (QLineEdit + 参照ボタン)。
  保存先は既存の prefs (`prefs/ffmpeg` キー。ランチャの探索順 NV_FFMPEG → prefs → PATH と整合)。
- PySide6 版も同じ行を追加 (現在は欄がない。同じ prefs キーを使う)。
- 検証: 欄に無効パスを入れたときのダイアログ、有効パスで動画オープンが成功すること。

## G3: L-2 OFX 引数解析の Python 一致

- `qtui/src/app/ofx.cpp` の数値解析を Python 準拠に置き換える:
  - `float("1e400")` → +inf (C++ の strtof は ERANGE で 0/ HUGE_VALF を返す実装がある。
    Python は inf を返す)
  - `int("0x10")` は Python では **ValueError** (0x 接頭辞なしの 10 進のみ。符号・空白は許可)。
    C++ が 16 進として受け付けていたら直す
  - Unicode の 10 進数字 (アラビア・インド数字など) は Python の int/float は受け付ける
    (`int("١٠") == 10`)。C++ は受け付けない → Python 互換の数字判定
    (Py_UNICODE_ISDECIMAL 相当: Unicode Nd カテゴリの変換テーブル。必要な文字だけの
    小さなテーブルでよい。REPORT に対応文字範囲を記す)
  - Python のエラー文の形式 (例: `ValueError: invalid literal for int() with base 10: '0x10'`)
    と終了コード 1 に揃える (I-6 と共通の整形出力)。
- 検証: 端値 16 ケース + 追加 20 ケースを表にして REPORT に。すべて Python と
  終了コード・出力が一致 (エラー文は §I-6 の整形経由)。

## G4: L-3 + 項目 20 — C++ 自己試験の強化

- 弱い 3 項目を Python 版の自己試験 (`repro/nv_ui_qt/selftest.py`) の同等項目と同じ検査にする。
- 項目 20 (未実装) に Python 版相当の検査を追加する (内容は selftest.py の対応項目)。
- 検証: 全項目実行で 105 → 109 項目以上が全 PASS。weak だった 3 項目は
  意図的に壊した場合に落ちること (1 回だけデモを実行して REPORT に記す)。

## G5: I-5 自己試験の自己完結化

- init フィクスチャで毎回動画を開く (現在は「前の項目が開いた」前提)。
  1 項目だけ実行 (`qtui_selftest <項目名>`) でも落ちないようにする。
  動画オープンが重い場合は、動画を要する項目に共通の init を付けて
  依存を明示する方式に変えてよい (Qt Test の initTestCase + init() の使い分け)。
- 検証: 単項目実行 10 種以上がすべて PASS。

## G6: I-6 エラー出力の整形

- 極端なフレームサイズなどの異常終了時に、C++ 版は Python 版と同じ形式で
  エラーを出す: `Traceback (most recent call last):` 行 + Python 版と同じ
  最終例外行 (`Exception: <message>`)。スタイルは簡易模倣でよいが、
  **最終行の例外型とメッセージは Python 版と同一文字列**にする。
- 終了コード・フレーム出力は既に一致済み (変更の影響なし)。
- 検証: 極端サイズ 8 ケース + G3 のエラー 6 ケースで、stderr の最終行が一致。

## G7: E1 AFT 取り消しの前段対応

- 現状: AFT の取り消しが効くのはプロファイル構築 (領域探索 0.5〜1.6 秒) の後。
- `nvcore_c` のプロファイル構築関数に cancel コールバック引数を追加
  (N1d と同じ方式: `bool (*)(void*)` + userdata。領域探索と曲線の各タイルで問い合わせ)。
  backend の UI 経路から渡す。
- 検証: 構築中に取り消すシナリオ (Auto Profile 直後すぐ Cancel) で、
  取り消しが 1 秒以内に効くこと。取り消しなしの結果はビット不変。

## G8: E2 プロファイル構築の完全 C 化

- 現状 C 化は領域探索のみ。N1 の部品 (band・curve・flat は dylib 内に既存) を使って
  `build_profile_v5` の**UI が使う出力全部** (curve 8 点、band_ratio、tot_ratio、
  sigma、levels、flat_status) を C 側で計算する nvcore_c 関数を追加する。
  目標: 128² のプロファイル構築 2.38 → **1.3 秒以下**。
- N1f (0xc300 blob) とは別物なので注意 (v5 の UI 用プロファイル。Python とビット一致)。
- 検証: 実写 6 フレーム × 2 領域で Python の `build_profile_v5` 出力 dict と
  C 版出力を全語ビット比較 (verify_bridge にケース追加)。自己試験・書き出しバイト一致。

## G9: x86 検証の穴埋め

- `nv_depth` の x86 ビルド (Rosetta) 照合 (208M 語の depth 変換を x86_64 バイナリで実行し
  arm64 出力とバイト比較)。
- x86_64 の libnvcore.dylib を作り (`-arch x86_64`、NV_PORTABLE)、verify_bridge の
  主要ケース (frame/profile/aft) を x86 で実行して arm64 とバイト比較。
- 検証できた範囲とできなかった範囲を REPORT に明記 (nv_ndimage の asm 残存部分は
  「x86 未保証」のまま)。

## 実行順序と分割

```
即実施 (小): G3 → G4 → G5 → G6 → G2   (既知問題の解決。1 セッション)
次 (中):     G7 → G8                   (速度・取消し。1 セッション)
並行/後:     G9 (x86。1 セッション)
最後:        G1 (R/G/B イコライザ。設計メモを統括が確認してから実装)
```

## 貼り付け用プロンプト

```
1. .claude/tasks/native_n3_fixes2.md の「G3」「G4」「G5」「G6」「G2」を実行してください。
2. 同ファイルの「G7」「G8」「G9」を実行してください。
3. G1 (R/G/B イコライザ) は設計メモを書いて統括の確認を待ってください。
```

(各セッション冒頭で native/MIGRATION.md の「移植の重要な知見」も読むこと。
全タスクで共通規約の 4 つの検証 (自己試験×2・make verify・verify_bridge・書き出し一致) を通すこと。)

## 統括の回答 (2026-10-31、4 点)

1. **旧形式 Y/Cr/Cb JSON**: **従来の意味のまま読む扱いでよい (承認)**。
   理由: 旧プリセット/プロファイルの意味は「Y/Cb/Cr 曲線の倍率」であり、従来の意味で読めば
   処理結果が変更前とビット一致する (決定論の維持)。色マトリクスで R/G/B に読み替えると
   「同じファイル = 同じ結果」が崩れるため不採用。運用: 読み込んだプロファイルは
   Y/Cb/Cr モードのまま保持し、RGB イコライザで編集した時点で RGB モードとして保存する
   (g1_legacy_* フィクスチャでの検証を確認済み)。
2. **#4 を閉じてよいか**: **閉じてよい**。#5 (C++ ファースト版) が置き換え。条件:
   - L-1 の C++ 側 SIGSEGV 修正 (check_rgb_keys) が #5 に含まれていることを確認済み (diff 6 か所)。
   - ABI 版数は #5 側で #1〜#3 マージ後の main を取り込んで 3 に上げる (提案どおり)。
   - #4 の PySide6 側 L-1 修正は参照実装の安全修正だが、C++ ファースト方針により
     main への取り込みはしない (必要なら後日 port)。
3. **約 1.1 GB の作業ログ (wp_qtui_fix/g1/)**: **削除してよい**。エビデンス方針どおり
   (REPORT_G1*.md・VERDICT・fixture JSON がコミット済みで、生ログは再生成可能)。
4. **#1 の 3 つの質問**:
   - Python 版 Preferences に Auto Fine-Tune 行を追加 → **このまま採用**
     (両 UI の設定行を揃えるのが正しい。キー auto_fine_tune は共有で挙動も一致)。
   - MemoryError を実際に malloc を試して判定する方式 → **採用**
     (割り当ての成否で判定するほうが正確)。
   - Python 自己試験の終了時まれな停止 → **別タスクとして記録**。
     まず G5 の自己完結化後に再発するかを観察し、再発するならスタック調査タスクを立てる。
