# VERDICT: wp_aft 修正 (2026-10-18) 独立再検証 (検証者)
本家の AFT は実行していない。本家は一度も呼んでいない (nvh 不使用)。逆アセンブルは /tmp/nv_expand/disasm_full.txt を禁止範囲を除いて読んだ。
(注: 9bdf30 の抽出中に 0x9be2d0 付近の行に 0x1489000 ページへの参照が出力に混じった。解析していない。)

## 判定表
| # | 項目 | 判定 |
|---|---|---|
| a1 | (1) 位相 2 がビン 0..8 | CONFIRMED |
| a2 | (2) 段係数 5 語 = 1.0f (位相 2・3、curve_fill.write) | CONFIRMED |
| a3 | (3) CV 鎖の NaN は続行 | CONFIRMED |
| a4 | 9bdf30 の memset_pattern16 は 8 か所 | CONFIRMED (前回の私の「7」は数え違い。REFUTED だったのは私の数) |
| a5 | 「FIX_REQUEST の 4 か所目は無かった」 | PARTIAL (下記) |
| b1 | verify_self.py 178 PASS | CONFIRMED |
| b2 | 誤り注入 12 種すべて検出 | PARTIAL: 作者の 12 種は未再実行。私の別 6 種のうち 3 種が未検出 |
| c | 区間 A/B 再測定 | 未実施 (任意)。作者の数値は読んだのみ。AFT 曲線のビン 8 以外の差は d で確認 |
| d | 修正で他の振る舞いが変わっていない | CONFIRMED (区間 A の 3 フレーム) |

## a. 逆アセンブルとの照合
- 位相 2: 0x9b5180 `tst; b.eq 0x9b5238` → 本体 0x9b5188〜 → 0x9b5238 `cmp x9,#0x100; b.eq 0x9b5258` は本体の後。0x9b5240〜0x9b5254 で次の旗を読み x9+=0x20 して 0x9b5188 へ戻る。x9=0x100 (ビン 8) で本体が走った後に終了する。ビン 0..8 で正しい。
  (本体中の 0x9b51ac〜0x9b5210 は禁止範囲で未読。0x9b5210〜0x9b5234 の格納 (level, 係数 q0+w11, count=1, +0x1c=0 (strh)、+0x1e = 3.0 < v) は移植と一致。)
- 段係数: 位相 2 は 0x9b5108 `memset_pattern16(sp, ..., 0x14)` → 0x9b521c `stur q0,[x20,#4]`・0x9b5220 `str w11,[x20,#0x14]` で 20 バイト = 5 語を書く。位相 3 は 0x9b52cc〜0x9b52d8 で x1=x19=0xc96000+0x2e0, w2=0x14。
  模様はファイル (arm64 スライス、`__TEXT,__const` addr==offset) の 0xc962e0 から `00 00 80 3f` ×4 を読んだ (自分の python で 16 バイト = 1.0f ×4 を確認)。
- 位相 3 の条件 (0x9b529c: count>0 または byte+4 ≠ 0 → 本体、`s10 = avg × level`、count=0、byte+4=1、byte+6 = 3.0 < s10) は移植の body() と一致。
- 9bdf30 の memset_pattern16: 禁止範囲を除いて数えた。関数本体は 0x9bdf30〜0x9be9b0 付近 (0x9be6c8 の ret の後に同じ関数の外出し部分 0x9be6cc〜が続く。0x9bea50 に次の関数の prologue)。
  呼び出し (`bl 0xc8faf0`): 0x9be33c・0x9be3c0・0x9be4b0・0x9be580・0x9be668・0x9be758・0x9be82c・0x9be8d0 の **8 か所**。すべて直前に `mov w2,#0x14`、x1 は adrp 0xc96000 + 0x2e0 由来 (x20/x23/x19)。禁止範囲 0x9be22c〜0x9be29c は読んでいない (そこに追加の呼び出しがある可能性は排除していないが、範囲外の 8 か所は作者の列挙と一致)。
  私の前回の「7 か所」は 0x9be6c8 の ret で数え止めた誤り。
- 4 か所目の主張: 移植で係数を 0 から 1.0f にした書き込み箇所は curve_post 位相 2・位相 3・curve_fill.write の 3 か所で、`grep -n COEF1` でも他に無い (auto_fine_tune_fast は共用)。9bdf30 側に memset_pattern16 は 8 か所あるが、移植の write() は 1 つの関数に集約されているので 8 か所すべてが write() 経由。
  FIX_REQUEST の「4 か所」が何を指すかは私には不明。PARTIAL: 移植の別経路に書き込みが残っていないことは diff と grep で確認、FIX_REQUEST 側の意図は未確認。
