## Verification Report — INT-2 (フィールド単位の処理パイプライン) 独立検証

### Verdict
**Status**: PASS（判定 **APPROVE**）
**Confidence**: high
**Blockers**: 0（High 0 / Medium 0 / Low 3。情報 3）

作業場所は `/private/tmp/claude-501/verify-int2/` です。中には次の 2 つを置きました。
- `new/`: worktree の写し
- `base/`: `git archive 6cd9e49` で取り出した main

worktree のソースは書き換えていません（`git status` は検証の前後で同じ）。次のものにも触れていません: 本家バンドル、Resolve、`reference/python/out/`、ユーザーの MeatVideoQt。
`~/.config/NeatRepro/nv_ui_qt.ini` の sha256 は、検証の前・selftest の後・verify_ofx の後・最後のいずれも `45d4258c…fcc6` で変わっていません。

### Evidence（どれも検証者が自分で実行した結果）

**プログレッシブ経路の回帰**

| Check | Result | Command/Source | Output |
|---|---|---|---|
| native クリーンビルド | pass | 写しで `make -j4 all` | rc=0、warning 0 |
| `make verify` 全本 | pass | `new/native` で `make verify` | rc=0、14 本。fieldproc は ALL BIT-EXACT、match 1107 ALL MATCH、fields ALL BYTE-EXACT、temporal real 36/36・frame 6/6、spatial real 30/30、profile real 12/12 など（10 分 55 秒） |
| 旧と新の nv_v5_test の出力を比較 | pass | `scripts/oldnew.py 777 --real` | kind 2/3 とその部品、570 件（NaN・inf・const・huge の入力と、実写を含む）の出力 453,713,680 バイトの sha256 が **同一** |
| 旧と新の libnvcore を比較 | pass | `scripts/libcmp.py`（NV_NATIVE_LIB を切り替え） | nvc_temporal と nvc_spatial の 162 件・2,394 配列のハッシュが同一 |
| verify_bridge（full） | pass | `python3 verify_bridge.py` | ALL MATCH、824 秒。frame 72/72（306,334,381 語）、depth 208,737,760 語、cancel 49、g8 152 |
| qtui のビルドと自己試験 | pass | cmake で全ターゲット → `qtui_selftest` | warning 0、130 passed / 0 failed |
| OFX | pass | `build_ofx.sh` → `verify_ofx.py --quick` | warning 0。81 比較で 0/67,028,480 語の不一致。ini は変化なし |
| 書き出しのバイト一致 | pass | compare_export の写し（REPRO を reference/python に直した）。`--exe` は新しい qtui_export | (i) 新しい lib で 19/19 SAME。(ii) Python 側だけ **main の libnvcore** にしても 19/19 SAME。(i) と (ii) の出力を突き合わせると、mp4・ts・mov はバイト単位で同一、mkv は framemd5 で同一 |
| 別の Makefile | pass | `Makefile.v5` と `Makefile.libm_alt` で nv_v5_test | どちらも rc=0、warning 0 |
| x86_64 でのコンパイル | pass | 変更した 3 ファイルを `-arch x86_64 -c` | 警告・エラーとも 0 |
| 公開シンボル | pass | `nm -gU` | 増えた C シンボルは `_nvc_temporal_shift` だけ。weak は新旧とも 0 |

**フィールド経路の照合**

| Check | Result | Command/Source | Output |
|---|---|---|---|
| verify_fieldproc を別の seed で | pass | `--seed 31337 --sensitivity` | copy 24/24・24/24、tshift 48/48、kind30=kind2 12/12、fields 96/96、real 18/18、bridge 24/24。作業者の変異 8 種もすべて検出 |
| 検証者が自分で書いた参照 | pass | `scripts/indep.py` | seed 9001 で 36/36、9002 で 48/48 がビット一致 |
| 検証者の変異 9 種 | pass | `scripts/mutants.py` | 9 種すべて検出（下記） |

`scripts/indep.py` について:
- PLAIN と SAME は、仕様から書いた `nvfilter.process_frame_v5` だけの参照です。SAME は「同じ偶奇の系列を半径 R//2 で処理する」と定義しました。fields_ref は使っていません。
- 素材は検証者が作りました: 斜めに動くテクスチャ、H = 26/33/38/59/70/88/100/124、TFF と BFF、R 0〜6。

