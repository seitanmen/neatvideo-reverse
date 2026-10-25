# 高ビット深度対応 — 作業報告 (2026-10-26、executor。統括が最終応答から転記)

実装者のサブエージェントはファイルの書き込みを許されていないため、その最終応答を統括がここへ保存した。数値は実装者の測定 (未検証)。検証は `VERDICT.md`。

仕様 SPEC.md の §1〜§4 を実装し、§4 の試験をすべて実行した。8 ビットの出力は控えの旧スクリプトとファイルごとバイト一致し、UI の selftest は 55/55。
守ったこと: `nvfilter.py`・`nv_aft_v5.py`・本家移植モジュールは無変更 (変更も不要だった)。`repro/out/` への書き込みなし。git の操作なし。本家バンドル・nvh は不使用。Qt はすべて offscreen。

## 1. 変更したファイル

- `repro/nv_depth.py` (新規)
  - 画素形式の表と変換: `PixFmt` (`plane_shapes`・`frame_bytes`・`internal_bytes`・`legacy`)、`pixfmt()`、`source_format()` → `SrcFmt`、`choose_format(src, depth)`、`to_internal` / `from_internal`、`full_range_internal`、`describe` / `describe_source`、`read_filter`、`split`。
  - 尺度: 整数は v/2^(b−8)、float は ×255、8 ビット (yuv420p) は uint8 のまま。RGB は `_KRB` の係数で full range の YCbCr 4:4:4 にし、`nv.yuv_to_rgb(full_range=True)` が逆変換になる。
  - 4:2:2 の色差の拡大: 偶数列 = 元の標本、奇数列 = 左右の平均、右端は複製。書き出しでは偶数列だけを戻す。
- `repro/denoise_video_v5.py`
  - `probe()` に `pix_fmt`・`bits_raw` を追加。
  - `read_frames(..., fmt=None, matrix="bt709", vf=None)`: `fmt=None` は従来と同一のコマンドで、uint8 を返す。
  - 新しい引数 `--depth {8,10,12,16,32,source}` (既定 8) と `--codec {auto,x264,x265,ffv1,prores,exr}`。
  - `pick_codec()` は SPEC の表どおりに選び、不可能な組み合わせは処理の前にエラーにする。
  - `encoder(..., fmt, codec)`: 8 ビット + x264 は従来のコマンドのまま。それ以外は入力・出力とも `-pix_fmt fmt.name` で、RGB には `-colorspace`・`-color_range` を付けない。
  - `is_sequence`・`sidecar_stem` を追加 (EXR 連番の .json は `%05d` を除いた名前)。
  - 8 ビットの書き出しは従来の行のまま、それ以外は `from_internal`。黒レベルとプロファイルは `full_range_internal`。
  - .json に `source_pix_fmt`・`pix_fmt`・`depth`・`codec` を書く。
- `repro/nv_ui_qt/main_window.py`
  - Preferences に Bit depth を追加: 8-bit (legacy yuv420p) / Source depth / 10 / 12 / 16 / 32-bit float (RGB)。既定は 8 で、変えると動画を読み直す。
  - 読み込み・Batch Profiler・書き出し (`--depth`) で同じ設定を使う。
  - メモリ上限は `fmt.internal_bytes` で計算する。
  - `self.full_range` (= `full_range_internal`) を使う。
  - 深度の表示: ステータスバーの `depth_lbl`、ビューアバーの `N-bit`、Image information。
  - 書き出しダイアログの既定拡張子を深度に合わせる。
  - 取り消し時に EXR 連番を後始末する。
  - プロファイルの `sub` が合わないと警告する。
- `repro/nv_ui_qt/export.py`: プロファイルの `sub` と処理形式が食い違えば止める。range の違いは警告だけ。
- `repro/nv_ui_qt/ofx_bridge.py`: 8/16/32 の frame_size・unpack・pack とプロトコルの docstring。8 は従来のまま。
- `repro/nv_ui_qt/main.py`: `--ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]` と、既存の不具合の修正 (§5)。
- `repro/nv_ui_qt/selftest.py`: 高ビット深度の 6 項目を追加。
- 試験用 (`sandbox/omo/coord/highbits/`): `make_synth.py`・`check.py`・`test_ofx.py`・`ofx_wrap.py`・`test/`。

## 2. 試験 (作業ディレクトリ `repro/`、T=../sandbox/omo/coord/highbits/test)

