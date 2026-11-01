# N4d 独立検証 VERDICT (verify-n4d、2026-11-01)

対象: worktree `.claude/worktrees/n4-ofx` の未コミット変更 (ofx/、qtui の ofx_session.{h,cpp}・ofx_session_drv.cpp・main.cpp・mainwindow.{h,cpp}・CMakeLists.txt)。
コードを先に読み、自分の試験を作ってから最後に REPORT.md を読んだ。製品コードは変更していない。
作業物: scratchpad `verify_n4d/` (新規ビルド・作業ディレクトリ)、写しは本ディレクトリの `scripts/`・`logs/`。

## 総合判定: **REQUEST_CHANGES (小規模)**
受け入れ条件 1〜8 はすべて観測で満たした (ビット一致・fsguard・既定 3 通り・回帰・クリーンビルド)。Critical / High はない。
差し戻しの理由は Medium 1 件 (深い入れ子の ClipParams でホストが SIGSEGV)。修正は小さい (PyObj::parse に深さ上限)。
Low 6 件は任意 (うち L1 は build_ofx.sh 1 行で直る)。

## 指摘 (重大度順)

### M1 (Medium): 入れ子の深い ClipParams でホストのプロセスが落ちる (観測)
- PyObj::parse (qtui/src/core/pyobj.cpp、プラグインに組み込み) は再帰下降で深さ上限がない。ClipParams は保存済みプロジェクト・貼り付けからホストが渡す値。
- 観測 (minihost、主スレッド 8 MB スタック、`--init-clip-params`):
  `{"format":"meatvideo_ofx_clip_params","version":1,"x": [[[…]]]}` の入れ子 1000 / 5000 / 20000 は正常 (素通し)、
  **50000 / 100000 は rc = −11 (SIGSEGV)**。`[`×300000 の素の配列も −11。落ちるのは CreateInstance (update_adjust の解析)。
- 推定: 1 段あたり約 170〜400 バイトのスタック。ホストがワーカースレッド (macOS の既定 512 KB) で CreateInstance / Render を呼ぶと数千段 (数 KB の文字列) で落ち得る。
- 「壊れた ClipParams はホストを落とさない」という条件に反する。正規の UI は深さ 5 程度しか作らないので、壊れた/細工されたプロジェクトでのみ起きる。
- 提案: PyObj::parse に深さ上限 (例 256) を入れて例外にする (qtui 側も同じパーサを使うので一緒に守られる)。またはプラグインで解析前に括弧の深さを数えて拒否。

### L1 (Low): 同梱ヘルパーがビルド機のパスを埋め込み、バンドル外のファイルを読む (観測)
- qtui/CMakeLists.txt の `NV_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."` がヘルパーにも入る (`strings` で
  `/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/qtui/..` を確認)。nvcore.cpp reference_floor() の候補は
  `<app>/Contents/Resources/data/ref_floor.npy` → `MacOS/data/…` → `NV_SOURCE_ROOT/data/ref_floor.npy` の順だが、build_ofx.sh は Resources/data を作らない。
- 観測: open を記録する自作の差し込み (scripts/logopen.c) で qtui_ofx_session_drv (同じ nvui) に "auto;cancel" を流すと、
  `fopen /Users/seitan/…/n4-ofx/qtui/../data/ref_floor.npy` (バンドル外・リポジトリ内) を読んだ。
- 影響は小さい: 別の機械では計算に落ち、`nvc_reference_floor(NULL, …)` は 0.44〜0.51 秒で data/ref_floor.npy とビット一致 (観測)。結果は変わらない。
- otool -L / LC_RPATH ではビルド機を指す参照はない (下の表)。「ビルド機のパスへの参照なし」は文字列としては満たしていない。
- 提案: build_ofx.sh で `data/ref_floor.npy` を `MeatVideoQt.app/Contents/Resources/data/` に入れる (既存の候補 1 番目に当たる)。

### L2 (Low・設計): 空の ClipParams で保存されたインスタンスは「新規」として扱われ、開き直しで既定が入る (観測)
- `--init-clip-params` に空ファイル + 既定あり (set) → CreateInstance で `--ofx-default` を起動し、SyncPrivateData で既定 12,574 バイトが ClipParams に保存された。
- 仕様は「新しいインスタンス (ClipParams が空)」と定義しているので仕様どおり。ただし「プロファイルを作らずに保存したノード」は、後で既定を設定すると開き直しで処理が変わる。
  REPORT の「ホストが InstanceChanged も SyncPrivateData も呼ばずに保存した場合」のリスクと同根。Resolve での確認項目に加えるとよい。
- 参考: 本家の内部名 `loadDefaultProfileOnSetSrcImage` は「ソース画像が渡されたとき」に読む設計を示唆する (推定)。

### L3 (Low): パイプの fd が親プロセスで CLOEXEC でない (コードレビュー)
- session.cpp run_helper: `pipe()` の 4 本に FD_CLOEXEC を付けていない。子は POSIX_SPAWN_CLOEXEC_DEFAULT で 0/1/2 だけになる (観測: 子の開いている fd は 0,1,2 のみ、シグナルのマスク・無視 0)。
- ただし多スレッドのホストが同時に fork/exec すると、その子が out[1] を継承し得る。応答は EOF まで読むので、その子が生きている間 InstanceChanged が戻らない (推定のリスク)。
- 提案: pipe 直後に fcntl(FD_CLOEXEC)、または応答の長さが揃った時点で読みを止める (APPLY n / CANCEL は自己区切り)。

### L4 (Low): store_clip_params が途中で失敗すると paramEditEnd が呼ばれない (コードレビュー)
- plugin.cpp store_clip_params: paramEditBegin の後の paramSetValue が失敗すると Failure を投げ、EditEnd を呼ばない (ホストの Undo ブロックが開いたまま)。RAII で閉じるとよい。
- commit_defaults は pending を先に空にしてから保存するので、保存が失敗すると既定が失われる (同じく Low)。

### L5 (Low・情報): dlopen_noload(/usr/lib/libstdc++.6.dylib) は Render 経路でも起こり得る
- 主張の機構は正しい: libc++ の `__libcpp_refstring` 破棄で `compute_gcc_empty_string_storage()` が RTLD_NOLOAD で探す。static 局所で**プロセスに 1 回だけ**。
- 観測: JSON でない ClipParams (`{not json`) を init に与えると dlopen_noload が 1 件記録された (CreateInstance の解析で)。
  ホストが InstanceChanged なしで ClipParams を変えてから最初の解析が Render で起きれば Render 経路で出る (稀)。PyObj の std::runtime_error が原因で、params.h の Error 型の工夫では避けきれていない。
  ファイルは開かない (dyld のイメージ照会)。システムコール単位の確認 (fs_usage / dtruss) は root が必要で未実施。

