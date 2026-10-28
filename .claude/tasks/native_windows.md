# Windows 対応計画 (W1〜W5)

作成: 2026-10-28 (統括)。N1 (コア移植) と N2 (macOS .app 梱包) を受けた、Windows (x86_64) 対応の
計画と委託タスク。N2 のセッションと並行に着手してよいのは W1 (CMake 化) だけ。
W2 以降は N2 のマージ後に行う (ファイルが重複するため)。

## 前提と目標の整理 (重要)

- **ビット一致の基準点**: これまでの「Python とビット一致」の基準点は
  **arm64 macOS の numpy 2.1.3 / scipy 1.15.3 / Python 3.13** という環境そのもの。
  Windows の numpy/scipy は SIMD カーネルとリダクションの順が違うため、
  「Windows の numpy とビット一致」は**目標にしない** (达成できないし、意味もない)。
- **Windows での目標** は次の 2 層:
  1. **C++ のクロスプラットフォーム決定論**: 同じ入力ブロブに対し、macOS の C++ 出力と
     Windows の C++ 出力がバイト一致する (libm 依存部は W2 の対策対象)。
     これは既に F4 で arm64↔x86_64 (Rosetta) で証明した手法の延長。
  2. **実用上の同一挙動**: libm の ULP 差を解消するか、差が出る語を文書化する。
- 本家もプラットフォームごとに別バイナリであり、プラットフォーム間で完全同一の値を
  出す保証はない (libm は OS のもの)。再現側の基準は「repro の Python 実装 (arm64)」で不変。

## タスク

### W1: ビルド基盤の CMake 化 (N2 と並行可)

- 現状の Makefile 9 本は macOS/clang 前提。Windows (MSVC / clang-cl / MinGW) で使えない。
- `native/CMakeLists.txt` を追加し、N1 の全ターゲット (9 テスト + libnvcore) を等価にビルドする。
  コンパイラは **clang-cl または MSVC (/fp:precise、/fp:contract-)**。gcc/MinGW は Second。
  - MSVC は `std::isnan`・`<math.h>`・`__builtin_*` の扱いが違うので、
    `nv_portable.h` に薄い互換マクロを追加する程度に抑える。
  - インラインアセンブラ (AArch64) は Windows では出番なし (x64 のみ)。`-DNV_PORTABLE` 相当、
    つまり **W2 のポータブル経路が既定**になる。
  - 検証: GitHub Actions で `windows-latest` + `macos-14` の両方をビルド (W3 と合わせて CI 化)。
- **成果物**: `native/CMakeLists.txt`、CI ワークフロー (`.github/workflows/native.yml`)、REPORT。
- **成果物の動作確認はユーザーの Windows マシンか Actions ランナーで行う** (この Mac では不可)。

### W2: libm 差異の確定と対策

- libm 依存は 4 種: `pow` (nv_curve・nv_aft)、`tanf` (nv_linekernel)、`atan2` + `__sincos_stret`
  (nv_model)。いずれも volatile ポインタ経由で呼ぶ構造済み (入替え容易)。
- Windows の UCRT / MSVC libm は macOS libSystem と ULP レベルで異なる →
  **方向ビン・曲線の末端の語が低ビットで変わり得る**。
- 対策は 2 択 (統括の推奨は (b)):
  - (a) **許容と文書化**: 差が出る出力語の一覧を作り (macOS と Windows で同一入力を流して
    語単位で diff)、REPORT に載せる。Quality/Completeness の % 表示への影響はほぼない
    (flatness・penalty は pow を使わない。AFT の pow は曲線係数の末尾ビットに効く)。
  - (b) **ベンダー libm**: openlibm (MIT) か musl の数学関数だけを `native/libm/` に同梱し、
    g_pow / g_tanf / g_atan2 / g_sincos をそれに差し替える。全プラットフォームで同じ値に
    なる (ただし「repro Python の arm64 libm との一致」は失われる — 検証の基準点を
    「macOS の C++ 出力」に移す必要がある。F4 の手法がそのまま使える)。
- **成果物**: 語単位 diff の結果 + 採用した対策 + (b) なら `native/libm/` と出典記録。

### W3: Windows クロスプラットフォーム検証

- `native/verify_windows.sh` (macOS 側) と `native/verify_windows.ps1` (Windows 側):
  同じ入力ブロブ列 (既存の verify スクリプトが生成するものをエクスポート) を
  macOS と Windows 両方の C++ で処理し、**出力同士をバイト比較** (Python を介さない)。
- 対象: N1a/b/d/e に加え、W2 の対策後の N1c/f/g/h/i も含める (F4 の x86 検証で未だった部分)。
- GitHub Actions (`W1` のワークフロー) で自動化し、artifacts として比較結果を残す。
- **合格基準**: libm 依存部を除く全語がバイト一致。W2-(b) 採用時は全語一致。

### W4: Windows 用 .exe 梱包

- PyInstaller の `onedir` (起動が速く AV 誤検知が少ない) で `NeatVideoRepro/` フォルダ + exe。
  spec は macOS 版を流用し、Windows 固有のフック (PySide6・numpy・scipy の隠し import) を追加。
- ffmpeg: macOS と同じ優先順で探索 (`ffmpeg.exe`、PATH、`%ProgramFiles%` 等) + Preferences 上書き。
  winget での導入手順を README に書く。
- `data/ref_floor.npy`・`native/libnvcore.dll` を同梱。`multiprocessing.freeze_support()` は必須
  (Windows は spawn のみ)。
- **検証**: Windows マシン (または Actions) で起動・動画オープン・Auto Profile・書き出し。
  .app と同じ「バンドル内自己試験 + 書き出しバイト一致」の基準。
- **成果物**: `packaging/nvrepro_win.spec`、`packaging/build_app.ps1`、README 追記、REPORT。

### W5 (将来 / N4 の一部): Windows OFX

- OFX は Windows では DLL (`OfxGetPlugin` を export)。N3 (Qt C++ UI) 完成後。
- 本家の Windows 版バイナリ (入手できれば) との比較は別途。

## 実行順序と依存

```
W1 (CMake + CI)   ── N2 と並行可
N2 マージ後:
W2 (libm 調査・対策) ─→ W3 (Windows 検証) ─→ W4 (.exe 梱包)
W5 は N3 の後。
```

## ユーザーに決めてほしいこと (統括の推奨つき)

1. **W2 の対策**: (a) 許容と文書化 (推奨。まず語単位 diff を取って影響範囲を見てから決めてよい)
   / (b) ベンダー libm (全プラットフォーム同一値。検証基準点の移行が必要)。
2. **Windows テスト環境**: GitHub Actions を使うか (推奨。手元の Windows がなくても可) /
   手元の Windows マシンで実行するか。
3. **配布形態**: .exe は自己責任の zip 配布か、インストーラ (Inno Setup など) にするか
   (推奨: まず zip。需要が出たらインストーラ)。

## 貼り付け用プロンプト (N2 の完了後)

```
.claude/tasks/native_windows.md を読んでください。ユーザーの決定は次のとおりです:
(ここに 1〜3 の決定を書く) 。この前提で「W1」を実行してください (N2 と並行でよい場合のみ)。
```

(N2 マージ後: 「W2」「W3」「W4」を順に実行。)
