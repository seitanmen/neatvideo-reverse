# N4d REPORT — 本家どおりの 2 ボタン + ウィンドウ、既定プロファイル・既定プリセット (作業者 n4d-worker)

転記: 統括 (2026-11-01)。作業者の最終報告を以下にそのまま転記した (作業者は REPORT を書けないため)。
仕様は `.claude/tasks/native_n4_ofx.md` の「N4d」「N4d 追加」「N4d 追加 2」。統括の再確認と独立検証の結果は末尾に追記する。

---

N4d 最終報告（全文）。追加 2 件（既定プロファイル「N4d 追加」・既定プリセット「N4d 追加 2」）は受領し、実装と試験まで済んでいます。最終照合 `caffeinate -i python3 ofx/tools/verify_ofx.py --pure --mutate` は 150 比較 0/158,846,464 語不一致で ALL MATCH。C ABI は無変更（NVC_ABI_VERSION 3）。コミット・/Library/OFX/Plugins・Resolve の操作はしていません。Auto Match (N4e) は未実装です（qtui の Prepare ページは触っていないので、後からボタンを足すことは妨げません）。

## 変更したファイル
承認済み N4b は work/omo/wp_native_ofx/N4d/backup_n4b/ofx/ に退避（SHA256SUMS.txt 付き）。

ofx/
- src/plugin.cpp（全面改訂）: 本家のページ "Controls" だけを定義（Prepare Profile... / Adjust Filters...（無効で定義）/ ClipParams（秘密の Custom）/ ParamsHash1..3）。18 スライダーと Rebuild は削除。ボタンで現在のフレームと前後最大 5 フレーム（連続する範囲）を clipGetImage で取ってウィンドウを開く。レンダーは ClipParams だけを読み、プロファイルが無ければ素通し＋メッセージ。新規インスタンスの既定と SyncPrivateData に対応
- src/params.{h,cpp}: ClipParams の解析と v5 ブロックへの写像（matrix "auto" を含む）
- src/core.{h,cpp}: プロファイル作成を削除。ClipParams の状態で temporal + finish
- src/session.{h,cpp}（新規）: ヘルパーの起動（--ofx-session / --ofx-default）
- src/ref_floor.inc: 削除（退避先に残る）
- CMakeLists.txt（qtui/src/core/pyobj.cpp を組み込む）、build_ofx.sh（ヘルパー同梱）
- tools/minihost.cpp: --press・--clip-params・--init-clip-params・--env・--sync・--clip-out、paramSetValue、本家と同じ定義かの検査を追加。--check-floor は削除
- tools/fsguard.c: posix_spawn の第 1 引数と、RTLD_NOLOAD 付き dlopen（dlopen_noload）を区別して記録
- tools/verify_ofx.py: N4d 用に書き直し

qtui/（INT-1 と衝突しうるもの）
- src/app/mainwindow.{h,cpp}: set_ofx_frames に既定値付きの引数 mat="bt709", full=false を足しただけ（差分 10 行・6 行）。既存の呼び出しは不変
- src/main.cpp: --ofx-session の分岐と、QApplication を作る前に処理する --ofx-default の分岐
- CMakeLists.txt: ターゲット qtui_ofx_session_drv
- 新規: src/app/ofx_session.{h,cpp}、tests/ofx_session_drv.cpp
- selftest.cpp は触っていない

## 設計
- ClipParams = JSON {"format":"meatvideo_ofx_clip_params","version":1,"frame_size":[W,H]|null,"preset":<UI のプリセット>,"profile":<UI のプロファイル>|null,"render":{"matrix":"bt709|bt601|bt2020|auto","full_range":true,"P":backend_params,"profile":effective}|null}。preset と profile はウィンドウの状態を戻す用で、プラグインは render だけを読む。写像は qtui nvcore.cpp の v5_blocks / uniform / channel_params / t_noise と同じ。数値は "C" ロケールで解析（uselocale）
- ParamsHash1..3: ClipParams の FNV-1a 64 の下位・上位 31 ビットと長さ（本家での意味は推定）
- --ofx-session: 要求 = JSON 1 行（page・width・height・fps・matrix・times・current・clip_params）＋ gbrpf32le のフレーム列。応答 = "APPLY <n>\n"＋ClipParams か "CANCEL\n"。それ以外の応答・0 以外の終了・シグナル・レンダーで使えない応答は「変更なし」＋メッセージ。ヘルパーは fd 1 を stderr に付け替えるので応答は汚れない
- --ofx-default（画面なし。QApplication を作らず QCoreApplication で設定だけ読む）: 応答 = "DEFAULTS <n> <none|ok|error> <none|ok|error>\n"＋ClipParams（matrix "auto"、frame_size null。既定がどちらも無ければ空）
- 起動: posix_spawn。DYLD_* と Qt のパス系の環境変数は渡さない。POSIX_SPAWN_CLOEXEC_DEFAULT、SIGPIPE 抑止
- ヘルパーの場所: dladdr で得たプラグイン自身のパスから Contents/Helpers/MeatVideoQt.app/Contents/MacOS/MeatVideoQt。macdeployqt で Qt を同梱し、libnvcore・offscreen プラグイン・qt.conf を足して ad hoc 署名（50 MB）。qt.conf が無いと、ビルド機の ~/Qt のプラグインが先に読まれ得る。試験では MEATVIDEO_OFX_HELPER で差し替え
- 既定プロファイル・既定プリセット:
  - ClipParams が空のインスタンスだけ、CreateInstance で --ofx-default を一度起動する（Render の外）。結果はメモリ（pending）に持ち、レンダーと GetFramesNeeded はその時点から使う
  - ClipParams への保存は、最初の InstanceChanged か SyncPrivateData のとき。ofxParam.h は paramSetValue を InstanceChanged と interact でしか許していないため
  - 保存済みの ClipParams は置き換えない。既定を変えても既存のインスタンスは不変
  - 読めない既定は本家の文言 "Selected default profile cannot be loaded." / "Selected default preset cannot be loaded." を出し、読めない側だけ入れない。既定が無ければ何もしない（素通し）。ヘルパーに問い合わせられないときは "the Default Profile and Default Preset could not be read (...)"
  - matrix "auto" は高さで決める（720 行以上 BT.709、未満 BT.601。ウィンドウの規則と同じ）
  - ウィンドウは ClipParams が空なら既定を読み込んで開く（既定プリセットは MainWindow の構築時に入る）。窓内の Reset Profile / Reset Filter Settings は既定に戻す。単体の qtui の動作は変えていない

## 検証（観測）
- 全体照合（logs/verify_full_pure_mutate2.txt）: 150 比較 0/158,846,464 語不一致。内訳 ブリッジ経路 70・純 Python 70・qtui 自身の処理 10。uint32 比較で NaN のビット列も対象
- ウィンドウ経由の場面: Prepare → Adjust → Cancel / close では ClipParams とハッシュは不変。時刻オフセットと負の rowBytes、プロファイル無しの Apply、フレーム欠け、HDR/NaN、1080p。Adjust の有効/無効はプロファイルの有無と一致し、無効中に押してもヘルパーは起動しない。素通しは入力と同一でメッセージ付き。窓が返した ClipParams は Python 側の検査で render.P == backend_params(params_from_json(preset))、Auto Profile の結果（AFT 前）== Python の build_profile
- 合成 ClipParams の場面（Python の UI 関数で作成）: 各パラメータ、チャンネル別の設定（uniform でない経路）、t_noise、Mix、bt2020/709/601、pyramid、tune、HDR、欠け、範囲変更、再入、半径 5
- 既定の 3 通り:
  - 既定あり（D-set）: 新規インスタンスがすぐ既定で処理し、参照とも、同じ状態を読み込んだ qtui の処理ともビット一致。SyncPrivateData で既定がそのまま保存された
  - 既定なし（D-none）: 何も保存されず素通し
  - 壊れた値（両方 D-broken / プロファイルだけ D-badprof / プリセットだけ D-badpre）: 本家の文言が出て、読めない側だけ入らない
  - D-window: 空の ClipParams のままウィンドウを開くと既定が読み込まれ、窓内のリセットでも既定に戻る。qtui の処理とプラグインのレンダーが一致（3 件）。既定を壊れた値に変えても既存の ClipParams は不変
  - D-existing: 保存済みの ClipParams を持つインスタンスは、既定を問い合わせない（spawn 0）
- fsguard（ファイル・プロセス呼び出しの監視）:
  - レンダーだけの場面: ホストによるバンドルの dlopen 1 件のみ
  - それ以外に記録されたのは、新規インスタンスごとの posix_spawn(... --ofx-default) と、ボタン押下ごとの posix_spawn(... --ofx-session) だけ
  - 例外 1 件: ヘルパーが JSON でない応答を返す経路で dlopen_noload(/usr/lib/libstdc++.6.dylib) が 1 件出る。PyObj::parse が投げた std::runtime_error の破棄で libc++abi の __refstring_imp が RTLD_NOLOAD で探すもの（バックトレースで確認）。読み込み済みイメージを探すだけでファイルは開かない。照合ではこの経路でだけ許可
  - 陽性対照: minihost --fsguard-selftest で 5 件記録
- プラグインの import: ファイル系は無く、read/write/close/pipe/fcntl（パイプ用）、posix_spawn 系、waitpid、dladdr、getenv だけ
- 拒否 7 種（インターレース 2・64 未満・8bit・16bit・現在フレームの欠け・読めない ClipParams）: すべて status 1 ＋メッセージ
- ヘルパーの失敗 6 種（存在しない・不正な応答・使えない設定・終了 3・入力を読まずに終了・途中で切れた応答）: すべて ClipParams 不変・メッセージ付き。ホストは落ちない（SIGPIPE なし）
- 同梱の実ヘルパー: offscreen で起動。--ofx-default で既定が入り、窓を SIGTERM で終わらせても既定は残る。DYLD_PRINT_LIBRARIES で Qt はすべてバンドル内から読まれていた
- 変異 0〜11（すべて期待どおり）:
  0. 無変更 → 0/5 で差なし（対照）
  1. threshold 4.0001 → 5/5 で差
  2. curve を float32 で 1 ulp → 5/5 で差
  3. Cr の high 帯域（チャンネル別経路）→ 5/5 で差
  4. 参照入力の 1 ulp → 1/5（5 語）で差
  5. preset と P の不整合 → 検査が失敗
  6. 別フレームのプロファイルとの比較 → 検査が失敗
  7. 実効プロファイルの area 改変 → 検査が失敗
  8. qtui 出力と別フレームの比較 → 691,200 語の差
  9. 既定の取り違え（set を badpre として検査）→ 検査が失敗
  10. 既定の tune 改変 → 検査が失敗
  11. 既定の render.matrix を auto → bt709 → 5/5 で差
- 既存の検査:
  - qtui_selftest: 117 passed / 0 failed（追加実装の後に再実行、logs/qtui_selftest2.txt）
  - 既存の --ofx モード: HEAD のソースと今回のソースでビルドしたものを深度 8/16/32 × 6 スクリプトで比べ 18/18 バイト同一（追加の後に再実行、logs/ofx_mode_regression2.txt）
  - make -C native verify: PASS（native は無変更。logs/native_verify.txt）
  - クリーンビルド: バンドル（build_ofx.sh を scratchpad の新しいディレクトリへ）と qtui --clean-first で警告 0・エラー 0。フラグに -ffp-contract=off -fno-fast-math を確認
  - ~/.config/NeatRepro/nv_ui_qt.ini: どの実行の前後でも sha256 が同じ（試験は HOME と NV_UI_QT_SETTINGS を作業ディレクトリに向ける。設定ファイルは PySide6 の QSettings で書いた）
