# INT-1 独立検証 VERDICT (検証者 verify-int1、2026-10-31)

## 判定: **APPROVE** (M はない。L-1 は INT-2 より前に直すこと)

- 対象: worktree `.claude/worktrees/int1` (branch int1-fields、未コミット)。ソースは編集していない。
- 作業はすべて scratchpad `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/vint1/` で行った。
  ここには、native と qtui の写しのクリーンビルド、検証者が自分で書いた照合 (`myf.cpp`・`mycheck.py`・`conv.py`・`x264c.py`・`tbcheck.py`)、素材 (`fo/`・`clips/`)、ログ (`*.log`) がある。
- 書き込んだのは、この VERDICT.md と scratchpad だけである。
- 次の 3 つは変わっていないことを確かめた。
  - `~/.config/NeatRepro/nv_ui_qt.ini`: md5 `70c806db…2d11` が検証の前後で同じ。
  - `reference/`: 検証中に更新されたファイルは 0 件 (`find -newer`)。
  - `reference/python/out/`: 書き込んでいない。
- 本家バンドルは実行も dlopen もしていない。本家バイナリについては `strings` で静的に読んだだけ。

以下、「観測」は検証者が自分で実行した結果、「推測」はそれ以外。

---

## 1. nv_fields の正しさ

### 観測

**作業者とは別の方法での照合 (`myf.cpp` + `mycheck.py`)**
- 照合用の C++ ドライバは検証者が書いた。ASan と UBSan を有効にしてビルドしている。
  - フレームは、行の間隔に余りがあり、上下に 2 行ずつの番兵がある (値 0x5A) バッファに置く。
  - フィールドは、コピーではなくビューの `FieldView::row()` を使って読み出す。
  - フィールドのコピーは自前のコードで作り、それを `join_frame` で戻す。
- 期待値は、numpy の参照を検証者が仕様から書いた (`p[par::2]`。H が奇数のときの下フィールドのパッド行は行 H−2、H=1 なら行 0)。
- 結果: **450/450 件が一致した**。番兵も壊れていない。
  - 型は u8・u16・f32 で、f32 には NaN の中身、−0、inf、非正規化数を含めた。
  - 形式は 4:2:0 と 4:4:4。
  - 大きさは W ∈ {1,2,3,7,33} × H 0〜13 と、1920×1081・720×486・720×487・1280×2・3×1。
- 照合が空振りしないこと: 作業者の変異 4 種をこのドライバでビルドすると、一致したのは NARROW 30/450、PADWRITE 171/450 (番兵の破壊も検出)、PADROW 225/450、CHROMA 84/450 で、**4 種とも検出した**。

**ffmpeg との往復 (検証者が作った乱数の rawvideo)**
- 64×36・720×480 の yuv420p、64×36 の yuv420p10le と yuv444p を、tff と bff の両方で試した。
- `separatefields` の出力は、nv_fields のフィールドとバイト単位で一致した。
- `weave` で戻したフレームは入力と一致し、nv_fields の join で戻したフレームも入力と一致した。
- H が 4 で割ると 2 余る場合は後述する (I-3)。

**形の合わない入力の拒否と、範囲の外への書き込み (`myf edge`)** (すべて期待どおり)
- 次の入力は拒否された: 高さの足りない下フィールド、幅の狭い上フィールド、高すぎる上フィールド (`split` は高さがちょうど ceil(H/2) であることを要求する)、データが NULL、W < 0、H < 0、stride < W、parity −1、出力先が NULL。
- 拒否したときは何も書かない。`split_frame` は色差の形だけが合わない場合も拒否し、輝度も書かない。
- 番兵の破壊はなかった。

**作業者の照合の再実行** (`verify_fields.py --sensitivity`、scratchpad のクリーンビルドで実行)
- 自己試験 508,002 項目で不一致 0、numpy 96/96。
- 変異 4 種はすべて DETECTED。

### 色差の規約 (色差の行 c はフィールド c&1 に属する) が ffmpeg・x264 と合っているか

素材は検証者が作り直した。

