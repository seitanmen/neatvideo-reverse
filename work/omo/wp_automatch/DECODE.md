# N4e 段階 1: 本家 "Auto Match" と "Match" 表示の静的解読

作成 2026-11-01。本家は実行していない (dlopen・nvh・デバッガとも使っていない)。DaVinci Resolve も操作していない。
材料:
- Demo arm64 スライス `reference/genuine/NeatVideo6.arm64.ofx` (SHA256 95bef55f…8430)。逆アセンブルは `otool -tv` で作り直した
  (scratchpad。`work/omo/wp_spatial/tmp/disasm_full.txt` と 1 行目のパス以外同一)。デコンパイルは Ghidra 12.1.4 の解析なしインポート
  (`notes/tools/dec.sh`・`DecompCreate.java`)
- Pro RS 6.1.3 の arm64 スライス (`/Library/OFX/Plugins/NeatVideo6RS.ofx.bundle` から `lipo -thin` で scratchpad に複写、SHA256 9ea5627a…13fe。読み取りのみ)
- 先行作業の本家 6.1.3 Pro (Resolve) 画面の撮影 `work/omo/coord/genuine_ui/shots/menu_profile.png`、`work/omo/coord/verify_quality/shots/g_prepare_fullframe_q65.png`

アドレスは特記なしで Demo。RS は「RS」と付ける。「観測」= 命令・データ・画面から直接読めたこと、「推定」= そこからの解釈。

## 0. 結論

**本家の OFX/Resolve 版では Auto Match は実装されていない (空関数) うえに、ボタン・Match 表示とも起動時に非表示にされる。**
Demo と Pro RS の両方で同じ (観測)。プロファイルフォルダの照合アルゴリズム・Match の計算式はこのバイナリに存在しないので、
「本家とビット一致する Auto Match」は再現の対象がない。

| 項目 | 内容 | 確度 |
|---|---|---|
| `ProfilePage::autoMatchProfile()` | 本体は `ret` 1 命令 (Demo 0xd7430 / RS 0xd62a4) | 観測 (高) |
| Prepare のツールバーの autoMatchButton | `ProfilePageToolbar` のコンストラクタが setupUi 直後に無条件で `setVisible(false)` (Demo 0xeb330〜0xeb340 / RS 0xea378〜0xea388)。再表示する経路は見つからない | 観測 (高。再表示がないことは参照の全数探索による) |
| "Match" 欄 (Simple・Advanced パネル) | Simple は見出しと値の両方を `setVisible(false)`、Advanced は見出しを " " にして値を `setVisible(false)`。値のラベルに文字を設定するコードはない | 観測 (高) |
| Profile メニューの "Auto Match (Ctrl+M)" | 動作 (QAction) は作られるが、本家 6.1.3 Pro の画面の Profile メニューに出ていない | 画面で観測。メニューに入れない仕組みは静的には未特定 |
| プロファイルフォルダ | 設定キー `ProfileDir`。既定は `<Documents>/Neat Video v6 OpenFX/Profiles` (Demo)・`<Documents>/Neat Video v6 for Resolve/Profiles` (RS)。環境変数 `NV6OFX_PATH` (RS `NV6RS_PATH`) で別の場所 (`$…/Profiles`) | 観測 (高)。分岐条件の細部は §3 |
| フォルダの列挙 (Auto Match ではなく「保存済みプロファイル」のポップアップ) | 再帰、`*.dnp`、フォルダ先頭・大文字小文字を無視した名前順。空なら "could not find any profiles…" を表示 | 観測 (中〜高。Qt の列挙子の値の読みを含む) |

「Match」の意味 (推定): ツールチップ "Profile-image match shows how close are device modes of current noise profile and input image" と、
`DeviceNameModePanel`・`updateProfileDevNameMode`・`saveProfileEXIFToFile`・"Device Name and Mode:" の存在から、Neat Image (静止画版) から引き継いだ
「プロファイルと画像のデバイス・モード (EXIF のカメラ機種・ISO など) の一致度」と、それで最も合うプロファイルを探す機能とみられる。
動画フレームには EXIF がないため OFX 版では無効化されている、というのは推定。