- 補足: 最初の照合が 19,109 秒かかったのは、00:34〜05:51 に電池残量 1% で Mac がスリープしたため（pmset -g log で確認）。以降の長いコマンドは caffeinate -i 付き

## 未対応・推定
- ボタン押下からウィンドウを閉じるまで InstanceChanged が戻らないので、その間 Resolve の UI は止まると見ている（推定）。本家は同じプロセス内のモーダルウィンドウ
- 新規インスタンスの作成は --ofx-default の起動のぶん待たされる（試験では 1 回の実行全体で約 0.3 秒）
- ホストが InstanceChanged も SyncPrivateData も呼ばないまま保存すると、開き直したときに既定を再び問い合わせる。その間に既定が変わっていれば別の既定が入る（推定のリスク）
- 未試験・未対応: プロキシ解像度の画像、インターレース、1 プロセスでの複数インスタンスの同時レンダー、x86_64 版、Windows 版、Resolve での実機確認。ウィンドウに渡すのは前後最大 5 フレームの連続範囲だけ
- ParamsHash の意味は推定
- qtui/tools/compare_ofx.py は再編前の repro/ パスのままで動かない（代わりに上の改変前後のバイト比較をした）
- 本番のヘルパーはユーザーの ini を共有する（Auto Fine-Tune などの設定が効き、ui/advanced は従来どおり書き込む）
- レンダーは ClipParams の matrix を使い、フレームサイズが作成時と違っても拒否しない

## Resolve での確認手順（ユーザーが行う）
1. /Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/build/MeatVideo.ofx.bundle を sudo cp -R で /Library/OFX/Plugins/ にコピーし、Resolve を再起動
2. ノードに "Meat Video Denoise" を適用。ボタンが 2 つだけで Adjust が無効になっていること、既定が無ければ素通し＋メッセージになることを確かめる
3. Prepare Noise Profile を押すと MeatVideoQt のウィンドウが別アプリとして開く。Auto Profile → Apply で Adjust が有効になり、ノイズが減ることを確かめる
4. Adjust で設定を変えて Apply すると結果が変わり、Cancel や窓を閉じたときは何も変わらないことを確かめる
5. ウィンドウの Profile > Assign As Default Profile と Filter > Assign As Default Preset で既定を設定し、新しいノードが既定で処理されること、既存のノードが変わらないことを確かめる
6. プロジェクトを保存して開き直し、設定が残っていることを確かめる
7. ウィンドウが Resolve の背面に出る場合は知らせてほしい（posix_spawn で起動した窓が前面に来るかは未確認）

経過は work/omo/wp_native_ofx/N4d/PROGRESS.md、ログは同じ場所の logs/。最新の結果は verify_full_pure_mutate2.txt・qtui_selftest2.txt・ofx_mode_regression2.txt・build_clean2.txt・qtui_build_clean2.txt。dbg*.txt は修正前の途中経過です。


---

## 補足: 作業者の報告の詳細 (統括の求めで作業者が送り直した 2 通。そのまま転記)

N4d 報告の続き (1/2): 既定の 3 通り・fsguard・既存の検査・変異

## 既定の 3 通りと関連の場面 (logs/verify_full_pure_mutate2.txt)
- 既定あり (D-set): 新しいインスタンスはすぐ既定で処理した。その出力は参照 (ブリッジ経路・純 Python) とも、同じ状態を読み込んだ qtui の処理 (ドライバの process) ともビット一致。--ofx-default の応答は "DEFAULTS 12670 ok ok"。SyncPrivateData で、CreateInstance で受け取った既定がそのまま ClipParams に保存された (12670 バイト)。2 回目の sync では変わらない。作成直後から Adjust 有効。
- 既定なし (D-none): 応答は "DEFAULTS 0 none none"。何も保存されず素通し (入力と同一)。メッセージは "no noise profile yet" だけ。
- 壊れた値:
  - D-broken (両方): 応答 "DEFAULTS 0 error error"。CreateInstance で "Selected default profile cannot be loaded. Selected default preset cannot be loaded." が出る。何も保存されず素通し。Adjust 無効。
  - D-badprof (プロファイルだけ壊れている): 応答 "DEFAULTS 2068 error ok"。プロファイルの文言が出る。プリセットだけの ClipParams が保存され (render は null)、素通し。
  - D-badpre (プリセットだけ壊れている): 応答 "DEFAULTS 12669 ok error"。プリセットの文言が出る。プロファイルと工場出荷時の設定が入り、処理する。
- D-window:
  - 既定なしで作ったインスタンスで、後から既定を設定してから (NV_UI_QT_SETTINGS を差し替え) ウィンドウを開くと、既定を読み込んで開く。Apply の結果は既定のプロファイル (base・factors・tune・device) とプリセットと一致。
  - 窓内で "auto; amt_Y 変更; tune 1.4" のあと Reset Profile / Reset Filter Settings を押すと既定に戻る (Apply は同じテキストを保存した)。
  - 既定を壊れた値に差し替えても、既存の ClipParams は sync・Adjust の開き直しとも不変。
  - 壊れた既定の状態で窓内のリセットを押すと、プロファイル無し・工場出荷時の設定になる。
  - qtui の処理とプラグインのレンダーの一致は 3 件。
- D-existing: 保存済みの ClipParams を持つインスタンスは、既定が設定されていても問い合わせない (spawn 0) し、ClipParams も不変。
- ClipParams の Python 側の検査 (ウィンドウ分):
  - render.P == state.backend_params(params_from_json(preset))
  - 実効プロファイルは curve・sigma 以外のキーが base と同じ
  - "auto" で作ったときは、AFT 前のプロファイルが受け取ったフレームでの Python の backend.build_profile と一致
  - 既定の検査 3 件はすべて yes

## fsguard
- レンダーだけの場面 (S-* と D-existing、計 6 件): 記録はホストによるバンドルの dlopen 1 件だけ。
- それ以外の場面で記録されたのは、新規インスタンスごとの posix_spawn(<helper> --ofx-default) と、ボタン押下ごとの posix_spawn(<helper> --ofx-session) だけ。記録数ごとの件数は 2 件 ×4 場面、3 件 ×5、4 件 ×1、6 件 ×2。
- 例外 1 件 (ヘルパー失敗のうち「使えない設定」の経路だけ): dlopen_noload(/usr/lib/libstdc++.6.dylib)。
  - ヘルパーが JSON でない応答を返すと、PyObj::parse が std::runtime_error を投げる。その破棄のときに libc++abi の __refstring_imp::compute_gcc_empty_string_storage が dlopen(RTLD_NOLOAD) を呼ぶ (FSGUARD_BACKTRACE のバックトレースで確認)。
  - RTLD_NOLOAD は読み込み済みのイメージを探すだけで、ファイルを開かず読み込みもしない。fsguard は RTLD_NOLOAD のとき dlopen_noload と別名で記録するようにした。
  - 照合ではこの 1 経路だけでこれを許可している。
- 陽性対照 (minihost --fsguard-selftest): 5 件記録 (fopen ×2・open・stat・dlopen)。
- プラグインの import: ファイル系の関数は無い。あるのは read・write・close・pipe・fcntl (パイプ用)、posix_spawn 系、waitpid、dladdr、getenv、environ、newlocale・uselocale、strtod、sscanf・snprintf だけ。

## 拒否・失敗・同梱ヘルパー
- 拒否 7 種はすべて status 1 とメッセージ: インターレース (Upper・Lower)、64 未満、8bit、16bit、現在のフレームの欠け、読めない ClipParams (version 2)。
- ヘルパーの失敗 6 種: 存在しない・不正な応答・使えない設定・終了コード 3・入力を読まずに終了・途中で切れた応答。どれも ClipParams 不変・Adjust 無効・メッセージ付きで、ホストは落ちない (SIGPIPE なし)。CreateInstance での問い合わせの失敗は "the Default Profile and Default Preset could not be read (...)" と出し、素通しにする。
- 同梱の実ヘルパー (MEATVIDEO_OFX_HELPER 未設定):
  - バンドル内の MeatVideoQt を --ofx-default (画面なし) で起動すると、既定 ("set") が入る。
  - 続いて --ofx-session で窓が出る (offscreen)。その窓を SIGTERM で終わらせると "terminated by signal 15" と出て、ClipParams は既定のまま (応答と同じテキスト)。
  - fsguard の記録は [dlopen, posix_spawn(exe --ofx-default), posix_spawn(exe --ofx-session)]。
  - DYLD_PRINT_LIBRARIES では、Qt と libnvcore はすべてバンドル内から読まれた。

## 既存の検査
- qtui_selftest: 117 passed / 0 failed (追加の実装後に再実行、logs/qtui_selftest2.txt)。
- 既存の --ofx モード: HEAD のソースからビルドした qtui_ofx_drv と今回のものを、深度 8/16/32 × 6 スクリプトで比べ、18/18 がバイト同一 (stdout・終了コード・メッセージ。logs/ofx_mode_regression2.txt)。
- make -C native verify: exit 0、PASS (logs/native_verify.txt。native は無変更なので追加後は再実行していない)。LZMA の本家 blob の比較はキャッシュが無くスキップ (従来どおり)。
- クリーンビルド:
  - build_ofx.sh を scratchpad の新しいディレクトリへ: 警告 0・エラー 0 (logs/build_clean2.txt)。
  - qtui の --clean-first (全ターゲット): 警告 0 (logs/qtui_build_clean2.txt)。
  - コンパイルフラグに -ffp-contract=off -fno-fast-math が入っていることを確認。
  - エクスポートは 3 つだけ、rpath は 1 本。codesign --verify --strict --deep は valid。
- ~/.config/NeatRepro/nv_ui_qt.ini: sha256 45d4258c53337ee4aa60017a4a4cce72b9f4bf3dc71303406cfb5c1c7d80fcc6 で、照合・selftest・--ofx 比較のどの前後でも不変。試験は HOME と NV_UI_QT_SETTINGS を作業ディレクトリに向けている (既定の設定ファイルは PySide6 の QSettings で書いた)。

## 変異 (感度試験、すべて期待どおり)
- 0 無変更: 0/5 で差なし (対照)
- 1 render.P の threshold 4.0001: 5/5 で差 (1,034,870 語)
- 2 curve Y[3] を float32 で 1 ulp: 5/5 (194,111 語)
- 3 Cr の high 帯域 0.99 (チャンネル別の経路): 5/5 (2,294,656 語)
- 4 参照入力を 1 ulp: 1/5 (5 語)
- 5 preset の amt_Y と P の不整合: 検査が失敗する
- 6 別フレームの Python プロファイルとの比較: 検査が失敗する (area の差)
- 7 実効プロファイルの area 改変: 検査が失敗する
- 8 qtui の 4 フレーム目と 0 フレーム目のレンダーの比較: 691,200 語の差
- 9 既定 "set" を "badpre" として検査: 失敗する (t_radius・amt_Y・bamt_Cb_high・mix・mix_enable)
- 10 既定の tune 改変: 失敗する
- 11 既定の render.matrix を auto → bt709: 5/5 で差 (3,455,907 語)

(続きは 2/2: 未対応・推定と Resolve での確認手順)

N4d 報告の続き (2/2): 未対応・推定と Resolve での確認手順