### L6 (Low・情報): 細かな不一致
- 本家の Describe にある FilmLight ホスト用 `uk.ltd.filmlight:OfxParamPropMainThreadAction` は付けていない (Resolve では無関係)。本家は Byte/Short/Half も受けるが、実装は Float のみ (N4b からの方針)。
- `"bs": "16"` (文字列) は as_int で 0 扱いになり「the frame is smaller than the motion block」と誤解を招く文言になる (落ちはしない)。`"search": -50`・`"radius": 2.5`・`threshold NaN` は黙って受け付けて描画する (UI はこの値を作らない)。
- ホストに SIGCHLD を刈り取るハンドラがあると waitpid が ECHILD で status 0 になり「終了 0」と扱う。応答は長さで検証されるので壊れた応答は通らない (情報)。

## 証拠

| 確認 | 結果 | コマンド / 出典 | 出力 |
|---|---|---|---|
| クリーンビルド (バンドル) | pass | `ofx/build_ofx.sh <scratch>/build` (新規ディレクトリ) | rc 0、warning/error 0 (logs/build.log)。MeatVideo.ofx の LC_UUID CEEF0A60… は worktree の ofx/build と同一 (ビルドは最新のソース) |
| クリーンビルド (qtui 全ターゲット) | pass | `cmake -S qtui -B <scratch>/qtui && cmake --build -j3` | warning 0 (logs/qtui_build.log)。両 CMakeLists に `-ffp-contract=off -fno-fast-math -Wall -Wextra`。qtui_ofx_session_drv は worktree のものと sha256 一致 |
| Describe | pass | `minihost <scratch bundle> --check` | 7 パラメータ: Controls / Prepare Profile... ("Prepare Noise Profile"、ヒント・スクリプト名一致、enabled 1) / Adjust Filters... ("Adjust Filter Settings"、**enabled 0**) / ClipParams (Custom, secret 1, default "", InvalidateAll, animates 0) / ParamsHash1..3 (Integer, secret, default 0, InvalidateAll, animates 0)、ページ子の順も一致。prof_0xa22c.c の 51〜119 行と文字列一致。CHECK OK |
| 作業者の照合 (全編) | pass | `verify_ofx.py --mutate --work <scratch>/vo_full` | 80 比較 0/85,386,752 語不一致、ALL MATCH、変異 0〜11 すべて期待どおり、ヘルパー失敗 6 種・拒否 7 種・同梱ヘルパー ok (logs/vo_full.log、57 秒) |
| **自作の場面** (新規) | pass | `scripts/my_scen.py <scratch>/my1 --pure` | 36 比較 0/59,616,000 語不一致 (ブリッジ・**純 Python**・qtui 自身の処理) (logs/my1.log) |
| 同 (de_DE ロケール) | pass | `LC_ALL=de_DE.UTF-8 … my_scen.py --only=…` | 14 比較 0/20,275,200 (logs/my_de.log)。ホストに `setlocale(de_DE)` を差し込んだ minihost (小数点 "1,500" を確認) でも描画・ClipParams がロケールなしの実行とバイト同一 |
| 空 ClipParams の素通し | pass | 自作 X-v02-aft t=52 ×2、作業者 D-none 等 | 入力と同一 (uint32)、"no noise profile yet…" 付き |
| 壊れた ClipParams 14 種 | 13 pass / **1 fail** | `scripts/run_mh.py -- … --init-clip-params X --render 2` | 非 JSON・levels 7・curve_centers 空/文字列・render が配列・P 欠落・limited range → status 1 + メッセージ、Adjust 無効。bs 巨大/文字列 → status 1。**入れ子 5 万段以上 → SIGSEGV (M1)** |
| ボタン・ヘルパー | pass | 作業者の場面 + 自作 (`scripts/inspect`, `fds.c`) | Apply で保存、Cancel/close/失敗で不変。子の fd は 0,1,2 のみ、シグナルのマスク・無視 0。DYLD_LIBRARY_PATH・QT_PLUGIN_PATH・QT_QPA_PLATFORM_PLUGIN_PATH・DYLD_INSERT_LIBRARIES は子に渡らない。押下 3 回で親の fd 数は不変 (漏れなし)、前回のヘルパーのゾンビなし |
| 既定 | pass | 作業者 D-* 7 場面 + 自作 X-dset | あり/なし/壊れた (両方・片方ずつ) で期待どおり。既存インスタンス (D-existing) は問い合わせ 0。既定ありの新規インスタンスの描画は Python・qtui とビット一致 |
| fsguard | pass (L5 付き) | 全場面の記録 | Render のみの場面は dlopen(バンドル) 1 件だけ。他はインスタンスごとの `--ofx-default` と押下ごとの `--ofx-session` の posix_spawn だけ |
| プラグインの import | pass | `nm -u MeatVideo.ofx` | ファイル API なし (read/write/close/pipe/fcntl/posix_spawn 系/waitpid/dladdr/getenv/newlocale/uselocale)。libnvcore の fopen/fread は nvc_reference_floor 用で、プラグインは import していない |
| 同梱の自己完結 | pass (L1 付き) | `codesign -v --deep --strict`、otool -L/-l 全 14 Mach-O、DYLD_PRINT_LIBRARIES | 署名 valid (adhoc)。依存は @rpath/@loader_path/@executable_path と /System・/usr/lib のみ。rpath はバンドル相対のみ。起動時 Qt 4 フレームワーク・libnvcore・libqoffscreen はすべてバンドル内から。qt.conf あり |
| qtui selftest | pass | `HOME=<scratch> QT_QPA_PLATFORM=offscreen <scratch>/qtui/qtui_selftest` | 117 passed, 0 failed (logs/selftest.log) |
| 既存 --ofx モード | pass | `scripts/cmp_ofxmode.py` (HEAD の qtui を git archive で別ビルド vs 今回) | compare_ofx.py の全 26 ケース + 引数エラー 3 種で stdout・終了コード・メッセージが同一 (ALL SAME) |
| make -C native verify | pass | `make -C native verify` | PASS (プロファイル 39/39、AFT 全件、667/667 バイト等、8 分 49 秒) (logs/native_verify.log) |
| ユーザー設定 | pass | sha256 前後 | ~/.config/NeatRepro/nv_ui_qt.ini = 45d4258c…fcc6 (全作業の前後で同じ) |

## 受け入れ条件

