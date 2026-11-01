## Verification Report — INT-4 の 1 Round 2（M-1・L-1・L-2 への対応）

### Verdict
**Status**: PASS（判定 **APPROVE**）
**Confidence**: high
**Blockers**: 0（新しい指摘は Low 1 件と情報 2 件）

M-1・L-1・L-2 はどれも直っていて、回帰で新たに壊れたものはありませんでした。

作業場所は `/private/tmp/claude-501/verify-int4r2/` です。
- worktree から同期した写しで、qtui と OFX をビルド物なしからビルドしました。
- 変更前のバンドルは、前回と同じく main からビルドしたもの（`/private/tmp/claude-501/verify-int2/base/ofx/build/`）を使いました。
- worktree は書き換えていません（`git status` 41 行のまま、ソースは写しと同一）。
- ini の sha256 は、検証中ずっと `45d4258c…fcc6` です。
- Resolve、/Library/OFX/Plugins、本家、ユーザーの MeatVideoQt には触れていません。

### Evidence（どれも検証者が自分で実行した結果）

| Check | Result | Command/Source | Output |
|---|---|---|---|
| 変更されたファイル | 確認 | 前回の写しと `diff` | ofx_session.{h,cpp}、params.{h,cpp}、verify_ofx.py だけ |
| native・reference | 変更なし | `cmp`、`git diff HEAD -- reference` | 差 0。make verify は省いた（native が検証済みの版と同一のため） |
| クリーンビルド | pass | qtui と `build_ofx.sh`、対照の `MV_MODAL_WAIT=OFF` | すべて rc=0、warning 0 |
| selftest | pass | offscreen | 138 passed / 0 failed |
| verify_ofx --quick | pass | `--work vo_new` | 109 比較で 0/89,831,040 語の不一致、ALL MATCH。ini は unchanged |
| **変更前のバンドルとの比較（progressive）** | pass | 前回の vo_base と今回の vo_new | 下記 L-2 |
| verify_modal --control | pass | 対照のバンドルを作り直して実行 | rc=0、すべての ok。open time の比は ≦1.10 |
| verify_interlace | pass | 新しい qtui_export | ALL OK |
| compare_export（写し） | pass | 新しい qtui_export | 19/19 SAME |
| qtui の書き出しを変更前と比較 | pass | INT-2 時点の C++ 出力と比べた | 82 ファイルで差 0 |
| M-1 のシナリオ | pass | 検証者の `myofx.py`（DEFTEST） | 下記 M-1 |
| version の組み合わせ（新しいプラグイン） | pass | 検証者の `combos.py` | 12 通りすべて期待どおり（下記 L-2） |
| version 2 を古いプラグインに渡す | pass | 変更前のバンドルで `combos.py` | 下記 L-2 |
| 検証者の変異 4 種 | 全種検出 | `omut2.sh` | 下表 |

**検証者の変異 4 種**

| 変異 | 検出した照合 | 結果 |
|---|---|---|
| NONE（変更なし） | — | FAILLIST が空、combos も期待どおり |
| P_ODD_PAD（奇数の高さで、下フィールドの詰め行の元を H−1 にする） | 作業者の **I-odd-upper・I-odd-lower・I-odd-both** | 43,248〜85,622 語の不一致 |
| P_V2_FIRST（interlace.first と scan の食い違いを見逃す） | 検証者の combos | 「v2 first != scan」が status 0 で通る。verify_ofx の拒否のケースにも「version 2 first != scan」がある（読みで確認） |
| H_M1_REVERT（ヘルパーの M-1 の修正を元に戻す。ヘルパーを作り直した） | 作業者の **I-default-host** | scan / fields が (None, None) で FAIL |

作業者は、M-1 の修正をヘルパーの変異では確かめていませんでした（報告の「未対応」）。H_M1_REVERT がその穴を埋めていて、I-default-host が修正の後退を実際に検出することを確かめました。

### Acceptance Criteria

| # | 基準 | Status | Evidence |
|---|---|---|---|
| 1 | M-1: 既定の ClipParams のときはホストの順序を残す | VERIFIED | 下記 M-1 |
| 2 | L-2: インターレースのときだけ version 2 にする | VERIFIED | 下記 L-2 |
| 3 | L-1: 奇数の高さの I-odd-* が P_ODD_PAD を検出する | VERIFIED | 変異の表のとおり |
| 4 | 回帰 | VERIFIED | Evidence の表のとおり |