## 未対応・推定
- ボタンを押してからウィンドウを閉じるまで、InstanceChanged は戻らない。その間 Resolve の UI は止まると見ている (推定)。本家は同じプロセス内のモーダルウィンドウ。
- 新規インスタンスの作成は --ofx-default の起動のぶん待たされる。試験では minihost の 1 回の実行全体で約 0.3 秒。
- 既定は CreateInstance で受け取ってメモリに持ち、最初の InstanceChanged か SyncPrivateData で保存する。ホストがどちらも呼ばずに保存すると、開き直したときに既定をもう一度問い合わせる。その間に既定が変わっていれば別の既定が入る (推定のリスク。Resolve の呼び出し順は未確認)。
- ウィンドウに渡すのは、前後最大 5 フレームの連続した範囲だけ。
- 未試験・未対応: プロキシ解像度の画像 (ボタン押下時にホストが縮小画像を渡す場合)、インターレース、1 プロセスでの複数インスタンスの同時レンダー、x86_64 版、Windows 版、Resolve での実機確認。
- レンダーは ClipParams の matrix を使い、フレームサイズが作成時と違っても拒否しない。
- ParamsHash1..3 の意味は推定 (FNV-1a 64 の下位・上位 31 ビットと長さ)。
- qtui/tools/compare_ofx.py は再編前の repro/ パスのままで動かない。代わりに、改変前後の --ofx モードのバイト比較 (18/18) をした。
- 本番のヘルパーはユーザーの ini (~/.config/NeatRepro/nv_ui_qt.ini) を共有する。Auto Fine-Tune・Profile mode などの設定が効き、ui/advanced は従来どおり書き込む。
- 窓内の Reset Profile を既定に戻す動作は、OFX のウィンドウ (--ofx-session) だけ。単体の qtui の Reset Profile は従来どおり空にする。
- Auto Match (N4e) は未実装。qtui の Prepare ページと ClipParams の形式には、後からボタンを足すのを妨げる変更はしていない。
- 推定 (未確認): qtui 全体の PyObj::parse は strtod を使うので、小数点がカンマのロケールでは数値を誤読し得る。OFX の経路 (プラグインとセッション) は uselocale("C") で回避した。

## Resolve での確認手順 (ユーザーが行う)
1. /Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/build/MeatVideo.ofx.bundle を sudo cp -R で /Library/OFX/Plugins/ にコピーし、Resolve を再起動する (作り直しは ofx/build_ofx.sh)。
2. ノードに "Meat Video Denoise" を適用する。インスペクタのボタンが Prepare Noise Profile / Adjust Filter Settings の 2 つだけで、Adjust が無効になっていることを確かめる。既定が無ければ素通しになり、メッセージが出る。
3. Prepare Noise Profile を押すと、MeatVideoQt のウィンドウが別アプリとして開く。Auto Profile → Apply で、Adjust が有効になりノイズが減ることを確かめる。
4. Adjust Filter Settings で設定を変えて Apply すると結果が変わり、Cancel や窓を閉じたときは何も変わらないことを確かめる。
5. ウィンドウの Profile > Assign As Default Profile と Filter > Assign As Default Preset で既定を設定する。新しくノードを作ると既定で処理されること、既存のノードは変わらないことを確かめる。
6. プロジェクトを保存して開き直し、設定が残っていることを確かめる。
7. ウィンドウが Resolve の背面に出る・開かない場合は知らせてほしい (posix_spawn で起動した GUI が前面に来るかは未確認。raise と activateWindow は呼んでいる)。

経過は work/omo/wp_native_ofx/N4d/PROGRESS.md、ログは同じ場所の logs/。最新の結果: verify_full_pure_mutate2.txt・qtui_selftest2.txt・ofx_mode_regression2.txt・build_clean2.txt・qtui_build_clean2.txt。dbg*.txt は修正前の途中経過。

---

## 統括の再確認 (2026-11-01、観測)
- `caffeinate -i python3 ofx/tools/verify_ofx.py --quick --pure --mutate --work <scratchpad>/n4d_lead/verify`: **139 比較、0/115,909,120 語不一致、ALL MATCH** (1080p・B クリップを除く短縮版、1 分 07 秒)。ヘルパー失敗 6 種はすべて ok (ClipParams 不変・メッセージ付き)、変異 1〜11 はすべて検出、~/.config/NeatRepro/nv_ui_qt.ini の sha256 は前後で同じ (45d4258c…fcc6)。
- 静的: エクスポートは `OfxGetPlugin` / `OfxGetNumberOfPlugins` / `FoundryOfxAllowPluginUnload` の 3 つだけ。`codesign -v --deep` 有効。`Contents/Helpers/MeatVideoQt.app` を同梱 (バンドル 50 MB)。プラグイン本体の import のうちファイル・プロセス系は posix_spawn 系だけ。バイナリはソースの最終更新より後にビルドされている。
- ログ: `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/n4d_lead/verify_quick.log`


---

## Round 1 の差し戻し (独立検証 REQUEST_CHANGES、M1 + L1〜L6、`verifier/VERDICT.md`) への作業者の修正報告 (そのまま転記)

N4d 修正報告: 独立検証の M1・L1・L3〜L6 をすべて直し、再照合はすべて合格しました。最終の全体照合 `caffeinate -i python3 ofx/tools/verify_ofx.py --pure --mutate` は 163 比較 0/170,596,864 語不一致、ALL MATCH (logs/verify_fix_full.txt)。C ABI は無変更 (NVC_ABI_VERSION 3)。コミット・Resolve・/Library は触っていません。backup_n4b はそのままです。

今回新たに触った qtui のファイル (INT-1 と衝突しうるもの): src/core/pyobj.{h,cpp}、src/core/nvcore.cpp (1 行)、CMakeLists.txt。

## M1 (差し戻しの理由): 入れ子の深い ClipParams でホストが落ちる
- 変更: PyObj::parse (qtui/src/core/pyobj.{h,cpp}。プラグインもヘルパーも同じパーサ) に入れ子の上限を入れた。
  - 上限は PyObj::MAX_DEPTH = 256 段 (配列とオブジェクトを数える)。超えると PyObj::ParseError ("JSON: nesting deeper than 256 levels at <位置>") を投げる。
  - プラグインはこれを status 1 + メッセージ、Adjust 無効、ClipParams 不変で扱う。
  - qtui では parse の呼び出し元 27 か所がすべて std::exception か `...` で受けるので、正常なファイルでの動作は変わらない。runtime_error で受けているのは export.cpp の 2 か所で、どちらも parse の例外ではないことを確認した。
- 試験: minihost に `--thread-stack KB` を追加した (セッション全体をそのスタックサイズのスレッドで動かす)。512 KB スレッドで、CreateInstance (保存済みの値) と、ホストが値を設定した後のレンダーの両方を試した。
  - 257 段 (567 バイト)・10 万段 (200,055 B)・"[" × 30 万 (300,000 B)・オブジェクト 10 万段 (600,001 B): すべて status 1 で "nesting deeper than 256 levels"。作成時のメッセージあり、Adjust 0、落ちない。
  - 上限ちょうど (256 段) は 128 KB スレッドでも読めて素通し。
  - 10 万段の ClipParams を持つインスタンスで Prepare を押すと、ヘルパーも上限で拒否し、既定から始まる。Auto Profile → Apply で正しい ClipParams に置き換わり、render は status 0。
  - 正規の場面を 512 KB スレッドで通し実行 (D-stack512: 既定・sync・Adjust・render): 参照と qtui の処理の両方とビット一致。

## L1: 同梱ヘルパーがビルド機のパスを持ち、チェックアウトのファイルを読む
- 変更:
  - qtui/CMakeLists.txt に option NV_EMBED_SOURCE_ROOT (既定 ON。単体の qtui と試験は従来どおり) を追加。OFF のとき NV_SOURCE_ROOT="" とし、nvcore.cpp は空なら候補に入れない (1 行)。
  - build_ofx.sh はヘルパーを `-DNV_EMBED_SOURCE_ROOT=OFF` でビルドし、data/ref_floor.npy を MeatVideoQt.app/Contents/Resources/data/ に入れる (reference_floor() が最初に見る場所)。
  - build_ofx.sh の最後に、バンドル内のどのファイルにもチェックアウトやビルドディレクトリのパスが無いことを grep -rlF で確かめ、あれば失敗させる。
- 試験 (verify の bundled_data):
  - grep の結果は 0 件。`strings MeatVideoQt` に "/Users/" は 0 件。
  - fsguard に FSGUARD_ALWAYS=1 (起動から終了まで記録し、終了時に stderr へ出す。連続する stdin の read は 1 行にまとめる) を追加した。
  - バンドルの qtui ツリー (OFF) で作ったドライバをヘルパー app の写しに入れ、Auto Profile を実行させた。開いた ref_floor は、写しの Contents/MacOS/../Resources/data/ref_floor.npy への access と fopen だけ。チェックアウトの data は 0 件。
  - 結果のプロファイルは、開発用ドライバ (チェックアウトの data/ref_floor.npy を読む) のものと完全に同一 (ビット一致を保つ)。
  - 感度 (変異 12): 開発用ドライバで同じ検査をすると qtui/../data/ref_floor.npy の access・fopen が検出される。

## L3: パイプの fd が CLOEXEC でない
- 変更 (session.cpp run_helper):
  - pipe() の直後に 4 本すべてへ FD_CLOEXEC を付ける。macOS には pipe2 が無いので、pipe と fcntl の間に隙間は残る。
  - 応答は自分の長さで区切られる ("CANCEL\n" / "APPLY n\n" + n / "DEFAULTS n …\n" + n) ので、揃った時点で読むのをやめるようにした。誰かが書き込み口を持ち続けても待たない。
- 試験: 偽ヘルパー "holder" (完全な応答のあと `(sleep 30 &)` が stdout の書き込み口を 30 秒持ち続ける。作成時の --ofx-default でも同じ) で、押下はすぐ戻り (計測 0.0〜0.4 秒)、status 0、変更なし。
- 未確認: CLOEXEC そのもの (多スレッドのホストが同時に fork する場合) は観測できないので、コードレビューの範囲。

## L4: paramEditEnd の取りこぼしと、保存失敗で既定を失う問題
- 変更 (plugin.cpp):
  - store_clip_params は RAII で、paramEditBegin の後のどの失敗でも paramEditEnd を呼ぶ。
  - commit_defaults は保存に成功するまで既定を消さない。失敗すると警告 "the defaults could not be stored in the instance yet (…); they are used and stored later" を出し、次の InstanceChanged / SyncPrivateData で再試行する。保存中にホストが呼ぶ入れ子の InstanceChanged で二重に保存しないよう、committing フラグを置いた。
- 試験 (D-failset):
  - minihost に `--fail-set N` (Custom / String への paramSetValue を N 回失敗させる) と、sync 行の `edit_depth` を追加した。
  - 1 回目の sync: 拒否され、ClipParams 空・edit_depth=0・警告あり。その間の render は保留中の既定を使い、参照とビット一致。
  - 2 回目の sync: 既定 12,670 バイトが保存され、render も一致。

## L5: dlopen_noload(libstdc++) が Render 経路でも起こり得る
- 変更: PyObj::parse の例外を ParseError (std::exception と std::string メンバー。libc++ の refstring を持たない) にした。プラグイン側の例外は mvofx::Error のまま。
- 試験:
  - JSON でない ClipParams ("{not json") を 512 KB スレッドで、CreateInstance と、ホストが設定した後のレンダーの両方に与えた。status 1 + "is not valid JSON"、fsguard はバンドルの dlopen 1 件だけ。
  - JSON でないヘルパー応答の経路 (以前は dlopen_noload が出た) も、追加の記録 0 件を必須にして合格。
  - 全体照合のログで dlopen_noload は 0 件。