| # | 条件 | 状態 | 根拠 |
|---|---|---|---|
| 1 | Describe が本家どおり (Controls・2 ボタン・ClipParams・ParamsHash1..3、他に見えるパラメータなし) | VERIFIED | minihost --check と prof_0xa22c.c の比較 (MainThreadAction の省略は L6) |
| 2 | Render は ClipParams だけ、qtui / ブリッジとビット一致、空は素通し、壊れた値の扱い | PARTIAL | ビット一致は作業者 80 + 自作 36 + ロケール 14 比較で不一致 0 (純 Python 含む)。素通し OK。**深い入れ子でホストが落ちる (M1)** |
| 3 | ボタン: Apply/Cancel/close、Adjust の有効/無効、ヘルパー失敗でも ClipParams 不変・ホスト無事・SIGPIPE なし | VERIFIED | 作業者の失敗 6 種 + SIGTERM、自作の inspect/fds、cancel/close の場面。fd 漏れ・ゾンビなし |
| 4 | 既定 (あり/なし/壊れた・片側ずつ)、既存不変、空で開くと既定、リセットで既定、単体 qtui 不変、pending 設計の妥当性 | VERIFIED (L2 付き) | D-* と X-dset。単体 qtui は selftest 117/117 と --ofx の比較。pending 設計は ofxParam.h の「paramSetValue は InstanceChanged / interact のみ」に沿う。リスクは L2・L4 と REPORT 記載のもの |
| 5 | fsguard: Render 0、ボタン/作成は posix_spawn のみ、libstdc++ 例外の検討、import にファイル API なし | VERIFIED (L5 付き) | 上表 |
| 6 | 同梱の自己完結 (Helpers、バンドル内 Qt、qt.conf、署名、ビルド機パスなし) | PARTIAL | otool/codesign/DYLD_PRINT_LIBRARIES は OK。ヘルパーにビルド機パスの埋め込みと実際の読み込み (L1) |
| 7 | 回帰 (selftest・--ofx・native verify・警告 0・ini 不変) | VERIFIED | 上表 |
| 8 | コードレビュー | VERIFIED (L3・L4・L6) | fd・ゾンビ・部分読み書き (write_all/read ループ・EINTR 対応)・ロケール (to_chars/from_chars + uselocale) は問題なし |

## 自作の場面 (scripts/my_scen.py。verify_ofx.py の run_scenario を流用し、場面と素材は新規)
- X-v02-aft: test_v02 n=560..569 の 720×720 切り出し (高さ 720 = BT.709 の境界)、first 50、origin (−7, 13)、負の rowBytes。
  Prepare@55 でウィンドウ内のフレームを 2 に移して Auto Profile → **AFT** → フレーム 5 に戻し、t_radius 3・amt_Cr 70・bnoise_Y_high 120・tune Cb 1.3 で Apply
  (チャンネル別経路 uniform False)。描画 55/50/59/57 (クリップ端の非対称な窓)。Adjust@58 で半径 5・noise_Y 140・Mix 50。Adjust@51 Cancel。Prepare@52 で Reset Profile (既定なし) → Apply → Adjust 無効・素通し。
- X-dset: 既定は V1 素材から作った別のプロファイル (matrix "auto" → 720 行で BT.709)。保存前の描画 2 時刻、sync、Adjust で変更、既定を消して sync (不変)。
- X-thin: 600×64 (最小の高さ、BT.601) で Auto Profile (Python の build_profile と一致)。

## 確認できなかったこと
- Resolve 上の動作 (ウィンドウの前面表示、UI の停止、SyncPrivateData / InstanceChanged の呼び出し順、プロキシ解像度)。REPORT の手順でユーザーが確認する。
- システムコール単位のファイルアクセス (fs_usage / dtruss は root が必要)。
- 複数インスタンスの同時レンダー、ワーカースレッドでの M1 の段数 (推定のみ)。

---

# Round 2 (2026-11-01、修正後の再検証)

## 総合判定: **REQUEST_CHANGES (1 行程度の修正 1 件)**
M1・L1・L3・L4・L5・L6 の修正はそれぞれ自分の試験で確かめ、どれも有効だった (Round 1 のバンドルを陽性対照に使い、修正前は失敗・修正後は成功を確認)。
回帰もすべて合格。ただし **L3 の修正 (pipe 直後の FD_CLOEXEC) が新しい不具合 R2-1 を入れた**。ホストの fd 0 (または 1) が閉じていると、ヘルパーの stdin (stdout) が閉じたまま起動し、ウィンドウが開かない。直すのは小さい。

## 新しい指摘

### R2-1 (Medium・回帰、観測): ホストの標準 fd が閉じているとウィンドウが開かない
- 機構: fd 0 が閉じたプロセスでは `pipe(in)` が in[0] = 0 を返す。L3 の修正で in[0] に FD_CLOEXEC が付く。
  `posix_spawn_file_actions_adddup2(&fa, 0, 0)` は同じ fd どうしの dup2 なので何もせず、フラグも消えない。
  そのため POSIX_SPAWN_CLOEXEC_DEFAULT と合わせて、子は stdin が**閉じた状態で** exec される。
  fd 1 が閉じていれば、同じことが応答の out[1] で起き、--ofx-default も壊れる (コードからの推定。fd 1 は minihost のログなので試していない)。
- 観測: `preexec_fn=os.close(0)` で minihost を起動し、既定ありのインスタンスで Adjust を押した。
  - Round 2: `press adjust status=1 … "the window helper exited with status 1 before it read the request"`。ヘルパーの stderr は `ofx-session: no request header on stdin`
  - Round 1 のバンドル: 同じ条件で status 0・ウィンドウ表示・Apply 成功
- 影響: Finder から起動した GUI アプリは通常 0/1/2 が /dev/null などに開いているので、Resolve で起きる可能性は低い (推定)。ただし起きればボタンの機能が全滅する。
- 提案 (どれか 1 つ):
  - パイプの fd を `fcntl(fd, F_DUPFD_CLOEXEC, 3)` で 3 以上に移してから使う
  - 0〜2 番に当たった fd だけは CLOEXEC を外す
  - in[0] == 0 / out[1] == 1 のとき addinherit_np を使う

### 軽微 (任意、Low)
- mix 1.0000001 の拒否メッセージが `P["mix"] = 1.000000 is outside [0.000000, 1.000000]` と表示される (std::to_string の 6 桁。%.17g か py_float_repr の方が明確)。
- 既定の保存に失敗したときの警告は、後の sync で保存に成功しても次の render まで残る (sync 行の message に残っていた。render で消える)。
- store_clip_params で ClipParams の設定は成功し ParamsHash の設定だけ失敗した場合: 次の commit では ClipParams が空でないので pending を捨て、ParamsHash は古いまま残る (コードレビュー。--fail-set は Custom/String だけなので試せない)。
- frame_size に巨大な float があると、as_int で float → int64 の範囲外変換 (未定義動作。arm64 では飽和) になる。情報用の値なので処理には影響しない。

## 修正の確認 (自分の試験。数値は観測)

