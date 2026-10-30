# VERDICT G2〜G6 (fix2-a, worktree n3fix-a) — 独立検証 verify-g2-6, 2026-10-30

## 判定: APPROVE（条件付き。LOW の指摘 3 件をマージ前に直すか文書に記すこと）

G3・G4・G5・G6・G2 は主張どおり動作しました。指摘は LOW 3 件と INFO 5 件です。
G6 で作業者は 2^46〜2^47 付近を「プロセスによって結果が変わり得る」として試験から外しましたが、実際には実アプリ同士で再現性のある差がある帯域です（L1）。

以下、「観測」は自分で実行した結果、「推測」はそれ以外です。検証用のファイルは `sandbox/omo/coord_fix2a/independent/` に置きました。壊したソースのビルドは scratchpad のコピーで行い、worktree のソースには触れていません。`git diff --stat` は検証の前後とも 24 files, +851/−198 です。

## 1. G3 pynum（int()/float() の Python 一致）: 合格

- **表の機械照合**（観測、`check_tables.py`）
  - `PY_ND_ZEROS` の 67 区間 × 10 = 670 文字が、Python 3.13.5（unicodedata 15.1.0）で `unicodedata.decimal` が値を持つ 127 以上の全文字と、集合も値も一致しました。
  - 1 文字ずつの `int()` とも 670/670 一致しました。
  - 非 ASCII の空白 19 文字は、`str.isspace()` が真になる 127 以上の文字の一覧と完全に一致しました。
  - Nd 以外の文字は Python でも C++ でも拒否されました: ²（No）、①（No）、Ⅻ（Nl）、½、U+066B、U+200B、U+FEFF、U+180E。
- **独立ファズ**（観測、`fuzz_pynum.py` と `pynum_drv`。pynum.cpp を単体でビルドした私のドライバ）
  - 375,170 件で不一致 0 でした。内訳は手で作った端値 170 件、乱数文字列 30 万件、文字種を混ぜた数値の形 7.5 万件です。float の比較は NaN の符号も含むビット列で行いました。
  - 手作りの端値に含めたもの:
    - 符号と空白の組み合わせ、`_` の位置（先頭・末尾・連続・`.`/`e` の隣・符号の直後）
    - `0x10`・`0b1`・`0o7`・`010`・`00`・`0_0`
    - 1e400・−1e400・1e-400、非正規数と正規数の境界、DBL_MAX の丸め境界、指数 1e±99999999999999999999
    - inf / infinity / nan の大小文字と符号、`nan(1)`
    - 全角・アラビア・デーヴァナーガリー・数学用ボールド・U+1FBF9 の数字
    - \x1c・\x7f・U+0085・U+2028、U+FFFD・サロゲート
    - 4300 桁と 4301 桁（`_` 入りを含む）、2^120 前後
- **実際の argv 経路**（観測、`real_apps.sh`）
  - `python3 -m nv_ui_qt.main --ofx …` と `MeatVideoQt.app … --ofx …` の実物どうしを、stdin を /dev/null にして比べました。
  - 全角空白と全角数字の W、アラビアの H、`nan` と負の MAX_FRAMES、`--depth` が末尾にある場合、`--depth` が 2 回ある場合、`-0`・`0`・`-1` で、終了コード・stdout・stderr の最終行が一致しました。
  - 不正な UTF-8 は統括の `compare_ofx_args`（W_invalid_utf8）で一致済みです（再実行はしていません）。
- **FPS 表示 `py_float_str`**（観測、`fstr.py`）
  - 200,026 値（指示の例 1e16・0.1・29.97・1e-5・−0.0・inf・nan と、ランダムなビット列・一様乱数）で、Python の `str()` と `f"{v}"` に完全に一致しました。

## 2. G6 エラー出力と極小フレーム: 合格（L1・L2 あり）