## L6: 範囲外の値を黙って受ける・誤解を招く文言
- 変更 (params.cpp):
  - radius・bs・search は JSON の整数で、範囲は [0, 16]・[4, 256]・[0, 256]。
  - levels は整数 [0, 5]。
  - 実数のパラメータはすべて有限で 0〜1e6、mix は [0, 1]。t_noise も有限で 0〜1e6。
  - curve_centers・curve・band_ratio・tot_ratio は有限。
- 試験 (512 KB スレッド、作成時と設定後のレンダー。すべて status 1 で、作成時のメッセージにも同じ文言):
  - "bs":"16" → `P["bs"] must be an integer in [4, 256]`
  - search −50 → `P["search"] must be an integer in [0, 256]`
  - radius 2.5 → `P["radius"] must be an integer in [0, 16]`
  - threshold NaN → `P["threshold"] is not a finite number`
  - mix 1.5 → `P["mix"] = 1.500000 is outside [0.000000, 1.000000]`
- UI が書く正規の値は、全照合ですべて受理された。

## L2 (仕様どおり)
変更していません。Resolve での確認項目に「プロファイルを作らずに保存したノード (ClipParams が空) は、開き直したときに新規として既定が入る」を加えます。

## 再実行 (すべて今回の修正後)
- verify_ofx.py --pure --mutate (全場面): 163 比較 0/170,596,864 語不一致。
  - 内訳: ブリッジ経路 76・純 Python 76・qtui 自身の処理 11。
  - 拒否 17 種 ok (従来 7 + 新規 10)、ヘルパー失敗 6 種 ok、書き込み口を持ち続ける子プロセス ok、上限ちょうどの入れ子 ok、深い ClipParams でのウィンドウ ok、同梱ヘルパー ok、同梱データ ok。
  - sync の検査 11 件 ok、変異 13 件 (0〜12) すべて期待どおり。
  - 分割実行 (W 系 --pure --mutate: 54 比較、S/D 系 --pure: 109 比較) も ALL MATCH。
- fsguard の要約 (場面ごとの記録数):
  - レンダーだけの場面 6 件: バンドルの dlopen 1 件だけ。
  - それ以外 (2 件 ×5 場面、3 件 ×6、4 件 ×1、6 件 ×2): 中身は dlopen と、新規インスタンスごとの posix_spawn(<helper> --ofx-default)、押下ごとの posix_spawn(<helper> --ofx-session) だけ。
  - dlopen_noload は 0 件。
- qtui_selftest: 117 passed / 0 failed (logs/qtui_selftest3.txt)。
- 既存の --ofx モード: HEAD のソースからビルドしたものと今回のもので、深度 8/16/32 × 6 スクリプトの 18/18 がバイト同一 (logs/ofx_mode_regression3.txt。pyobj の変更後)。
- クリーンビルド: バンドル (build_ofx.sh を scratchpad の新しいディレクトリへ。パス検査込み) は警告 0・エラー 0 (logs/build_clean3.txt)。qtui --clean-first の全ターゲットは警告 0 (logs/qtui_build_clean3.txt)。
- ~/.config/NeatRepro/nv_ui_qt.ini: sha256 45d4258c53337ee4aa60017a4a4cce72b9f4bf3dc71303406cfb5c1c7d80fcc6 で、すべての実行の前後で不変。
- make -C native verify: native は無変更なので再実行していない (前回 PASS)。

経過は PROGRESS.md の末尾に追記しました。

## 統括の再確認 (修正後、2026-11-01、観測)
- `caffeinate -i python3 ofx/tools/verify_ofx.py --quick --pure --mutate --work <scratchpad>/n4d_lead/verify2`: **152 比較、0/127,659,520 語不一致、ALL MATCH** (1 分 17 秒)。
  512 KB スレッドで深い入れ子 4 種 (257 段・10 万段・"[" 30 万・オブジェクト 10 万段) はすべて status 1 + "nesting deeper than 256 levels"、fsguard はバンドルの dlopen 1 件だけ。256 段ちょうどは 128 KB スレッドで読めて素通し。D-failset・D-stack512 も一致。ログ中の dlopen_noload は 0 件。
- バンドル内のファイルに `/Users/seitan` を含むものは 0 件、同梱ヘルパーの strings に "/Users/" は 0 件、`Contents/Helpers/MeatVideoQt.app/Contents/Resources/data/ref_floor.npy` あり。
- ログ: `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/n4d_lead/verify_quick2.log`


---

## Round 2 の差し戻し (R2-1、`verifier/VERDICT.md` の Round 2) への作業者の修正報告 (そのまま転記)

N4d Round 2 修正報告: R2-1 (差し戻しの理由) と任意の Low 4 件をすべて直し、再実行した照合はすべて合格。最終の全体照合 `caffeinate -i python3 ofx/tools/verify_ofx.py --pure --mutate` は 163 比較で 0/170,596,864 語不一致、ALL MATCH (logs/verify_r2_full.txt)。C ABI は無変更。コミット・Resolve・/Library は触っていない。

## R2-1: ホストの fd 0 / 1 が閉じているとウィンドウが開かない
- 変更 (ofx/src/session.cpp run_helper)
  - pipe() の 4 本を、それぞれ fcntl(fd, F_DUPFD_CLOEXEC, 3) で 3 番以上の fd に移し、元の fd を閉じてから使う。
  - これで子の 0 / 1 への adddup2 は必ず別の fd からの複製になり、CLOEXEC は複製先に引き継がれない。
  - ホストの fd 2 が閉じていれば、addinherit_np(2) を付けない (閉じた fd に付けると posix_spawn が EBADF で失敗し得るため)。
- 変更 (qtui/src/app/ofx_session.cpp)
  - --ofx-session と --ofx-default は、fd 2 が閉じていると dup2(2, 1) が失敗して「cannot set up stdout」で終了していた。
  - fd 2 が閉じている場合は fd 1 と 2 を /dev/null に向けるようにした (ヘルパーのプロセス側の処理)。
- 試験 (verify の closed_fds。minihost に `--log FILE` を追加し、ログをファイルに書くようにした)
  - minihost を 4 通りで起動した: 全部開いた状態、fd 0 を閉じた状態、fd 1 を閉じた状態、fd 0〜2 を閉じた状態 (Python の preexec_fn で os.close)。
  - 手順はどれも同じ: 既定ありの新規インスタンス (--ofx-default) → render → Prepare (auto;apply) → render → Adjust (amt_Y 30・Mix 20;apply) → render ×2。
  - 4 通りとも rc 0。作成時に既定が入り Adjust 有効。押下 2 回とも status 0 で ClipParams は変更済み、render 4 回とも status 0。
  - ログの "host fd N: closed at start" が、閉じた fd と一致していた。
  - fsguard の記録は dlopen、posix_spawn(--ofx-default)、posix_spawn(--ofx-session) ×2 だけ。
  - **出力 (全レンダー) と ClipParams 2 件は、全部開いた起動とバイト単位で同一**。
- 陽性対照 (logs/r2_closed_fd_control.txt)
  - Round 2 のコード (pipe 直後の FD_CLOEXEC) に戻したプラグインを scratchpad でビルドし、同じ条件で試した。
  - fd 0 を閉じた場合は Prepare が status 1 になり、ヘルパーの stderr に "ofx-session: no request header on stdin" が出た (検証者の観測を再現)。
  - 修正版は同じ条件で status 0、ウィンドウが表示された。
  - 観測: fd 1 を閉じた場合は、Round 2 のコードでも動いた。カーネルの割り当てでは out[1] が 1 に来ず (in[0] が 1 になり、0 へ複製される)、推定とは違った。今回の試験は両方を含む。

## 任意の Low
- mix などの範囲外メッセージ
  - std::to_string をやめて py_float_repr にした。
  - 結果: `P["mix"] = 1.0000001 is outside [0.0, 1.0]`、`P["mix"] = 1.5 is outside [0.0, 1.0]`。2 件を拒否試験に加え、どちらも status 1 で文言が一致した。
- 古い「defaults could not be stored」警告
  - 後の保存が成功したら消す。defaults の読み込み時の注意 (note) があれば、それに戻す。
  - D-failset で、2 回目の sync 行に警告が残っていないことを照合の必須条件にした。11 件の sync 検査すべて ok。
- ClipParams だけ保存されて ParamsHash の保存が失敗した場合
  - commit_defaults は、ClipParams が保留中のテキストと同じなら、もう一度 store_clip_params を呼んで ParamsHash1..3 を揃える (別の値なら、従来どおり既定を捨てる)。
  - minihost の `--fail-set` は Custom / String にしか効かないので、この経路はコードを読んで確かめた範囲。
- 巨大な frame_size
  - 0〜1e6 の整数の組のときだけ読み、それ以外は無視する (情報用の値)。
  - `frame_size: [1e300, -1e300]` は受理され、render は status 0、Adjust 1。範囲外の float→int 変換はしない。

## 再実行 (すべて今回の修正後)
- verify_ofx.py --pure --mutate (全場面)
  - 163 比較、0/170,596,864 語不一致。内訳はブリッジ経路 76・純 Python 76・qtui 自身の処理 11。
  - 拒否 19 種 ok (前回の 17 に mix 1.0000001 と、受理される frame_size 1e300 を追加)。
  - ヘルパー失敗 6 種、パイプを持ち続ける子プロセス、上限ちょうどの入れ子、深い ClipParams でのウィンドウ、同梱ヘルパー、同梱データ: すべて ok。
  - fd を閉じた起動 4 通り: ok。sync 11 件: ok。変異 0〜12: すべて期待どおり。dlopen_noload: 0 件。
- fsguard の要約
  - レンダーだけの場面 6 件は、バンドルの dlopen 1 件だけ。
  - それ以外の場面 (記録 2 件 ×5、3 件 ×6、4 件 ×1、6 件 ×2) も、dlopen のほかは posix_spawn(<helper> --ofx-default) と posix_spawn(<helper> --ofx-session) だけ。
- qtui_selftest: 117 passed / 0 failed (logs/qtui_selftest4.txt)
- 既存の --ofx モード: HEAD のソースからビルドしたものと今回のものを比べ、深度 8/16/32 × 6 スクリプトの 18/18 がバイト同一 (logs/ofx_mode_regression4.txt)
- クリーンビルド
  - バンドル (build_ofx.sh を scratchpad の新しいディレクトリへ。パス検査込み): 警告 0・エラー 0 (logs/build_clean4.txt)
  - qtui --clean-first の全ターゲット: 警告 0 (logs/qtui_build_clean4.txt)
- ~/.config/NeatRepro/nv_ui_qt.ini: sha256 45d4258c53337ee4aa60017a4a4cce72b9f4bf3dc71303406cfb5c1c7d80fcc6。すべての実行の前後で不変。
- make -C native verify: native は無変更なので、再実行していない (前回 PASS)。

今回触ったファイル: ofx/src/session.cpp・plugin.cpp・params.cpp、ofx/tools/minihost.cpp (--log)・verify_ofx.py、qtui/src/app/ofx_session.cpp。qtui のうち ofx_session.cpp 以外は今回は触っていない。PROGRESS.md の末尾に追記済み。

## 統括の再確認 (Round 2 の修正後、2026-11-01、観測)
- `verify_ofx.py --quick --pure --mutate`: **152 比較、0/127,659,520 語不一致、ALL MATCH** (1 分 29 秒)。fd 0 / fd 1 / fd 0〜2 を閉じたホストの 3 通りは rc 0、作成時に既定あり、押下 2 回とも status 0 で変更、レンダー 4 回 status 0、出力と ClipParams は全部開いた起動と同一。dlopen_noload 0 件。
- ログ: `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/n4d_lead/verify_quick3.log`

