# Windows ホスト側 Claude 用 手順書 (N1 コアの Windows ビルドと検証)

作成: 2026-10-28 (統括)。この手順書は **Windows (x86_64) マシン上で動く Claude** が
単独で実行できるように書かれている。リポジトリ: https://github.com/seitanmen/neatvideo-reverse

## 0. この作業の目的と「合格」の定義 (必ず最初に読む)

- 目的: N1 で C++ に移植した 9 部品を **Windows (x64) でビルドし、動作を確認する** (計画の W1)。
- **合格の基準は 2 種類あり、混同しないこと**:
  1. **ビルド成功** (Windows で 9 個のテストバイナリが作れること。警告は数えて記録する)。
  2. **クロスプラットフォーム決定論**: 同じ入力ブロブに対し、macOS の C++ 出力と
     Windows の C++ 出力がバイト一致すること。**これはビルドの後、統括 (macOS 側) が
     結果ファイルの SHA256 を突き合わせて判定する。** Windows 側は出力ファイルと
     ハッシュ一覧を作るまでを行う。
- **重要な期待値の調整**: リポジトリの `verify_*.py` は「C++ 出力 vs その場の Python」を比べる。
  Windows の numpy/scipy/libm は arm64 macOS と演算順や ULP が違うため、
  **N1b (pow)・N1d (pow)・N1e (atan2/sincos)・N1f (pow)・N1g/N1h (scipy の演算順)・N1i (pow) は
  Windows の Python との比較で不一致が出ることが仕様どおり** であり、失敗として扱わない。
  修正もしない (C++ の算法を変えて「Windows Python に合わせる」ことは絶対にしない)。
  不一致は「N0/N1a/N1c は全件一致した / 他は期待どおり不一致」のように分類して REPORT する。
  一致が**必須**なのは N0 (Quality)・N1a (帯域測度)・N1c (LZMA) の 3 本
  (libm も SIMD リダクションも使わないため)。
- 禁止事項: `repro/` の変更、C++ の算法変更、本家バイナリの解析、コミット・プッシュ
  (REPORT と成果物の置き場所は §6)。

## 1. 環境の準備

PowerShell (管理者) で:

```powershell
# ツールチェーン (Visual Studio 2022 Build Tools + C++ ワークロード + CMake)
winget install Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
winget install Kitware.CMake
winget install Git.Git
winget install Python.Python.3.13
```

- インストール後、新しいターミナルを開き直す。`cl` を使うには
  「x64 Native Tools Command Prompt for VS 2022」から作業するのが確実
  (通常の PowerShell からは `& "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"` を読み込む)。
- Python 依存 (リポジトリのルートで):

```powershell
git config --global core.autocrlf false   # 改行変換でバイナリ比較を壊さない
git clone https://github.com/seitanmen/neatvideo-reverse.git C:\work\neatvideo-reverse
cd C:\work\neatvideo-reverse
py -3.13 -m venv .venv
.venv\Scripts\activate
pip install numpy==2.1.3 scipy==1.15.3 PySide6==6.12.0 PyInstaller
```

- 注意: リポジトリのルート直下に `test_v02.mp4` / `test.mp4` が必要な検証がある
  (N1g/N1h など)。実写フレームを使うケースは §5 の「実写あり」に分けている。
  動画がない場合は LFS サイズの関係で git に無いので、**統括に要求する**
  (同じファイルを macOS 側から送る。勝手に別の動画で代用しない)。

## 2. ビルド (W1)

`native/Makefile` のターゲットと同じ構成を CMake で作る。`native/CMakeLists.txt` を新規作成し:

- コンパイラ: MSVC (cl)。`/O2 /fp:precise /EHsc /std:c++17 /utf-8`。
  **/fp:fast は禁止**。MSVC に `/fp:contract-` があれば付ける (VS2022 17.6+)。
- 定義: `NV_PORTABLE` を必ず付ける (Windows では AArch64 asm を使わない。
  `nv_portable.h` の明示的 NaN 選択ヘルパが正しい経路)。LZMA には `_7ZIP_ST`。
- ターゲット (ソースは Makefile と同じ。ヘッダは依存に含める):
  | 生成物 | ソース |
  |---|---|
  | nv_quality_test | main.cpp, nv_quality.cpp |
  | nv_band_test | nv_band_main.cpp, nv_band.cpp |
  | nv_curve_test | nv_curve_main.cpp, nv_curve.cpp (リンカオプション不要) |
  | nv_lzma_test | nv_lzma_main.cpp, nv_lzma.cpp, lzma/{LzmaLib,LzmaEnc,LzmaDec,LzFind,Alloc}.c |
  | nv_flat_test | nv_flat_main.cpp, nv_flat.cpp, nv_band.cpp, nv_curve.cpp |
  | nv_model_test | nv_model_main.cpp, nv_model.cpp, nv_band.cpp |
  | nv_v5_test | nv_v5_main.cpp, nv_ndimage.cpp, nv_spatial.cpp, nv_temporal.cpp |
  | libnvcore (dylib の代わりに DLL) | native_cbridge のソース (N2a のマージ後に追加) |

- 既知の Windows 固有の修正が必要な箇所 (ビルドエラーになったら直してよい。挙動は変えない):
  - `nv_model.cpp` の `__sincos_stret` は macOS 専用。`#if defined(__APPLE__)` で分岐し、
    それ以外は `sin()` + `cos()` を個別に呼ぶ実装にする **(macOS 側も同じ分岐に揃える必要が
    あるので、REPORT に「要 macOS 側修正」と明記すること。統括が macOS 側を修正して
    再照合する)**。
  - AArch64 インラインアセンブラは `#if defined(__aarch64__)` ガードがあるはず。
    ガードが無くて x64 でコンパイルエラーになる場合は、**算法を変えずに**
    `nv_portable.h` のヘルパへ置き換えた箇所を REPORT に列挙する。
  - `bits(0x...)` は memcpy 方式なので MSVC でそのまま動くはず。

