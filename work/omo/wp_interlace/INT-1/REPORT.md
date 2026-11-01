# INT-1 REPORT — 走査方式の取得と表示 + フィールドの分離と再結合 (作業者 int1)

転記: 統括 (2026-10-31)。作業者の最終報告をそのまま転記した。経過は `PROGRESS.md`。タスクの文面は「PySide6 版にも表示」だが、同じファイルの共通規約と C++ ファースト方針に従い、統括の判断で qtui だけに表示した (PySide6 版は凍結)。統括の再確認と独立検証は末尾に追記する。

## INT-1

INT-1 は完了しました。field_order の取得と表示、`nv_fields` の分離と再結合、往復のバイト一致を実装し、検証もすべて通っています。PySide6 版は変更していません。C ABI も変えていません (`NVC_ABI_VERSION` は 3 のまま、`nv_native.py` も変更なし)。コミットはしていません。

## 作ったファイル・変えたファイル (worktree `/Users/seitan/Desktop/work/reverse/.claude/worktrees/int1`)

新規:
- `native/nv_fields.h` / `native/nv_fields.cpp` — 分離と再結合の本体。
- `native/nv_fields_main.cpp` — 試験用の実行ファイル `nv_fields_test` (自己試験モードと、生フレームを読むモード)。
- `native/verify_fields.py` — 照合。既定は乱数のみ。`--video`、`--make-clips`、`--sensitivity` を付けると実素材と変異の検出も行う。
- `work/omo/wp_interlace/PROGRESS.md` — 経過の記録。

変更:
- `native/Makefile` — `all` に `nv_fields_test` を追加、`LIB_SRC` と `LIB_HDR` に `nv_fields` を追加、`verify` に `verify_fields.py` を追加。
  - `make verify` に入れた理由: ffmpeg も動画も要らず、約 1.7 秒で終わるので、退行の検出に使えるためです。
- `qtui/src/core/video.{h,cpp}`
  - `probe(path, with_field_order = false)` と `field_order_text()` を追加。
- `qtui/src/app/mainwindow.{h,cpp}`
  - ステータスバーに `scan_lbl` を追加。
  - ツールチップに ffprobe の値と「表示のみ」の旨を出す。
  - OFX モードでは空にする (OFX の属性は INT-4 で扱う)。
- `qtui/tests/selftest.cpp` — 3 項目 (`fieldOrderText`・`fieldOrderProbe`・`fieldOrderShownOnOpen`) を追加し、`loadAndAutoProfile` に 1 行加えた。

## 設計

**field_order の取り方**
- 動画を開くときの ffprobe の呼び出しに `field_order` を足しました (別の呼び出しは増やしていません)。
- 書き出し側の probe は従来のコマンドのままです。`compare_export.py` が Python 版とコマンドを語単位で比べているため、変えると照合が崩れます。
- `tb` は「下が先に表示」、`bt` は「上が先に表示」と表示します (ffmpeg の AVFieldOrder の定義どおり)。
- `unknown` や値が無いときは "scan unknown" と出します。

**ビューの形**
- top は `data` から、bottom は `data + stride` から始まり、どちらも行の間隔は `2·stride` です。コピーはしません。
- 高さはどちらも ceil(H/2) です。
- コピーする版 (`split`・`split_frame`) と、ビューから組み立てる `join` も用意しました。
- 型は uint8 / uint16 / float32 で、どれも行単位の memmove なので、NaN の中身や −0 もそのまま残ります。

**H が奇数のとき**
- bottom の実際の行は floor(H/2) です。足りない最後の 1 行 (パッド行) は、同じフィールドの最後の行 (フレームの行 H−2) を繰り返します。H=1 のときは行 0 を使います。
- 理由: 1 つのフィールドは同じ瞬間の画像なので、端の補いは自分のフィールドの行で行うのが自然だからです。
- `join` はパッド行をフレームに書き戻しません。
- ビューでは、パッド行だけが `pad` ポインタを指します。そのためパッド行の位置だけは「先頭 + 行番号 × 間隔」で計算できません。

**4:2:0 の色差**
- 輝度と同じく、色差の面も行の偶奇で分けます (色差の行 c はフィールド c&1 に属する)。
- 1 つのフィールドはそれ自体が 4:2:0 の画像になります (色差の高さは ceil(ceil(H/2)/2))。
- 理由: ffmpeg の tinterlace・separatefields と、x264 のインターレース符号化の解読結果が、どれもこの規約だったためです (下の照合で確認)。
- 本家がどう扱っているかは分かっていません。