- **swscale** (`conv.py`): 上フィールドと下フィールドが別の画像になっている 4:4:4 のインターレースのフレームを、`scale=interl=1` で yuv420p にした。
  - 色差の偶数行は上フィールドと一致した (平均絶対誤差 0.00)。奇数行は下フィールドの値で、平均絶対誤差は 2.00。
  - `interl=0` では偶数行の誤差が 107.5 になり、両フィールドが混ざった。
  - したがって、ffmpeg のインターレース対応の色差間引きは c&1 の規約である。
- **x264** (`x264c.py`、`-qp 32` の非可逆符号化): 各フィールドが滑らかな画像を、色差の行の配置を変えて符号化し、色差の平均絶対誤差を比べた。

  | 色差の配置 | インターレース符号化 (`+ilme+ildct`、tff=1) | プログレッシブ符号化 |
  |---|---|---|
  | c&1 (作業者の規約) | **0.56** | 2.01 |
  | (c>>1)&1 (2 行ごとに交互) | 1.76 | — |

  - c&1 の配置だけが、インターレース符号化で大きく改善した。x264 のフィールド MB は、色差の行の偶奇をフィールドとして扱っている。
- **tinterlace**: 作業者の素材の確認 (separatefields との一致) を再実行した。さらに mpeg2video で作った 1440×1080 の tt と 720×480 の bb でも、`--video` は 8/8 で一致した。
- 結論 (観測): 規約は ffmpeg (separatefields・tinterlace・swscale の interl) とも x264 のインターレース符号化とも合っている。

---

## 2. 「3-line offset」の解釈

- 文献上の根拠はない (観測)。この語が出てくるのは `docs/INTERLACE_PROGRESSIVE.md` §1 (「処理後に 3 線 (3-line) オフセットで再結合する」、「解析から」) と、タスクの文面だけである。
  - worktree 全体を grep しても、ほかに出てこない。
  - 本家の arm64 スライスの `strings` にも、"3-line" やそれに類する語はない。
  - 本家にある関連の文字列は次のとおり:
    - `Image::ConvertScanType(ImageFields, GrowingMemoryStack*, bool)` のラムダ 4 個
    - `astfqg::FilterVideoField`
    - "Interlaced (one field)" / "Interlaced (two fields)"
    - "Video Scan Type"
    - OFX の `OfxFieldUpper/Lower/Both/Single/None`、`OfxImageClipPropFieldExtraction`
  - docs の記述は、アドレスの付いた解読に基づいていない (推測)。
- 作業者の解釈 (インターレースの 4:2:0 の行の構造。上の色差は輝度の行 4k と 4k+2、下は 4k+1 と 4k+3) は妥当である。INT-1 はバイトを動かすだけなので、この構造は自動的に保たれる。
- **別の読み方** (推測)。いずれも INT-1 の正しさには影響しない。
  1. **色差の標本位置**: MPEG-2・H.264 のインターレース 4:2:0 では、上フィールドの色差はフレームの行 0.5、下フィールドの色差は行 2.5 に位置する (各フィールドの 1/4 と 3/4)。
     - 色差を補間する処理 (RGB への変換、FindFlatArea・プロファイルの経路、4:2:2 の拡大など) では、フィールドごとに位相を変える必要がある。INT-2 と INT-3 の課題になる。
  2. **奇数行のずれによるフィールドの優先順の反転**: NTSC の 486↔480 の切り出しや、OFX の画像範囲 y1 が奇数のときがこれにあたる。
     - nv_fields の parity はバッファの行 0 を基準にしている。INT-4 では、ホストのフィールド属性と画像範囲から parity を決め直す必要がある。
  3. 典拠のない記述: docs の言い回しを改めるか、「未確認」と明記することを勧める。

---

## 3. field_order の取得と表示

### 観測

検証者がビルドした小さなプログラムで、qtui の `nvqt::probe(path, true)` と `field_order_text` を実際の素材に対して呼んだ (`probe_t/`、素材は `fo/`)。

| 素材 | ffprobe | 表示 |
|---|---|---|
| mpeg2 tt | tt | interlaced, top field first (tt) |
| mpeg2 bb | bb | interlaced, bottom field first (bb) |
| mkv/mov (`-c copy -field_order tb/bt`) | tb / bt | "…bottom field first (tb: top coded first)" / "…top field first (bt: bottom coded first)" |
| progressive mkv・test.mp4 | progressive | progressive |
| mpeg4 avi・rawvideo nut | (値なし) | scan unknown |