- **境界値**（観測、実アプリどうし）: 次はすべて一致しました。

  | 入力 | 最終行（両 UI 共通） |
  |---|---|
  | fs = 2^63−1−33 | `MemoryError` |
  | fs = 2^63−1−32 〜 2^63−1 | `OverflowError: byte string is too large` |
  | fs = 2^63 | `OverflowError: cannot fit 'int' into an index-sized integer` |

  `--depth 32` の W = 2^61 と 2^61−1、`--depth 16` の W = 2^62 も一致しました。

- **L1（LOW）: MemoryError の帯域で実アプリどうしに確定した差がある**（観測）
  - fs が 2^46（70,368,744,177,664）から約 1.0507e14 の間で、H = 1 なら W = 7.04e13・9e13・1.0507e14 のとき、両 UI の出力が分かれます。
    - Python: 読み込みに進み `no frames received on stdin` を出します。
    - C++（`MeatVideoQt` と `qtui_ofx_drv` の両方）: `MemoryError` の traceback を出します。
  - 終了コード（1）と stdout（0 バイト）は同じで、違うのは stderr の最終行だけです。
  - 上限を二分探索すると、Python は 105,072,079,340,449 で 3 回とも同じ値、C++ の 2 本は 70,368,740,048,896（刻み 2^24）、単純な C の malloc は 70,368,743,130,049 で、どちらも ≈ 2^46 でした。
  - **原因（観測）:** 上限は実行ファイルが何の SDK でリンクされたか（LC_BUILD_VERSION の sdk）で決まるようです。同じ C ソース `mallocmax.c` を `-Wl,-platform_version,macos,11.1,11.1` でリンクすると上限が 1.0507e14 になり、anaconda の python3.13（sdk 11.1）と同じでした。通常のリンク（sdk 26）では 2^46 です。
  - つまりこれはプロセスごとの揺れではありません。「参照に使う Python の実行ファイル」によって決まる、再現性のある差です。
  - 作業者の REPORT（「2^46〜2^47 付近だけはプロセスごとに結果が違い得る。試験から外した」）は、範囲も性質も正確ではありません。
  - malloc を実際に試す方式そのものは妥当と判断します。Python の上限が確保できるかどうかそのもので決まることは、`read(n)` と `PyBytes_FromStringAndSize` の上限が一致したことで確かめました（観測）。
  - 要求: REPORT と MIGRATION.md の既知差に、この帯域と原因を正しく書くこと。コードの変更は必須ではありません。64 TiB を超えるフレームは実用上ありえないためです。
- **L2（LOW）: traceback の行番号が古い**（観測）
  - C++ の traceback が出す行番号は main.py の 146・28・63 です。G2 で main.py を編集した後の実際の Python では 154・33・71 で、ずれています。
  - Python は、ほかに `<frozen runpy>` の 2 フレーム、絶対パス、3.13 のキャレット行も出します。
  - 最終行は一致しており、仕様は「簡易模倣でよい」としているので合否には影響しません。
  - 要求: 行番号を直すこと。できれば試験で Python の実際の traceback と行番号を突き合わせ、同じずれを防ぐこと。
- **極小フレーム（nvcore.cpp の素通し分岐）**（観測、`ind_emit.py`。作業者のスクリプトは使わず自分で作成。ドライバ `qtui_emit_sizes` だけ流用）
  - 作業者が使っていない構成 7 種で比べました: radius 0 の 5 フレーム、2 フレームの最終フレーム、3 フレームで Amount 63%、Amount 0 の 3 フレーム、1 フレームで Amount 0.5%、1 フレームで threshold 9・t_noise 50・noise_level 150、4 フレームで radius 1 の t = 3。
  - サイズ 24 種: 1×1、1×2、2×1、4×3、16×15、15×16、17×16、1×33、64×3、19×23 など。
  - 中身 4 種: 乱数・一定値・飽和多め・ランプ。depth は 8/16/32 を巡回させ、32 では NaN・−inf・1e6・−0 も混ぜました。
  - 計 672 件で、NV_NATIVE=1 と 0 のどちらも不一致 0 でした。内訳はバイト一致 340 件、例外文 4 種 332 件です。

## 3. G4 自己試験の強化: 合格