**「3-line offset」の解釈 (推測)**
- この語は、docs とタスクの文面にしか出てきません。worktree 全体と `reference/genuine/analysis` を検索して、根拠になる記述は見つかりませんでした。
- インターレースの 4:2:0 の行の構造のことだと解釈しました。上のフィールドの色差の行はフレームの輝度の行 4k と 4k+2 に、下のフィールドの色差の行は 4k+1 と 4k+3 に対応します。下の側の範囲は、上の始まりから 1〜3 行ずれています。
- この構造を崩さないことを、「面ごとに行の偶奇で分け、同じ規則で戻す」ことで満たしています。

## 検証の数値 (すべて自分で実行した結果)

**nv_fields の自己試験**
- 508,002 項目で不一致 0。
- 範囲: 3 型、高さ 0〜33 と幅 8 通りの組み合わせ、行の間隔 2 通り、大きなサイズ 8 種 (1920×1081 など)、4:2:0 と 4:4:4。
- 行の外側の領域が書き換えられていないこと、形の合わない入力を拒否すること、同じバッファ内での再結合も確かめました。

**numpy との照合**
- 96/96 ケース (6 形式 × 16 サイズ、1×1〜1920×1080)。分離は numpy の `p[0::2]` / `p[1::2]` と一致し、再結合は入力とバイト一致しました。

**実際のインターレース素材 (ffmpeg 8.0.1)**
- 作った素材は 3 本です。どれも x264 の可逆符号化 (`-qp 0` と `+ilme+ildct`) です。
  - test_v02 を 60p とみなし、tinterlace で 1080i30 の tt にしたもの
  - 同じく 1080i30 の bb にしたもの
  - test.mp4 から作った 360i15 の bb
- ffprobe の `field_order` は tt / bb / bb と出ました。
- 4 形式 (yuv420p、yuv420p10le、yuv444p、gbrpf32le) で、分離が numpy と一致し、再結合が元とバイト一致しました (各 8/8)。
- ffmpeg の separatefields の出力と、3 面とも一致しました (8/8 × 3 本)。
- 各フィールドが、作る元にしたフレームのフィールドと 3 面とも一致しました (8/8 × 3 本)。

**照合が空振りしないことの確認**
- nv_fields にわざと誤りを入れた変異 4 種 (`NVF_MUT_*`) は、自己試験と numpy 照合の両方で検出されました。
- 色差の分け方を変えた変異では、実素材での separatefields との一致が 0/4 になりました。
- qtui で probe から `field_order` を外した変異では、3 項目が FAIL しました (元に戻して再ビルド済み)。

**プログレッシブの経路**
- `make verify` の 11 本はすべて一致しました: quality 4000/4000、band 5706/5706、curve、lzma 667/667、flat、flat_cancel、model 1244/1244、temporal 474/474、spatial 1287/1287、profile 39/39、aft 67 件。
- qtui_selftest: 120 passed, 0 failed (118 項目と、初期化・後始末の 2 つ)。
- compare_export: 19 ケースすべて SAME (出力ファイルと ffmpeg・ffprobe のコマンド列)。
- ビルドの警告は 0 です (native と qtui)。

**libnvcore への影響**
- `nv_fields.o` が外に出すシンボルは `nvf::` だけで、弱い定義はありません。外から使うのは memmove だけです (nm で確認)。
- ほかのソースとビルドの設定は変えていません。dylib の中の配置 (アドレス) は変わります。
- C ABI を変えていないので、`verify_bridge.py` は実行していません。

## そちらで判断・対応が必要な点
- `qtui/tools/compare_export.py` は今も `ROOT/repro` を参照していて、2026-10-31 の構成変更の後はそのままでは動きません。ROOT と REPRO だけ書き換えた写しを scratchpad で実行しました。同じ参照は `compare_n3c.py`・`compare_ofx.py`・`py_dump*.py` などにもあります。worktree の tools は変えていません。
- `PROGRESS.md` は `.gitignore` の除外リストに当たっていて、そのままでは追跡されません。マージするときは `git add -f` が必要です。
- 依頼の文面 (INT-1 の 1) は「MeatVideoQt と PySide6 版の両方に表示」でしたが、ご指示どおり qtui だけに表示し、PySide6 版 (`reference/python/`) には一切触れていません。

## INT-2 への注意点
- フィールドの時間順は、tt と bt なら top が先、bb と tb なら bottom が先です。
- H が奇数のときは、ビューのパッド行の位置を「先頭 + 行番号 × 間隔」で計算できません。先頭と間隔だけを受け取るカーネルに渡すときは、`split` でコピーしてください。偶数の高さなら問題ありません。
- 4:2:0 では、上と下のフィールドで色差の縦の標本位置が違います。2 つのフィールドを混ぜる処理では、この差に注意が要ります。
- フィールド処理を C ABI に出すときは、ABI を 4 に上げ、`nv_native.py` もそろえてください。
- 書き出し (INT-3) に関係する観測:
  - x264 で bb にするには `-x264-params bff=1` が必要でした。`-top 0` や `setfield=bff` だけでは tt のままです。
  - ffv1 を mkv に入れると、bb の素材が "bt" と表示されました。

