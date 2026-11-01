# VERDICT: wp_automatch/DECODE.md (本家 Auto Match / Match 表示) の独立検証

検証日 2026-11-01。検証者 verify-automatch。本家は実行していない (dlopen・nvh・デバッガなし。Resolve も操作していない)。
DECODE.md の notes/ と tools/ は使わず、自分で作り直した材料だけで判定した:
- Demo arm64 `reference/genuine/NeatVideo6.arm64.ofx` (SHA256 95bef55f…88430)、RS は `/Library/OFX/Plugins/NeatVideo6RS.ofx.bundle/Contents/MacOS/NeatVideo6RS.ofx`
  を `lipo -thin arm64` (SHA256 9ea5627a…10413fe。DECODE と同一)。どちらも `otool -tV` で逆アセンブルを作り直した (scratchpad)。
- 自作スクリプト: moc の QByteArrayData 表から QMetaObject とメソッド表を復元、ジャンプ表の復号、adrp+add / adr 参照の全数探索
  ("1autoMatchProfile()" などの既知の参照を拾えることで探索の動作を確認)。
- どちらのバイナリも LC_DYLD_INFO_ONLY (チェーン型 fixup ではない) で、__DATA の vmaddr = ファイルオフセット。ポインタ値はファイル上の値をそのまま読める。

「観測」= 命令・データから直接読めたこと、「推定」= そこからの解釈。

## 総合判定: **APPROVE**
中心の主張 1〜4 はすべて CONFIRMED。軽微な誤り・見落とし 3 件と、未解決 §6-1/§6-2 を解決する追加の観測がある (「DECODE への修正の推奨」。結論は変わらない)。

---

## 主張 1: autoMatchProfile (moc 33 番) は `ret` だけで、照合の実装はどこからも到達しない — **CONFIRMED**

観測:
- moc を自前で復元: Demo の stringdata 0x1425758 → QMetaObject 0x13c4d88 (super 0x13c2920、data 0x10a5e8c、qt_static_metacall 0x1d8270)、revision 8・53 メソッド。
  33 番 `autoMatchProfile` (argc 0, flags 0xa = public slot)、36 番 `smartProfile` (同)。RS は stringdata 0x15c9a70 → QMetaObject 0x1565098
  (data 0x123d808、metacall 0x222888)、並びは Demo と同一。
- metacall の InvokeMetaMethod 分岐 (`cbnz w1` で _c == 0、`cmp w2,#0x34` で 53 件):
  Demo 0x1d82a0 で表 0x10a5e20、`adr x10,#0x10` (基点 0x1d82b8)、`ldrh`。復号すると 33 → 0x1d85e4 → `b 0xd7430`、36 → 0x1d8608 → `b 0xd7434`。
  RS は表 0x123d79c・基点 0x2228d0 で 33 → 0x222bfc → `b 0xd62a4`、36 → 0x222c20 → `b 0xd62a8`。
- 本体: Demo `0xd7430 ret` / `0xd7434 ret`、RS `0xd62a4 ret` / `0xd62a8 ret` (直前は別関数の `brk #0x1`)。
- 参照の全数探索: 0xd7430/0xd7434 (RS 0xd62a4/0xd62a8) を指す b/bl はそれぞれ metacall の 1 か所だけ。adrp+add・adr は 0 件
  (新形式 connect の PMF はほかのスロットでは `adr` で現れる。例: loadProfile 0xd67b0 などは 0xd5358 等の adr で参照される)。u64・u32 のデータ値としても 0 件。
- 文字列: "1autoMatchProfile()" の参照は Demo 0xed290 (RS 0xec2d8) の 1 か所だけで、`connect(ui.autoMatchButton, "2clicked(bool)", ProfilePage, "1autoMatchProfile()")`
  (0xed178 で x20 = "2clicked(bool)"、0xed284 `ldr x0,[x19,#0xa0]`、0xed288 `ldr x2,[x19,#0xe0]`、0xed110〜0xed114 で +0xe0 ← +0x78)。
  "autoMatchProfile" (接頭辞なし) は moc の文字列表にしかなく、コードからの参照はない (invokeMethod 経由もない)。