- **Python 版との読み比べ:**
  - **#7:** Python は 4 モードを回したあと、最後の Y Freq について `["rgb"] + PANES["Y Freq"]` を確かめるだけです。C++ は 7 モードすべてについてペイン名と順序を確かめます。照合に使う表は viewer.py の `PANES` と一致することを確認しました。Python より強い試験です。
  - **#9:** 試験の中身は Python と同じです（スライダー 50、キャッシュへの登録、`rot90(engine.assist(...))` との一致、effective が非 None、undo でキャッシュの画像に戻る）。C++ は開始時の画像の一致と、undo 後の `pc_cached` も確かめていて、これも Python より強いです。
  - **#96:** Python と同じです（ソースを読み直し、`.ui_profile.json` を使い、yuv420p10le で from_internal の比較）。
  - **項目 20:** Python の R3 パネル項目と、条件を 1 つずつ対応させて照合しました。
- **別の壊し方で確認**（観測、scratchpad のコピーでビルド）

  | 壊し方 | 旧試験 | 新試験 |
  |---|---|---|
  | viewer.cpp の Cb Freq のペインを 2 個にする | `componentPanesEveryMode` は PASS | 単独実行で `got == names (Cb Freq)` により FAIL |
  | NLC 終了時にラベルの styleSheet を戻さない | NLC の 5 項目は全部 PASS | `nlcPanelYellowAndDisabled` が `texts1 == texts0` で FAIL |
  | NLC 中に General タブを無効にしない | `nlcBeginShowsMask` で FAIL（既に検出されていた） | 項目 20 も FAIL |

  3 段目は、旧試験でも検出できた壊し方です。新しく検出できるようになったのは 1 段目と 2 段目です。ログは `independent/g4break/` にあります。

## 4. G5 単項目実行: 合格（L3 あり）

- `selftest_each.py` で全 108 項目を 1 項目ずつ実行し、108/108 PASS でした（観測、`selftest_each_all.txt`）。それとは別に、無作為に選んだ 22 項目も 22/22 PASS でした。
- 全体実行の最初の 7 項目は、すべて `without_window` の一覧に入っていました（init は何もしない）。
- **L3（LOW、文書）:** REPORT の「全体実行ではこれらは何もしません」は正確ではありません（観測）。
  - 各補助関数に「実際に動いた」ときの出力を足して全体を実行すると、`show_filtered_adjust` が performanceTable で動いていました（tab 1・idle・filtered 0 の状態から、filtered になるまで待つ）。
  - もう 1 つ動いた `make_v10`（bitDepth10Reload での作成）は本来の動作です。
  - 判定は変わらず、その scratch ビルドの実行では ofx 以外の 107 項目が PASS でした。ofx の 3 項目が落ちたのは、scratch に `qtui_ofx_drv` をビルドしていなかったためです。統括の全体実行は 110/110 です。
  - 要求: REPORT の記述を直すこと。

## 5. G2 ffmpeg 欄: 合格

- **探索順の照合**（観測、`g4break/ffcmp.py` と C++ ドライバ `ffdrv.cpp`。libnvui にリンク）
  - C++ の `find_ffmpeg_dir` / `ffmpeg_tools_in` と、Python の `ffmpeg_path.find_ffmpeg_dir` / `tools_in` を 240 件で比べ、不一致 0 でした。
    - 設定値 16 種: 空、ディレクトリ、末尾の `/`、バイナリ、ffprobe、`..`、ffprobe がない、実行権限がない、ffmpeg がディレクトリ、シンボリックリンク、存在しない、相対パス
    - NV_FFMPEG 5 種
    - PATH 3 種（空要素を含む）
  - ただしこの機械には `/opt/homebrew/bin` に ffmpeg があるため、固定 3 か所より後ろにある PATH の順番は、この照合では確かめられていません。PATH の順番は test_launcher（12/12）が FFMPEG_DIRS を空にして確かめています（作業者のログ）。