- `unknown`: ffprobe の JSON は既定では `unknown` を出力しない (`-show_optional_fields always` のときだけ出る)。そのため、実際には「値なし」と同じ扱いになる。テキストの写像 `field_order_text("unknown")` は selftest で確かめられている。
- 書き出し側の `probe(path)` は field_order を取らない (全件で空)。
- ffprobe の代わりに引数を記録するラッパーを置いて、実際に渡される引数を記録した。
  - UI の呼び出しは `…,start_time,field_order:stream_side_data=…`。
  - 書き出しの呼び出しは HEAD のコマンドと 1 文字も違わない (`export.cpp` の trace の文字列とも一致)。
- **OFX モード**: selftest の写し (scratchpad) の `ofxWindowSetup` に、検査を 1 つ足した。
  - 内容: 先に `scan_lbl` に "stale" を入れ、`set_ofx_frames` の後で文字列とツールチップが空であることを確かめる。
  - 結果: PASS。`clear()` を外した変異では FAIL した。
  - worktree の selftest には、この検査はない (I-4)。
- **compare_export**: 19/19 件が SAME (出力ファイルと、ffmpeg・ffprobe のコマンド列)。ALL SAME。
  - 実行したのは、`compare_export.py` の ROOT/REPRO だけを `reference/python` に直した写しで、`--exe` には worktree の `qtui/build_main/qtui_export` を渡した (I-5)。
- **プログレッシブの経路**:
  - qtui の差分は、probe の引数と表示ラベルだけである。
  - `fieldOrderShownOnOpen` は、インターレースの素材でも、読み込んだフレームが `read_frames` とバイト単位で一致することを確かめている (PASS)。
  - libnvcore については、ほかのソースとフラグは変わっていない (`git diff`)。
  - `verify_bridge --quick` の新しい dylib (worktree の `native/libnvcore.dylib`) での結果は §4 にある。

---

## 4. 既存の検証の再実行 (観測)

- `qtui_selftest` (worktree の build_main、test.mp4): **120 passed / 0 failed**。fieldOrderText・fieldOrderProbe・fieldOrderShownOnOpen はどれも PASS。
- `compare_export`: §3 のとおり 19/19 SAME。
- `verify_bridge.py --quick` (`NV_NATIVE_LIB` = worktree の dylib): **ALL MATCH** (424 秒)。

  | 項目 | 件数 | 一致した語 |
  |---|---|---|
  | curve | 12 | 120 |
  | profile | 7 | 553 |
  | aft | 11 | 400,026 |
  | frame | 32 | 117,644,461 |
  | cancel | 49 | 199,807 |
  | fallback | 9 | 691,209 |
  | g8 | 152 | 8,488 |
  | prof5 | 1 | 30 |
  | ui | 1 | 16,346 |
  | depth | 1 | 56,585,267 |

- `verify_fields.py` は、乱数、作業者の変異、検証者が作った素材のどれでも一致した。ただし tb の素材は I-3・L-1 を参照。
- `make -C native verify` の残り 10 本は回していない。理由: 対象の試験の実行ファイルのソースとフラグが HEAD と同じで、Makefile の差分はターゲットの追加だけだから (観測)。nv_fields の影響を受けうる経路 (dylib 経由) は、verify_bridge と compare_export で確かめた。
- **照合が空振りしないこと** (qtui の変異。scratchpad の写しで実行し、元に戻したことを cmp で確認):
  - M1 (OFX で消さない): 追加した検査が FAIL。
  - M3 (open_video が field_order を取らない): loadAndAutoProfile と fieldOrderShownOnOpen が FAIL。
  - M2 (書き出しの probe が field_order を取る): compare_export は **SAME のまま** (I-5)。

---

## 5. libnvcore への影響 (観測)