- 照合の実装の有無: "match" を含む文字列は Qt・libpng 等の内部を除くと、Auto Match のツールチップ "Automatically find and load a device noise profile that
  matches the input image" (参照 0xed570、ツールバーの retranslate) と Match 欄の文字列だけ。照合・走査らしい文言はない。

**メニュー項目と Ctrl+M (DECODE §6-1・§6-2 は解決できる。DECODE の結論を補強する)**:
- 動作の生成 (FUN_00074430) で "Auto Match" の QAction は **1 つだけ** (0x78148〜0x781f8)。translate("Auto Match") → アイコン "AutoMatch.png" →
  translate("Ctrl+M", 曖昧さ回避 "Auto Match") → 0x781e8 でショートカット設定 → **保持体 +0x1b0 に格納**。
- 保持体 (主ウィンドウ +0x268。0x96748〜0x96754) の各 +off を、生成順の文字列で対応付けた。+0x1e8 は "C&hange Default Profile..." (0x780d4) で Auto Match ではない。
- Profile メニューの組み立て (0x7ea04 内の 0x7f0b0〜0x7f444): +0x188 (&Profile の QMenu) に 30 項目の QList を addActions (0x7f444 `bl 0x74a8c8`)。
  並びは Undo/Redo | Load/Save As/Reset | Copy/Paste | 既定プロファイル 4 項目 | **+0x1b0 Auto Match**、+0x1f8、+0x1f0、**+0x200 Calibration Target**、**+0x208 Smart Profile**、+0x210 Generic |
  AFT 4 項目 | Profile Viewer | Profile Check。つまり **Auto Match は Profile メニューに追加されている** (観測)。
- それでも画面に出ない仕組み: `FUN_0008106c` (Demo。RS は 0x7f364) が保持体の子を走査し、ActionEx (QAction の派生、metaobject 0x13b9cf0) ごとに、
  `data()` が特定の QVariant と等しいものを除いて、`0x3d0a2c(action, "2triggered(bool)")` (QObject::receivers と同形: 正規化・indexOfSignal・signal spy・接続表) が 0 なら
  **QAction::setVisible(false)** (Demo 0x71d244 / RS 0x7ac7ac。QAPP_CHECK の文言 "…before calling 'setVisible'." の参照元から同定) を呼ぶ (0x81134〜0x8114c)。
  続くループは QMenu (metaobject 0x13ee670) の子の可視項目の有無で menuAction の表示を決める (空のメニューを隠す)。この関数は主ウィンドウのイベント処理で、
  独自のイベント型 (大域 0x1538b90) を受けたときに呼ばれる (0xa0c0c〜0xa0c24)。そのイベントは初期化中に `new QEvent(type)` → postEvent される (0x98868〜0x9888c)。
- ProfilePage の動作の接続 (0xd4df0〜0xd5a1c、QObject::connectImpl 0x3d6598 を 1 動作ずつ) で接続される保持体の動作は
  +0x190/198/1a0/1a8/1c0/1c8/c0/c8/1d0/1d8/1e0/1e8/1b8/1f8/1f0/210/218/220/228/230/238/240。**接続されないのは +0x1b0 (Auto Match)・+0x200・+0x208 だけ**。
  RS も同じ (0xd3f3c〜0xd4654 で同じ集合、+0x1b0/+0x200/+0x208 なし)。文字列形式の "2triggered" の connect 44 か所にも +0x1b0 の動作は出てこない。
- 結論 (観測 + 推定): Auto Match・Calibration Target・Smart Profile の 3 動作は triggered の受け手がないので、起動後に非表示になる → 画面の Profile メニュー (menu_profile.png) と一致。
  QAction::setVisible(false) は enabled のビットも落とす (0x71d290〜0x71d2f8) ので、Ctrl+M も効かない (Qt の仕様と一致。推定)。仮に発火しても受け手がない。
  画面では AFT 4 項目と Profile Check も見えないが、これらは接続されているので別の理由 (モード等。未調査)。

