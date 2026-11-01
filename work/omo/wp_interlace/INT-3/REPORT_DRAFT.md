# INT-3 作業者報告 (int2-worker、SendMessage 3 通の転記。統括が転記、本文は無改変)

INT-3 報告 (1/3): INT-3 の 1〜5 を実装した。回帰はすべて変更前と一致 (2/3)、OFX は変更していない。この通は設計と変更ファイル。worktree は .claude/worktrees/int2 (INT-2 の上)。コミットはしていない。native/ は INT-3 では変更していない。

## 設計の決定
### 1. 走査方式 (Video Scan Type)
- **置き場所**: 下段の左にあった無効の「P ▾」ボタンを、実際に使えるようにした (本家と同じ位置)。
- **選択肢**: Progressive / Interlaced, top field first / Interlaced, bottom field first。表示は P / TFF / BFF ▾。
- **既定**: INT-1 の判定 (最初のフレームの旗) をそのまま使う。
- **順序が不明なとき** (scan_of = Interlaced、first −1、例: prores の +ildct なし):
  - 本家の文言 "Please specify the scan type of the current video clip." の非モーダルな箱を出す。ボタンは 3 つ。
  - 答えるまでは progressive で扱い、Auto Profile は走らせない。
- **変更したとき**:
  - プロファイルを空にし、本家の文言 "The noise profile has been reset because the scan type of the clip has changed." を出す。この文言には製品名が含まれていないので、そのまま使った。
  - 色差の読み方が変わる素材 (下記 3) なら、動画を読み直す。
- **OFX モード**: progressive に固定し、ボタンは無効にした (INT-4 の範囲)。

### 2. プロファイル
- **JSON**: base に `"fields": "two"|"one"` と `"field": "top"|"bottom"` を足した。
  - "field" の意味: One field では、プロファイルを作ったフィールド。Two fields では、領域があるフィールド (時間で最初のフィールド)。
  - `"fields"` が無い JSON (旧ファイルのすべて) は frame として読む。値が不正なときも frame。
  - **progressive では "fields" を書かない。** そのため既存の JSON と照合は変わらない。
- **One field (Top / Bottom)**: その偶奇のフィールド系列に対する backend.build_profile と同じ。
  - pair と中央値の曲線は、同じ偶奇のフィールド f+1, f+2。
- **Two fields**: 本家での定義は解読されていない。こちらで次のように決めた。
  - 領域を探す画像: 時間で最初のフィールド A_f。
  - 中央値をとる曲線: A_f と、時間で次の 2 フィールド (B_f, A_{f+1})。両方の偶奇の曲線が入る。
  - 静止画素の pair: A_{f+1}。逆の偶奇のフィールドとは半ラインずれるので、同じ偶奇を選んだ。
  - nvc_build_profile_v5 の pair と more を別々に渡すため、nvcore に build_profile_planes を追加した。progressive の build_profile はこの関数に同じポインタを渡すので、結果は同じ。
- **UI**:
  - Prepare ページに、インターレースのときだけ「Two fields / One field (top) / One field (bottom)」を出す。
  - 手動で選んだ領域は、フィールドの行に変換する (y0/2、h/2)。
  - Device Noise Profile パネルの表示: フィールドのプロファイルでは "Frame size: …i"、"Scan type: Interlaced"、"Profiled with: Two fields (640x180)" または "One field, bottom (…)"。フレームのプロファイルでは従来の文字列のまま。
  - フィールドのプロファイルの Main Sample の枠は、フレームの座標に戻して描く。
  - AFT・Quality/Completeness・Match のモデルは、フィールドの画像で計算する。
  - プロファイルの種類と走査方式が食い違うと、ステータスに注記を出す。処理は続ける。

### 3. 処理と読み込み
- **パラメータの渡し方**: MainWindow::proc_params() が、インターレースのときだけ backend の P に `"interlace": {"first","mode","radius"}` を足す。progressive の P と ui_params.json は 1 文字も変わらない。
  - プレビュー、NLC、Profile Check、Check Speed、書き出しは、この P を使う。
