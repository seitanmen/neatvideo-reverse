# N4: macOS 用 OFX プラグイン (MeatVideo.ofx)

作成: 2026-10-31 (統括)。目標: **DaVinci Resolve が読み込む本家と同じ形の
Mac 用 OFX プラグイン**。素材はファイルからではなく、**ホスト (Resolve) から
渡される画像をそのまま処理する** (ファイル読み込みは行わない)。

## 既知の情報 (解析済み)

- 本家の OFX エクスポートは 3 関数のみ: `OfxGetPlugin` / `OfxGetNumberOfPlugins` (=1) /
  `FoundryOfxAllowPluginUnload` (Agents.md §3)
- 本家バンドル構成: `NeatVideo6.ofx.bundle/Contents/{MacOS/NeatVideo6.ofx, Info.plist, ...}`
  (実物が `reference/genuine/NeatVideo6.ofx.bundle/` にある。**構成の雛形として使う**)
- 本家バイナリのシグネチャ: `com.absoft.NeatVideo6-OFX` (我々は
  `com.meatvideo.MeatVideo-OFX` にする)
- OFX API: バージョン 1.x の Image Effect (SupportsTiles, FieldProcessing なし=
  まずプログレッシブのみ)。docs/INTERLACE_PROGRESSIVE.md のとおり、
  インターレースは「非対応」とホストに正直に報告する段階から
- 処理: libnvcore (libnvcore.dylib の C ABI / NVC_ABI_VERSION = 3) を直接リンク

## N4a: 最小 OFX プラグイン (まず読み込ませる)

1. `ofx/` ディレクトリに CMake ターゲット `MeatVideo_ofx` (dylib) を追加:
   - エクスポート: `OfxGetNumberOfPlugins`, `OfxGetPlugin`, `FoundryOfxAllowPluginUnload`
     (シンボル名は C リンケージ。本家の 3 関数と同じ名前)
   - OfxPlugin 構造体: API version 1, pluginApi "OfxImageEffect", 独自 pluginId
     (`com.meatvideo.denoise`)
2. バンドル化スクリプト `ofx/build_ofx.sh`:
   `MeatVideo.ofx.bundle/Contents/{MacOS/MeatVideo.ofx, Info.plist, Resources/}` を作り、
   libnvcore.dylib を Contents/Frameworks/ に同梱 (rpath で参照)
3. **読み込みテスト** (ホストなしでもできる):
   - Resolve に無い環境での最初の検査: `OfxGetPlugin` を dlopen して呼ぶ
     ミニホスト (`ofx/tools/minihost.cpp`) を作り、構造体の妥当性を確認
   - ユーザーが Resolve の OFX フォルダ
     (`/Library/OFX/Plugins/MeatVideo.ofx.bundle`) にコピー → Resolve 起動 →
     Effect リストに表示されるかを確認 (ユーザー操作)
4. 合格: Resolve がプラグインを認識し、クラッシュしない

## N4b: レンダーの実装 (ホストから渡された画像の処理)

1. OFX アクションの実装:
   - `OfxImageEffectActionDescribe` (クリップ Source/Output、プロパティ)
   - `OfxImageEffectActionCreateInstance` / `DestroyInstance`
     (インスタンスごとに libnvcore の状態を保持)
   - `OfxImageEffectActionRender`: ソース画像のプロパティ
     (`OfxImageEffectPropSourceClipData` / row pointers) から画素を取得し、
     libnvcore の処理 ( temporal + spatial、パラメータは OFX パラメータから) を実行、
     出力画像へ書き込む
   - `OfxImageEffectActionGetRegionOfDefinition` / `GetFramesNeeded`
     (Temporal Radius に応じたフレーム要求)
2. パラメータ: 第 1 段は最小構成
   (Temporal Radius, Temporal Threshold, Luma/Cb/Cr Noise Level,
   Spatial Amount 系 = Python 版 UI の既定値と同じ既定)
   パラメータ → `v5_params` への写像を `ofx/params.cpp` に集約
3. プロファイル: **OFX ではファイルから作れない** ので、
   (a) Load Profile アクション/パラメータで JSON ファイルを読む
   (b) インスタンスに profile blob (0xc300 相当 or v5 dict 相当) を渡す
   仕様は N2a の `nvc_profile_build_cancel` / G8 の `nv_prof5` C ABI を使う
   (UI と同じ計算経路 → ビット一致が保証される)
