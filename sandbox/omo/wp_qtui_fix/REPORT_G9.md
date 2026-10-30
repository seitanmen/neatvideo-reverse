# N3 fixes2 REPORT — G9 (作業者 fix2-c)

転記: 統括 (2026-10-30)。作業者は REPORT.md に書けないため、作業者の最終報告をそのまま転記した (独立検証の M1・L1〜L6 への対応を含む最新版)。統括の再確認と独立検証は末尾に追記する。

PR 用に `REPORT.md` から改名した。G2〜G6・G7/G8・G9 の 3 つの PR が同じパスに別の内容を足して衝突しないようにするため。共通の `REPORT.md` へのまとめは G1 の完了後に行う。

# G9 最終報告: x86 検証の穴埋め (作業者 fix2-c、3 回目の修正 M1・L1〜L6 を追記)

中断前に修正と確認は最後まで済んでいました。再開後に確かめた作業ツリーの状態は次のとおりで、途中の編集は残っていません。

- `git diff` には `.gitignore` と `native/Makefile` だけが出て、どちらも前回と同じ内容です。新しいファイルは未追跡のまま置いてあります。
- シェルスクリプトと Python の構文検査は全部通りました。分類器の合成試験も 23/23 のままです。
- M1 の修正 (古い結果を先に消す・分類は rc 3 のときだけ) がファイルに入っていることを確かめました。

依頼の確認 3 つはすべて想定どおりでした。

- 分類器の合成試験 (L1・L2 の境界を追加): 23/23
- fakepy.sh を使った実行: 失敗 (rc 1)
- `--quick --only curve,depth,frame --allow-known`: 成功 (rc 0)

## REPORT 用の追記 (2026-10-30〜31、独立検証の REQUEST CHANGES への対応)

**M1 (必須。`--allow-known` が x86 の再生の失敗を成功にしてしまう)**
- `native/verify_x86_bridge.sh` は再生の前に、その回の `replay_*.json`・`dump_x86`・`dump_arm64` を消すようにしました。
- `native/x86_bridge_replay.py` の終了コードを分けました。
  - 0: 全件一致
  - 3: 差あり (比較を最後まで終えた場合)
  - 2: 記録が無い・再生 0 件・群の不足
  - 1: Python の例外
- 分類は rc が 3 で、かつその回の `replay_x86.json` があるときだけ行います。arm64 と portable の再生は、0 以外ならすべて失敗です。

**L1**: 分類 (a) は、両側とも有限 (NaN でも ±inf でもない) の語だけを対象にしました。

**L2**
- 再生の差の記録に、C の引数番号 (`arg`) とポインタ配列の要素番号 (`entry`) を入れるようにしました。
- 分類の対象は次の領域だけです。どれも esize 4 で、役割は out です。
  - temporal: 出力 out と rv (引数 16 と 17)
  - nlc_image: 出力 (引数 4)
- それ以外の領域、uint8 や int の領域、`arg` の無い古い JSON は分類外 (失敗) です。

**L6**
- 記録のラベルの付け方を変えました。verify_* のフレームが無く、プロセスの `__main__` か `__mp_main__` が `nv_ui_qt/export.py` (または `denoise_video_v5.py`) のときは ("export", 関数) にします。export の frame worker は `backend._work` を実行するので、呼び出しのスタックからは判定できないためです。
- ラベルの無い呼び出しは、群 "unlabelled" にまとめます。
- 群があるかどうかを数えるとき、`nvc_abi_version` は数えません。fallback だけは例外で、他に native の呼び出しをしないので数えます。

**L3 (訂正)**
- 前回まで「nv_depth の 208,737,760 語は x86 で全件一致」と書いていましたが、誤りでした。208,737,760 は、arm64 での Python と C の比較の語数です。
- x86 側の根拠は、depth 群 18,751 呼び出しの再生が全件バイト一致したことです。内訳は sanitize 1,296・to_internal 3,029・from_internal 13,829・ofx_pack 487・ofx_unpack 110 で、再生で比べた出力は 216,163,431 語です。
- この一致を経由して、Python との 208,737,760 語の比較も x86 で成り立ちます。

**L4**: `x86_exe_pair.py --report` を作り直し、`sandbox/omo/wp_qtui_fix/x86_g9/exe_pair/` に保存しました。結果は前回の報告と同じです。