1. **8 ビットの回帰**
   - コマンド: `PYTHONPATH=$PWD python3 ../sandbox/omo/coord/backup/denoise_video_v5.py.20261026_pre_highbits ../test.mp4 $T/t1_old.mp4 --start 300 --frames 8 --crf 0 --jobs 4`。新しいスクリプトにも同じ引数を渡した。
   - test.mp4: framemd5 は 8/8 一致。mp4 もバイト一致 (MD5 e5a86aeb…dfe)。json の profile と params も一致。
   - test_v02 (`--start 304 --frames 6`): 6/6 一致。mp4 もバイト一致 (d53b7bfe…b46)。
2. **10 ビットの尺度の整合** (test.mp4 n=296〜315 の 8 ビット FFV1 と、その ×4 の yuv420p10le FFV1)
   - profile の json の全キーが一致した (σ Y 19.66037204849416 / Cb 4.19275381321132 / Cr 3.024636162413099)。
   - |out10 − 4·out8| の最大は YUV とも 2。平均は 0.9987 / 0.9992 / 1.0023。Y の度数は 0: 462509、1: 920660、2: 460031。
3. **10/12/16 ビット** (320×256・8 フレームの合成 FFV1、`--depth source`)
   - 出力の pix_fmt は入力と同じ。`--mix 1` は 4 形式とも入力とビット一致。
   - 格子の倍数でない値の割合: 0.7620 / 0.7620 / 0.9393 / 0.9962。
   - x264 10 ビットは High 10 yuv420p10le、x265 12 ビットは Rext yuv444p12le、ProRes 4:2:2 は HQ yuv422p10le。
   - 奇数サイズ (321×255) の FFV1 Mix 1 はビット一致。
   - 不可能な組み合わせ 6 種は、処理の前に終了コード 1 で止まる。
4. **32 ビット float** (gbrpf32le の FFV1 と EXR、値域 −0.0755〜1.4099)
   - Mix 1 の最大絶対誤差は 1.19e-07 (mkv・EXR とも)。
   - 通常処理の値域は −0.0903〜1.3725 で、1.0 超・負の割合は入力と同じ。
   - yuv ソースを float で読む経路と、ffmpeg の bt709 変換との差は最大 1.19e-07。
5. **UI**
   - selftest は 55/55 (既存 49 + 新しい 6)。
   - OFX の往復: --depth 8 / 16 / 32 で、1 フレームの stdout は 345600 / 691200 / 1382400 B。
   - |out16/256 − out8| と |out32·255 − out8| の最大はどちらも 0.5。
6. **py_compile**: すべて OK。

## 3. 判断・近似

1. **FFV1 の float**: ffmpeg 8.0.1 では `-level 4 -strict experimental` が必要。可逆なことは確認したが、他のツールで読めるかは未確認。
2. **full range の高ビットの白**: 255.75 になる (SPEC の近似)。
3. **4:2:2 の奇数列**: 補間なので雑音が約 1/√2 になる (合成素材の Cb σ は 4:2:2 3.713 / 4:2:0 4.587 / 4:4:4 4.694)。
4. **yuv ソースを RGB で読むとき**: `scale=in_color_matrix=…:in_range=…` を付ける。HDR の 1.0 超は内部で 255 を超え、クリップ扱いになる。
5. **RGB の色タグ**: primaries と trc だけを付ける。
6. **ソースの扱い**:
   - grey は yuv 4:2:0 として扱う。
   - grayf32 を source で読むと gbrpf32le になる。
   - RGB float を 10/12/16 で読むと gbrp になる。
7. **auto の表にない組み合わせ**:
   - 10/12 ビットで .mp4・.mov・.m4v・.mkv 以外の拡張子はエラー。
   - .m4v の 12 ビットは x265。
   - x265 は log-level=error。
8. **奇数サイズ**: 新しい経路で切り詰めるのは、x264・x265・prores で色差が間引かれた形式のときだけ。
9. **EXR**: 1 始まりで音声は落とす。`+faststart` は mp4・mov・m4v のときだけ。
10. **UI のメモリ**: 高ビットは float32。1080p 4:2:0 10 ビットは 12,441,600 B/フレームで、4 GB なら約 345 フレーム (計算値)。
11. **export.py の range**: 違いは警告だけ。
12. **OFX**: 閉じたときにプロファイルがなければ Auto Profile を作る。OFX の深度は `--depth` で決まる。

## 4. 残っている問題

1. **既存の不具合 (未修正)**: `widgets/filter_panel.py` の `_temporal` の `BeginRow("Noise Level Check")` がどのレイアウトにも入っておらず、独立したトップレベル窓になる。そのため `app.exec()` が戻らない。OFX モードは `app.quit()` で回避した。filter_panel.py は 02:27 に別の作業で更新されている。
2. **OFX の Apply**: プレビューの更新だけで、フレームを出すのは閉じたときだけ。
3. **OFX の Image information**: 形式が「unknown」と出る。
4. **x264 10 ビットの `--crf 0`**: 可逆ではない (可逆には `-qp 0` が要る)。
5. **未使用の import (既存)**: main.py の QMessageBox、ofx_bridge.py の os。
6. **AGENTS.md への反映**: 統括が行う。

