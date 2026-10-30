# VERDICT: nv_ui_qt 本家配置への再設計 (2026-10-25) 独立検証

総合判定: **REQUEST CHANGES** (軽微)。中核の正しさ (既定でのビット一致、書き出しのビット一致、Frequencies の 3 成分同時書き込み、mix_enable、プロファイルの Noise Level、押している間の元画像、プレビュー範囲、サムネイル、Beginner/Advanced、D1〜D5・D7・D8・D10) はすべて確認した。
修正を求めるのは次の 3 点: L1 (ドラッグの Undo 併合。以前からある不具合で、D6 の「ドラッグは 1 件」は実際には成り立っていない)、L2 (NaN の tune で例外)、L3 (D9 の可視表示が一部後退)。L4 以降は見た目の差で、任意で直せばよい。

条件: 画面は出さず QT_QPA_PLATFORM=offscreen だけで動かした。スクリプトと出力は `scratchpad/verify_layout/` (v1_layout.py〜v6_v02.py、vc.py) に置いた。プロジェクトのファイルは変更していない (nv_ui_qt/*.py の SHA256 が開始時と同じ)。repro/out/ には書いていない。本家バンドル・nvh・make は使っていない。

## 実行した検査 (観測)
| 検査 | 結果 | 根拠 |
|---|---|---|
| selftest | 43/43 PASS | 新規に実行した `selftest.log`、36.5 秒。Traceback 0 件 (標準エラーは offscreen の propagateSizeHints 警告だけ) |
| 既定 == nv.process_frame_v5 / V5_DEFAULTS | 一致 | v2_func.py |
| mix を保存しても Enable が off → backend の mix は 0、出力は既定とビット一致 | 一致 | v2 |
| Frequencies の行 → bamt_{Y,Cr,Cb} を同じ値にし、Undo 1 回で 3 つとも戻る。成分ごとの値が違うと "mixed"、もう一度動かすと 3 つ揃う | OK | v2・v4 |
| Mix: Enable → 内部 25 (表示 75%)・backend 0.25・Undo 1 件。表示 60% → 内部 40。Undo で元に戻る。古いプリセット {mix:30}→Enable、{mix:0}/{}→off、{mix:30, mix_enable:false}→off | OK | v2・v3 |
| tune: all +50% で 3 成分の曲線が ×1.5、Cr −20% と組み合わせると Cr は ×1.2。プレビューに効き、JSON に保存され、tune のない古い JSON は 1.0 として読み込む。文字列・None・未知のキーは拒否 | OK (NaN だけ NG → L2) | v2 |
| 押している間だけ元画像 ("Filtered" が消える)・離すと戻る・Preview off | OK | v2 |
| プレビュー範囲: 範囲の外は元画像とビット一致し、中は処理される。off で全面、選択なしで on にすると中央 50% | OK | v2 |
| サムネイル: 青は現在のフレーム、オレンジは ±Radius (2→4 枚、4→8 枚、Temporal off→0 枚)、クリックで +4 のフレームへ移り、プレビューも追従する。末尾では空きになる | OK | v2 |
| Beginner では行を隠すが値は残る (noise_Cr 130 → backend は不変、Advanced に戻すと +30%) | OK | v2 |
| 選択範囲を描くと Build Profile▾ (メニューも)、ビューアをクリックで Auto Profile に戻る | OK | v2 |
| 実装済みの全キーにウィジェットがあり、操作で backend の値が変わる | 全キー OK | v2 |
| 書き出し CRF0 (既定以外: tune all 1.3/Cb 0.7、Mix 20、Frequencies Low 60、Cr Mid 140、Radius 3) == backend.process_frame (書き出した ui_profile/ui_params を使用)。書き出したプロファイル == UI の effective_profile | 3/3 フレーム一致 | v5_export.py done (成分ごとの設定が異なるので nv.process_frame_v5 とは別物になることも確認) |
| D8/D10: 書き出しの取り消し・ウィンドウを閉じる → プロセスグループ (7 個) が 0、部分ファイルを削除 | OK | v5_export.py cancel/close |
| D3: 1080p で AFT 中に閉じる → 0.07 秒で戻る。QThread の警告なし、spawn の残りなし | OK | v6_v02.py |
| D7: メモリ上限 0.5GB で `1920x1080 · preview 172/721 frames (memory cap)` | OK | v6 |
| test_v02 読み込み + Auto Profile 3.0 秒、area [0,976,128]、1080p プレビュー 2.6 秒 | OK | v6 |
| 幅 1440/1100/900/700 × Advanced/Beginner × 3 タブ (すべて展開) と Prepare: 横にはみ出す行なし (ウィンドウの最小幅は 1084px) | OK | v6、w*_*.png |
| D1/D2/D4/D5 | selftest の該当項目が PASS | selftest.log |

## 欠陥
- **L1 (低〜中、以前からある。D6 は不完全)** スライダーをドラッグすると、動かすたびに Undo の段が増える。
  - 原因 (観測): rows.py:155 `lambda a: None if a == QSlider.SliderMove else self.editStarted.emit()` で、actionTriggered が渡すのは int (7) だが、PySide6 6.12 の `QSlider.SliderMove` は Python の Enum で `7 == QSlider.SliderMove` は False になる。そのため移動のたびに new_edit が呼ばれる。
  - 再現: v4b.py。ハンドルを押して 5 回動かして離すと、Frequencies Mid・amt_Y とも 5 件になり、merge_key の seq が 2,3,4,5,6 と増える。プロファイルの Noise Level の行も同じ (v3 で 3 件)。
  - 旧版の sliders.py:43-44 も同じコードなので以前からある (推論。旧版は実行していない)。前回の判定の「実マウスのドラッグ → 1 件」は誤り。
  - 修正案: `a == QSlider.SliderMove.value` で比べるか、`QAbstractSlider.SliderAction(a)` に変換してから比べる。
- **L2 (低、D5 の系統)** tune に NaN がある JSON を読み込むと受け付けてしまい、on_profile で `ValueError: cannot convert float NaN to integer` が出る (rows.py:169)。表示の更新は途中で止まり、曲線は NaN になる。
  - 再現: 保存したプロファイルの `"tune": {"all": NaN}` を Load Profile する (v2 の bad tune {'all': nan})。
  - 修正案: from_json で `math.isfinite` を確かめる。
- **L3 (低、D9 が一部後退)** 実装していない項目のうち、スライダーの行は値の欄に "n/a" と出る。一方、Enable のチェックボックス (Dust and Scratches・Flicker・Sharpening・Edge Smoothing・Artifact Removal・Adapt to Changing Noise・Jitter Extended Filtration・Prevent Over-Sharpening)、Quality Mode / Dust Mode のコンボ、Noise Level Check などの Begin ボタンは、灰色になるだけで目に見える n/a の表示がない。
  - 特に Beginner の Temporal タブでは Flicker と Dust and Scratches が見出しだけなので、使えないことが分からない (adjust_beg_temporal.png)。
  - 旧版は "(n/a)" と見出しに "— not available" を付けていた。
  - 再現: v2 の最後の検査が一覧を出す。
- **L4 (見た目、要確認)** Temporal の "Noise Reduction Amount" が最上位の見出しになっている。本家の Advanced の Temporal タブ (p23_0、閉じた状態) の最上位は Temporal Filter / Temporal Noise / Repeated Frames / Jitter of Details / Dust and Scratches / Adapt の 6 つだけで、Noise Reduction Amount は Temporal Noise の中の子グループとみられる (推論。LAYOUT_WEB の資料の順とも合う。バイナリの 0x5b は子グループで、親は特定されていない)。
- **L5 (見た目)** Noise Level 系のスライダーの範囲が本家 (−100〜+150) と違うため、+0% のつまみの位置がずれる。見出しの Overall は −75〜+300 なので左端寄り (adjust_adv_spatial.png と p31_0 を比べると分かる)。Luminance/Cr/Cb と Frequencies は −100〜+100 で中央。表示だけの問題で、内部の値の範囲は state の Spec。
- **L6 (見た目)** Mix with Original の Enable が off のときに Mix が "100%" と出る。本家 (m4) は Enable off でも 75% を灰色で表示する。
- **L7 (配置)** Beginner の General タブに Mix with Original が出る。バイナリの Beginner (FUN_000c74e0) の General は Performance だけ。設計書は Mix を残す指示なので、設計書とバイナリが食い違っている (バイナリを優先する約束)。
- **L8 (見た目)** イコライザの縦軸の目盛りが 35px の枠で切れ、"+200%" と "+100%" が "200%" と "·100%" と表示される (prepare_adv.png)。curve_editor.py の drawText の幅が足りない。
- **L9 (見た目)** Prepare ページでもサムネイルにオレンジの枠 (±Radius) が出る。本家の Prepare (p14) は青い枠だけ。
- **L10 (表記)** Radius の値が "2 frames" になっている。本家の行は "2" (p23・db-1-10)。"N frames" はコンボの選択肢の文字列。
- 参考 (欠陥ではない):
  - Prepare の下段 3 区画は、主画面と同じ倍率・中心を共有するため、選択範囲が区画の外に出ることがある。本家の p14 では灰色の枠が各区画の中央にあり、選択範囲を中心に表示しているとみられる (推論)。
  - Undo/Redo は Edit メニューにある (本家は Profile / Filter メニューにもある)。
  - PARAM_SPECS の sharp_very-low・sharp_ultra-low はウィジェットがない (プリセット互換のための未使用のキー)。

## 配置の照合 (観測。layout_adv.txt / layout_beg.txt / menus.txt、スクリーンショットは v1/)
- Spatial (Advanced) は p31_0 とほぼ同じ: Spatial Filter (Enable・↺) → Filter Tuning Assist → Quality Mode → Noise Reduction Amount (Luminance、Chrominance の Cr→Cb、Frequencies の High→Ultra Low) → Noise Level (見出しのスライダー、Luminance→Cr→Cb、Frequencies) → Artifact Removal → Edge Smoothing → Sharpening (High/Range…、Channels、Prevent) → Per-component bands (repro) → Advanced (repro)。色の丸・帯域の模様・"off"・"+0%"・⋈ もある。
- Prepare: Device Noise Profile → アイコン → 名前 (* 付き) → Device Name and Mode → 情報欄 → Quality/Match → (Adv) Main Sample/Selection/Noise Levels → (Adv) イコライザ (All/Y/Cr/Cb、AFT/Manual/Auto Complete、✕) → Profile Check and Tuning (Profile Check、Noise Level と Adv の Luminance→Cr→Cb、Frequencies は n/a) → Repro。イコライザが Profile Check and Tuning より前で、バイナリの順と一致する。
- ビューア: YCrCb + の 1+3 Bottom、各区画に "Filtered"、下に Y/Cr/Cb Enhanced。下のバーは Fit・YCrCb +・✋・☼・W/H・R/G/B・Frame・ⓘ。最下段は P/γ/x1.0 (無効)、サムネイル 11 枚、Cancel/Apply。メニューは Effect/Edit/View/Profile/Filter/Tools/Help で、右隅に Mode ▾。いずれも db-1-10・p14 と同じ構成。

## 推奨
L1〜L3 を直してから承認したい (どれも数行の修正)。L4〜L10 は任意。バックエンド (nvfilter.py 10/16、denoise_video_v5.py 10/16) は変わっていない。backend.py の変更は hh1_sigma (表示用) を足しただけ、export.py は変わっていない。

---
# Round 2 (L1〜L10 の修正後)

総合判定: **REQUEST CHANGES (軽微、2 点)**。L1〜L10 は L5 を除いて修正を確認した (L5 は部分的)。ただし L1 の修正で新たな後退 N1 が起き、L3 の修正で軽いはみ出し N2 が起きた。

変更されたファイル (開始時の SHA256 との比較): main_window.py・selftest.py・state.py と、widgets/ の curve_editor.py・filter_panel.py・profile_panel.py・rows.py・viewer.py。backend.py・export.py・nvfilter.py・denoise_video_v5.py は変わっていない。検証中にファイルは変わっていない (sha_r2.txt)。

## 再検査 (観測)
| 検査 | 結果 |
|---|---|
| selftest | **47/47 PASS**、Traceback 0 件 (selftest_r2.log) |
| 既定値で process_frame_v5 とビット一致・V5_DEFAULTS | OK (r2_v2.log) |
| 書き出し CRF0 (tune・Mix・Frequencies・成分別・Radius 3) == backend.process_frame | 3/3 一致 (r2_v5done.log) |
| D8/D10: 取り消し・閉じる → プロセスグループ 7 → 0、部分ファイル削除 | OK |
| D3: 1080p で AFT 中に閉じる | 0.08 秒、警告なし |
| D7 のラベル・test_v02 の読み込み (4.4 秒)・1080p プレビュー (2.6 秒) | OK |
| 実装済みの全キーが backend に効く・サムネイル・押している間の元画像・プレビュー範囲・Beginner で値を保持・Build Profile▾・Mix の意味・古いプリセット | OK (r2_v2.log。undo の件数を見る検査の NG は、私のスクリプトが count で数えていたためで、v3/v4 で index を使って確かめ直した) |
| 範囲を広げた影響 | 古いプリセットの noise_Y 180/240 はそのまま、300 は 250 に制限される。backend の channel_noise と band_noise は値 /100 をそのまま渡す (nvfilter 側に制限はなく、σ に掛かるだけ)。最大値 250 での処理で例外なし |
| 標準エラーの例外 | 全ログで 0 件 |

## L1〜L10
| ID | 判定 | 観測 |
|---|---|---|
| L1 | FIXED (ドラッグ) | ハンドルを押して 5 回動かすと Undo 1 件 (Frequencies 行・amt_Y・tune とも)。キー操作 3 回は 3 件。溝を押したまま (ページ送り) は 1 件。v4b.py・v3_undo.py |
| L2 | FIXED | tune に NaN・文字列・None があると拒否し、状態は変わらない。曲線・係数・σ が有限数でない場合も拒否するようになった |
| L3 | FIXED | Quality Mode と Begin の行、見出し (Flicker・Dust・Artifact・Edge・Sharpening・Adapt)、Enable に "(n/a)" が付く (adjust_beg_temporal.png・panel_full_spatial.png)。私の v2 の検査は型だけを見ていたので NG と出るが、画面で確認した |
| L4 | FIXED | Noise Reduction Amount は Temporal Noise の子になった |
| L5 | 部分的 | 成分別と Frequencies は −100〜+150 で、+0% の位置が本家と同じになった。見出しの Overall は内部の範囲が 25..400 (−75〜+300) のままで、つまみが左寄りに残る (実装者も同じ説明。許容してよい) |
| L6 | FIXED | Enable が off のとき Mix は 75% |
| L7 | FIXED | Beginner の General は Performance だけ |
| L8 | FIXED | 目盛りに +200% と +100% が切れずに出る |
| L9 | FIXED | Prepare ページではオレンジの枠が出ない |
| L10 | FIXED | Radius は "2" と表示 |
| 追加 | 確認 | Prepare の下段 3 区画が選択範囲を中心に表示する (prepare_adv.png) |

## 新しい欠陥
- **N1 (低〜中、D6 の後退)** マウスホイールの操作が、直前の同じキーの Undo の段に併合される。
  - Qt のホイールは actionTriggered(SliderMove) を出すので、L1 の修正後は editStarted が出なくなった。
  - 再現 (v7_wheel.py): amt_Y を 70 から 35 にドラッグし、ホイールで 2 段上げると 55 になる。Undo の段は 1 件だけで、Undo 1 回で 70 に戻る (ドラッグの段が失われる)。キー操作 1 回 + ホイール 1 回でも 1 件。1.2 秒あけた 2 回のホイールも 1 件。
  - 修正案: `a == SliderMove and self.slider.isSliderDown()` のときだけ併合する。ホイールは押していない状態の SliderMove なので、別の段になる。
- **N2 (低、見た目)** Advanced の Temporal タブで、すべて展開したとき、サイドパネルの幅が 336px 以下だと本文の最小幅 (318px) が表示幅 (306px / 290px) を超え、右端 12〜28px が切れる (横スクロールは無効)。
  - 原因: BeginRow の題名に "(n/a)" が付いて長くなった (Repeated Frames Check (?) (n/a) の行が 318px)。
  - 再現: v6_v02.py の幅 1100/900/700 の検査、v8_tmp.py と v8/w1100_temporal.png。幅 1440 (パネル 350px) では問題ない。
  - 修正案: 題名の折り返しを許すか、"(n/a)" をボタン側に移す。

## 推奨
N1 を直せば APPROVE (N2 は任意)。それ以外の回帰はない。

---
# Round 3 (N1・N2 の修正後)

総合判定: **APPROVE**

変わったファイル: widgets/rows.py、widgets/filter_panel.py、selftest.py (sha_r3.txt と比べて検証中に変化なし)。

| 項目 | 判定 | 観測 |
|---|---|---|
| selftest | 49/49 PASS | selftest_r3.log。FAIL・Traceback とも 0 件 |
| 既定値でビット一致 (process_frame_v5・V5_DEFAULTS) | OK | v9_r3.py |
| N1 (ホイールの併合) | FIXED | v7_wheel.py。ドラッグ (35) の後にホイール 2 回 (55) → 3 件で、Undo 1 回で 45 に戻る。キー操作の後のホイール → 2 件。間をあけた 2 回のホイール → 2 件。ハンドルのドラッグは Frequencies 行・amt_Y とも 1 件のまま (v4b.py)。溝を 1 秒押し続けると 2 件 (ページ送り 1 回ごとに 1 件。仕様どおり) |
| N2 (狭い幅) | FIXED | v6_v02.py。幅 1440/1100/900/700 × Advanced/Beginner × 3 タブ (すべて展開) と Prepare で、横にはみ出すものなし |
| L3 (n/a の可視表示) | 維持 | implemented=False の全項目が無効で、本体・行のラベル・見出しのどれかに "n/a" がある (v9_r3.py、漏れ 0 件)。無効の Begin ボタンは "Begin (n/a)" |
| D3・D7・test_v02 | OK | AFT 中に閉じて 0.07 秒、メモリ上限のラベル、読み込み 3.2 秒、プレビュー 2.6 秒 |

## Overall Noise Level の範囲についての訂正
- Round 2 で「見出しの Overall は −75〜+300 のまま、つまみが左寄り」と書いたのは誤りだった。
- 私は state の Spec (25..400) を見て書いただけで、スライダーそのものは確かめていなかった。
- 今回の観測: スライダーの範囲は −100..150 で、既定 +0% のつまみは 100/250 の位置にある (Round 2 のスクリーンショット panel_full_spatial.png でも約 40% の位置で、実装者の説明と一致する)。
- 左端まで動かすと内部値は Spec の下限 25 (−75%) に制限され、右端では 250 (+150%) になる。
- L5 は FIXED に改める (左端の −100〜−75% は Spec の下限で止まるだけで、許容してよい)。

## 推奨
APPROVE。中核の正しさ、D1〜D10、L1〜L10、N1・N2 はすべて満たしている。
