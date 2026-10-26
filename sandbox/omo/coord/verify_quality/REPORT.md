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
