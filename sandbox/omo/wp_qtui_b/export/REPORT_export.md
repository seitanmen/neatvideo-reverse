# N3b export 担当 REPORT: 書き出しエンジンの C++ 移植 (2026-10-29)

Python 版 `repro/nv_ui_qt/export.py` → `repro/denoise_video_v5.py` main() と `repro/nv_depth.py` の IO 変換を
`qtui/src/core/export.cpp` に移した。フレーム処理は libnvcore (`nvqt::temporal` / `nvqt::finish` = backend.process_frame)
のみ。ffmpeg / ffprobe の引数列は Python と語単位で同一 (下記の照合で確認)。コミットはしていない。
AGENTS.md・MIGRATION.md・CMakeLists.txt・native/・repro/ には触れていない。

## 変更・新規ファイル
- `qtui/src/core/export.cpp` (スタブを全面置換。`export.h` の宣言は不変、追加なし)
- `qtui/tests/export_cli.cpp` (スタブを全面置換): `qtui_export SRC DST --ui-profile P.json --params Q.json [--jobs J] [--crf C]
  [--audio copy|aac|none] [--depth D] [--codec X] [--start S] [--frames N]`。`--selftest-cancel` (環境変数 `NV_CANCEL_MS`、既定 800 ms 後に
  `ExportJob::kill()`) で ExportJob のシグナル経路と取り消しを確かめる
- `qtui/tools/compare_export.py` (新規): 同じプロファイル・パラメータで `python3 -m nv_ui_qt.export` (NV_NATIVE=1) と `qtui_export` を動かし、
  出力ファイル (mp4/ts はバイト、mkv は framemd5 + ファイル長)、spawn した全コマンド列 (Python 側は subprocess.Popen のパッチ、C++ 側は
  `NV_EXPORT_TRACE=<file>`)、メタ JSON (`seconds` 以外) を比べる
- `qtui/src/core/video.h` / `video.cpp`: `std::string tool_path(const char* name)` を追加 (既存関数は不変)
- 記録: `sandbox/omo/wp_qtui_b/export/` (本書、`compare_export_log.txt`、`out/` に各ケースの出力・ログ・トレース・テストクリップ)

## 照合結果 (観測。`compare_export_log.txt`、`out/results.json`)
実行: `cd repro && QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/compare_export.py sandbox/omo/wp_qtui_b/export/out`
(C++ 側は `qtui/build_exp/qtui_export`、両側 `--jobs 2`)。プロファイルは `backend.build_profile(..., "auto")` + `run_aft` (test.mp4 は
フレーム 300、test_v02 はフレーム 100)、パラメータは `state.backend_params(default_params() + 変更)`。

| ケース | 入力 | パラメータ | 引数 | 出力 | コマンド列 |
|---|---|---|---|---|---|
| a_t_default | test.mp4 | 既定 | crf 0, start 300, 4 f | **バイト一致** 512,082 B, 4 f | 同一 8/8 |
| b_v02_default | test_v02.mp4 | 既定 | crf 0, start 100, 4 f | **バイト一致** 2,447,586 B, 4 f | 同一 8/8 |
| c1_t_perchan | test.mp4 | bamt_Cr_medium 60, bamt_Cb_low 30, amt_Y 50, bnoise_Y_high 130 | crf 0, 4 f | **バイト一致** 560,656 B | 同一 |
| c2_v02_mix | test_v02.mp4 | mix_enable, mix 25, adv_noise_level 150, adv_threshold 3, t_noise 60 | crf 0, 4 f | **バイト一致** 3,182,018 B | 同一 |
| c3_t_radius1_s_off | test.mp4 | t_radius 1, s_enable off, noise_Cb 150 | crf 0, 4 f | **バイト一致** 765,085 B | 同一 |
| d_aac_copy_crf16 | clip_aac.mp4 (test.mp4 + sine AAC, 6 s) | 既定 | crf 16, audio copy, start 20, 24 f | **バイト一致** 324,556 B, 24 f (copy 経路: -ss/-itsoffset/-copypriorss) | 同一 9/9 |
| d2_ts_mp2_crf16 | clip_mp2.ts (MPEG-TS, mp2) | 既定 | crf 16, audio copy, start 20, 24 f | **バイト一致** 333,913 B (discont 経路、mp2 → aac) | 同一 9/9 |
| e_10bit_mkv | src10.mkv (yuv420p10le FFV1、selftest と同じ作り方) | 既定 | crf 0, depth 10, start 4, 3 f | **復号フレーム一致 + 同じ長さ** 674,147 B, 3 f | 同一 8/8 |
| f1_audio_aac | clip_aac.mp4 | 既定 | crf 16, audio aac, 12 f | **バイト一致** 174,597 B | 同一 9/9 |
| f2_audio_none | clip_aac.mp4 | 既定 | crf 16, audio none, 12 f | **バイト一致** 165,639 B | 同一 8/8 |
| h_full_short | clip_aac.mp4 | 既定 | crf 16, audio copy, 範囲指定なし (全 180 f) | **バイト一致** 2,453,110 B, 180 f | 同一 9/9 |
| e2_12bit_x265 (追加) | src10.mkv | 既定 | crf 0, depth 12 → yuv420p12le libx265 .mp4 | **バイト一致** 315,655 B | 同一 |
| e3_16bit_mkv (追加) | src10.mkv | 既定 | depth 16 → yuv420p16le FFV1 | **復号フレーム一致 + 同じ長さ** 1,502,465 B | 同一 |
| e4_32float_mkv (追加) | src10.mkv、プロファイルは depth 32 (4:4:4) で構築 | 既定 | depth 32 → gbrpf32le FFV1 level 4 | **復号フレーム一致 + 同じ長さ** 5,801,637 B | 同一 |