**M-1**
- V-defaults-host（ini "set"、ホストは OfxFieldLower、Prepare で Auto Profile → Apply）: ClipParams は version 2、**scan bff、fields two** になりました。レンダーも参照と MATCH しました。
- frame_size を持つ古い progressive のインスタンス（version 1、ホストは Lower）:
  - Adjust → Apply: ClipParams は unchanged（version 1、scan なし）で、ウィンドウは Progressive に戻りました。
  - Prepare → Auto → Apply: version 1、scan なし、frame のプロファイルでした。
  - どちらのレンダーにも、ホストがインターレースと言うときの警告が出て、参照と MATCH しました。
- 読んで確認したこと（`apply_clip_params`）: version 2 なら scan に従い、version 1 で frame_size があれば Progressive、version 1 で frame_size が null なら走査方式を変えません。

**L-2**
- progressive の ClipParams が 1 バイトも変わらないこと:
  - ヘルパーが書いた ClipParams 46 件が、変更前とバイト一致しました。
  - 合成の progressive の ClipParams は、すべて同じ内容が新しい側にもありました（cp_badversion は、意図的に version 2 から 3 に変えたもの）。
  - プラグインの出力 .f32 61 件が同一で、ログの frames_needed・fetched・status・message 28 件も同一でした。
- version の組み合わせ（新しいプラグイン、`combos.py`）:
  - 通るもの: v1、v2、v2 で render が null（プロファイルなしで素通し）。
  - 止まるもの（すべて status 1 と、内容の分かるメッセージ）: v1 に scan、v1 に interlace、v2 に scan なし、v2 で scan が progressive、v2 に interlace なし、v2 で first と scan が食い違う、v2 で mode が不正、v3、v2 で radius 33。
- 古いプラグイン: version 2 は「ClipParams: version 2 is not supported (1)」（status 1）で止まりました。前回の、黙って progressive で処理する挙動は無くなっています。version 1 は従来どおり処理しました。

### Gaps
- **[Low] I-default-host の組み合わせ（フレームのプロファイルでフィールド処理）にレンダー側の警告がない。**
  - 既定のプロファイルを持つインスタンスで、Auto Profile を作り直さずに Apply すると、scan は bff、プロファイルはフレームのもののまま、フィールド単位で処理されます。
  - ヘルパーのウィンドウには「full-frame profile on an interlaced clip: rebuild on fields」の注記が出ます（読みで確認、INT-3 の on_profile）。ホストに出るレンダーのメッセージは空でした（観測）。
  - INT-2 の測定では、フィールドの処理にフレームのプロファイルを使うと、静止部が悪くなる場合がありました（σ3 で 1.03 → 1.24）。
  - 処理を止める必要はないと考えます。Apply のときか、レンダーのメッセージで「フィールドで作り直すこと」を勧める注記を出すことを推奨します（任意）。
- **[情報 1]** 1 フィールドだけの画像の拒否は、今回も読みでの確認だけです（minihost では作れないため）。
- **[情報 2]** P_V2_FIRST は、検証者の combos で検出しました。verify_ofx の拒否のケースにも同じ内容（version 2 first != scan）が入っていることは読みで確かめましたが、変異したバンドルで verify_ofx 全体は回していません。

### Recommendation
**APPROVE。** M-1 は既定からの新しいインスタンスと古い progressive のインスタンスの両方で正しく動き、L-2 は 12 通りの組み合わせと古いプラグインで期待どおりでした。L-1 の I-odd-* は P_ODD_PAD を検出し、H_M1_REVERT は I-default-host が検出しました。progressive は変更前とバイト一致です。残る Low（フレームのプロファイルでフィールド処理するときの注記）は任意です。

検証者のスクリプトとログは `/private/tmp/claude-501/verify-int4r2/` にあります。
- スクリプト: `my/myofx.py`、`my/combos.py`、`my/omut2.sh`
- ログ:
  - `my/omut2.log`（変異）、`mut/*/run.log`・`mut/*/combos.log`
  - `my/deftest.log`（M-1）
  - `my/combos_old/`（古いプラグインでの結果）
  - `vo_new.log`、`verify_modal.log`、`selftest.log`、`verify_interlace.log`、`cexp.log`、`build.log`