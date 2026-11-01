# インターレース対応 REPORT (INT-1〜INT-4)

段階ごとに節を分ける。共通規約は `.claude/tasks/interlace.md`。

## INT-1 (2026-10-31、main にマージ済み)
- 作業者の報告と独立検証 (APPROVE、L-1 は INT-2 の前に直すこと) の写し: `INT-1/REPORT.md`・`INT-1/VERDICT.md`。
  原本は `.claude/worktrees/int1/work/omo/wp_interlace/` にある。

## INT-2: フィールド単位の処理パイプライン (2026-11-02、作業者 int2-worker、独立検証 verify-int2 **APPROVE**)
- worktree: `.claude/worktrees/int2` (branch `int2-fields`、main 6cd9e49 から。**未コミット**)。
- 作業者の報告全文 (SendMessage 3 通の転記): `INT-2/REPORT_DRAFT.md`。
- 独立検証の全文: `INT-2/verifier/VERDICT_round1.md`。
- 変更前の控え: `INT-2/backup/` (SHA256SUMS 付き)。ログ: `INT-2/logs/`。

### L-1 (観測)
- 修正は main にすでに入っていた (INT-1 の Round 1 対応)。今回コードは変えず、再確認だけ行った。
- フィールドの順序は、最初のフレームの interlaced_frame / top_field_first で決める。stream の値は tt/bb だけを使い、tb/bt は順序不明として扱う。
- 照合の結果:
  - 作業者: 判定したものは 11 本中 10 本が実際の順序と一致し、prores (+ildct なし) の 1 本は不明と判定した。食い違いは 0。
  - 検証者: 別に作った素材で、判定した 12 本がすべて一致した。progressive と prores (+ildct なし) の 3 本は拒否された。
- 順序を決められないとき (first = −1) は、フィールド経路に入らない。API は例外を投げ、試験用 CLI は止まる。

### 設計
- **時間の並び**: フィールドの時間順は [n top, n bottom, n+1 top, …] (BFF なら下が先)。時間半径はフィールド数で数える。
- **フィールドごとの処理**: temporal_v5 → spatial_v5 (既存のものを高さ半分の画像に掛ける) → mix の順に掛け、nv_fields で再結合する。
- **半ラインのずれ**: 3 方式を実装し、呼び出し側がモードを選ぶ。
  - PLAIN: ずれの推定は動き推定に任せる。
  - SHIFT: 既知のずれを補償する。
  - SAME: 同じ偶奇のフィールドだけを使う。
  - ずれの符号と量は、輝度 −0.5 フィールド行・4:2:0 の色差 −0.5 色差フィールド行。作業者と検証者の両方が測定で最小を確認した。
- **C ABI**: `nvc_temporal_shift` だけを追加した。NVC_ABI_VERSION は 3 のまま (追加のみ。repro/nv_native.py が版 3 ちょうどを要求する。検証者も妥当と判断)。

### 試験 (観測)
- **プログレッシブ経路は不変。**
  - 検証者が旧と新の nv_v5_test を直接比べ、570 件・453 MB の出力の sha256 が同一だった。libnvcore も 162 件で新旧同一。
  - `make -C native verify` 14 本、verify_bridge (ALL MATCH)、selftest 130/130、verify_ofx --quick (不一致 0)、書き出し 19/19 SAME (main の libnvcore に向けても SAME)。
  - クリーンビルドは warning 0。
- **フィールド経路は C++ と Python の参照 (`native/fields_ref.py`) がビット一致。**
  - temporal(shift) 48/48、process_frame_fields 96/96、実素材 18/18、ブリッジ 24/24。
  - 検証者は、自分で書いた参照 (36/36・48/48) と、作業者の 8 種・検証者の 9 種の変異 (1ulp のずれまで) で照合の感度も確かめた。
- **統括の再実行**
  - `make -C native nv_v5_test lib` は warning 0。verify_fieldproc 全件 ALL BIT-EXACT。
  - scratch での qtui クリーンビルドは warning 0、selftest 130/130。
  - verify_ofx --quick は不一致 0。ini は変化なし。

### 効果 (合成の 60p → tinterlace、σ6、TFF。BFF もほぼ同じ。作業者と検証者で一致)

