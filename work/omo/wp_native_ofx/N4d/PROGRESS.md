# N4d PROGRESS (作業者 n4d-worker)

## 2026-11-01
- 承認済み N4b の ofx/ (build を除く) を `backup_n4b/ofx/` に退避 (SHA256SUMS.txt 24 ファイル)。
- 設計: ClipParams = JSON (format meatvideo_ofx_clip_params v1。preset / profile = UI の保存形式、render = matrix・full_range・backend_params・effective profile)。
  プラグインは render 部だけを読む (PyObj = qtui/src/core/pyobj.cpp をプラグインにも組み込み、"C" ロケールで解析)。
  v5 ブロックへの写像は qtui nvcore.cpp の v5_blocks / uniform / channel_params / t_noise と同じ。
- ヘルパー: MeatVideoQt --ofx-session (qtui/src/app/ofx_session.{h,cpp})。要求 = JSON 1 行 + gbrpf32le フレーム、応答 = "APPLY n\n"+ClipParams か "CANCEL\n"。
  プラグイン側 ofx/src/session.{h,cpp} (posix_spawn、CLOEXEC_DEFAULT、SIGPIPE 抑止、DYLD_*/Qt パス変数を除いた環境)。
- Describe を本家の "Controls" ページ構成に変更 (18 パラメータと Rebuild を削除)。minihost --check に本家との一致検査を追加 → CHECK OK。
- ref_floor.inc はプラグインがプロファイルを作らなくなったので削除 (backup に残る)。minihost の --check-floor も削除。
- qtui: set_ofx_frames に matrix / full_range の既定引数を追加 (既存の --ofx は既定値で不変)、main.cpp に --ofx-session の分岐、
  試験ドライバ tests/ofx_session_drv.cpp と CMake ターゲット qtui_ofx_session_drv。qtui 全ターゲットのビルド警告 0。
- 手動の通し試験: minihost で render(素通し) → Prepare(auto;apply) → render → Adjust(auto;apply)。fsguard は dlopen 1 + posix_spawn 2 のみ。
- verify_ofx.py を N4d 用に書き直し (窓の場面・合成 ClipParams の場面・拒否・ヘルパー失敗・変異)。
- 最初の照合の実行が 19109 秒かかったのは、00:34〜05:51 にバッテリー残量 1% で Mac がスリープしたため (pmset -g log で確認)。
  以降の長いコマンドは caffeinate -i で実行。
- 悪い応答 (JSON でない) の経路で fsguard が dlopen(/usr/lib/libstdc++.6.dylib) を記録 (PyObj::parse の std::runtime_error の破棄で
  libc++abi が RTLD_NOLOAD で探す。バックトレースで確認)。fsguard を RTLD_NOLOAD のとき dlopen_noload と記録するように変更し、
  照合はこの 1 経路だけ dlopen_noload を許す。
- build_ofx.sh: Contents/Helpers/MeatVideoQt.app (macdeployqt + libnvcore + offscreen プラグイン + qt.conf)。
  qt.conf がないと Qt の組み込みプラグインパス (~/Qt) が先に使われ得るので追加。DYLD_PRINT_LIBRARIES で Qt はすべてバンドル内から読まれることを確認。
- 照合 (logs/verify_full_pure_mutate.txt): 108 比較 0/121,060,864 語不一致 (native 51・pure 51・qtui 6)、拒否 7・ヘルパー失敗 6・同梱ヘルパー・変異 0〜8 すべて期待どおり。
- 既存: qtui_selftest 117/117、--ofx モードの改変前後バイト比較 18/18 同一、make -C native verify PASS、クリーンビルド警告 0。

## 追加: 既定プロファイル・既定プリセット (同日、統括からの範囲追加 2 件)
- qtui: `MeatVideoQt --ofx-default` (QApplication を作らない。QCoreApplication で設定だけ読む) → "DEFAULTS n profile preset\n"+ClipParams (matrix "auto", frame_size null)。
  ウィンドウは ClipParams が空なら既定プロファイルを読み込み (既定プリセットは MainWindow の構築時に入る)、Reset Profile / Reset Filter Settings を既定に戻す処理へ
  つなぎ替え (単体の UI の動作は変えない)。読めない既定は本家の文言を出す。
- プラグイン: ClipParams が空のインスタンスだけ CreateInstance で --ofx-default を起動し、結果をメモリ (pending) に持つ
  (ofxParam.h は paramSetValue を InstanceChanged / interact だけに許すため)。レンダーと GetFramesNeeded は pending を使い、
  最初の InstanceChanged か SyncPrivateData で ClipParams に保存。保存済みの ClipParams は置き換えない。render.matrix "auto" はフレームの高さで決める。
- fsguard は posix_spawn の第 1 引数も記録 (--ofx-default / --ofx-session の区別)。minihost に --sync。
- 照合 (logs/verify_full_pure_mutate2.txt): 150 比較 0/158,846,464 語不一致 (native 70・pure 70・qtui 10)、sync 8・既定の検査 3・変異 0〜11 すべて期待どおり。
- 既存: qtui_selftest 117/117、--ofx モードの改変前後比較 18/18 同一、クリーンビルド (バンドル・qtui) 警告 0。native は無変更 (前回の verify PASS)。