## 1. Auto Match の制御の流れ (問 1)

### 1.1 スロットの特定 (観測)
- moc の文字列表 `ProfilePage` (Demo stringdata 0x1425758、QMetaObject 0x13c4d88、qt_static_metacall 0x1d8270、data 0x10a5e8c)。
  メソッド表の 33 番が `autoMatchProfile` (引数 0)。メソッド一覧は `notes/moc_demo_ProfilePage.txt`。RS も 33 番 (QMetaObject 0x1565098、metacall 0x222888、`notes/moc_rs_ProfilePage.txt`)。
- moc 文字列 "autoMatchProfile" は Demo 0x14263d2 の 1 か所だけで、ほかのクラスにこのスロットはない。
- qt_static_metacall の InvokeMetaMethod 分岐 (0x1d8294〜0x1d82b4) は `ldrh` の表 0x10a5e20 で飛ぶ。33 番 → 0x1d85e4 → `b 0xd7430`。
  0xd7430 は `ret` (次の 0xd7434 も `ret` で、これは 36 番 `smartProfile`)。RS は表 0x123d79c、33 番 → 0x222bfc → `b 0xd62a4` = `ret`
  (36 番 smartProfile → 0xd62a8 = `ret`)。抜粋は `notes/disasm/demo_metacall_slots.txt`・`rs_metacall_slots.txt`
- 0xd7430 を参照するのは metacall の分岐 (0x1d85ec) だけ (adrp/add・データポインタ・bl/b を全数探索)。新形式の connect (メンバ関数ポインタ) からの参照はない。
- 接続 (`FUN_000ed0f4`、ProfilePageToolbar 側): `connect(autoMatchButton (this+0xa0), "2clicked(bool)", ProfilePage (this+0xe0 = this+0x78), "1autoMatchProfile()")` (0xed284〜0xed298)。
  "1autoMatchProfile()" (0x1305810) を参照するのはここだけ。

### 1.2 ボタンの非表示 (観測)
- `FUN_000eb2e4` は `ProfilePageToolbar` のコンストラクタ (vtable 0x13c5c88+0x10 の metaObject() 0x1d9a70 が QMetaObject 0x13c5c58 = "ProfilePageToolbar" を返す)。
  0xeb328 で setupUi (`FUN_000eb7fc`、Ui は this+0x80。autoMatchButton は Ui[4] = this+0xa0)、0xeb32c〜0xeb340:
  ```
  str  xzr, [x19,#0xe0]
  ldr  x0, [x19,#0xa0]      ; autoMatchButton
  ldr  x8, [x0] ; ldr x8, [x8,#0x68]   ; QWidget::setVisible (QObject の 12 個の仮想関数の後、devType 0x60 の次)
  mov  w1, #0x0
  blr  x8                   ; setVisible(false)
  ```
  RS 0xea374〜0xea388 も同じ (`notes/disasm/*_profilepage_ctor_hide.txt`)。
- ProfilePage/ProfilePageToolbar の範囲 (0xd0000〜0xf8000) で `[x, #0xa0]` を読む命令を全数確認した。setVisible に渡す 0xf45c4 は別クラス
  (`FUN_000f4470`、呼び出し元は +0x1b0/+0x1b8 を持つビューアのツールバー系オブジェクト。`notes/disasm/f4470_other_class.txt`) なので、autoMatchButton を再表示する経路はない。
- ボタンは buttonsWidget (固定 495×32、レイアウトなし) の子で、ツールバーの他のボタンも addWidget されていない (`FUN_000eb7fc` の 0x734fc8 呼び出しは buttonsWidget と rightButtonsWidget だけ)。
  配置はコンストラクタで作る 0x28 バイトのオブジェクト (vtable 0x13e41b0) が担うとみられる (推定)。
- 画面の観測と一致: 本家 Pro の Prepare ページのツールバーは "Build Profile / Generic Profile / Load Profile" だけ (g_prepare_fullframe_q65.png)。