---

## 独立検証の結果 (統括の要約。全文は `verifier/VERDICT.md`)
- Round 1: REQUEST_CHANGES (M1 深い入れ子でホストが SIGSEGV、L1〜L6)。
- Round 2: REQUEST_CHANGES (R2-1: L3 の修正で、ホストの fd 0 が閉じているとヘルパーの stdin が閉じたまま起動する回帰)。
- Round 3: **APPROVE** (ブロッカー 0)。閉じる fd の組み合わせ 8 通りで出力と ClipParams が全部開いた起動とバイト同一、親のパイプ fd は 3 番以上で close-on-exec・漏れなし、全体照合 163 比較 0/170,596,864 語、自作場面 36 + 19 比較 0 不一致、selftest 117/117、--ofx モード 29 件同一、警告 0、ini 不変。
- 残り (情報): R3-1 ヘルパーの「fd 2 が閉じていれば /dev/null」分岐は、Qt が内部パイプを fd 2 に入れるため通らず、診断出力が失われ得る (応答は壊れない)。提案は QApplication 作成前に閉じた 0〜2 を /dev/null で埋めること。L2 (空の ClipParams で保存したノードは新規扱いで既定が入る) は仕様どおり、Resolve の確認項目に入れた。
- 未確認: Resolve 上の動作 (ウィンドウの前面表示、UI の停止、アクションの呼び出し順、プロキシ解像度)。


---

# Round 4: ウィンドウ表示中に Resolve が「応答なし」になる件 (2026-11-01)

## 発端 (ユーザーの報告)
実機の Resolve で「Meat Video のウィンドウが開いた時、Resolve が無応答という通知が Clean My Mac からきました。Apply で閉じた後は普通に使えました」。
統括がコードで確認した原因: ofx/src/session.cpp の run_helper が、ホストの主スレッド (InstanceChanged の中) でヘルパーの返事を読み込みで待っていた。

## 作業者の報告 (原文、3 通)

N4d 修正報告「ウィンドウ表示中に Resolve が応答なしになる件」(1/3): 変更内容とイベントの扱い

結論: ボタンの待ちの間、主スレッドで NSApp が動いているホストでは、ホストのイベントを独自のランループモード "MeatVideoOFXModal" で取り続けるようにした。それ以外のホストは従来どおり読み込みで待つ。
- 試験ホスト (NSApp 内で minihost を動かす) で、待ちの間の主スレッドを別プロセスから sample(1) で見た。サンプルの 95〜99.9% がイベントの取り出し (ModalWait::pump → -[NSApplication nextEventMatchingMask:…]) で、read などで止まっていたのは 0。
- 待ちの間に送った入力は、待ちの最中にも後にもホストに配られなかった (捨てられた)。
- 対照 (MV_MODAL_WAIT=OFF) では、主スレッドが read で 100% 止まり、送った入力は溜まって待ちの直後に 35〜53 件まとめて配られた。
- ClipParams の保存は、従来どおり InstanceChanged の中で行う。
- 既存の照合はすべて合格。ofx/build/MeatVideo.ofx.bundle は最後に作り直した (署名 OK)。
- コミット・Resolve・/Library・ユーザーの ini は触っていない。

## 退避
変更前の Round 3 承認版を work/omo/wp_native_ofx/N4d/backup_r3/ に置いた (SHA256SUMS.txt 35 件)。
- ofx/ (build を除く)
- qtui の関係ファイル: CMakeLists.txt、main.cpp、ofx_session.{h,cpp}、mainwindow.{h,cpp}、pyobj.{h,cpp}、nvcore.cpp、tests/ofx_session_drv.cpp

## 変更したファイル (backup_r3 との差)
- 新規 ofx/src/modal_wait.{h,cpp} (51 / 142 行): クラス ModalWait
  - active() になる条件: pthread_main_np() が真、dlsym(RTLD_DEFAULT, "NSApp") が非 nil、かつ [NSApp isRunning]。sharedApplication は呼ばない (NSApp を作らない)。
  - wait_fd(fd, POLLIN/POLLOUT): poll(fd, 0) で準備できていなければ、pump(20 ms) を繰り返す。
  - wait_child(pid): waitpid(WNOHANG) と pump を交互に繰り返す。
  - 非 active のときは何もしない (呼び出し側が従来どおり読み込みで待つ)。
  - Objective-C は C++ から objc ランタイム (objc_msgSend) で呼ぶ。リンクするのは libobjc だけ。
    - 理由: AppKit / Foundation をリンクすると、AppKit の無いホスト (minihost) にもバンドルの読み込み時に AppKit を読み込ませ、その初期化子のファイルアクセスが fsguard の「レンダー経路 0」の基準を汚すおそれがあるため。
    - AppKit / Foundation は、ホストが読み込み済みのときだけ objc_getClass / dlsym で使う。
  - 試験用: 環境変数 MEATVIDEO_OFX_TRACE があると、stderr に「meatvideo-ofx: helper wait: host event loop (MeatVideoOFXModal)」か「…: blocking」と書く。
- ofx/src/session.cpp (±23 行): run_helper の送信・読み取り・waitpid だけを ModalWait 経由にした。
  - active のときだけ、パイプを O_NONBLOCK にし、EAGAIN を待ちとして扱う。
  - 非 active のときは従来とまったく同じ (読み込みで待つ write / read、EINTR の再試行、長さで区切る読み取り、waitpid の EINTR ループ)。
  - F_DUPFD_CLOEXEC ≥3、F_SETNOSIGPIPE、CLOEXEC_DEFAULT、fd 2 の継承判定は変えていない。
- ofx/src/plugin.cpp (±11 行): ウィンドウは一度に 1 つ (std::atomic g_window_open)。開いている間の押下 (待ち中にホストから届いた分や、別スレッドからの分) は開かずに警告を出す: "a Meat Video window is already open; close it first."
- ofx/CMakeLists.txt (±17 行): modal_wait.cpp と libobjc を追加。option MV_MODAL_WAIT (既定 ON。OFF にすると従来の待ちだけになる。対照のビルド用)。試験ホストのターゲット cocoahost (OBJCXX、-fobjc-arc、AppKit)。
- 新規 ofx/tools/cocoahost.mm (112 行): NSApplication の中で minihost のセッションを主スレッドで動かす試験ホスト (詳細は 2/3)。
- ofx/tools/minihost.cpp (±40 行):
  - press 行に待ちの区間 wall=t0..t1 (CLOCK_REALTIME) を出す。
  - g_poke フック: ホスト側のタイマー・ブロック・スレッドから InstanceChanged を呼ぶ。
  - MINIHOST_EMBEDDED のときは main を minihost_main にする。
- 新規 ofx/tools/verify_modal.py (287 行): 応答性の試験。
- ofx/tools/verify_ofx.py (±3 行): fd を閉じた起動を 8 通り (0/1/2 のすべての組み合わせ) にした。
- qtui/tests/ofx_session_drv.cpp (±6 行。試験ドライバだけ): 手順 "sleep MS" を追加。
- 製品の qtui (MeatVideoQt) は今回変えていない。

## イベント種別の扱い (ModalWait::pump)
- 取得するマスク: NSEventMaskAny から ApplicationDefined (15) と Periodic (16) を除いたもの。
  - ApplicationDefined / Periodic は取らずにキューに残す。アプリ自身 (Qt のイベントディスパッチャも含む) が自分のループを起こすために投げるもので、捨てると待ちの後のホストの起床を失い得る。待ちの後にホストが処理する。
- 入力 (LeftMouse / RightMouse / OtherMouse の Down・Up・Dragged、MouseMoved、MouseEntered / Exited、KeyDown / KeyUp、FlagsChanged、ScrollWheel、Tablet*、Gesture・Magnify・Swipe・Rotate・Begin / EndGesture・SmartMagnify・QuickLook・Pressure・DirectTouch・ChangeMode、CursorUpdate): 取り出して捨てる。待ちの間ホストは入力を受けない (モーダル)。
  - マウスダウン 3 種とキーダウンでは NSBeep を鳴らし、ヘルパーを前面に出す。
- AppKitDefined (13): -[NSApplication sendEvent:] に渡す。ウィンドウの移動・露出、画面の変更、アプリの (非) アクティブ化といった AppKit 自身の管理に必要なため。
  - 副作用として、アクティブ化の通知のハンドラは同期で走り得る (推定)。
  - ホストがアクティブになった (subtype ApplicationActivated) ときは、ヘルパーを前面に戻す (モーダルパネルが焦点を保つのと同じ)。
- SystemDefined (14、電源ボタン・メディア・音量キーなど): ハードウェアの入力なので捨てる。
- 前面化 (0.25 s に 1 回まで):
  - NSRunningApplication runningApplicationWithProcessIdentifier:pid を使う。ホストがアクティブなら [NSApp yieldActivationToApplication:] (macOS 14 の協調的アクティブ化) のあと [ra activateWithOptions:NSApplicationActivateAllWindows]。
  - 20 ms 待ってもイベントが来ず、ホストがアクティブなときにも行う。
  - ヘルパーが WindowServer にアプリとして登録される前 (ra が nil の間) は何もしない。
- 独自モードで回すので、ホストのタイマー・main queue のブロック・default / common モードのランループソースは待ちの間に動かない (2/3 で観測)。

(続く: 2/3 試験と数値、3/3 本家の静的確認・残るリスク)

N4d 修正報告「応答なし」(2/3): 試験とその数値

## (a) Cocoa の試験ホスト: `caffeinate -i python3 ofx/tools/verify_modal.py --control <MV_MODAL_WAIT=OFF のバンドル>`
ログは logs/verify_modal.txt と、最終バンドルでの再実行 logs/verify_modal_final.txt。
- 試験ホスト ofx/build/cocoahost:
  - NSApplication (Regular、アクティブにはしない) と、小さなウィンドウ 1 枚 (画面左下に出る)。
  - 0.5 s 後に default モードのタイマーから、minihost のセッションを主スレッドで実行する。
  - 記録 (stderr):
    - 0.1 s ごとの default モードのタイマーの tick
    - ホストが配った入力 (sendEvent: から呼ばれるローカルモニター)
    - セッション開始 2 s 後に dispatch_after (main queue) したブロック。プラグインの ClipParams の InstanceChanged を呼ぶ
    - (--poke のとき) 3 s 後に別スレッドからの Prepare の押下
- ヘルパーはスタブのシェルスクリプト。--ofx-default には即答し、セッションでは N 秒後に APPLY / 死亡 / 無応答 (EOF)。または C++ UI の試験ドライバ (offscreen) で "sleep 9000;close"。
- 別プロセス (試験スクリプト) からの測り方:
  - (1) sample(1) で主スレッドを 1 秒ずつ 2 回 (待ちの 3 s・6 s の時点)。
  - (2) CGEventPostToPid でキーの押下・離し、マウス移動、4 回に 1 回クリックを 0.5 s ごとに送る (追加の権限は不要だった)。待ちの前・最中・後のそれぞれで、ホストが配った数を数える。
  - (3) SkyLight の CGSEventIsAppUnresponsive(CGSMainConnectionID(), psn) を 0.25 s ごとに問い合わせる。
- (3) を判定に使わない理由 (観測): ホストを SIGSTOP で止めてイベントを送っても、12 秒間ずっと false だった。この旗は WindowServer 自身が配るイベント (実際のカーソルやキーボードの焦点) で立つとみられ (推定)、合成の送信では立たない。ユーザーのカーソルを動かす方法 (kCGHIDEventTap、CGWarpMouseCursorPosition) は、実機で作業中のユーザーの邪魔になるので使っていない。代わりに (1) と (2) で判定した。(2) の「待ち中に送ったイベントを読まずにキューに残すか」は、システムが「応答なし」とする条件そのもの (推定)。

