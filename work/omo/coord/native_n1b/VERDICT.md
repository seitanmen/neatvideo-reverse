# 独立検証 VERDICT: ネイティブ N1d・N1e・N1g・N1h (2026-10-27)

独立検証者 (verifier サブエージェント) の最終応答を統括が転記した。観測と推定を分けている。

## 判定
- **総合**: PASS。4 件とも APPROVE。確信度 高、ブロッカー 0 件。

| 件 | 判定 |
|---|---|
| N1d FindFlatArea (nv_flat) | APPROVE |
| N1e 1 面モデル 4096 語 (nv_model) | APPROVE |
| N1g 時間カーネル (nv_temporal) | APPROVE |
| N1h 空間カーネル (nv_spatial・nv_ndimage) | APPROVE |
| 統合 Makefile | APPROVE (条件: 統括の `make -C native verify` のログで全件一致を確かめる → 確認済み、下記) |

## 方法
- C++ のビルド: native/ の現ソースを規約のフラグで `independent/bin/` に自前でビルドした (native/ には書いていない)。
- Python の期待値: repro の関数を毎回直接呼んで作った。作業者のキャッシュは使っていない。
- 比較: uint32 のビット列で比べた。Python 出力の dtype が float32 であることも毎回確かめた。
- 置き場所: `sandbox/omo/coord/native_n1b/independent/`
  - スクリプト: `ind_flat.py`・`ind_flat_extra.py`・`ind_model.py`・`ind_v5.py`・`neg_control.py`・`probe_prof.py`・`common.py`
  - ログ: `flat.log`・`model.log`・`v5.log`
  - `tmp/` (約 199 MB) はフレームのキャッシュで、消してよい。

## 新しい入力での照合 (観測)

| 件 | 新しい入力 | 結果 |
|---|---|---|
| N1d | 実写: test_v02 n=30/333/640、test.mp4 n=60/510/800 × 色モード 0..2 (18 件)<br>合成: 23 件 (区画ごとにノイズの異なる画像 1280×720・2560×1440、周辺減光、上部 1/3 白飛び、NaN 行・sNaN・−inf、0..255 外の値、小さい画像はモード 0..3) | 41 件 × (-j 1, -j 2) = 82/82 一致 |
| N1d 追加 | 縞柄・B 飽和・G 黒・低周波の柄 15 件、実写の切り出し 15 件 | 15/15、15/15 一致 |
| N1e | 13 面 × set −1, 0..4<br>乱数: 160²・75×130・130×75・有色・縞・周期・NaN ペイロード・512×33・sNaN/inf 33²・inf<br>実写: test_v02 n=640 Y 160²、test.mp4 n=510 G 75×130、test_v02 n=30 Cb 140×90 | 78/78 一致 (状態 + 4096 語) |
| N1g/N1h | 窓: test_v02 n=200..206 (1080p 4:2:0)、test.mp4 n=600..606 (4:2:0・4:4:4・4:2:2)<br>処理: temporal_v5 × 3 組、process_frame_v5 × 3 組、spatial_v5<br>乱数: NaN ペイロード入りの窓 8 件 | 31/31 一致 (43,438,074 語、うち NaN 語 163,562。出力・rv・フロー全件) |

**陰性対照**: C++ 側の入力をわずかに変えると、どれも不一致として検出された。照合が空振りしていないことの確認である。
- model: 1 画素を 1 ulp 変えると 23 語が違った。
- flat: 1 画素に +1e-3 で、trace の float 46 個・曲線 5 点が違った。1 ulp は縮小の丸めに吸収されて差が出なかった。
- v5: threshold を +1 ulp 変えると 321 語が違った。

## 項目別の確認

### 照合スクリプト (コードを読んだ。重大な問題なし)
- verify_model.py
  - 状態と 4096 語を全部比べている。
  - キャッシュの作成時刻は repro の最終変更より後で、repro に差分もないので、古い期待値ではない。
- verify_flat.py
  - 比べているもの: 結果・曲線 (u64)・縮小面・候補・IsClip・trace・構造体。
  - キャッシュは `--cache` を付けたときだけ使う。