### 1.3 メニュー (観測と未解決)
- `FUN_00074430` (アクションの一括生成) が "Auto Match" ("AutoMatch.png"、"Ctrl+M") の動作を 2 つ作り、保持体の +0x1e8 と +0x1b0 に置く (0x78154〜0x78190)。
  同じ関数で "Auto Profile with Calibration Target" (0x7855c) と "Smart Profile" (0x786b4) も作る。
- 本家 6.1.3 Pro の Profile メニュー (menu_profile.png) には Auto Match・Smart Profile・Calibration Target の項目がない。
- これらを Profile メニューに入れない仕組み (メニューの組み立て表) は静的には特定していない (未解決。§6)。

### 1.4 フォルダ列挙・照合・状態表示
Auto Match の本体が空なので、Auto Match としてのフォルダ列挙・.dnp の読み込み・画像側の計算・スコア・同点処理・閾値・メッセージは**存在しない**。
文言 "Neat Video could not find any profiles in the assigned Profile folder:" は Auto Match ではなく、プロファイル名の横の▼ (ProfilePopupButton、
"Show saved noise profiles") が使う (§3.3)。

## 2. "Match" 表示 (問 2)

### 2.1 ウィジェット (観測)
- Simple パネル (setupUi `FUN_000dfec8`、Ui は this+0x68): simpleQualityMatchGroupBox (Ui[1])、simpleQualityLabel (Ui[4] = +0x88)、
  simpleMatchCaptionLabel (Ui[6] = +0x98)、simpleMatchLabel (Ui[7] = +0xa0)。retranslate `FUN_000e07dc` が見出し "Match" (0xe0990) と
  ツールチップ (0xe09f4) を設定。
- Advanced パネル (setupUi `FUN_000e84d8`、Ui は this+0x68): advancedQualityLabel (+0x90)、advancedMatchCaptionLabel (Ui[7] = +0xa0)、
  advancedMatchLabel (Ui[8] = +0xa8)。retranslate `FUN_000ea530` (0xea730・0xea794)。

### 2.2 非表示 (観測)
- Device Noise Profile パネル (`FUN_000dec00`、x24 = this+0xd678) の Simple 側、0xdf5f0〜0xdf630:
  `simple[+0x98]->setVisible(false)`、`simple[+0xa0]->setVisible(false)`、続けて this+0xd698 ← simpleMatchLabel、this+0xd6a0 ← simpleQualityLabel を保存。
- Advanced 側 (`FUN_000e0b18`) 0xe1154〜0xe11d8: advancedMatchCaptionLabel に `setText(" ")` (0x809dc0)、advancedMatchLabel を `setVisible(false)`、
  this+0xd698 ← advancedMatchLabel、this+0xd6a0 ← advancedQualityLabel。
- RS も同じ形 (0xde4f8〜0xde524、0xe00f0〜0xe0104)。抜粋 `notes/disasm/demo_match_label_hide.txt`・`rs_match_label_hide.txt`。

### 2.3 値が書かれないこと (観測)
- this+0xd698 (Match の値のラベル) を読むのは配色関数 `FUN_000db83c` (0xdb858、+0x30/+0x40 に色を設定) だけ。+0xd678/+0xd690 を基底とする
  アクセス (0xd690+8 = 0xd698 を含む) も全数確認し、setText に渡す箇所はない。オブジェクト名 "simpleMatchLabel"・"advancedMatchLabel" を参照するのは setupUi だけ (findChild 経由もない)。
- パネルの更新関数 `FUN_000dc2a8` (Quality の関数 0x9bb3e0 の呼び出し元) は Quality (this+0xd6a0 → `0x16a78` で "n%") と表示切替 (+0xd6c8) だけで、Match には触れない
  (`notes/disasm/demo_panel_quality_update.txt`)。
- したがって Match の式・書式・再計算の契機はバイナリにない。画面でも Quality の下に Match の行はない (g_prepare_fullframe_q65.png)。
  `work/omo/coord/verify_qtui_layout/LAYOUT.md` の「Match "100%"」は setupUi の既定文字列で、実際には表示されない。