結果 (待ち = minihost の press の wall 区間):

| 場面 | 結果 | 待ち | 主スレッド (sample) | 入力 (前 / 待ち中に送った数 / 待ち中に配った数 / 待ち後 1 s に配った数) | tick (待ち中) | main queue ブロック |
|---|---|---|---|---|---|---|
| apply-10s (Apply) | status 0、ClipParams 変更 | 10.56 s | 3 s: 836/847 pump・11 poll・0 停止。6 s: 807/846 pump・39 poll・0 停止 | 5 / 51 / 0 / 0 | 0 (全 12) | 待ち終了 +0.00 s に実行 |
| **対照** (同じ、MV_MODAL_WAIT=OFF) | status 0、変更 | 10.04 s | 3 s: **0/861 pump・861 が read で停止**。6 s: 0/864・864 停止 | 5 / 48 / 0 / **35** (溜まって後で配られた) | 0 (全 11) | +0.00 s |
| helper-dies (3 s 後に kill -9) | status 1 "terminated by signal 9; the settings were not changed"、不変 | 3.46 s | 817/845 pump・28 poll・0 停止 | 5 / 14 / 0 / 0 | 0 | +0.00 s |
| no-reply-eof (9 s 後に何も返さず終了) | status 1 "unexpected reply (0 bytes)"、不変 | 9.33 s | 840/844、833/848 pump・0 停止 | 5 / 45 / 0 / 0 | 0 | +0.00 s |
| window-closed (C++ UI ドライバ。9 s 後に close → closeEvent → CANCEL) | status 0、不変 | 9.89 s | 835/848、849/851 pump・0 停止 | 5 / 48 / 0 / 0 | 0 | +0.00 s |

- CGSEventIsAppUnresponsive は、すべての場面 (対照も含む) で 0 (上の理由で判定には使っていない)。
- 再入:
  - 待ちの間、ホストのタイマー (tick 0) と main queue のブロック (待ち終了の直後に実行) は動かなかった。待ち中にプラグインが呼ばれることはなかった。
  - 別スレッドからの押下 (apply-10s と対照で各 1 回) は status 0 で、"a Meat Video window is already open; close it first." と出た。2 つ目のヘルパーは起動していない (fsguard の posix_spawn は --ofx-default 1 と --ofx-session 1 のみ、記録は計 3 件)。
- プラグインのトレース: 試験ホストでは "helper wait: host event loop (MeatVideoOFXModal)" が 2 回 (--ofx-default と --ofx-session)。対照では "blocking"。
- minihost (NSApp の無い、画面なしのホスト): トレースは "helper wait: blocking" ×2 で、従来の待ちのまま。押下は status 0・変更。

## (b) 既存の照合 (すべて今回のソースで実行)
- `verify_ofx.py --pure --mutate` (全場面、logs/verify_r4_full.txt): **163 比較 0/170,596,864 語不一致、ALL MATCH**。
  - 内訳: ブリッジ経路 76・純 Python 76・qtui 自身の処理 11。
  - 拒否 19 種・ヘルパー失敗 6 種・変異 13 件は期待どおり。dlopen_noload は 0 件。
- fd を閉じた起動 8 通り (なし・0・1・2・0+1・0+2・1+2・0+1+2): すべて ok。出力と ClipParams は、全部開いた起動とバイト単位で同一。
- fsguard: レンダーだけの場面 6 件で記録はバンドルの dlopen 1 件だけ。他の場面も dlopen と posix_spawn(--ofx-default / --ofx-session) だけ。libobjc を足しても記録は増えていない。
- qtui_selftest: 117 passed / 0 failed (logs/qtui_selftest5.txt)。
- 既存の --ofx モード: 検証者の cmp_ofxmode.py (compare_ofx.py の全 26 ケースと引数エラー 3 種) を、HEAD からビルドしたものと今回のもので実行し、**29/29 SAME** (logs/ofx_mode_29.txt)。
- クリーンビルド:
  - build_ofx.sh を新しいディレクトリへ: cocoahost を含めて警告 0・エラー 0 (logs/build_clean5.txt)。
  - qtui --clean-first: 警告 0 (logs/qtui_build_clean5.txt)。
- ~/.config/NeatRepro/nv_ui_qt.ini: sha256 45d4258c53337ee4aa60017a4a4cce72b9f4bf3dc71303406cfb5c1c7d80fcc6 で、すべての前後で不変。
- 最後に ofx/build/MeatVideo.ofx.bundle を作り直した。codesign --verify --strict --deep は OK、依存は libnvcore・libobjc・libc++・libSystem。作り直した後のバンドルでも verify_modal (apply・minihost) と verify_ofx --quick (81 比較 0 語) が合格。
- make -C native verify: native は無変更なので再実行していない。

(続く: 3/3)

N4d 修正報告「応答なし」(3/3): 本家の静的確認・残るリスク・Resolve での確認手順

## 本家の待ち方 (静的のみ。実行も dlopen もしていない)
観測:
- reference/genuine/NeatVideo6.arm64.ofx の nm に `U _NSModalPanelRunLoopMode` (import) がある。
- strings に `QCocoaEventDispatcher`・`N12NeatNV6Space21QCocoaEventDispatcherE`・`N12NeatNV6Space28QCocoaEventDispatcherPrivateE` があり、セレクタ `beginModalSessionForWindow:`・`runModalSession:`・`runModalForWindow:` がある。
- analysis/functions.csv には `runApplicationModalPanel`・`worksWhenModal`・`setWorksWhenModal:`・`runModal` がある。

推定:
- 本家は名前空間 NeatNV6Space 付きの Qt (Cocoa プラットフォームを含む) を静的に持つ。ウィンドウは同じプロセス内の Qt のモーダル (QDialog::exec → QEventLoop → QCocoaEventDispatcher の beginModalSession → [NSApp beginModalSessionForWindow:] + runModalSession、NSModalPanelRunLoopMode) で、ホストの NSApp の上で回しているとみられる。
- ボタンのハンドラからこの経路に入ることは、コードの流れでは確認していない。

## 残るリスク (観測と推定を分ける)
- 推定: Resolve がボタンの InstanceChanged を主スレッド以外で呼び、その間 Resolve の主スレッドが完了を待つ作りなら、この修正は効かない。ModalWait は主スレッドでしか動かず、別スレッドから主スレッドのイベントは回せない。
  - Resolve を Terminal から `MEATVIDEO_OFX_TRACE=1` 付きで起動すれば、"helper wait: host event loop" か "blocking" かで、どちらの経路を通ったか分かる。
  - Resolve が NSApp を isRunning にしていない場合も、"blocking" になる。
- 推定: AppKitDefined を sendEvent: に渡すと、アクティブ化やウィンドウ移動の通知のハンドラ (Resolve や Qt のもの) が待ちの最中に同期で走り得る。
  - そこからプラグインが呼ばれても、プラグインの防止で 2 つ目のウィンドウは開かない (別スレッドからの押下で観測)。
  - ClipParams の InstanceChanged などは普通に処理される (再入しても安全な処理だけ)。
- 観測: WindowServer の「応答なし」の旗 (CGSEventIsAppUnresponsive) そのものは、合成イベントでは確認できなかった (SIGSTOP でも立たない)。
  - 判定は「主スレッドがイベントを取り出しているか」(sample) と「待ち中のイベントがキューに残るか」による。
  - Resolve の実機と実際のカーソルで「応答なし」が出ないかは、ユーザーの確認が要る。
- 推定: 前面化 (yieldActivationToApplication / activateWithOptions) は、ヘルパーが本物の app (同梱の MeatVideoQt.app) のときだけ効く。
  - 試験のヘルパー (スタブや offscreen のドライバ) は NSRunningApplication にならないので、前面化とビープの動作は試験では観測していない。
  - Resolve で、窓が前面に来るかと、Resolve をクリックしたときに窓に戻るかを確かめてほしい。
- 観測: 待ちの間、Resolve の画面は更新されない (タイマーも main queue も止める設計)。
  - 窓の再描画は WindowServer の合成が保つので、真っ白にはならないとみられる (推定)。
  - Resolve の再生やプレビューは、窓を閉じるまで止まる (本家のモーダルも同じとみられる)。
- 推定: macOS の協調的アクティブ化の規則上、ユーザーが別のアプリに切り替えたときは前面化しない (ホストがアクティブなときだけ前面化する)。
- 観測: 「窓の閉じるボタン」による終了は、offscreen のドライバの win->close() (closeEvent と同じ経路) で試した。実際の閉じるボタンのクリックは試していない。
- 既知のまま: L2 (ClipParams が空のまま保存したノードは、開き直しで新規として既定が入る)。

## Resolve での確認手順 (今回の分)
1. ofx/build/MeatVideo.ofx.bundle (作り直し済み) を、sudo cp -R で /Library/OFX/Plugins/ に入れ直し、Resolve を再起動する。
2. 可能なら Terminal から `MEATVIDEO_OFX_TRACE=1 "/Applications/DaVinci Resolve/DaVinci Resolve.app/Contents/MacOS/Resolve"` で起動する。ボタンを押したときに、stderr に "meatvideo-ofx: helper wait: host event loop (MeatVideoOFXModal)" が出るかを見る。"blocking" なら上のリスク 1 (主スレッド以外での呼び出し) に当たっている。
3. ボタンでウィンドウを開いたまま 10 秒以上待つ。次を確かめる:
   - CleanMyMac やアクティビティモニタで Resolve が「応答なし」にならない。
   - Resolve 上でカーソルが回らない。
   - Resolve をクリックすると音が鳴り、ウィンドウが前面に戻る。
   - Resolve のメニューやキーが効かない。
4. Apply / Cancel / 閉じるボタンのどれでも、Resolve がすぐ操作できるようになり、クリックが後からまとめて効いたりしないことを確かめる。
5. 従来の確認項目 (既定・保存・開き直し、L2 を含む) を続ける。

ファイル: work/omo/wp_native_ofx/N4d/PROGRESS.md (末尾に追記)、logs/ の verify_modal*.txt・verify_r4_full.txt・ofx_mode_29.txt・build_clean5.txt・qtui_build_clean5.txt・qtui_selftest5.txt。backup_r3/ は変更前の版。

補足: ofx/.DS_Store が backup と違っていたのは Finder が作るファイルのため。こちらでは触っていない。

## 統括の再実行 4 (2026-11-01)
- backup_r3 の SHA256SUMS は全件 OK。差分は報告どおり (modal_wait.{h,cpp}・cocoahost.mm・verify_modal.py が新規、CMakeLists.txt・plugin.cpp・session.cpp・minihost.cpp・verify_ofx.py が変更)。
- バンドルの依存: libnvcore (@rpath)・libobjc・libc++・libSystem。codesign --verify --strict --deep OK。
- 対照は統括が自分で作成した: `cmake -DMV_MODAL_WAIT=OFF` でビルドした Mach-O を最終バンドルの写しに入れ替え、ad hoc で再署名 (`/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/n4d_lead/ctl/`。警告 0)。
- `verify_modal.py --control <統括の対照>` で ALL OK (ログ `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/n4d_lead/verify_modal_lead.log`)。
  - apply-10s: 主スレッドは 840/848・850/851 がイベントの取り出しで、停止は 0。入力は待ち中に 51 件送って、配られたのは待ち中 0・待ちの後 0。
  - 対照: 865/865・876/876 が read で停止。待ち中に送った 50 件のうち 41 件が、待ちの後 1 s のうちに配られた。
  - helper-dies・no-reply-eof・window-closed: 停止は 0 で、ClipParams は不変。
  - 別スレッドからの押下は警告になり、2 つ目のウィンドウは開かなかった。
  - minihost は "blocking" (従来の待ち)。
  - CGSEventIsAppUnresponsive は対照を含めて全件 0 (作業者の報告どおり、判定には使えない)。