- **Python の Preferences**（観測、`g4break/pyprefs.py`）
  - ffprobe のないディレクトリ、ffprobe のない binary、`~` のパス: 警告が出て、ダイアログは閉じず、`prefs/ffmpeg` は書かれませんでした。
  - 前後に空白を付けた有効なディレクトリ: 前後を削って保存されました。
  - 空白だけ: キーが削除されました。
  - `auto_fine_tune` は以前から PREFS にあるキーで、使われ方（main_window.py:1232）も同じです。読み書きの形式（bool → "true"）も C++ と揃っていて、矛盾はありません。
- **両 UI の比較:** `compare_n3c` を再実行し EQUAL でした（観測）。Preferences は 11 行対 11 行で、違いは既知の native 欄だけです。dlg_prefs.png の差は 1.312% で、作業者の報告と一致しました。settings.ini の [prefs] 10 キーも一致しました。
- C++ / Python の単項目 `preferencesFfmpegRow` は単独実行で PASS でした（観測）。

## 6. 変更範囲・警告・報告の数値

- **変更範囲**（観測）: `repro/nvfilter.py`・`nv_aft_v5.py`・`nv_depth.py`・`native/` には変更がありません（git status が空）。
- **警告**（観測）:
  - scratch での qtui のクリーンビルド（configure から）で warning 0。
  - 作業者のクリーンビルドのログでも 33 ファイルのコンパイルで warning 0。
- **報告の数値**（観測。作業者のログで照合）: 以下はすべて一致しました。
  - ファズ 800,159（= 200,053 + 300,053 × 2）
  - emit 4810 ジョブ、バイト一致 2967、例外 1843（= 455 + 174 + 150 + 1064）、壊した版の差 440
  - selftest 110/110 × 2、単項目 108/108、test_launcher 12/12
  - Preferences ダイアログの grab 342×450（153,900 px）

## INFO（対応は任意）

- **I1:** pynum に NUL を埋め込むと Python と違う結果になります（観測）。`"1\0"` と `"1\0002"` を py_int が 1 として受け付け、`"inf\0x"` を py_float が inf として受け付けます。Python ではどれも ValueError です。argv は NUL を含められないので、OFX の経路では起きません。pynum.h が「Python の int(str) と同じ」とうたうなら、注記するか、文字列の末尾まで読んだことを確かめるようにするとよいです。
- **I2:** `ffmpeg_path.py` は `.gitignore` の `repro/**` に掛かります（観測、`git status --ignored` で `!!`）。main.py と packaging/main.py がこれを import するので、マージで漏れると両方の Python 起動が壊れます。必ず `git add -f` すること。作業者も報告済みです。
- **I3（推測）:** Python は見つけたディレクトリを PATH の先頭に足し（`use_dir`）、C++ は PATH を変えません。そのため、ユーザー設定のディレクトリを使ったあとに欄を空にした場合、固定 3 か所に ffmpeg がない環境では両 UI の探索結果が変わり得ます。この機械では固定の場所が先に見つかるので差は出ません。
- **I4:** `~user/...` 形式の展開が違います（C++ は先頭の `~` をホームに置き換えるだけ）。変更前からある挙動で、ごくまれな入力です。
- **I5:** `selftest_each.py --only` で存在しない名前を指定すると 0/0 で終了コード 0 になります。指定した項目数が 0 なら失敗にするとよいです。

## 指摘の一覧

| ID | 重大度 | 内容 | 要求 |
|---|---|---|---|
| L1 | LOW | fs が 2^46〜約 1.05e14 のとき、Python は no frames、C++ は MemoryError（実アプリどうしで再現。原因はリンク SDK による malloc の上限） | REPORT・MIGRATION の記述を正しい範囲と原因に直す |
| L2 | LOW | traceback の main.py の行番号が古い（146/28/63 → 154/33/71） | 直す（試験で突き合わせるのが望ましい） |
| L3 | LOW | 「全体実行で補助関数は何もしない」は不正確（performanceTable で show_filtered_adjust が待つ） | REPORT を直す |
| I1〜I5 | INFO | 上記 | 任意 |

---