## 3. プロファイルフォルダと設定キー (問 3)

### 3.1 既定のパス (観測)
フォルダの大域オブジェクト (0x88 バイト、ポインタは 0x155b590) を `FUN_0015c860` が作り、0x15d480〜0x15d498 で 3 つの遅延初期化を呼ぶ。
- `FUN_0015e87c`: +0x18 = 文書フォルダ。環境変数 `APP_SANDBOX_CONTAINER_ID` (CFString 0x140f818、判定 `FUN_00984b5c`) が無ければ
  `QStandardPaths::writableLocation(1 = DocumentsLocation)`、有れば `getpwuid(getuid())->pw_dir + "/Documents"` (サンドボックスの外の本当のホーム)。
  +0x20 = writableLocation(9)、+0x28 = "/Library/Application Support/ABSoft/NeatVideo6-OFX"。(列挙子の値は Qt 5 の定義からの読み)
- `FUN_0015ec38`: +0x00 のバイトが 0 なら プロファイル = 文書 + "/Neat Video v6 OpenFX/Profiles"、プリセット = 文書 + "/Neat Video v6 OpenFX/Presets"。
  非 0 なら プロファイル = (+0x08) + "/Profiles"、プリセット = (+0x08) + "/Presets"。どちらも Generic = プロファイル + "/Generic Profiles"。結果は +0x68 / +0x78 / +0x70。
- +0x00 のバイトは `FUN_0015c860` の w23。`getenv("NV6OFX_PATH")` が有り、そのディレクトリが存在すると (0x15c9f4〜0x15cb34) 設定を `<NV6OFX_PATH>/Settings.ini` (INI 形式) にして w23 = 1 (0x15cc0c)。
  そのとき +0x08 は NV6OFX_PATH (`FUN_0015e560` が第 2 引数を使う)。それ以外の経路は QSettings("ABSoft", "NeatVideo6-OFX") で w23 = 0 (0x15d27c)。
  w23 = −1 にする経路 (0x15ce80) は旧版の設定 ("NeatVideo5-OFX,NeatVideo4-OFX,NeatVideo OFX"、0x15cd08) を引き継ぐループの入口にあり、
  ループの後 0x15d27c で 0 にしてから 0x15d2ec で格納するとみられる (分岐を全部は追っていない。推定)。
- RS の文字列は `NV6RS_PATH`、"/Neat Video v6 for Resolve/Profiles"、`com.absoft.NeatVideo6-RS` (Demo の "OpenFX" の部分が "for Resolve")。
  この機械の RS の設定 (`defaults read com.absoft.NeatVideo6-RS`) は `ProfileDir = "/Users/seitan/Documents/Neat Video v6 for Resolve/Profiles"` で、フォルダは空 (観測)。

### 3.2 設定キー (観測)
- `ProfileDir` (0x13110d6): 読み込み `FUN_00132ed8` 0x1337f4〜0x133818 (設定 → 設定構造体 +0x198)、書き込み `FUN_00134394` 0x135814。
  隣は `PresetDir` (+0x1a0)。ほかに `IndependentImProfPresDir` ("Use independent folders for profiles and presets")、`CurProfileOpenDir`・`CurProfileSaveDir` (直近の開く/保存先)、
  `DefaultNoiseProfile`・`StoreNModelImage` ("Save noise samples in profiles (*.dnp files)")。
- Preferences ダイアログ (`OptionsDialog`、metaobject 0x13c96f8、`notes/moc_demo_OptionsDialog.txt`): Folders の profileFolderGroupBox ("Folder used by Neat Video to store device noise profiles")、
  選択ダイアログの題名 "Select profile folder" (`FUN_001ad510`)、resetProfileDirButton (ツールチップ "Reset profile folder to default Neat Video profile folder")。
  スロット `resetProfileDirButtonClick` (1 番、0x1aeaf8) は大域オブジェクトの +0x68 (`FUN_0015f778`) を編集欄 ([dlg+0x380]) に設定する = 既定に戻すのは §3.1 の値 (観測)。

