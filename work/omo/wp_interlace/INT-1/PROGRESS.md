# INT-1 経過 (作業者 int1、worktree .claude/worktrees/int1、branch int1-fields、base 19072bc)

依頼: .claude/tasks/interlace.md の INT-1 (field_order の検出と表示、nv_fields の分離/再結合、往復のバイト一致)。
統括の判断: PySide6 版は変更しない。qtui (MeatVideoQt) だけに表示する。

## 2026-10-31

- 読んだもの: タスクファイル、docs/INTERLACE_PROGRESSIVE.md、native/MIGRATION.md の「移植の重要な知見」と「C++ ファースト方針」。
- 観測: 「3-line offset」という語は docs/INTERLACE_PROGRESSIVE.md とタスクファイルにしかない
  (worktree 全体と reference/genuine/analysis を grep。解析成果物に根拠となる記述は見当たらない)。
- 観測: qtui の書き出し (export.cpp probe_full) は video.cpp の probe() と同じ ffprobe コマンドを trace に書き、
  compare_export.py が Python 側のコマンドと語単位で比べる。probe() のコマンドを常に変えると書き出しの照合が崩れる。
- native/nv_fields.{h,cpp} と試験 nv_fields_main.cpp (nv_fields_test)、照合 native/verify_fields.py を作成。
  Makefile: all に nv_fields_test、LIB_SRC に nv_fields.cpp (C ABI は足さない)、verify に verify_fields.py。
- 観測 (nv_fields_test selftest): 508,002 検査で不一致 0 (uint8/uint16/float32 × H 0..33 × W 8 種 × stride 2 種、大きい 8 サイズ、4:2:0/4:4:4)。
- 観測 (verify_fields.py 既定): 乱数の 96 ケース (6 形式 × 16 サイズ、1x1 〜 1920x1080) で numpy の p[0::2]/p[1::2] と一致、join で入力とバイト一致。1.7 秒。
- 観測 (--make-clips、ffmpeg 8.0.1): test_v02 を 60p と読み替えて tinterlace → libx264 -qp 0 (+ilme+ildct) で 1080i30 tt と bb、
  test.mp4 から 360i15 bb を 8 フレームずつ作成。ffprobe の field_order は tt / bb / bb。
  - 4 形式 (yuv420p, yuv420p10le, yuv444p, gbrpf32le) で分離が numpy と一致し、再結合が元とバイト一致 (各 8/8)。
  - yuv420p で ffmpeg の separatefields の出力と 3 面とも一致 (8/8 × 3 本。tt は top が先、bb は bottom が先)。
  - 各フィールドが、作る元にしたソースのフレームのフィールドと 3 面とも一致 (8/8 × 3 本)。4:2:0 の色差を面ごとの行の偶奇で分ける規約は、ffmpeg (tinterlace・separatefields・x264 の interlaced 符号化) と同じ。
  - 失敗例: x264 の bb は `-top 0` や setfield=bff だけでは tt のまま。`-x264-params bff=1` が必要。ffv1 の mkv は bff の素材が "bt" と出る。
- 観測 (感度): -DNVF_MUT_{NARROW,PADWRITE,PADROW,CHROMA} の 4 つの変異は、自己試験と numpy 比較の両方で検出された。
  CHROMA (色差を 1 つのフィールドとして分ける変異) は、実素材でも separatefields との一致が 0/4 になった。
- qtui: video.{h,cpp} の probe(path, with_field_order) と field_order_text()、mainwindow のステータスバーに scan_lbl を追加。
  UI の open_video だけが field_order 付きで呼ぶ。書き出し (export.cpp probe_full) は従来のコマンドのまま (compare_export.py の語単位の照合のため)。
  OFX モードでは scan_lbl を空にする (ホストのフィールド属性は INT-4)。
- 自己試験に 3 項目 (fieldOrderText・fieldOrderProbe・fieldOrderShownOnOpen) と loadAndAutoProfile への 1 行を追加。
  観測: 4 項目とも PASS。感度: probe から field_order を外す変異で fieldOrderProbe・loadAndAutoProfile・fieldOrderShownOnOpen の 3 項目が FAIL (元に戻して再ビルド済み)。
- 回帰 (観測、make -C native all を警告 0 で再ビルドの後): verify_quality 4000/4000、verify_band 5706/5706、verify_curve ALL BIT-EXACT、
  verify_lzma 667/667、verify_flat ALL BIT-EXACT、verify_flat_cancel ALL IDENTICAL。
- 回帰の続き (観測): verify_model 1244/1244 (初回は Python のキャッシュが空で負荷下 9452 秒、再実行 59 秒)、verify_temporal 474/474、
  verify_spatial 1287/1287、verify_profile 39/39 (PASS)、verify_aft ALL BIT-EXACT (67 件)。これで make verify の 11 本すべて一致。verify_fields も ALL BYTE-EXACT。
- qtui_selftest 全件 (観測): 118 項目 + initTestCase/cleanupTestCase で 120 passed, 0 failed (23 秒)。
- compare_export (観測): 19 ケースすべて SAME (出力とコマンド列)。qtui/tools/compare_export.py は ROOT/repro を参照していて
  再構成後の配置では動かないので、ROOT と REPRO (reference/python) だけ書き換えた写しを scratchpad で実行した (worktree の tools は変えていない)。