- CV 鎖: 0x9b65c0〜0x9b6960 を読んだ (0x9b636c〜0x9b63c8 は未読)。各語 m: 0x40+4m の t が `b.mi` 終了、p も `b.mi` 終了、許容 0 なら `b.ne` に落ちて次語の t を読み `b.pl` で続行 (0x9b65fc, 0x9b668c, 0x9b671c) / 負なら終了。
  どちらの検査も「順序付きで < 0 のとき終了、NaN は続行」(fcmp NaN は N=0 で b.mi 不成立・b.pl 成立)。m=6 の許容 0 は 0x9b6910 `b.eq 0x9b6960` で終了。新しい `if wt < 0: break` はこれと一致、旧 skip_t 分岐 (NaN で break) が誤りだった。

## b. 自分のフォルダでの再実行
- cp/ に src/verify/base を複写 (verify_self.py の本家ファイルのパスだけ絶対パスに変更)、`PYTHONDONTWRITEBYTECODE=1 python3 -I verify_self.py`: **178 PASS / 0 FAIL / ALL PASS / exit 0** (`log_verify_self.txt`、本家ファイル照合の 4 件を含む)。
- 私の誤り注入 6 種 (作者の 12 種とは別。`mut/n*/`、`run_mut.sh`、`log_mut_*.txt`):
  | 注入 | 結果 |
  |---|---|
  | n1: 類似度の分位点許容 stats[0x31+l] → stats[0x30+l] | 検出 (6 件 FAIL) |
  | n3: 位相 3 の本体条件 `or` → `and` | 検出 (4 件) |
  | n4: avg = Σ/(n+1) | 検出 (2 件) |
  | n2: curve_fill の上側外挿 `last > 5` → `last > 4` | **未検出** (exit 0) |
  | n5: 位相 2 の旗 `THREE < q` → `<=` | **未検出** (境界の等号。影響は小さい) |
  | n6: CV 鎖の許容に s_mult でなく s_lvl を掛ける | **未検出** (exit 0) |
  結論: 作者の「12 種検出」は私の再実行ではなく未確認。さらに試験には盲点がある (n6 は実出力に影響する実質的な誤り。n2 も外挿結果が変わる)。試験は移植者の読みとの整合で、本家との一致証拠ではない。

## d. 修正前後の出力差
cmp_prepost.py: 修正前 (`../../wp_aft/tmp/pre_fix_20261018/nv_aft.py`) と修正後の `auto_fine_tune_fast` を、区間 A (test_v02 n=296 から 32f のうち 16, 20, 24 番目) の同一入力・同一プロファイルで比較し、出力 0xc300 バイト全体をバイト比較 (`log_cmp_prepost_296.txt`)。
- 差が出るバイトはすべて曲線表 0xbf4a〜0xc29b の中。内訳は、各ビンの段係数 5 語 (上位 2 バイトのみ変化 = 0 → 1.0f)、および ch0 のビン 8 のレベル (+count +旗: 0xc044〜0xc046・0xc05c・0xc060) だけ。
- ビン 0〜7 のレベル語、ch1/ch2 のビン 8 のレベル、曲線表の外のバイトは不変。
- ch0 ビン 8: f16 0.7649 → 0.7047、f20 0.7858 → 0.7132、f24 0.6868 → 0.7208 (フレームにより増減)。作者の「ビン 8 だけ変わる」と整合 (係数は PSNR 経路で使われない、と作者は言うが、その点は私は未確認)。
- 区間 B と PSNR の再測定はしていない。

## マージしてよい
- 位相 2 のビン 0..8、段係数 1.0f (curve_post 位相 2・3、curve_fill.write)、CV 鎖の NaN 判定: 逆アセンブルと一致 (禁止範囲を除く)。
- `shuffle_order`、`similarity`/`_penalty`/`_dev` の構造は前回どおり。
## マージ前に直す・注記する
- 試験の盲点 (n2 上側外挿閾値、n6 CV 許容の s_mult/s_lvl、n5 の等号) を MERGE_PLAN/REPORT に「未検出」として注記するか、試験を足す。
- 作者の「12 種検出」は独立に未再確認。「4 か所目」の記述は意図不明なので表現を弱める。
- PSNR (区間 A 47.125 ほか) は私は再測定していない。AFT の本家との一致はまだ未照合 (本家で実行不可)。
