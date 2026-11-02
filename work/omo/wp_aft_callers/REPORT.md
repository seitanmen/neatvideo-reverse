# AFT の呼び出し側の修正 (Phase C の提案 1・2) (2026-11-02)

(作業者 aft2-worker の報告を統括が転記。作業者の環境ではレポート .md の書き出しが止められたため。)
作業場所は worktree `.claude/worktrees/aft2` (ブランチ `aft2-callers`、基点 `73eb1be` = PR #9)。コミットなし。
変更前の写しは `backup/` に同じ相対パスで置き、基点は `backup/BASE_COMMIT.txt`。本家のバンドルは実行・dlopen・リンクしていない
(照合は .dnp と TIFF のデータだけ)。`~/.config/NeatRepro/nv_ui_qt.ini` の sha256 は試験の前後とも `45d4258c…80fcc6`。

## 0. 結論
- 観測: 本番の入口 (Python の `nv_aft_v5.apply_aft` と、C ABI を通る `nv_native.apply_aft`) に 16 ビット化を入れ、"Two fields" では
  上フィールドの行を下フィールドの行の上に積んだ画像を渡すと、本家の .dnp 14 本すべてで旗 27/27・曲線 ≤5 ulp (目標 ≤6)。
- 観測: 14 本とも、BuildNoiseModel のデータ・AFT のデータ・結果の dict が Python と C でバイト一致。
- 観測: モデル (6 面 × 2005 = 12,030 語) は 12 本で .dnp と 0 語差。old_town_cross のプログレッシブ 2 本だけ 3,105 / 3,076 語差 (§4.2)。この 2 本も曲線 ≤3 ulp・旗 27/27。
- 観測 (負の対照): 16 ビット化を外すと 14 本とも曲線 1,672〜2,782,737 ulp・モデル 3,278〜3,395 語差。積み方を逆 (下を上) にすると 4 本とも
  235,969〜4,040,388 ulp、旗も 2 本で 25/27・26/27。
- 回帰: `make -C native verify` 14 本合格、`verify_bridge.py` ALL MATCH、qtui 自己試験 139/139 (1 回目の不合格への対応は §5)、
  AFT を使わない経路は変更前の libnvcore と出力 8/8 一致。
- 2 回目以降の AFT (提案 4) は調査のみで変更なし (§3)。

## 1. 変更したファイルと要点
| ファイル | 要点 |
|---|---|
| `reference/python/nv_aft_v5.py` | `to_internal16(rgb)` を追加 (`nv_lzma.plane_int16` = FUN_009f0554 の移植で int16 にして /16)。`apply_aft` の先頭で RGB を通すので BuildNoiseModel と AFT の両方に効く。色結合と曲線への乗算は不変 |
| `native/nvcore_c.h`・`nvcore_c.cpp` | `nvc_aft_input16(in, n, out)` を追加 (`nvlz::plane_int16` を /16。in と out は同じでもよい)。ABI 版は 3 のまま (追加のみ) |
| `reference/python/nv_native.py` | `_apply_aft` は `nvc_aft_input16` を通してから `profile_blob`・`aft_blob`。シンボルのない旧 lib では None を返して Python の AFT が走る (結果は同じ。判定 `HAS_AFT_INPUT16`) |
| `qtui/src/core/nvcore.{h,cpp}` | `run_aft` を `run_aft_rgb(base, rgb, H, W, cancel)` に分割。`run_aft_rgb` は先頭で `nvc_aft_input16`。G1 のビルド時の色の重み (`nvc_rgb_weights`) も同じ 16 ビットの RGB から作る (§5) |
| `qtui/src/core/fields.{h,cpp}` | `run_aft_fields` を追加。フレームのプロファイルはフレーム、"One field" はそのフィールド (従来どおり)、"Two fields" は先のフィールドの RGB の下にもう一方のフィールドの RGB を積んだ W×2·ceil(H/2) の画像で AFT を 1 回。BFF で先のフィールドを上に積むのは推定 (コメントに明記) |
| `qtui/src/app/mainwindow.cpp` | `auto_fine_tune` が `run_aft_fields` を呼ぶ |
| `qtui/tests/selftest.cpp` | INT-3 の項目に追記: Two fields の AFT 結果は積んだ面から作ったフレームで `run_aft` した結果と dict 全体が一致、上フィールドだけの AFT とは `aft_levels` が違う。項目数 139 のまま |
| `qtui/tests/fixtures/g1_before_combine.json`・`qtui/tools/g1_fixture/g1_fixture.cpp` | G1 の期待値を作り直し (§5) |
| `Agents.md` | §4 の対応表の後と §7 の AFT の項に短く追記 |

照合・調査のスクリプトは `tools/` (`verify_callers.py` 受け入れ照合、`locate_area.py` プログレッシブ .dnp の見本の場所、
`no_aft_invariant.py` AFT を使わない経路の不変、`aft_effect.py` 手元の素材での 16 ビット化の効き)、結果は `out/`。

## 2. 変更前の Two fields / One field の動作 (コードの読み)
- プロファイルの作成 (`fields.cpp` の `build_profile_fields`):
  - "Two fields": 先のフィールド (`parity`) の画像 (W × ceil(H/2)) で FindFlatArea と build_profile_v5。単一フレームの曲線は、同じフレームの
    もう一方のフィールドと次のフレームの同じパリティのフィールドの曲線と合わせて中央値。静止画素の対は次のフレームの同じパリティのフィールド。
  - "One field": そのパリティのフィールドの列 (フレーム f, f+1, f+2)。
  - 手動の領域はフレームの行からフィールドの行へ (y0/2、h/2)。`prof["area"]` は先のフィールドの画像の座標。
- AFT (変更前): `profile_image` (= `field_frame(frame, parity)`) に `run_aft` を 1 回。BuildNoiseModel の領域も同じフィールド座標。
  Two fields でも先のフィールド 1 枚 (W × H/2) だけを見ており、Phase C §2 の「上フィールドだけ」の型 (本家と 39,697〜4,044,070 ulp)。
- 変更後: Two fields は先のフィールドの RGB を上、もう一方を下に積んだ画像に AFT を 1 回。領域は先のフィールド座標のまま (積んだ画像の上半分なので不変)。
  RGB はフィールドごとに `to_rgb(to_planes(field_frame))` (変更前の AFT と同じ変換) で作って縦に連結。4:2:0 の色差は行の繰り返しで拡大するので、
  フィールドの高さが偶数なら「面を積んでから変換」と同じ (自己試験で確認)。奇数 H では下フィールドの詰め物の行も残る (本家での扱いは未確認)。
  "One field" は不変 (本家のデータがない)。

### 2.1 見本の領域の座標系
観測 (`wp_aft_runtime/out/logs/locate_i.log` と .dnp の見本の大きさ):

| .dnp | 見本の位置 (フィールド座標) | 大きさ |
|---|---|---|
| sintel_tff_y3 | 下フィールド y116 x1784 | 96² |
| sintel_tff_y6 | 上フィールド y88 x1792 | 128² |
| old_town_cross_tff_y6 | 上フィールド y64 x328 | 128² |
| park_joy_tff_y6 | 下フィールド y348 x720 | 192² |

- 積んだ画像 (上フィールドが行 0〜539) の座標では、下フィールドの見本は y = 540 + フィールドの y。どの見本も片方のフィールドに収まる。
  照合はこの座標で領域を渡し、モデルは 4 本とも 0 語差。
- 推定と未変更: 本家の見本は 4 本中 2 本が後のフィールド (TFF の下) にあり、本家が積んだ画像全体で見本を探していることを示唆する (推定)。
  こちらの `build_profile_fields` は先のフィールドだけで FindFlatArea を行うので後のフィールドの領域は選ばれない。探索範囲の変更は v5 の
  プロファイル (曲線・帯域比) にも効き、根拠が AFT の照合だけなので変えていない。後のフィールドの領域は今の `prof["area"]` では表せず、
  再現には探索範囲の変更と座標系の再定義 (積んだ画像の座標) が要る。別の作業で本家の見本 4 本と照合して決めるのがよい。
- 制限: 本家と一致するのは領域が同じ (先のフィールドにある) ときだけ。e9i の本家の見本は 4 本中 2 本 (sintel_tff_y3・park_joy_tff_y6) が後のフィールドにあり、
  repro の Two fields はこれらを選べない (独立検証 L2)。

## 3. 2 回目以降の AFT (提案 4、調査のみ・変更なし)
- 観測 (コード): `backend._run_aft` (Python) と `nvcore.cpp run_aft_rgb` (C++) は、`ui_pre_aft` に残した AFT 前の v5 曲線を戻してから AFT を掛け、
  AFT 用の BuildNoiseModel もそのたびに作り直す。AFT が読む輝度曲線は新規ビルド (各チャンネルの平均輝度のビンだけ有効) なので、2 回目は 1 回目と同じ
  (自己試験 aftTwiceEqualsOnce)。Reset Fine-Tuning (`aft_reset`) の後も同じ (本家は Reset の曲線から始めるが、Phase C で新規と Reset は 14 本とも同じ結果)。
  手動編集の factors は AFT の後 `after_aft` が AFT のレベルで置き換える。
- 本家との違い: 本家は AFT 済みの曲線に再度掛けると前回の補間ビンを持ち越す (Phase C §3、pa1_s5 の 8/8 ビン)。「平均比で拡大縮小」の式は平均比 1 の例しかなく未検証。
  pa1_s5 を本番の入口 (新規ビルドから) で再現すると ≤5 ulp・旗 27/27 (本家の s4 と s5 の補間ビンが同一なので、この例では区別できない)。式が確かめられるまで変更しない。

## 4. 照合 (受け入れ条件)
コマンド: `tools/verify_callers.py --python --neg` (結果 `out/verify_callers.{txt,json,log}`)。
- 入力: フレーム 5 の TIFF の code/257 (ここでは量子化しない。入口が量子化する)。Two fields は上フィールドの行を下フィールドの行の上に積んだ 1920×1080。
- AFT 前の曲線: 入口自身が作る新規ビルドのもの (本番の動作)。
- 比べたもの: 入口の中で作られた BuildNoiseModel と AFT のデータ (`npr.build_profile` / `nv_aft.auto_fine_tune`、`nv_native.profile_blob` / `aft_blob` を包んで取り出した) と .dnp。
- 照合の「本番の入口」は apply_aft から先。prof は平らな曲線 (9 点すべて 1.0) の stub で、領域には本家の見本を直接渡した。build_profile_v5 と FindFlatArea は通っていない (独立検証 L6)。

### 4.1 結果 (C の入口。Python の入口も 14 本とも同じ数値で、データ 2 つと dict がバイト一致)
| .dnp | 領域 (y, x, size) と出典 | モデル語の差 /12030 | 最大 ulp | 0 ulp の語 | 旗 | 負: 16 ビットなし (モデル差 / 最大 ulp) | 負: 逆に積む (最大 ulp / 旗) |
|---|---|---|---|---|---|---|---|
| pa1_s1_autoprofile | 160,616,128² (Phase C) | 0 | 5 | 19/27 | 27/27 | 3,310 / 2,782,736 | — |
| pa1_s2_autoprofile_curframe | 同上 | 0 | 2 | 21/27 | 27/27 | 3,310 / 2,782,736 | — |
| pa1_s4_aft (本家は Reset の後) | 同上 | 0 | 5 | 17/27 | 27/27 | 3,310 / 2,782,737 | — |
| pa1_s5_aft_again (本家は s4 の上) | 同上 | 0 | 5 | 20/27 | 27/27 | 3,310 / 2,782,736 | — |
| e9p_sintel_p_y3 | 160,616,128² (平均) | 0 | 5 | 15/27 | 27/27 | 3,310 / 2,782,737 | — |
| e9p_sintel_p_y6 | 680,304,128² (平均) | 0 | 2 | 20/27 | 27/27 | 3,295 / 23,852 | — |
| e9p_old_town_cross_p_y3 | 0,1568,128² (平均、差 4.2e-5) | 3,105 | 3 | 21/27 | 27/27 | 3,286 / 14,455 | — |
| e9p_old_town_cross_p_y6 | 0,1568,128² (平均、差 6.5e-5) | 3,076 | 3 | 12/27 | 27/27 | 3,285 / 4,787 | — |
| e9p_park_joy_p_y3 | 720,968,128² (平均) | 0 | 2 | 11/27 | 27/27 | 3,278 / 3,762 | — |
| e9p_park_joy_p_y6 | 984,248,96² (平均) | 0 | 3 | 8/27 | 27/27 | 3,341 / 19,476 | — |
| e9i_sintel_tff_y3 | 656,1784,96² (下 y116) | 0 | 1 | 24/27 | 27/27 | 3,358 / 25,852 | 235,969 / 27 |
| e9i_sintel_tff_y6 | 88,1792,128² (上 y88) | 0 | 2 | 25/27 | 27/27 | 3,302 / 146,825 | 445,284 / 25 |
| e9i_old_town_cross_tff_y6 | 64,328,128² (上 y64) | 0 | 3 | 21/27 | 27/27 | 3,285 / 1,672 | 540,366 / 27 |
| e9i_park_joy_tff_y6 | 888,720,192² (下 y348) | 0 | 2 | 10/27 | 27/27 | 3,395 / 1,777 | 4,040,388 / 26 |

- 「平均」: E9 プログレッシブの .dnp には見本がないので、見本平均 (+0xbf28) に最も近い正方形 (320〜48 px、全位置) を領域とした。その場所のモデルは 4 本で 0 語差 (場所の裏付け)。
- 16 ビット化を外しても旗は 14 本とも 27/27 で、ずれるのは値だけ (pa1 系 22.7%)。
- Phase C の表との差: pa1_s5 は Phase C (s4 入力) の 2 ulp に対し新規ビルドからで 5 ulp。otc_p_y6 は Phase C 6 ulp に対し 3 ulp。

### 4.2 old_town_cross プログレッシブ 2 本のモデル差
- 観測: 最も近い位置 (y0, x1568, 128²) でも見本平均が 2〜3 ulp 合わない (park_joy・sintel は 0)。フレーム 4・6 や float の変換 3 通り (q16・d257・f65535) ではさらに離れる。
- 推定: 平均の差を 1/16 単位の画素の和に直すと R −8・G −4・B −12 程度で、少数の画素の値が違うとみられる。見本がないので画素単位では確かめられない。
  インターレースの old_town_cross (別の TIFF) は見本と 0 画素差。
- 曲線は ≤3 ulp・旗 27/27 で受け入れ条件は満たす。原因は入力側 (Resolve が渡した画像) とみて変えていない。

### 4.3 参考: 手元の素材での効き (`out/aft_effect.txt`)
UI のバックエンド (native) で 16 ビット化あり/なしの AFT を比較:
- test.mp4 n=300 (48²): RGB 面曲線の最大相対差 5.2e-5、Y 曲線 1.6e-5、マーカーのビンは違いなし
- test_v02 n=310 (128²): RGB 面曲線 2.8e-3、Y 曲線 8.4e-4、マーカーのビンは違いなし

## 5. 回帰
- `make -C native verify` (14 本、同時に 1 本だけ): すべて合格、26 分 35 秒 (`out/make_verify.log`)。verify_aft: shuffle 4097・sim 3000・finish 3000・
  aft 67/67 (-j1/-j3)・combine 69/69。verify_profile 39/39、verify_match 1107 スコア ALL MATCH、verify_fields・verify_fieldproc 合格
  (verify_match は止まらずに完走)。
- `make -C native lib` の後の `python3 native/verify_bridge.py`: ALL MATCH (`out/verify_bridge.log`、707 s)。aft 26/26 件 (999,732 語)、frame 72/72、
  cancel 49/49、fallback 9/9、ui・depth・g8・prof5 も一致。AFT の期待値は Python と C で毎回一緒に作られるので期待値の更新は不要。
- qtui 自己試験 (`QT_QPA_PLATFORM=offscreen qtui/build/qtui_selftest`): 2 回目で 139/139 (`out/qtui_selftest.log`)。1 回目は 122 通過・6 不合格:
  - g1FreshAndAft: ビルド時の G1 の色の重み `ui_rgb_weights` と AFT 後の `aft_color_weights` が一致しなくなった (AFT の BuildNoiseModel が 16 ビットの RGB を
    見るようになったため)。対応: `nvc_rgb_weights` の入力も `nvc_aft_input16` に通し、G1 の不変条件を保った。g1EditsResetComplete・g1MalformedKeys は連鎖。
  - g1EditMatchesAftCombine: 期待値ファイル `g1_before_combine.json` の AFT レベルが予定どおり変わった。対応: 生成器 `g1_fixture.cpp` (5dabc1f のコードで
    ビルド) に 16 ビット化をテスト対象とは独立に書き足し (`nvc_aft_input16` は使わない)、AFT 後の曲線を AFT のデータの `nvc_aft_combine` から作るようにして
    `make_g1_fixture.sh 5dabc1f` で作り直した。対照: 同じ生成器で 16 ビット化だけ外すと、旧い期待値ファイルと made_by の文字列以外がバイト一致。
  - n4eSavedProfilesMenu・n4eAutoMatchPicksSameClip (タイムアウト): 上の連鎖。修正後は合格。
- AFT を使わない経路 (`tools/no_aft_invariant.py`): 変更前の libnvcore (基点のソースでビルドした写し) と変更後で、`backend.build_profile` (auto・spatial) と
  `process_frame` (既定)、test.mp4 n=300〜304 と test_v02 n=310〜314 のプロファイルの dict と出力の面のハッシュが 8/8 一致
  (`out/no_aft_before.json`・`no_aft_after.json`)。nvfilter.py・nv_depth.py は不変、libnvcore の既存の関数も不変 (関数を 1 つ追加)。
- `native/verify_rgb_eq.py` (make verify・verify_bridge の外。独立検証 M1): 変更直後は apply_aft 群が量子化していない RGB の C 側と比べていて MISMATCH 921/932 だった。
  apply_aft 群の C 側に to_internal16 の RGB を渡すよう直し、`--quick` 932/932、`--quick --before-lib` 932/932、全件 `--before-lib` 9234/9234 で ALL SAME
  (`out/verify_rgb_eq_*.log`。G1 より前の lib は 5dabc1f からビルド)。直す前のスクリプトは今のコードで 921/932 (`verify_rgb_eq_quick_oldscript.log`)。
  ほかの `native/verify_*.py` のうち verify_prof5 は AFT・G1・rgb_weights に触れないので未実行、verify_libm_alt・verify_x86*.sh は x86 の Python と別ビルドの lib が要るので未実行。
- qtui の新規プロファイルは AFT を使わなくても `ui_rgb_weights` を 16 ビットの RGB から作るので (aft_color_weights と等しく保つため)、AFT 前の R/G/B イコライザの編集の
  結果がわずかに変わる。編集のない処理は変わらず、旧い JSON は保存済みの重みを読む。「AFT を使わない経路は不変」の範囲は Python と libnvcore (独立検証 L3)。

## 6. 変えなかったことと理由
(nv_aft_v5.py の docstring の名前は run_aft_fields / run_aft_rgb に訂正済み。独立検証 L4)
- v5 のノイズ除去本体・FindFlatArea・build_profile_v5: 本家の根拠がない。
- Quality の profile_numbers (フィールド画像のまま): 同上。
- N4e の `match_model_of` (Auto Match のモデル。コメントに「run_aft と同じ BuildNoiseModel」とあるが量子化していない): 変えると保存済みプロファイルの
  match_model と verify_match の期待値が変わるので別に決めるべき。
- "One field" の AFT 画像 (そのフィールド 1 枚): 本家のデータがない。
- Two fields の FindFlatArea を積んだ画像で行うこと (§2.1): v5 のプロファイルにも効く。
- 2 回目以降の AFT の持ち越し (§3): 平均比の式が未検証。
- AFT のモード: 0 のまま (現状どおり)。
- profile_panel のツールチップ ("static port … its factors were not compared with the genuine"): 文言は古くなったが範囲外。

## 7. 未確認の点
- BFF で先のフィールドを上に積むこと (推定)、奇数 H の詰め物の行 (積んだ画像は W × 2·ceil(H/2)。実在の形式は H が偶数なので出ない)、"One field"。
- 16 ビット化がホスト・ビット深度で変わるか。確かめたのは Resolve 21.1.1 の 32-bit float 入力だけ。こちらの 8 ビット素材の RGB は YUV→RGB の float で
  1/16 の格子に乗らないので量子化が効く (§4.3)。
- old_town_cross プログレッシブの入力の数画素の違い (§4.2)。
- e9p_park_joy_p_y3 の見本の位置: こちらの FindFlatArea (q16・d257 とも) は (y872, x1064) を選ぶが、本家の見本は平均から (y720, x968)
  (`out/areas.json`、その場所のモデルは 0 語差)。範囲外なので調べていない。

## 8. ファイル
- 照合スクリプト: `tools/verify_callers.py` (ほか `locate_area.py`・`no_aft_invariant.py`・`aft_effect.py`)
- 結果: `out/` (verify_callers.txt、make_verify.log、verify_bridge.log、qtui_selftest.log、aft_effect.txt、areas.json)
- 変更前の写し: `backup/` (BASE_COMMIT.txt = 73eb1be)

## 9. 独立検証
- Round 1 (aft2-verifier): REQUEST CHANGES。ブロッカー M1 (verify_rgb_eq.py が古い比較のまま) と Low L1〜L6。本番コードの誤りはなし。
  検証者は作業者のツールを使わない独自スクリプトで 4 本 (pa1_s1・e9p_park_joy_p_y6・e9i_sintel_tff_y6・e9i_park_joy_tff_y6) を再現し、量子化の 4 者一致 (500,031 値) と
  qtui 139/139 を確認。M1・L1〜L4・L6 は作業者が修正 (この版の §2.1・§4・§5・§6 と Agents.md)、L5 (git add -f) は統括がコミット時に対応。
- 統括の再実行: `tools/verify_callers.py --neg` (C の入口) で 14 本とも本表と同じ数値 (モデル 0 語差 12 本、old_town_cross プログレッシブ 3,105 / 3,076 語差、旗 27/27、≤5 ulp)。
- Round 2 (aft2-verifier): **APPROVE** (ブロッカー 0)。M1 の直し方は空疎でない (apply_aft から 16 ビット化を外す変異で 921/932 に落ちることを確認)。
  to_internal16 そのものの誤り (恒等写像など) はこの試験では両側が同じ関数を使うので検出できないが、量子化の正しさは verify_bridge の aft 群
  (Python to_internal16 と C nvc_aft_input16) と qtui の g1EditMatchesAftCombine (生成器が独立に量子化) が受け持つ (検証者の推定)。L1〜L4・L6 は解消、L5 はコミット時に統括が対応。
