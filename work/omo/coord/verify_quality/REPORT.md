# Quality の式と Assist Image の検証 (2026-10-26)

指示書: `.claude/tasks/phase2_quality_and_assist.md`。本家・RS バイナリ・nvh は実行していない。処理コア (nvfilter.py・nv_aft_v5.py・nv_depth.py) と repro/ のファイルは変更していない。
観測 = 逆アセンブル・測定で実際に確かめたこと、推定 = それ以外。

## タスク A: Quality の計算式 — 一部特定 (23% は未再現)

詳細は `static/QUALITY.md` (解析者の記録)、草稿 `static/quality_draft.py`、計算結果 `static/quality_draft_results.json`。独立検証 `verify/VERDICT.md` は **APPROVE** (g を除いた式。反証 0 件)。

### 参照の連鎖 (観測、RS のアドレス)
- Profile Viewer: setupUi 0x1131e0 で "profileQualityLabel" を Ui+0x170 に。"Overall Profile Quality" (0x11a540) はその親のグループボックスの題名とみられる (推定)。更新関数 0x117b98 が Quality 関数 0xa4acf8 を呼び、`qRound(q·100)` に "%" を付けて表示 (クランプなし)。
- Device Noise Profile パネル: simpleQualityLabel / advancedQualityLabel は同じクラスで、更新関数 0xdb1b0 が同じ 0xa4acf8 を呼ぶ。0xa4acf8 を呼ぶのはこの 2 か所だけ。→ Simple・Advanced・Profile Viewer は同じ式。
- 0xa4acf8 は Demo の 0x9bb3e0 に当たる (定数 3.625 が各バイナリで 1 か所にしかなく、前後の命令列も同じ)。

### 読めた式 (観測。p = プロファイル 0xc300 バイト)
```
p[0xbf34] (u8) == 0 → 0
s11 = 0.5·g(p+0xbf40) + 0.5                 # g は未読 (下記)
面 i = 3..5 (色モード 1 で Y・Cr・Cb): f_i = 平坦度 (repro の nv_flat.flatness と同じ関数)
  u_i = clamp(2.5 − f_i, 0, 1)、色差は u_i = 0.5·u_i + 0.5
P = ((u0+.5)(u1+.5)(u2+.5) + 0.25) / 3.625    # 最大 1
S = min(W,H) (p+0xc2c0/0xc2c4) の区間表 16/32/64/128/256 で 0〜5 の区分線形 (W か H が 0 なら int p[0xbf38])
q = s11 · P · S/5、R/G/B のクリップの印 (p[0xbf3c..0xbf3e]) 1 つごとに ×0.5
```

### 照合 (観測)
- test_v02 では、どの入力 (1080p n=312 の本家の領域 128²、移植の FindFlatArea、2 倍に拡大した近似入力) でも平坦度が 1.0〜1.24 です。そのため P = 1 になり、クリップもありません。
- g を除いた部分 R は、128² で 0.8、192² で 0.9 です。
- 本家の 23% にするには、128² なら g = −0.425 が要ります。g が [0, 1] なら Quality は 40〜80% (128²) になり、23% にはなりません。

### 独立検証 (verify/VERDICT.md、観測)
- 呼び出し元を __text の B/BL 全件で走査した。RS は 0xdb25c・0x1186bc、Demo は 0xdc354・0x1196bc の 2 件ずつだけだった。
- 禁止範囲の外の命令列は、RS 0xa4acf8 と Demo 0x9bb3e0 でオペランドまで同じ。3.625 の fmov は各バイナリに 1 件 (RS 0xa4aeb0、Demo 0x9bb5a0)。
- 表 (u8 の luma {1,1,1,1,0,0,1,0,0,0,0,0}、int64 のサイズ {16,32,64,128,256}) はファイルから直接読んで一致した。
- 罰の関数 (15 値)、サイズ項 (1,200 組)、R = 0.8 を独立に書いた式で再計算し、草稿とビット一致した。
- 軽微な誤記として、コンストラクタは 0x15c04 ではなく 0x15b50 だった。
- 残り:
  - [obj+0xd6a0] がパネルのラベルであることは未確認。
  - Demo のサイズの表 0x1488128 は ROOT スロットの領域 (0x1488xxx) にある。読みは普通の adrp+ldr で、静的な書き込みは見つからなかったが、実行時に書き換わらないとは言えない。
  - Demo 0x9d49d4〜0x9d49e0 のダミー読みは記録だけにして、解釈していない。

