# Q1: Quality / Completeness の欄の読み直しと実装の修正 (2026-11-02)

(作業者 q1 の報告を統括が転記。作業者の環境では本ファイルへの書き込みが拒否されたため。)

作業場所は worktree `.claude/worktrees/q1` (ブランチ `q1-quality-fix`)。本家は実行も dlopen もしておらず、Demo の arm64 スライスを
`/usr/bin/objdump` で静的に読んだだけ。処理コアと AFT の数値は変えていない。
背景は `work/omo/wp_genuine_capture/REPORT.md` §2・§5・§9・§10 (main の作業ツリー)。

## 1. g の読み (観測)
- 呼び出し元 Demo 0x9bb45c〜0x9bb464 は `x0 = p+0xbf40` で g を呼ぶ。ループは `x1 = x0 + 0x20k` を基点に `[x1+0x22]`・`[x1+0x21]`・
  `s1=[x1+0x4]`・`w1=[x1+0x1c]`・`ldrb [x1+0x20]` を読む (ch1 は +0x120、ch2 は +0x240)。別経路の 0x9bb9fc〜 も同じオフセット。
- ビンの基点: BuildNoiseModel の曲線初期化 FUN_009b3c84 (x19 = p+0xbf40) は `[x28+4]` に 1.0、`+8〜+0x1b` に 1.0 を 5 つ書き、
  `+0x1c〜+0x22` を 0 にする。したがってビンは +0xbf44 から始まり、+0x1f は書かれない (.dnp に残る 0x6d と整合)。
- g が読む欄 (ビン o 基点): +0x1e (>3.0) → +0x1d (クリップ) → +0x00 (レベル = 0) → +0x18 (u32 有効) → +0x1c (マーカーのバイト)。
  ch2 のビン 8 が読むのは 0xc2a0 で、0xc2a4 は読まない。
- g_relabel.py を使わずに書いた g で 13 画面を計算し 13/13 一致。

## 2. 課題 A: AFT 前の 27 ビン
根拠は本家とビット一致する `nv_profile.build_profile` (観測) と FUN_009b3c84 の読み。
- バイト (観測): 全ビンがレベル 1.0・段係数 1.0・有効 0・旗 0。例外は各チャンネルの平均輝度のビンだけで、有効 (+0x18) = 1、+0x1d = そのチャンネルのクリップ旗。
- 値: g = 3/27 (完成度 11%)。クリップしたチャンネルは −1/27 になり、1 ch なら 4%、2 ch 以上なら 0%。

`work/omo/wp_quality_fix/tools/pre_aft.py` (観測):

| 入力 | 完成度 | Quality |
|---|---|---|
| pa1_s1 の見本 128² (モデル 6 面がファイルと 6/6 一致) | 11% | 44% (AFT 後のファイルは 72%/80%) |
| test_v02 n=312 の x832 y0 128² | 11% | 44% |
| test_v02 n=312 の全面 | 11% | 11% |

この状態は RS の UI からは観測できない (Build / Auto Profile が AFT まで行うため)。

旧い観測との照合:
- Reset 直後 35% / 0%: 訂正後の読みで説明できる (Reset は全ビンの有効とマーカーを 0 にする。pa1_s3 の復号像)。pa1_s3 は見本ありで保存されており
  保存時点の 0xc2a4 は 1。「コンストラクタが 0xc2a4 を消す」という説明は撤回。
- Reset 後の赤 4 ビン編集で +1/81 ずつ: 手動編集がそのビンのマーカーを立てる (s6/s7 で観測) ことで説明できる。
- 65% / 85%: 21 測定 + 6 埋め = 69/81 と整合 (ファイルがなく内訳は未確認)。
- 23% / 完成度 0%: 新規ビルドではない (新規なら 11%)。g = 0 の状態 (Reset か汎用プロファイル相当) と整合するが、どの状態かは不明。
- 43%: 同じ画面の "90%" 欄を完成度とみると g = 73/81 と整合 (推定)。
- 「新規の完成度は 1%」: 撤回。

## 3. 課題 B: 実装の修正
- g 本体: `quality.py`・`nv_quality.cpp`・`quality_draft.py` の `genuine_g` を訂正後の欄に直した。
- 再現側の状態 → ビンのバイト (`quality._curve_blob` と C++ `nvu::quality_numbers` を同じ規則に):