- 14 ケースすべてで出力・コマンド列・メタ JSON (seconds 以外) が一致 (`RESULT: ALL SAME`)。
- 取り消し (`--selftest-cancel`、`out/cancel.log`・`cancel2.log`): kill 0.8 s 後 (プロファイル用フレームの読み中) と 5 s 後 (符号化中) のどちらも
  `finished(ok=0)` が届き、ffmpeg の子プロセスは残らず、部分出力は残らない (符号化中の kill はエンコーダへの SIGKILL → 書き込みの EPIPE →
  Python と同じ「encoder failed」経路で `remove_output`)。
- 速度 (参考、負荷あり): 1080p 24 フレーム・jobs 2・crf 16 (start 100、復号を含む) C++ 7.8 s / Python 10.1 s、出力はバイト一致。4 フレームでは
  C++ 2.2 s / Python 3.1 s (表の time 列)。
- `qtui_export` のビルド (build_exp) は警告 0。

## Python 版との相違 (仕様)
- 並列はプロセスではなく **スレッド** (`jobs` 本のプール)。nvcore の時間・空間カーネルは可変の静的状態を持たない (grep で確認) ので結果は同じ (照合済み)。
- 取り消しは別プロセスのプロセスグループ kill ではなく、`ExportJob::kill()` が cancel フラグを立て、その書き出しが spawn した ffmpeg/ffprobe に
  SIGKILL を送る (子プロセスの登録表は cancel フラグのアドレスをキーにした thread_local)。部分出力の後始末は Python UI と同じく **呼び出し側**
  (`_remove_export_files` 相当) が行う前提 (エンコーダ失敗経路に入った場合はエンジン側でも `remove_output` する)。
- `print` 行は Python の stdout + `sys.exit` のメッセージを 1 本の流れにしたもの (Python UI は MergedChannels で読んでいたので同じ見え方)。
  ffmpeg の子の stderr はそのまま継承 (アプリの stderr へ)。
- メタ JSON の `seconds` は実測なので一致しない。`profile frame` 行の sigma は `round(v, 3)` を `nearbyint(v*1000)/1000` で近似 (表示だけ)。
- `--flicker` は実装していない (UI から使われない)。
- プロファイル用フレーム (pf..pf+2) は Python と同じく読む (読めないときの note / error の経路のため) が、中身は使わない (数は数える)。
- SIGPIPE は `run_export` の最初に `SIG_IGN` にする (プロセス全体に効く)。

## 未照合の経路
- prores (prores_ks)、EXR 連番 (`-f image2`、`remove_output` の連番削除)、4:2:2 の widen / 偶数列の戻し、gbrp 整数 (10/12/16 ビット RGB)、
  sanitize / restore (NaN・inf を含む float 入力)、奇数サイズの crop、回転タグ、音声トラック 2 本以上、音声がコピーも再符号化もできない警告経路。
  いずれも Python のコードを写したが、照合は上の 14 ケースの範囲のみ。
- `audio_plan` / `trial_encode` の一時ディレクトリは QTemporaryDir (Python は tempfile)。パス以外の引数は同一 (トレースで確認)。

## 質問
- `ExportJob::kill()` の後の部分出力の削除は、Python UI と同じく mainwindow 側 (`_remove_export_files`: dst・`<stem>.json`・
  `.ui_params.json`・`.ui_profile.json`) で行う前提にしたが、それでよいか。
- `qtui_export` は `settings()` (mainwindow.h) 経由で `prefs/ffmpeg` を読む。CLI が mainwindow.h に依存するのが望ましくなければ `find_ffmpeg_dir("")` に変える。
