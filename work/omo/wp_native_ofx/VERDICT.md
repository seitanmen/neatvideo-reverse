# N4a/N4b 独立検証 VERDICT — MeatVideo.ofx (worktree n4-ofx、未コミット)

検証者: verify-n4 (統括の依頼)。日付 2026-10-31。
対象: `ofx/` (新規) と `.gitignore`。タスク `.claude/tasks/native_n4_ofx.md` の N4a・N4b、作業者の報告 `REPORT.md`。
ソースは編集していない。本家のバンドルは実行・dlopen・リンクしていない (静的な読み取りだけ: `nm`・`llvm-objdump`・`plutil`)。Resolve と `/Library/OFX/Plugins/` には触れていない。
検証用の補助ファイルとログは `verifier/` に置いた (scratchpad は揮発するため)。

## 判定: **APPROVE**

M (重大) の指摘はない。L が 4 件、I が 9 件ある。L は Resolve の実機確認の前か N4c で直すことを勧めるが、N4a/N4b の合格条件 (3 関数のエクスポート、ホストの画像だけを使う、ミニホストでのビット一致) は満たしている。

---

## 1. 観測 (自分で実行した結果)

### 1.1 ビルド・バンドル構成
- scratchpad で `ofx/build_ofx.sh <OUT>` をクリーンビルドし、exit 0、"warning" は 0 件だった (`-Wall -Wextra`)。
- エクスポートは `nm -gU` でも `dyld_info -exports` でも `_FoundryOfxAllowPluginUnload` / `_OfxGetNumberOfPlugins` / `_OfxGetPlugin` の 3 つだけ。
- 本家の arm64 スライスも、同じ 3 つを静的に確認した。本家の `FoundryOfxAllowPluginUnload` は `mov w0,#0; ret` で、実装の 0 と一致する。
- `MacOS/MeatVideo.ofx` の import は、libc++ のランタイム・`memcpy`/`memmove`/`bzero`/`strcmp`/`strlen` と、`nvc_*` の 12 関数だけ。ファイル系・プロセス系の関数はない。
- libnvcore は `@rpath/libnvcore.dylib` として参照され、LC_RPATH は `@loader_path/../Frameworks` の 1 本だけ (ビルドツリーの rpath は削除済み)。
- `Info.plist` の内容:
  - `com.meatvideo.MeatVideo-OFX`、BNDL、1.0.0、LSMinimumSystemVersion 26.0。`plutil -lint` を通る。
  - キーの構成は本家 `reference/genuine/NeatVideo6.ofx.bundle/Contents/Info.plist` と同じ形。本家にある DT* / BuildMachineOSBuild / NSHumanReadableCopyright は無い。
- 構成の違い: 本家は `Contents/{Info.plist, MacOS/}` だけだが、実装には `Frameworks/libnvcore.dylib` と `Resources/THIRD_PARTY.txt` がある。
- 署名: ad hoc。`codesign --verify --strict --deep` は OK。
- アーキテクチャは arm64 のみ (本家は x86_64+arm64 の Universal、最低 10.13)。

### 1.2 ファイル・ffmpeg・stdin を使っていないこと

**静的**
- libnvcore の `fopen`/`fread`/`fclose` スタブを呼ぶのは `nvp::load_reference_floor` だけ。その呼び出し元は `_nvc_reference_floor` (0x357f4) の 1 か所だけ。アドレスが関数ポインタとして使われている箇所もない。
- プラグインは `nvc_reference_floor` を import していない。
- libnvcore はほかに getenv、exec、spawn などを import していない。
- バイナリの文字列に、.json/.npy/.ini やユーザーのパスはない。
- 埋め込みの `ref_floor.inc` は `data/ref_floor.npy` と 51/51 語がビット一致した。`minihost --check-floor` (計算値) とも 0/51 の差。