- クリーンビルド (scratchpad の写し、`make all`、cmake の qtui 全ターゲット) の警告は **0**。
- `nm -gU` の比較 (HEAD → 新): 216 → 247 で、増えた 31 個はすべて `nvf::` だった。
  - `_nvc*` の C ABI は 36 個で、名前も同じ。
  - `NVC_ABI_VERSION` は **3** のまま。
  - 依存ライブラリ (`otool -L`) は変わっていない。外から使うのは memmove だけ。
- 作業者の「弱い定義はありません」は誤り (I-1)。

---

## 6. PySide6 版 (`reference/`) (観測)

- `git status` と `git diff HEAD -- reference/` は空だった。
- 無視されているのは `__pycache__` だけで、どちらも 09:43 (検証の開始前) のもの。検証中に更新されたファイルは 0 件。

---

## 指摘

### L-1 tb / bt の表示が、ffmpeg が書いた素材では実際のフィールドの順序と逆になる
- `field_order_text` は、AVFieldOrder の文書どおり tb を「下が先」、bt を「上が先」と表示する。
- ところが ffmpeg 8.0.1 自身は、上が先 (TFF) のフレームを ffv1 や prores で符号化すると、ストリームに **tb** と書く。下が先 (BFF) のフレームには **bt** と書く。
- このため、ffmpeg で作った ProRes や FFV1 のインターレース素材では、UI の表示が実際と逆になる。
- 再現 (検証者の観測):
  ```
  ffmpeg -i test.mp4 -frames:v 2 -vf setfield=tff -c:v ffv1 ffv1_tb.mkv
  ffprobe … stream=field_order        → tb
  ffprobe … frame=top_field_first      → 1 (interlaced_frame=1)
  probe_t ffv1_tb.mkv → "interlaced, bottom field first (tb: top coded first)"   ← 実際は上が先
  ```
  - 同じことは、mpeg2 の tt から作った 1440×1080 の FFV1 (`clips/i1080_ffv1_tff.mkv`) でも起きた。
    - `verify_fields.py --video` は、tb を「下が先」とみなすので、separatefields との一致が **0/8** になり FAILURES になる。
    - 上を先として比べると (`tbcheck.py`)、3 面とも全行が一致した。
  - 作業者が気づいていた「ffv1 の mkv では bb が bt と表示される」も、同じ現象である。
- 重大度 L とした理由: INT-1 の範囲では表示だけで、ツールチップにも「表示のみ」とあるため。
- **INT-2 より前に必ず直すこと**。REPORT の「INT-2 への注意点」にある「bb と tb なら bottom が先」をそのまま使うと、時間の順序を誤る。
- 対案:
  - フィールドの順序はストリームの tb/bt から決めず、最初のフレームの `top_field_first` (`ffprobe -read_intervals %+#1 -show_entries frame=interlaced_frame,top_field_first`) から決める。
  - あるいは、tb/bt を「順序不明 (要確認)」と表示する。
  - 本家にはユーザーが選ぶ "Video Scan Type" の設定がある (後述の I-6)。

### L-2 `join` の注釈「フィールドは dst 自身のビューでもよい」は、同じフレームの field_view のときしか成り立たない
- `nv_fields.h` の `join(Plane…)` には「The fields may be views into dst itself (rows are moved with memmove)」とある。
- 実際は、行 0 から順に書くため、次の場合は true を返したままフレームが壊れる。
  - dst の前半に上フィールド、後半に下フィールドを詰めた配置 (よく使われる「フィールドを詰めた」配置)
- 正しく動くのは、dst 自身の field_view を渡す場合 (恒等で、行がすべて同じ位置) だけである。
- 再現: `scratchpad/vint1/myf edge` → `aliasing (fields = halves of dst): join returned 1, frame CORRUPTED`。
- INT-2 でその場での処理を作るときの罠になる。注釈を直すか、重なりを検出して拒否する。

### I-1 弱い定義がある (作業者の報告の誤り)
- `nv_fields.o` の明示的なインスタンス化 30 個は `weak external` である (`field_src_row` だけが強い定義)。
- これにより libnvcore に新たに `WEAK_DEFINES` が付いた。HEAD の dylib は弱い定義 0 で、このフラグはない。
- 動作への影響は見ていない (試験はすべて通る)。
- REPORT の「弱い定義はありません」は訂正が要る。将来、OFX のバンドルと dylib が同じ `nvf::` を持つ構成では、dyld が両者を統合しうる (推測)。