ファイル (scratchpad、`/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/int1/`):
- 作った素材: `.../scratchpad/int1/clips/`
- ログ: `.../scratchpad/int1/*.log`

---
## 統括の再確認 (2026-10-31、ログ: scratchpad `int1_lead/rerun.log`)
- `make -C native lib nv_fields_test` と qtui のビルドは、どちらも警告 0。
- `native/verify_fields.py` (乱数): 自己試験 508,002 項目で不一致 0。numpy との照合は 96/96 で、ALL BYTE-EXACT。
- `verify_fields.py --video` を、作業者が作った素材 3 本 (t_360i15_bb・v02_1080i30_bb・v02_1080i30_tt) で実行した。3 本とも、yuv420p10le・yuv444p・gbrpf32le の各形式で 8 フレームが一致し、ALL BYTE-EXACT。
- `qtui_selftest`: 120 passed / 0 failed。fieldOrderText・fieldOrderProbe・fieldOrderShownOnOpen はすべて PASS。
- 追跡しているファイルの変更は `native/Makefile`・`qtui/src/app/mainwindow.{h,cpp}`・`qtui/src/core/video.{h,cpp}`・`qtui/tests/selftest.cpp` の 6 本。`reference/` (PySide6 版と処理コア) は変わっていない。

## 独立検証 Round 1 (verify-int1): **APPROVE** (M 0)
判定書は `VERDICT.md`。L-1 は INT-2 より前に直す必要があると指摘された。

**観測**
- **nv_fields:**
  - 自前の ASan/UBSan のドライバと numpy の参照で 450/450 一致した。
  - 作業者の変異 4 種をすべて検出した。
  - ffmpeg の separatefields/weave と往復で一致した。
- **色差の規約 c&1:** ffmpeg (separatefields・tinterlace・swscale の interl=1) と x264 のインターレース符号化に合っている。x264 の qp32 で、色差の誤差は c&1 の配置で 0.56、別の配置で 1.76 だった。
- **既存の照合:**
  - qtui_selftest: 120/0
  - compare_export: 19/19 SAME
  - verify_bridge --quick: ALL MATCH
- **ビルドと ABI:**
  - 警告 0。C ABI は 36 関数で変わらず、版数は 3 のまま。増えたシンボルは nvf:: の 31 個。
  - reference/ と ini は変わっていない。

**指摘**
- L-1: ffmpeg 8.0.1 は、上が先 (TFF) の ffv1/prores を "tb"、下が先 (BFF) を "bt" と書く。そのため UI の表示が実際と逆になる。REPORT の「bb と tb なら下が先」を INT-2 でそのまま使うと、時間の順序を誤る。順序は、フレームの top_field_first かユーザーの選択で決めるべき。
- L-2: nv_fields.h の join の注釈「fields may be views into dst」は、同じフレームの field_view を渡すときしか成り立たない。
- I-1: 「弱い定義はない」は誤り。30 個が weak で、dylib に WEAK_DEFINES が付く。
- I-3: H を 4 で割って 2 余る高さでは、下フィールドの最後の色差の行が separatefields と違う。ffmpeg が面の外を読んでいるためで、nv_fields の方が妥当。
- I-5: compare_export の ffprobe の trace は、手書きの文字列だった。実際のコマンドは、ラッパーで HEAD と同じことを確かめた。
- そのほか: I-2 (W=0 の NULL の面で UB)、I-4 (OFX で表示を消すことの試験がない)、I-6 (本家は Video Scan Type をユーザーが選ぶ)。
- 「3-line offset」: 典拠は docs とタスクのほかに無い。作業者の解釈は妥当とした。別の読み方 2 通りは VERDICT に挙げてある。