| 試験バイナリの入力 | x86 と arm64 の比較結果 |
|---|---|
| spatial | 668,088 語が違う。うち NaN のビット違いが 667,858 語、有限値の差 230 語はすべて `exp` のケース |
| temporal | NaN のビット違い 24,187 語、有限値の差 160,083 語 (temporal と frame の expf) |
| aft | finish 835 語・sim 66 語がすべて NaN のビット違い |
| profile | 6/6 一致 |
| LZMA | 3/3 一致 (667/667 バイト) |

v5 については、ケースごとの表と NaN のビットの組も同じ場所に残しました。

**L5 (記録)**: MAX_ULP = 8 は、観測した最大値そのものです。余裕はありません。入力によっては 9 ULP が出て、`--allow-known` が誤って失敗し得ます。これは安全側の誤りです。

**確認** (ログは `sandbox/omo/wp_qtui_fix/x86_g9/fix_m1/`)

| 手順 | 結果 |
|---|---|
| 分類器の合成試験 (`known_selftest.py`) | 23/23。±inf と FLT_MAX は不合格、FLT_MAX と 1 ULP 下は合格、new_flow (uint8)・flow_out・役割 in・esize 8・arg なしは不合格 |
| (A) `--quick --only curve,depth,frame --allow-known --keep` | arm64・portable とも 3577/3577。x86 は 3546/3577 で、temporal 31 呼び出し・186 面・435,256 語を分類、分類外 0。**rc 0** |
| (B) 同じ作業ディレクトリで `--no-capture --allow-known --py86 fakepy.sh` | 「simulated replay crash」・「1 REPLAYS FAILED」。古い結果は消えていて分類されない。**rc 1** |
| (C) その後、本物の Python で `--no-capture --allow-known` | rc 0 |
| (D) 厳密判定のまま `--no-capture` | rc 1 |
| `--quick --only curve --export --allow-known` (L6) | export 群 48 呼び出し、unlabelled 0。x86 の差は export の temporal 8 件で、すべて既知の分類に入った。rc 0 |

前回の 0 件の再生の修正 (記録が無い・0 件・群の不足で失敗する) は、そのまま効いています。

## 結論 (前回までのまとめ)
- verify_bridge の全群と書き出しを x86 で再生すると、38,667 呼び出しのうち 38,593 が arm64 とバイト一致しました (M1 の修正前の全群の実行)。違った 74 件は既知の 2 種類です。
  - **x86 の temporal は expf の差 (1〜8 ULP) でビット一致しません。W2c の範囲と同じです。** 共通実装は入れず、基準は「macOS arm64 の libm (Python と同じ)」のままです。
  - nvc_nlc_image の 2 件は、0×inf でできる既定 NaN の符号の違いです。
- nv_depth は、x86 の depth 群 18,751 呼び出しが全件一致しました (L3 で書き直した内容)。
- arm64 の既定ビルドに回帰はありません。コミットはしていません。

## x86 環境の作り方
- **新しく入れたものはありません。** 既にある python.org の Python 3.12.10 (universal) を `arch -x86_64 /Library/Frameworks/Python.framework/Versions/3.12/bin/python3.12` で x86_64 として起動し、標準ライブラリだけで動かしました。
- **記録**: arm64 で verify_bridge.py をいつもどおり実行し、PYTHONPATH に置いた sitecustomize で ctypes.CDLL を包みます。処理コードは変えていません。
- **再生**: 同じ呼び出しを各ライブラリで実行し、記録とバイト比較します。

## `--allow-known` の全群実行 (M1 の修正前の 1 回。判定の内容は今の版と同じ)
| 段階 | 結果 |
|---|---|
| 記録しながらの verify_bridge | ALL MATCH (727 秒) |
| arm64 の再生 | 38,667/38,667 |
| arm64 portable の再生 | 38,667/38,667 |
| x86_64 の再生 | 38,593/38,667 |
| 分類 (a) temporal | 72 呼び出し・432 面・1,472,151 語 |
| 分類 (b) nlc_image | 2 呼び出し・6 語 |
| 分類外 | 0 |
| 終了コード | rc 0 (29 分 26 秒) |

この実行の差の記録には、引数番号 (`arg`) がまだ入っていません。そのため、今の分類器にこの JSON を渡すと失敗になります。M1・L2・L6 の修正後の全群の実行はしていません。代わりに、上の quick の (A)〜(D) と export の実行で確認しました。

