## Verification Report — INT-3 (プロファイル・UI・書き出しのフィールド対応) 独立検証

### Verdict
**Status**: FAIL（判定 **REQUEST_CHANGES**）
**Confidence**: high
**Blockers**: 1（Medium 1）。ほかに Low 3、情報 3

差し戻しの理由は 1 件だけで、修正は小さいです（M-1）。
- Preferences の Bit depth を 32-bit float にすると、4:2:0 のインターレース素材の読み込みに `interl=1` が付きません。プレビューと書き出しの両方で、色差のフィールドが混ざります。
- インターレースの素材として最も多い 4:2:0 で、利用者が選べる設定で起きます。警告も出ません。

それ以外（プログレッシブが変わらないこと、フィールド処理のビット一致、旗、UI）はすべて確認できました。

作業場所は `/private/tmp/claude-501/verify-int3/` です。worktree の写しを `new/` に置き、qtui はビルド物を消してから同期しました。
- worktree の `git status` は、検証の前後とも M 20 件・?? 9 件で変わっていません。
- `~/.config/NeatRepro/nv_ui_qt.ini` の sha256 は、検証の前・selftest の後・verify_ofx の後・最後のいずれも `45d4258c…fcc6` です。
- 本家、Resolve、/Library/OFX、ユーザーの MeatVideoQt には触れていません。

### Evidence（どれも検証者が自分で実行した結果）

| Check | Result | Command/Source | Output |
|---|---|---|---|
| native に変更がないこと | pass | worktree の native と INT-2 で検証した写しを `cmp` | 差 0。reference/ と ofx/ の diff も空 |
| `make -C native verify` | pass | 写しで実行 | rc=0。fieldproc は ALL BIT-EXACT、match 1107 ALL MATCH など（10 分 12 秒） |
| verify_bridge | 再実行していない | 判断の根拠は右欄 | native と reference が INT-2 で ALL MATCH を出したときから 1 バイトも変わっていない。qtui はこの照合の対象外 |
| qtui のクリーンビルド | pass | 新しいディレクトリで cmake → build | warning 0 |
| qtui_selftest | pass | `QT_QPA_PLATFORM=offscreen` | 136 passed / 0 failed。int3 の 6 項目も PASS |
| compare_export（写し） | pass | REPRO を reference/python に直した写し。新しい qtui_export | 19/19 SAME、RESULT ALL SAME |
| 書き出し: 変更前の C++ と直接比較 | pass | INT-2 時点の qtui_export（main と同じ qtui）の出力と比べた | 74 ファイルと EXR 15 枚で差 0（詳細は下記） |
| OFX | pass | `build_ofx.sh` → `verify_ofx.py --quick` | warning 0。81 比較で 0/67,028,480 語の不一致。ini は変化なし |
| verify_interlace.py | pass | 新しい qtui_export と qtui_fieldprof | ALL OK（詳細は基準 2） |
| 検証者の追加の書き出し | pass | `extra.py` | 4 本すべて fields_ref と 0 バイト差（詳細は基準 2） |
| プレビューの選択範囲 | pass | 検証者の C++ プログラム `qtui_vint3 crop` | 128/128 で完全一致（詳細は基準 2） |
| 手動の領域の変換 | pass | `area_chk.py` | 6/6 で dict の全値が一致（詳細は基準 3） |
| 検証者の変異 7 種 | 下表 | `qmut.py`（3 回目） | 下表 |
| 32-bit float での読み込み | **fail** | 下記 M-1 | RGB 345,600 標本のうち 341,732 が異なり、最大 92（8 ビット換算） |

書き出しの直接比較で突き合わせたもの:
- mp4・ts・mov・EXR はバイト単位。
- mkv は framemd5 とファイル長。
- sidecar の json は seconds と src を除いて比べた。

`extra.py` の 4 本:
- yuv444p の BFF 素材を interl=1 で読んだもの。
- 奇数の半径: same R3、shift R5、plain R1（R はフィールド数。「Temporal Radius counts: fields」で選べる設定）。

プレビューの選択範囲の照合について:
- 期待値は、検証者が自分で書いたものです。範囲を 4 行単位に広げ、process_fields を掛けてから元の行を切り出します。
- 範囲は TFF・BFF、same・shift、y0 が 0〜6・30・42・1・3、h が 48・50・64・98 の組み合わせです。
- 結果は 128/128 で完全一致しました。全画面処理との内部の差は、平均 0.4〜1.7 階調です。

**検証者の変異 7 種**

最初の 2 回は、ビルドの古い .o が残って結果が汚れたので捨てました。下表は、すべてを強制的に再コンパイルした 3 回目の結果です。