| 項目 | 結果 | 試験 | 出力 |
|---|---|---|---|
| M1 深い入れ子 | 修正を確認 | `round2/scripts/gen.py` が作る ClipParams (有効な ClipParams に `"zz": <入れ子>` を足したもの。配列だけと、配列とオブジェクトの交互)。`--thread-stack 512`・`128` で、CreateInstance (init) と、ホストが設定した後のレンダー (`--clip-params` → render) の両方 | 深さ 255・**256 (上限ちょうど)**: 受理し描画は status 0、Adjust 1 (128 KB スレッドでも同じ)。257・3000・100000: status 1 で "nesting deeper than 256 levels"、Adjust 0、落ちない、fsguard 1 件。**陽性対照: Round 1 のバンドルは 512 KB スレッドで深さ 3000 のとき rc −10 (SIGBUS)** (Round 1 の推定どおり、数千段で落ちる) |
| M1 qtui への影響 | なし | リポジトリ内の JSON 28 件の入れ子の最大は 5 (`qtui/tests/fixtures/py_profile.nvprof.json`)。selftest 117/117 (プロファイル・プリセットの読み込みを含む)。--ofx モードは HEAD と全 29 ケース同一 | 上限 256 は実ファイルより 50 倍深い |
| L1 ビルド機のパス | 修正を確認 | 新規ディレクトリへのクリーンビルドのバンドル全体を `LC_ALL=C grep -rlaF`: `/Users/seitan`・`/private/tmp`・`worktrees`・`Qt/6.12.0` | どれも 0 ファイル (`/Users/qt/work` は Qt 社のビルドパスで QtWidgets 内。当方のものではない)。ヘルパーの `strings` の絶対パスは /dev/null と ffmpeg 探索用の /opt/homebrew/bin・/opt/local/bin・/usr/local/bin だけ。Resources/data/ref_floor.npy は data/ と同一。codesign --deep --strict 有効 |
| L1 実行時の読み込み | 修正を確認 | OFF でビルドした自分の qtui_ofx_session_drv を、バンドルのヘルパー app の写しに入れた (rpath は @executable_path/../Frameworks だけにした)。open/stat/access/fopen を記録する差し込み (Round 1 の logopen.c) で "auto;aft;state" を実行し、開発用ドライバと比べた | 写し: ref_floor は `appcopy.app/Contents/MacOS/../Resources/data/ref_floor.npy` の access + fopen だけ。Qt 4 つ・libnvcore・libqoffscreen もすべて写しの中から読んだ。開発用: `qtui/../data/ref_floor.npy` を fopen。**状態 JSON (AFT 後のプロファイルを含む) は両者で完全に同一** |
| L3 CLOEXEC | 修正を確認 (ただし R2-1) | posix_spawn に差し込んだ自作ライブラリで、spawn の瞬間の親の FIFO の fd フラグを見た | Round 2: fd 3〜6 すべて cloexec (--ofx-default・--ofx-session とも)。Round 1: すべて INHERITABLE |
| L3 長さ区切りの読み | 修正を確認 | 偽ヘルパー `holder`: 完全な APPLY を返したあと、stdout を握った子 `(sleep 20 &)` を残す (--ofx-default も同じ) | Round 2: 実行全体 0.4 秒、Apply が保存された。Round 1: 40.1 秒 (20 秒の待ちが 2 回) |
| L4 paramEditEnd | 修正を確認 | `--fail-set 1` → Prepare (auto;apply) → sync | Round 2: Apply は status 1 "paramSetValue ClipParams"、ClipParams 不変、sync の edit_depth=0。**Round 1: edit_depth=1 (ブロックが開いたまま)**。次の Prepare で正常に保存 |
| L4 既定の再試行 | 修正を確認 | 既定あり + `--fail-set 2` → sync ×3 (間に render)。`--fail-set 1` → sync → Adjust 押下 | 1・2 回目の sync は警告を出して保存しない (edit_depth=0)。その間の render は保留中の既定で status 0。3 回目で 12,312 バイトを保存。押下の場合は、押下時の commit で保存してから窓を開き、Apply (edit_blocks=2) |
| L5 dlopen_noload | 修正を確認 | 512 KB スレッドで: JSON でない init、ホストが設定した後の非 JSON / 途中で切れた JSON の render、非 JSON の既定 (`DEFAULTS 5 ok ok\n{bad}`) と非 JSON の APPLY を返す偽ヘルパー | どの経路でも dlopen_noload 0 件 (記録は dlopen と posix_spawn だけ)。**陽性対照: Round 1 は「設定後の非 JSON」で dlopen_noload 1 件**。作業者の全体照合のログでも 0 件 |
| L6 UI の上限・下限 | 修正を確認 | `round2/scripts/ext_scen.py` (新しい素材: test_v02 n=400..411 の 400×256 切り出し、BT.601)。ウィンドウで全スライダーを下限 (半径 0・量 0・ノイズ 0・threshold 0.25・spatial 0.25・Overall 25%・Mix 有効 0・t_noise 0・tune 0.2 ×4) と上限 (半径 5・量 100・ノイズ 250%・threshold 8・spatial 4・Overall 400%・Mix 100・t_noise 250%・tune 3.0 ×4、AFT 込み) にして Apply。Temporal/Spatial 無効も試した | 3 回の Apply がすべて受理された。ClipParams の値を確認 (radius 0/5、noise_level 0.25/4.0、mix 0/1.0、t_noise 0/2.5 など)。**19 比較 0/7,475,200 語不一致** (qtui・ブリッジ・純 Python) |
| L6 範囲外の拒否 | 修正を確認 | 512 KB スレッドで、正しい状態から `--clip-params` で差し替えて render (26 種) | radius 17/−1/true/2.0、bs 3/257、search 257、mix −0.1/1.0000001、threshold −1/1e7/inf、t_noise NaN/−0.5、noise_level が文字列、band_amounts −1、channel_noise NaN、levels 6/2.0、curve NaN、band_ratio inf、tot_ratio −inf、曲線 17 点、チャンネル別の bands −2: すべて status 1 で、どのキーが悪いかを示すメッセージ。radius 16 (境界) は受理 |

## 回帰 (すべて今回のソースで、新規にビルド)
| 確認 | 結果 | 出力 |
|---|---|---|
| クリーンビルド (build_ofx.sh、新規ディレクトリ。パス検査込み) | pass | rc 0、warning/error 0。バイナリは worktree の ofx/build・qtui/build と sha256 一致 |
| qtui 全ターゲット (新規ディレクトリ) | pass | warning 0 |
| verify_ofx.py --pure --mutate (全場面) | pass | **163 比較、0/170,596,864 語不一致、ALL MATCH**。拒否 17 種・ヘルパー失敗 6 種・holder・同梱ヘルパー・同梱データ・変異 0〜12 すべて ok、dlopen_noload 0 件 (round2/logs/vo_full.log) |
| 自作の Round 1 の場面 (my_scen.py --pure) | pass | 36 比較 0/59,616,000 |
| 自作の上限・下限の場面 (ext_scen.py --pure) | pass | 19 比較 0/7,475,200 |
| qtui_selftest | pass | 117 passed / 0 failed |
| 既存の --ofx モード (HEAD vs 今回) | pass | 26 ケース + 引数エラー 3 種、ALL SAME |
| native | 未実行 | git status で native は無変更。前回 PASS |
| ~/.config/NeatRepro/nv_ui_qt.ini | pass | 45d4258c…fcc6 (前後で同一) |