- **処理** (qtui/src/core/fields.cpp の process_fields): nvfp::process_frame_fields と同じ流れ。
  - 時間段: nvcore の temporal に shift を渡す (nvc_temporal_shift)。
  - 空間段と Mix: UI の finish (成分ごとの空間段 + Mix)。
  - 再結合: join。
- **プレビューの選択範囲**: 行をフィールドの境目に揃えて広げてから処理し、処理後に元の範囲を切り出す。境目の単位は 4:2:0 で 4 行、4:4:4 で 2 行。
- **NLC のマスク**: 時間段で平均したフィールドの数 n を使う。
- **読み込み** (検証者の L-a): 処理の形式が 4:2:0 で、ソースが 4:2:0 でない (4:2:2・4:4:4・RGB・440 など) インターレース素材は、`scale=interl=1` を付けて読む。
  - プレビュー (video.cpp の read_frames に vf 引数を追加) と書き出しの両方。
  - 4:2:2 を高ビットで読む経路は横方向に広げるだけ、RGB は 4:4:4 なので、フィールドは混ざらない。

### 4. 書き出しの旗
- **全コーデック**: 符号器の入力に `setfield=tff|bff` を付ける。奇数サイズの crop があれば、同じ -vf に連ねる。
- **x264**: `-flags +ilme+ildct -x264-params tff=1|bff=1`。bff は x264 自身の旗が要る。
- **prores_ks**: `-flags +ildct`。
- **FFV1**: setfield だけでよい。
- **x265 と EXR**: フレームに旗が立たないので、警告を出す。
- **ログ**: "interlaced: top field first, fields mode same, Temporal Radius 4 fields[, read with scale=interl=1]" を出す。sidecar の .json に "interlace" を足す。

### 5. 既定のモードと半径 (仮。最終は INT-4)
- **mode = SAME (同じ偶奇)**。根拠は INT-2 の表で、Y の全体の PSNR が最良だった。
- **半径**: 既定は「Temporal Radius のスライダーはフレームで数え、フィールド半径 = 2R」。
  - R=2 なら SAME で ±2・±4 フィールドとなり、フレーム処理の R=2 と同じ時間幅・同じ標本数になる。
  - INT-2 の数値 (TFF、σ6): SAME R4 は静止 1.35 / 動き 4.05。フレーム処理 R2 は 1.24 / 6.40、SAME R2 は 1.64 / 3.81。
- **変更できる設定**: Preferences の「Interlaced clips (fields)」で変えられる。
  - Temporal neighbours: same / shift / plain
  - Temporal Radius counts: frames (=2R) / fields (=R、本家の読み方)
- **設定の保存先**: `interlace/field_mode` と `interlace/field_radius`。理由: Python 版と共有している prefs/ のキー集合と、Preferences の行の照合 (selftest の preferencesDialogRows / AcceptWritesKeys) を変えないため。
- **UI での見せ方**:
  - スライダーはフレームのまま。
  - Adjust ページのツールバーに "Fields: TFF · same parity · radius 4 fields" を出す。
  - タイムラインの時間窓 (オレンジ) は (フィールド半径 + 1)/2 フレームにする。

## 変更したファイル
- **新規**
  - qtui/src/core/fields.{h,cpp}
  - qtui/tests/fieldprof.cpp (qtui_fieldprof)
  - qtui/tools/verify_interlace.py
- **変更** (qtui/)
  - CMakeLists.txt (qtui_fieldprof)
  - src/core/nvcore.{h,cpp}: temporal の shift、build_profile_planes、profile_numbers(Frame)
  - src/core/engine.{h,cpp}
  - src/core/export.cpp
  - src/core/video.{h,cpp}: read_frames の vf、interlaced_vf
  - src/app/mainwindow.{h,cpp}
  - src/widgets/profile_panel.cpp
  - tests/selftest.cpp: 6 項目を追加
