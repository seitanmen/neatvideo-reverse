# F2: N1e の未到達経路の確定と明示化 — REPORT

作業: 2026-10-27、worktree `.claude/worktrees/n1fix`。タスク文は `.claude/tasks/native_n1_fixes.md` の F2。
**一致させる対象は repro の Python 実装である。** 本家での挙動は、経路 2 が未初期化メモリに依存する
(実質的に未定義動作)。経路 1 は本家では定義された分岐で、移植していないだけである。

## 2 つの経路と本家での性質

### 経路 1: `repro/nv_model_bm.py` の `pad_mirror` (余白 ≥ 辺、FUN_0097ef28 の端複製分岐)

- Python は `p >= H or p >= W` で `NotImplementedError("edge-replicating branch of FUN_0097ef28 (p >= side)")` を送出する。
- **訂正**: タスク文は「両経路が本家では未定義動作」としているが、経路 1 は未定義動作ではない。
  - repro の docstring に「FUN_0097ef54 (rows: … edge row when p >= H)」とある。
    model_notes/special_algo.md にも「鏡映余白の『余白 ≥ 辺』の場合 (端の複製) は移植していない (この経路では辺 ≥ 84)」とある。
  - つまり本家には端の行を複製する定義された分岐があり、移植していないだけである (repro 側の記述に基づく。本家バイナリは読んでいない)。
- 到達不能の根拠 (コードからの事実): ad1ae0 を呼ぶのは FUN_00ad319c だけである。
  - FUN_00ad319c は min(W, H)·3 < 83 なら何もしない。
  - W ≥ 89 かつ H > 88 なら直接、それ以外は 3×3 に並べて呼ぶ。
  - よって ad1ae0 に渡る辺は ≥ 84。余白は設定 0 が 4、設定 1 が 8 なので、model_full からは決して到達しない。

### 経路 2: `nv_model_bm.py:328 / 438` (FUN_00ad11b0 で、条件を満たす方向ビンのない 32×32 ブロック)

- 本家はこのとき出力リストを初期化しない (プールのメモリのまま。model_notes/special_algo.md)。
  後続はその中身に依存するので、実質的に未定義動作である。
- Python は `NotImplementedError("… blocks without a qualifying direction bin (list left uninitialised)")` を送出する。

### 補足: AGENTS.md §7 の「<32px は本家がタイルの外を読む」について

これは nv_profile の FUN_009b2f34 (`repro/nv_profile.py:88`) の話で、N1f の範囲である。N1e の 2 経路とは別物なので、本タスクでは扱っていない。

## 到達不能の確定 (経路 2: 幾何の全数検査)

- best_direction_all の判定 (`nsel ≤ (len(idx) − ninf)/2`) は、ビンごとの INF の個数だけで決まり、コストの値によらない。
- INF になるかどうかは、sad_grid の規則 (`X + dx ≥ 0`, `X + dx + bs ≤ Wp`、y も同様) により、
  ブロックの位置と辺の長さだけで決まる。x と y は独立である。
- そこで、辺 84〜1536 (並べた場合は最大 3 × 512) に現れるすべてのブロック位置について、dx 範囲を [−r, r] に切った区間を集めた。
  区間の数は設定 0 で 10 種、設定 1 で 18 種あった。
- x 区間 × y 区間の全組み合わせ (設定 0 で 100、設定 1 で 324 の配置) を、Python 自身の `best_direction_all` に通した。
  **どの組み合わせでも条件を満たすビンがある**。すなわち、辺 ≥ 84 では経路 2 は値によらず起きない
  (以前の検証者の「1 辺 512 以下では幾何的に起きない」より強い結論)。
- 検査が空振りしないことも確かめた。
  - この再構成を、sad_grid の実際の INF マスク (ゼロ画像、5 サイズ) と比べて一致した。
  - 小さい辺では失敗を正しく予測した (設定 1 の 17×20 と 9×30、設定 0 の 9×30 で NotImplementedError)。

## C++ の挙動の統一 (エラーの伝え方は既存の nv_model の状態コードに合わせた)

- 変更前:
  - 経路 1: `pad_mirror` が `std::logic_error` を投げ、ドライバは状態 99 (例外) にしていた。
  - 経路 2: `NOT_IMPLEMENTED` (状態 1)。
- 変更後: 両経路とも `NOT_IMPLEMENTED` (1)。
  - ad1ae0 の先頭で `pad >= H || pad >= W` なら偽を返す (Python と同じく、最初の pad_mirror の前に止まる)。
  - `pad_mirror` 内の throw は、内部の表明として残した。
- 照合用の入口を追加した:
  - `nvm::bm_quantiles(clean, A, W, H, cfg, q[2])` (nv_model.h)。ad1ae0 を単体で呼ぶ。
  - `nv_model_test bm IN OUT`。
- model_full の結果は 1 ビットも変わっていない (下の照合)。

## verify_model.py への追加 (`ub_checks`、既定で実行)

- (1) 上の幾何の全数検査と、INF マスクの突き合わせ。
- (2) ad1ae0 を直接呼ぶケース 28 個 (各サイズ × 2 種のデータ)。
  - 経路 1: (cfg 1, 6×40)、(1, 40×8)、(0, 4×4)、(0, 30×3)。
  - 経路 2: (1, 20×20)、(1, 17×28)、(0, 10×12)、(0, 9×9)。
  - 通常: (0, 14×14)、(1, 29×29)、(0, 84×84)、(1, 84×84)、(1, 89×120)、(0, 97×61)。q のビットを比較する。
  - データ: 正規ノイズと、NaN・inf を散らした周期パターン。
  - 判定: 経路 1・2 は Python が NotImplementedError、C++ が状態 1 で、双方そうであること。通常のケースは分位点 2 語がビット一致すること。

## 検証

- `python3 native/verify_model.py --jobs 3 --genuine` (F2 の後、ログ `verify_model_f2.txt`):
  - model_full **1244/1244** ビット一致 (218 面 × 組、NaN を含む入力 254 回、5,095,424 語)。
  - ub: 幾何 OK、直接呼び出し **28/28** (経路 1 8/8、経路 2 8/8、通常 12/12)。
  - 本家のダンプ (キャッシュ): **469/469**。184 件はフレームが worktree の model_verify/data になく、照合していない
    (data は git 管理外で、worktree にはリンクされていない)。
- ASan + UBSan でビルドした nv_model_test で直接呼び出しの 28 件を実行し、エラーなし
  (小さい面でも C++ が範囲外を読まない)。
- F3・F4 の後も同じ件数で一致した (`final_verify_all.txt`、`final_verify_portable.txt`)。

## 未解決点

- 経路 1 (端複製の分岐) は移植していない。到達不能なので不要である。
- `bm_quantiles` は照合用で、model_full の外の小さい面も受け付ける。Python と一致することを確かめたのは上の 28 件だけである。

## Python 側への不具合報告

不具合はない。repro の記述について 1 点:
nv_model_bm.pad_mirror の NotImplementedError は「本家の未定義動作」ではなく「未移植の定義済み分岐」である
(タスク文の前提が誤り。docstring 自体は正しい)。