4. **素材はホストから**: すべての画素はホストが渡す画像バッファから取得する。
   ファイル・stdin パイプ・ffmpeg は一切使わない
   (デノイザー本体に ffmpeg は不要、という OFX の利点をそのまま実現)
5. 検証: ミニホストを拡張して Render アクションを呼び、
   同じ入力フレーム列に対する出力が
   (i) N2a のブリッジ経路 (backend.process_frame / process_frame_v5) と
   ビット一致することを確認 (レンダー 1 フレーム = temporal+spatial 1 回分)
   (ii) ユーザーが Resolve で実際に処理をかけて目視確認

## N4c: 実用化

- パラメータの追加 (イコライザ = R/G/B 9 ビン、Noise Level Check 相当の
  表示補助は OFX では別 persist やダイアログになるので後回しでよい)
- プロファイルの UDP/エクスポート (UI で作った JSON プロファイルを
  Resolve のインスタンスに渡す仕組み: Load Profile パラメータのパス指定)
- メタデータ: プラグイン名 "Meat Video Denoise"、バージョン、About (Developer: kawata)
- Windows 版 (.dll) は W5

## N4d: 本家どおりの 2 ボタン + ウィンドウ (2026-11-01 ユーザー決定)

ユーザーの指示: 「OFX は本家と同じで 2 つのモードを直接呼び出すボタンでウィンドウを呼び出します」。
決定: ウィンドウは**別プロセス** (MeatVideoQt) で開く / 現在の 18 スライダーと Rebuild は**インスペクタから隠す** (本家どおり)。

### 本家の観測 (静的、`reference/genuine/analysis/prof_0xa22c.c` の Describe)
- ページ "Controls" に次だけを置く:
  - PushButton `Prepare Profile...` (ラベル "Prepare Noise Profile"、ヒント "Prepare a noise profile to enable the filters in this instance of the effect")
  - PushButton `Adjust Filters...` (ラベル "Adjust Filter Settings"、ヒント "Adjust filter settings to customize the processing applied by this instance of the effect"、**OfxParamPropEnabled 0 で定義** = プロファイルができるまで無効)
  - Custom `ClipParams` (Secret 1、既定 ""、CacheInvalidation OfxParamInvalidateAll、Animates 0) = 設定とプロファイルの保存先
  - Integer `ParamsHash1..3` (Secret、既定 0、InvalidateAll、Animates 0)
- ボタンは本家のウィンドウ (Qt、プロセス内) を Prepare / Adjust のページで開く。FilmLight ホストだけ MainThreadAction を付ける

### 実装
1. Describe を上の構成にする (名前・ラベル・ヒントは本家と同じ文字列)。18 パラメータと Rebuild は定義しない
   (v5 への写像 `params.cpp` は ClipParams の中身からの写像として残してよい)。
2. ClipParams: qtui が保存できる形 (プロファイル JSON + 設定) を 1 つの文字列にしたもの。レンダーに必要な情報 (プロファイル全体、AFT 後の係数、全設定)
   をすべて含め、**レンダー中にプロファイルを作らない**。空ならレンダーは入力をそのまま出す (+ 永続メッセージでプロファイル作成を促す)。
   ClipParams にプロファイルがあれば Adjust Filters を有効にする (InstanceChanged / CreateInstance で更新)。
3. ボタン (InstanceChanged, kOfxChangeUserEdited):
   - 現在時刻の前後のフレーム (例: 本家のサムネイル数 11 と時間窓をまかなう範囲、クリップ範囲で切る) を **clipGetImage でホストから取得**
   - ヘルパー `MeatVideoQt --ofx-session` (新モード。既存の `--ofx` は変えない) を posix_spawn で起動し、パイプで
     ヘッダ (ページ prepare/adjust、W/H、各フレームの時刻、現在のフレーム、現在の ClipParams) とフレーム (float32、--depth 32 と同じ変換) を渡す
   - ウィンドウを閉じるまで待つ。Apply なら新しい状態を stdout で受け取り ClipParams に書く (+ ParamsHash を更新)。Cancel なら何も変えない
   - ヘルパーの場所: OFX バンドル内 (`Contents/Helpers/MeatVideoQt.app` 等、Qt を同梱して自己完結、アドホック署名)。試験用に環境変数で差し替え可
4. qtui の `--ofx-session`: 受け取ったフレームでウィンドウを指定ページで開く。Apply で状態を出力。ユーザー設定ファイル (~/.config/NeatRepro/nv_ui_qt.ini) を試験で書き換えないこと。