## 独立検証 (verify-n4d) の指摘への対応 (同日)
- M1: PyObj::parse に入れ子の上限 256 (PyObj::MAX_DEPTH) を追加。超えると ParseError ("nesting deeper than 256 levels")。
  minihost --thread-stack KB で 512 KB スレッド上の試験: 257 段・10 万段・[×30 万・オブジェクト 10 万段は status 1 + メッセージ、
  上限ちょうどは 128 KB スレッドでも読める。深い ClipParams のインスタンスでウィンドウを開くと既定から始まり Apply で置き換わる。
- L1: qtui の CMake に NV_EMBED_SOURCE_ROOT (既定 ON)。build_ofx.sh は OFF でビルドし ref_floor.npy を Resources/data に同梱、
  バンドル内にチェックアウトのパスが無いことを grep で確認 (あれば失敗)。fsguard に FSGUARD_ALWAYS (起動から終了まで記録) を追加して確認。
- L3: パイプ 4 本に FD_CLOEXEC、応答は自身の長さまで読んで止める。L4: paramEditEnd を必ず呼ぶ、保存失敗時は既定を保留のまま再試行。
- L5: ParseError (std::exception + std::string) にして dlopen_noload が出なくなった (全照合で 0 件)。L6: 型・範囲の検査。
- 照合 (logs/verify_fix_full.txt): 163 比較 0/170,596,864 語不一致。selftest 117/117、--ofx 比較 18/18 同一、クリーンビルド警告 0、ini 不変。

## Round 2 の指摘への対応 (同日)
- R2-1: パイプの 4 本を fcntl(F_DUPFD_CLOEXEC, 3) で 3 以上に移してから使う (元の fd は閉じる)。fd 2 が閉じていれば addinherit しない。
  ヘルパーは fd 2 が閉じていると fd 1・2 を /dev/null に向ける。minihost に --log。fd 0 / fd 1 / 0〜2 を閉じた起動で既定・Prepare・Adjust・Apply が
  全部開いた起動とバイト同一。対照: Round 2 のコードは fd 0 を閉じると status 1 ("no request header on stdin")。fd 1 を閉じた場合は Round 2 のコードでも動いた。
- Low: mix などの範囲外メッセージを py_float_repr (1.0000001 / [0.0, 1.0])、保存失敗の警告は後の成功で消す、ClipParams だけ保存されたときは同じテキストで再保存して
  ParamsHash を揃える、frame_size は 0〜1e6 の int のときだけ読む。
- 照合 (logs/verify_r2_full.txt): 163 比較 0/170,596,864 語不一致。selftest 117/117、--ofx 18/18 同一、クリーンビルド警告 0、ini 不変。

## Resolve が「応答なし」になる件 (同日、統括からの依頼)
- backup_r3/ に Round 3 承認時の ofx/ と qtui の関係ファイルを退避 (SHA256SUMS.txt 35 件)。
- ofx/src/modal_wait.{h,cpp}: 主スレッドで NSApp が動いているホストでは、ヘルパー待ちの間ホストのイベントを独自モード MeatVideoOFXModal で取り出す
  (入力は捨てる、AppKitDefined は sendEvent:、SystemDefined は捨てる、ApplicationDefined / Periodic は取らない)。objc ランタイムを C++ から呼ぶ (libobjc だけ)。
  それ以外は従来の待ち。プラグインに「窓は一度に 1 つ」の防止。
- ofx/tools/cocoahost.mm (NSApp 内の試験ホスト)、verify_modal.py。CGSEventIsAppUnresponsive は SIGSTOP したホストでも立たないので判定に使わず、
  sample(1) の主スレッドと、待ち中に送った入力が後で配られるかで判定。対照 (MV_MODAL_WAIT=OFF) は主スレッドが read で止まり、入力が溜まって後で配られる。
- 照合: verify_ofx 163 比較 0 語、fd 閉じ 8 通り同一、selftest 117/117、--ofx 29 件同一、クリーンビルド警告 0、ini 不変。最後に ofx/build のバンドルを作り直した。

## Round 4 の指摘 (空回り・開くのが遅い) への対応 (同日)
- ModalWait: poll(fd, 10 ms) でカーネル待ち + untilDate: distantPast でイベントを空になるまで取る (空のモードでの空回りをやめた)。
  要求の送信は従来どおりのブロッキング書き込み (1 MiB 単位) をワーカースレッドで行い、主スレッドは 10 ms ごとに起きてイベントを取る。
- R4-3: 待ち中に押されたボタン・キーは離しと一緒に捨て、待ちの前に押されたものの離しは sendEvent:、flagsChanged は sendEvent:。
- R4-4: 窓が開いている間の DestroyInstance は待ちの後に解放 (window / destroyed 旗)。cocoahost --destroy で試験 (MallocScribble)。
- verify_modal.py: CPU (ps) < 0.2 s/4 s、sample で主スレッドの 80% 以上がカーネル待ち、1080p×11 の開く時間が対照の 1.5 倍以内、held / destroy を合格条件に。
- 結果: CPU 0.01〜0.03 s / 4 s (対照 0.000)、sample 96〜99% がカーネル待ち、開く時間 0.64 s (対照 0.67)。verify_ofx 163 比較 0 語、fd 閉じ 8 通り、selftest 117、--ofx 29 件、警告 0、ini 不変。
