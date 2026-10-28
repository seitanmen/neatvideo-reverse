# W2: libm 差異の確定と対策 (委託タスク)

作成: 2026-10-29 (統括)。背景: C++ 移植 (N1) の libm 依存は 4 種 — `pow` (nv_curve・nv_aft)、
`tanf` (nv_linekernel)、`atan2` + sincos (nv_model)。いずれも volatile 関数ポインタ経由
(`g_pow`・`g_tanf`・`g_atan2`・sincos) で、入替えは容易。Windows の UCRT と macOS の
libSystem は ULP レベルで異なるため、これらを使う出力語がプラットフォーム間で変わり得る。

## 方針 (統括決定済み)

まず **W2a の計測**で影響範囲を確定し、その結果を見て W2b (ベンダー libm) を行うか
W2c (文書化) にするかを統括が判定する。根拠: 本家もプラットフォームごとに別バイナリで
プラットフォーム間の完全一致は本家自身が保証していないため、**差の範囲が小さければ
「本家と同じ性質」として文書化が妥当**。

## W2a: libm 感度ハーネス (macOS のみで完結)

1. `native/libm_alt.cpp` (+ ヘッダ): 代替実装を 3 種用意して切り替える
   (`NV_LIBM_ALT=1|2|3` 環境変数またはビルド定義):
   - alt1: openlibm (MIT。`native/openlibm/` にソースを同梱、出典と SHA256 を記録) の
     pow / tanf / atan2 / sin / cos
   - alt2: musl の同関数 (同様に同梱)
   - alt3: x87/long double 経由の素朴実装 (誤差の上限を見るため)
2. 既存の 11 本の verify + verify_bridge を **alt ビルドで**再実行し、
   Python (macOS libm) との差分を語単位で記録する:
   - 差分 0 の部品 / 差分のある部品と語のオフセット・割合
   - Quality / Completeness の % 表示が変わるケースの有無
3. **成果物**: `native/verify_libm_alt.py`、語単位 diff の表 (REPORT)。
   実行: `make -C native libm-alt && python3 native/verify_libm_alt.py`。

## W2b (W2a の差が大きい場合): ベンダー libm への統一

- alt1 (openlibm) を既定にし、全プラットフォームで同一値にする。
- その場合の検証基準点の移行: 「Python (macOS libm) との一致」→
  「**macOS の C++ 出力 (システム libm ビルド)** との一致」を gold にする
  (F4 の手法。入力ブロブと gold 出力を fixtures として固定する)。
  Python との差分 (libm 語) は QUALITY/MIGRATION に文書化する。
- x86_64 (Rosetta) でも同一になることを F4 の手順で再確認。

## W2c (差が小さい場合): 文書化

- 差が出る語のオフセット一覧と「macOS libm 基準」であることを MIGRATION.md に記す。
- Windows 側 (runbook の Claude) には、差が出る部品の Windows 結果を
  「期待どおり不一致」として分類するよう依頼済み (native_windows_runbook.md §3)。

## 共通規約

- N1 の規約を引き継ぐ (ビット一致思考・fast-math 禁止・コミットしない・
  REPORT は `sandbox/omo/wp_native_libm/REPORT.md`)。
- openlibm / musl を同梱する場合はライセンス表示と出典 URL・SHA256 を
  `native/openlibm/SOURCE.md` に記す。
- スナップショット・AGENTS.md・MIGRATION.md の更新は統括が行う。

## 貼り付け用プロンプト

```
.claude/tasks/native_w2_libm.md の「W2a」を実行してください
(openlibm / musl の同梱と、alt ビルドでの 11 スイート + verify_bridge の語単位 diff)。
完了したら sandbox/omo/wp_native_libm/REPORT.md に差分の表を書いてください。
W2b か W2c の判定は統括が行うので、判定材料だけを揃えてください。
```