## 判定の条件
R2-1 を直し、fd 0 を閉じた起動で Prepare/Adjust の窓が開き Apply できること (上の preexec の試験) を確認できれば APPROVE。
軽微 4 件は任意。

---

# Round 3 (2026-11-01、R2-1 と軽微 4 件の修正後の再検証)

## 総合判定: **APPROVE**
R2-1 は直っていた。ホストの fd 0/1/2 を閉じる 8 通りの組み合わせすべてで、既定・Prepare・Adjust・Apply・描画が動き、出力と ClipParams は全部開いた起動とバイト単位で同一だった。
fd の漏れはなく、L3 の挙動 (CLOEXEC、長さ区切りの読み) も保たれている。軽微 4 件も直っていた。回帰はすべて合格。
新しい指摘は情報レベルの 1 件 (R3-1) だけで、差し戻しの理由にはならない。

## 確認 (自分の試験。数値は観測)

| 項目 | 結果 | 試験 | 出力 |
|---|---|---|---|
| fd を閉じたホスト | pass | `round3/scripts/fdmatrix.py`: minihost を `preexec_fn` で fd を閉じて起動 (ログは --log)。閉じる組は ()・(0)・(1)・(2)・(0,1)・(0,2)・(1,2)・(0,1,2) の 8 通り (作業者の試験にない fd 2 単独と 2 つの組を含む)。手順: 既定ありの新規インスタンス (--ofx-default) → render → Prepare (frame 3;auto;aft;frame 5;t_radius 4;apply) → render ×3 → Adjust (amt_Cb 40;tune Y 1.3;apply) → render。first 20 | 8 通りとも rc 0、作成時に既定が入り Adjust 1、押下 2 回 status 0・changed、描画 5 回 status 0、fsguard 4 件 (dlopen + spawn ×3)。**出力と ClipParams 2 件は 8 通りとも全部開いた起動とバイト同一** |
| 親の fd | pass | posix_spawn に差し込んだライブラリで、spawn の瞬間の親の FIFO を記録 (3 回押下 + 作成)。組 ()・(0)・(1) | 毎回 4 本、すべて fd ≥ 3 で cloexec。押下を重ねても同じ組で、漏れなし。fd 0/1 にパイプは残っていない (元の fd は閉じられている) |
| 子の fd | pass | 偽ヘルパー (fds.c) で、子が持つ fd・シグナルのマスク・無視を記録。組 ()・(0)・(1)・(2)・(0,1,2) | 0/1/2 (fd 2 はホストが開いているときだけ) と、ヘルパー自身が開いた fd だけ。継承された余分な fd なし、マスク・無視 0 |
| L3 の維持 | pass | holder (完全な APPLY の後、stdout を握った子を残す)。組 () と (0,1) | どちらもすぐ戻り Apply が保存された |
| `--ofx-default` 単体 | pass | ドライバを直接、fd 2 を閉じて / 開いて起動。設定は set・badprof・badpre・broken | 応答 (見出し・長さ・中身) が両者で同一 |
| Low: メッセージ | pass | mix 1.0000001、threshold 1e7 | `P["mix"] = 1.0000001 is outside [0.0, 1.0]`、`P["threshold"] = 10000000.0 is outside [0.0, 1000000.0]`。mix −0.0 は受理 |
| Low: 古い警告 | pass | 既定 set + `--fail-set 2` → sync ×3。既定 badpre + `--fail-set 1` → sync ×2 | 保存に成功した sync で警告が消えた。badpre では、元の "Selected default preset cannot be loaded." に戻った |
| Low: frame_size | pass | [1e300, −1e300]・["a", null]・7 | すべて受理、status 0、Adjust 1 |
| Low: ParamsHash の再同期 | コードレビューのみ | 試験手段なし (--fail-set は Custom/String 用) | commit_defaults の分岐を読んで確認 |
| L1 の維持 | pass | 新規ビルドのバンドルを `LC_ALL=C grep -rlaF` | /Users/seitan・/private/tmp・worktrees: 0 ファイル。codesign 有効 |

### R3-1 (情報・Low): ヘルパーの「fd 2 が閉じていたら /dev/null」の分岐は実際には通らない
- 観測:
  - ドライバに dup/dup2 を記録する差し込み (`round3/scripts/dup.c`) と lsof で調べた。
  - fd 2 を閉じて起動すると、`ofx_session_main` / `ofx_default_main` に入る時点で fd 2 は既に FIFO になっていた。QApplication/QCoreApplication の構築中に Qt 内部のパイプが fd 2 に割り当てられる。lsof では、そのパイプの片側が fd 3。
  - そのため `stray_output_to_stderr()` は fd 2 を stderr とみなして `dup2(2, 1)` を行う。stderr への出力は、このパイプの読み口への書き込みになり失敗するとみられる (推定)。
- 影響:
  - 応答は reply_fd (fd 4) なので壊れない。上の 8 通りと `--ofx-default` の 4 種でバイト同一を確認した。
  - 失われるのはヘルパーの診断出力だけ。
- 提案 (任意): main() の冒頭、QApplication を作る前に 0〜2 番が閉じていれば /dev/null で埋める。

## 回帰 (今回のソースで新規にビルド)
| 確認 | 結果 | 出力 |
|---|---|---|
| クリーンビルド (build_ofx.sh、新規ディレクトリ) | pass | rc 0、warning/error 0。バイナリは worktree のものと sha256 一致 |
| qtui 全ターゲット (新規ディレクトリ) | pass | warning 0 |
| verify_ofx.py --pure --mutate (全場面) | pass | **163 比較、0/170,596,864 語不一致、ALL MATCH**。fd を閉じた起動 4 通り ok、dlopen_noload 0 件 (round3/logs/vo_full.log) |
| 自作の場面 (my_scen.py --pure) | pass | 36 比較 0/59,616,000 |
| 自作の上限・下限 (ext_scen.py --pure) | pass | 19 比較 0/7,475,200 |
| qtui_selftest | pass | 117 passed / 0 failed |
| 既存の --ofx モード (HEAD vs 今回) | pass | 29 件 ALL SAME |
| native | 未実行 | 無変更 (Round 1 で PASS) |
| ~/.config/NeatRepro/nv_ui_qt.ini | pass | 45d4258c…fcc6 (前後で同一) |