### 合格
- ミニホストでボタンを押す → ヘルパーを offscreen + 試験ドライバで動かし (Auto Profile → Apply 等) → ClipParams 受領 → Render が、同じ状態・同じフレームでの
  qtui / Python ブリッジ経路の処理と**ビット一致**。Cancel で ClipParams 不変。プロファイルなしは素通し。Adjust の有効/無効の切り替え。
- fsguard: **Render の経路ではファイル・プロセス呼び出し 0**。ボタンの経路はヘルパーの posix_spawn だけ (プラグインはファイルを開かない)。
- 既存: N4b の照合 (状態を ClipParams で与える形に置き換え)、qtui selftest、make -C native verify、警告 0。
- 注意: INT-1 (int1 worktree) も qtui の mainwindow.{h,cpp}・selftest.cpp を変える。qtui の変更は最小にし、触ったファイルを REPORT に列挙する (マージ順はユーザーが管理)。
- コミットしない。REPORT: `work/omo/wp_native_ofx/N4d/REPORT.md`。/Library/OFX/Plugins と Resolve には触れない。

### N4d 追加: 既定プロファイルの自動読み込み (2026-11-01 ユーザー指示)
- 本家の文言 (バイナリの文字列): "Default Profile is used when a new instance of the effect is created or the current profile is reset."、
  "Selected default profile cannot be loaded."、"No default profile will load in new instances of the effect or when the current profile is reset."、
  内部名 `loadDefaultProfileOnSetSrcImage`・`saveAsDefaultProfile`・`assignAsDefaultProfile`・`resetDefaultProfile`
- qtui には既に Assign As Default Profile / Reset Default Profile / Change Default Profile… がある (mainwindow.cpp 659〜661、設定は nv_ui_qt.ini 共有)。これを使う
- 動作:
  1. 新しいインスタンス (ClipParams が空) で既定プロファイルが設定されていれば、それを ClipParams に入れる。プラグインはファイルを読まないので、
     ヘルパーを**画面なしの短いモード** (例 `MeatVideoQt --ofx-default`) で起動し、既定プロファイル (+ 既定の設定) を stdout で受け取る。
     Render の中では行わない (CreateInstance か、最初の InstanceChanged / BeginInstanceChanged など、レンダー外で 1 回)。読めなければ空のまま + メッセージ
     "Selected default profile cannot be loaded." 相当。既定なしなら何もしない (素通し)
  2. ウィンドウを開いたとき ClipParams が空なら既定プロファイルを読み込んだ状態で開く。ウィンドウでの Reset Profile は既定プロファイルに戻す
  3. 既存のインスタンスの ClipParams は既定を変えても変えない (本家の文言どおり新規とリセットのときだけ)
- 合格: 既定あり/なし/壊れたファイルの 3 通りを試験。既定ありの新規インスタンスのレンダーが、同じプロファイルを qtui で読み込んだ処理とビット一致。
  Render 経路のファイル・プロセス呼び出し 0 は変わらず。試験はユーザーの nv_ui_qt.ini を変えない (隔離した設定で)
- 本家の "Automatically find and load a device noise profile that matches the input image" (プロファイル庫から合うものを探す機能) は今回の対象外

### N4d 追加 2: 既定プリセットも (2026-11-01 ユーザー指示「どちらも入れてください」)
- 本家の文言: "Default Preset is used when a new instance of the effect is created or the current filter settings are reset."、
  "Selected default preset cannot be loaded."、Filter > Change Default Preset…
- 既定プロファイルと同じ仕組みで、新規インスタンス (ClipParams 空) とウィンドウでの設定リセット時に既定プリセット (フィルタ設定) を入れる。
  `--ofx-default` は既定プロファイルと既定プリセットの両方を返す。試験も同じ 3 通り (あり/なし/壊れたファイル)

## N4e: Auto Match (プロファイル庫から入力画像に合うプロファイルを探して読み込む) (2026-11-01 ユーザー指示)
- 本家の文言: ボタン "Auto Match" (`autoMatchButton`、`autoMatchProfile()`)、ヒント "Automatically find and load a device noise profile that matches the input image"、
  Profile フォルダ (Tools > Preferences > Folders > Profile folder、"Neat Video could not find any profiles in the assigned Profile folder:")、
  パネルの "Match" 欄 ("Profile-image match shows how close are device modes of current noise profile and input image"、`prof_0xea530.c` 81/96 行)
