# INT-4 (1) 作業者報告 (int2-worker、SendMessage の転記。本文は無改変)

INT-4 の 1 の報告 (1/2): OFX で、インターレース素材をプラグインの中でフィールドに分けて処理するようにした。この通は設計と変更ファイル。worktree は .claude/worktrees/int2 (INT-2/3 の上)。コミットはしていない。native/ は変更していない (INT-2 の差分のまま)。

## 設計の決定
1. **走査方式を決めるのは利用者** (本家の観測に合わせた)
   - 走査方式は、ヘルパー (MeatVideoQt --ofx-session) の走査方式ボタンで決める。Resolve のクリップ属性やプロジェクト設定には従わない。
   - ホストのフィールド順 (Source clip の kOfxImageClipPropFieldOrder) は、ウィンドウの既定値としてだけ使う。
   - ClipParams は version 1 のまま、インターレースのときだけ 2 つ足す。progressive の ClipParams は 1 バイトも変わらない。
     - 最上位に `"scan": "tff"|"bff"`
     - `render.P["interlace"] = {"first","mode","radius"}`
   - レンダーは、render.P["interlace"] があればフィールド単位で処理する。ホストがフィールドの情報を渡さなくても同じ。
2. **OFX の行の向き**: OFX の画像の行は下から数える。kOfxImageFieldLower は下から 0, 2, 4… の行、Upper は 1, 3, 5… の行。
   - 上からの偶奇は `(H − 1 − up) & 1` で求める。偶数の高さでは Upper → 上が先 (TFF)、Lower → 下が先 (BFF) になる。
3. **ホストがフィールドの情報を渡さない場合**
   - FieldOrder が None か、取得できないとき: ClipParams の scan に従う。無ければ progressive。ウィンドウの既定値も progressive。
   - ホストがインターレースと言っているのに走査方式が Progressive のとき: フレーム単位で処理し、警告を出す。警告文は "the host reports interlaced material; the Video Scan Type … is Progressive …"。
4. **フィールドごとのレンダーを要求された場合** (kOfxImageEffectPropFieldToRender が Lower / Upper)
   - フレーム全体をフィールド単位で計算し、要求されたフィールドの行だけを書く。
   - そのため、2 回呼ばれると計算も 2 回になる。キャッシュはしていない。
   - FieldRenderTwiceAlways = 0 と FieldExtraction = Both は従来どおり。
   - 1 フィールドだけの画像 (kOfxImagePropField が Lower / Upper) を渡すホストは、メッセージを出して拒否する。
   - 従来の「インターレースのクリップ / フィールドごとのレンダーはエラー」は、ここに書いた動作に置き換えた。
5. **レンダーのフィールド処理** (ofx/src/core.cpp の process_fields)
   - qtui の fields.cpp と同じ手順で、C ABI (nvc_temporal_shift / nvc_spatial / nvc_mix) だけを使う。4:4:4 フル範囲の内部プレーン。
   - process_frame は、時間段と仕上げ (temporal_stage / finish_stage) の 2 つに分けて共通化した。progressive の演算は同じ。
   - GetFramesNeeded は、インターレースのとき t ± (半径/2 + 1) フレームを要求する。
6. **ヘルパー**
   - リクエストのヘッダに `"field_order": "tff"|"bff"` を足す。ホストがフィールド順を報告したときだけ。
   - `ofx_scan` が立つと、走査方式ボタンが有効になる。
   - 既定はホストの順序。ClipParams に scan があればそちらを戻す。走査方式を戻してからプロファイルを戻すので、リセットは起きない。
   - Prepare で Two fields / One field を選べる。
   - Apply すると、走査方式と field の処理を ClipParams に書く。
7. **L-3 (Undo と走査方式)**: 「履歴を切る」方を選んだ。
   - 走査方式の変更でプロファイルをリセットしたら、Undo の履歴を消す。
   - 理由: 走査方式の変更を Undo の単位に含めると、読み直し (動画の再読み込み) まで Undo することになり、元に戻せない。また、別の走査方式で作ったプロファイルが Undo で戻ると、食い違った状態になる。
   - 読み直しの経路は、もともと install_frames が履歴を消している。