## N4d としての結論
Round 1 の M1・L1〜L6、Round 2 の R2-1 と軽微 4 件はすべて解消した。
残るのは情報レベルの R3-1 と、仕様どおりの L2 (空の ClipParams = 新規)。Resolve 上での確認項目 (ウィンドウの前面表示、UI の停止、アクションの呼び出し順、L2) はユーザーが REPORT の手順で行う。

---

# Round 4 (2026-11-01、「ウィンドウ表示中に Resolve が応答なし」の修正 = ModalWait)

## 総合判定: **REQUEST_CHANGES**
主スレッドがイベントを取り出し続けること、入力が待ちの後に溜まって配られないこと、既存の照合と回帰はすべて確認できた。
しかし ModalWait の待ちは実際には待っておらず、**ホストの主スレッドを CPU 100% で回し続ける** (R4-1、High)。
そのうえ、Cocoa ホストではウィンドウが開くまでの時間が 1080p × 11 フレームで約 16 倍 (0.67 → 10.5 秒) になる (R4-2、High)。
どちらも、作業者の試験 (verify_modal.py) は「待ち中の sample の大半が pump の中」を合格の条件にしており、空回りと区別できないため見逃している。

## 指摘

### R4-1 (High、観測): 待ちの間、ホストの主スレッドが空回りして CPU を 100% 使う
- 観測 1 (CPU 時間): cocoahost で、ウィンドウを開いたまま 8 秒待つ C++ UI ドライバ ("sleep 8000;close"、640×360×8) を使い、待ちの途中の 4 秒間に `ps -o time` を 2 回取った。
  - ModalWait (ON): **4 秒で 4.010 秒の CPU** (1 コアを使い切る)
  - 対照 (`-DMV_MODAL_WAIT=OFF` でビルドしたものに差し替え): **0.000 秒**
- 観測 2 (`sample`、round4/logs/sample_modal.txt):
  - 主スレッドの 767 サンプルのうち 757 が `ModalWait::pump → nextEventMatchingMask → _DPSNextEvent → RunCurrentEventLoopInMode → _CFRunLoopRunSpecificWithOptions` の中にあった。
  - その先端は `__CFRunLoopCopyMode → CFSetGetValue / CFEqual / __CFStringEqual` などで、CPU の上で動いている。
  - 主スレッドには `mach_msg2_trap` (カーネルでの待ち) が 1 つもない。
- 推定 (機構): 独自のモード "MeatVideoOFXModal" にはランループのソースもタイマーも登録されていない。空のモードでの CFRunLoopRunInMode はすぐに戻る (kCFRunLoopRunFinished) ので、`untilDate:` の 20 ms は効かない。結果として `poll(fd, 0)` → `nextEventMatchingMask` の往復が休みなく回る。
  - それでもイベントは取り出せている (待ち中に送った入力が後で配られなかった)。_DPSNextEvent はブロックする前にイベントキューを直接見るためとみられる。
- 影響: ウィンドウを開いている間ずっと、ホストの主スレッドが 1 コアを占有する (電池・発熱)。ヘルパーの UI や Auto Profile と CPU を取り合う。
- 作業者の判定条件の問題: 「pump のサンプル > 80%、read での停止 0」は、ブロックして待っている状態と空回りしている状態を区別しない。
  CPU 時間か、主スレッドの先端が mach_msg で待っている割合を条件に加えるべき。

### R4-2 (High、観測): Cocoa ホストでウィンドウが開くまでが遅い (1080p で約 16 倍)
- 観測 (round4/scripts/timing.py。cocoahost、C++ UI ドライバ、script "close"、1920×1080×11 フレーム、Prepare@5。待ち = press の wall の区間):
  - 対照 (OFF): 0.67 秒 / 0.67 秒 (2 回)
  - ModalWait (ON): **10.53 秒 / 10.50 秒**
  - minihost (NSApp なし、blocking) は 0.69〜0.71 秒、cocoahost (ON) は 9.22〜11.12 秒
  - 640×360×8 では minihost 0.39 秒に対し cocoahost (ON) 1.43〜1.55 秒
  - すぐに読み捨てる偽ヘルパー (drain) でも、1080p×11 で 0.13 秒 → 2.86 秒
- 推定: フレーム (1080p×11 = 約 274 MB) を非ブロッキングのパイプに書く際、パイプが一杯になるたびに poll(0) + nextEventMatchingMask を 1 往復してから書き直す。1 往復が重い (ms 単位) ので、書き込みの速さがそれに律速される (約 26 MB/s)。
  4K なら 4 倍のデータで 40 秒前後になるとみられる。
- 影響: Resolve でボタンを押してからウィンドウが出るまで、1080p で約 10 秒待たされる (Resolve は応答するが、画面は止まったまま)。
- 提案 (R4-1 と一緒に):
  - fd で待ち、イベントは待たずに取る: `poll(fd, events, 5〜10 ms)` で fd を待ち、その後 `untilDate:[NSDate distantPast]` で取れるだけイベントを取る。
  - または、独自モードにパイプの CFFileDescriptor (か mach port のソース) を登録して、モードを空にしない (fd が準備でき次第起きる)。
  - 送信は従来どおり大きな塊で書き、パイプが一杯のときだけ上の待ちに入る。
  - 判定には CPU 時間 (待ち中に < 5% など) と、ウィンドウが開くまでの時間の対照比 (1080p×11 で OFF の 1.5 倍以内など) を加える。

### R4-3 (Medium、推定・Resolve で要確認): 入力を捨てることの副作用
- マウスの押下と離しの片方だけを捨てる場合: ホストのボタンが押下 (mouseDown) で InstanceChanged を出す作りなら、対になる mouseUp は待ちの間に捨てられる。ホストの Qt は押されたままだと思い、閉じた後のマウス移動がドラッグになるおそれがある (Resolve の作りは未確認)。
  Qt の QPushButton は離しで clicked を出すので、通常は起きないとみられる。
- 修飾キー: 待ちの間に捨てた flagsChanged・keyUp のために、閉じた後にホストが修飾キーやキーが押されたままと認識するおそれがある (推定)。
- 本家は同じプロセス内で、Qt のモーダル (beginModalSessionForWindow / runModalSession) を使っているとみられる (作業者の静的な観測)。AppKit のモーダルセッションは、ほかのウィンドウ宛ての入力を自分で処理する。
- Resolve で、ボタン → ウィンドウ → 閉じる の後に、クリックや修飾キーの取り違えがないかを確認項目に加えるとよい。