- 第 1 段 (解析): 本家の照合の式を静的に解読する (`work/omo/wp_automatch/DECODE.md`)。本家の実行はガード付き nvh 以外では行わない
- 第 1 段の結果 (2026-11-01、独立検証 APPROVE、`work/omo/wp_automatch/DECODE.md`・`VERDICT.md`): **本家の Auto Match は実装が空** (スロット 33 番 → `ret`、Demo 0xd7430 / RS 0xd62a4)。
  ボタンはツールバーの構築時に、メニュー項目は起動後に、Match 欄はパネル構築時に非表示にされる。照合の式も Match の計算式もバイナリにない。
  本家で実際に動くのは Profile フォルダ (設定キー `ProfileDir`、既定 `<書類>/Neat Video v6 OpenFX/Profiles`、選択・既定に戻す) と、
  保存済みプロファイルの一覧 (▼ "Show saved noise profiles": `*.dnp` を再帰、フォルダが先・大文字小文字を無視した名前順、空なら "Neat Video could not find any profiles in the assigned Profile folder:")
- ユーザー決定 (2026-11-01): **本家どおりの部分に加えて、再現側独自の Auto Match も作る**
- 第 2 段 (実装、N4d のマージ後に着手):
  1. 本家どおり: Preferences > Folders > Profile folder (既定・選択・既定に戻す)、保存済みプロファイル一覧 (▼。再現側は .json。並び順と空の文言は本家どおり)
  2. 独自の Auto Match: Prepare ページのボタンと Profile メニューの項目。Profile フォルダ内のプロファイルを入力画像 (現在のフレーム) に対して評価し、最も近いものを読み込む。
     評価は移植済みの部品 (FindFlatArea・BuildNoiseModel・AFT の類似度 `nv_aft::similarity` = FUN_009b6338 など) で組み、式は REPORT に明記する。本家の定義ではないことを UI のツールチップ等に "repro" と明記
  3. Match 欄: 独自の Auto Match の評価値を表示 (同じく "repro" と明記)。Profile Viewer の再現側の "Match:" 行は本家の Match と別物なので改名
  4. 「Auto Match (n/a)」のメニュー表示は上の実装に置き換える
  5. 検証: 自己試験に、フォルダ設定の往復、一時フォルダでの一覧の並び順と空の文言、Auto Match が既知の正解 (同じ素材から作ったプロファイルを混ぜたフォルダ) を選ぶこと、評価値の C++ と Python 参照のビット一致 (参照を作る場合)

### N4d Round 4 (2026-11-01): ウィンドウ表示中の「応答なし」の修正 — Resolve の実機確認は保留
- 発端: 実機の Resolve で、ウィンドウの表示中に CleanMyMac が「応答なし」と通知した (Apply で閉じた後は正常)。
- 原因: プラグインが主スレッドで、ヘルパーの返事を読み込みで待っていた。
- 修正: ofx/src/modal_wait.{h,cpp}。待ちの間、ホストのイベントを独自のモード MeatVideoOFXModal で取り、入力は捨てる。
- 済み: 統括の再実行は合格。独立検証 Round 4 は依頼済み。
- 保留: Resolve の実機確認 (ユーザーの指示で後回し)。項目は N4d/REPORT.md の「保留」節。
- N4e は N4d のマージ後に始める (変更なし)。

## 規約・注意

- libnvcore の C ABI の変更は NVC_ABI_VERSION を上げ、nv_native.py (参照実装) の
  版数も揃える (G1/G8 と同じ手順)
- 合格判定はビット一致 (ミニホスト vs ブリッジ経路)
- OFX ヘッダーは公開の ofxh (OpenFX 標準ヘッダ) を使う
  (`ofx/vendor/` に同梱、出典を記録)
- Resolve での実機確認はユーザーが行う (REPORT に手順を書く)
- **コミットしない**。REPORT: `sandbox/omo/wp_native_ofx/REPORT.md`
  (sandbox は既に work/ に移動済み → `work/omo/wp_native_ofx/REPORT.md`)
- スナップショット・MIGRATION.md・AGENTS.md は統括が更新

## 貼り付け用プロンプト

```
.claude/tasks/native_n4_ofx.md の「N4a」「N4b」を実行してください。
本家と同じ 3 関数エクスポートの .ofx バンドルを作り、
ミニホストで Render アクションのビット一致 (libnvcore 経路 vs ブリッジ経路) を
確認してください。素材はホストから渡された画像のみを使い、
ファイル読み込み・ffmpeg は一切使わないこと。完了したら
work/omo/wp_native_ofx/REPORT.md に書いてください。コミットはしないでください。
```