検証者の変異 9 種と、ずれた件数（48 件中）:
- JOIN_SWAP（BFF で上下を取り違える）: 15
- HI_TRUNC（窓の上端を 1 つ削る）: SIGSEGV で落ちる
- MIX_SRC（Mix の元を別のフィールドにする）: 14
- C444_NOSHIFT（4:4:4 の色差のずれを外す）: 2
- FLOW_SIGN（動き推定の前のずれの符号を逆にする）: 8
- CHROMA_X（色差のずれを横に足す）: 6
- PARITY2（2 番目のフィールドの偶奇を誤る）: 16
- CHROMA_ZERO（色差のずれを 0 にする）: 8
- ULP（ずれを 1ulp 動かす）: 8

**L-1・ずれの符号・効果**

| Check | Result | Command/Source | Output |
|---|---|---|---|
| L-1 | pass | `l1/mk.sh` → `l1/chk.py` | 判定できた 12 本はすべて実際の順序と一致、食い違い 0。拒否は 3 本（progressive と prores の +ildct なし 2 本）（詳細は基準 4） |
| 順序が不明のとき | pass | `fields_cli.py` と kind 31 | CLI は rc=2 で止まる。API は first = −1/2 と mode = 3 で `std::invalid_argument` を投げる |
| ずれの符号と量 | pass | `scripts/sign.py` | 輝度の RMS は d=−0.50 で 0.390（−0.25 で 1.459、0 で 2.843、+0.5 で 5.629）。色差は −0.50 色差行で 0.594（ffmpeg の 420）と 0.466（2×2 平均）が最小 |
| 効果の表 | 再現した | `effect_fields.py --sigma 6` | TFF・BFF とも、作業者の表と小数第 2〜3 位まで一致。内部の照合の assert も通った |

### Acceptance Criteria

| # | 基準 | Status | Evidence |
|---|---|---|---|
| 1 | プログレッシブ経路が 1 ビットも変わらない | VERIFIED | 下記 1-1〜1-3 |
| 2a | 時間順は [n top, n bottom, …]、BFF なら下が先 | VERIFIED | `nv_fieldproc.cpp:74-80` を読んで確認。検証者の参照（`p[q::2]`、q は first から始まる）とビット一致。変異 PARITY2・JOIN_SWAP を検出 |
| 2b | 半径はフィールド数 | VERIFIED | PLAIN は、フィールド系列に `process_frame_v5`（P.radius そのまま）を掛けたものとビット一致 |
| 2c | 空間段はフィールド画像にそのまま掛ける。join で再結合する | VERIFIED | `process_field_v5` の後に `nvs::spatial_v5` と `mix_v5(fields[ci])`、最後に `join_fields`。odd H・H≡2 (mod 4)・4:4:4 も一致 |
| 2d | 半ラインのずれ: 輝度 −0.5、色差 −0.5 色差行 | VERIFIED | 下記 2d |
| 2e | 4:2:0 の色差 | VERIFIED（ただし条件付き） | 色差面は自分の行の偶奇で分ける。ずれは FY/2 + dyc（`nv_temporal.cpp` の to_grid 分岐）で、fields_ref と一致。変異 CHROMA_X・CHROMA_ZERO を検出。条件は下記 2e |
| 3 | 照合が空振りしない | VERIFIED | 下記 3 |
| 4 | L-1 は main で直っている | VERIFIED | 下記 4 |
| 5 | 仕様 4「静止部の残差が (b) で大幅に減る」 | 満たされない。ただし**仕様の想定違い**で、実装の誤りではない | 下記「効果の数値の判断」 |
| 6 | NVC_ABI_VERSION を 3 に据え置く | VERIFIED（妥当） | 下記 6 |

**1. プログレッシブ経路**
- 1-1（コードの読み）: `shift == nullptr` なら dyl = dyc = 0.0 です。3 つの分岐とも従来の文（`estimate_flow_v5(nb.p[0],…)`、`warp_plane(nb.p[0], FY, FX)`、FY と to_grid/2）だけを通ります。浮動小数点の演算は増えも減りもしません。命令列が完全に同じというわけではなく、分岐が 1 つ増えています。ただし算術は同一です。
- 1-2（観測）: 旧と新の nv_v5_test の出力は、570 件・453 MB で同一です。libnvcore も新旧で同一でした。verify_bridge・selftest・verify_ofx・書き出しもすべて一致しています。
- 1-3（nvc_temporal を包みにした変更）: 本体を `temporal_call(…, nullptr)` に移しただけで、文は同じです。差分を目で確認しました。