### I-3 H ≡ 2 (mod 4) の 4:2:0 では、下フィールドの最後の色差の行が separatefields と違う
- 対象は、720×486 のように色差の高さが奇数になる場合である。
- separatefields は、色差の面より 1 行先 (面の外) を読む。nv_fields は、同じフィールドの行を繰り返す。
- 違うのはその 1 行だけである。
  - 乱数の 66×34 で色差の行 8/9。
  - FFV1 の 720×486 で行 121/122。
- ffmpeg 側の値は面の外の内容なので、nv_fields の方が妥当である。ただし、REPORT の「separatefields と一致」は H が 4 の倍数の素材だけで確かめたものである。
- 参考: x264 は `height not divisible by 4 (720x486)` で、インターレースの符号化を拒否する。

### I-2 W=0・data=NULL・stride>0 の面は受け付けるが、`field_view` が NULL にずれを足す
- UBSan で `applying non-zero offset 3 to null pointer` (`nv_fields.cpp:64`) が出る。
- 実際の使い方では起きない。

### I-4 OFX モードで表示を消すことが selftest で確かめられていない
- 検証者の写しでは確かめた (§3)。worktree の `ofxWindowSetup` に、同じ 2 行を足すことを勧める。

### I-5 compare_export の性質 (既存の問題)
- `qtui/tools/compare_export.py` は今も `ROOT/repro` を参照していて、そのままでは動かない (作業者の報告どおり)。
- 書き出しの最初の ffprobe の trace は、`export.cpp` の `probe_full` に手で書いた文字列で、実際のコマンドではない。
  - そのため、書き出しの probe に field_order を足す変異 (M2) でも、compare_export は SAME のままだった。
  - REPORT の「変えると照合が崩れる」は正確ではない。
  - ただし、書き出しの probe を変えなかった判断は正しい。実際のコマンドが HEAD と同じであることは、ffprobe のラッパーで確かめた。

### I-6 INT-3 の参考 (本家の文字列、静的に読んだもの)
- 本家の走査方式は、ユーザーが選ぶ設定である。
  - "Video Scan Type"、"Scan type: Interlaced/Progressive"
  - "Interlaced (one field)" / "Interlaced (two fields)"
  - "Please specify the scan type of the current video clip."
  - "The noise profile has been reset because the scan type of the clip has changed."
- 自動で検出した値を既定にして、ユーザーが変えられる UI にすれば、L-1 の曖昧さも吸収できる (推測)。

---

# Round 2 (検証者 verify-int1、2026-10-31)

## 判定: **APPROVE** (M・L はない)

- 確かめた対応: 作業者の「Round 1 の指摘への対応」と PROGRESS.md の「修正 2」。
- 作業場所: scratchpad `/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/vint1/`。
  - クリーンビルド: `r2/`
  - 自前のドライバ: `myf2.cpp`
  - 判定の確認用の小さなプログラム: `r2/pt/`
  - 素材: `fo/`・`fo2/`・`clips/`
  - ログ: `r2_*.log`
- ソースは編集していない。qtui の変異は scratchpad の写しで行い、元に戻したことを cmp で確かめた。
- 変わっていないことを確かめたもの:
  - `~/.config/NeatRepro/nv_ui_qt.ini`: md5 は前と同じ。
  - `reference/`: 検証中に更新されたファイルは 0 件。
- 以下はすべて観測 (自分で実行した結果)。推測はその旨を書く。

## 1. L-1 (フィールド順序の判定): 解消

- qtui の `probe(path, true)`・`scan_of`・`first_field`・`scan_text` を小さなプログラム (`r2/pt`、写しの libnvui.a にリンク) から呼んだ。
- 結果は次の表のとおり。どれもフレームの旗、つまり実際の順序と合っている。