| 状態 | 有効 (+0x18) | マーカー (+0x1c) | クリップ / >3.0 |
|---|---|---|---|
| AFT 後 (`aft_bin_flags` あり) | `marker` が偽のビン | `marker` が真のビン | 保存された値のまま |
| AFT 後で旗なし (旧い保存) | なし | 全ビン (g = 33%) | なし |
| 新規 (AFT 前) | 各チャンネルの平均のビン | なし | 平均のビンにクリップ旗、レベル > 3.0 で >3.0 旗 |
| Reset (`aft_reset`) | なし | なし | なし (g = 0) |
| 手動編集のビン (`ps.pinned`) | 0 | 1 | クリップはそのまま、>3.0 はレベルから (推定) |

  0xc2a4 は Quality が読まないので常に 1。
- AFT ポートの「マーカー」(観測): `aft_port` の curve_post は測定ビンに count (+0x18) = 1・+0x1c = 0、curve_fill の write は埋めたビンに count 0・
  +0x1c = 1 を書く (本家と同じ意味)。`aft_bin_flags["marker"]` は +0x1c の u32 ≠ 0 なので「有効 = marker が偽」とした。本家の AFT 後の 16 本では
  全ビンが有効とマーカーのどちらか一方だけで、クリップ・>3.0 の旗は 0 件 (この導出と矛盾しない)。
- C ABI: `nvc_quality_numbers_pinned` を追加 (`nvcore_c_ui.h`。既存の `nvc_quality_numbers` は pinned = NULL の同じ関数)。追加のみで NVC_ABI_VERSION = 3 のまま。
  qtui の `profile_numbers` は新関数を使う。`x86_bridge_cap.py` に記録用の表を追加。
- G1 (R/G/B モード) のレベル語の扱いは残した (訂正後はレベル = 0 かどうかしか効かず、factors ≥ 0.2 なので影響なし)。旧い保存ファイルは形式不変でそのまま読める。

回帰試験:
- `native/testdata/genuine_quality.json` (45 KB): 13 本の .dnp の復号像のうち Quality が読む範囲と画面の値。作成 `work/omo/wp_quality_fix/tools/make_genuine_fixture.py`。
  `.gitignore` に `!native/testdata/`・`!native/testdata/*.json` を追加。
- `native/verify_quality.py`: (1) 13 件の画面値 (Python・C++、double と float の qRound)、(2) 写像 (状態 → 27 ビンが本家の像と同じ、完成度が画面と一致。
  状態は「ファイルのビンから作った aft_bin_flags」「Reset」「s5 + 編集ビン (s6: R4、s7: R4・G2)」)、(3) 13 件 + 乱数 blob の C++/Python ビット一致
  (有効 u32 の上位バイトだけの値・+0x1f・レベル NaN/−0 を追加)。旧い読みでは (1) は 13 件中 1 件しか一致しない (backup/quality_draft.py)。
- `native/verify_bridge_ui.py` の quality: 編集ビン (pinned) のある変種を追加 (v%3==2 か v%8==3)。新旧両関数を Python の profile_numbers と比較。
- qtui selftest: 新項目 genuineCompletenessMapping。qualityFreshResetAndNone の新規の期待値を 0〜1% → 11% (クリップ時 4/0%)。resetFineTuneCompleteness は説明のみ更新。
- Python selftest (参照実装): g の合成試験 6 件を訂正後の欄で 8 件に書き直し、新規の完成度の期待値を 11% に。

## 4. 試験結果 (観測)
- `make -C native nv_quality_test lib`: 警告なし。
- `python3 native/verify_quality.py 20000`: genuine screens 13/13・bin mapping 13/13・20013/20013 bit-exact。
- `python3 native/verify_bridge_ui.py --jobs 3 --tmp <scratch>`: ui 34755/34755 bit-exact (quality 100/100)。
- `python3 native/verify_match.py`: 1107 scores ALL MATCH。
- qtui (Qt 6.12.0) `qtui_selftest`: 139 passed / 0 failed (138 + 1)。初回は新項目が落ちた (試験側で marker に本家の未初期化 +0x1f を含めていた) → 外して全件通過。
- Python selftest: 99/99 (従来 97)。
- `~/.config/NeatRepro/nv_ui_qt.ini` は試験の前後とも sha256 45d4258c53337ee4…。
- `make -C native verify` の通しは走らせていない (13 本のうち 11 本は変更ファイルを使わない。lib を使う 2 本のうち verify_match は通過、verify_fieldproc は nv_v5_test 未ビルドで起動できず (変更とは無関係))。
- libnvcore: `nvc_abi_version()` = 3。