| 変異 | verify_interlace | 検証者の照合 |
|---|---|---|
| JOIN_SWAP（上下を取り違えて再結合） | 検出 | — |
| TWO_NO_B（Two fields の曲線から B_f を外す） | 検出（profile が DIFFER） | area 3/6 |
| CB_PARITY（Cb だけ逆の偶奇で分ける） | 検出 | — |
| SHIFT_CHROMA0（色差のずれを 0 にする） | 検出 | — |
| AREA_NOHALF（領域の y0 を半分にしない） | **検出しない** | area_chk で検出（2/6） |
| X264_NO_BFF（BFF でも x264 に tff=1 を渡す） | **検出しない** | BFF の mp4・mov のストリームの field_order が `bb` から `tt` に変わる |
| MARGIN（フレームの余白 = R/2） | **検出しない** | extra の R5・R1 で検出（96,664 / 370,826 バイト差） |

変異を入れない元の版は、verify_interlace が ALL OK、area_chk が 6/6、extra が 4 本とも 0 バイト差でした。

### Acceptance Criteria

| # | 基準 | Status | Evidence |
|---|---|---|---|
| 1 | プログレッシブの経路が変わらない | VERIFIED | 下記 1 |
| 2 | インターレースの処理 | VERIFIED（8 ビット・高ビットの 4:2:0 と 4:4:4、4:2:2→4:2:0） | 下記 2 |
| 3 | Two fields / One field と UI | VERIFIED（一部は読みだけ） | 下記 3 |
| 4 | 書き出しの旗 | VERIFIED | 下記 4 |
| 5 | 4:2:2 / 4:4:4 を interl=1 で読む | PARTIAL | 下記 5 |
| 6 | 仮の既定（SAME、2R フィールド） | VERIFIED（仮の既定として妥当） | 下記 6 |

**1. プログレッシブの経路**
- 書き出しは、新しい C++ と変更前の C++ の出力が 74 ファイル（と EXR 15 枚）で同一でした。compare_export も 19/19 SAME です。
- 読んで確認したこと（観測）:
  - `proc_params()` が `"interlace"` を足すのは scan ≥ 0 のときだけです。
  - `build_profile` は、`build_profile_planes` に `&more[0]` と more をそのまま渡しています。
  - Preferences を受け入れたとき、`interlace/` のキーは値が変わったときだけ書きます。
- selftest の int3ProfileJsonFields（progressive のプロファイルに "fields" が無いこと）と、preferencesDialogRows / AcceptWritesKeys は PASS しました。
- OFX も ALL MATCH です。

**2. インターレースの処理**
- `fields.cpp` の field_temporal と field_finish を読み、nvfp::process_frame_fields と同じ流れであることを確認しました（観測）。偶奇は j ^ first、SAME の選び方、ずれ 0.5·(q − parity)、nvcore の temporal の切り出しが sub の中に収まること、の 4 点です。
- verify_interlace（ALL OK）で fields_ref とビット一致したもの:
  - same R4（TFF・BFF）、shift R2、plain R2 + Mix 0.25、範囲指定、4:2:2 の interl=1 読み込み
  - Two fields と One field のプロファイル 4 件（dict の全値）
- 検証者の追加（extra.py）でも、yuv444p の BFF 素材と奇数の半径 R3・R5・R1 で 0 バイト差でした。
- プレビューの選択範囲は、検証者の独立な期待値と 128/128 で一致しました。
- 変異は 7 種のうち 4 種を verify_interlace が検出し、残る 3 種は検証者の照合で検出しました。この 3 種が素通りすることは L-1 にまとめました。

**3. Two fields / One field と UI**
- 手動の領域は y0/2・h/2 に変換されます（area_chk で 6/6、奇数の y0 と h を含む）。
- selftest の int3 の 6 項目が PASS しました（走査方式の判定、リセットと本家の文言、順序が不明なときの質問から BFF を選ぶと interl=1 で読み直すこと、Preferences、書き出しの旗、JSON）。
- 次の 2 点は読んだだけで、実行していません: Undo との食い違い、質問に答えた後の Auto Profile（L-2・L-3）。

**4. 書き出しの旗**

| 出力 | interlaced_frame / top_field_first | stream の field_order |
|---|---|---|
| x264 mkv TFF | 1/1 | tb |
| x264 mkv BFF | 1/0 | bt |
| x264 mp4 TFF | 1/1 | tt |
| x264 mp4 BFF | 1/0 | bb |
| x264 mov 10bit BFF | 1/0 | bb |
| FFV1 8・16bit BFF | 1/0 | bt |
| FFV1 10bit TFF | 1/1 | tb |
| ProRes 10bit TFF | 1/1 | tb |
| ProRes 12bit BFF（検証者が 4:4:4 の素材で） | 1/0 | bt |
| x265 | 0/0 | tb / bt |
| EXR | — | — |

- x265 と EXR は、警告の文言が出ることを確認しました。
- 警告だけの扱いは妥当と判断します。推測ですが、libx265 のインターレースは高さ半分のフィールド画像を入力する方式なので、今の作りにはそのまま合いません。