**動的 1: fsguard**
- 作業者の 5 場面と、私の 6 場面のすべてで、計測中の記録はホスト自身の `dlopen(bundle)` の 1 件だけだった。
- 陽性対照:
  - `minihost --fsguard-selftest` で 5 件を記録した。
  - 追加の陽性対照として、libnvcore 自身の fopen 経路を確かめた。`verifier/pc_lib.c` から `nvc_reference_floor(path)` を呼ぶと、`fopen(.../data/ref_floor.npy)` が記録された。つまり libnvcore 内部の fopen も捕捉できる。

**動的 2: sandbox-exec (fsguard とは別の手段)**
- 使ったプロファイル `verifier/final.sb` の中身:
  - `/Users`・`/private/tmp`・`/private/var/folders`・`/Volumes`・`/Library/OFX`・`/Library/Application Support`・`/opt`・`/usr/local`・`/Applications` の読み取りを SIGKILL で禁止する。許可するのはホストの実行ファイル、そのディレクトリ、バンドル、フレームファイル、ログだけ。
  - 書き込みは出力・ログ・/dev 以外を禁止。fork・exec・network も禁止。
- A-params (10 レンダー、プロファイルの作り直しを含む) と D-1080p (2 レンダー) が exit 0 で完走した。出力は、fsguard 下で照合済みの出力とバイト単位で同一 (`cmp`)。
- 陽性対照: 同じプロファイルで、ホストが `data/ref_floor.npy` を読むと exit 137 になり、許可外のファイルへ書いても exit 137 になった。
- 補足: プロファイルの調整中に、ホストの実行ファイルのディレクトリ自体の読み取りでも kill された。原因の切り分けとして、引数なしで即終了する minihost でも起きることを確認した。プラグインと無関係な起動時の dyld の動作なので、そのディレクトリだけ許可した。

### 1.3 ビット一致

**作業者の照合の再実行**
- `verify_ofx.py --pure --mutate` を、私のクリーンビルドで再実行した (`verifier/run_verify.py` でパスだけ差し替え)。結果は 48 比較、0/66,248,704 語の不一致、ALL MATCH。
- インターレースの拒否、mutation 1 (5/5 レンダーで差)、mutation 2 (2/5、9 語) も報告どおりだった。

**参照側の独立性 (コードを読んだ結果)**
- NV_NATIVE=0 の参照は C を通らない:
  - 画素の変換は `nv_depth.to_internal`/`from_internal` (numpy) で、`nvc_depth_*` は使わない。
  - プロファイルは `nv.build_profile_v5`、AFT は `nv_aft_v5.apply_aft` (Python)、フレーム処理は `nvfilter` (Python)。
  - パラメータは `state.backend_params(state.coerce(...))` で組む。窓は Python 側で `[k−R, k+R] ∩ [0, N)` として組み立てている。
- NV_NATIVE=1 の参照について:
  - 核は同じ C 関数だが、呼んでいる libnvcore はリポジトリの `native/libnvcore.dylib` で、バンドル内のコピーではない。
  - 画素の変換・パラメータの写像・窓・プロファイルと AFT の組み立ては Python なので、プラグイン側の結合部 (core.cpp・params.cpp) とは独立している。
- 弱い点: `OFX_DEFAULTS` は verify_ofx.py に複写したもので、プラグインの既定値が誤っていれば参照も同じ値を使ってしまう。そこで既定値と範囲を `state.PARAM_SPECS` / `default_params()` と機械的に比べた。13 個の UI パラメータの既定値・範囲はすべて一致した (bool は範囲の概念がないので対象外)。
  - `amt_C` は Cb・Cr の両方の既定値 100 と一致した。
  - プロファイル関係の既定値も UI の prefs と一致した (auto、AFT 有効。PySide6 は `main_window.py:54`、qtui は `mainwindow.cpp:404`)。

**照合が空振りしていないことの追加確認**
- A-default の出力は全画素が入力と異なる (Y の RMS 差 19〜23 階調)。
- A-params の 10 レンダーは、どの 2 つも互いに異なる (45/45 組)。
- α は入力とビット一致した。C の非有限値 9 個/フレームはビット単位で元に戻っていた。