| 素材 | stream | 最初のフレームの旗 (interlaced_frame/top_field_first) | 判定 (first) | 表示 |
|---|---|---|---|---|
| x264 tff / bff (tinterlace, +ilme+ildct) | tt / bb | 1/1, 1/0 | 0 / 1 | top / bottom field first (注記なし) |
| prores_ks +ildct TFF / BFF | tb / bt | 1/1, 1/0 | 0 / 1 | top / bottom field first (stream field_order: tb / bt) |
| ffv1 mov TFF / BFF、ffv1 mkv TFF / BFF (Round 1 の再現素材) | tb / bt | 1/1, 1/0 | 0 / 1 | 同上 |
| prores_ks、+ildct なし (prt / prb) | tb / bt | 0/0 | −1 | interlaced, field order unknown (stream field_order: tb, first frame not flagged interlaced) |
| mpeg2 tt を `-field_order bt` に付け替えた mov | bt | 1/1 | 0 | top field first (stream field_order: bt) |
| mpeg2 tt / bb | tt / bb | 1/1, 1/0 | 0 / 1 | 注記なし |
| x264 bff の長い GOP (B フレームあり) の TS、GOP の途中から切り出したもの | bb | 1/0 | 1 | bottom field first |
| 音声付きの mp4 (x264 tff) | tt | 1/1 | 0 | top field first |
| progressive mkv・test.mp4 | progressive | 0/0 | −1 | progressive |
| mpeg4 avi・rawvideo nut | (なし) | 0/0 | −1 | scan unknown |

- **食い違いの注記**: 実際の順序とストリームの値が食い違う素材には、すべて "stream field_order: X" が付いた。食い違わない素材 (tt/bb/progressive) には付かなかった。
- **旗の無い素材**: 最初のフレームが interlaced でない場合、判定はストリームの値で決まる。
  - tb/bt は「interlaced, field order unknown」になり、順序は −1 になる。
  - 値も無い場合は「scan unknown」になる (I-7)。
- **速さ**: 2 回目の ffprobe で、動画を開くときの probe は次のように延びた。延びる幅は 20〜75 ms 程度。
  - 640×360: 書き出しの probe の 19〜27 ms に対し、38〜66 ms。
  - test_v02 (1080p): 46〜48 ms に対し、121〜125 ms (3 回)。
- **書き出しへの影響はない**: ffprobe の代わりに引数を記録するラッパーを置いて確かめた。
  - UI の probe は、ストリームの呼び出し (`…,start_time,field_order:…`) と `-read_intervals %+#1 -show_entries frame=interlaced_frame,top_field_first` の 2 回。
  - 書き出しの probe は 1 回だけで、`field_order` も frame の呼び出しも含まない。
  - `export.cpp` の trace の ffprobe は 3 行で、Round 1 と同じ。
- **照合が空振りしないこと**: 写しで `scan_of` がフレームの旗を見ないように変異させた。fieldOrderText (tb 1 1) と fieldOrderProbe が FAIL した。

## 2. L-2・I-1・I-2・I-3・I-4: 解消

- **L-2 (重なり)**: 自前のドライバ `myf2.cpp` に境界の試験を足し、ASan と UBSan (-fno-sanitize-recover=all) を有効にして実行した。すべて期待どおりで、報告は 0 件。
  - 受け付けたもの:
    - dst に接するだけ (終わり == 始まり) のフィールド
    - パッド行 (読まれない行) だけが dst に重なる下フィールド
    - dst の field_view (FieldView でも、data+parity·stride・stride 2× の Plane でも同じ)
    - 上が field_view で、下が別のバッファ
  - 拒否し、何も書かなかったもの:
    - 1 要素だけ重なるもの (先頭側・末尾側)
    - 上下を入れ替えた field_view
    - data は field_view と同じで stride が違うもの
    - dst を上下半分に分けて詰めた配置 (Round 1 で壊れていたもの)
    - split で、上下の出力どうし、または読む面と書く面が 1 要素重なるもの
    - join_frame で、色差のフィールドが dst の輝度の面に重なるもの
  - 受け付けて結果が変わらなかったもの: join_frame に、自分自身の field_views を渡す場合。
  - 作業者の NVF_MUT_NOALIAS は、この試験でも検出した。