## 主張 2: ProfilePageToolbar のコンストラクタが autoMatchButton を setVisible(false) し、再表示しない — **CONFIRMED**

観測:
- Demo 0xeb2e4: vtable 0x13c5c88+0x10 を格納 → `add x0,x0,#0x80; bl 0xeb7fc` (setupUi、Ui = this+0x80) →
  `0xeb32c str xzr,[x19,#0xe0]` / `0xeb330 ldr x0,[x19,#0xa0]` / `ldr x8,[x0]` / `ldr x8,[x8,#0x68]` / `mov w1,#0` / `0xeb340 blr x8`。
  RS 0xea374〜0xea388 も同じ命令列 (setupUi 0xea844)。条件分岐はない。
- +0xa0 = autoMatchButton: setupUi の 0xebc60〜0xebc94 で `new(0x88)` → `0xebc74 str x21,[x20,#0x20]` (Ui+0x20 = this+0xa0) → setObjectName("autoMatchButton")。
  RS 0xeacbc も `str x21,[x20,#0x20]`。さらに 0xed284 の connect の送り手も [x19,#0xa0]。
- vtable 0x68 = QWidget::setVisible: ボタン (vtable 0x13c0190) とツールバー (0x13c5c98) のどちらも slot 12 (0x60) = 0x742fdc `mov w0,#1; ret` (devType = QInternal::Widget)、
  slot 13 (0x68) = 0x753afc。0x753afc は「ExplicitShowHide 属性が未設定ならセットして本体へ、設定済みなら data->widget_attributes のビット 16 (WA_WState_Hidden) と引数を比べ、
  hidden == !visible なら return」という Qt 5.15 の QWidget::setVisible(bool) と同じ構造 (観測。関数名は構造からの同定)。
- ツールバーは 0xf8 バイト (0xd4e0c `mov w0,#0xf8`、new → `bl 0xec728` → `b 0xeb2e4`)、ProfilePage+0x1c8 に保持。
- 再表示がないこと: ツールバーのコード (コンストラクタ・setupUi・moc の 7 スロット 0xec72c〜0xecab0・接続 0xed0f4) で +0xa0 を読むのは 0xeb330 と 0xed284 だけ。
  スロット内の setVisible の仮想呼び出しは +0xd8 と +0xd0 に対するものだけ (0xecee8、0xecf30)。0xf45c4 の `[x19,#0xa0]` は +0x120・+0x1b0・+0x1b8 を持つ別クラス
  (ツールバーは 0xf8 バイトなので入らない)。全体で「+0x1c8 を読んでから +0xa0」を探すと Qt 内部の 1 件 (0x8a3e50、vtable の +0x1c8) だけ。
  オブジェクト名 "autoMatchButton" の参照は setupUi の 1 か所 (findChild なし)。RS の +0xa0 アクセスの分布も同じ形。
- 限界 (推定): 子ウィジェットを一括で走査して setVisible(true) を呼ぶ汎用コードの全数否定まではしていない。なお Qt では親の show は明示的に隠した子を再表示しない。

## 主張 3: Simple/Advanced の Match 欄は非表示で、値は書かれない — **CONFIRMED (表現に注記)**

観測:
- Simple: setupUi 0xdfec8 (呼び出し 0xdfe80 `add x0,x19,#0x68`) で simpleMatchCaptionLabel → Ui+0x30 (0xe0334)、simpleMatchLabel → Ui+0x38 (0xe03ec)、
  simpleQualityLabel → Ui+0x20。パネル基準で +0x98/+0xa0/+0x88。Advanced: setupUi 0xe84d8 (0xe8274 `add x0,x19,#0x68`) で caption Ui+0x38、value Ui+0x40、
  quality Ui+0x28 → +0xa0/+0xa8/+0x90。
- 非表示: Demo 0xdf5f0〜0xdf61c (x24 = this+0xd678、0xdec20 `mov w8,#0xd678`) で simple+0x98 と +0xa0 にそれぞれ vtable+0x68 (w1 = 0)。
  0xdf628 で this+0xd698 ← simpleMatchLabel、+0xd6a0 ← simpleQualityLabel。Advanced 0xe1154〜0xe11d8 (x20 = this+0xd678、0xe0b3c) で caption に
  `setText(" ")` (0xe1178 `bl 0x809dc0`)、value (+0xa8) に setVisible(false)、+0xd698 ← advancedMatchLabel。RS 0xde4f8〜0xde524・0xe00f0〜0xe0104 も同形。
- 読み手: +0xd698 を読むのは 0xdb83c (0xdb858〜0xdb890) だけで、+0x30/+0x40 に色 (0x5a9938) を入れる配色の処理。その呼び出し元は 0xe5a44 だけ。
  0xd690 を基準にする関数 (0xdcfd8、0xe3a3c、0xe40a8、0xe4a5c、0xe5064、0xe5478) で基準+8 を読む命令はない。
- **注記**: 「値のラベルに文字を設定するコードはない」は厳密には不正確。retranslateUi が静的な既定文字列を入れる:
  Simple 0xe0a74 (`ldr x19,[x19,#0x38]` → setText("100%"))、Advanced 0xea814 (`[x19,#0x40]` → "100%")。DECODE §2.3 の末尾でこの既定文字列に触れているので、
  結論 (計算した値を書くコードはなく、式も再計算の契機もない。欄は非表示) は変わらない。

## 主張 4: プロファイルフォルダと保存済みプロファイルの一覧 — **CONFIRMED (列挙子の意味は Qt 5 の定義による推定を含む)**

観測:
- 設定キー "ProfileDir": Demo の参照は 0x1337fc (読み) と 0x135814 (書き) の 2 か所。RS は 0x133f6c・0x135f84。
- 既定のパス (Demo 0x15ec38): `ldrb w8,[x19]` が 0 なら (+0x18) + "/Neat Video v6 OpenFX/Profiles" (0x15ed84) を +0x68 に、非 0 なら (+0x8) + "/Profiles" (0x15ecc0)。
  プリセットは +0x78、+0x68 + "/Generic Profiles" を +0x70。RS は "/Neat Video v6 for Resolve/Profiles" (0x162e74) ほか同形。環境変数は "NV6OFX_PATH" (0x15c9f0) / "NV6RS_PATH" (0x1608c4)、
  QSettings の名前は "ABSoft"/"NeatVideo6-OFX" / "NeatVideo6-RS"。
- 文書フォルダ (+0x18、0x15e87c): `bl 0x984b5c` (NSProcessInfo.environment の objectForKey:。CFString 0x140f818 → 0x133b790 "APP_SANDBOX_CONTAINER_ID"、長さ 24) が偽なら
  `mov w0,#1` → writableLocation (0x15e97c〜0x15e998)、真なら getuid/getpwuid → pw_dir + "/Documents" (0x15ea10)。+0x20 は `mov w0,#9`、+0x28 は "/Library/Application Support/ABSoft/NeatVideo6-OFX"。
- この機械の RS の設定 (`defaults read com.absoft.NeatVideo6-RS ProfileDir`) は "/Users/seitan/Documents/Neat Video v6 for Resolve/Profiles" で、フォルダは空 (読み取りだけで再確認)。
- 一覧 (FUN_00026954): 0x26adc `mov w1,#0x6402` → 0x33e5dc (setFilter)、名前フィルタの設定 (0x33e298)、`x22 != 0 && w24` のとき `mov w1,#-1`、それ以外は `mov w1,#0x14` → 0x33e6b8 (setSorting)、
  0x33ee8c (entryInfoList)。自身への再帰呼び出し 0x271c0・0x27688 と 0x26858 (空フォルダ) がある。0x6402 = Files|AllDirs|NoDot|NoDotDot、0x14 = Name|DirsFirst|IgnoreCase
  (Qt 5 の enum 値。AllDirs なのでフォルダは名前フィルタの対象外)。
- 表示 (0x2342c): "*" + "." + 拡張子 (+0x90) で名前フィルタを作り (0x234d4〜0x2351c)、0x279a0 で組み立て、空なら +0xd8 の文言を使う (0x236d0)。
  ProfilePopupButton の設定 0xd8ec8 (パネル +0x128) で拡張子 "dnp"、空のときの文言 (0xd91bc)、"Show saved noise profiles" (0xd9214)。

## DECODE への修正の推奨 (いずれも結論には影響しない)

1. **(軽微な誤り) §1.3**「"Auto Match" の動作を 2 つ作り、保持体の +0x1e8 と +0x1b0 に置く」は誤り。動作は 1 つ (+0x1b0)。"Auto Match" の 2 回目の参照 (0x78190) は
   ショートカット "Ctrl+M" の translate に渡す曖昧さ回避の引数。+0x1e8 は "C&hange Default Profile..." (0x780d4)。
2. **(解決) §6-1・§6-2**: Profile メニューから外れる仕組みは主張 1 の「メニュー項目と Ctrl+M」のとおり。Auto Match は addActions で Profile メニューに入るが、
   triggered(bool) の受け手がない ActionEx を初期化後のイベント処理で setVisible(false) にする共通の処理 (Demo 0x8106c / RS 0x7f364) で非表示になる。
   Calibration Target (+0x200) と Smart Profile (+0x208) も同じ理由。Ctrl+M は、非表示で enabled も落ちるので効かない (推定)。
3. **(表現) 主張 3**: retranslateUi が Match の値のラベルに静的な "100%" を入れる (Simple 0xe0a74、Advanced 0xea814)。表の「文字を設定するコードはない」は
   「計算した値を設定するコードはない」と書くのが正確。
4. **(見落とし) §3.3**: 空のときの文言 "Neat Video could not find any profiles…" の参照は 2 か所ある。0xd91bc (Device Noise Profile パネルの ProfilePopupButton、+0x128) のほかに、
   0xf80dc で別クラスの +0x1b0 にある ProfilePopupButton ("ProfilePopupButton%1.png"、拡張子 "dnp"、"Show saved noise profiles" も設定) がある。
   画面下端左の "P▾" ボタンとみられる (推定)。qtui で一覧を作るなら、両方の入口が同じ一覧を出すことを確かめるとよい。

## Auto Match が動く構成がないか (反証の試み)
- ボタン: コンストラクタで無条件に隠す (0xeb32c〜0xeb340 は直線)。再表示の経路は見つからない。
- メニュー/ショートカット: triggered の受け手がない限り隠れる。受け手を付けるコードは Demo・RS とも見つからない (ProfilePage の接続一覧、文字列形式の connect 44 件)。
- スロット: どの経路 (ボタンの clicked、メタオブジェクトの index 呼び出し) でも本体は `ret`。PMF・データからの参照はない。
- 別のビルド: Demo と Pro RS の両方で同じ。スタンドアロン版・Premiere 版は範囲外 (DECODE §6-5 と同じ)。
→ この 2 つのバイナリには Auto Match が働く構成はない (上の探索の範囲での全数否定)。

## 範囲外・限界
- 実行時の確認はしていない (方針どおり)。関数名 (QWidget::setVisible、QAction::setVisible、QObject::receivers、QDir::setFilter 等) は構造と Qt 5 のソース・文言からの同定。
- `data()` による除外の条件 (0x810fc〜0x81130) の比較値の意味は読んでいない (Auto Match が画面に出ないことと矛盾しない)。
- 画面で AFT 4 項目と Profile Check が Profile メニューに出ない理由は本件の外 (それらは接続されている)。

Recommendation: **APPROVE** — 主張 1〜4 は自前の逆アセンブルで再現できた。上の 1・3・4 の記述修正と §6-1/§6-2 の解決を DECODE に反映することを推奨する (必須ではない)。