**2d. ずれの符号と量**
- 符号: `warp_plane` は out(p) = img(p + F) です。下フィールドの行 i はフレームの行 2i+1 なので、上フィールドの座標では i+0.5 に当たります。したがって上に合わせる量は −0.5 です。
- 検証者の測定では、輝度は −0.5 が最小（0.390）で、0 の 2.843 より大きく下がりました。
- 色差も、progressive 由来の 4:2:0（ffmpeg と 2×2 平均）で −0.5 色差行が最小（0.594 / 0.466）でした。
- 推測: MPEG-2 のインターレース 4:2:0 の色差の位置（上のフィールドは 1/4、下のフィールドは 3/4）でも、計算するとフレームで 2 行、つまり 0.5 色差フィールド行になります。

**2e. 4:2:0 の色差の条件**
- 正しいのは、入力が「フィールドごとに作られた 4:2:0」であるときだけです。
- 4:2:2 のインターレース素材を qtui の `read_frames`（`-pix_fmt yuv420p`。interl=1 を付けていない）で読むと、色差でフィールドが混ざります。これは INT-3 で必ず扱うべき点で、作業者のリスク 4 と同じです。

**3. 照合が空振りしないこと**
- fields_ref の `temporal_shift` は、`nvfilter.temporal_v5`（887〜938 行）と行ごとに突き合わせました。足されているのは dyl/dyc の 4 か所だけです。float32 の扱いも C++ の f32 加算と一致します。
- ずれ 0 のときは temporal_v5 とビット一致します（24/24）。
- 作業者の変異 8 種に加えて、検証者の変異 9 種もすべて検出しました。ULP（1ulp のずれ）まで検出しています。

**4. L-1**
- `video.cpp:189-198` を読みました。フレームの旗（interlaced_frame=1）があれば top_field_first で順序を決めます。旗がなければストリームの tt/bb だけを使い、tb/bt は Interlaced（順序不明、first = −1）として扱います。
- 素材は検証者が作りました。60p の各フレームの輝度を 20+20k にして tinterlace し、setfield を付けて次の形式で書き出しました: ffv1 mkv・ffv1 mov、prores +ildct と +ildct なし、mpeg2 mpg・mpeg2 mov、x264（各 TFF と BFF）、progressive。
- 結果:
  - 実際の順序（どちらのフィールドが暗いか）と一致したもの: 12 本。ffv1 mkv/mov は tb/bt、mpeg2 mov も tb/bt、mpeg2 mpg と x264 は tt/bb と書かれた素材です。
  - 拒否されたもの: 3 本（progressive と prores の +ildct なし 2 本）。
  - 食い違い: 0 本。
- 推測: このとおり、tb/bt から順序を決めることはありません。

**6. NVC_ABI_VERSION**
- 呼び出し側で版を調べているのは `nv_native.py:77` の `!= 3`（完全一致）だけです。qtui と ofx は版を調べていません（grep で確認）。
- 変更は追加だけで、既存の関数の意味は変わりません。N4e と同じ扱いになります。
- 新しい関数を使う側は、シンボルの有無を自分で調べる必要があります。このことはヘッダに書かれています。

### 効果の数値の判断（基準 5）

再現値（TFF、σ6。BFF もほぼ同じ）は次のとおりで、作業者の表と一致しました。

| 処理 | Y 静止 | Y 動き | comb 動き |
|---|---|---|---|
| (a) フレーム R2 | 1.235 | 6.397 | 11.228 |
| (b) PLAIN R2 | 1.974 | 3.809 | 4.693 |
| (b) SHIFT R2 | 1.926 | 3.921 | 4.838 |
| (b) SAME R2 | 1.643 | 3.805 | 4.776 |
| (b) SAME R4 | 1.345 | 4.051 | 5.134 |

**判断: 仕様の想定違いで、実装の誤りではありません。** 根拠は次の 2 つです（観測、`scripts/diag.py`。16 フレームで評価）。

1. **静止部では、フィールド処理には改善の余地がない。**
   - 静止した部分では、インターレースのフレームは progressive のフレームと同じ画像です。
   - 対照として、60p の元素材をそのまま progressive の v5 R2 で処理しました。静止部は **1.204** で、(a) の 1.215 とほぼ同じです。
   - つまり、フレーム単位の経路は静止部ですでに progressive と同等で、(b) にできるのは近づくことだけです。
   - 仕様が期待した「大幅に減る」は、インターレースの害（櫛状の残像）が出る動き部で起きています。Y 動きは −40%、comb は −58% です。