- **I-1 (weak シンボル)**: 解消。
  - scratchpad のクリーンビルド: 警告 0。`nv_fields.o` の外部シンボルは強い定義 31 個で、weak 0。
  - libnvcore の weak も 0。Mach-O のフラグは HEAD と同じで、WEAK_DEFINES はない。
  - `nm -gU`: 247 個。HEAD からの差は `nvfield::` の 31 個だけ。`_nvc*` は名前も同じで、`NVC_ABI_VERSION` は 3 のまま。
  - worktree の dylib は、検証者のビルドとバイト単位で同じ。
- **I-2**: 解消。W=0 で data=NULL、stride 3 の面を、view・split・join に渡しても UBSan は報告しない。
- **I-3**: 解消。verify_fields は、色差の高さが奇数のときに下フィールドのパッド行を比較から外し、外した数を表示する。
  - 例: 720×486 の ffv1 (旗は TFF) で「16 planes」と表示し、8/8 で一致。
  - nv_fields 自体の動作は Round 1 と同じ (自前の照合で 450/450)。
- **I-4**: 解消。`ofxHidesScan` が PASS した。

## 3. 再実行

| 照合 | 結果 |
|---|---|
| verify_fields (worktree、`--sensitivity`、検証者の素材 8 本で `--video`) | 下表 |
| qtui_selftest 全件 (build_main、test.mp4) | **121 passed / 0 failed**。ofxHidesScan・fieldOrderText・fieldOrderProbe・fieldOrderShownOnOpen はどれも PASS |
| compare_export (ROOT/REPRO だけを直した写し、`--exe` は worktree の build_main/qtui_export) | **19/19 SAME** (出力とコマンド列)。ALL SAME |
| verify_bridge --quick (`NV_NATIVE_LIB` = worktree の新しい dylib) | **ALL MATCH** (413 秒)。curve 120・profile 553・aft 400,026・frame 117,644,461・cancel 199,807・fallback 691,209・g8 8,488・prof5 30・ui 16,346・depth 56,585,267 語 |
| 検証者の照合 `mycheck.py` (ASan/UBSan ビルドのドライバ) | numpy の参照と 450/450。separatefields・weave の往復は Round 1 と同じ結果 (H ≡ 2 mod 4 の差は I-3 のとおり) |

verify_fields の詳細は次のとおりで、最終判定は ALL BYTE-EXACT。

| 項目 | 結果 |
|---|---|
| 自己試験 | 528,636 項目で不一致 0 |
| 乱数 | 96/96 |
| 変異 5 種 | すべて DETECTED |
| separatefields (順序はフレームの旗で決める) | i486_ffv1 (tb、旗は TFF) 8/8、i480_bb.mpg 8/8、i1080_tt.mpg 8/8、i1080_ffv1_tff (tb) 8/8、ffv1_bff.mov (bt) 4/4、midgop_bff.ts 8/8 |
| separatefields を省いたもの | pr_ildct_tff (yuv422p10le のため)、prt.mov (旗 0 で順序不明のため) |

- Round 1 の L-1 の再現 (`i1080_ffv1_tff.mkv` で 0/8) は、8/8 になった。

## Round 2 の指摘 (いずれも I。対応は任意)

- **I-7**: 最初のフレームが interlaced でなく (`interlaced_frame` = 0)、ストリームにも値が無い素材は「scan unknown」と表示される。
  - 例: mpeg4 の avi、rawvideo の nut。
  - 安全側の表示だが、「progressive (first frame not flagged interlaced)」としてもよい。
  - INT-2 は first_field = −1 をプログレッシブとして扱うはずなので、処理への影響はない (推測)。
- **I-8**: 2 回目の ffprobe の時間切れは 30 秒で、失敗しても例外は握りつぶされ、ストリームの値だけで判定される。
  - 最初のパケットのデコードが極端に遅い素材では、動画を開くのがその分遅れうる (推測。今回の素材では最大でも 81 ms 程度)。
- **I-9**: 判定は最初のフレームだけで行う (作業者も明記している)。
  - 途中で順序や走査方式が変わる素材は対象外。
  - H.264 の PAFF (フィールド単位で符号化した放送の TS など) は、x264 では作れないので確かめていない。
- **参考 (INT-1 とは無関係)**: workspace 直下の `test_v02_reduce.mov` が、シンボリックリンクの循環 (`Too many levels of symbolic links`) で読めなかった。触っていない。