### 3.3 保存済みプロファイルのポップアップ (Auto Match ではない。観測)
- ProfilePopupButton ([パネル+0x128]、`FUN_000d8ec8` が設定): 拡張子 "dnp" (+0x90)、空のときの文言 "Neat Video could not find any profiles in the assigned Profile folder:\n %1\n\n…" (+0xd8)、
  "Profile Folder" (+0xe0)、ツールチップ "Show saved noise profiles"。
- 表示 `FUN_0002342c`: 名前フィルタ "*" + "." + 拡張子 = "*.dnp" を作り、`FUN_000279a0` → `FUN_00026954` でメニューを組む。メニューが空なら
  文言の %1 に `QDir::toNativeSeparators` したフォルダを入れて表示 (0x2342c 内)。
- `FUN_00026954`: `QDir(path)`、`setFilter(0x6402)` (= Files | AllDirs | NoDot | NoDotDot)、名前フィルタ、`setSorting(0x14)` (= Name | DirsFirst | IgnoreCase。条件付きで −1 = 既定)、
  `entryInfoList`。ファイルは completeBaseName を表示名にした項目、ディレクトリは下位メニューにして同じ関数を再帰で呼ぶ (空の下位フォルダの扱いは `FUN_00026858` で別項目)。
  列挙子の値は Qt 5 の QDir::Filter / SortFlag の定義からの読み (推定を含む)。

## 4. 保護・難読化 (問 4。手を加えていない)
- Auto Match の本体 (0xd7430) とボタン・Match の非表示の命令列には、難読化・改ざん検知様のコードはない (観測)。
- 近くにあるもの (記録のみ):
  - `FUN_0002342c` (保存済みプロファイルのポップアップ) の入口と空判定の分岐に、大域語のビット混合 → 0x141a3d8 への書き込み、
    `(w ^ w>>1) & 0x41040011` の型の判定 → ハッシュ化したアドレスへの書き込み (AGENTS.md §6.5/QUALITY.md の記帳型と同じ形)。
  - `FUN_000f4470` (ビューアのツールバーの表示切替) の 0xf4518〜0xf4570 に、0x141bfa8/0x141bfb8 経由の語の ror/csel と書き戻し。
  - どちらも本件の機能 (Auto Match・Match) の値には関わらない (推定)。

## 5. C++ 移植で必要なもの・ハーネス (問 5)
- **ビット一致させる対象がない**: Auto Match の照合も Match の数値も本家に存在しないので、libnvcore に追加すべき数値関数はない。
- 本家の挙動として再現できるのは UI の事実だけ: (a) Auto Match ボタンなし、(b) Match 行なし、(c) Profile フォルダの設定と既定値・リセット、(d) 保存済みプロファイルの一覧 (再帰・*.dnp・フォルダ先頭の大小無視の名前順・空のときの文言)。
- 既存の libnvcore で関係しうるもの (参考): `nv_aft::similarity` (FUN_009b6338、タイルとプロファイルのモデルの類似度) と `nvc_profile_build` / `nvc_flat_area`。
  再現側の独自 Auto Match を作るならこれらで組めるが、本家の定義ではない。
- `nvh`: 新しいサブコマンドは提案しない。autoMatchProfile は `ret` で呼ぶ意味がなく、フォルダ列挙・ポップアップは Qt のウィジェットと QApplication を要する葉ではない関数なので、
  AGENTS.md §5 の基準 (外を呼ばない葉) に当てはまらない。静的事実の再確認は `notes/tools/refs.py`・`moc.py` と逆アセンブルで足りる。

