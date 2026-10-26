# ネイティブ移行 N1 残件の解決 (F1〜F4) + N1f/N1i

作成: 2026-10-27 (統括)。N1a〜N1h が完了 (コミット 9d6a362) したあとの残件を解決するタスク。
対象の「未移植・未確認」は MIGRATION.md と AGENTS.md に記録済み。

## 共通規約

`.claude/tasks/native_n1_split.md` の共通規約をすべて引き継ぐ (ソースは Python 移植、
repro 変更禁止、ビット一致のみ合格、定数はビットパターンから、fast-math 禁止、
コミットしない、REPORT は sandbox/omo/wp_native_<名前>/REPORT.md)。
加えて MIGRATION.md「移植の重要な知見」(clang の被演算子順入れ替え・NaN 優先の明示選択・
FRECPE/FRECPS 必須) を最初に読むこと。

---

## F1: N1d の取り消し (状態 5) の移植

- **対象**: `native/nv_flat.{h,cpp}` (既存の移植に追加)。
- **仕様** (repro/nv_flat.py の docstring と AGENTS.md §6.5「status 5」節が源):
  - cancel はコールバック (C++ では `bool (*)(void* user)` + userdata、または `std::function<bool()>`。
    真を返したら取り消し済み)。
  - 問い合わせ箇所: 各サイズのループ開始時 (サイズ表の 1 サイズごとに 1 回)。
    曲線 (N1b 側) はタイルごと + スケジューラ空のときもう 1 回 (合計 タイル数+1)。
    N1b の C++ が既に 1 本の曲線を受け取る構成なら、問い合わせ回数の再現は FindFlatArea 側のみでよい
    (REPORT に判断を記す)。
  - 取り消しのとき: 状態 5 を返す。領域は書かない (書きかけの最良値も返さない —
    本家の状態 5 は出力を書かない。repro の実装に合わせる)。
  - キャッシュに入れない (Python の find_flat_area キャッシュは repro 側の話。C++ にキャッシュは不要)。
- **検証** (`native/verify_flat_cancel.py`): `repro/../neatvideo_ofx_614/analysis/noise/verify_cancel.py` と
  同じ発想。環境変数 `NV_FLAT_CANCEL_AT=k` をドライバで受けて、k 回目以降の問い合わせで真を返す
  ホストを実装。実写 8 フレーム × k = 1, 2, 中間, 最終-1, 最終, 最終+1 で:
  - 状態 5 が返ること、問い合わせ回数が Python 側と一致すること (Python 側の回数は
    `find_flat_area_genuine(..., cancel=数え役)` で取得)。
  - cancel=None のときは既存の verify_flat.py と同じ結果 (回帰)。
- **成果物**: 上記 + REPORT.md (`wp_native_flat_cancel/`)。

## F2: N1e の未到達経路の確定と明示化

- **経路 1**: `repro/nv_model_bm.py:151` — FUN_0097ef28 の端複製分岐 (p >= side)。
- **経路 2**: `repro/nv_model_bm.py:328/438` — 条件を満たす方向ビンのない 32×32 ブロック
  (FUN_00ad11b0 がリストを初期化しない)。
- **やること**:
  1. 両経路が本家では**未定義動作**であることの根拠を整理して REPORT に書く
     (AGENTS.md §7「nv_model の制限」: <32px は本家がタイルの外を読む /
     方向ビン不在は 1 辺 512px 以下では幾何的に起きないことを検証者が確認済み)。
  2. C++ の現状の挙動を確認 (例外? 未定義?) し、Python と同じ「明示的なエラー」に揃える
     (`raise NotImplementedError` 相当: 例外を投げるか、エラーコードを返す。どちらにするかは
     既存の nv_model.cpp のエラー伝播の型に合わせる)。
  3. 到達条件の入力を実際に作って (例: 32×31 の領域、方向ビンがゼロになる構造的面)、
     Python と C++ が同じエラーになることを verify_model.py に 1〜2 ケース追加する。
     (本家の「挙動」は UB なので、一致させる対象は repro の Python 実装であることに注意。
     REPORT にその旨を明記する。)