## 5. 文書
- `work/omo/coord/verify_quality/static/QUALITY.md`: 先頭に §0 訂正 (2026-11-02) を追加。§3 の表・§8.2・§8.5 を訂正。§8.3・§8.4・§8.6 の見出しに撤回 / 読み直しを明記。
- `Agents.md` §4「Quality」節を訂正 (wp_genuine_capture/REPORT.md §5 を引用)。
- `reference/python/nv_ui_qt/main_window.py` の reset_fine_tune の docstring、quality.py・nv_quality.h・nv_ui.h・nvcore_c_ui.h・nvcore_c.h のコメントを更新。
- 変更前の控え: `work/omo/wp_quality_fix/backup/` (Agents.md・QUALITY.md・quality_draft.py)。

## 6. 残り・未確認
- 新規ビルド直後の本家画面 (完成度 11% の予測) は未観測 (RS の UI では出せない)。
- クリップ (+0x1d)・>3.0 (+0x1e)・0.5^クリップ数の経路は本家の 18 本に該当データがなく未検証。
- 本家の手動編集でレベルを 3.0 より上にしたときの +0x1e とクリップ旗の扱いは未観測 (再現側は推定)。
- 既知の差 (推定): あるチャンネルで AFT が結び目を作れなかった場合、埋まらないビン (有効 0・マーカー 0) が残る。保存された旗からは区別できず
  +1/27 と数える (本家なら 0)。直すには nv_aft_v5.py に有効旗を足す必要があり、変更禁止のため記録のみ。
- 旗なしの旧い AFT 保存は全ビンをマーカー扱い (完成度 33%)。
- `work/omo/wp_quality_fix/` 以下は .gitignore で無視される (コミット時は `git add -f`)。

## ファイル (worktree 内)
変更: .gitignore, Agents.md, native/{nv_quality.cpp,nv_quality.h,nv_quality_test,nv_ui.cpp,nv_ui.h,nvcore_c.h,nvcore_c_ui.cpp,nvcore_c_ui.h,verify_bridge_ui.py,verify_quality.py,x86_bridge_cap.py},
qtui/src/core/nvcore.cpp, qtui/tests/selftest.cpp, reference/python/nv_ui_qt/{main_window.py,quality.py,selftest.py},
work/omo/coord/verify_quality/static/{QUALITY.md,quality_draft.py}
新規: native/testdata/genuine_quality.json, work/omo/wp_quality_fix/tools/{make_genuine_fixture.py,pre_aft.py}, work/omo/wp_quality_fix/backup/

## 7. 独立検証 (2026-11-02)
- 判定: **APPROVE** (ブロッカー 0、所見は Low 7 件)。検証者は fixture を .dnp から独立に再生成して一致、13 画面を目視して一致、
  g を Demo 0x9bb748 / 0x9bba74 の両経路の逆アセンブル (定数 ±1/27・±1/81 の movk、fadd の順、`fmax`) と照合、
  変異試験 (有効のオフセットを +0x1c に変える) で verify が落ちることを確認、qtui selftest 139/139、verify_bridge_ui --quick 16346/16346、ABI 3。
- Low の所見: (1) 写像試験の入力が AFT 移植の実出力ではない (本家 .dnp のビンから作った旗) → Phase C の結果で補う、(2) 旗なしの旧保存は 33%、
  (3) AFT が結び目を作れないチャンネルは +1/27 と数える (要 nv_aft_v5 の変更)、(4) クリップ・>3.0・0.5^クリップ数は本家データなし、
  (5) 新規 11% は本家画面で未観測、(6) `native/nv_quality_test` のバイナリが差分に入る、(7) `make -C native verify` の通しは統括が実行 (§8)。

## 8. 統括の再実行 (2026-11-02)
- `make -C native verify VJ="--jobs 3"` を単独で実行: verify_quality (13/13・13/13・4013/4013)、band 5706/5706、curve ALL BIT-EXACT、lzma 667/667、
  flat ALL BIT-EXACT、flat_cancel ALL IDENTICAL、model・temporal・spatial 全件 bit-exact、profile 39/39、aft ALL BIT-EXACT まで通過。
  verify_match は既存の不具合 (`--jobs` を受け付けない) で make が止まったので、verify_match・verify_fields・verify_fieldproc は単独で実行し、
  1107 scores ALL MATCH・ALL BYTE-EXACT・ALL BIT-EXACT。**14 本すべて通過**。
- qtui_selftest 139/139、`nv_ui_qt.ini` 不変。
- 注: 統括の操作ミスで、途中に `make verify` が 2 本同時に走って一時ファイルが衝突した回がある (verify_band / verify_model の失敗)。上はその後の単独実行の結果。