# Round 2（L1〜L3・I3・I5 への対応の再検証、2026-10-30）

## 判定: APPROVE

L1〜L3・I3・I5 はいずれも解消しています。新しい指摘は INFO 1 件（R2-I1）だけです。

以下はすべて自分で実行した結果（観測）です。推測にはその旨を書いています。

- **手順:** 第 1 回の時点の写しが scratchpad にあるので（`g4break/qtui/`）、それと今のファイルを直接 diff して、第 2 回で何が変わったかを特定しました。
- **成果物:** `sandbox/omo/coord_fix2a/independent/r2/` に置きました。
- **触れていないもの:** worktree のソース。触れたのは scratchpad のコピーだけです。

## L1（MemoryError の帯域の記述）: 解消

- **ofx.cpp:** 第 1 回の写しとの差は 2 か所だけです。`read_exception` の上にあるコメントの書き換えと、`print_read_exception` のコメントと行番号の定数（L2 の分）です。実行されるコードに変更はありません。
- **再実行の結果:** 実アプリどうしの比較（`real_apps.sh`）を再実行したところ、SAME 19 / DIFF 3 で第 1 回と同じでした。DIFF の 3 件は、どれも説明どおりの帯域に入る W = 2^46・1.0507e14・1.0507e14+1 です。
- **SDK の版:** otool で確かめると、MeatVideoQt と qtui_ofx_drv が sdk 26.5、python3.13 が sdk 11.1 で、報告の記述どおりでした。
- **REPORT と MIGRATION 追記案の記述:** 私の観測と合っています。
  - 細かく言うと、C++ が失敗し始める境界は 2^46 ちょうどではなく、malloc(33+fs) の分だけ少し下です。測った値は 70,368,740,048,896（刻み 2^24）と、単純な C の 70,368,743,130,049 です。
  - MIGRATION の案は「約 2^46」と書いているので問題ありません。REPORT の「2^46 以上」も実用上は差し支えありません。

## L2（traceback の行番号）: 解消

- **行番号:** C++ の 5 フレーム（main.py 157 / 34 / 73、ofx_bridge.py 110 / 39）を今のファイルと突き合わせました。行番号もソースの文字列も 5/5 一致しました（linecache で確認）。
- **compare_ofx_args.py の size グループ:** 14/14 一致しました。実際の `python3 -m nv_ui_qt.main` との照合も、フレーム 5/5 と最終行 `MemoryError` が一致しました（`r2/compare_ofx_args_size.log`）。
- **ずれを本当に検出するか:** 作業者とは別の方法で確かめました。
  - scratchpad に repro のコピーを作り、`ofx_bridge.py` の read_all の中に 1 行足しました。
  - その実際の Python の traceback と MeatVideoQt の traceback を、ツールの `tb_frames` で比べました。
  - worktree では SAME [157, 34, 73, 110, 39] でした。コピーでは DIFF になり、Python 側だけ 110 → 111 に変わっていました。ずれは検出されます。
- **Python 側に足した注記行:** 3 か所（main.py 2・ofx_bridge.py 2 の計 4 行）とも `#` で始まるコメントだけです。動作は変わりません。注記の行番号を含めた全体の照合は上のとおり一致しています。

## I3（PATH の前置）: 解消（R2-I1 あり）

- **コードの差:** 第 1 回の写しとの差は 3 つです。
  - `search_path()` を追加した
  - `find_ffmpeg_dir` の PATH の候補を `search_path()` に切り替えた
  - `use_dir` が `search_path()` を元に PATH を作り、`NV_FFMPEG_PATH0`（前置する前の PATH）と `NV_FFMPEG_PATH_SET`（設定した PATH）を記録するようにした
