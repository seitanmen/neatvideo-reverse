## Verification Report — INT-4 の 1（OFX のフィールド対応と既定の変更）独立検証

### Verdict
**Status**: FAIL（判定 **REQUEST_CHANGES**）
**Confidence**: high
**Blockers**: 1（Medium 1）。ほかに Low 3、情報 2

差し戻しの理由は 1 件で、修正は小さいです（M-1）。
- Default Profile か Default Preset を割り当てている利用者では、新しいインスタンスのウィンドウが、ホストのフィールド順を無視して Progressive で始まります。
- これは、作業者の設計（報告 1/2 の 6「既定はホストの順序」）とコードの注釈に反します（観測）。

それ以外は確認できました。
- progressive の OFX は、変更前のバンドル（main からビルドしたもの）と、出力・ClipParams・GetFramesNeeded のすべてで同一でした。
- フィールド処理は、検証者の独自のシナリオ（奇数の高さを含む）と変異 6 種でも正しく、照合も空振りしていません。

作業場所は `/private/tmp/claude-501/verify-int4/` です。
- worktree の写しを `new/` に、main からビルドした変更前のバンドルを `/private/tmp/claude-501/verify-int2/base/ofx/build/` に置きました。
- worktree は書き換えていません（`git status` 41 行のまま）。
- ini の sha256 は、検証中ずっと `45d4258c…fcc6` です。
- Resolve、/Library/OFX/Plugins、本家、ユーザーの MeatVideoQt には触れていません。

### Evidence（どれも検証者が自分で実行した結果）

| Check | Result | Command/Source | Output |
|---|---|---|---|
| native に変更がないこと | pass | `cmp`（ソースと Makefile） | 差 0。INT-2 と Round 1 で `make verify` rc=0 を確かめた版と同一なので、make verify は省いた |
| reference に変更がないこと | pass | `git diff HEAD -- reference` | 空 |
| qtui のクリーンビルドと OFX（`build_ofx.sh`） | pass | 新しいディレクトリでビルド | rc=0、warning 0 |
| 変更前のバンドルのビルド | pass | main の qtui と ofx | warning 0 |
| 対照のバンドル（`MV_MODAL_WAIT=OFF`） | pass | 自分でビルド | warning 0 |
| selftest | pass | offscreen | 138 passed / 0 failed。int4ScanChangeClearsUndo も PASS。作業者がツールチップの文言を変えた後に作り直して、もう一度 138/138 |
| verify_ofx --quick（新） | pass | `--work vo_new` | 101 比較で 0/84,769,280 語の不一致、ALL MATCH |
| verify_ofx --quick（変更前） | pass | 変更前のバンドルと変更前の verify_ofx | 81 比較で 0/67,028,480 語の不一致 |
| **progressive の OFX を変更前と直接比較** | pass | 両方の `--work` の出力を突き合わせた | 下記 1 |
| verify_modal --control | pass | 対照のバンドルを使用 | rc=0。7 ケースすべて ok。1080p の open time の比は 1.01 / 1.10 |
| verify_interlace | pass | 新しい qtui_export | ALL OK |
| compare_export（写し） | pass | 新しい qtui_export | 19/19 SAME |
| qtui の書き出しを変更前と比較 | pass | INT-2 時点の C++ 出力と比べた | 82 ファイルで差 0 |
| 検証者の OFX シナリオ | pass | `my/myofx.py`（verify_ofx の仕組みを使った独自のシナリオ） | 下記 2 |
| 検証者のプラグインの変異 6 種 | 全種検出 | `my/omut.sh` | 下表 |
| 既定の ClipParams とホストの順序 | **fail** | `my/myofx.py`（DEFTEST） | M-1 |
| 変更前のプラグインに新しい ClipParams を渡す | 観測 | 変更前のバンドルで I-* を実行 | L-2 |

**検証者のプラグインの変異 6 種**

変異を入れない版（NONE）は FAILLIST が空でした。

| 変異 | 検出した照合 |
|---|---|
| P_JOIN_SWAP（再結合で上下を取り違える） | I-synth・I-synth-none などで 691,200 語の不一致 |
| P_FN_RADIUS（GetFramesNeeded でフレームの半径を使う） | I-synth・I-field-* で「undeclared frames」 |
| P_FTR_INV（要求と逆のフィールドの行を書く） | I-field-* と V-odd の画素の不一致、検証者の行の検査 |
| P_MODE_MAP（same を plain と読む） | I-synth など 20 行 |
| P_ODD_PAD（奇数の高さで、下フィールドの詰め行の元を変える） | **検証者の奇数の高さのシナリオだけ**（V-odd-upper・V-odd-both）。verify_ofx の I-* は検出しない（L-1） |
| P_WARN（ホストがインターレースと言うときの警告を消す） | I-synth の警告の検査 |