- **控え**: work/omo/wp_interlace/INT-3/backup/ (14 本、SHA256SUMS.txt 付き)
- **ログ**: work/omo/wp_interlace/INT-3/logs/

INT-3 報告 (2/3): 試験の数値 (どれも自分で実行。ログは work/omo/wp_interlace/INT-3/logs/)。プログレッシブはすべて変更前と一致し、インターレースの書き出しとプロファイルは Python の参照とビット一致した。

## プログレッシブが変わっていないこと
- **書き出し**: compare_export の写し (ROOT/REPRO だけを直したもの)
  - 新しい qtui_export と Python: 19/19 SAME、RESULT ALL SAME。出力もコマンド列も同じ (compare_export_new.log)。
  - 変更前の qtui_export (INT-2 時点の build_main の写し) でも ALL SAME (compare_export_base.log)。
  - 新旧の C++ 出力を直接比べると 41/41 SAME (export_base_vs_new.log)。比べたのは 19 ケースの出力 (mp4・ts・mov・EXR はバイト、mkv は framemd5 + サイズ) と sidecar の json (seconds と src を除く)。
  - qtui_export のバイナリ自体は変わっている (sha 144e5b19… → 92d76024…)。
- **プロファイルの JSON**: progressive では "fields" を書かない。これは selftest の int3ProfileJsonFields で確かめた (progressive の build_profile に "fields" と "field" が無い)。旧 JSON は frame として読む。
- **`make -C native verify`** (14 本): rc=0 (make_verify.log)。
- **`cd native && python3 verify_bridge.py`** (full): ALL MATCH、858 秒 (verify_bridge_full.log)。
  - frame 72/72 (306,334,381 語)、depth 208,737,760 語、cancel 49、g8 152、ほか。
- **qtui_selftest**: 136 passed / 0 failed (130 + 新規 6)。
  - preferencesDialogRows と preferencesAcceptWritesKeys は、設定を interlace/ に移した後に PASS した。
  - 新しい 6 項目と、INT-1 / Preferences の関連 3 項目は、selftest_each で 1 項目ずつ単独でも PASS。
- **`ofx/tools/verify_ofx.py --quick`**: 81 比較で 0/67,028,480 語の不一致、ALL MATCH (ofx を作り直してから)。
- **クリーンビルド**: qtui を新しいディレクトリでビルドして warning 0。build_ofx.sh も warning 0。
- **ini**: sha256 45d4258c…fcc6 は変わっていない。

## インターレース (qtui/tools/verify_interlace.py、verify_interlace.log)
- **素材**: 合成の 60p (320×180、動く円 + ノイズ) を tinterlace で TFF と BFF にし、x264 の可逆符号化 (インターレース符号化) で保存した。4:2:2 の FFV1 版も作った。
- **書き出し (C++) と Python の参照の比較**
  - 参照: qtui と同じ読み方をしたフレームに fields_ref.process_frame_fields を掛け、rint と clip をしたもの。
  - 結果: 6 ラン・64 フレームで不一致 0 バイト (BIT-EXACT)。
    - same R4 (TFF・BFF)
    - shift R2
    - plain R2 + Mix 0.25
    - 範囲指定 (--start 3 --frames 4)
    - 4:2:2 素材の interl=1 読み込み
- **プロファイル**: qtui_fieldprof (build_profile_fields) を、nvfilter.build_profile_v5 にフィールドを渡したものと比べた。
  - Two と One、top と bottom、端のフレームの 4 件で、dict の全値 (float も厳密に) が一致した。"fields"・"field"・"ui_frame" も正しい。
- **出力の旗** (ffprobe の最初のフレームの interlaced_frame と top_field_first)

| 出力 | 旗 | stream の field_order |
|---|---|---|
| x264 mkv TFF | 1/1 | tb |
| x264 mkv BFF | 1/0 | bt |
| x264 mp4 8bit BFF | 1/0 | bb |
| x264 mov 10bit BFF | 1/0 | bb |
| FFV1 8bit・16bit BFF | 1/0 | bt |
| FFV1 10bit TFF | 1/1 | tb |
| ProRes 10bit TFF | 1/1 | tb |
| x265 mp4 | 0/0 (旗なし) | bt |

  - stream の tb/bt は ffmpeg の癖で、INT-1 の L-1 と同じ。
  - x265 は旗が立たないので、警告を出す仕様にした。
