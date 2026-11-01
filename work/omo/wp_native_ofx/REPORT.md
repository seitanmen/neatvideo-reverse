# N4a/N4b REPORT — macOS 用 OFX プラグイン MeatVideo.ofx (作業者 n4-ofx)

転記: 統括 (2026-10-31)。作業者の最終報告をそのまま転記した。経過は `PROGRESS.md`、ログは `logs/`。ユーザーの指定 (素材はホストから渡された画像のみ、ファイル読み込み・ffmpeg を使わない) に従い、タスク N4b-3 (a) の JSON の Load Profile は行っていない。統括の再確認と独立検証は末尾に追記する。

N4a/N4b 完了: MeatVideo.ofx.bundle をビルドし、ミニホストで 48 比較とも Python のブリッジ経路・純 Python 経路とビット一致 (0/66,248,704 語)。C ABI は変更なし。コミットはしていない。

worktree: /Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx (変更は .gitignore と新規の ofx/ だけ。経過は work/omo/wp_native_ofx/PROGRESS.md、ログは同じ場所の logs/*.txt)

## 作ったファイル (ofx/ 以下)
- src/plugin.cpp: OFX のアクション。Load / Describe / DescribeInContext / CreateInstance / DestroyInstance / InstanceChanged / GetRegionOfDefinition / GetRegionsOfInterest / GetFramesNeeded / Render
- src/params.{h,cpp}: 18 個のパラメータと、v5 ブロックへの写像 (state.backend_params と同じ式)
- src/core.{h,cpp}: 画素の変換、プロファイル + AFT、temporal + spatial + mix。すべて C ABI を直接呼ぶ
- src/ref_floor.inc: data/ref_floor.npy の 51 語を埋め込んだもの
- CMakeLists.txt (ターゲット MeatVideo_ofx / minihost / fsguard)、exports.txt、Info.plist.in、build_ofx.sh
- tools/minihost.cpp: 試験用ホスト
- tools/fsguard.c: DYLD_INSERT_LIBRARIES で注入するファイル/プロセス呼び出しの監視
- tools/verify_ofx.py: 照合スクリプト
- vendor/openfx/: BSD-3-Clause のヘッダ 8 本と LICENSE・SOURCE.md (出典と SHA256)。ライセンス表記のない Resolve 拡張ヘッダは写していない
- .gitignore: ofx/ のソースを許可する行を追加

## 設計
- 画素: ホストの RGBA float32 を上の行から並べた G,B,R の平面 (gbrpf32le) にし、面ごとに nvc_depth_sanitize で NaN/±inf を記録してから nvc_depth_to_internal (full range YCbCr 4:4:4、0..255) に渡す。出力は nvc_depth_from_internal で非有限値を戻し、RGB にして α は入力のまま。--depth 32 の nv_depth の経路と同じで、Python 側の参照も nv_depth.to_internal / from_internal(pixfmt("gbrpf32le")) を使う。rowBytes の符号・パディング・bounds の原点に対応。
- 申告: Filter / General、RGBA float32 のみ、タイルなし、多解像度なし、TemporalClipAccess 1、InstanceSafe、FieldExtraction Both、FieldRenderTwiceAlways 0。インターレースは、クリップの FieldOrder が Upper/Lower、field 単位の render、画像の Field のどれでも kOfxStatErrUnsupported を返し、永続メッセージを出す。64px 未満のフレームと 8/16 ビットも拒否する。
- プロファイル: レンダー中に Profile Frame (クリップ先頭からのオフセット、既定 0) と続く 2 フレームをホストから取得し、nvc_build_profile_v5 (auto、FindFlatArea で探索、levels 5) で作る。Auto Fine-Tune (既定 ON、UI の prefs と同じ) は backend._run_aft と同じ手順 (nvc_profile_build_cancel → nvc_aft → nvc_aft_combine)。インスタンス内にキャッシュし、プロファイル関係のパラメータか Rebuild Profile ボタンが変われば作り直す。JSON などのファイルは読まない。
- パラメータ (UI と同じ単位・既定値): Temporal Enable / Radius 2 / Amount 100 / Threshold 4.0、Luminance・Cb・Cr Noise Level 100、Overall Noise Level 100、Spatial Enable / Luminance Amount 70 / Chroma Amount 100 (Cb と Cr 共通なので spatial は 1 回の呼び出しで済む) / Spatial Strength 2.0、Mix + Enable、Profile Frame / Mode (auto/spatial/temporal) / Auto Fine-Tune / Color Matrix (709/601/2020)、Rebuild Profile。
- FoundryOfxAllowPluginUnload は 0 を返す (観測: 本家の arm64 スライスを otool で静的に読むと mov w0,#0; ret)。

## 検証 (観測)
- minihost --check: エクスポートは 3 つだけ (nm -gU)、プラグイン数 1、OfxImageEffect / apiVersion 1 / com.meatvideo.denoise 1.0。各アクションと申告内容は CHECK OK。
- 照合: python3 ofx/tools/verify_ofx.py --pure --mutate
  - 5 場面 24 レンダー × 参照 2 種 (NV_NATIVE=1 のブリッジ / NV_NATIVE=0 の Python) = 48 比較で 0/66,248,704 語が不一致。uint32 で比べ、NaN のビット列も対象。
  - 含めた条件: 端のフレーム (t=0,1,末尾)、パラメータを途中で変える 6 段 (プロファイルの作り直し、行列 3 種、モード 3 種、AFT なし、Mix、時間・空間のオフ、Profile Frame がクリップ外)、半径 1〜5、HDR・負値・NaN/±inf、1080p、クリップ先頭 t=100、原点 (10,20)、負の rowBytes。
  - 1080p のプロファイル領域は [0,832,128] で、§6.5 の n=312 の既知の結果と一致した。
  - 宣言していないフレームの取得 0、画像はすべて解放された。
- 空振りの確認: プラグイン側の Threshold を 4.0001 にすると 5/5 レンダーが不一致。参照側の入力 1 語を 1 ulp 変えると 2/5 が不一致。
- 途中で直した点: 最初は minihost が native/libnvcore をリンクしていたため、バンドル内の Frameworks の libnvcore が使われていなかった。リンクを外して照合をやり直し、DYLD_PRINT_LIBRARIES でバンドル内のものが読まれることを確認した。build_ofx.sh の install_name_tool は Anaconda のものだったので xcrun に変更した。
- ファイル・ffmpeg に触れていない証拠:
  - 実行時: fsguard は open/openat/fopen/freopen/fdopen/opendir/stat/lstat/fstatat/access/faccessat/mkdir/unlink/rename/creat/popen/system/fork/exec*/posix_spawn*/dlopen と、stdin への read/pread を監視している。全場面で、プラグインのセッション中に記録されたのはホスト自身の dlopen(バンドル) の 1 件だけ。陽性対照は 5 件を記録した。
  - 静的: MeatVideo.ofx の import にファイル系・プロセス系の関数はない。libnvcore は fopen/fread を import しているが、使うのは nvc_reference_floor(path≠NULL) だけで、プラグインはこれを import していない。埋め込んだ基準値は nvc_reference_floor(NULL) の計算値と 51/51 一致。
- 既存の検証:
  - make -C native verify: exit 0、11 本すべて一致。LZMA の本家 blob 比較は、この worktree にキャッシュがないためスキップされた (従来どおり)。
  - qtui の全ターゲットのビルド: 警告 0。selftest は 117 passed / 0 failed。
    - 初回は qtui_selftest だけをビルドしていたため、qtui_ofx_drv が無く OFX の 3 項目が失敗した。全ターゲットをビルドして解消。
    - ~/.config/NeatRepro/nv_ui_qt.ini は実行前後で変化なし。
  - クリーンビルドの警告 0。
  - C ABI は無変更 (NVC_ABI_VERSION 3) なので verify_bridge は実行していない。

## Resolve での確認手順 (ユーザーが行う)
1. 必要なら作り直す: ofx/build_ofx.sh。バンドルは ofx/build/MeatVideo.ofx.bundle。
2. sudo cp -R ofx/build/MeatVideo.ofx.bundle /Library/OFX/Plugins/
3. Resolve を起動 (起動中なら再起動) し、Color ページの OpenFX ライブラリの "Meat Video" グループに "Meat Video Denoise" が出ることを確認する。出ない場合は ~/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml を削除して再起動すると出る可能性がある (推測)。
4. プログレッシブ素材 (例: test_v02.mp4) のノードに適用する。最初のフレームはプロファイル作成のため 1080p で数秒かかる見込み。以降は 1 フレームあたり 1 秒前後の見込み。ノイズが減ること、クラッシュしないこと、パラメータを変えると結果が変わることを確認する。
5. インターレース素材ではエラー表示になることを確認する (任意)。

## 未対応の点
- 8/16 ビット、RGB のみ (α なし) の画像、タイル、プロキシ解像度、インターレース (フィールド処理)、GPU。
- イコライザ・帯域ごとの設定・Temporal Noise Level などの詳細パラメータ (N4c)。
- プロファイル作成中のキャンセル。abort は各段の間でだけ確認している。
- フレームをレンダーをまたいでキャッシュしていない (毎回 2R+1 フレームを取得する)。
- ネイティブの AFT が使えない領域では AFT を省き、警告メッセージを出す (Python の UI は Python 版の AFT にフォールバックする)。
- パラメータの丸め: OFX の params.cpp は四捨五入、Python と qtui は銀行丸めで、整数以外の値を入れたときだけ差が出る (統括の訂正 (検証者の I-3・R2-I-4): ホストは整数のパラメータを int で渡すので、実際には起こらない)。
- Profile Frame の基準は、ホストのクリップの FrameRange の先頭。Resolve の時刻の付け方は未確認。
- x86_64 版はない。最低 macOS は 26.0 (libnvcore に合わせた)。
- .gitignore に !work/ がないため、work/omo/wp_native_ofx/ の新規ファイルは無視される。

---
## 統括の再確認 (2026-10-31)
- `ofx/build_ofx.sh`: rc 0、警告 0。
- `nm -gU` で見たエクスポートは `_OfxGetPlugin`・`_OfxGetNumberOfPlugins`・`_FoundryOfxAllowPluginUnload` の 3 つだけ。
- `python3 ofx/tools/verify_ofx.py --mutate` (ブリッジ経路、`--pure` は付けていない): 24 比較で 0/33,124,352 語が不一致、ALL MATCH。
  - 1080p の t=0/1 は、プロファイル領域 [0,832,128] で一致。
  - fsguard が記録したのは、ホスト自身によるバンドルの dlopen の 1 件だけ。
  - インターレースのクリップは status 5 (Unsupported) とメッセージで拒否された。
  - 変異 1 (Threshold 4.0001) では 5/5 のレンダーが不一致、変異 2 (1 ulp) では 2/5 が不一致。照合は空振りしていない。
- ログは scratchpad の `n4_lead/`。

## 独立検証 Round 1 (verify-n4): **APPROVE** (M 0・L 4・I 9)
判定書は `VERDICT.md`。補助のスクリプトとログは `verifier/`。

**観測:**
- **ビルド:** クリーンビルドで警告 0。エクスポートは 3 つだけで、rpath は 1 本。Info.plist は本家と同じ形。
- **ファイルを使わないこと:**
  - 静的: fopen の呼び出し元は `nvc_reference_floor` だけで、プラグインはこれを import していない。`ref_floor.inc` は `ref_floor.npy` と 51/51 で一致。
  - 動的: fsguard に加えて、sandbox-exec で /Users・/tmp の読み取りと書き込み・fork・exec・network を禁止した状態でも完走し、出力は照合済みのものとバイト同一だった。陽性対照は kill された。
- **ビット一致:** `verify_ofx --pure --mutate` は 48 比較で 0/66,248,704 語の不一致。追加の 6 場面 20 レンダー × 参照 2 種も 40/40 で一致した。追加したのは、奇数サイズ・負の原点・1 フレームのクリップ・一定画像・HDR ×40 など。
- **窓の縮小と abort の注入:** 正しく動いた。abort の後に画像はすべて解放され、再レンダーの出力も一致した。
- **パラメータ:** 既定値と範囲が `state.PARAM_SPECS` と機械的に一致した。
- **qtui:** `qtui_selftest` は 117/117。vendor のヘッダは Resolve の開発キットと同一で、SPDX は BSD-3-Clause。

**指摘:**
- L-1: Render が kOfxStatErrUnsupported を返す。Render の戻り値としてヘッダに定められていないので、kOfxStatFailed とメッセージにすべき。
- L-2: render がロックを持ったまま、clipGetImage と数秒かかるプロファイル作成を行う。frames_needed と instance_changed も同じロックを取るので、ホストの UI が止まり、デッドロックするおそれがある (推測)。
- L-3: プロファイルを作り直す条件が、frames_needed と render で違う。そのため宣言していないフレームを取得し得る。
- L-4: 近傍のフレームが取得できないと、レンダー全体が失敗する。
- I-1: abort の失敗にメッセージが無い。
- I-2: Mix の値の向きが、UI の表示 (100 − mix) と逆。
- I-3: 丸めの記述を訂正する。四捨五入なのは OFX の params.cpp で、qtui は銀行丸め。実用上は到達しない。
- I-4: Color Matrix の既定が常に BT.709。UI は、720 行未満でタグの無い素材には BT.601 を使う。
- I-6: work/ が gitignore で除外されている。
- I-7: 1080p・R=2 で最大 RSS は 818 MB。
- I-8: 複数のインスタンスを同時にレンダーする場合は、動的には試していない。

---
## Round 1 の指摘への対応 (作業者 n4-ofx、2026-10-31)
変えたのは `ofx/` だけで、C ABI は変えていない。経過は PROGRESS.md の「第 2 回」に書いた。

**指摘ごとの修正**
- L-1: Render の失敗は、すべて kOfxStatFailed と永続メッセージで返すようにした。GetClipPreferences で RGBA・Float を宣言する。
- L-2: ロックの中では、key の比較とプロファイルのコピーだけを行う。clipGetImage とプロファイルの作成はロックの外で行う。作成中に設定が変わって無効になったプロファイルは、世代番号で判別して保存しない。
- L-3: プロファイルを作り直す条件を profile_key にまとめ、GetFramesNeeded と Render で同じものを使う。profile_key は、設定・プロファイルに使うフレームの時刻・その時刻の Source の RoD の組。
- L-4: 近傍のフレームが取れないときは、その側の窓をそこで打ち切る (クリップの端と同じ扱い)。参照は、同じフレーム列を backend.process_frame と build_profile に渡したもので、ビット一致する。
- I-1: abort のときは Failed を返し、メッセージは出さない。
- I-2: Mix は UI の表示値にした (既定は 100、内部の値は 100 − Mix)。
- I-4: Color Matrix に Auto を足して既定にした。720 行以上は BT.709、それ未満は BT.601。境界の 160×720 と 160×719 で、2 つの参照の両方とビット一致した。

**付随して見つけた問題**
- 失敗の経路で、libc++abi が `std::runtime_error` を破棄するときに `/usr/lib/libstdc++.6.dylib` を dlopen していた。
- プラグイン独自の例外型に替えて解消した。

**結果 (観測)**
- `verify_ofx --pure --mutate`: 7 場面・74 比較で、0/90,210,304 語の不一致。新しい場面は、フレームの欠け・クリップ範囲の変更・再入 16 回。
- 拒否の 6 種は、すべて status 1 とメッセージで返る。
- 変異は 2 種とも検出された。
- 検証者の追加ケース (run_extra) は 40 比較で一致した。abort の注入と窓の縮小も合格した。
- 警告 0。fsguard の記録は、バンドルの dlopen の 1 件だけ。

## 独立検証 Round 2 (verify-n4): **APPROVE**
- Round 1 の L-1〜L-4・I-1・I-2・I-4 はすべて解消した。新しい M・L は無く、I が 4 件。
- **ビルド:** クリーンビルドで警告 0、エクスポートは 3 つ。
- **照合:**
  - `verify_ofx --pure --mutate`: 74 比較で 0/90,210,304 語の不一致。
  - 検証者の追加ケース: 70 比較で不一致 0。中身は、Mix 0・45.5・100・−20、Color Matrix の Auto/709/601/2020、フレームの欠けの各パターン、1280×720 の Auto。
- **Color Matrix と Mix の境界:**
  - Auto は、720 行で BT.709 を明示した出力と、187 行で BT.601 を明示した出力と、それぞれバイト同一。
  - Mix 100 は Mix を切った出力と同一、Mix −20 は Mix 0 と同一。
- **L-2 (ロック):**
  - 別スレッドから、レンダー中に 2 ms 間隔で GetFramesNeeded と Rebuild を最大 196 回呼んだ。プラグインは TSan でビルドした。
  - TSan の警告は 0、デッドロックは無く、出力はバイト同一。無効化されたプロファイルは保存されなかった。
- **abort と窓の縮小:** abort の注入と窓の縮小は合格した。
- **ファイルを使わないこと:** fsguard と sandbox-exec の両方で合格した。陽性対照は期待どおりだった。
- **例外型の件:** 検証者が再現した。最初に std::runtime_error を破棄したときだけ `dlopen(libstdc++.6.dylib, RTLD_NOLOAD)` が呼ばれ、NULL が返る。ファイルは読まないが、監視の基準をきれいに保てるので、変更は妥当と判断された。
- **新しい I:**
  - R2-I-1: 例外型の注釈に「RTLD_NOLOAD の問い合わせ」と書き添えるとよい。
  - R2-I-2: ホストが GetFramesNeeded と Render の間に InstanceChanged を呼ぶと、宣言していないプロファイルのフレームを取得し得る。TemporalClipAccess = 1 なので、取得そのものは許される。
  - R2-I-3: 無効化が続く間は、レンダーのたびにプロファイルを作り直す。設計どおり。
  - R2-I-4: 丸めの旧記述が残っていた。上で訂正した。
- **未確認:** 1 つのプロセスで複数のインスタンスを同時にレンダーする場合は試していない。

## Resolve での確認手順 (ユーザーが行う)
1. バンドルは `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/build/MeatVideo.ofx.bundle` (最新のソースでビルド済み、ad hoc 署名)。作り直すときは `ofx/build_ofx.sh` を使う。
2. `sudo cp -R <上のバンドル> /Library/OFX/Plugins/` でコピーする。
3. Resolve を起動する。すでに起動中なら再起動する。
4. Color ページの OpenFX ライブラリで、"Meat Video" の中に "Meat Video Denoise" があることを確かめる。無いときは `~/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml` を消してから再起動する (推測)。
5. プログレッシブの素材 (例: test_v02.mp4) のノードに適用し、次の点を確かめる。
   - ノイズが減る。
   - クラッシュしない。
   - パラメータを変えると結果が変わる。
   - 最初のフレームはプロファイルを作るので、数秒かかる。
6. 任意: インターレースの素材では、エラーのメッセージが出ることを確かめる。

---

# N4e 第 2 段 — Profile フォルダ・保存済みプロファイルの一覧 (本家どおり) と独自の Auto Match (repro) (2026-11-01、作業者 n4e-worker)

- worktree: `.claude/worktrees/n4e` (branch `n4e-automatch`、origin/main 922dc8b から。**未コミット**)。
- 作業者の報告全文 (Round 1 の 3 通と Round 2): `work/omo/wp_n4e/REPORT_DRAFT.md`。
  - サブエージェントは報告ファイルを書けないため、統括が SendMessage の本文を無改変で転記した。
- 独立検証の全文: `work/omo/wp_n4e/verifier/VERDICT_round1.md`・`VERDICT_round2.md`。
- 変更前の控え: `work/omo/wp_n4e/backup/` (SHA256SUMS.txt 付き)。ログ: `work/omo/wp_n4e/logs/` (Round 2 は `logs/r2/`)。

## 作業者の報告 (要約。全文は上記)

### 変更したファイル
- **新規**
  - `native/nvcore_c_match.{h,cpp}`: C ABI `nvc_match_model` (0xc300 プロファイルから AFT が読む語を取り出す) と `nvc_match_score` を追加。
  - `native/match_ref.py`・`native/verify_match.py`: Python の参照実装と、C++ とのビット一致照合。`make -C native verify` の最後に実行される。
  - `qtui/src/core/automatch.{h,cpp}`: フォルダ・一覧・match_model の JSON・best_match・run_auto_match。
- **変更**
  - `native/Makefile`、`native/nvcore_c.h` (コメントのみ)。
  - `qtui/src/core/nvcore.{h,cpp}`・`state.{h,cpp}`、`qtui/src/app/mainwindow.{h,cpp}`、`qtui/src/widgets/profile_panel.{h,cpp}`、`qtui/tests/selftest.cpp`。
- **触っていないもの**
  - 処理コアの Python (`nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`)。`reference/python/out/` にも書いていない。
  - 本家は実行も dlopen もしていない。Resolve と `/Library/OFX/Plugins/` にも触れていない。

### 本家どおりの部分
- **Profile フォルダ**
  - 既定は `<書類>/Meat Video/Profiles`。本家の `<書類>/Neat Video v6 OpenFX/Profiles` の製品名だけを置き換えた。
  - 環境変数 `MEATVIDEO_PATH` がディレクトリなら `$MEATVIDEO_PATH/Profiles` を使う。本家の `NV6OFX_PATH` と同じ型。
  - 設定キーは `prefs/profile_dir`。
  - Preferences > Folders に、編集欄・"…" (Select profile folder)・"Default" (Reset profile folder to default …) がある。
  - フォルダは自動では作らない。
- **▼ 保存済みプロファイルの一覧**
  - 列挙と並べ替えは本家と同じ QDir のフラグ (再帰、フォルダ先頭、大小無視)。
  - 対象は `*.json` で、`*.nvpreset.json` と `*.ui_params.json` は除く。
  - 空のときは、本家の文言の製品名だけを置き換えて表示する。
  - シンボリックリンクのループでは止まらない (canonical パスで各フォルダを 1 回だけ辿る)。
- **OFX ヘルパー**: 同じ設定ファイルと同じコードを通るので、同じフォルダを使う (コードの経路からの判断。Resolve 上は未確認)。

### 独自の Auto Match (repro。本家の Auto Match は空関数)
- 部品はすべて照合済みの移植: `model_full` (FUN_00995250)、`is_clip_genuine` (FUN_009c6f88)、`similarity` (FUN_009b6338)、`penalty` (FUN_009d49b4)。
- **新規プロファイルの JSON の追加キー**
  - Auto / Build Profile で作ったプロファイルの JSON の最上位に `"match_model"` を書く。各面 492 語と 51 語の 16 進で、約 13 KB。
  - 処理に使う base は変えない。match_model のない古い JSON も読める。不正な match_model は拒否する。
- **スコアの式**
  - 参照 r: 現在のフレームの FindFlatArea の正方形から作ったモデル。
  - 各 32×32 タイル (IsClip が R・G・B とも ≤ 0.33) で q_c = similarity × penalty(F₅/m₅) × penalty(F₆/m₆) を作り、p = q_G·q_R·q_B とする (NaN は 0)。
  - penalty の 2 項は、再現側が足した「逆向きのレベル検査」。
  - score = Σmin(p, r) / Σmax(p, r) (ファジー Jaccard 指数)。
  - 最大のプロファイルを読み込む。同点なら一覧の順で先のもの。最大が 0 以下なら読み込まない。
- **表示**
  - Prepare のツールバーに Auto Match ボタン。Profile メニューの "Auto Match (repro)" (Ctrl+M)。
  - パネルに "Match (repro)" 欄。Profile Viewer の "Match:" は "Area status (repro):" に改名。
  - ツールチップとメッセージには "repro" を明記した。
- **実行**: ワーカーで実行し、取り消せる。Auto Match と Auto/Build Profile は後から来た要求が勝つ。
- **C ABI**: 追加のみなので NVC_ABI_VERSION は 3 のまま。理由は `nvcore_c.h` に記載。

### 作業者の試験結果
| 試験 | 結果 |
|---|---|
| C++ と Python のビット一致 (34 フレーム、スレッド 1/4) | 1394 スコア、ALL MATCH |
| qtui クリーンビルド | warning 0 |
| qtui 自己試験 | 126/126 (117 + 新規 9 項目) |
| `make -C native verify` | exit 0 |
| `verify_bridge` | ALL MATCH |
| OFX クリーンビルド | warning 0 |
| `verify_ofx --quick` | 0/67,028,480 語不一致 |
| `verify_modal` | ALL OK |
| ini | 変化なし |

- **既知の正解の選択** (自己試験の項目 8)
  - 現在フレームの Auto Profile が **1.0000** で選ばれた。
  - 他の候補: 同じクリップのテクスチャ領域 0.0324、白色 σ8 の別クリップ 0.0739、白色 σ1 の別クリップ 0。
  - 除外されたもの: sub 違い、match_model なし、壊れた JSON。
  - 正解を除くと、40 フレーム離れた同じクリップのプロファイルが 0.6859 で選ばれた。
- **Round 2** (Round 1 の指摘への対応後): 自己試験 126/126、`make -C native verify` exit 0 (`verify_match` 1107 スコア ALL MATCH)、OFX と `verify_ofx --quick` 合格、ini 変化なし。

## 統括の再実行 (観測)
- **Round 1**
  - `make -C native lib` は warning 0。
  - scratch での qtui クリーンビルド (Qt 6.12.0) は warning 0。自己試験は 126/126。
  - `verify_match` (seed 777、24 フレーム) は 744 スコア × 2 で ALL MATCH。
  - OFX の再ビルドは warning 0。`verify_ofx --quick` は 81 比較で不一致 0。
  - ini は変化なし。`~/Documents/Meat Video` は作られていない。`repro/`・`reference/` と `*.py` は変更なし。
  - **不手際 (修正済み)**: 統括が実行した `make -C native clean` で、追跡中のバイナリ `native/nv_quality_test` が消えた。検証者の指摘 L1 で気づき、`git checkout` で戻した。
- **Round 2** (`make clean` は使わなかった)
  - 修正箇所は報告どおりだった (目視)。`native/*.py` は追跡対象だった。
  - `make -C native lib` と qtui のクリーンビルドは warning 0。自己試験は 126/126。
  - `native/verify_match.py` (seed 31337、23 フレーム) は 713 スコア × 2 で ALL MATCH。
  - OFX の再ビルドは warning 0。`verify_ofx --quick` は不一致 0。
  - ini は変化なし。追跡ファイルの削除はなし。

## 独立検証 Round 1 (verify-n4e): **REQUEST_CHANGES** (M 2・L 6)
- **回帰と照合はすべて合格**
  - クリーンビルド、自己試験 126/126、`verify_match` (seed 4242) 868 スコア、実写の全画面での照合 100 スコア (スレッド 1/3/8)。
  - `make -C native verify`、`verify_bridge` (669 秒)、`verify_ofx --quick`、ini 変化なし。
- **感度試験**: Python 参照を 10 通りに変え、8 通りを検出した。
  - 検出されなかったのは加算順の逆転と、クリップの閾値の 0.33 → 0.34 で、どちらも入力上の理由で差が出ない (想定内)。
- **本家の文字列との照合**: 本家バンドルのファイルを読むだけで照合し (実行していない)、空の文言・フォルダ名・環境変数・ボタン文言が製品名以外一致した。
- **指摘**
  - **M1 (blocker)**: Match 欄のツールチップが、試作段階の「平均」の式を説明していた。
  - **M2**: 照合スクリプトが `work/` (git の除外対象) にあり、マージ後に根拠を再現できない。
  - **L1**: `native/nv_quality_test` の削除 (統括の `make clean` が原因)。
  - **L2**: ABI 版数を据え置いたことの記録がない。
  - **L3**: シンボリックリンクのループで UI が止まる。
  - **L4**: 色差形式の判定が 2 か所で違う。
  - **L5**: Auto Match と Auto/Build Profile を同時に動かすと、プロファイルを取り違える余地がある。
  - **L6**: ボタンの表示文字列は "Auto Match" で、"repro" はツールチップにある (観測事項)。

## 独立検証 Round 2 (verify-n4e): **APPROVE** (blocker 0、新しい Low 3)
- **Round 1 の指摘**: M1・M2・L1〜L5 はすべて VERIFIED。
  - M2 は、work/ を含まない追跡対象だけの複製から `python3 -I verify_match.py` が動き、713 スコアが ALL MATCH だった。
  - L3 は、本物のリンク (根へのループ・自己ループ・2 本目) を使った試験を確認した。
  - L5 は、prof_req と automatch_req、busy の表示、AFT との関係を `worker.cpp` まで読んで確認した。
- **回帰**
  - クリーンビルドは warning 0。自己試験は 126/126。
  - `make -C native verify` は 12 本 (`verify_match` を含む) すべて合格。
  - OFX のビルドと `verify_ofx --quick` も合格。ini は変化なし。
- **verify_bridge の省略は妥当**: Round 1 との差は `nvcore_c.h` と `nvcore_c_match.h` のコメントだけで、`nvcore_c_match.cpp`・`nvcore.cpp` は同一だった。
- **新しい Low (記録して後回し)**
  - **R2-L1**: Auto Profile で Auto Match を取り消した直後、古いタスクが終わる前に Auto Match を再び押すと、二重押しの扱いになって新しい Auto Match が始まらない。窓は短い。
  - **R2-L2**: Load Profile・▼・Paste・Generic・Reset は、実行中の Auto Match を取り消さない。後から届く Auto Match の結果がそれを上書きする。対策案は `set_profile` の入口で `automatch_req` を進めること。
  - **R2-L3**: 取り消された古い profile のタスクが `busy("profile", false)` を呼ぶので、ビジー表示が早く消え得る。変更前からある挙動。
- **L3 の限界は許容と判断**: 並び順で先に来るリンクが一覧に出て、実フォルダの方は出ない。中身は必ず 1 か所に出て、二重に採点されることはない。

## 残る点
- **Resolve の実機確認 (未実施。N4d の保留項目に追加)**
  - ClipParams は match_model の分 (約 13 KB) 大きくなり、26 KB 前後になる。文字列パラメータとして問題ないか。
  - OFX のヘルパーが同じ Profile フォルダを使うか。
  - 実画面での Ctrl+M。
- **作業者が挙げたリスク**
  - Batch Profiler は match_model を書かないので、そのプロファイルは Auto Match の対象外になる。
  - Python UI との比較ツール (`compare_ui.py`・`compare_n3c.py`) は、新しいキーとボタンの分だけ差が出るはず (未実行)。
  - Auto/Build Profile のたびに BuildNoiseModel をもう 1 回走らせる (128² で約 1 秒、320² で約 4 秒)。
  - スコアの識別力を確かめたのは test.mp4 と合成ノイズだけ (推定の範囲)。
  - 二重押しの UI 試験は「取り消し」か「完了」のどちらでも合格にしている。
- **任意の後始末**: R2-L1〜L3。