### Acceptance Criteria

| # | 基準 | Status | Evidence |
|---|---|---|---|
| 1 | progressive の OFX が変わらない | VERIFIED | 下記 1 |
| 2 | OFX の行の向きと Upper/Lower の対応（奇数の高さを含む） | VERIFIED | 下記 2 |
| 3 | FieldToRender、1 フィールドの画像、ホストがフィールド情報を渡さない場合、Progressive のままの場合 | VERIFIED（1 フィールドの画像の拒否だけは読み） | 下記 3 |
| 4 | プラグインの process_fields と照合の空振り | VERIFIED | 下記 4 |
| 5 | ヘルパーと ClipParams | PARTIAL | 下記 5 |
| 6 | L-3（Undo の履歴を消す） | VERIFIED | 下記 6 |
| 7 | 既定の変更 | VERIFIED | 下記 7 |
| 8 | 回帰 | VERIFIED | 上の Evidence の表のとおり |

**1. progressive の OFX**
- 変更前と新しい版で、共通のシナリオの出力を突き合わせました。
  - プラグインの出力 .f32: 61/61 がバイト単位で同一。
  - ClipParams（cp_*.json）と押下ごとの ClipParams: 同一。
  - minihost のログから frames_needed・fetched・status・message を抜き出して比べると、28/28 で同一。
  - 違ったのはハーネスのメタデータだけです（spec/ref の json に新しく足された `field_to_render: None` と `fields: None`、作業ディレクトリのパス、ヘルパーの写し）。
- 読んで確認したこと: progressive では、GetFramesNeeded の半径は従来の `s->radius` のままです。process_frame は `temporal_stage` と `finish_stage` に分けただけで、nvc_temporal・nvc_spatial・nvc_mix の呼び方は同じです。出力が同一であることとも合っています。

**2. 行の向きと Upper/Lower の対応**
- vendor の ofxImageEffect.h によると、Lower は rows 0,2,4…、Upper は rows 1,3,5… で、OFX の行は下から数えます。
- したがって、偶数の高さでは Lower が BFF、Upper が TFF です。I-press-host（Lower → scan bff）と、検証者の V-even-lower-bff（書かれた行が上から 1,3,…,359）で確かめました。
- 奇数の高さ（97 行）:
  - Lower なら上から 0,2,…,96 の 49 行、Upper なら 1,3,…,95 の 48 行が書かれ、OFX の定義どおりでした。
  - 画素も参照と MATCH しました（V-odd-lower・V-odd-upper・V-odd-both）。

**3. FieldToRender とホストの情報**
- FieldToRender を指定すると、要求されたフィールドの行だけが書かれ、ほかの行はホストのゼロのままです（I-field-* と検証者の行の検査）。
- ホストがフィールド情報を渡さない場合（I-synth-none）は、ClipParams に従ってフィールド処理しました。
- ホストがインターレースと言うのに走査方式が Progressive の場合（I-synth の最後のレンダー）は、フレーム単位で処理して警告を出しました。
- 1 フィールドだけの画像の拒否（`fetch` が kOfxImagePropField の Lower / Upper で失敗を返す）は、コードを読んで確認しただけです。試験はありません（情報 1）。

**4. process_fields と照合の空振り**
- `core.cpp` の process_fields は、qtui の fields.cpp と同じ手順でした（観測）。偶奇は j ^ first、SAME の選び方、ずれ 0.5·(q − parity)、nvc_temporal_shift / finish、再結合の 5 点です。
- I-* の 20 比較と、検証者の V-* の 10 比較が、どれも参照と一致しました。
- 変異は 6 種すべてを検出しました（上表）。

**5. ヘルパーと ClipParams**
- 押下で ClipParams に scan と P["interlace"] が書かれ、2 回目の押下では scan が復元されました。Cancel でインスタンスが変わらないことも確かめました（I-press-user）。
- 既定の ClipParams がある場合は M-1 のとおりです。
- version を 1 のまま据え置いた判断は、L-2 にまとめました。

**6. L-3**
- 走査方式を変えると `if (reset) state->undo->clear()` を通ります。読み直しの経路では、install_frames が履歴を消します（`mainwindow.cpp:1042`）。
- 履歴を消す方針は妥当だと考えます。走査方式を Undo の単位にすると、動画の読み直しまで巻き戻すことになるからです。
- 食い違った状態に戻れる経路は、利用者が明示的に「Load Profile」で別の種類のプロファイルを読む場合だけでした。このときはステータスに注記が出ます（読みで確認）。