### 本家の追加の観測 (2026-10-26、Resolve、`shots/g_prepare_fullframe_q65.png`)
- 本家の状態がその後変わり、タイムラインは 1920×1080 になっていた。プロファイルは "Profiled with: Full frame (1920x1080)"。表示は Quality 65%、イコライザの完了度 85%、Main Sample 6.5。
- 全面のプロファイルでは min(W,H) ≥ 256 なので S = 5 になり、q = s11·P となる。全面の平坦度は大きく P < 1 とみられるので (推定)、g は 1 点では決まらない。

### 特定できなかった理由
- 輝度曲線の項 g (RS 0xa4b03c / Demo 0x9bb748) は、入口が難読化のブロックなので読んでいない (方針どおり停止)。
- 0xa4acf8 の中にも難読化のブロックがある (RS 0xa4ad28〜0xa4ad84・0xa4ae4c〜0xa4ae90・0xa4aed0〜0xa4aed4・0xa4af9c〜0xa4afec)。解析者はブロックの中身を解釈しておらず、数値の計算がブロックの出力を読まないことをレジスタの流れで確かめただけである。この関数の数値の部分は、停止を判断する前に一度表示した範囲から読んでいる (解析者の申告)。
- 指示書の Demo の禁止範囲には入っていない。
- RS の平坦度 0xa402f4 と罰 0xa63938 がバイトでは Demo と違う点は、中身を読んでいない (同じ式に難読化を挿入したものとみるのは推定)。
- → 式が完全ではないので、`repro/nv_ui_qt/quality.py` は作っていない (指示書の「特定できない場合は報告」)。
- g が分かれば、草稿を `quality.py` にできる。g が AFT 後の曲線で変わる項だという見方は推定である (根拠は別の観測の 43%)。

## タスク B: Assist Image の検証

測定 `assist/measure.py` → `assist/measure.json`。test_v02 の n=312、auto プロファイル、AFT なし。Noise Level Check の操作の再現は `assist/nlc_probe.py` (offscreen、test.mp4)。

### こちらの Profile Check (`main_window.assist_params` → `PreviewEngine.assist`) — 正常 (観測)
- 出力は `nv.spatial_v5` (残留分散 1、Luminance/Cr/Cb と全帯域 100%) とビット一致し、2 回の実行でも同じだった。
- 通常の処理との差は平均絶対誤差 1.65、PSNR 40.9 dB。
- 空の 128² で取り除く量 (輝度の標準偏差) は、Assist 4.12、通常 3.93 (Assist のほうが強い)。
- 自己試験 70/70 (`st/st_now.log`、現在のコード)。

### `nv_aft_v5.assist_params` と `main_window.noise_level_check` (別エージェントの追加分) — 不具合 (観測)
1. `noise_level_check` は UI の値 (`state.params`) を渡すので、`KeyError: 'band_amounts'` になる。Begin を押しても例外が出てプレビューは変わらない (`nlc_probe.py` で再現)。自己試験には Noise Level Check の項目がないので、70/70 でも検出されない。
2. バックエンドの値を渡しても、`profile_check` は Luminance 70% のまま、時間段も通す。そのため取り除く量は空で 2.88 となり、通常の 3.93 より弱い。本家の「わざと過剰に処理する」と逆である。
3. `noise_level_check` を指定した結果は、通常の処理と PSNR 71.0 dB で、ほぼ同じ画像 (Ultra Low が 0→100% になるだけ)。
4. 帯域の上書きは `ui_channels` を変えないので、成分ごとに設定が違う (uniform でない) と無視される (コードの読み。今回の既定値は uniform)。

### 本家との違い (観測: バイナリの文字列と資料の記録)
- 本家の Noise Level Check は時間フィルタのマスクを出す。文字列は "Click Begin to show the Temporal Filter mask."、"The mask shows the areas where temporal noise reduction is applied."。
- 資料 (`verify_qtui_layout/LAYOUT_WEB.md`) には「Check 系は青いマスクを表示する」とある。
- 今の実装 (空間だけ・全帯域 100% の処理結果) はこれと違う。

### Resolve での比較 (2026-10-26、ユーザーの許可のあとで実施。`shots/`)
- Noise Level Check の Begin (`g_nlc1.png`):
  - 主画像が "Assist Image" になり、上中央に黄色の "⚙ Noise Level Check" が出る。
  - ツールバーは "Noise Level Check ▾" が押された状態になる。パネルは "■ Finish" と、関係する行が黄色、Spatial / General のタブは無効。