### R4-4 (Low、推定): 待ち中の再入
- AppKitDefined の sendEvent: から、ホストのアクティブ化やウィンドウのハンドラが同期で走る。そこからホストがこのインスタンスの DestroyInstance を呼ぶと、待ちの後に解放済みの `Instance*` を使う。
  通常は入力が捨てられホストのタイマーも動かないので、経路は見つかっていない。
  - 2 つ目のウィンドウは g_window_open で防がれる (作業者の観測)。
  - g_window_open は RAII のデストラクタで、例外を含むすべての出口で戻る (コードで確認)。

## 確認できた点 (コードレビューと観測)
- objc_msgSend の型 (コードで確認): `nextEventMatchingMask:untilDate:inMode:dequeue:` (unsigned long long, id, id, BOOL)、`type` (NSUInteger)、`subtype` (short = NSEventSubtype)、`activateWithOptions:` (NSUInteger → BOOL)、`runningApplicationWithProcessIdentifier:` (pid_t)、`dateWithTimeIntervalSinceNow:` (double) は正しい。
  `yieldActivationToApplication:` は macOS 14 以降のセレクタで、バンドルの最低 OS は 26.0 (otool の minos、Info.plist) なので問題ない。
  autorelease pool は pump ごとに push/pop していて、front() もその中。
- wait_fd: POLLHUP / POLLERR / POLLNVAL では poll が r > 0 を返して抜け、続く read / write の失敗でループが終わる (コードで確認)。空回りの原因は R4-1 で、fd 側ではない。
- 非 active の経路は従来と同じ (コードで確認): wait_fd はすぐ戻り、EAGAIN の扱いは active のときだけ、fd はブロッキング、wait_child は EINTR ループの waitpid。
  minihost と MV_MODAL_WAIT=OFF の対照は 0.67〜0.71 秒・CPU 0 で、Round 3 と同じ振る舞い。
- wait_child: WNOHANG で 0 → pump、ECHILD → status 0 (従来どおり)。ゾンビなし (作業者の helper-dies / no-reply-eof の場面と同じ結果)。
- 作業者の verify_modal.py を自分の対照バンドル (OFF に差し替えて再署名) で実行: ALL OK。
  - apply-10s: 待ち中の入力は待ちの後も配られず、対照では read で 100% 停止して入力が後で配られた。
  - 別スレッドからの押下は "already open" で拒否。minihost は blocking。
  - ただし sample の「pump 815/846」などは、R4-1 の空回りのサンプルを含む数え方。
- 「応答あり」の判定基準: 「主スレッドがイベントを取り出し、送ったイベントが溜まらない」は、WindowServer の判定 (キューのイベントが一定時間取り出されない) の代わりとして妥当とみる (推定)。
  CGSEventIsAppUnresponsive が SIGSTOP でも 0 のままなのは作業者の観測どおりで、合成イベントでは立たないとみられる。
  ホストを前面にして実際の入力を使う確認は、ユーザーのフォーカスとカーソルを奪うので行っていない。
  前面化 (front) の動作は、試験のヘルパーが本物の app でないため観測していない。

## 回帰 (今回のソースで新規にビルド)
| 確認 | 結果 | 出力 |
|---|---|---|
| クリーンビルド (build_ofx.sh、新規ディレクトリ。cocoahost を含む) | pass | 警告・エラー 0。MeatVideo.ofx・minihost・cocoahost・fsguard は worktree のものと sha256 一致。依存は libnvcore・libobjc・libc++・libSystem |
| qtui 全ターゲット | pass | 警告 0 |
| verify_ofx.py --pure --mutate | pass | 163 比較 0/170,596,864 語不一致、ALL MATCH。fd を閉じた起動 8 通りすべて ok、dlopen_noload 0 |
| 自作の場面 (my_scen.py --pure) | pass | ALL MATCH |
| qtui_selftest | pass | 117/117 |
| 既存の --ofx モード (HEAD vs 今回) | pass | 29 件 ALL SAME |
| verify_modal.py --control (自分の対照) | pass (判定条件は不十分、R4-1) | ALL OK |
| ~/.config/NeatRepro/nv_ui_qt.ini | pass | 45d4258c…fcc6 (前後で同一) |

## APPROVE の条件
1. R4-1: 待ち中のホストの CPU がほぼ 0 になる (例: 上の 4 秒の測定で < 0.2 秒)。
2. R4-2: Cocoa ホストでウィンドウが開くまでの時間が対照並みになる (1080p×11 で 1 秒前後)。
3. その上で、応答性の観測 (sample で主スレッドが mach_msg で待っていること、入力が溜まらないこと) が保たれる。

---

# Round 5 (2026-11-01、R4-1〜R4-4 の修正後の再検証)

## 総合判定: **APPROVE**
Round 4 で示した APPROVE の条件 3 つを、今のソースから新規にビルドしたもので自分で測り、すべて満たした。
- 待ち中の CPU はほぼ 0。
- 1080p×11 でウィンドウが開くまでの時間は対照並み。
- 主スレッドはカーネルで眠りながらイベントを取り、入力は溜まらない。

新しい設計 (poll で待ち、合間に distantPast でイベントを取る / 送信はワーカースレッド / 押下と離しの対の管理 / DestroyInstance の後回し) に、差し戻すほどの問題は見つからなかった。回帰もすべて合格。残るのは Low・情報の 4 件。

## APPROVE の条件の確認 (観測)
| 条件 | 結果 | 試験 | 出力 |
|---|---|---|---|
| 1. 待ち中の CPU < 0.2 s / 4 s | pass | round5/scripts/cpu.py: cocoahost、C++ UI ドライバ "sleep 9000;close"、640×360×8。待ちの中で 4 秒あけて `ps -o time` を 2 回 | 修正版 **0.020 s / 0.020 s** (2 回)。対照 (今のソースを -DMV_MODAL_WAIT=OFF でビルドし、バンドルの写しで再署名) 0.010 s。Round 4 は 4.010 s |
| 2. 開くまでの時間が対照並み | pass | round5/scripts/measure.py: cocoahost、1080p×11 (test_v02 n=100..110)、Prepare@5、script "close"。対照と交互に 3 回ずつ | 対照 0.661 / 0.667 / 0.656 s、修正版 **0.638 / 0.633 / 0.630 s**。Round 4 は 10.5 s。作業者の verify_modal でも、私の対照で modal 0.620/0.628 s 対 対照 0.649/0.651 s (比 0.97)、drain stub で比 1.10 |
| 3. カーネルで眠り、イベントを取る | pass | 同じ待ちを `sample` で 1 秒、主スレッドの先端を集計 | 修正版: 842 中 824 が `poll (in libsystem_kernel)`、18 が `ModalWait::drain`、read は 0。2 回目は 846 中 824 / drain 22。対照: 863/863 が read |
| 3'. 入力が溜まらない | pass | 作業者の verify_modal.py を私の対照で実行 (ALL OK) と、自作の input2 | apply-10s で待ち中に送った入力は、待ち中にも待ちの後 1 s にも配られない。対照では溜まって後で配られた |

