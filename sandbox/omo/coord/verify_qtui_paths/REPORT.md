# nv_ui_qt 経路テスト (スタンドアロン / OFX) 報告 — 2026-10-26

実装者のサブエージェントはファイルを書けないため、その最終応答を統括が転記した (要約)。数値はこのフォルダのログの実測。この時点では独立検証は未実施。

## 結果

テスト 1〜4 はすべて PASS (4.3 は修正前 FAIL → 修正後 PASS)。selftest は 55/55。

| 項目 | 結果 | 証跡 |
|---|---|---|
| 1.1 読み込み | PASS | 901 フレーム、640×360 (`test1_standalone.log`) |
| 1.2 Auto Profile | PASS | flat_status 2、area [288,416,48]、frame 300 |
| 1.3 AFT の自動実行 | PASS | aft_applied True、8.2 秒 |
| 1.4 スライダー | PASS | amt_Y 70→20 で gen 8→9 |
| 1.5 Apply | PASS | 表示が `engine.process(現在の設定)` と一致 |
| 1.6 Export | PASS | h264 5 フレーム。CRF 0 で `backend.process_frame` とビット一致 |
| 2 ブリッジ単体 | PASS | 8/16 ビットは 3 サイズで往復バイト一致。32 ビットは NaN・±inf のビットが一致し、有限値の差は最大 1.19e-7 (`test2_bridge.log`) |
| 3 パイプ | PASS | `frames: 5` (`test3_pipe.log`) |
| 4.1 stdin からの読み込み | PASS | 7 ケースとも 12 フレームを受信 |
| 4.2 Open / Export / Generic の無効化 | PASS | 4 つとも enabled False |
| 4.3 Apply で stdout へ | 修正前 FAIL → 修正後 PASS | 7 ケース・54 項目 (`test4_ofx.log`)。runpy 経由も 12/12 (`test4_ofx_runpy.log`) |
| 4.4 Auto Profile → AFT | PASS | AFT 4.8〜11.8 秒 |
| 深度をまたいだ照合 | PASS | 16/256 と 8 ビットの差、32×255 と 8 ビットの差はともに最大 0.5 (`cross_depth.log`) |

## 直した不具合

修正前のファイルは `coord/backup/*.20261026_pre_paths`、差分は `main.py.diff`・`ofx_bridge.py.diff`。

1. **OFX の Apply が stdout に書き出していなかった。** `main.py` の `_emit()` を、Apply と閉じるときの両方から呼ぶようにした。(フレーム, 設定, プロファイル) が直前と同じなら書かない。
2. **`ofx_bridge.py` の docstring が、存在しない `--ofx-serve` を説明していた。** 記述を削除し、プロトコルを書き直した。stdout は k 個の完全なフレームで、ホストは最後のフレームを使う。
3. **`--ofx` の引数を誤るとトレースバックで落ちていた。** usage を出して終了コード 1 にした。
4. **OFX モードの案内文が、無効にした Open を案内していた。** OFX 用の文に替えた。

## 未修正 (要判断)

- Cancel でも閉じるときの書き出しが起きる。
- OFX モードでは起動時に Auto Profile が走らない。
- Apply と閉じるときの処理を GUI スレッドで同期実行する (推定)。
- stdin の末尾の不完全なフレームは捨てる。
- MAX_FRAMES を超えた分の stdin は読まない (推定)。

## 独立検証 (verify2/、2026-10-26)

- 第 1 回: APPROVE (ブロッカー 0)。自前のドライバで次を確かめた。
  - Apply で 1 フレーム、変更のない Apply で 0、変更の後に閉じると 1。
  - 値は参照と一致 (8/16 ビットは差 0、32 ビットは差 2.97e-08)。
  - 重複を防ぐ署名は、AFT の完了・フレームの移動・tune の変更を取りこぼさない。
  - selftest は 55/55。
  - 指摘は次の 2 つ。
    - F1: W/H が 0 以下だとハングする (既存の不具合)。
    - F2: Cancel でも書き出す。
- 統括が F1・F2 を `main.py` で直した (控えは `coord/backup/main.py.20261026_pre_cancelfix`)。
  - W/H が 0 以下なら usage を出して終了コード 1 にする。
  - Cancel は書き出さずに閉じる (ウィンドウを閉じる操作は従来どおり)。
- 再検証: APPROVE。
  - 不正な引数 9 種はすべて usage と終了コード 1 になり、stdout は 0 B。
  - Cancel の 4 シナリオでは Cancel のときに書き出さない。
  - seq の 8/16/32 ビットの stdout は前回とバイト単位で同一。
  - selftest は 55/55。
- 画面ありの起動 (統括): `python3 -m nv_ui_qt.main ../test.mp4` を起動し、25 秒後に窓を撮影 (`visible_standalone.png`、Auto Profile 済み、エラー出力なし) してから終了した。