## 6. 未解決
1. Profile メニューから Auto Match / Smart Profile / Calibration Target を外す仕組み (アクションは作られる。メニューの組み立てはアクション保持体のポインタ表 (0x7caa0 付近の `FUN_00082d14` の連続) 以降とみられるが未読)。画面では出ていない。
2. ショートカット Ctrl+M の動作が非表示のアクション経由で有効かどうか (有効でもスロットが `ret` なので結果は何も起きないはず。推定)。
3. フォルダの大域オブジェクトの w23 = −1 の経路 (0x15ce80) が 0x15d2ec まで −1 のまま届く分岐があるか (あれば +0x00 ≠ 0 で "/Profiles" 側になる)。
4. `FUN_00026954` の「条件付きで並べ替えなし」の条件 (`param_7 != 0 && param_4`) がポップアップで成り立つか。
5. 本家の他の版 (Neat Video のスタンドアロン・Premiere 版) で Auto Match が生きているかは未確認 (Premiere Demo の dmg は未展開)。

## 7. 抜粋・道具
- `notes/disasm/`: metacall と空のスロット (Demo/RS)、ProfilePageToolbar のコンストラクタの非表示 (Demo/RS)、Match ラベルの非表示と唯一の読み手、
  パネルの Quality 更新、フォルダの初期化・リセット・ProfileDir の読み込み、別クラスの表示切替 (0xf4470)
- `notes/dec/`: Ghidra のデコンパイル (接続 0xed0f4、ツールバー setupUi 0xeb7fc、ポップアップの文言 0xd8ec8、パネル setupUi・retranslate、フォルダ 0x15e560/0x15e87c/0x15ec38/0x984b5c、ポップアップ 0x2342c/0x279a0/0x26954)
- `notes/moc_*.txt`: moc のメソッド表 (ProfilePage Demo/RS、OptionsDialog)
- `notes/tools/`: `refs.py` (adrp+add/ldr の参照の全数探索)、`moc.py` (QByteArrayData の文字列表から QMetaObject とメソッド表)、`dec.sh`・`DecompCreate.java` (scratchpad の Ghidra プロジェクトでデコンパイル)

## 8. qtui への提案 (段階 2 用の案)
本家に合わせるなら「足す」より「外す」が中心になる。
1. Prepare のツールバー: Auto Match ボタンは置かない (本家は非表示)。
2. Device Noise Profile パネル: Match 行は置かない (本家は非表示)。Profile Viewer の再現側の "Match:" 行 (領域選択の状態) は本家の Match とは別物なので、
   名前を "Area status:" などに変えて混同を避ける。
3. Profile メニュー: 現行の "Auto Match (n/a)" (無効) は本家の画面にないので外す。残すならツールチップを「本家の OFX/Resolve 版では未実装 (空のスロット) で非表示」とする。
4. Preferences > Folders > Profile folder (本家にあり、実装の価値あり): 設定キー (共有設定ファイルの命名規則に合わせる。本家名は `ProfileDir`)、
   既定 `<QStandardPaths::DocumentsLocation>/<製品名>/Profiles`、"…" で "Select profile folder"、リセットのボタン (ツールチップは本家の文言)。
5. 保存済みプロファイルの一覧 (プロファイル名の横の▼。"Show saved noise profiles"): Profile folder を再帰で列挙、ファイルは再現側の拡張子 (.json。.dnp は読めない)、
   フォルダ先頭・大小無視の名前順、下位フォルダは下位メニュー、空なら本家の文言で案内。
6. 再現側の独自 Auto Match を作る場合 (本家にない機能として明示): 現在フレームの自動領域でモデルを作り (`nvc_flat_area` + `nvc_profile_build`)、
   フォルダの各プロファイルのモデルと `nv_aft::similarity` で比べて最大を読み込む、等。本家との照合はできない。

検証案: qtui の自己試験に (a) ツールバーに autoMatchButton がない、(b) パネルに Match 行がない、(c) Profile folder の既定値・リセット・保存の往復、
(d) 一時フォルダ (入れ子・大小混在の名前・拡張子違い・空フォルダ) で一覧の順序と項目、空のときの文言の完全一致、を加える。
本件の静的事実 (§1〜§3 の命令列) は独立検証者が `notes/tools/refs.py`・`moc.py` と逆アセンブルで再確認する。