- 色の当てはめ (I = (R+G+B)/3): 出力 = (1 − m)·I·(1, 0.9, 0.75) + m·(I·(0.3, 0.45, 0.5) + (0, 0, 126.5))。m は画素ごとのマスク。残差の平均は 0.35〜1.33 階調。
- Temporal の Noise Level ごとの m の平均 (静止した test_v02):
  - +0%: 0.996
  - −60%: 0.33 (画素ごとの斑点)
  - −100%: 0.003 (セピアだけ)
- 変えた Noise Level は本家の Undo で戻した (+0%、プリセット名も "program's default settings" に戻り、Undo は押せない状態)。そのあと Finish を押し、Prepare ページに戻した。Apply・Cancel・保存は押していない。
- Profile Check は前回の観測 (`genuine_ui2/`) どおりで、今回は撮り直していない。
- 修正の仕様は `nlc/SPEC.md`。実装と結果は `nlc/REPORT.md` と下の「修正」。

## 修正 (Noise Level Check、UI 側のみ)
- 仕様は `nlc/SPEC.md`、実装の報告は `nlc/REPORT.md`、独立検証は `nlc_verify/VERDICT.md` で **APPROVE**。
- 変えたファイルは `repro/nv_ui_qt/` の state・backend・display・main_window・menus・widgets/filter_panel・widgets/viewer・selftest。控えは `backup/*.20261026_pre_nlc`。
- 内容:
  - Temporal の Noise Level (`t_noise`) を実装した。時間段だけ noise_level を ×t_noise にする。+0% では変更前とビット一致し、書き出しも一致する。
  - Noise Level Check を「時間フィルタのマスク」の Assist Image にした (色は上の当てはめの式、m は repro の定義)。
  - 3 つの入口を同期させ、Filter Tuning Assist とは排他にした。パネルは関係する行を黄色にし、ほかを無効にする。
  - 旧実装 (`nv_aft_v5.assist_params` を呼ぶ経路) は除いた。nv_aft_v5.py は変更していない。
- 検証者の測定:
  - 自己試験 77/77。8 設定 × 3 フレーム × 2 本で変更前とビット一致。状態の管理 19/19。OFX の往復も正常。
  - 処理コアのスナップショット 181/181 (AGENTS.md の行だけを更新後に再計算。控えは `backup/readonly_snapshot_before.sha256.20261026c`)。
- 残る差:
  - −60% の平均 m は repro 0.52、本家 0.33 (フレームも違う)。
  - 無効にした見出しの文字が白いまま (L2)。
  - 設定を変えた直後に古いマスクが約 0.1 秒 + 計算時間残る (L1)。
- AGENTS.md の nv_ui_qt の段落を更新した (控え `backup/AGENTS.md.20261026_pre_nlc`)。

## 次の手順 (提案)
1. 本家のマスクの式 (時間フィルタの重みからマスクへの写像) を、時間フィルタの静的な解読の続きとして読む。この部分が保護区画の外かどうかを先に確かめる。
2. L2 の見出しの色を直す (`widgets/rows.py` の Header、小さな変更)。
3. Quality の g は保護区画の中なので、読み進めるかどうかはユーザーの判断。

## Manual Fine-Tune 後の Quality (2026-10-26 夜、指示書 `.claude/tasks/phase2_manual_ft_quality.md`)
条件 (観測): Resolve 上の本家 6.1.3 Pro、Prepare Noise Profile、"Profiled with: Full frame (1920x1080)"、Frame 1920×1080、AFT 済みの状態から開始。
操作はユーザーが直接行った (統括は撮影・読み取りのみ)。押したのは Profile メニューの Copy Profile・Reset Fine-Tuning Results・Undo Last Change と、赤カーブの点のドラッグだけ。Apply・Cancel・保存は押していない。撮影は `shots/manual_ft_*.png` (3600×2250)。

### 手順 1: Phase 1 の Undo の確認 (観測)
- 開始時点で、Phase 1 の赤のドラッグはすでに取り消されていた (Profile メニューの Redo Last Change が有効)。統括は Undo を押していない。
- Quality 65%、完成度 85%、赤カーブはドラッグ前の形 (`g_prepare_fullframe_q65.png` の赤と同じ)。`manual_ft_red50_undo.png`。