## 差の原因 (観測)
- **expf**: 全 2^32 の float 入力を走査すると、2,018,697 個の入力で 1 ULP 違いました。openlibm の `expf` を両アーキで使うと、temporal と spatial の 178 呼び出しは全件一致しました。
- **nlc_image**: 0×inf の既定 NaN が、arm64 では 0x7fc00000、x86 では 0xffc00000 になります。
- **NaN を含む試験入力**: nv_ndimage・nv_aft・nv_ui の非 AArch64 経路で、NaN の語の符号とペイロードだけが違いました。実フレームの入力ではこの差は出ていません。

## 未保証の範囲
- x86 の temporal とそれを含む書き出しはビット一致しません (expf の差。W2c の範囲)。
- NaN が出る入力では、nv_ndimage・nv_aft・nv_ui の NaN のビットが違います。「x86 未保証」のままです。
- 照合はこの Mac の Rosetta 2 でだけ行いました。実機の x86 CPU での確認はしていません。
- verify_quality は x86 版の試験バイナリでは確認していません。同じ処理は ui の nvc_quality_numbers (100/100) で照合済みです。
- band・curve・model・flat は今回は再実行せず、F4 の結果に頼っています。
- MAX_ULP には余裕がありません (L5)。

## 変更ファイル
- `native/Makefile`: `lib_x86` 目標を追加しました。
- `.gitignore`: `native/x86cap/` の例外を追加しました。
- 新規:
  - `native/x86_bridge_cap.py` (記録。今回 export のラベルを変えた)
  - `native/x86cap/sitecustomize.py` (記録の読み込み口)
  - `native/x86_bridge_replay.py` (再生。今回 rc 3・`arg` / `entry`・群の数え方を変えた)
  - `native/x86_bridge_known.py` (既知の差の分類。今回 L1・L2 を直した)
  - `native/x86_bridge_diff.py` (語単位・ULP の差)
  - `native/x86_exe_pair.py` (試験バイナリを x86 と arm64 で対にして走らせる)
  - `native/verify_x86_bridge.sh` (通し実行。今回 M1 を直した)
- 証拠は `sandbox/omo/wp_qtui_fix/x86_g9/` (`allow_known/`・`fix_zero/`・`fix_m1/`・`exe_pair/` を含む)、進捗は `sandbox/omo/wp_qtui_fix/PROGRESS_c.md` です。

## 質問
ありません。

---
## 統括の再確認 (2026-10-30)
- `make lib_x86` を含む `native/verify_x86_bridge.sh --quick --only curve,depth,frame --allow-known`: arm64 3577/3577・portable 3577/3577・x86 3546/3577、差の 31 件はすべて nvc_temporal (435,256 語)、分類外 0、rc 0 (`sandbox/omo/coord_fix2c/rerun_quick_allow.log`)
- 統括が見つけた穴: 記録が消えた後の `--no-capture` が 0/0 calls で ALL IDENTICAL・rc 0 (`rerun_quick_strict.log`)。作業者が修正 (上の追記)
- 修正後 (frame の quick、`rerun2_*.log`): 空ディレクトリの `--no-capture` は FAIL no capture・rc 1 / `--keep` での厳密判定は x86 62/93 で rc 1 (既知の temporal 差) / 同じ記録の `--no-capture` 厳密判定も rc 1 / `--allow-known` は 31 件すべて temporal・分類外 0・rc 0
- M1 修正後 (統括、2026-10-30 夕): `--quick --only curve,depth --allow-known --keep` は rc 0 (curve 12・depth 3472 呼び出し、ALL IDENTICAL)。同じ作業ディレクトリで `--no-capture --allow-known --py86 coord_fix2c/independent/fakepy.sh` は「simulated replay crash」「1 REPLAYS FAILED」・rc 1 (古い結果で成功にならない)。ログ `sandbox/omo/coord_fix2c/rerun3_*.log`
- 独立検証: `sandbox/omo/coord_fix2c/VERDICT_g9.md` — Round 1 REQUEST CHANGES (M1 必須・L1〜L6) → Round 2 **APPROVE** (2026-10-30。古い結果が残った状態での 5 通りの壊し方がすべて rc 1、分類器の境界 24/24)