- libnvcore への影響 (観測): nv_fields.o が定義する外部シンボルは nvf:: だけで、参照するのは memmove だけ (nm)。
  **訂正 (検証 I-1)**: 「弱い定義はなく」は誤り。明示的インスタンス化 30 個が weak external だった (下の修正 2 で解消)。
  ほかの翻訳単位のソースとフラグは変えていない。ただし dylib の配置 (アドレス) は変わる。C ABI は不変 (NVC_ABI_VERSION 3 のまま)。
- 最終の verify_fields (--make-clips --sensitivity): ALL BYTE-EXACT、4 つの変異をすべて検出。

## 2026-10-31 修正 2 (VERDICT.md の L-1・L-2・I-1・I-2・I-4)

- L-1: 走査方式の判定を最初のフレームの interlaced_frame / top_field_first で行うようにした (qtui video.{h,cpp}: VideoInfo.frame_interlaced / frame_tff、
  Scan scan_of()・first_field()・scan_text()。field_order_text() は廃止)。UI の probe は ffprobe を 2 回呼ぶ (ストリームの呼び出しは従来どおり、
  2 回目が `-read_intervals %+#1 -show_entries frame=interlaced_frame,top_field_first`)。書き出しの probe は不変。
  - 観測 (ffmpeg 8.0.1): ffv1 (mkv・mov) と prores_ks (-flags +ildct) は TFF のフレームでストリームが tb、BFF で bt。フレームの旗は正しい (TFF 1/1、BFF 1/0)。
    prores_ks を -flags +ildct なしで作ると、フレームの旗は interlaced_frame 0 で、ストリームだけが tb/bt。x264 は tt/bb とフレームの旗が一致。
  - 1 回目に試した「同じ呼び出しに frame= を足す」形は、stream の出力に start_time が 1 つ増えるので採らなかった (別の呼び出しにした)。
  - 時間: 2 回目の ffprobe は test.mp4 で 35〜37 ms、test_v02 (1080p) で 79〜81 ms (ストリームの呼び出しは 28〜30 / 50 ms)。各 5 回。
- L-2: join / join_frame は、フィールドが dst と同じメモリを使うとき、dst 自身の field_view (data = dst.data + parity·stride、stride = 2·dst.stride) だけを受け付け、
  ほかの重なり (2 つのフィールドを dst の上下半分に詰めた配置など) は false を返して何も書かない。split / split_frame は読み書きの面の重なりをすべて拒否する。
  重なりは行の範囲 [最初の行, 最後の行 + W) で判定する (面の stride の隙間に別の面を置く配置も拒否される)。注釈も書き直した。
- I-1: 公開関数をテンプレートの明示的インスタンス化から普通の多重定義 (uint8_t・uint16_t・float) にした。実装のテンプレートは無名名前空間。
  観測: nv_fields.o の weak は 0、libnvcore.dylib の weak は 0 で、Mach-O のフラグは HEAD の dylib と同じ (WEAK_DEFINES なし)。
  名前空間 nvf は nv_flat (FindFlatArea) がすでに使っていたので、nvfield に改名した。公開シンボルは nvfield:: の 31 個。
- I-2: W = 0 または data = NULL の面のビューは data = pad = NULL・stride 0 とし、NULL へのずらしをしない。copy の各経路も W = 0 なら何も計算しない。
  観測: ASan + UBSan (-fno-sanitize-recover=all) で自己試験 528,636 項目が不一致 0・報告 0。
- I-4: 自己試験に ofxHidesScan を追加 (先に "stale" を入れ、set_ofx_frames の後で文字列とツールチップが空)。
- 自己試験: fieldOrderText (17 組の表)、fieldOrderProbe (test.mp4 と x264 / ffv1 / prores の TFF・BFF の 6 本)、fieldOrderShownOnOpen を更新。
- verify_fields.py: separatefields の順序をフレームの旗で決める (旗がなければ tt/bb、tb/bt だけでは決めない)。色差の高さが奇数 (H ≡ 2 mod 4) のときは、
  下フィールドの最後の色差の行 (パッド行) を separatefields との比較から外して数を表示する (I-3)。元が yuv420p でない素材 (prores の yuv422p10le) は、
  yuv420p への変換がフィールドを区別しないので separatefields の比較を省く。変異に NVF_MUT_NOALIAS を追加。

### 修正 2 の検証 (観測)
- nv_fields_test selftest: 528,636 項目で不一致 0 (ASan/UBSan でも同じ)。
- verify_fields.py (--make-clips --sensitivity と --video 9 本): ALL BYTE-EXACT。分離と再結合は 12 本 × 4 形式の 48/48。
  separatefields: ffv1 tb 3/3 (上が先)・bt 3/3 (下が先)、x264 tt/bb、ffv1 720×486 bt 3/3 (パッド行 6 面を除外)。prores の 2 本は省略 (理由は上)。
  -flags +ildct なしの prores (旗 0、ストリーム tb) は順序不明として比較しない。
- 変異 5 種すべて検出 (NOALIAS は自己試験だけが検出: 2,730 件)。
- qtui: L-1 の旧規則に戻す変異で fieldOrderText と fieldOrderProbe が FAIL (元に戻して cmp で確認、再ビルド)。
- qtui_selftest 全件: 121 passed, 0 failed (119 項目 + init/cleanup、24 秒)。
- compare_export: 19/19 SAME (3 回に分けて実行、写しは scratchpad の compare_export_int1.py)。
- ビルド警告 0 (make -C native all lib、cmake --build qtui/build_main)。