- **成果物**: REPORT.md (`wp_native_model_ub/`) + verify への追加ケース。コード変更は最小限。

## F3: N1e の線カーネル重複の解消 (挙動不変のリファクタ)

- `native/nv_model.cpp` が `native/nv_curve.cpp` と同じ線カーネル表生成を持っている。
  共通の翻訳単位 (例: `native/nv_linekernel.{h,cpp}`) に抜き出し、双方から include する。
- **必須**: リファクタ前後で `verify_model.py` と `verify_curve.py` が同じ全件ビット一致
  (リファクタで 1 ビットも変えない)。差が出たら即中断して REPORT に記す。

## F4: arm64 以外の動作の解決

現状: N1a/N1b は AArch64 インラインアセンブラで演算順を固定 (x86 は NaN 入力の一致を保証しない)、
N1e は arm64 以外で実行時に例外。
- **方針**: アセンブラ / NEON 組み込みに依存しない**明示的ヘルパ関数**に統一する。
  N1b が既に持つ add1/add2/mul1/mul2 (NaN の優先被演算子を明示選択) と
  frecpe32/frecps32 (ソフトウェア FRECPE/FRECPS、`recip-selftest` 済み) を
  `native/nv_portable.{h,cpp}` (新規) に集め、N1a/N1b/N1e の画素演算をすべてこれ経由にする。
  - AArch64 でもこれらのヘルパは通常 -O2 でインライン化されて速度はほぼ変わらないはず。
    N1a/N1b/N1e の verify を再実行して一致 + 計時を REPORT に残す (大幅な速度低下があれば
    arm64 では asm を残すフォールバックでよいが、その場合も portable 版を併置)。
- **x86 での検証** (`native/verify_x86.sh` + REPORT):
  1. `clang++ -arch x86_64 ...` で x86_64 バイナリを作る (Rosetta 2 で実行できる)。
  2. 同じ入力ブロブを arm64 バイナリと x86_64 バイナリに食わせ、**出力同士をバイト比較**
     (Python は介さない。NaN を含むケースを必ず含める)。
  3. 一致すれば「C++ 実装はアーキ非依存で同じ挙動」と言える。不一致なら差の出た入力を
     REPORT に添付 (調査は続行)。
- **N1g/N1h の前提の記録**: 期待値が numpy 2.1.3 / scipy 1.15.3 (arm64) の演算順に依存するのは
  検証ハーネス側の性質であり、移植の性質ではない。MIGRATION.md にその旨を明記する
  (numpy/scipy 更新時は照合をやり直す)。
- **成果物**: REPORT.md (`wp_native_portable/`)、`native/nv_portable.{h,cpp}`、
  `native/verify_x86.sh`、MIGRATION.md の現状更新。

---

## 並行で着手できる本線: N1f (プロファイル) と N1i (AFT)

依存 (N1c+N1d+N1e / N1a+N1e) が揃ったので、F1〜F4 と並行で委託してよい。
タスク内容は `native_n1_split.md` の該当節どおり。注意点:
- N1f の LZMA 照合は**長さ 0 の入力を除外**する (repro/nv_lzma.py と SDK の既知の差)。
- N1f は最終出力が「バイト列の完全一致」(0x00..0x17 のヒープポインタは双方 0)。
- N1i は `nv_aft_v5.py` の `aft_bin_flags` 読み (Quality 表示用) とは無関係で、
  `auto_fine_tune` の出力バイト (3 面 × 9 ビンの語 + 旗) を照合する。

## 貼り付け用プロンプト

```
1. .claude/tasks/native_n1_fixes.md の「F1」「F2」「F3」「F4」を実行してください。
2. 同ファイルの「N1f」「N1i」を実行してください。
```

(1 と 2 は別セッション推奨。各セッションの冒頭で native/MIGRATION.md の
「移植の重要な知見」も読ませること。)