**私が足した入力 (`verifier/run_extra.py`、作業者が使っていないフレーム・条件)**
- 6 場面 20 レンダー × 参照 2 種 = 40 比較で、すべて MATCH (0 語の不一致)。
- 宣言外のフレームの取得は 0、画像はすべて解放された。
- 場面の内容:

| 場面 | 入力 | 条件 |
|---|---|---|
| E | test.mp4 n=600〜602、奇数サイズ 333×187 | first 7、原点 (−3, 5)、負の rowBytes、R=5 で窓が 3 フレームに切れる、Profile Frame がクリップ末尾 (後続フレームなし)、BT.601、続けて spatial モード |
| F | 1 フレームのクリップ 128×96 | 極端なパラメータ: 全体 400 / Y 250 / Cb 0 / amt_Y 0 / 空間 4 / 閾値 0.25、temporal モード |
| G | ちょうど 64×64 | R=0、Mix 100 と 45.5、t_amount 33.3、noise_Cr 175.25 |
| H | test_v02 の区間 B n=560〜563、720×404 の切り出し | 原点 (600, 500)、範囲外の値 (閾値 100・半径 9・noise_Y −5・amt_C 250) を coerce に通す |
| K | ノイズのない一定の灰色 | flat_status 1 の経路 |
| L | ×40 の HDR グラデーション + ノイズ | NaN・±inf を R/B/α に入れる、BT.2020、AFT なし |

- 最後のインターレース場面だけは、私のクリップに A がないため照合スクリプト側の KeyError で止まった。照合は全件済んだ後で、ハーネスの都合による停止。

**レンダー窓・取り消し (`verifier/minihost_v.cpp`。minihost の scratch 用の写しに、窓の縮小と abort の注入を足したもの)**
- 窓を 37/74/111/37 px 縮めた場合、窓の内側は全面のレンダーとビット一致し、外側は 0 のまま (未書き込み) だった。
- abort を 1〜8 回目の問い合わせで返させた場合 (プロファイル構築の前・途中・後のすべて):
  - そのレンダーは status 1 で、画像はすべて解放された。
  - 直後の t=0 と t=4 の再レンダーは、中断なしの出力とビット一致した。
  - プロファイル構築の後で中断した場合は、プロファイルがキャッシュから再利用された (frames_needed が [0 2] だけになる)。
- 拒否の確認: 63 px のフレームは status 5 で「below 64 x 64」、FieldOrder Lower は status 5 で「interlaced」と表示された。

### 1.4 既存の検証
- qtui を scratchpad の新しいビルドディレクトリで全ターゲットをビルドした。"warning" は 0 件。`qtui_selftest` は 117 passed / 0 failed。
- `~/.config/NeatRepro/nv_ui_qt.ini` の SHA-1 は実行前後で同じだった。
- `make -C native verify` は回していない。理由は次のとおり:
  - `git status` で変更は `.gitignore` と新規の `ofx/` だけで、`native/`・`qtui/`・`reference/` に差分がない。
  - `make -C native lib` も "Nothing to be done" だった。
  - 並行作業の負荷を避けたかった。
- `minihost --check` は CHECK OK (0 failures)。

### 1.5 OpenFX ヘッダの出典・ライセンス
- 8 本は Resolve の開発キット `/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/OpenFX/OpenFX-1.4/include/` の同名ファイルと `cmp` で同一だった。SHA-256 も SOURCE.md の記載と一致した。
- 8 本とも `SPDX-License-Identifier: BSD-3-Clause` を持つ。ライセンス表記のない Resolve 拡張 (ofxImageEffectExt.h・ofxParamExt.h) は写していない。
- `THIRD_PARTY.txt` に LICENSE と LZMA SDK の注記がバンドルされている。記録は概ね適切 (I-5 を参照)。

---

## 2. 指摘