- **4:2:2 の読み込み**: interl=1 ありとなしで、色差 92,357 標本が異なる。書き出しは interl=1 の方で参照とビット一致した。
- **空振りしないこと**: qtui の写しに変異 5 種を入れてビルドし、verify_interlace を回した。5 種とも FAILURES になった (mutants.log)。
  - PARITY (first を無視): plain で 255,074 バイトの不一致。
  - SHIFT_SIGN: shift で 120,821 バイト。
  - PAIR_TWO (Two fields の pair を逆の偶奇に): プロファイルの band_ratio などが不一致。
  - NO_INTERL: 4:2:2 で 44,757 バイト。
  - SETFIELD_SWAP: 旗の WRONG が 12 行。
- **selftest の新しい 6 項目**
  - int3ScanDefaultsAndFieldProfile: TFF の素材で、走査方式の判定、TFF ▾ の表示、Two fields のプロファイル、パネルの表示、P の interlace、プレビューと process_fields の一致、選択範囲の大きさ、One field (bottom) を確かめる。
  - int3ProfileJsonFields
  - int3ScanChangeResetsProfile: 本家の文言、BFF / P への切り替え。
  - int3PrefsFieldMode: shift と fields → radius 2。設定は interlace/ に保存され、prefs/ には書かれない。
  - int3ExportKeepsFieldOrder: UI からの書き出しで 1,1 と 1,0。ui_params.json に interlace がある。
  - int3UnknownOrderAsks: prores の +ildct なし → 質問が出る → BFF を選ぶ → interl=1 で読み直し、フレームが read_frames(…, "scale=interl=1") とバイト単位で一致する。

INT-3 報告 (3/3): 残るリスクと未対応の点。INT-4 の本家比較で確かめてほしいものを先に挙げる。

## 本家と照合していない設計の決定
1. **Two fields の定義**
   - こちらで決めた内容: 領域は最初のフィールド。曲線は A_f・B_f・A_{f+1} の中央値。pair は同じ偶奇。
   - 本家の Two fields は解読されていない。本家は「フィールドごとに別のプロファイルを持つ」可能性もある。
   - INT-4 で本家の数値 (Noise Level など) と比べるべき。
2. **既定の SAME と「半径はフレーム数、内部では 2R フィールド」は仮のもの**
   - 本家が R をフィールド数で数えるとすると、本家の R=2 は、こちらの設定では「Temporal Radius counts: fields」の R=2 に当たる。
   - INT-4 では両方の設定で比べてほしい。
   - 静止部がフレーム処理より 9% ほど悪い点 (INT-2 の L-b) は残っている。
3. **走査方式を変えたときのリセット**
   - Undo の対象になっている (ProfileCommand)。Undo すると古いプロファイルが戻るが、走査方式は戻らない。この食い違いはステータスの注記で知らせるだけにした。
   - 走査方式は、セッション・プリセット・プロファイルのどれにも保存しない。動画を開くたびに判定し直す。

## 未対応の範囲
4. **Batch Profiler** は、インターレースの素材にもフレームのプロファイルを作る (フィールドに対応していない)。
5. **Auto Match・Match の行**: インターレースでは、現在のフレームの最初のフィールドの画像で計算する。保存されたフレームのプロファイルと、フィールドのプロファイルが混ざっている場合の比べ方は詰めていない。
6. **x265 (HEVC) と EXR**: フィールドの旗を書けないので、警告だけ出す。
   - x265 の interlace オプションは、フィールドを別々の画像として入力する方式なので使わなかった。
   - 推奨は x264・ProRes・FFV1。
7. **インターレースの判定**: INT-1 と同じく、最初のフレームだけで決める。PAFF のように途中で順序が変わる素材は対象外。