| 処理 | Y 静止 | Y 動き | comb 動き |
|---|---|---|---|
| 入力 | 6.01 | 6.02 | 7.38 |
| (a) フレーム単位 R2 | **1.24** | 6.40 | 11.23 |
| (b) PLAIN R2 | 1.97 | 3.81 | 4.69 |
| (b) SHIFT R2 | 1.93 | 3.92 | 4.84 |
| (b) SAME R2 | 1.64 | **3.81** | 4.78 |
| (b) SAME R4 | 1.35 | 4.05 | 5.13 |

- **動き部は大幅に減った**: Y −40%、comb −58%。
- **静止部は減らず、悪化した**: +9〜60%。
  - 検証者の判断は、実装の誤りではなく仕様の想定違い。
  - 根拠: 静止部ではインターレースのフレームは progressive と同じ画像で、60p をそのまま v5 で処理した対照でも 1.204 だった。フレーム単位 (1.215) はすでに上限に近い。
  - 仕様 4 の合格条件は「動き部と comb が大幅に減り、静止部の悪化は X% 以内」に改めるのが妥当 (検証者の提案)。

### 残る点 (INT-3 以降へ)
- **L-a**: 4:2:2・4:4:4 のインターレース素材を qtui の read_frames (`-pix_fmt yuv420p`) で読むと、フィールドの色差が混ざる。INT-3 で `scale=interl=1` 相当が必須。
- **L-b**: 既定のモードと半径 (SAME R4、動き適応の切り替えなど) は、INT-3 で決め、INT-4 の本家比較で確かめる。
- **L-c**: 既存の問題 (`make verify JOBS=N` が止まる、compare_export の ROOT/repro) は未修正。
- **情報**: 動き部の色差の悪化は v5 に元からある性質 (progressive の対照でも出る)。
- `INT-2/` 以下は .gitignore の対象なので、追跡するには `git add -f` が要る。

## INT-3: プロファイル・UI・書き出しのフィールド対応 (2026-11-02、作業者 int2-worker、独立検証 verify-int2 Round 2 **APPROVE**)
- 作業場所は INT-2 と同じ worktree で、INT-2 の上に重ねた。**未コミット**。native/ と ofx/ は変更していない。
- 作業者報告 (Round 1 の 3 通と Round 2) の転記: `INT-3/REPORT_DRAFT.md`。
- 独立検証: `INT-3/verifier/VERDICT_round1.md` (REQUEST_CHANGES) と `VERDICT_round2.md` (APPROVE)。
- 変更前の控えは `INT-3/backup/`・`backup_r2/`、ログは `INT-3/logs/`。

### 実装 (作業者の報告から)
- **走査方式**: 下段の「P ▾」で選ぶ。選択肢は Progressive / Interlaced TFF / Interlaced BFF。
  - 既定は INT-1 の自動判定。順序が不明なら、本家の文言で利用者に尋ねる。
  - 走査方式を変えると、本家の文言を出してプロファイルをリセットする。
  - OFX モードでは progressive に固定する (INT-4 の範囲)。
- **プロファイル**: Two fields / One field (top・bottom) を作れる。JSON の base に `"fields"`・`"field"` を足す。progressive では書かないので、既存の JSON と照合は変わらない。
  - Two fields の定義は本家で未解読のため、こちらで決めた。領域は時間で最初のフィールド、曲線は A_f・B_f・A_{f+1} の中央値、pair は同じ偶奇。
- **処理**: インターレースのときだけ、処理パラメータに `"interlace"` を足し、INT-2 のフィールド経路を通す。対象はプレビュー・NLC・Profile Check・書き出し。
- **読み込み**: ソースと処理の形式で色差の縦の間引きが違うときは、`interl=1` を付けて読む。4:2:2・4:4:4・RGB → 4:2:0 と、4:2:0 → RGB (32-bit float を含む) が該当する。
- **書き出しの旗**: すべての形式に setfield を付ける。x264 は +ilme+ildct と tff/bff、ProRes は +ildct。x265 と EXR は旗を書けないので警告を出す。
- **仮の既定**: mode は SAME (同じ偶奇)、半径は「スライダーはフレーム数、内部では 2R フィールド」。Preferences の「Interlaced clips (fields)」で、モードと本家式の数え方 (R = フィールド数) に変えられる。キーは `interlace/` に置いた。