### L-1 (L): Render が kOfxStatErrUnsupported (5) を返す
- 該当: `plugin.cpp:365, 370, 384` と `fetch()` の 285/289。
- ヘッダが定める Render の戻り値は kOfxStatOK / kOfxStatErrMemory / kOfxStatFailed / kOfxStatErrFatal で (`ofxImageEffect.h` の kOfxImageEffectActionRender の @returns)、ErrUnsupported はその中にない。
- 永続メッセージは出しているので、kOfxStatFailed にすれば規約どおりになる。
- Resolve が 5 をどう扱うかは未確認 (推測)。
- 再現: `minihost BUNDLE --frames F --size 80 63 2 --render 0` は status=5。`--field-order OfxFieldLower` も status=5。

### L-2 (L): プロファイル構築の間、ホストへのコールバックをまたいでインスタンスのロックを持ち続ける
- `render()` は `in->mu` を持ったまま `clipGetImage`・`abort`・数秒かかる `build_profile` (1080p で 1〜2 秒、AFT 込み) を実行する (`plugin.cpp:401-420`)。
- 同じ mu を `frames_needed()` (345-348) と `instance_changed()` (230-232) も取る。
- 影響 (推測):
  - Resolve が構築中に別スレッドから GetFramesNeeded や InstanceChanged (UI スレッド) を呼ぶと、その間ずっと止まる。UI が固まって見える。
  - ホストが `clipGetImage` の中で同じインスタンスの別アクションを同期的に待つ作りなら、デッドロックになり得る。
- 提案: 構築はロックの外で行い、結果を入れ替えるときだけロックする。frames_needed は原子的な旗だけを見る。
- 再現: コードを読むことでの指摘。ミニホストは単一スレッドなので、動的には再現していない。

### L-3 (L): GetFramesNeeded と Render で、プロファイルを作り直す条件が違う
- frames_needed の `need_profile` は `!has_profile || key != settings` だけを見る (347)。
- render は、それに加えて `key_time != pf` (クリップの FrameRange の先頭が変わった場合) と `key_W/H` の変化でも作り直す (404)。
- このため、パラメータは同じままクリップの範囲や大きさだけが変わると、render が宣言していないフレーム (pf..pf+2) を取得する。
- Resolve でトリムやタイムラインの解像度変更がこの状況になるかは未確認 (推測)。
- 提案: frames_needed でも pf と、分かれば大きさを比べる。
- 再現: コードを読むことでの指摘。minihost は FrameRange が固定なので、動的には再現していない。

### L-4 (L): 近傍フレームが取得できないと、レンダー全体が失敗する
- `planes_at()` → `fetch_planes()` → `fetch(required=true)` で、`clipGetImage` が kOfxStatFailed を返すと例外になり、render は失敗する (`plugin.cpp:277, 303`)。
- ヘッダの clipGetImage の説明では、kOfxStatFailed は「その時刻に画像がない。プラグインは処理を続けること」とされている。
- Resolve で FrameRange の内側なのに近傍が返らない場合 (編集点の外のメディアなど) には赤いエラーになる (推測)。
- 提案: 時間窓の近傍は、取得できなければ窓から外す。現在のフレームとプロファイルのフレームだけを必須にする。窓を外す場合は UI 側の窓 (クリップ範囲で切る) との一致条件が変わるので、REPORT に条件を書く。
- 再現: コードを読むことでの指摘。

### I-1: abort のときに kOfxStatFailed をメッセージなしで返す
- 該当: `plugin.cpp:388, 409, 421, 432`。
- 規約上、Failed は「メッセージを出す」が前提。Resolve がこれをエラー表示にするかは未確認 (推測)。
- 動作自体 (画像の解放、その後の再レンダーとの一致) は 1.3 のとおり問題ない。