## 3. 検証の実行

各テストバイナリを `verify_*.py` と同じ使い方で動かす (入力はスクリプトが numpy で
決定的に生成する。シード固定なので Windows でも同じ入力になるはず — numpy の
Generator ストリームは版が同じならプラットフォーム非依存)。

```powershell
# 順に実行し、出力を全部ログに残す
python native\verify_quality.py   >  native\win_quality.log  2>&1
python native\verify_band.py      >  native\win_band.log     2>&1
python native\verify_lzma.py      >  native\win_lzma.log     2>&1
python native\verify_flat.py      >  native\win_flat.log     2>&1
python native\verify_model.py     >  native\win_model.log    2>&1
python native\verify_curve.py     >  native\win_curve.log    2>&1
python native\verify_temporal.py  >  native\win_temporal.log 2>&1
python native\verify_spatial.py   >  native\win_spatial.log  2>&1
python native\verify_profile.py   >  native\win_profile.log  2>&1
python native\verify_aft.py       >  native\win_aft.log      2>&1
python native\verify_flat_cancel.py > native\win_cancel.log  2>&1
```

- **合格必須 (全件一致のはず)**: quality / band / lzma。
- **不一致が出ても仕様どおり**: curve / flat / model / temporal / spatial / profile / aft
  (Windows Python・Windows libm の差。分類して記録する)。
- もし quality / band / lzma のどれかが不一致になったら、それは**本当に直すべき不具合**。
  入力ブロブのうち不一致の番号と、C++ と Python の出力語を REPORT に載せる
  (入力生成の再現性が崩れている可能性もあるので、まず numpy の版を確認:
  `pip show numpy` → 2.1.3 であること)。

## 4. クロスプラットフォーム決定論のチェック (統括が判定する材料)

macOS 側の C++ 出力と Windows の C++ 出力を直接比べるため、結果ファイルの SHA256 を作る:

```powershell
# verify スクリプトが残した一時結果 (tmp_results.bin 等) と、各ドライバの出力をハッシュ化
Get-ChildItem native -Filter *.bin -Recurse | ForEach-Object {
  (Get-FileHash $_.FullName -Algorithm SHA256).Hash + "  " + $_.FullName
} | Out-File native\win_sha256.txt
```

- さらに確実なのは「同一入力ファイル → 結果ファイル」のペアを残すこと。
  verify スクリプトが入力を残さない場合は、スクリプトの入力生成関数を import して
  `native\win_inputs\` に保存し、その入力に対するドライバ出力を `native\win_results\` に
  保存する (quality は `blobs` 4000 個 = 約 13 MB なのでそのまま保存できる)。
- これらを REPORT と一緒に統括へ渡す (§6)。統括が macOS で同じ入力を流して判定する。

## 5. 実写フレームを使う検証について

- **動画はリポジトリに入っている** (`test_v02.mp4` 137 MB・`test.mp4` 77 MB、Git LFS)。
  **Git for Windows に git-lfs が入っている必要がある** (同梱が一般的。無ければ
  `winget install GitHub.GitLFS` → `git lfs install` → `git lfs pull` で実体を取得。
  LFS が無いとポインタファイル (数百バイトのテキスト) だけが降りてくるので、
  最初に `ls test_v02.mp4` のサイズで確認すること)。
- これらを使うケースは ffmpeg で読む。Windows でも **ffmpeg を入れると実行できる**
  (`winget install Gyan.FFmpeg`)。動画は 2 本ともコミット済みなので、
  動画を使うケースも通常どおり実施すること (スキップ不要)。

## 6. REPORT と成果物の納め方

- `sandbox/omo/wp_native_windows/REPORT.md` に:
  - 環境 (OS 版・VS 版・CMake 版・Python/numpy/scipy の版)
  - ビルドの修正箇所 (MSVC 対応で変えた行。挙動を変えた箇所があれば理由)
  - 各スイートの結果 (一致/期待どおり不一致 + 不一致ケースの分類)
  - 出力ハッシュ一覧 (win_sha256.txt) と入力ファイルの置き場所
  - 要 macOS 側修正 (`__sincos_stret` の分岐など)
- コミット・プッシュはしない。REPORT と `native/CMakeLists.txt`・MSVC 対応の差分だけを
  統括に渡す (パッチか、作業ブランチ `windows-verify` への push のどちらでもよい。
  迷ったら REPORT に貼るのが確実)。

## 7. この後 (別タスク)

- 統括が macOS↔Windows の出力比較と、`__sincos_stret` 揃えの再照合を行う。
- W4: PyInstaller での .exe 梱包 (この手順書の環境がそのまま使える)。
- W2 の libm 対策 (ベンダー libm) は語単位 diff の結果を見てから開始。

## トラブルシュート (よくあるもの)

| 症状 | 対処 |
|---|---|
| `cl` が見つからない | x64 Native Tools Command Prompt から実行する |
| numpy 2.1.3 が入らない | Python 3.13 の 64bit か確認 (`py -3.13 -c "import struct;print(struct.calcsize('P')*8)"` → 64) |
| CRLF でビルドが壊れる | `git config core.autocrlf false` して再チェックアウト |
| 検証が極端に遅い | N1g/N1h・profile は重い (macOS でも数分〜)。タイムアウトを長くする |
| `__sincos_stret` でエラー | §2 の分岐を入れる (macOS 側の追従は統括がやる) |
| verify の不一致 | §3 の分類に従う。quality/band/lzma の不一致だけが本当のバグ |