2. **静止部が悪くなる原因（作業者の推測の妥当性）**

   | 条件 | (a) フレーム | (b) フィールド |
   |---|---|---|
   | 時間段だけ | 2.239 | 2.319（SAME R4） |
   | 空間段だけ | 2.665 | 2.730 |
   | 両方 | 1.215 | 1.321 |

   - 段ごとの差は小さく、両方を掛けたときに差が広がります。
   - 静止部の縦の高域の誤差（comb 静止）も、(a) 1.259 に対し (b) 1.490 と大きくなっています。
   - 推測: 「フレーム全体に掛ける空間段は、別の時刻の隣り合う行を使える。フィールドに掛けると縦の細部が 2 倍の周波数になって削られない」という作業者の推測は、この傾向と矛盾しません。ただし、主な原因は 1 段では決まりません（部分的に裏付けられた、という程度）。
   - もう 1 つの要因として、R2 をフィールド数で数えると時間の幅が半分になります。SAME R4 にすると (a) に近づくのはそのためです（1.345）。

- 提案: 仕様 4 の合格条件は次のように改めるのがよいと考えます。
  - 動き部の残差と comb が大幅に減ること。
  - 静止部は、フレーム単位からの悪化が X% 以内であること。
- INT-3 の既定（SAME R4 か、動きに応じてフレーム処理とフィールド処理を切り替える方式か）は、本家との比較（INT-4）で決めるべきです。

### Gaps
- **[Low] L-a 色差の前処理（INT-3 への申し送り）。**
  - 4:2:2・4:4:4 のインターレース素材を qtui の `read_frames` で読むと、`-pix_fmt yuv420p`（interl=0）のためフィールドの色差が混ざります。この場合、SHIFT の色差のずれ 0.5 の前提が崩れます。
  - INT-2 の API の範囲の外ですが、INT-3 で `scale=interl=1` 相当の処理が必須です。
- **[Low] L-b 静止部の悪化。**
  - フィールド経路は、静止部でフレーム単位より 9%（SAME R4）〜60%（PLAIN R2）悪くなります。
  - 本家と同じ R=2 をフィールド数として既定にすると、プログレッシブ相当の静止部の画質が落ちます。
  - 既定のモードと半径は INT-3・INT-4 で、本家との比較を見てから決めることを推奨します。API はモードを引数で受け取るので、実装の欠陥ではありません。
- **[Low] L-c 既存の問題（INT-2 が原因ではない）。**
  - `make -C native verify JOBS=N` は `verify_fields.py` が `--jobs` を受け付けないので止まります。INT-1 からの問題で、検証者も再現しました。
  - compare_export の ROOT/repro の参照も直っていません。
- **[情報] 色差の動き部の悪化は v5 に元からある性質。**
  - 効果の表では、U/V の動き部の誤差がどの方式でも入力（1.53）より悪くなっています。
  - progressive の対照（60p 素材のフレーム単位の処理）でも Um 1.879・Vm 2.59 で、入力より悪くなります。したがって INT-2 の起因ではなく、v5 の動き部の色差の扱いの問題です。
- **[情報] 未試験の範囲。** PAFF のように途中でフィールドの順序が変わる素材は対象外です。本家との PSNR 比較はできていません（本家の出力が手元にないため。INT-4 で行う）。
- **[情報] 「同じ命令列」の厳密さ。** プログレッシブ経路で分岐が 1 つ増えるので、命令列が完全に同じではありません。浮動小数点の演算は同一で、出力のビット一致で確かめました。

### Recommendation
**APPROVE。**
- プログレッシブ経路は、旧と新のバイナリの直接比較を含む全回帰でビット不変でした。
- フィールド経路は、独立に書いた参照・別の seed・17 種の変異で正しさと照合の感度を確かめました。
- 静止部が悪くなるのは仕様の想定違いで、実装の誤りではありません。progressive の対照で、静止部の上限がフレーム単位と同等であることを示しました。

検証者が書いたスクリプトと素材は `/private/tmp/claude-501/verify-int2/` にあります。
- スクリプト: `scripts/oldnew.py`、`libcmp.py`、`indep.py`、`mutants.py`、`sign.py`、`diag.py`、`badfirst.py`
- L-1 の素材と確認: `l1/`
- ログ: `make_verify_new.log`、`bridge_new.log`、`selftest.log`、`verify_ofx.log`、`cexp_new.log`、`cexp_base.log`、`effect_s6.log`、`vfp_seed.log`、`mutants.log`