### I-2: Mix with Original の値の向きが UI の表示と逆
- OFX のパラメータは内部の mix をそのまま使う (0 = フィルタ結果のみ、既定 0)。
- PySide6 と qtui のパネルは、本家の向きに合わせて 100 − mix を表示する (`state.py:95-97`、`filter_panel.py:258`)。
- 式は同じで (backend_params と一致)、ヒントにも書かれているが、「UI と同じ単位」としては表示が逆。N4c で揃えるかどうかを統括が判断する。

### I-3: 「丸めの違い」の影響範囲
- OFX の `params.cpp:65` は `std::round` (四捨五入)。一方、Python の `state.coerce` と **qtui の C++** (`qtui/src/core/state.cpp:170` の `std::nearbyint`) は銀行丸め。REPORT の「C++ は四捨五入」は「OFX の params.cpp は」と書くのが正しい。
- この差には到達しない: `read_values` は Int・Bool・Choice をホストから `int*` で受け取るので、x は常に整数になる (`plugin.cpp:210-212`)。
- Double は丸めずに clamp するだけで、Python と同じ。
- 同じく到達しない差として、Double が NaN の場合、C++ は fmax/fmin で lo になり、Python の min/max は NaN のままになる。

### I-4: Color Matrix の既定が常に BT.709
- UI はタグのない素材を `nv.matrix_from_tags` で 720 行未満なら BT.601 にする。
- OFX にはタグがないので固定値は妥当だが、REPORT に書かれていない。

### I-5: LICENSE の出典が書かれていない
- 開発キットには LICENSE ファイルがない。`vendor/openfx/LICENSE` の本文は、ヘッダの著作権行に標準の BSD-3-Clause の本文を合わせたものとみられる (推測)。
- SOURCE.md にその旨と、上流 (AcademySoftwareFoundation/openfx の LICENSE) を一行足すとよい。

### I-6: work/ が gitignore で無視される
- `.gitignore` に `!work/` がないため、`work/omo/wp_native_ofx/` の REPORT・PROGRESS・本 VERDICT・`verifier/` は、そのままではコミットされない。作業者も報告している。統括がコミットするときに扱いを決める。

### I-7: メモリ量
- 観測: 1080p・R=2 で、minihost のプロセス全体 (ホストが持つフレーム約 95 MB を含む) の最大 RSS は 818 MB。
- 推測: 4K・R=5 では窓 11 フレームの内部面などで数 GB になり得る。Resolve で複数ノードを使う場合は注意。

### I-8: 未試験の経路
- 1 プロセスで複数インスタンスを同時にレンダーする場合 (InstanceSafe の申告)。
  - 静的に見た範囲では、libnvcore の可変な static はミューテックスで守られたキャッシュ (`nv_curve.cpp:354`、`nv_model.cpp:175, 954`) と thread_local だけ。
  - 動的には試していない。
- 画像の `kOfxImagePropField` と `kOfxImageEffectPropFieldToRender` による拒否 (minihost は常に None を渡す)。
- 非整数の時刻での動作。

### I-9: 本家と違う既知の点
- arm64 のみ、最低 macOS 26 (本家は Universal・10.13)。
- 署名は ad hoc。
- Profile Frame を「Source クリップの FrameRange の先頭」から数える前提が Resolve で成り立つかは未確認 (推測。REPORT にも記載あり)。

---

## 3. 再現手順

`S` = scratchpad の作業ディレクトリ、`WT` = worktree。

```
ofx/build_ofx.sh $S/build                                    # クリーンビルド、warning 0、エクスポート 3 つ
VN4_BUILD=$S/build python3 verifier/run_verify.py --pure --mutate --work $S/verify     # 48 比較 ALL MATCH
VN4_BUILD=$S/build python3 verifier/run_extra.py  --pure --work $S/verify_extra        # 追加 40 比較 MATCH (末尾の KeyError はハーネスの都合)
clang++ -std=c++17 -O2 -I$WT/ofx/vendor/openfx/include verifier/minihost_v.cpp -o minihost_v   # ref_floor.inc の include パスを直す
MH_SHRINK=37 ./minihost_v ...   /  MH_ABORT_AT=k ./minihost_v ... --render 0,0,4
sandbox-exec -f verifier/final.sb $S/build/minihost $S/build/MeatVideo.ofx.bundle --frames <clip> ...   # パスは書き換えて使う
DYLD_INSERT_LIBRARIES=$S/build/libfsguard.dylib ./pc_lib <bundle>/Contents/Frameworks/libnvcore.dylib $WT/data/ref_floor.npy
QT_QPA_PLATFORM=offscreen $S/qtbuild/qtui_selftest           # 117/117
```

