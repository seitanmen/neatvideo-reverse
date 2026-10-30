# G9 (x86 検証の穴埋め) 進捗 — 作業者 fix2-c

worktree: .claude/worktrees/n3fix-c (df46dda)

## 方針 (2026-10-30)
- x86_64 の Python は既存の python.org 3.12.10 universal (`arch -x86_64 /Library/Frameworks/Python.framework/Versions/3.12/bin/python3.12`)
  の標準ライブラリ (ctypes・pickle) だけを使う。numpy などは入れない (新規インストールなし)。
- 方式: arm64 の verify_bridge.py を通常どおり走らせ、ctypes の境界 (libnvcore の nvc_* 呼び出し) で
  入力バッファ・呼び出し前の出力バッファ・取り消しの応答列を記録 (sitecustomize で ctypes.CDLL を包む。処理コード無変更)。
  記録を x86_64 Python + libnvcore_x86.dylib で再生し、呼び出し後の全バッファ・戻り値を arm64 の記録とバイト比較。
  同じ記録を arm64 (再生の健全性) と arm64 portable (-DNV_PORTABLE) でも再生する。
- Makefile: lib_x86 目標 (LZMA SDK の x86 オブジェクトは別ディレクトリ)。既定ビルドは不変にする。

## 2026-10-30 午前
- Makefile: `lib_x86` 目標 (SDK_OBJDIR/SDK_XFLAGS 変数。既定値は従来と同じ)。lib / lib_portable / lib_x86 とも警告 0。
- 追加: native/x86_bridge_cap.py (記録)、native/x86cap/sitecustomize.py (読み込み口)、native/x86_bridge_replay.py (再生・標準ライブラリのみ)、
  native/x86_bridge_diff.py (語単位の差)、native/verify_x86_bridge.sh (通し)。.gitignore に native/x86cap/ の例外。
- 試験 (verify_bridge --quick --only frame の記録 93 呼び出し): arm64 再生 93/93 一致。x86 再生 62/93 一致、差は nvc_temporal 31 件だけ
  (out・rv の 0.05〜0.5% の語が 1〜6 ULP。NaN なし。流れ場は一致)。
- 観測: libm の expf は arm64 と x86_64 で 2,018,697 個の float 入力 (全 2^32 を走査) で 1 ULP 違う。tanf は 5,931,082 個 (|x|<1 で 1 ULP、±inf で NaN の符号)。
  pow (k≥2) は標本の約 0.14%、sincos は標本の約 18% で 1〜2 ULP。atan2 は標本 4,066,049 個で差なし。
- 観測: openlibm の expf (W2a の libm_alt、NV_LIBM_ALT=1 NV_LIBM_ALT_FUNCS=expf) を両アーキで使うと、上の 80 呼び出し (temporal+spatial) は
  arm64 と x86 で全件一致 → x86 の temporal の差は expf (libm) だけで説明できる (この入力では NaN の順の問題は出ていない)。
- 全件の記録 (verify_bridge 全群 + --export) を実行中。

## 2026-10-30 昼
- 全件記録: verify_bridge.py 全群 + --export (記録しながら ALL MATCH、682 秒、記録 9.8 GB、38,667 呼び出し)。
- 再生: arm64 38,667/38,667、arm64 portable 38,667/38,667、x86_64 38,593/38,667 (差は nvc_temporal 72 件・nvc_nlc_image 2 件)。
  - temporal 72 件: openlibm の expf を両アーキで使うと 178/178 (temporal+spatial) 一致、差の出る呼び出しの集合も同一 → expf だけが原因。
    差は out/rv 432 面 1,472,151 / 337,651,200 語 (0.44%)、1〜8 ULP、NaN なし。
  - nlc_image 2 件: 入力 +inf・m=0 で 0×inf の既定 NaN が arm64 0x7fc00000 / x86 0xffc00000 (6 語)。
  - void 関数の戻り値 (レジスタの残り) は比較から外した。
- 試験バイナリの x86 版 (v5・profile・aft・lzma) を各 verify の入力で arm64 版と比較 (x86_exe_pair.py):
  spatial/temporal は NaN の語のビット (符号・ペイロード) と expf の差だけ。profile 2/2・lzma 3/3 一致、aft は finish/sim で NaN のビットのみ。
- 次: make verify (11 本)・素の verify_bridge で arm64 回帰確認。

## 2026-10-30 午後 (完了)
- 回帰 (arm64): クリーンビルド (all + lib_portable + lib_x86) 警告 0。make verify 11 本全件一致 (x86_g9/make_verify.log)。
  記録なしの verify_bridge.py 全群 ALL MATCH (x86_g9/vb_plain.log)。旧 Makefile との `make -n -B` 比較で all/lib/lib_portable/portable/x86 の
  コマンドは空白 1 個を除き同一 (SDK_XFLAGS が空)。
- 通しスクリプトの試運転: `native/verify_x86_bridge.sh --quick --only curve,depth` で 3487/3487 (arm64・portable・x86) ALL IDENTICAL。
- 証拠: sandbox/omo/wp_qtui_fix/x86_g9/ (再生結果 JSON、要約 summary_x86.md、libm 走査の C と結果、x86 試験バイナリ対の集計スクリプト)。
  大きな記録 (9.8 GB) とダンプは消した。