**5. 4:2:2 / 4:4:4 の interl=1 読み込み**
- 8 ビットの処理（4:2:0）で、4:2:2 と 4:4:4 のソースは interl=1 で読み、ビット一致しました。
- 読み直しは selftest の int3UnknownOrderAsks で PASS しました。
- **32-bit float の処理で 4:2:0 のソースを読むときは interl=1 が付きません（M-1）。**

**6. 仮の既定**
- 既定は SAME で、半径はフレームで数えて 2R フィールドです。フレーム処理の R と同じ時間幅・同じ標本数になり、INT-2 の表（静止部 1.35 / 動き部 4.05）とも合うので、INT-4 までの仮の既定として妥当です。
- 本家式（R = フィールド数）への切り替えは、selftest の int3PrefsFieldMode（radius 2）で確かめました。その設定での奇数の R の書き出しも、検証者の照合でビット一致しました。

### Gaps

- **[Medium] M-1 32-bit float では、インターレースの 4:2:0 素材を読むときに色差のフィールドが混ざる。**
  - 原因（観測）:
    - `export.cpp` の `if (il && vf.empty()) vf = interlaced_vf(...)` と、`mainwindow.cpp` の open_video の `if (sc >= 0 && vf.empty())` は、vf が空のときだけ `interl=1` を足します。
    - 32-bit float（RGB）の処理では、`read_filter`（`export.cpp:543` と `:1107`）が `scale=in_color_matrix=…:in_range=…` を返すので、vf は空になりません。
    - その結果、4:2:0 から RGB への上方変換が progressive の補間になります。
    - `set_scan` が読み直すかどうかも `interlaced_vf` だけで決めるので、この場合は読み直しもしません。
  - 実測（観測）: syn_tff.mp4 を `gbrpf32le` で読むと、interl=1 の有無で RGB 345,600 標本のうち 341,732 が異なりました。最大 92、平均 1.41（8 ビット換算）です。
  - 影響: プレビュー、プロファイル、書き出しのすべて。警告は出ません。
  - 修正案:
    - インターレースで vf がこの scale のとき、`:interl=1` を足す。`set_scan` の読み直しの判定にもこの条件を入れる。
    - verify_interlace に「4:2:0 の素材を depth 32 で書き出す」ケース（reference は interl=1 付きで読んだ RGB float）を足す。
- **[Low] L-1 verify_interlace は 3 種類の誤りを素通りさせる**（今のコードは正しいことを、検証者の照合で確かめた）。
  - 手動の領域を試していない（AREA_NOHALF）。
  - ストリームの field_order を見ていない（X264_NO_BFF では、フレームの旗は正しいまま、mp4・mov のストリームが tt になる）。
  - 偶数の半径しか試していない（MARGIN）。
  - 提案: 領域ありのプロファイル、mp4 と mov のストリームの field_order の期待値（TFF なら tt、BFF なら bb）、奇数の半径（R3・R5）を足す。
- **[Low] L-2 走査方式の質問と切り替えの、細かな食い違い**（コードの読みで、実行はしていない）。
  - 4:2:0 の素材で「順序不明 → 利用者が答える」の流れでは、読み直しをしません。そのため、Auto Profile の設定が有効でも、答えた後に Auto Profile が走りません。
  - 一方、読み直しを伴う切り替えでは、プロファイルを空にしますが、本家の文言の箱を出しません。
- **[Low] L-3 Undo と走査方式の食い違い**（作業者のリスク 3。コードの読み）。
  - Undo すると古いプロファイルが戻りますが、走査方式は戻りません。ステータスに注記が出るだけです。
  - INT-4 までは許容できますが、走査方式を Undo の単位に含めるか、Undo の履歴を切ることを検討してください。
- **[情報]**
  - `build_profile_fields` に 3 要素（正方形）の area を渡すと、辺を半分にしません。UI はいつも 4 要素で渡すので、今は到達しません。
  - インターレースのプレビュー範囲が 17 行のように小さいと、例外が出ました。UI は高さを 48 行以上に制限するので、これも到達しません。
  - x265 の mp4 は、フレームに旗が立たないのに、ストリームには tb/bt が書かれます。

### Recommendation
**REQUEST_CHANGES。** M-1（32-bit float で 4:2:0 のインターレース素材に interl=1 を付けること、読み直しの判定、verify_interlace へのケースの追加）を直せば、APPROVE に足りると考えます。L-1 の照合の補強も、同時に入れることを推奨します。

検証者が書いたスクリプト、C++、ログは `/private/tmp/claude-501/verify-int3/` にあります。
- C++: `new/qtui/tests/vint3.cpp`（`qtui_vint3`）
- スクリプト: `area_chk.py`、`extra.py`、`qmut.py`
- ログ: `qmut3.log`、`verify_interlace.log`、`selftest.log`、`cexp.log`、`verify_ofx.log`、`make_verify.log`