ログ: `verifier/log_verify_full_pure_mutate.txt`・`log_verify_extra.txt`・`log_qtui_selftest.txt`。

---

# Round 2 (2026-10-31)

対象: 作業者が L-1〜L-4・I-1・I-2・I-4 に対応したあとの `ofx/`。`git status` で見た変更は、引き続き `.gitignore` と `ofx/` だけ。
補助ファイルとログは `verifier/round2/` に置いた。制約は前回と同じで、ソースの編集や本家の実行はしていない。

## 判定: **APPROVE** (Round 1 の L 4 件はすべて解消を確認、新しい M・L はない)

## R2-1. 指摘ごとの確認 (観測)

| 指摘 | 判定 | 根拠 |
|---|---|---|
| L-1 | 解消 | `plugin.cpp` から `kOfxStatErrUnsupported` がなくなった (grep 0 件)。拒否 6 種 (インターレース上・下、64 px 未満、8 ビット、16 ビット、プロファイルのフレームの欠け) は、すべて status 1 とメッセージで返り、fsguard の記録は 1 件だけ。GetClipPreferences は Source・Output に RGBA/Float を設定し、ヘッダの仕様に沿っている |
| L-2 | 解消 | コード上、`mu` の中ではキーの比較・コピー・世代番号 `gen` の読み書きだけを行う (446-451、466-471、386-388、254-256)。clipGetImage・RoD の問い合わせ・プロファイル作成はすべてロックの外。動的な確認は 2 つ: (a) 作業者の `--reenter` (同じスレッドから再入) の場面が合格。(b) 私が追加した**別スレッドからの同時呼び出し**の試験も合格。内容と結果は表の下 |
| L-3 | 解消 | GetFramesNeeded と Render が同じ `profile_key` (設定・pf・pf での RoD) を比べる。範囲の変更を含む場面 (作業者の A-missing と、私の M-missing) で全レンダーが declared=1 |
| L-4 | 解消 | 窓は左右それぞれ、最初に欠けたフレームで打ち切る (479-494)。プロファイルの後続フレームも、最初の欠けで打ち切る (459-464)。プロファイルのフレームそのものが欠けたら失敗する。端の扱いは参照 (`verify_ofx.reference` の `available()`) と同じ規則。照合の結果は表の下 |
| I-1 | 解消 (方針として妥当) | abort は status 1、メッセージなし、画像はすべて解放。その後のレンダーは中断なしの出力とビット一致 (後述) |
| I-2 | 解消 | 表示値 Mix から内部値への変換は `(100 − Mix)/100`。UI の rows.py `"inv"` と qtui の `rows.cpp:54` と同じ式。参照は `state.coerce("mix", 100 − Mix)`。動作の確認は表の下 |
| I-4 | 解消 | Auto の規則は 720 行以上が BT.709、それ未満が BT.601 で、`nv.matrix_from_tags(None, H)` と qtui の `video.cpp:218` と同じ。動作の確認は表の下 |

**L-2 の別スレッド試験**
- 使ったもの:
  - `verifier/round2/minihost_storm.cpp`: minihost の写しに、レンダー中に 2 ms 間隔で別スレッドから呼ぶ仕掛けを足したもの。モード 1 は GetFramesNeeded だけ、モード 2 は GetFramesNeeded と InstanceChanged(Rebuild Profile)。
  - プラグインは `-fsanitize=thread` で作り直し、libnvcore は計装していないバンドル内のものを使った。