8. **既定のモードと半径**: INT-3 の仮の既定 (same、2R フィールド) のまま。e9-prep の結果を待つ。

## 変更したファイル
- **ofx/**
  - src/plugin.cpp: フィールド順の読み取りと上下の対応、レンダー、GetFramesNeeded、ウィンドウへの順序の受け渡し、拒否の置き換え、説明文
  - src/core.{h,cpp}: process_fields と field_margin、段の分割
  - src/params.{h,cpp}: P["interlace"] の検査と読み取り
  - src/session.{h,cpp}: ヘッダの field_order
  - tools/minihost.cpp: フィールド順が Lower / Upper のときは画像の kOfxImagePropField を Both にする。`--field-to-render` を追加。
  - tools/verify_ofx.py: インターレースの 6 シナリオと Python の参照を追加。インターレースの拒否 2 件を削除。
- **qtui/**
  - src/app/ofx_session.{h,cpp}: scan の読み書き、ホストの順序、ボタンの有効化
  - src/app/mainwindow.{h,cpp}: ofx_scan、Undo 履歴の消去
  - tests/ofx_session_drv.cpp: 手順 scan と fprof、フィールドの process
  - tests/selftest.cpp: int4ScanChangeClearsUndo
- **控え**: work/omo/wp_interlace/INT-4/backup/ (15 本、SHA256SUMS 付き)
- **ログ**: INT-4/logs/

---

INT-4 の 1 の報告 (2/2): 試験はすべて合格した。progressive は変更前とバイト一致、インターレースは C++ 同士でも Python の参照ともビット一致した。ログは work/omo/wp_interlace/INT-4/logs/。

## progressive が変わっていないこと
- **verify_ofx --quick** (最終、既定の作業ディレクトリ): 101 比較で 0/84,769,280 語の不一致、ALL MATCH、ini も変わらず (verify_ofx_quick_final.log)。
  - 内訳は、従来の 81 比較 (すべて MATCH) と、新しい I-* の 20 比較。
- **変更前との直接比較**: 変更前のバンドル・minihost・ヘルパー、変更前の verify_ofx.py を写しの木で回した (ALL MATCH)。
  - その出力と、新しい版の出力を比べた。対象は、旧シナリオのプラグイン出力と、押下ごとの ClipParams。
  - 結果: 75/75 バイト単位で同一 (ofx_old_vs_new.log)。refusal の出力も含む。
- **verify_modal** (`--control`、MV_MODAL_WAIT=OFF の対照バンドルを自分で作った): ALL OK (verify_modal.log)。
  - 1080p×11 フレームの open time の比は 0.96 / 1.04 (基準は 1.5 以下)。
- **qtui のクリーンビルド**: 新しいディレクトリで warning 0。
- **ofx のクリーンビルド** (`build_ofx.sh <新しい OUT>`): warning 0。対照の plug-in のビルドも warning 0。
- **qtui_selftest**: 138 passed / 0 failed。新しい int4ScanChangeClearsUndo は単独でも PASS。
- **compare_export** (写し): 19/19 SAME。変更前の qtui_export とも 41/41 SAME。
- **verify_interlace** (INT-3): ALL OK。
- **ini**: sha256 45d4258c…fcc6 は変わっていない。

## インターレース (verify_ofx の I-*、minihost)
- **参照**: qtui の process_fields を Python で書き直したもの。fields_ref の temporal_shift (t_noise を含む) と、Python UI の backend.finish (フィールドに掛ける) を使い、フレーム t−m..t+m で計算する。
- **I-synth** (TFF のクリップ、合成した ClipParams): 7 レンダーすべて MATCH。
  - same R4 (t = 0/3/7)
  - bff shift R3 (One field bottom、ホストの順序と逆)
  - plain R2 + Mix 30 + Cb の量 60
  - 最後に progressive の状態: フレーム単位で処理され、警告が出る。
- **I-synth-none** (ホストのフィールド順なし、bff same R2、t_noise 60、原点 (10,20)、行間隔が負): MATCH。
- **I-field-lower / I-field-upper** (FieldToRender): そのフィールドの行だけが書かれ、ほかの行はホストのゼロのまま。3 レンダーとも MATCH。
- **I-press-host** (Lower → BFF): 押下で Auto Profile → scan "bff"、profile fields "two"。
  - プロファイルの組み立ては Python と一致 (fields.h の定義どおり)。
  - ウィンドウでの処理 (driver の process) とプラグインのレンダーが MATCH (C++ 同士)。参照とも MATCH。
- **I-press-user** (ホストの順序なし、ウィンドウで scan 0 と fprof one1 を指定): scan "tff"、fields "one"。
  - 2 回目の押下では、ウィンドウが ClipParams から TFF を戻し、process がレンダーと MATCH。
  - その後の scan −1 → Cancel では、インスタンスは変わらない。
- **空振りしないこと**: プラグインの変異 5 種を、写しの木でビルドした plug-in に入れて verify_ofx の I-* を回した。5 種とも検出した。
  - PARITY_BOTTOMUP (行の向きを無視): レンダーが 900,655/921,600 語の不一致、さらにウィンドウの既定値も誤る。
  - SHIFT_SIGN: 679,045 語の不一致。
  - MARGIN: 691,197 語の不一致。
  - ONLY_ROWS (要求外のフィールドの行も書く): 450,327 語の不一致。
  - NO_SCAN_DEFAULT (ホストの順序をウィンドウに渡さない): scan / fields が (None, None) になり、期待の (bff, two) と食い違う。
  - 写しの木には qtui のビルドツリーが無いので、変異の verify は最後の bundled_data 検査で例外になる。シナリオの判定は、その手前の出力から読んだ (mutant_*.log、手順は ofx_mutants.sh.txt)。

## 残るリスクと未確認の点
- **本家の Video Scan Type の選択肢**: 本家が上下の順序も選べるのかは未確認 (統括の画面記録待ち)。こちらは Progressive / TFF / BFF の 3 つで、順序はホストの属性か利用者の指定で決まる。本家が「Interlaced」1 つだけなら、順序はホストの属性で決めることになり、今の作りで対応できる。
- **本家のフィールド構造が見えない**: 本家の表示「Fields: 1920x540x2」や Profiled with「Two fields (1920x540x2)」の内部の構造は解読されていない。こちらの Two fields の定義は INT-3 のまま。
- **Resolve の「インターレース処理を有効化」**: このとき Resolve がどう渡してくるか (FieldOrder・FieldToRender・1 フィールドの画像) は、Resolve 上で確かめていない。minihost で、FieldOrder Upper/Lower/None、FieldToRender lower/upper、Both の画像は試した。1 フィールドの画像はエラーにする。
- **FieldToRender で 2 回呼ばれる場合**: 計算も 2 回になる (キャッシュなし)。
- **Undo**: 走査方式を変えると Undo の履歴が消える (L-3 の方針)。ほかの編集も Undo できなくなる。
- **cocoahost**: verify_modal (progressive) だけを回した。インターレースは minihost で試した。
- **verify_ofx の変更**: インターレースのクリップを拒否する 2 件は、今回の仕様で意味がなくなったので削除した。progressive の既存ケースは変えていない。
- **守ったこと**: Resolve と /Library/OFX/Plugins に触れていない。本家を実行していない。reference/ の Python は読むだけで、変更していない。

---

INT-4 の既定の変更が終わった。インターレースの既定を「mode = same、Temporal Radius はフィールド数で数える (R2 → ±2 フィールド)」に変え、回帰はすべて通った。progressive は変わっていない。native/ も変えていない。コミットはしていない。

## 変更
- **既定値**: mainwindow.h/.cpp の field_radius_pref を "frames" から "fields" にした。
  - 設定の読み込みも、キー interlace/field_radius が無ければ "fields" になる。
  - 利用者が設定に "frames" を保存していれば、それが残る。
- **Preferences の「Interlaced clips (fields)」**
  - 選択肢の並びと文言を変えた: 「Fields: R fields, as the genuine (default)」/「Frames: R frames = 2R fields」。
  - ツールチップを、実際のキー名 (interlace/field_mode・interlace/field_radius) に直した (前は prefs/… と書いていた)。半径 2 の意味も書き足した。
  - コード中の注釈に、根拠として work/omo/wp_e9/eval/E9_interlaced_findings.md を書いた。
- **fields.h**: Interlace 構造体の既定の radius を 4 から 2 にした (UI の既定は field_radius() で決まるので、揃えただけ)。
- **控え**: INT-4/backup_defaults/ (SHA256SUMS 付き)。

## 試験の期待値を直したところと、その理由
理由はどれも、既定の半径が 2R=4 から R=2 に変わったこと。
- **selftest の int3ScanDefaultsAndFieldProfile**
  - Adjust の表示: "radius 4 fields" → "radius 2 fields"
  - proc_params の interlace: radius 4 → 2
- **selftest の int3PrefsFieldMode**
  - 初期値の期待: "frames" → "fields"
  - 試す組み合わせを「shift + frames → radius 4」に入れ替え、既定に戻すと radius 2 になり、interlace/field_radius = "fields" が保存されることを確かめる形にした。
- **verify_interlace と verify_ofx**: 直す必要はなかった。
  - 合成のケースは、どちらも interlace のパラメータを明示しているので、既定に依存しない。
  - verify_ofx の押下のシナリオは、ウィンドウの既定で ClipParams を作る。その ClipParams を参照側もそのまま使うので、期待値は変わらない。
  - 押下のシナリオの ClipParams が radius 2 になったことは確かめた: I-press-host は {"first":1,"mode":"same","radius":2}、I-press-user は first 0 で同じ。

## 回帰 (ログは INT-4/logs/*_defaults.log)
- qtui_selftest: 138 passed / 0 failed。変えた 2 項目と int4 は、単独でも PASS。
- verify_ofx --quick: 101 比較で 0/84,769,280 語の不一致、ALL MATCH。
- 変更前のバンドルとの比較: 旧シナリオの出力と ClipParams が 75/75 バイト単位で同一。
- verify_interlace: ALL OK。
- compare_export (写し): ALL SAME。変更前の qtui_export とも 41/41 SAME。
- verify_modal (--control): ALL OK。
- クリーンビルド: qtui と ofx (build_ofx.sh を新しい OUT で) の warning 0。
- ini: sha256 45d4258c…fcc6 は変わっていない。

---

INT-4 の 2 点への対応 (1 通にまとめた): 既定の変更は前の報告と行き違いで、すでに入っていた。今回はツールチップの注記を足し、回帰を回し直してすべて通った。progressive は変わっていない。native/ は変えておらず、コミットもしていない。

## 1. 既定 (same、Temporal Radius はフィールド数)
**入っていることの確認**
- `mainwindow.h:92` の field_radius_pref が "fields"。
- `mainwindow.cpp:435`: 設定にキー interlace/field_radius が無ければ "fields"。
- Preferences の選択肢は「Fields: R fields, as the genuine (default)」/「Frames: R frames = 2R fields」。mode の既定は same のまま。
- OFX ヘルパーで新しく作ったインスタンスにも、この既定が入る。ヘルパーの MainWindow が同じ prefs を読むため。
  - 確認: verify_ofx の押下のシナリオで、ウィンドウが書いた ClipParams が I-press-host `{"first":1,"mode":"same","radius":2}`、I-press-user `{"first":0,"mode":"same","radius":2}` になっている。
- 控え: INT-4/backup_defaults/

**直した期待値** (理由: 既定の半径が 2R=4 から R=2 に変わったため)
- selftest の int3ScanDefaultsAndFieldProfile
  - 表示: "radius 2 fields"
  - P の interlace: radius 2
- selftest の int3PrefsFieldMode
  - 初期値の期待: "fields"
  - 試す組み合わせ: shift + frames → radius 4
  - 既定に戻すと radius 2、保存値は "fields"
- verify_interlace と verify_ofx の I-*: 変更は要らなかった。
  - 合成のケースは interlace のパラメータを明示しているので、既定に依存しない。
  - 押下のシナリオは、ウィンドウが書いた ClipParams を参照もそのまま使うので、期待値は既定に依存しない。

## 2. 走査方式ボタンのツールチップ (今回追加)
次の文を repro の注記として足した。
"repro: the genuine offers Progressive / Interlaced only; its field order is presumably taken from the host's clip properties (TFF material was processed correctly with no field order set). Here the order is chosen explicitly (top / bottom field first), defaulting to the detected one"
- この文言に依存する試験はない。

## 回帰 (最終、ログは INT-4/logs/*_final*.log)
- qtui_selftest: 138 passed / 0 failed。
- verify_ofx --quick: 101 比較で 0/84,769,280 語の不一致、ALL MATCH。I-* の 20 比較も含む。
- 変更前のバンドルとの比較: 旧シナリオの出力と ClipParams が 75/75 バイト単位で同一。
- verify_interlace: ALL OK。
- compare_export (写し): ALL SAME。変更前の qtui_export とも 41/41 SAME。
- verify_modal (--control、MV_MODAL_WAIT=OFF の対照): ALL OK。
- クリーンビルド: qtui と ofx (build_ofx.sh を新しい OUT で) の warning 0。
- ini: sha256 45d4258c…fcc6 は変わっていない。

---

INT-4 の 2 の項目 (repro の注記) を入れた。selftest と verify_ofx --quick はどちらも通った。

## 変更 (qtui/src/app/mainwindow.cpp のみ)
- 定数 SCAN_REPRO_NOTE を 1 か所に置いた。
  - 文言: "repro: the genuine offers Progressive / Interlaced only (field order not selectable; presumably taken from the host: TFF material was processed correctly with no field order set). Here the order is chosen explicitly (top / bottom field first), defaulting to the detected one"
- 走査方式ボタンのツールチップ: 従来の「Video Scan Type: … (detected: …). Interlaced clips are profiled and processed field by field.」の次の行に、この注記を足した。
  - 前回の報告で「入れた」と書いた注記は、これと同じ内容の文言を直接書いたもので、メニュー項目には付いていなかった。今回、定数にまとめてメニュー項目にも付けた。
- メニューの 3 項目 (Progressive / Interlaced, top field first / Interlaced, bottom field first): 各項目のツールチップを「項目名 + 改行 + 注記」にした。QMenu::setToolTipsVisible(true) で表示されるようにした。
- 控え: INT-4/backup_defaults/qtui/src/app/mainwindow.cpp.before_menu_tooltip

## 試験 (ログは INT-4/logs/*_note.log)
- qtui_selftest: 138 passed / 0 failed。
- verify_ofx --quick: 101 比較で 0/84,769,280 語の不一致、ALL MATCH (ofx/build_ofx.sh で作り直し、ヘルパーのドライバも作り直した後)。
- ビルドの warning は 0 (qtui build_main・ofx・ドライバ)。
- コミットはしていない。native/ も変えていない。

---

# Round 2

INT-4 Round 2 の報告: M-1・L-1・L-2 を直し、回帰はすべて通った。progressive は変わっていない。native/ は変えておらず、コミットもしていない。控えは INT-4/backup_r2/ (SHA256SUMS 付き)、ログは INT-4/logs/*_r2.log。

## 変更
**M-1 (既定から作った ClipParams で、ホストのフィールド順が上書きされる)**
- 直したのは qtui/src/app/ofx_session.cpp の apply_clip_params。判定は次のとおり。
  - version 2: "scan" (tff / bff) に従う。
  - version 1 で frame_size があるもの (適用済みの progressive のインスタンス): Progressive にする。従来どおり。
  - version 1 で frame_size が null のもの (Default Profile / Preset から作ったもの): 走査方式を変えない。つまり、ウィンドウが始めた値 (ホストのフィールド順) が残る。
- verify_ofx にシナリオ I-default-host を足した。ini "set" (既定あり) で、FieldOrder Lower。
  - 押下前のレンダー: 保留中の既定で progressive に処理され、警告が出る。
  - 押下して Apply すると、ClipParams の scan が "bff" になる。プロファイルは既定のフレームのプロファイル (fields None) のまま。
  - そのあとのレンダーは、ウィンドウでの処理 (driver の process) とも参照ともビット一致した。

**L-2 (インターレースのときだけ version 2)**
- 書く側 (clip_params_text): インターレースのときだけ version 2 にする。progressive は version 1 のままで、テキストは 1 バイトも変わらない。
- 読む側 (プラグインの params.cpp と、ヘルパーの apply_clip_params): version 1 と 2 を受け付ける。
  - version 1 が "scan" か render.P["interlace"] を持っていたら拒否する。
  - version 2 で、scan が無い、interlace が無い、または interlace.first が scan と食い違う場合も拒否する。
  - どちらもメッセージを出す。3 以上は「version N is not supported (1, 2)」。
- 変更前 (INT-3) のプラグインの挙動: version 2 の ClipParams を渡すと「ClipParams: version 2 is not supported (1)」(status 1) で止まる。黙って progressive で処理することはなくなった (変更前の minihost とバンドルで確認、old_plugin_v2.log)。
- verify_ofx の変更
  - 既存の「読めない ClipParams」の拒否のケースは version 2 から 3 に変えた。version 2 は今は正しい値なので。
  - version の組み合わせの拒否を 5 件足し、5 件とも ok。
  - 合成の ClipParams は、インターレースのとき version 2 で作る。

**L-1 (奇数の高さ)**
- clip O を足した。clip A を 352×359 に切り出したもの。
- 足したシナリオ
  - I-odd-upper: FieldToRender upper、bff、same R2、t=3/7
  - I-odd-lower: FieldToRender lower、tff、shift R3、行間隔が負
  - I-odd-both: plain R2
- 結果: すべて MATCH。

## 試験
- **verify_ofx --quick**: 109 比較で 0/89,831,040 語の不一致、ALL MATCH。I-* と、拒否の 5 件を含む。
- **変更前のバンドルとの比較**: 旧シナリオの出力と ClipParams が 75/75 バイト単位で同一。
- **空振りしないこと**: プラグインの変異を 2 種追加し、どちらも検出した。
  - P_ODD_PAD (奇数の高さで、下フィールドの詰め行を H−1 にする): I-odd-* で 43,248〜44,670 語の不一致。
  - V2_ACCEPT_V1_IL (version と interlace の組み合わせの検査を外す): 拒否のケース 2 件が FAIL。
  - 手順は ofx_mutants.sh.txt、ログは mutant_*.log。
- **verify_modal** (--control): ALL OK。
- **qtui_selftest**: 138 passed / 0 failed。
- **verify_interlace**: ALL OK。
- **compare_export** (写し): ALL SAME。変更前の qtui_export とも 41/41 SAME。
- **クリーンビルド**: qtui と ofx (build_ofx.sh を新しい OUT で) の warning 0。
- **ini**: sha256 45d4258c…fcc6 は変わっていない。

## 未対応
- M-1 の修正をヘルパーの変異で確かめることはしていない。変異の試験の仕組みはプラグイン側だけを作り直すため。I-default-host のシナリオで、修正後の挙動を直接確かめている。
