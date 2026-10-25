# 高ビット深度対応 — 実装仕様 (統括が作成、2026-10-26)

元のタスク: `.claude/tasks/highbits_and_ui_fixes.md` の優先度 1 (A〜F)。優先度 2〜4 は対象外。
作業ディレクトリ: `/Users/seitan/Desktop/work/reverse` (repro は `repro/`)。

## 0. 基本方針 (統括の決定)

**処理コアの尺度は変えない。** `nvfilter.py` の v5 は「8 ビット相当の 0〜255 の float32」で動く
(ノイズ曲線のビン `CURVE_BINS`・クリップ判定 254.5・黒レベル 16・閾値など)。高ビット深度の入力は
読み込み時にこの尺度の float32 へ正規化し、書き出し時に元の深度へ戻す。こうすると:

- `nvfilter.py` を変更せずに済む (**nvfilter.py は編集禁止**。どうしても必要なら実装せず報告に書く)。
- `CURVE_BINS` のビン計算は 0〜255 尺度のまま正しく働く (タスク B の「ビット深度に対応」はこれで満たす。報告に明記)。
- プロファイルが深度に依存しない (UI で作ったプロファイルを別深度の書き出しにそのまま使える)。
- 整数深度 b の正規化は v / 2^(b−8) (2 の冪の割り算なので float32 で厳密。16 ビットまで仮数 24 ビットに収まる)。
  逆変換は clip(rint(x·2^(b−8)), 0, 2^b−1)。処理しない画素 (Mix 100% など) は元の値に厳密に戻る。
- 8 ビット (b=8) では正規化が恒等で、**従来の経路と完全に同じ** (後述の回帰試験でビット一致を示す)。
- full range の高ビット (例 10 ビット白 1023) は 255.75 になる。ITU の full range の尺度 (2^b−1) とは僅かに違うが、
  往復は厳密で、影響はクリップ判定の境界だけ。報告と docstring に近似として明記する。

## 1. 新モジュール `repro/nv_depth.py`

画素形式の表・正規化・RGB↔YCbCr・4:2:2 の変換をここに集める (UI・CLI・OFX ブリッジ共通)。