- モード 1 では 1 レンダーあたり 60〜159 回、モード 2 では 158〜196 回の同時呼び出しがあった。どちらのモードでも次のとおり:
  - TSan の警告 0、デッドロックなし。
  - 5 レンダーとも status 0・declared=1。
  - 出力は A-default の照合済み出力とバイト単位で同一。
- 構築の途中で無効化されると保存されないことも確認した。モード 2 では、すべてのレンダーがプロファイルのフレームを取り直している。モード 1 では、2 回目以降のレンダーはキャッシュを使っている。
- 競合を手で追った結果: 設定の読み込みと `gen` の取得の間に InstanceChanged が入ると、古い設定で作ったプロファイルが新しい `gen` の下で保存される。ただしキーに設定が含まれるので、次のレンダーで不一致になって作り直される。Rebuild だけなら、押した後に取得したフレームで作っているので意味は正しい。

**L-4 の照合**
- 作業者の場面: A-missing (欠け 2・5、範囲 [1,6] への変更)。
- 私の場面: M-missing (test.mp4 n=700〜709、256×160、first 20、欠け 21・24・27、R=5)。含めた条件は次のとおり:
  - pf+1 が欠けて pf+2 があるケース。後続フレームなしとして扱われ、22 は取得されない。
  - 両側の穴。
  - 範囲を [22,28] に変える。
  - Profile Frame 3 → 0。
- すべて、ネイティブの参照と純 Python の参照の両方とビット一致した。

**I-2 の動作確認**
- Mix 100 (表示) は Mix 無効とバイト同一。
- Mix −20 (0 に切り詰め) は Mix 0 とバイト同一。
- Mix 0 の出力と入力の差は最大 3.0e-5 階調で、YCbCr の往復誤差だけだった。
- Mix 45.5 の出力は Mix 100 とも Mix 0 とも異なる。
- 以上すべてが参照とビット一致した。

**I-4 の動作確認**
- Auto で 1280×720 を処理した出力は、BT.709 を明示した出力とバイト同一。BT.601 とは異なる。
- Auto で 333×187 を処理した出力は、BT.601 を明示した出力とバイト同一。

## R2-2. 例外型の件

作業者の観測は再現した。結論として、この変更は妥当で害もない。ただし実体はファイルの読み込みではないので、記述を補うのがよい (R2-I-1)。

- `verifier/round2/dlmode.c` (dlopen の引数を表示する interpose) と `exc.cpp` で確認した。`std::runtime_error` を最初に破棄したときだけ、`dlopen("/usr/lib/libstdc++.6.dylib", mode=0x10 = RTLD_NOLOAD)` が呼ばれ、NULL が返る。
- 結果はプロセス内で保存されるので、2 回目の破棄では呼ばれない。独自の型 (std::string を持つ型) では一度も呼ばれない。
- RTLD_NOLOAD は「既に読み込まれていれば」その画像を返すだけで、ファイルを読み込まない。このマシンには `/usr/lib/libstdc++.6.dylib` 自体が存在しない。
- つまり、fsguard が記録していたのは libc++ の GCC 互換の問い合わせで、ファイルへのアクセスではない。独自の型にしたことで「記録はバンドルの dlopen 1 件だけ」という監視の基準が失敗の経路でも成り立つので、変更そのものは妥当で害もない。

## R2-3. 再実行の結果 (観測)

- **クリーンビルド** (`ofx/build_ofx.sh` を scratchpad の build2 へ):
  - exit 0、warning 0。
  - エクスポートは 3 つだけ、rpath は 1 本。
  - import は前回の一覧に `snprintf` が加わっただけ。
  - 署名の検証 OK、`--check` は CHECK OK、`--check-floor` は 0/51。
- **`verify_ofx.py --pure --mutate`**:
  - 7 場面・74 比較で 0/90,210,304 語の不一致、ALL MATCH。
  - 拒否 6 種はすべて ok。
  - mutation 1 は 5/5、mutation 2 は 1/5 (5 語) で検出。
  - fsguard は全場面で 1 件だけ。