## 実装上の注意
8. **4:2:2 / 4:4:4 の 8 ビット読み込み** (interl=1)
   - 走査方式を変えると、動画を読み直す (メモリ上のフレームを作り直す)。
   - 読み直しの間にプロファイルは空になり、Auto Profile が設定どおり走る。
9. **プレビューの選択範囲**
   - フィールドの境目に揃えた範囲で処理し、処理後に切り出している。そのため、同じ範囲のフレーム処理とは端の画素が違う。これは progressive の選択範囲処理が全画面と違うのと同じ性質。
   - フィールドの時間段には flow のキャッシュがない。フィールドごと・ずれごとに別になるため。
10. **NLC のマスク**: 最初のフィールドの時間窓のフィールド数で正規化している。クリップの端では、2 つのフィールドで数が違いうる。
11. **OFX**: ofx/ は変えていない。OFX のヘルパー (MeatVideoQt) の中では progressive に固定し、走査方式のボタンは無効にした。
    - qtui のコードは OFX バンドルにも入るので、build_ofx.sh を作り直して verify_ofx --quick を回した。結果は ALL MATCH。
12. **qtui/tools/verify_interlace.py の位置**: compare_export と同じく qtui/tools にある。ROOT は worktree なので、そのまま動く。
    - 変異の試験に使ったスクリプトは scratchpad (int3/mutants.sh) にあり、リポジトリには入れていない。
13. **既存の問題 (未修正)**
    - compare_export.py の ROOT/repro。写しで実行した。
    - make verify の JOBS と verify_fields --jobs。
    - verify_ofx が qtui/build を前提にしていること。