- verify_spatial.py / verify_temporal.py
  - 期待値は毎回計算している。キャッシュするのは入力だけ。
  - 飛ばすケースは、決まった条件 (`min(H,W) < bs`) の部品ケースだけ。
  - プロファイル JSON の往復は等価。

### 定数・ビルド
- 定数: 約 20 個を Python 側のビット列と突き合わせて、全部一致した。
- ビルド: フラグは規約どおり、警告 0 (Apple clang 21)。フラグを打ち消す pragma や属性はない。

### repro
- `git status --short repro/` と `git diff --stat repro/` はどちらも空。処理コアは変わっていない。

### 作業者の「Python 側の注意」
- NEP 50 の型昇格: 事実。
  - build_profile_v5 は Python の float/int を返す。
  - `nv_ui_qt/backend.py:62` は `tn = float(...)` なので問題にならない (作業者の「未確認」を解消)。
- numpy/scipy の版依存: 事実。照合はこの環境 (numpy 2.1.3・scipy 1.15.3・arm64) に固定される。
- pyramid の段数 < 3: 事実。Python は IndexError、C++ は 0 で埋める。通常は 5 段なので実害は低い。

## 見つけた問題 (いずれも合否に影響しない)

| 重大度 | 内容 | 補足 |
|---|---|---|
| 低 | verify_spatial/temporal の `diff_count` は、期待値を float32 に変換してから比べる | Python 側が float64 に昇格していても気づけない。今回の入力はすべて float32 だった (検証者が確認)。提案: dtype の assert |
| 低 | verify_model.py のキャッシュの鍵に repro の版が入っていない | 将来 repro を変えると、古い期待値と比べることになる。提案: ソースのハッシュを鍵に含める |
| 低 | verify_flat.py の中間値の一部は、スクリプトが再計算した値か、`nf.candidates` を別に呼んだ値 | 結果と trace は本体から取っているので、空振りではない |
| 情報 | verify_temporal の flows の件数には、事前キャッシュの返却分も含まれる | 件数が少し多めに見えるだけ |
| 情報 | 4:2:2 の時間処理では、`_to_grid` が縦も半分にする (Python の挙動) | C++ も一致して再現している。本番は nv_depth が 4:4:4 に広げるので、この経路には入らない (推定) |

## 未確認
- N1d: 検証者の入力では状態 3 が出なかった (作業者の照合には 13 件ある)。状態 5 (取り消し) は未移植。
- N1e: 状態 1 (NotImplementedError) の経路は、作業者も検証者も通していない。

## 検証者から統括への注意
1. MIGRATION.md に、照合の前提が numpy 2.1.3 / scipy 1.15.3 / arm64 に固定されることを書く。
2. C++ の API のパラメータは、Python の float と同じ扱い (演算の前に float32 にする) が前提。UI からつなぐときは、P に np.float64 を混ぜない。
3. C++ の API で pyramid を使うとき、Y の段数が 3 未満なら拒否する (Python と揃える)。
4. 一致を保証するのは AArch64 だけ。

## 統括の確認 (2026-10-27)
- 各件の再実行 (統括の環境): 次のとおり。ログは `rerun_*.log`。

  | 件 | 結果 |
  |---|---|
  | N1d | 181/181 |
  | N1e | 1244/1244 + 本家キャッシュ 753/753 |
  | N1h | 1287/1287 |
  | N1g | 474/474 |

- `make -C native clean && make -C native all` で警告 0 (`build_all.log`)。
- `make -C native verify` (8 本の通し) は終了コード 0、全件一致 (`rerun_all.log`)。

  | 本 | 結果 |
  |---|---|
  | Quality | 4000/4000 |
  | band | 5706/5706 |
  | curve | 1094/1094 |
  | LZMA | 949/949 |
  | flat | 362/362 |
  | model | 1244/1244 |
  | temporal | 474/474 |
  | spatial | 1287/1287 |

- 統合の作業:
  - `native/Makefile` に 3 ターゲット (nv_flat_test・nv_model_test・nv_v5_test) を追加した。控えは `coord/backup/native_Makefile.20261027_pre_n1b`。
  - `.gitignore` に `native/Makefile.flat`/`.model`/`.v5` の例外を追加した。
  - スナップショットの 2 行を計算し直した。控えは `readonly_snapshot_before.sha256.20261027_pre_n1b`、238 行で不一致 0。