- **`verifier/round2/run_extra2.py --pure`**: 新しい意味に合わせて改めた。
  - Color Matrix は 0 Auto / 1 709 / 2 601 / 3 2020。
  - Mix は表示値で、0・45.5・100・−20 を試す。
  - M-missing と N-720 (1280×720 で Auto = 709) を足した。
  - 結果は 9 場面・35 レンダー × 参照 2 種 = 70 比較で 0/28,922,848 語の不一致。拒否 6 種も ok。
- **minihost_v2** (現行の minihost に窓の縮小と abort の注入を足したもの):
  - 窓を縮めたとき、内側は全面のレンダーと一致し、外側は 0 のままだった。
  - abort を 1〜8 回目に返させた。1〜6 回目は 1 回目のレンダーに当たり、7・8 回目は 2 回目のレンダーに当たる。今回はレンダーの最初の abort 問い合わせが 6 回なので、こうなる。どの場合も:
    - 中断したレンダーは status 1・メッセージなしで、画像はすべて解放。
    - 中断していないレンダーは、中断なしの出力とビット一致。
    - 解放されない画像は 0。
- **fsguard**: 陽性対照は selftest の 5 件と、libnvcore の fopen の捕捉。
- **sandbox-exec** (`verifier/round2/final.sb`。前回と同じ規則で、パスを build2 に替えたもの):
  - A-params・A-missing・D-1080p が完走した。出力は fsguard の下で照合済みの出力とバイト単位で同一。
  - 失敗の経路 (プロファイルのフレームの欠け、インターレース) も kill されずに status 1 で返った。
  - 陽性対照 (`data/ref_floor.npy` の読み取り) は exit 137。
- **native・qtui**: 差分がないので再実行していない。前回の qtui_selftest 117/117 と、クリーンビルドの warning 0 がそのまま有効。

## R2-4. 新しい指摘 (すべて I)

- **R2-I-1**: 例外型の注釈を補ったほうがよい。`params.h:17` の注釈と REPORT の「libstdc++.6.dylib を dlopen していた」は事実だが、RTLD_NOLOAD の問い合わせでファイルは読み込まない、と添えると誤解が減る。
  - libnvcore 内には std 例外の throw が 1 か所残っている (`nv_temporal.cpp`)。その経路を通ると fsguard に同じ記録が 1 件出得るが、これもファイルへのアクセスではない。
- **R2-I-2**: GetFramesNeeded と Render の間に、ホストが InstanceChanged を呼ぶ場合がある。その場合、そのレンダーは直前の GetFramesNeeded で宣言していないプロファイルのフレームを取得する。
  - プラグイン側では防げない順序の問題で、TemporalClipAccess = 1 なので取得自体は許される。
  - storm 試験では、minihost の呼び出し順の都合で declared=1 のままだった。
- **R2-I-3**: 無効化が続く間は、毎回のレンダーがプロファイルを作り直して捨てる (reenter と storm モード 2 で観測)。設計どおりだが、Resolve で Rebuild を連打するとその間のレンダーが遅くなる。
- **R2-I-4**: REPORT の本文の「未対応の点」の行 (「パラメータの丸め: C++ は四捨五入…」、61 行) は、まだ直っていない。訂正は統括の追記側 (97 行) にだけある。
- Round 1 から持ち越すもの:
  - I-3 (丸めの差には到達しない)、I-5 (SOURCE.md に LICENSE 本文の出典を書く)、I-6 (work/ が gitignore で無視される)、I-7 (メモリ量)、I-9 (arm64 のみ・最低 macOS 26・ad hoc 署名) はそのまま。
  - I-8 のうち、「1 プロセスで複数インスタンスを同時にレンダーする」場合は依然として試していない。今回の storm 試験は、同じインスタンスへの同時呼び出しだけ。