- `verify_ofx.py --quick`: 81 比較 0/67,028,480 語不一致、ALL MATCH (ログ `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/n4d_lead/verify_quick4.log`)。

## 保留 (2026-11-01、ユーザーの指示)
ユーザーの指示「一旦この確認タスクを後回しにして」により、Resolve の実機確認を保留した。
- 済んでいること: 作業者の修正と統括の再実行。独立検証 Round 4 は依頼済み。
- 保留中の確認項目:
  1. ウィンドウ表示中の「応答なし」(10 秒以上待つ)
  2. Resolve をクリックしたときのビープと前面化
  3. Apply / Cancel / 閉じるボタンで閉じた後、すぐ操作できるか
  4. MEATVIDEO_OFX_TRACE=1 で起動したときのトレース ("host event loop" か "blocking" か)
  5. 従来の項目: 既定、保存と開き直し、L2
- 実機で確認できているのは、修正前のバンドルで「Apply で閉じた後は普通に使えた」ことだけ。

## 独立検証 Round 4: REQUEST_CHANGES (verify-n4d、詳細は verifier/VERDICT.md の Round 4 節)
- R4-1 (High、観測): 待ちの間、ホストの主スレッドが空回りする。待ち中の 4 秒で CPU 4.010 秒 (対照 0.000 秒)。sample で mach_msg2_trap での待ちは 0。推定原因は、独自モードにソースがないため CFRunLoopRunInMode がすぐ戻ること。
- R4-2 (High、観測): Cocoa ホストでウィンドウが開くまでが遅い。1080p×11 で対照 0.67 秒に対し 10.5 秒。
- R4-3 (Medium、推定): 入力を捨てることで、mouseUp や修飾キーの状態が残り得る。
- R4-4 (Low、推定): AppKitDefined の sendEvent: からの再入で DestroyInstance が呼ばれる可能性。経路は見つかっていない。
- 回帰はすべて合格。

### 統括の再実行
- verifier/round4/scripts/timing.py: 対照 0.68 秒 / 0.68 秒、修正版 10.32 秒 / 10.70 秒 (R4-2 を再現)。
- CPU 時間 (cocoahost、"sleep 8000;close"、1080p×11、起動から 4 s → 8 s の ps time): 対照 0:00.29 → 0:00.29、修正版 0:03.48 → 0:07.47。4 秒で約 4 秒 = 1 コアを使い切る (R4-1 を再現)。


---

# Round 5: R4-1〜R4-4 の修正 (2026-11-01)

## 作業者の報告 (原文、3 通)

N4d Round 4 修正報告 (1/3): 変更内容と設計の選択

結論: R4-1〜R4-4 を直した。
- 待ちの間のホストの CPU: 0.01〜0.03 秒 / 4 秒 (対照 0.000 秒、修正前 4.01 秒)。
- 主スレッド: sample の 96〜99% がカーネルでの待ち (poll)。
- 1080p×11 でウィンドウが開くまで: 0.64 秒 (対照 0.67 秒、修正前 10.5 秒)。
- 入力は捨てたまま。ただし押下と離しの対は崩さない。タイマーと main queue は止めたまま。
- 既存の照合はすべて合格。ofx/build/MeatVideo.ofx.bundle は作り直した (署名 OK)。backup_r3 は触っていない。コミットはしていない。

## 変更したファイル (行数は backup_r3 との差分の +/− 行の合計)
- ofx/src/modal_wait.{h,cpp} (68 / 209 行。書き直し)
  - wait_fd(fd, events): drain() → poll(fd, events, 10 ms) を、fd が準備できるかエラーになるまで繰り返す。主スレッドは poll の中、つまりカーネルで眠る。
  - drain(): `nextEventMatchingMask:… untilDate:[NSDate distantPast] inMode:"MeatVideoOFXModal" dequeue:YES` を、nil が返るまで (最大 256 件) 繰り返す。待たずにキューを空にする。
  - wait_child(pid): waitpid(WNOHANG) → drain → poll(NULL, 0, 10 ms)。
  - run(f): 新設。f をワーカースレッドで実行し、その間は主スレッドで drain と poll(NULL, 0, 10 ms) を繰り返す。f が終わったら join してから戻る。
  - 非 active のとき (主スレッドでない・NSApp が動いていない・MV_MODAL_WAIT=OFF): wait_fd はすぐ戻り、wait_child は EINTR を再試行する従来の waitpid、run は f をその場で実行する。従来とまったく同じ。
- ofx/src/session.cpp (±21 行)
  - 送信: 従来どおりのブロッキング書き込み (1 MiB 単位、EINTR を再試行)。それを mw.run() の中で実行するので、active のときはワーカースレッドで書く。
  - 前の版で入れた O_NONBLOCK と EAGAIN の扱いは削除した。
  - 受信: mw.wait_fd(out[0], POLLIN) の後にブロッキングの read。poll が読めると言った後なので read は止まらない。
  - F_DUPFD_CLOEXEC ≥3・F_SETNOSIGPIPE・CLOEXEC_DEFAULT・fd 2 の扱い・長さで区切る読み取りは変えていない。
- ofx/src/plugin.cpp (±38 行。前回の g_window_open を含む)
  - Instance に window と destroyed の旗を追加した (R4-4)。
  - open_window は run_session の前に window=true にし、戻ったら window=false。その時点で destroyed なら、インスタンスを delete して、ホストのハンドルには触れずに戻る。
  - destroy_instance は、window が立っていれば destroyed=true だけにして解放を後に回す。立っていなければ従来どおり delete する。
- ofx/tools/cocoahost.mm (144 行)
  - --held: セッションの前に、左ボタンとキー "s" (keyCode 1) の押下を sendEvent: で配っておく。
  - --destroy: 3 秒後に別スレッドから DestroyInstance を呼ぶ。
- ofx/tools/minihost.cpp: g_poke に "destroy" を追加した (DestroyInstance)。
- ofx/tools/verify_modal.py (375 行。書き直し): 合格の条件を CPU・sample の睡眠・開く時間・入力の対・解放の後回しにした (2/3)。
- ofx/CMakeLists.txt、qtui は今回変えていない。

## 設計の選択
- 待ち方: 「poll(fd, 10 ms) でカーネルで待ち、その合間にイベントを待たずに取る」を選んだ。
  - 理由 1: 独自のモードは空のままでよい。CFFileDescriptor をそのモードに登録すると、モードに CF のソースを足すことになる。そのソースのコールバックがランループの中で走り、待ちの最中にホストのコードに近い場所で動く。poll のほうが単純で、ホストのランループに何も残さない。
  - 理由 2: 入力の遅れは最大 10 ms。CPU は 1 秒に 100 回起きる分だけで、測定では 4 秒あたり 0.01〜0.03 秒。
  - 理由 3: sample では、主スレッドの先端が `poll (in libsystem_kernel)` にある (眠っている) ことが見える。
  - 推定: _DPSNextEvent は、イベントを NSEventThread (WindowServer からの受信スレッド) が積んだキューから直接取る。そのため distantPast でも新しいイベントを取れる。観測でも、待ち中に送った入力は待ちの後に配られなかった (取られて捨てられた)。
- 送信: パイプが一杯になるたびにイベントを回す方式はやめた。
  - 代わりに、ブロッキング書き込み (1 MiB ずつ) をワーカースレッドに任せ、主スレッドは 10 ms ごとにイベントを取る。
  - ワーカースレッドが触るのは、パイプの write と close だけ (ホストの API は呼ばない)。
  - フレームは呼び出し元の SessionRequest にあり、run() は join するまで戻らない。
- 入力の一貫性 (R4-3): ホストには、押下なしの離しも、離しなしの押下も見せない。
  - マウスボタン (buttonNumber ごと) とキー (keyCode ごと) の押下が待ち中に来たら、記録してから捨てる (ビープを鳴らし、ウィンドウを前面にする)。対応する離しも捨てる。
  - 記録にない離し (押下が待ちの前にホストへ届いたもの) は sendEvent: で渡す。
  - FlagsChanged (修飾キー) は常に sendEvent: で渡す。ホストの修飾キーの状態はキーボードに従う。
  - キーのリピート・ドラッグ・移動・スクロール・タブレット・ジェスチャ・カーソル更新・SystemDefined は捨てる。
  - AppKitDefined は sendEvent: で渡す。ApplicationDefined と Periodic は取らない (前回と同じ)。
  - 残る推定: 待ち中に押されたまま窓が閉じた場合、その離しは待ちの後にホストへ届く。ホストから見ると、押下のない離しが 1 つ来る。Qt は対応のない離しを無視するとみられる。
- 解放の後回し (R4-4): 窓を待っている間に DestroyInstance が来たら、旗を立てて解放を後に回す。待ちが終わった時点でインスタンスを解放し、ホストの API は呼ばない。

(続く: 2/3 試験と数値、3/3 残るリスク)

N4d Round 4 修正報告 (2/3): 試験と数値 (対照との比較)

## verify_modal.py
コマンド: `caffeinate -i python3 ofx/tools/verify_modal.py --control <MV_MODAL_WAIT=OFF のバンドル>`。ログは logs/verify_modal_r5.txt と logs/verify_modal_r5_dies.txt。
- 対照のバンドル: 同じソースを -DMV_MODAL_WAIT=OFF でビルドしたもの (scratchpad の mctl/ctl.ofx.bundle。警告 0)。
- ホストの CPU: 別プロセスから `ps -o time=` を、待ちの中で 4 秒の間をあけて 2 回読んだ差。合格は 0.2 秒未満。
- 主スレッド: `sample` で 1 秒ずつ取った。「カーネルで待機」は、先端が poll / mach_msg2_trap / kevent / wait4 / read / write / __semwait などにあるサンプル。合格は 80% 以上で、かつ drain のサンプルがあること (イベントを取っている)。

| 場面 | 結果 | CPU / 4 s | 主スレッド (sample) | 待ち中に配られた入力 / 待ち後 1 s |
|---|---|---|---|---|
| apply-10s (修正版) | status 0、ClipParams 変更 | **0.020 s** | 827/840・827/847 がカーネルで待機 (poll)、drain 12・20、read/write での停止 0 | [] / 0 |
| apply-10s **対照 (OFF)** | status 0、変更 | 0.000 s | 864/864・868/868 が read で停止、drain 0 | [] / **41** (溜まって後で配られた) |
| 修正前 (Round 4、検証者の測定) | — | 4.010 s | mach_msg 0・空回り | — |
| helper-dies (7 s 後に kill -9) | status 1 "terminated by signal 9"、不変 | 0.030 s | 826/843 待機、drain 16 | [] / 0 |
| no-reply-eof (9 s 後に何も返さず終了) | status 1 "unexpected reply (0 bytes)"、不変 | 0.020 s | 821/840・835/855 待機 | [] / 0 |
| window-closed (C++ UI ドライバ、9 s 後に close) | status 0、不変 | 0.020 s | 822/839・851/856 待機 | [] / 0 |
| held (下記) | status 0、変更 | 0.020 s | 820/841・833/850 待機 | [2, 11, 12, 12] / 0 |
| destroy (下記) | status 0、不変 | 0.020 s | 819/839・837/851 待機 | [] / 0 |