**7. 既定の変更**
- 既定は same で、半径はフィールド数で数えます。押下で作られた ClipParams にも radius 2 が入りました（I-press-*）。
- progressive への影響はありません（compare_export、verify_ofx、qtui の書き出しの比較のとおり）。
- 設定に `frames` を保存した利用者は、その値が残ります。INT-3 は公開されていないので、該当するのは試験した人だけのはずです（推測）。

### Gaps

- **[Medium] M-1 既定の ClipParams があると、ホストのフィールド順が既定として使われない。**
  - 観測（検証者のシナリオ V-defaults-host: ini は "set"、ホストは OfxFieldLower、Prepare で Auto Profile → Apply）:
    - ClipParams の scan / fields が **(None, None)** になりました。期待は (bff, two) です。
    - レンダーには「host reports interlaced material; … Progressive」の警告が出ました。
  - 原因（読み）:
    - `apply_clip_params` は、`"scan"` が無い ClipParams を "progressive" として扱い、`set_scan(-1)` を呼びます。
    - 新しいインスタンスの ClipParams は、Default Profile / Preset から作られ（`ofx_default_main`）、scan を持ちません。
    - そのため、`ofx_session_main` が直前に `set_scan(s->field_order)` で設定したホストの順序が上書きされます。
    - コードの注釈「a new instance (or ClipParams without "scan") starts from the host's field order」と食い違います。
  - 修正案:
    - `"scan"` が無く、かつ `frame_size` が null の ClipParams（既定から作ったもの）では、走査方式を変えない。
    - `frame_size` を持つ、適用済みの古い progressive のインスタンスは、従来どおり Progressive に戻す。
    - verify_ofx に「既定あり＋フィールド順あり」のシナリオを足す。
- **[Low] L-1 verify_ofx の I-* はすべて偶数の高さ（clip A、360 行）。**
  - P_ODD_PAD を検出できません。今のコードは、検証者の奇数の高さのシナリオで正しいことを確かめました。
  - 奇数の高さで FieldToRender upper/lower を使うシナリオを足すことを推奨します。
- **[Low] L-2 ClipParams の version 1 据え置きの副作用。**
  - 観測: 変更前（main）のプラグインに、新しいインターレースの ClipParams を渡しました。
    - ホストがフィールド情報を渡さない場合（I-synth-none）は、status 0 で**黙って progressive として処理**しました（691,199 語の違い、メッセージなし）。
    - フィールド情報がある場合は、従来どおり拒否しました。
  - 新しい ClipParams で保存したプロジェクトを古いプラグインで開くと、気づかずに画質が変わります。
  - 案: インターレースのときだけ version 2 を書く。progressive は 1 のままにすれば、バイト一致は保てます。古いプラグインは「version 2 is not supported」で止まります。
  - 未公開でプラグインとヘルパーを一緒に配るので、Low としました。
- **[Low] L-3（INT-3 由来、変化なし）** Batch Profiler と、Match の比べ方の未対応は、作業者の記録どおりです。
- **[情報 1]** 1 フィールドだけの画像の拒否は、minihost で試せないので未試験です（読みで確認）。
- **[情報 2]** 走査方式の注記は届いていました。
  - ボタンのツールチップと、メニューの各項目のツールチップ（`SCAN_REPRO_NOTE`）に入っています。
  - この変更は検証の途中（03:27）に入りました。取り込んで作り直し、selftest 138/138 を確認しました。
  - 文言だけの変更なので、OFX の照合（それより前の版で実施）には影響しないとみています（推測）。

### Recommendation
**REQUEST_CHANGES。** M-1（既定の ClipParams のときはホストのフィールド順を残すことと、その試験）を直せば、APPROVE に足りると考えます。L-1（奇数の高さの I-* シナリオ）と L-2（インターレースのときだけ version 2 にする）は、同時に入れることを推奨します。

検証者が書いたものとログは `/private/tmp/claude-501/verify-int4/` にあります。
- スクリプト: `my/myofx.py`、`my/omut.sh`
- ログ:
  - `my/omut.log`
  - `my/deftest.log`（M-1）
  - `my/oldplug.log`（L-2）
  - `vo_base.log` / `vo_new.log`（verify_ofx の変更前・新）
  - `verify_modal.log`
  - `selftest.log` / `selftest2.log`
  - `verify_interlace.log`
  - `cexp.log`