### 独立検証
- **Round 1 (REQUEST_CHANGES)**
  - M-1: 32-bit float では、4:2:0 のインターレース素材に interl=1 が付かず、色差のフィールドが混ざっていた。RGB の 99% の標本で差が出ていた。
  - L-1: 照合の弱さ。手動の領域、ストリームの field_order、奇数の半径の誤りを素通りしていた。
  - L-2: 順序を尋ねた後に Auto Profile が走らない。読み直しの際に文言が出ない。
  - L-3: Undo と走査方式の食い違い。
- **Round 2 (APPROVE)**
  - M-1・L-1・L-2 が直っていることを確認した。
  - 検証者の変異 8 種のうち 7 種を検出した。残る 1 種は出力が等価な変異。
  - interlaced_vf の条件表 90 通りを確かめた。
  - プログレッシブは、変更前の C++ と 82 ファイルで同一だった。
  - compare_export 19/19 SAME、selftest 137/137、verify_ofx --quick 不一致 0、クリーンビルドの warning 0。ini は変化なし。

### 統括の再実行 (観測)
- **Round 1 の後**: クリーンビルドは warning 0、selftest は 136/136、verify_interlace は ALL OK、verify_ofx --quick は不一致 0、ini は変化なし。
- **Round 2 の後**: クリーンビルドは warning 0、selftest は 137/137、verify_interlace は ALL OK (depth 32 も BIT-EXACT)、ini は変化なし。

### 残る点 (INT-4 へ)
- L-3: Undo と走査方式の食い違い。
- 本家と照合していない設計の決定: Two fields の定義、既定の mode と半径の数え方。
- 静止部がフレーム処理より約 9% 悪い (INT-2 の L-b)。
- Batch Profiler はフィールドに対応していない。
- 情報: 440 の RGB 読み込みには interl=1 が付かない (まれな形式)。

## INT-4: OFX のフィールド対応 + 本家比較 (2026-11-02)

### INT-4 の 1: OFX のフィールド対応 (作業者 int2-worker、独立検証 verify-int2 Round 2 **APPROVE**)
- 作業場所は INT-2/3 と同じ worktree で、その上に重ねた。**未コミット**。native/ は変更していない。
- 作業者の報告 (1/2・2/2・既定の変更・注記・Round 2) の転記: `INT-4/REPORT_DRAFT.md`。
- 独立検証: `INT-4/verifier/VERDICT_round1.md` (REQUEST_CHANGES) と `VERDICT_round2.md` (APPROVE)。
- 変更前の控えは `INT-4/backup/`・`backup_defaults/`・`backup_r2/`、ログは `INT-4/logs/`。

**実装 (作業者の報告から)**
- **走査方式の決め方**: 利用者がヘルパーのボタンで決める。本家の観測に合わせ、Resolve のクリップ属性には従わない。
  - ホストのフィールド順 (kOfxImageClipPropFieldOrder) は、ウィンドウの既定値にだけ使う。
  - OFX の行は下から数えるので、偶数の高さでは Upper → TFF、Lower → BFF になる。
- **ClipParams**: インターレースのときだけ version 2 にし、`"scan"` と `render.P["interlace"]` を持たせる。progressive は version 1 のままで、1 バイトも変わらない。
  - 読む側は v1 と v2 を受け付け、組み合わせの合わないものは拒否する。古いプラグインは「version 2 is not supported」で止まる。
- **レンダー**: render.P["interlace"] があれば、プラグイン内でフィールド単位に処理する (C ABI のみ。qtui の process_fields と同じ手順)。
  - FieldToRender が Lower / Upper なら、その行だけを書く。
  - 1 フィールドだけの画像は拒否する。
  - ホストがインターレースと言うのに走査方式が Progressive なら、フレーム単位で処理して警告を出す。
- **既定から作ったインスタンス**: version 1 で frame_size が null の ClipParams (既定から作ったもの) では、走査方式を変えない。そのため、ホストのフィールド順が既定として残る (Round 2 の M-1 修正)。
- **既定 (統括の決定)**: mode = same、Temporal Radius はフィールド数で数える (R2 → ±2 フィールド)。根拠は下の本家比較 (`work/omo/wp_e9/eval/E9_interlaced_findings.md`)。
- **L-3 (Undo と走査方式)**: 走査方式の変更でプロファイルをリセットしたら、Undo の履歴を消す。
- **本家との違いの注記**: 走査方式のボタンとメニュー項目のツールチップに repro の注記 (SCAN_REPRO_NOTE) を付けた。本家の選択肢は Progressive / Interlaced の 2 択で、順序は選べない。