- すべての場面で、待ち中の default モードのタイマーの tick は 0。main queue のブロックは、待ちが終わった直後 (+0.00 s) に実行された。
- 別スレッドからの押下 (--poke) は status 0 で "a Meat Video window is already open; close it first."。
- プラグインのトレース: cocoahost では "host event loop (MeatVideoOFXModal)"、対照と minihost では "blocking"。
- held (R4-3): セッションの前に左ボタンとキー "s" の押下を配っておいた。待ちに入って 1 s 後に、左ボタンの離し・"s" の離し・Shift の flagsChanged を 2 回送った。3 s 後に、新しい左クリックの押下と離し・キー 0 の押下と離し・マウス移動を送った。
  - 待ち中にホストが配ったもの: [LeftMouseUp 2, KeyUp 11, FlagsChanged 12, FlagsChanged 12]。待ちの前に押されたものの離しと、修飾キーだけが届いた。
  - LeftMouseDown・KeyDown・MouseMoved は 0。待ち中の押下は、その離しと一緒に捨てられた。待ちの後に配られたものも 0。
- destroy (R4-4): 待ちに入って 3 s 後に、別スレッドから DestroyInstance を呼んだ (MallocScribble=1、MallocGuardEdges=1)。
  - DestroyInstance は status 0 で戻った。待ちが終わると press も status 0 で戻り、ClipParams は不変、ホストのプロセスは rc 0 (落ちない)。
  - 解放は待ちの後に行われた (コードで確認。解放済みのメモリへの書き込みがあれば MallocScribble で壊れて表に出るはずだが、出なかった)。
- 開くまでの時間 (R4-2): 1080p×11 フレーム (test_v02 の n=312..322)、Prepare@5、すぐ閉じるセッション。press の wall 区間を、対照と交互に 2 回ずつ測った。合格は、修正版の最悪値が対照の最良値の 1.5 倍以内。
  - C++ UI ドライバ (script "close"): 対照 0.668 / 0.672 s、修正版 **0.641 / 0.640 s** (比 0.96)。修正前は 10.3〜10.7 s。
  - すぐ読み捨てる stub (drain): 対照 0.113 / 0.114 s、修正版 0.123 / 0.124 s (比 1.10)。修正前は 2.86 s。
- minihost (NSApp なし): トレースは "blocking" ×2 で、従来の待ちのまま。press は status 0・変更。
- 1 回目の全体実行では、helper-dies が失敗した。stub が 3 s で死ぬため、2 回目の CPU の読み取りが待ちの外にずれたのが原因 (試験側のタイミング)。stub を 7 s で死ぬようにし、読み取りを 1 s と 5 s に直して再実行し、ok (logs/verify_modal_r5_dies.txt)。製品のコードは変えていない。
- CGSEventIsAppUnresponsive は全場面で 0 (前回の報告どおり、合成のイベントでは立たないので参考値)。

## 既存の照合
- `verify_ofx.py --pure --mutate` (logs/verify_r5_full.txt): **163 比較 0/170,596,864 語不一致、ALL MATCH**。
  - 内訳はブリッジ経路 76・純 Python 76・qtui 自身の処理 11。拒否 19・ヘルパー失敗 6・変異 13 も期待どおり。dlopen_noload は 0。
  - **fd を閉じた起動 8 通りすべて ok** (出力と ClipParams は、全部開いた起動とバイト同一)。
  - fsguard: レンダーだけの場面 6 件はバンドルの dlopen 1 件だけ。ほかの場面も dlopen と posix_spawn (--ofx-default / --ofx-session) だけ。
- qtui_selftest: **117 passed / 0 failed** (logs/qtui_selftest6.txt)。
- 既存の --ofx モード (HEAD のソースからビルドしたものと今回のもの): **29/29 SAME** (logs/ofx_mode_29_r5.txt)。
- クリーンビルド: build_ofx.sh を新しいディレクトリへ (cocoahost を含む) で警告 0・エラー 0 (logs/build_clean6.txt)。qtui の --clean-first で警告 0 (logs/qtui_build_clean6.txt)。
- ~/.config/NeatRepro/nv_ui_qt.ini: sha256 45d4258c53337ee4aa60017a4a4cce72b9f4bf3dc71303406cfb5c1c7d80fcc6 で、すべての前後で不変。
- 最後に ofx/build/MeatVideo.ofx.bundle を作り直した (codesign --verify --strict --deep OK)。作り直したバンドルで verify_modal の全場面を実行し合格 (上の数値)。

(続く: 3/3)

N4d Round 4 修正報告 (3/3): 残るリスク (観測と推定を分けて)

- 観測: ホストのイベントは最大 10 ms 待たされる (poll の間隔)。CPU は 1 秒に 100 回起きる分だけで、4 秒あたり 0.01〜0.03 秒 (1 コアの 0.25〜0.75%)。
- 推定: Resolve がボタンの InstanceChanged を主スレッド以外で呼び、主スレッドがその完了を待つ作りなら、この修正は効かない (前回から変わらず)。Terminal から `MEATVIDEO_OFX_TRACE=1` 付きで起動すれば、"host event loop" か "blocking" かで、どちらの経路を通ったか分かる。
- 推定: WindowServer の「応答なし」の旗そのものは、合成のイベントでは確かめられない (SIGSTOP でも立たない)。
  - 判定の代わりにしたのは 3 点: 主スレッドがイベントを 10 ms 以内に取り続けること (sample の drain と poll)、待ち中に送った入力が溜まらないこと、CPU が使われないこと。
  - Resolve を実際のカーソルで操作して「応答なし」が出ないかは、ユーザーの確認が要る。
- 推定: R4-3 の扱いで、押下が待ちの前にホストへ届いたボタンやキーの離しは、待ちの最中にホストへ渡る。
  - Resolve のボタンが離しで動く作り (Qt の QPushButton の clicked と同じ) なら、離しが待ちの前に終わっているので、この場面は起きない。
  - 押下で動く作りなら、その離しが待ち中に Resolve のウィンドウへ渡る。その結果、Resolve 側で「クリック」が完了する (ボタンの見た目が戻るなど)。
- 推定: 待ち中に押されたボタンやキーが、窓を閉じた後に離されると、ホストには対応のない離しが 1 つ届く (Qt は無視するとみられる)。
- 推定: AppKitDefined と、待ちの前に押されたものの離し・FlagsChanged を sendEvent: に渡すと、ホストのハンドラが待ちの最中に同期で走り得る。
  - そこから DestroyInstance が来ても、解放は後に回る (別スレッドからの呼び出しで観測)。
  - そこから同じ主スレッドで別の押下が来た場合は、g_window_open で拒否される。これは別スレッドからの押下で観測した経路と同じコードを通る (推定)。
  - 同じ主スレッドでの再入そのものは、合成できずに試していない。
- 観測: 開いているウィンドウの最中に DestroyInstance が来た場合、Apply の結果は保存されない (インスタンスが無いので捨てる)。ヘルパーのプロセスは返事をして終わる。
- 推定: 前面化 (yield → activate) は、ヘルパーが本物の app (同梱の MeatVideoQt.app) のときだけ効く。試験では観測していない (前回から変わらず)。
- 観測: 4K 以上での開く時間は測っていない。送信はブロッキング書き込みなので、対照と同じくデータ量に比例するとみられる (推定)。
- 既知のまま: L2 (ClipParams が空のまま保存したノードは、開き直したときに新規として既定が入る)。

## Resolve での確認手順 (前回に加えるもの)
1. ofx/build/MeatVideo.ofx.bundle を入れ直して、Resolve を再起動する。
2. ウィンドウを開いたまま 10 秒以上待つ。次を確かめる:
   - アクティビティモニタで Resolve の CPU が上がっていない (前の版では 100% だった)。
   - CleanMyMac やアクティビティモニタで「応答なし」にならない。
3. ボタンを押してから、1080p や 4K のウィンドウが出るまでの時間を確かめる (前の版は 1080p で約 10 秒)。
4. ウィンドウを閉じた後に、Resolve のクリックやドラッグ、Shift / Command などの修飾キーが正しく効くことを確かめる (押されたままになっていない)。
5. 可能なら `MEATVIDEO_OFX_TRACE=1` 付きで Terminal から起動し、stderr の "helper wait:" の行を見る。

ファイル: work/omo/wp_native_ofx/N4d/PROGRESS.md (末尾に追記)、logs/ の verify_modal_r5*.txt・verify_r5_full.txt・ofx_mode_29_r5.txt・build_clean6.txt・qtui_build_clean6.txt・qtui_selftest6.txt。

## 統括の再実行 5 (2026-11-01)
- 対照: 統括の対照のビルド (`-DMV_MODAL_WAIT=OFF`) を今のソースで作り直し、最終バンドルの写しに入れて再署名した。警告 0。Mach-O は修正版と異なる。
- 条件: cocoahost、C++ UI ドライバ、1080p×11 (検証者の clip_1080x11.f32)、Prepare@5。
- ウィンドウが開くまで (script "close"、交互に 2 回ずつ):
  - 対照 0.68 / 0.66 s
  - 修正版 0.64 / 0.65 s
  - トレースは対照が "blocking"、修正版が "host event loop"。
- 待ち中の CPU ("sleep 9000;close"。cocoahost のプロセスを ps で、3 s → 7 s の 4 秒):
  - 対照 0:00.29 → 0:00.29
  - 修正版 0:00.31 → 0:00.33 (0.02 s / 4 s)
- sample (修正版、待ち中に 1 秒):
  - 主スレッド 850 サンプル中 834 が ModalWait::wait_fd → poll (カーネルで待機)、15 が drain。
  - 先端の集計は poll 834、mach_msg2_trap 850、__workq_kernreturn 1700。
- `verify_ofx.py --quick`: 81 比較、0/67,028,480 語不一致、ALL MATCH。

## 独立検証 Round 5: APPROVE (verify-n4d、詳細は verifier/VERDICT.md の Round 5 節)
- 条件 1 (CPU): 待ち中 0.020 s / 4 s (2 回)。今のソースで作った対照は 0.010 s、Round 4 は 4.01 s。
- 条件 2 (開くまで、1080p×11): 0.630〜0.638 s。対照は 0.656〜0.667 s (交互に 3 回ずつ)。
- 条件 3 (sample): 主スレッドの 824/842・824/846 が poll で眠り、drain は 18・22 サンプル。待ち中に送った入力は後から配られない。
- 新しい verify_modal.py は空回りを検出する。Round 4 のバンドルでは FAIL (CPU 3.97 s、カーネルでの待ち 40/846)。
- 敵対的な試験はすべて合格: 押下と離しの対の管理、ヘルパーが 6 秒読まない場合、ヘルパーが要求の途中で終了する場合。
- 残る指摘:
  - R5-1 (Low): run_session が例外で抜けると、in->window が true のまま残る。その後の DestroyInstance で解放されず漏れる。std::thread の生成に失敗すると fd も閉じない。いずれも資源が尽きたときだけ。
  - R5-2〜R5-4 (情報): リピートでない KeyDown の扱い、閉じた後に対応のない離しが 1 つ届く件、前面化が未観測な件。
- 回帰はすべて合格:
  - verify_ofx 163 比較、0/170,596,864 語不一致。closed-fd 8 通りとも ok。
  - 自作の場面 0/59,616,000 語不一致。
  - selftest 117/117、--ofx モード 29 件同一。
  - 警告 0、ini は不変。
- Resolve の実機確認は保留のまま。
