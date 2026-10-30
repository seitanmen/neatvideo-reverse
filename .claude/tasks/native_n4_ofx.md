# N4: macOS 用 OFX プラグイン (MeatVideo.ofx)

作成: 2026-10-31 (統括)。目標: **DaVinci Resolve が読み込む本家と同じ形の
Mac 用 OFX プラグイン**。素材はファイルからではなく、**ホスト (Resolve) から
渡される画像をそのまま処理する** (ファイル読み込みは行わない)。

## 既知の情報 (解析済み)

- 本家の OFX エクスポートは 3 関数のみ: `OfxGetPlugin` / `OfxGetNumberOfPlugins` (=1) /
  `FoundryOfxAllowPluginUnload` (Agents.md §3)
- 本家バンドル構成: `NeatVideo6.ofx.bundle/Contents/{MacOS/NeatVideo6.ofx, Info.plist, ...}`
  (実物が `reference/genuine/NeatVideo6.ofx.bundle/` にある。**構成の雛形として使う**)
- 本家バイナリのシグネチャ: `com.absoft.NeatVideo6-OFX` (我々は
  `com.meatvideo.MeatVideo-OFX` にする)
- OFX API: バージョン 1.x の Image Effect (SupportsTiles, FieldProcessing なし=
  まずプログレッシブのみ)。docs/INTERLACE_PROGRESSIVE.md のとおり、
  インターレースは「非対応」とホストに正直に報告する段階から
- 処理: libnvcore (libnvcore.dylib の C ABI / NVC_ABI_VERSION = 3) を直接リンク

## N4a: 最小 OFX プラグイン (まず読み込ませる)

1. `ofx/` ディレクトリに CMake ターゲット `MeatVideo_ofx` (dylib) を追加:
   - エクスポート: `OfxGetNumberOfPlugins`, `OfxGetPlugin`, `FoundryOfxAllowPluginUnload`
     (シンボル名は C リンケージ。本家の 3 関数と同じ名前)
   - OfxPlugin 構造体: API version 1, pluginApi "OfxImageEffect", 独自 pluginId
     (`com.meatvideo.denoise`)
2. バンドル化スクリプト `ofx/build_ofx.sh`:
   `MeatVideo.ofx.bundle/Contents/{MacOS/MeatVideo.ofx, Info.plist, Resources/}` を作り、
   libnvcore.dylib を Contents/Frameworks/ に同梱 (rpath で参照)
3. **読み込みテスト** (ホストなしでもできる):
   - Resolve に無い環境での最初の検査: `OfxGetPlugin` を dlopen して呼ぶ
     ミニホスト (`ofx/tools/minihost.cpp`) を作り、構造体の妥当性を確認
   - ユーザーが Resolve の OFX フォルダ
     (`/Library/OFX/Plugins/MeatVideo.ofx.bundle`) にコピー → Resolve 起動 →
     Effect リストに表示されるかを確認 (ユーザー操作)
4. 合格: Resolve がプラグインを認識し、クラッシュしない

## N4b: レンダーの実装 (ホストから渡された画像の処理)

1. OFX アクションの実装:
   - `OfxImageEffectActionDescribe` (クリップ Source/Output、プロパティ)
   - `OfxImageEffectActionCreateInstance` / `DestroyInstance`
     (インスタンスごとに libnvcore の状態を保持)
   - `OfxImageEffectActionRender`: ソース画像のプロパティ
     (`OfxImageEffectPropSourceClipData` / row pointers) から画素を取得し、
     libnvcore の処理 ( temporal + spatial、パラメータは OFX パラメータから) を実行、
     出力画像へ書き込む
   - `OfxImageEffectActionGetRegionOfDefinition` / `GetFramesNeeded`
     (Temporal Radius に応じたフレーム要求)
2. パラメータ: 第 1 段は最小構成
   (Temporal Radius, Temporal Threshold, Luma/Cb/Cr Noise Level,
   Spatial Amount 系 = Python 版 UI の既定値と同じ既定)
   パラメータ → `v5_params` への写像を `ofx/params.cpp` に集約
3. プロファイル: **OFX ではファイルから作れない** ので、
   (a) Load Profile アクション/パラメータで JSON ファイルを読む
   (b) インスタンスに profile blob (0xc300 相当 or v5 dict 相当) を渡す
   仕様は N2a の `nvc_profile_build_cancel` / G8 の `nv_prof5` C ABI を使う
   (UI と同じ計算経路 → ビット一致が保証される)
4. **素材はホストから**: すべての画素はホストが渡す画像バッファから取得する。
   ファイル・stdin パイプ・ffmpeg は一切使わない
   (デノイザー本体に ffmpeg は不要、という OFX の利点をそのまま実現)
5. 検証: ミニホストを拡張して Render アクションを呼び、
   同じ入力フレーム列に対する出力が
   (i) N2a のブリッジ経路 (backend.process_frame / process_frame_v5) と
   ビット一致することを確認 (レンダー 1 フレーム = temporal+spatial 1 回分)
   (ii) ユーザーが Resolve で実際に処理をかけて目視確認

## N4c: 実用化

- パラメータの追加 (イコライザ = R/G/B 9 ビン、Noise Level Check 相当の
  表示補助は OFX では別 persist やダイアログになるので後回しでよい)
- プロファイルの UDP/エクスポート (UI で作った JSON プロファイルを
  Resolve のインスタンスに渡す仕組み: Load Profile パラメータのパス指定)
- メタデータ: プラグイン名 "Meat Video Denoise"、バージョン、About (Developer: kawata)
- Windows 版 (.dll) は W5

## 規約・注意

- libnvcore の C ABI の変更は NVC_ABI_VERSION を上げ、nv_native.py (参照実装) の
  版数も揃える (G1/G8 と同じ手順)
- 合格判定はビット一致 (ミニホスト vs ブリッジ経路)
- OFX ヘッダーは公開の ofxh (OpenFX 標準ヘッダ) を使う
  (`ofx/vendor/` に同梱、出典を記録)
- Resolve での実機確認はユーザーが行う (REPORT に手順を書く)
- **コミットしない**。REPORT: `sandbox/omo/wp_native_ofx/REPORT.md`
  (sandbox は既に work/ に移動済み → `work/omo/wp_native_ofx/REPORT.md`)
- スナップショット・MIGRATION.md・AGENTS.md は統括が更新

## 貼り付け用プロンプト

```
.claude/tasks/native_n4_ofx.md の「N4a」「N4b」を実行してください。
本家と同じ 3 関数エクスポートの .ofx バンドルを作り、
ミニホストで Render アクションのビット一致 (libnvcore 経路 vs ブリッジ経路) を
確認してください。素材はホストから渡された画像のみを使い、
ファイル読み込み・ffmpeg は一切使わないこと。完了したら
work/omo/wp_native_ofx/REPORT.md に書いてください。コミットはしないでください。
```