---
## Round 1 の指摘への対応 (作業者 int1、2026-10-31)
- **L-1: フィールド順序の判定**
  - 最初のフレームの `interlaced_frame` と `top_field_first` を優先して判定する。動画を開くときの probe に、2 回目の ffprobe (`-read_intervals %+#1`) を足した。
  - 追加にかかる時間は 35〜81 ms。1 回目の呼び出しと、書き出しの probe は変えていない。
  - フレームに旗が無いときだけストリームの値を使い、`tt` なら上が先、`bb` なら下が先とする。`tb` と `bt` は順序の根拠にしない。
  - ストリームの値とフレームの旗が食い違うときは、表示の末尾に注記を付ける。
  - 判定の結果は次のとおりで、どれも実際の順序と合っていた。

    | 素材 | 判定 |
    |---|---|
    | x264 TFF | 上が先 |
    | x264 BFF | 下が先 |
    | ffv1 と prores (+ildct)、TFF | 上が先。注記に "stream field_order: tb" |
    | ffv1 と prores (+ildct)、BFF | 下が先。注記に "stream field_order: bt" |
    | prores (+ildct なし) | 順序は不明 |
- **INT-2 への注意点の訂正** (前回の「bb と tb なら下が先」を置き換える)
  - フィールドの時間順は、最初のフレームの旗で決める。`interlaced_frame` = 1 なら、`top_field_first` = 1 のとき上が先、0 のとき下が先。
  - 旗が無いときだけストリームの値を使う (`tt` = 上が先、`bb` = 下が先)。`tb` と `bt` は根拠にしない。ffmpeg 8.0.1 は、ffv1 と prores で、上が先のフレームに `tb`、下が先のフレームに `bt` を書くためである。
  - 判定には qtui の `scan_of` と `first_field` を使う。
  - 見るのは最初のフレームだけなので、フレームごとに順序が変わる素材は対象外。
- **L-2: join の重なり**
  - フィールドが dst と同じメモリを使うときは、dst 自身の field_view だけを受け付ける。それ以外の重なりは false を返し、何も書かない。
  - split は、読む面と書く面の重なりをすべて拒否する。
- **I-1: weak シンボル**
  - テンプレートの明示的インスタンス化をやめ、普通の多重定義にした。weak は 0 になり、dylib のフラグは HEAD と同じになった。
  - 名前空間は `nvf` から `nvfield` に改めた (`nvf` は nv_flat が使っていたため)。前回の「弱い定義はない」という記述は誤りで、PROGRESS で訂正した。
- **I-2: W=0 で NULL の面**
  - ビューを NULL のまま安全に扱う。ASan と UBSan で 528,636 項目を回し、報告は 0 件だった。
- **I-4: OFX のときは表示しない**
  - 自己試験 `ofxHidesScan` を足した。
- **I-3: 720×486 のような高さ**
  - 照合で、下フィールドの色差のパッド行を separatefields との比較から外し、外した数を表示する。
- **確かめた結果**
  - verify_fields: ALL BYTE-EXACT。分離と再結合は 12 本 × 4 形式で 48/48。ffv1 の tb/bt、x264 の tt/bb、720×486 は、separatefields と一致した。
  - qtui_selftest: 121/0。
  - compare_export: 19/19 SAME。
  - 警告: 0。
  - 変異: 5 種をすべて検出した。tb を下が先とみなす旧規則に戻す変異は、自己試験 2 項目で FAIL した。

## 独立検証 Round 2 (verify-int1): **APPROVE** (M・L なし)
**L-1 は解消した。**
- 素材 21 本で、判定はすべてフレームの旗 (実際の順序) と一致した。
  - 素材の内訳: x264 の tt/bb、ffv1 と prores (+ildct) の TFF/BFF、tt の中身に bt を付けた mov、mpeg2、GOP の途中から切った TS、音声付き、progressive、旗なし。
  - 実際の順序とストリームの値が食い違う素材には、"stream field_order: X" の注記が付いた。
- 動画を開くときの probe は、1080p で 46 ms → 121〜125 ms、640×360 で約 20〜40 ms 増える。書き出しの probe は変わっていない。

**ほかの指摘もすべて解消した。**
- L-2: 重なりの境界は、ASan/UBSan で報告 0 件。受け付ける配置と拒否する配置の両方を確かめた。
- I-1: weak は 0 で、dylib のフラグは HEAD と同じ。公開シンボルは nvfield:: の 31 個で、ABI は 3 のまま。
- I-2・I-3・I-4 も解消した。

**再実行:**
- verify_fields: ALL BYTE-EXACT (自己試験 528,636 項目で不一致 0、変異 5 種はすべて検出)。前回は 0/8 だった ffv1 の tb 素材も 8/8 になった。
- qtui_selftest: 121/0
- compare_export: 19/19
- verify_bridge --quick: ALL MATCH

**新しい指摘 (どれも I、対応は任意):**
- I-7: 旗もストリームの値も無い素材は「scan unknown」と表示される。
- I-8: 2 回目の ffprobe の時間切れは 30 秒。
- I-9: 判定に使うのは最初のフレームだけで、PAFF は試していない。