14. **work/omo/wp_interlace/INT-3/** (backup・logs) は .gitignore の対象なので、追跡するなら `git add -f` が要る。

## 守ったこと
- 本家・Resolve・/Library/OFX・ユーザーの MeatVideoQt に触れていない。
- reference/ (処理コアの Python・PySide6 版) は変更していない。
- AGENTS.md・MIGRATION.md は変更していない。
- コミットしていない。
- make clean はしていないので、nv_quality_test は変わっていない。


---

# Round 2 作業者報告 (転記、本文は無改変)

INT-3 Round 2 の報告: M-1・L-1・L-2 を直し、回帰はすべて一致した。L-3 (Undo と走査方式) は指示どおり手を付けていない。native/ は Round 2 でも変更していない (INT-2 の差分のまま) ので、`make -C native verify` は省いた。コミットはしていない。

## 変更
**M-1 (32-bit float で interl=1 が付かない件)**
- `interlaced_vf` を `interlaced_vf(vf, src_pix_fmt, out_chroma)` に変え、元の vf を受け取って、付け足した vf を返す形にした (video.h/.cpp)。
- interl=1 を付ける条件は、「色差の縦方向の標本化が変わる変換」かどうか。つまり、ソースが 4:2:0 かどうかと、処理の形式が 4:2:0 かどうかが食い違うとき。
  - vf が空なら `scale=interl=1` にする。
  - vf が単独の scale (read_filter の `scale=in_color_matrix=…:in_range=…`) なら `:interl=1` を足す。
  - それ以外は `,scale=interl=1` を連ねる。
- 新しく付くようになった組み合わせ: 4:2:0 → RGB (32-bit float・gbrp)。8 ビットの非 4:2:0 → 4:2:0 は従来どおり付く。
- 付かない組み合わせ: 4:2:0 → 4:2:0 の高ビット、4:2:2/4:4:4 → 高ビット、RGB → RGB。
- 呼び出し元を 3 か所とも直した。
  - export.cpp: `if (il) vf = interlaced_vf(vf, …)`
  - open_video: 同じ形
  - set_scan の読み直しの判定: read_filter の vf を基にして比べる
- 控えは INT-3/backup_r2/ (SHA256SUMS.txt 付き)。

**L-2**
- 4:2:0 で「順序不明 → 利用者が答える」とき、答えた後に、Auto Profile の設定が有効で、プロファイルが無く、読み直しもしない場合は Auto Profile を走らせる。読み直す場合は、読み直しの側で走る。
- 読み直しを伴う切り替えでも、プロファイルを空にしたときは本家の文言の箱を出す。

**L-1 (verify_interlace の補強)**
- 手動の領域があるプロファイルを 2 件足した: two top (37,41,70,90) と one bottom (90,150,64,64)。qtui_fieldprof に AREA 引数を足した。期待値は y0//2 と max(h//2, 8)。
- ストリームの field_order の期待値を足した。
  - x264 の mp4・mov: TFF → tt、BFF → bb。x264 mp4 の TFF のケースも足した。
  - mkv・ffv1・prores: tb / bt (ffmpeg 8.0.1 の値)。
- 奇数の半径を足した: plain R3、same R5、shift R3 (範囲指定あり)。
- depth 32 のケースを足した。TFF の 4:2:0 素材を gbrpf32le の FFV1 で書き出す。
  - reference は、qtui と同じ `scale=in_color_matrix=bt601:in_range=tv:interl=1` で読み、nv_depth.split と to_internal を通して fields_ref を掛け、from_internal の float32 バイトにしたもの。

## 試験 (ログは INT-3/logs/*_r2.log)
- **verify_interlace**: ALL OK。
  - プロファイル 6 件が全値一致 (うち領域あり 2 件)。
  - 書き出し 9 ラン (奇数の半径 3 本を含む) で、不一致 0 バイト。
  - depth 32: 5 フレームで float 語の不一致 0。なお interl=1 を付けないと、0 フレーム目の RGB 172,800 標本のうち 170,925 が異なる。
  - 旗: x264 mp4 は tt (1,1) と bb (1,0)、mov 10 ビットは bb、mkv は tb/bt。ffv1 と prores も期待どおり。x265 は旗なし (警告)。
- **qtui の写しに入れた変異 9 種**: すべて FAILURES になった (mutants_r2.log)。
  - 検証者が素通りすると指摘した 3 種は、今回は検出した。
    - AREA_NOHALF: 領域ありのプロファイルが DIFFER。
    - X264_NO_BFF: mp4・mov のストリームが tt になり UNEXPECTED。
    - MARGIN: R3 で 99,510 / 62,990 バイトの不一致。
  - M-1 を元に戻す変異 D32_NO_INTERL: depth 32 で 863,975 語の不一致。
  - 従来の 5 種 (PARITY・SHIFT_SIGN・PAIR_TWO・NO_INTERL・SETFIELD_SWAP) も検出した。
- **qtui のクリーンビルド**: 新しいディレクトリで warning 0。
- **selftest**: 137 passed / 0 failed。
  - 新しく足した項目は int3UnknownOrder420AutoProfile。stream が tb でフレームに旗の無い 4:2:0 の mkv で、質問が出る → TFF を選ぶ → 読み直さない → Two fields の Auto Profile と AFT が走ることを確かめる。
  - int3UnknownOrderAsks に、読み直しのときも本家の文言が出ることの確認を足した。
  - この 2 項目は単独でも PASS。
- **compare_export** (写し): 新しい qtui_export と Python で 19/19 SAME、ALL SAME。
- **変更前の qtui_export との比較**: 41/41 SAME。
- **OFX**: build_ofx.sh の warning は 0。verify_ofx --quick は 81 比較で 0/67,028,480 語の不一致、ALL MATCH。
- **ini**: sha256 45d4258c…fcc6 は変わっていない。

## 残る点
- L-3 (Undo と走査方式) は INT-4 で扱う。
- 情報として指摘された 2 点は、どちらも UI からは届かない。
  - 3 要素の area は辺を半分にしない。
  - 17 行のように小さい選択範囲では例外が出る。
- 440 のソースを RGB で読むときは、source_format が 440 を 4:4:4 として扱うので、interl=1 が付かない。440 は縦方向に色差を間引いているので、本来は付けるべきもの。まれな形式なので記録だけにする。