- **独自のシナリオ**（`r2/i3_ind.py`。固定の 3 か所を空にしたうえで実行）: 7/7 合格でした。
  1. X → Y → 空 → X と切り替えると、結果は X・Y・Z・X になりました。PATH に X は 1 回だけで Y は残らず、PATH0 は最初の PATH のままで、エントリーが溜まりませんでした。
  2. 子プロセスで、`ffmpeg` と `ffprobe` を名前で実行すると X のものが動き、子の `setup()` / `ffmpeg_dir()` も X を返しました（NV_FFMPEG_DIR を受け継ぐため）。
  3. use_dir の後に外から PATH を変えると、今の PATH が読まれました（変更前と同じ挙動に戻る）。
  4. 元から PATH にあった Z は、別の設定にして欄を空にした後もまた見つかりました。
  5. 外から残っていた PATH0 は、PATH_SET が今の PATH と違えば無視されました。
  6. 下の R2-I1 のケース。
  7. どこにも ffmpeg がない場合は `""` になり、PATH は変わらず、PATH0 も記録されませんでした。
- **両 UI とランチャの一致:**
  - C++ の `find_ffmpeg_dir` / `ffmpeg_tools_in`（第 1 回のドライバ。video.cpp は第 1 回から変更なし）と ffmpeg_path を 384 件で比べ、不一致 0 でした（`r2/ffcmp2.py`）。第 1 回の 240 件に、PATH0 / PATH_SET が「整合している」「PATH0 がでたらめ」「PATH_SET が別の値」の 3 通りの環境を足したものです。
  - ただしこの機械では固定の場所 `/opt/homebrew/bin` が先に見つかるので、PATH の違いはこの比較には表れません。
  - `packaging/test_launcher.py` は 13/13 でした。ランチャの find と ffmpeg_path の find が一致すること、I3 の項目を含みます。
- **退行がないか:**
  - `env -i PATH=/usr/bin:/bin` で `setup()` すると PATH = /opt/homebrew/bin:/usr/bin:/bin、PATH0 = /usr/bin:/bin になりました。
  - その環境で子プロセスとして `denoise_video_v5.py` を 3 フレームだけ書き出すと、終了コード 0 で出力ができました（ffprobe と ffmpeg を名前で呼ぶ経路）。
  - Python の自己試験の全体は再実行していません。統括と作業者の 97/97 に依ります。
- **R2-I1（INFO）: 整合しない PATH0 が外から残っていると信用してしまう**
  - PATH_SET が今の PATH と同じで、PATH0 がそれと無関係な値（例 `/bogus`）だと、PATH0 がそのまま使われます。その結果、PATH に Z があるのに「見つからない」になりました（観測）。
  - ただし我々のプロセスが書いた PATH0 / PATH_SET なら、PATH_SET は「先頭 + (PATH0 から先頭を除いたもの)」の形なので、この不整合は手で設定しない限り起きないとみられます（推測）。
  - 気になるなら、`search_path` で PATH_SET がこの形になっているかを確かめ、合わなければ今の PATH を使うようにすると堅くなります。

## I5（selftest_each の 0 件）: 解消

次のように、存在しない名前が 1 つでもあれば終了コード 1、全部が実在すれば 0 になりました（パイプを使わず直接確認）。

| `--only` の値 | 終了コード |
|---|---|
| `nosuch` | 1 |
| `,`（0 件） | 1 |
| `pyFloatRepr,nosuch` | 1（何も実行しない） |
| `pyFloatRepr` | 0 |

## L3（G5 の記述）: 解消

書き直した記述は、私が全体実行に計装して得た結果と一致しています。

- `show_filtered_adjust` が動くのは performanceTable だけです（tab 1・idle・filtered 0 から、filtered になるまで待つ）。
- `make_v10` は bitDepth10Reload で作る本来の動作です。
- それ以外の補助関数と `ensure_window` は何もしません。

「状態を変えず」という表現は、`setCurrentIndex(1)` が何も変えないことと、待つあいだにイベントを処理するだけであることを指していて、正確です。

## 再検証していないもの

- qtui の警告 0（統括の差分ビルドに依る）
- `qtui_selftest` の全体
- Python 自己試験の全体
- verify_bridge

いずれも第 2 回の変更（コメント・定数・ffmpeg_path・ツール）の範囲外か、統括・作業者が再実行済みです。