### 手順 2: Reset と手動の編集 (観測)
復旧用に、先に Copy Profile で AFT 済みのプロファイルを控えた (Paste Profile が有効になったのを確認)。結果として Paste は使っていない。

| 段階 | 撮影 | Quality | 完成度 | 赤カーブ |
|---|---|---|---|---|
| Reset Fine-Tuning Results の直後 | `manual_ft_p2_0_reset.png` | **35%** | 0% | 9 点とも +0% (黄色の点) |
| ビン 4 を約 +50% へ | `manual_ft_p2_1.png` | 36% | 1% | ビン 4 だけ上がる |
| + ビン 5 | `manual_ft_p2_2.png` | 36% | 2% | |
| + ビン 3 | `manual_ft_p2_3.png` | 37% | 4% | |
| + ビン 6 | `manual_ft_p2_4.png` | 37% | 5% | ビン 3〜6 が約 +50% |
| Undo 1〜4 回 | `manual_ft_p2_undo1〜4.png` | 37 → 36 → 36 → 35% | 4 → 2 → 1 → 0% | 編集の逆順に戻る。4 回目で平坦 |
| Undo 5 回目 (Reset の取り消し) | `manual_ft_p2_undo5_reset.png` | **65%** | 85% | AFT 済みの形 |

- Reset Fine-Tuning Results は Undo Last Change で取り消せた。AFT の再実行は不要だった。
- 最後の画面と開始時の画面 (`manual_ft_red50_undo.png`) を画素で比べた。差があったのは右上の macOS の通知バナーとその影 (y 0〜302、x 2809〜3599。独立検証者の測定) だけで、プラグインの画面とパネルは一致した (カーブの領域は差 0)。
- 編集した点は黄色のまま。AFT 済みの点は赤い。

### 判定 (式 q = s11·P·(S/5)、s11 = 0.5g + 0.5 に当てはめた計算。全面なので S/5 = 1)
- AFT 後の 65% から、P ∈ [0.645, 0.655) になる (g = 0.999999762)。
- 編集 1 ビンあたりの g の増分を仮に置いて、5 段の系列 35・36・36・37・37% と AFT 後の 65% を同時に満たす (P, g0) を総当たりで探した。
  - 予測 A (マーカーなし、+1/81/ビン): 解あり。P 0.645〜0.655、Reset 直後の g0 = 0.078〜0.100 (6.3〜8.1/81)。
  - 予測 B (マーカーあり、+1/27/ビン) と、+1/27 + 1/81 の場合: 解なし。B なら 4 ビンで約 +5% 上がるはずで、観測の +2% (丸めを含めて +1〜3%) と合わない。
- → **手動 Fine-Tune は +0x1c のマーカーを立てない可能性が高い** (+1/27 型は観測と合わない)。`quality.py` の `_curve_blob` は変更しない。QUALITY.md §8.5 の該当行を「観測と整合 (マーカー型は不整合)」に書き換えた。
  - 独立検証者の走査では、系列に合う増分は 1 ビンあたり 1/81〜1.5/81 の範囲 (1.75/81 以上は解なし)。ちょうど 1/81 とまでは確定しない。
  - 増分の +1/81 が「編集したビンの前のビン」(次のビンの語 0 の分岐) に付くのか、別の分岐なのかは、Quality の値からは区別できない。§8.2 の読みと矛盾はしない。

### 新しく見つかった差 (未解決)
- Reset 直後の Quality は 35% で、指示書の予測 23% ではなかった。予測は P ≈ 0.45 を前提としていたが、この画面の P は約 0.65 だった。
- 式に当てはめると、Reset 直後の g0 は 6.3〜8.1/81 になる。§8.3 の「新規プロファイル g = 1/81」とは合わない。
  - 1/81 だとすると、Quality は 0.5062 × 0.65 ≈ 33% になる。
- 原因として、次の 2 つが考えられる (推定)。
  - Reset Fine-Tuning Results の後の曲線のバイトが、新規の Build の直後と違う。例えば、いくつかのビンのレベル語の下位バイトが 0 でない。
  - この P では、§8.3 の新規の状態の読みのどこかが違う。