**判定条件が空回りを見分けられるか (陰性対照)**: verify_modal.py の今の判定を、Round 4 の (空回りする) バンドルに向けて実行した (`V.BUNDLE` を差し替え)。
window-closed の結果は **FAIL**: CPU 3.970 s / 4.05 s、カーネルで眠っていたのは 40/846、drain 0。
新しい条件 (CPU < 0.2 s、カーネル ≥ 80%、drain > 0) は、空回りと本当の待ちを区別できる。

## 自作の敵対的な試験 (round5/scripts/adv.py、cocoahost で ModalWait が有効)
- **input2 (R4-3 の対の管理)**: `--held` (左ボタンとキー 's' を待ちの前に押下) のうえで、待ち中に次を送った。
  - 's' の自動リピートの KeyDown ×2 (kCGKeyboardEventAutorepeat = 1)
  - 右ボタンの押下と離し
  - Shift の flagsChanged を押下・離しの 2 回
  - 待ちの前に押した左ボタンの離しと 's' の離し
  - 新しい左ボタンの押下 (離しは待ちの後)
  - キー 7 の押下・リピート・離し
  
  待ち中にホストへ配られたのは **[FlagsChanged, FlagsChanged, LeftMouseUp, KeyUp]** だけ (期待どおり)。リピート、待ち中の押下とその離し、右ボタンの対はすべて捨てられた。CPU 0.030 s、カーネルで眠っていたのは 824/843。
- **stall (ヘルパーが要求の途中で 6 秒読むのを止める)**: 1080p×11 で、3 MB 読んだあと 6 秒止まり、その後に読み切って CANCEL を返す偽ヘルパー。
  ワーカースレッドは write で止まり、主スレッドはカーネルで眠りながらイベントを取り続けた (821/838、CPU 0.020 s / 4 s)。最後に status 0・不変で戻った。
- **middie (ヘルパーが要求の途中で終了)**: 3 MB 読んで exit 0。
  0.09 s で status 1 "unexpected reply (0 bytes)"・不変。ワーカーは EPIPE で抜け、join されて、ホストは rc 0 で終了した。

## コードレビュー
- run(): ワーカーが触るのは pipe の write と close だけ。終わりは std::atomic の done で知り、join してから戻るので、ワーカーが呼び出しより長く生きることはない。
  フレームは呼び出し元の SessionRequest にあり、join の後で消える。f (write_all) は例外を投げない。
- 非 active の経路は従来どおり: run は f をその場で実行し、wait_fd はすぐ戻り、read はブロッキング、wait_child は EINTR ループの waitpid。
  write_all は backup_r3 と同じ (差分なし)。O_NONBLOCK と EAGAIN の扱いは削除済み。
  minihost (NSApp なし) と対照は "blocking" で、照合と fd を閉じた 8 通りは従来とバイト同一。
- 押下と離しの管理: buttonNumber (0〜31) と keyCode の集合で持つ。待ちの前の押下の離しと FlagsChanged は sendEvent: で渡す。リピートは捨て、集合にも入れない。
- 解放の後回し:
  - 窓が開いている間の DestroyInstance は、destroyed を立て、インスタンスのデータを nullptr にして返る。
  - open_window は待ちが終わると、ホストの API を呼ばずに delete して返る。instance_changed も、その後にインスタンスやハンドルを使わない (コードで確認)。
  - 2 回目の DestroyInstance は nullptr を受けるので、二重には解放しない。
  - 解放の後の別スレッドの Render は、instance_of が "no instance data" で失敗する (解放済みのメモリは触らない)。
  - 作業者の destroy の場面 (別スレッドから、MallocScribble 付き) は ok。

## 指摘 (すべて Low・情報、差し戻しの理由ではない)
- **R5-1 (Low、コードレビュー)**: run_session が例外で抜けると、in->window が true のまま残る。その後の DestroyInstance は解放を後に回したまま、どこでも解放されない (インスタンス 1 つ分のメモリ漏れ)。
  同じく、run() の std::thread の生成が失敗した場合 (std::system_error)、in[1] と out[0] の fd が閉じられず、ヘルパーも回収されない。どちらも資源が尽きたときにしか起きない。
  in->window の後始末を RAII にすればよい。
- **R5-2 (情報)**: 待ちの前に押したキーのリピートでない KeyDown が待ち中に来ると、その離しも捨てられる。実際のキーボードはリピート扱いで送るので起きないとみられる (推定)。
- **R5-3 (情報、作業者の記載どおり)**: 待ち中に押したボタンを窓が閉じた後に離すと、ホストには対応のない離しが 1 つ届く。
- **R5-4 (情報、未観測)**: front() の前面化 (yield → activate) は、試験のヘルパーが本物の app でないため観測していない。ホストを前面にして実際の入力を使う確認も、ユーザーのフォーカスを奪うので行っていない。Resolve での確認項目に残す。

## 回帰 (今回のソースで新規にビルド)
| 確認 | 結果 | 出力 |
|---|---|---|
| クリーンビルド (build_ofx.sh、新規ディレクトリ。cocoahost を含む) | pass | 警告・エラー 0。MeatVideo.ofx・minihost・cocoahost・fsguard は worktree の ofx/build と sha256 一致。codesign --deep --strict 有効 |
| qtui 全ターゲット | pass | 警告 0 |
| 対照 (MV_MODAL_WAIT=OFF) のビルド | pass | 警告 0。修正版の Mach-O とは異なる |
| verify_ofx.py --pure --mutate | pass | **163 比較 0/170,596,864 語不一致、ALL MATCH**。fd を閉じた起動 8 通りすべて ok、dlopen_noload 0 |
| verify_modal.py --control (私の対照) | pass | ALL OK (apply-10s・対照・helper-dies・no-reply-eof・window-closed・held・destroy・開く時間 2 種・minihost) |
| 自作の場面 (my_scen.py --pure) | pass | 36 比較 0/59,616,000 |
| qtui_selftest | pass | 117/117 |
| 既存の --ofx モード (HEAD vs 今回) | pass | 29 件 ALL SAME |
| ~/.config/NeatRepro/nv_ui_qt.ini | pass | 45d4258c…fcc6 (前後で同一) |

## Resolve での確認 (ユーザー、作業者の手順に加えて)
- `MEATVIDEO_OFX_TRACE=1` で "host event loop" の経路を通ること (主スレッドで呼ばれていること)
- ウィンドウ表示中の Resolve の CPU と「応答なし」
- 前面化の振る舞い
- 閉じた後のクリック・修飾キー