- `class PixFmt` (dataclass で可): `name` (ffmpeg の rawvideo pix_fmt)、`family` ("yuv" | "rgb")、
  `bits` (8/10/12/16/32)、`is_float`、`chroma` ("420" | "422" | "444")、`dtype` (np.uint8 / "<u2" / "<f4")。
  プレーンの大きさ (W, H 奇数対応: 4:2:0 は ((W+1)//2, (H+1)//2)、4:2:2 は ((W+1)//2, H)) とフレームのバイト数を返すメソッド。
- `source_format(pix_fmt_name)` → ソースの (family, bits, is_float, chroma)。ffprobe の pix_fmt 名から判定
  (yuv/yuvj/yuva 4xx pNN、nv12/p010/p016、gbr/gbra pNN/f32/f16、rgb48/rgba64/rgbaf32、gray* など。不明は 8 ビット 4:2:0 yuv)。
- `choose_format(src_info, depth)` → 処理に使う `PixFmt`:
  - `depth=8` (既定): **常に yuv420p** (従来どおり。ソースが何であっても)。
  - `depth` ∈ {10, 12, 16}: yuv 系ソースはソースのクロマ形式で `yuv{420,422,444}p{depth}le`
    (4:4:0・4:1:1・4:1:0 は 4:4:4 で読む)、RGB 系整数ソースは `gbrp{depth}le`。
  - `depth=32`: 常に `gbrpf32le` (RGB float。EXR などの float ソースはこれでそのまま、整数ソースは ffmpeg が RGB float に変換)。
  - `depth="source"`: ソースの深度 (9→10、14→16、float/17 ビット以上→32、8→8 のときは上の depth=8 と同じ yuv420p)。
- `to_internal(raw_planes, fmt, matrix)` → (Y, Cb, Cr) float32 (0〜255 尺度):
  - yuv 系: 各面を 2^(b−8) で割る。4:2:2 は色差を横方向に 4:4:4 へ拡大する
    (偶数列 = 元の標本そのまま (co-sited)、奇数列 = 左右の平均、右端は複製)。8 ビット (yuv420p) は**変換せず uint8 のまま返す**
    (従来の read_frames と同じ型。回帰のため)。
  - rgb 系: ffmpeg の面の順は **G, B, R** に注意。R,G,B を 0〜255 尺度 (整数は v/2^(b−8)、float は v·255) にして、
    `matrix` (nvfilter の `_KRB` と同じ係数) で **full range** の YCbCr 4:4:4 に変換 (float64 で計算して float32 に)。
    `nvfilter.yuv_to_rgb(planes, matrix, full_range=True)` で元の RGB×255 に戻る式にすること (FindFlatArea・表示がこれを使う)。
- `from_internal(planes, fmt, matrix)` → 書き出し用の生バイト (`to_internal` の逆。整数は rint と clip、
  **float は clip も丸めもしない** (HDR の 1.0 超・負値もそのまま)。4:2:2 は偶数列を取り出す)。
- `full_range_internal(info, fmt)`: rgb 系は True、yuv 系は `info["full_range"]`。プロファイル・黒レベル・表示はこれを使う。
- 必要なら `describe(fmt)` (ステータスバー用の短い文字列、例 "yuv420p10le · 10-bit 4:2:0")。

## 2. CLI `repro/denoise_video_v5.py` (タスク A・C)

- `probe()` に `pix_fmt` (と分かれば `bits_per_raw_sample`) を追加。
- `read_frames(path, W, H, lo=0, hi=None, fps=None, fmt=None)`: `fmt=None` は**従来と同一** (yuv420p, uint8)。
  `fmt` 指定時は `-pix_fmt fmt.name` で読み、`nv_depth.to_internal` の結果を返す (matrix が要るので引数 `matrix` も追加してよい)。
  既存の呼び出し (UI・calibrate など) の互換を保つこと (位置引数の意味を変えない)。
- 新しい引数: `--depth {8,10,12,16,32,source}` (既定 8)、`--codec {auto,x264,x265,ffv1,prores,exr}` (既定 auto)。
  `--crf` は x264/x265 だけに効く。ffv1・exr は可逆。
- 書き出し (`encoder()`): 生の入力の `-pix_fmt` は fmt.name、出力の `-pix_fmt` も**同じ** (ffmpeg に変換させない)。
  auto の選び方:
  | 処理形式 | 出力拡張子 | auto の符号器 |
  |---|---|---|
  | yuv420p (8) | 何でも | libx264 (**従来どおり、引数も同じ**) |
  | yuv 10 ビット | .mp4/.mov/.m4v | libx264 (High 10 / 4:2:2 / 4:4:4。pix_fmt はソースのクロマ形式) |
  | yuv 12 ビット | .mp4/.mov | libx265 |
  | yuv 10/12/16 ビット | .mkv | ffv1 (可逆) |
  | yuv 16 ビット | .mkv 以外 | エラー (「.mkv を指定」の案内) |
  | gbrp 整数 | .mkv | ffv1 |
  | gbrpf32le | .mkv | ffv1 (float 可逆。ffmpeg 8 で対応を確認済み) |
  | gbrpf32le | `%d` を含む .exr のパターン | exr 連番 (`-f image2`。音声は落として注記) |
  明示の `--codec` で組み合わせが不可能 (例 x264 で 12 ビット、prores で 4:2:0) なら分かりやすいエラーで止める。
  prores は prores_ks、4:4:4 なら 4444 (profile 4)、4:2:2 なら HQ (profile 3)。
- 色のタグ: yuv 系は従来どおり入力側・出力側の両方に付ける。rgb 系は色行列のタグ (`-colorspace`) を付けず
  (付けると ffmpeg が変換し得る)、primaries / trc は付けてよい。**ffmpeg が画素を変換していないことを試験で確かめる** (§4)。
- `info["full_range"]` を使っている箇所 (プロファイル・フリッカの黒レベル) は `nv_depth.full_range_internal` に。
- `.json` の副ファイルに処理形式 (`pix_fmt`・`depth`・`codec`) を書く。EXR 連番は `%05d` 等を取り除いた名前にする。
- プロファイルの `sub` (色差の間引き) と処理形式のクロマが食い違えば (UI で 4:2:0 で作ったプロファイルを 4:4:4 で書き出す等) エラーで止める
  (`nv_ui_qt/export.py` 経由のとき)。
- 書き出し: `np.clip(np.rint(o), 0, 255).astype(np.uint8)` の行は 8 ビット経路でそのまま残し、それ以外は `from_internal`。

## 3. UI `repro/nv_ui_qt/` (タスク D・E)

- Preferences に「Bit depth」(8-bit (従来) / Source depth / 10 / 12 / 16 / 32-bit float) を追加。既定は 8-bit (従来と同じ動作)。
  読み込み・Generic Profile の読み込み・書き出し (`export.py` に `--depth`) で同じ設定を使う。設定を変えたら動画を読み直す。
- メモリ: 高ビット深度のフレームはメモリに float32 で持つ (または uint16 で持ち、取り出し時に正規化する薄いラッパー。どちらでも可)。
  メモリ上限 (`mem_gb`) の計算は実際の 1 フレームのバイト数で行う。8 ビットは従来どおり uint8 のタプルのリスト。
- 表示: ビューアは 0〜255 尺度の RGB を 8 ビットに落として表示する (既存の経路がそのまま使えるはず。rgb 系は full_range=True で変換)。
- ステータスバー (または動画情報の欄) に「ソースの pix_fmt とビット深度」と「処理の深度」を出す (例 `source yuv420p10le (10-bit 4:2:0) · processing 10-bit`)。
- Export Full Video の保存ダイアログ: 深度に合わせた既定の拡張子 (16 ビット・32 ビットは .mkv、32 ビットは .exr 連番も選べる)。
- OFX ブリッジ (`ofx_bridge.py`・`main.py`): `--ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]` (既定 8 = 従来のプロトコルのまま)。
  - 8: yuv420p uint8 (今のまま。frame_size = W*H + 2*(W//2)*(H//2))
  - 16: 同じ 4:2:0 配置で uint16 little-endian (frame_size = 2 × 8 ビットの値)。内部尺度 = v/256
  - 32: 同じ 4:2:0 配置で float32 little-endian (frame_size = 4 × 8 ビットの値)。値は「8 ビットの符号値 / 255」(Y 16/255〜235/255 など)。出力は clip・丸めなし
  - プロトコルを docstring に書く。
  - 既存の不具合で OFX モードが動かない箇所 (例: `_ofx_main` の `QApplication(argv)` で `argv` が未定義、`win.params()`・`win.process_frame()` が
    MainWindow に無い) は、**深度の往復試験を通すのに必要な最小限だけ**直し、報告に列挙する。
- `selftest.py`: 既存の 49 項目は維持 (8 ビットの動作は不変のはず)。高ビット深度の項目を追加してよい (10 ビットの読み込み・表示・ステータス表示・書き出し)。

## 4. 試験 (タスク F。結果は数値で報告。推測で書かない)

すべての出力は `sandbox/omo/coord/highbits/test/` に書く (**`repro/out/` には絶対に書かない**)。合成素材もここに作る。
Qt を動かすときは必ず `QT_QPA_PLATFORM=offscreen`。重い Python/ffmpeg ジョブは同時に 3 つまで。

1. **8 ビットの回帰**: 控えの旧スクリプト `sandbox/omo/coord/backup/denoise_video_v5.py.20261026_pre_highbits` を
   `repro/` に一時コピーせずに (例: `cd repro && python3 <控えのパス> ...` のように repro を sys.path に置いて) 実行し、
   新しいスクリプトと同じ引数 (既定の depth 8) で `test.mp4` の短い範囲 (例 `--start 300 --frames 8 --crf 0`) を処理。
   出力のデコード画素 (`ffmpeg -f framemd5` など) が一致すること。test_v02.mp4 の短い範囲でも 1 回。
2. **10 ビットの尺度の整合**: test.mp4 の数フレームを `yuv420p10le` (値 = 8 ビット値 ×4) の ffv1 .mkv にした素材で
   `--depth 10 --codec ffv1` と、元の 8 ビットで `--depth 8 --crf 0` を処理し、プロファイル (json の sigma 等) が一致すること、
   出力の |out10 − 4·out8| の最大値を報告 (8 ビット側の丸め分だけの差のはず)。
3. **10/12/16 ビットの深度維持**: numpy で作った雑音入りの合成素材 (yuv420p10le・yuv422p10le・yuv444p12le・yuv444p16le。
   1 フレーム 256×256 以上、8 フレーム程度、ffv1 .mkv) を処理し、出力の pix_fmt が同じであること、
   出力値が 4 の倍数などに偏っていない (下位ビットが使われている) こと、`--mix 1` (Mix 100%) で出力 = 入力が**ビット一致**すること。
   x264 10 ビット・x265 12 ビットの出力も 1 回ずつ作り、ffprobe で pix_fmt を確認。
4. **32 ビット float**: gbrpf32le の合成素材 (1.0 超・負の値を少し含む。ffv1 .mkv と EXR 連番の両方) を処理し、
   出力が gbrpf32le であること、`--mix 1` で入力との最大絶対誤差 (RGB→YCbCr→RGB の往復誤差、1e-6 程度のはず) を報告、
   通常の処理でも 1.0 超・負の値が切られていないこと。
5. **UI**: `QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest ../test.mp4 <出力先>` が全項目 PASS。
   追加した高ビット深度の項目の結果。OFX ブリッジの 8/16/32 の往復 (stdin に数フレームを書き、画面を出さずに Apply/閉じる → stdout の
   フレームの大きさと値の範囲を確認)。
6. 変更したすべての .py に `python3 -m py_compile`。

## 5. してはいけないこと

- `nvfilter.py`・`nv_aft_v5.py`・本家移植のモジュール (`nv_curve` 等) の編集。`repro/out/` への書き込み。
- 本家バンドルの読み込み・nvh の実行 (この作業には不要)。
- git の commit / add / rm / reset など index を変える操作 (読むだけの `git diff`・`git status` は可)。
- `AGENTS.md` の編集 (統括が検証の後に行う)。
- 実画面の Qt (offscreen 以外)。

## 6. 報告 (`sandbox/omo/coord/highbits/REPORT.md`)

変更したファイルと要点、§4 の各試験のコマンドと結果 (数値)、本家にも元のタスクにもない判断 (近似・制限)、残っている問題。