## 5. 直した既存の OFX モードの不具合

1. `argv` が未定義だった。
2. `win.params()`・`win.process_frame()` が無かった。`state.backend()` と `backend.process_frame(engine.window(...))` に置き換えた。
3. プロファイルがないと処理できなかった。閉じたときに Auto Profile を作るようにした。
4. 差し替えた `closeEvent` が `MainWindow.closeEvent` を呼んでいなかった。
5. 閉じてもプロセスが終わらなかった。`app.quit()` を呼ぶようにした。
6. 引数が足りないと IndexError になった。usage を出すようにした。

---

# 第 2 回の修正 (VERDICT Round 1 への対応。実装者の最終応答から統括が転記、未検証)

指摘 1〜6 と 8 を直した。7 と 9 は SPEC の近似として残す。

## 変更点

1. **12 ビットの .m4v**: auto の x265 は .mp4/.mov だけで選ぶ。.m4v の 12 ビットは処理の前にエラーにする。
2. **容器に入らない組み合わせ**:
   - `trial_encode()` を追加した。プロファイルを作る前に、同じ符号器・pix_fmt・容器で 64×64 の 1 フレームを試しに書き、失敗すれば止める。
   - 途中で失敗したときは、BrokenPipe を捕まえて後のジョブを取り消し、`remove_output()` で dst (EXR 連番の書きかけを含む) を消して終了コード 1 で止める。トレースバックは出さない。
   - UI の後始末も `remove_output` を使う。
   - 符号器の引数は `_vcodec()` に一本化した。
3. **NaN/±inf**:
   - `nv_depth.sanitize()` を追加した。NaN → 0、+inf → その面の有限な最大値、−inf → その面の有限な最小値に置き換える。
   - 元の値は、非有限があったフレームの面だけ記録する。
   - `from_internal(..., restore)` で同じ画素に戻す。CLI・UI・OFX の 32 ビットで同じ扱い。
4. **docstring**: `nv_depth.read_filter` の記述を、swscale の変換誤差 (BT.709 の定義式との差は最大 0.0038) に訂正した。第 1 回の報告の「1.19e-07」は ffmpeg 自身との比較だった。
5. **4:2:2 の奇数サイズ**: 幅だけを偶数に切り詰める。
6. **msb 形式**: `*msb` の判定を直した。ほかに rgb96/rgba128 と rgb4_byte/bgr4_byte の扱いも直した。
8. **Frame Bitdepth**: 実際の処理深度を出す。

## 再試験 (出力は `test/r2/`)

- **容器の組み合わせ**: 7 件とも終了コード 1 で、ファイルはできず、プロファイルも作らない。12 ビットの .mp4/.mov への出力は成功した。
- **途中での失敗** (符号器を差し替えて試験。8 ビット mp4・10 ビット mkv・EXR): いずれも終了コード 1 で、トレースバックも残るファイルもない。
- **非有限の値**:
  - 検証者の nan.mkv と自作の素材 (NaN 616、±inf 各 8): 非有限の標本はビット単位で元に戻り、それ以外はすべて有限。
  - 出力の非有限の割合は入力と同じ (検証者の素材で 0.000026、修正前は 99.3%)。
  - Mix 1 の差は最大 1.19e-07。
  - UI の経路と OFX の 32 ビットでも有限になった。
- **4:2:2 の奇数サイズ**: 323×257 → 322×257 (x264・x265・prores)。4:2:0 は 259×195 → 258×194 (従来どおり)。
- **msb 形式**: ffmpeg の 267 形式と比べた不一致は 8 件で、すべて bayer_*16 (ffmpeg の一覧の深度が 8 と出る形式)。
- **8 ビットの回帰**: framemd5 は 8/8 一致し、mp4 もバイト一致した (MD5 e5a86aeb…dfe)。
- **既存の試験 3・4**: 修正前と同じ結果。
- **UI**: selftest は 55/55。OFX の往復は前回と同じ値。
- **py_compile**: すべて OK。

## 残っている問題

- 試し書きのたびに ffmpeg を 1 回余分に呼ぶ。
- 大きさに依存する失敗は試し書きでは捕まらず、途中で失敗したときの後始末で扱う。
- +inf の置き換え先 (面の有限な最大値) が巨大なときに溢れる可能性がある。
- 第 1 回から残っている問題: Noise Level Check の迷子の窓、OFX の Apply、OFX の Image information、FFV1 の float が実験扱い、x264 10 ビットの `--crf 0` が可逆でない。