## 2026-10-30 夕 (統括の回答を受けて)
- expf は共通実装にしない (W2c どおり記録のみ)。
- verify_x86_bridge.sh に `--allow-known` を追加 (既定は厳密のまま)。分類は native/x86_bridge_known.py: (a) nvc_temporal は差が有限値の 1..8 ULP だけ
  (NaN が絡めば外れ。上限は定数 MAX_ULP=8 を表示)、(b) nvc_nlc_image は両側 NaN の語のビット違いだけ。ほかは 1 件でも失敗。
  合成配列で 8 ULP 合格・9 ULP/NaN/±0/有限値の nlc/他関数は不合格を確認。
- 全群 + --export を `--allow-known` で 1 回通した: 記録中の verify_bridge ALL MATCH (727 秒)、arm64 38,667/38,667、portable 38,667/38,667、
  x86 38,593/38,667 → 分類 temporal 72 呼び出し・432 面・1,472,151 語、nlc_image 2 呼び出し・2 面・6 語、範囲外 0 → rc 0 (29 分 26 秒)。
  ログは x86_g9/allow_known/。実行後に最後の表示行だけ「(x86_64: known differences only, --allow-known)」を足した (判定は不変)。

## 2026-10-30 夜 (統括の再確認で見つかった穴: 0 件の再生が成功になる)
- 修正: verify_x86_bridge.sh は再生の前に記録 (cap/cap_*.pkl) の有無を確かめ、無ければ失敗 (rc 1、理由を表示)。
  x86_bridge_replay.py は記録が無い・再生 0 件・`--require-groups` の群に記録が 1 件も無いとき rc 2 で失敗 (理由を表示)。
  必要な群は --only (+ --export なら export)、無ければ記録時の群 (cap/groups.txt に保存)、それも無ければ verify_bridge の全群。
  --allow-known の分類は x86 の rc が 1 (差あり) のときだけ行う (rc 2 は分類しない)。失敗の最終行を「REPLAYS FAILED (...)」に変更。
- 確認 (x86_g9/fix_zero/): (1a) 記録なしの作業ディレクトリで `--quick --only frame --no-capture` → FAIL・rc 1、(1b) 空の cap → rc 1、
  (1c) curve,depth の記録に `--only frame --no-capture` → 3 再生とも「no recorded call of the group(s) frame」・rc 1、
  (1d) 再生 0 件 (関数の絞り込みが空) → rc 2。(2a) `--quick --only curve,depth --keep` → 3487/3487×3・rc 0、
  (2b) その後の `--no-capture` (--only なし。groups.txt の curve,depth を要求) → rc 0、(2c) `--no-capture --only curve,depth` → rc 0。
- 前回の全群 --allow-known の実行は再実行していない。その JSON から群ごとの呼び出し数を数えると 10 群すべて 1 件以上 (fallback 3 〜 ui 19,424) で、
  新しい群の要求を満たす (オフラインの確認)。

## 2026-10-30〜31 (独立検証 REQUEST CHANGES: M1・L1〜L6 の修正)
- M1: verify_x86_bridge.sh は再生の前に replay_*.json・dump_x86・dump_arm64 を消す。x86_bridge_replay.py の「差あり」を rc 3 にした
  (0 一致 / 2 記録なし・0 件・群の不足 / 1 は Python の例外)。分類は rc 3 で、かつその回の replay_x86.json があるときだけ。
- L1: 分類 (a) は両側とも有限 (NaN でも ±inf でもない) の語だけ。L2: 再生の差に C の引数番号 (arg)・配列の要素番号 (entry) を記録し、
  分類は temporal の out/rv (引数 16/17)・nlc_image の出力 (引数 4) で esize 4・役割 out の領域だけ。arg の無い古い JSON は分類外。
- L6: 記録のラベルは、verify_* のフレームが無く、プロセスの __main__ / __mp_main__ が nv_ui_qt/export.py (か denoise_video_v5.py) のとき ("export", 関数)。
  ラベルの無い呼び出しは群 "unlabelled"。群の有無は nvc_abi_version を数えない (fallback だけは数える)。
- 確認 (x86_g9/fix_m1/): 分類器の合成試験 23/23 (L1 の ±inf/FLT_MAX、L2 の new_flow uint8・flow_out・役割 in・esize 8・arg なし を含む)。
  (A) `--quick --only curve,depth,frame --allow-known --keep` 成功 (x86 3546/3577、temporal 31 呼び出し・186 面・435,256 語、分類外 0、rc 0)。
  (B) 同じ作業ディレクトリで fakepy.sh を --py86 → 「simulated replay crash」・「1 REPLAYS FAILED」・rc 1 (古い結果は消えていた)。
  (C) その後 --no-capture --allow-known (本物の Python) → rc 0、(D) 厳密の --no-capture → rc 1。
  export: `--quick --only curve --export --allow-known` で export 群 48 呼び出し (temporal 8・spatial 24・abi 16)、unlabelled 0、rc 0。
- L4: x86_exe_pair の --report を作り直して x86_g9/exe_pair/ に保存 (spatial・temporal・aft・profile・lzma、v5 は事例ごとの表と NaN のビットの組も)。