**独立検証 (観測)**
- **progressive は不変**: main からビルドした変更前のバンドルと比べた。
  - 出力 .f32 は 61/61 で同一。
  - ヘルパーの ClipParams は 46 件、frames_needed・status などは 28 件でバイト一致。
- **照合の感度**: 検証者の変異 6 種 (Round 1) と 4 種 (Round 2) を、すべて検出した。奇数の高さ、ヘルパーの M-1 の後退、version の組み合わせを含む。
- **version の組み合わせ**: 12 通りすべて期待どおり。古いプラグインは version 2 で止まる。
- **回帰**:
  - verify_ofx --quick は 109 比較で不一致 0。
  - verify_modal、verify_interlace、compare_export 19/19、selftest 138/138 はすべて合格。
  - クリーンビルドの warning 0。ini は変化なし。

**統括の再実行 (観測)**
- Round 1: クリーンビルドは warning 0、selftest は 138/138、verify_ofx --quick は 101 比較で不一致 0、ini は変化なし。
- Round 2: クリーンビルドは warning 0、selftest は 138/138、verify_ofx --quick は 109 比較で不一致 0 (I-odd-* と I-default-host を含む)、ini は変化なし。

**残る点**
- Low: 既定のフレームのプロファイルのままフィールド処理したときに、レンダー側の警告がない。ヘルパーのウィンドウには注記が出る。
- 情報: 1 フィールドだけの画像の拒否は、コードを読んで確認しただけ。
- 未確認: Resolve の「インターレース処理を有効化」のとき、ホストがどう渡すか。本家もその設定では黒か無処理だった。

### INT-4 の 2: 本家との比較 (統括が Resolve 21.1.1 で本家 6.1.3 Pro を操作、比較は作業者 e9-prep)
- **素材**: Xiph derf の SVT 1080p50 (park_joy、old_town_cross) と Sintel 予告編 (CC-BY 3.0) から、きれいな 50p/24p を組んで 1080i の TFF にし、RGB で σ3/σ6 のガウス雑音を足した。
  - 正解 (きれいな元画像) との比較で評価した。
  - 置き場所は `work/omo/wp_e9/` (git の追跡外)。
- **本家の処理条件**:
  - Resolve のインターレース処理は無効。有効にすると、本家の出力は黒か無処理だった。
  - Neat の Video Scan Type = Interlaced、Profiled with: Two fields (1920x540x2)、Auto Profile、フィルタは既定。
  - Resolve の読み書きの誤差は 0 (効果なしの書き出しが入力とバイト一致)。
- **結果** (Y の PSNR [dB]。当方のフィールド処理は新しい既定の same / fields):

| クリップ | 入力 | 当方 (フレーム単位) | 当方 (フィールド単位) | 本家 |
|---|---|---|---|---|
| Sintel σ3 | 39.01 | 33.14 | 41.32 | 44.83 |
| Sintel σ6 | 33.08 | 32.55 | 37.24 | 39.81 |
| old_town_cross σ6 | 32.56 | 33.99 | 34.29 | 35.80 |
| park_joy σ6 | 32.64 | 23.80 | 31.90 | 34.17 |

- **コーミング比** (大きく動く区分、正解 = 1): 本家 0.84〜0.99、当方のフィールド単位 0.60〜0.96、フレーム単位 0.06〜0.63。
- 本家とは 1.2〜3.5 dB の差が残る。差の形は、どの mode・半径でもほぼ同じだった。
  - 推測: プログレッシブの比較と同じ原因 (処理コアの較正) とみられる。具体的には、雑音の見積もりの過大、色差の処理の強さ (Cb 3 倍・Cr 4 倍)、低域の除去。
- 本家の比較の限界:
  - 本家のフィールド処理の内部は解読されていないので、数値の完全一致ではなく水準の比較である。
  - 素材は 4 組だけ。
- 詳細は `work/omo/wp_e9/eval/E9_interlaced_findings.md`・`E9_progressive_findings.md`、本家の画面は `work/omo/wp_e9/genuine/obs/`。