- この未解決の点があるので、編集 1 ビンあたりの増分のモデル (上の判定) の前提も完全には固まっていない。
- repro は Reset を「AFT 前」として扱い g = 1/81 にするので、同じ状態でも 2% 低く出るとみられる (推定。repro の全面の P は測っていない)。
- 確かめる方法は 2 つある。
  - 本家で Reset せずに新しく Build Profile (Full frame) を作り、Quality を見る。ただし今の AFT 済みのプロファイルを置き換えるので、Copy Profile の控えか Undo での復旧が前提になる。
  - Reset Fine-Tuning Results の関数を静的に読む。

### 独立検証 (サブエージェント、2026-10-26)
- 撮影 11 枚の読み値、仮説の総当たり、最後の画面と開始時の一致を再現し、**APPROVE** (条件付き)。
- 条件は §8.5 の「観測で確認」を弱めること。上の書き換えで対応した。
- 検証者の一時ファイル: scratchpad/verify_mft/ (s.py と切り出し画像)。

## 統括の追記 (2026-10-26 深夜): 完成度=g×100 の発見と厳密な検証

### 発見
- g の呼び出し元を全逆アセンブルで探すと Quality (RS 0xa4acf8) の他に 4 か所あった。
  Simple/Advanced パネル (RS 0xe2e20 / 0xe437c) と Profile Viewer (RS 0x118ef8 / 0x119038) で、
  **g×100 を qRound して "%" を付ける** (同じ丸めと `bl 0x15ce0` のラベル書式。片方は条件付きで "n/a")。
  これがパネルの **Completeness** 表示である (ラベル文字列 "Completeness" RS 0x14a92de)。
  観測の「完成度 85%→0%→1→2→4→5%」は g の直接読みだった。

### 厳密な検証 (float32 の演算順まで再現。全 6 ペアが一致)
- AFT 状態: g = 23/27 = 0.85185 (85.19% → 85)。s11 = 0.92593。Quality 65% から P ∈ [0.704, 0.707)。
- Reset 直後: g = 0 (完成度 0% → g < 0.005)。q = 0.5×P ≈ 0.352 → 35%。
- 赤 4 ビン編集: 各 +1/81 (完成度 1.23→1、2.47→2、3.70→4、4.94→5。Quality も P ∈ [0.704, 0.707) で 35→36→36→37→37% の全段が一致)。
- マーカーあり仮説 (+1/27/編集) は初段で完成度 4%/Quality 37% になり、観測 (1%/36%) と初段から不整合。**完全に否定された**。
- 検証者の「1〜1.5/81 の範囲」は Quality の丸めだけを見たもの。完成度を合わせると**ちょうど +1/81** に決まる (1.5/81 なら初段が 1.85% → 2% になり観測の 1% と合わない)。

### AFT のマーカー条件の訂正 (§8.3 の「全ビン」は誤り)
- `curve_post` は測定ビンの +0x1c を明示的に 0 にし、`curve_fill` の `write()` は fillable (未測定・無印) なビンにだけ +0x1c = 1 を書く。**マーカー = 未測定ビン**である。
- 全面の猫 = 23 マーク (4 ビン測定)、test.mp4 の auto 領域 = 19 マーク (8 ビン測定)。厳しい類似度ゲートで通るタイルが少ない busy な画像では多くのビンが未測定になる。
- repro の `_curve_blob` は従来「AFT 後は全マーク」としていたため Quality が系統的に高く出ていた。
  `nv_aft_v5.apply_aft` が AFT ポート自身のバイトからビン旗を読んで prof["aft_bin_flags"] に保存し、
  `_curve_blob` がそれを使うように修正した (フィルタ計算には使わない付加情報のみ。処理コアの算法は不変)。
  フラグなしの旧保存プロファイルは全マーカー代替 (従来の近似)。

### Reset の未解決点 (§8.6 に記録)
- Reset 直後は g = 0 で、新規ビルドの 1/81 (完成度 1%) とは違う。段係数は常に 1.0 なので −1/81 の相殺はなく、
  残る説明は **0xc2a4 = 0 (Reset が LZMA 旗を消すか復元しない)** である。
- 確かめるには新規 Build (Full frame) の Quality/完成度の観測 (完成度 1% なら Reset 固有、0% なら Reset ≡ 新規) か Reset 関数の静的読み。
- repro は Reset を新規同様に扱う (最大 0.5 ポイントの境界差)。

### UI への追加
- Completeness (g×100) の表示を Device Noise Profile パネルと Profile Viewer に追加した (本家と同じ 2 つの表示)。
- 自己試験に Completeness と AFT ビン旗の項目を追加。パネル幅テストの 1px オーバーフローは値ボックス 58→52px で解消。
