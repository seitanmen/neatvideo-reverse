# ネイティブ移行 N1: コア部品の C++ 移植 — 委託タスク一覧 (分割版)

作成: 2026-10-26 (統括)。計画の全文は `native/MIGRATION.md`。このファイルは N1 を
ClaudeCode ワーカーに委託できる単位に分割したもので、各タスクは独立したセッションで実行する。

## 共通規約 (全タスクで絶対に守ること)

1. **ソース・オブ・トゥルースは Python の移植実装**。各タスクに書かれた repro の関数を
   C++ へ写す。本家バイナリは読まない (保護区画に触れる必要は一切ない)。
2. **repro の Python ファイルは変更禁止** (読み込みのみ)。移植対象に誤り・不明点を見つけたら
   直さずに REPORT.md に記録して報告する。
3. **合格判定はビット一致のみ**。誤差許容 (1e-7 など) は不合格とする。出力の全語を
   `struct.pack("<f", ...)` やビット列で比較する。
4. ビルド規約: `-std=c++17 -O2 -ffp-contract=off -fno-fast-math -Wall -Wextra`。
   fast-math・FMA 融合の禁止。加算順・演算順を Python 版と 1 行単位で同じにする。
5. **float 定数は Python 側のビットパターンから写す** (手計算の 16 進は誤る実績あり。
   例: −0.25 = 0xBE800000、1e-5 = 0x3727C5AC、2.5 = 0x40200000)。
   `np.array([0x...], np.uint32).view(np.float32)[0]` で確認してから C++ に `bits(0x...)` で書く。
6. NaN/±inf は明示的に扱う (ARM の fmax/fcsel と numpy の比較の意味論の違いを吸収する。
   `verify_quality.py` の `genuine_g`・`flatness` 実装と MIGRATION.md の落とし穴一覧を参照)。
7. 検証スクリプトは `native/verify_<名前>.py` として `native/verify_quality.py` と同じ形式
   (入力ブロブ列 → C++ バイナリ → Python 側の期待値と全語ビット比較)。
   入力は (a) 乱数・境界 (ゼロ/±inf/NaN/非正規/センチネル)、(b) 実写フレーム
   (ffmpeg で test_v02.mp4 / test.mp4 から切り出し。`quality_draft._frame` を参照) の両方。
8. 作業記録は `sandbox/omo/wp_native_<名前>/REPORT.md` (やったこと・検証結果・計測時間・
   Python 側への不具合報告)。処理コア (nvfilter.py / nv_aft_v5.py / nv_depth.py) は触らない。
9. **コミットしない** (統括が検証後にまとめてコミットする)。AGENTS.md も編集しない。
10. numpy のブロードキャスト・累積の順序 (特に double へのaccumulate、4 レーン処理、
    ブロック和) は AGENTS.md §6.5 の記述と Python コードの両方を読んで再現する。

## 検証の合格ライン

- 乱数・境界ケース: 各タスクの指定数 (重い部品は数百、軽い部品は数千)。
- 実写ケース: 指定のフレーム×領域すべてで不一致 0。
- `make -C native && python3 native/verify_<名前>.py` 一発で通ること。

## タスク一覧と依存関係

```
N1a 帯域測度 (nv_band)          ──┬─→ N1d 領域探索 (nv_flat)  ──┐
N1b ノイズ曲線 (nv_curve 一式)  ──┤                              ├─→ N1f プロファイル (nv_profile)
N1c LZMA (nv_lzma ↔ SDK 9.20)  ──┼─→ N1f                        │
N1a ─→ N1e モデル (nv_model)    ──┘                              │
N1a ─→ N1g 時間カーネル (temporal_v5)   N1a ─→ N1h 空間カーネル (spatial_v5)
N1a + N1e ─→ N1i AFT (nv_aft / nv_aft_v5)
```

並列開始できるのは N1a・N1b・N1c (依存なし)。以降は依存が済みしだい順次。

---

## N1a: 5 段ラプラシアン帯域測度 (nv_band)

- **移植元**: `repro/nv_band.py` — `model_levels` (語 0..54) とその部品
  (3×3 箱平均 `FUN_009ed014` 相当・2×2 平均 `FUN_009f15b4` 相当・(3,1)/4 拡大・
  局所 RMS `FUN_009ed164` 相当・平均/標準偏差 `FUN_009ef3fc`/`FUN_009ef520` 相当・
  分位点 `FUN_00997bcc` 相当)。境界扱い (端の複製/0 詰め、小さすぎる段の外挿、
  −0/NaN のビット列) も含む。
- **検証**: 乱数面 (正規/一様/スパイク/直流/NaN/±inf/非正規、サイズ 9×9〜512×512、
  奇数サイズ含む) 300+ と実写 12 フレーム (test_v02 n=0/100/312/576、test.mp4 n=0/300) の
  Y/Cb/Cr 面 128²・320² 切り出し。出力 55 語すべてをビット比較。
- **注意**: 分位点の反復 (刻み −1/2 倍、最大 8 往復、標本 49 以下の補間、正規分布近似の下限) は
  制御の流れを正確に。平均の 2048 値ブロック 4 レーン×2 組→double ブロック和の順序も再現する。
- **成果物**: `native/nv_band.{h,cpp}`、`native/verify_band.py`、REPORT.md。

## N1b: EstimateNoiseLevelCurve (nv_curve 一式)

- **移植元**: `repro/nv_curve.py` + `repro/nv_pixnoise.py` + `repro/nv_lines.py` —
  `genuine_noise_curve` (96px タイル配置・16×16 ブロックの IsClip 重み・BT.601 輝度の
  fma 連鎖・画素ノイズ `FUN_00998d98` (16 方向×2 段の線フィルタ・ランプ重み)・
  ビン集計・多項式補間・[1,2,1]/4 平滑・`noise_at` 補間器) と `is_clip_genuine`。
- **検証**: 乱数面 500+ (境界・巨大値 1e13〜1e20 を含む) と実写 8 フレーム全面。
  曲線 8 点 (輝度, ノイズ) と is_clip 値をビット比較。IsClip の 1/8 刻みヒストグラムの
  意味論 (最頻ビン過半/端 10 倍) も含む。
- **注意**: 線カーネルのオフセット `frintn(t·i+0.26f)`、NEON 4 画素累積和の順、
  frecpe+frecps の除算 (`FUN_009ee04c` 相当 — C++ の `/` は真の除算なので端数部だけ一致すれば
  よいかどうかは Python 実装を読んで判断し、REPORT に記す)、モード 0 の対 2 本目の除数 r..1 の
  本家の癖 (移植済み) を再現。
- **成果物**: `native/nv_curve.{h,cpp}` (pixnoise/lines を含めてよい)、`native/verify_curve.py`、REPORT.md。

## N1c: LZMA 圧縮 (nv_lzma ↔ SDK 9.20)

- **移植元**: `repro/nv_lzma.py` (Python 移植) — ただし C++ では **LZMA SDK 9.20 の
  パブリックドメイン C ソースをそのまま使う** (`LzmaEnc.h/LzmaEnc.c` 等。取得元と SHA256 を
  REPORT.md に記録する。AGENTS.md §6.5 の lzma_oracle/README.md に取得先の記録がある)。
- **本家の設定**: level 5、辞書 = 入力長を 4KiB に切り上げ、lc3 lp0 pb2 fb32、1 スレッド、BT4、
  `LzmaEncProps_Normalize` の reduceSize なし (9.20 の構造体)。
- **検証**: (a) 乱数面 (16 ビット型 `rint(x·16)` 飽和込み) 500+、(b) Python nv_lzma との
  バイト一致、(c) 本家の圧縮データが保存されている場合はそれとも一致 (`neatvideo_ofx_614/`
  配下の既存の照合産物があれば使う。なければ (b) まで)。
- **成果物**: `native/lzma/` (SDK ソース + LICENCE/出典記録)、`native/nv_lzma.{h,cpp}` (薄いラッパ)、
  `native/verify_lzma.py`、REPORT.md。

## N1d: FindFlatArea (nv_flat)

- **依存**: N1a + N1b の完了。
- **移植元**: `repro/nv_flat.py` — `find_flat_area_genuine` (縮小 `FUN_009b7e58` 相当・
  候補 `FUN_009999cc` (右下から逆順・厳密な小なり)・色変換・平坦度 (N1a で移植済みなら流用)・
  スコア式・サイズ表・状態コード 0〜5) と `transform` (色モード 0/1/2)。
- **検証**: 実写 20 フレーム (test_v02・test.mp4 から) × 色モード 0/1/2 の領域・状態コード、
  合成 10+ (黒帯・白飛び・定数・NaN/inf・同点候補)。縮小面・候補リストも中間出力として
  比較できるようにするとよい (Python 側も同様に取り出す)。
- **注意**: 取り消し (cancel) は対象外 (REPORT に「未移植」と明記)。難読化カウンタ
  (1 プロセス 1 回の加算) は 1 回目の値で固定。
- **成果物**: `native/nv_flat.{h,cpp}`、`native/verify_flat.py`、REPORT.md。

## N1e: BuildNoiseModel の 1 面モデル (nv_model)

- **依存**: N1a の完了。
- **移植元**: `repro/nv_model.py` (+ `nv_model_special.py`・`nv_model_bm.py`) —
  `model_full` の全 4096 語 (語 0..54 は N1a を流用、語 55 以降の方向フィルタ組
  DAT_01243988 の 5 組、族 A/R/dc、組 3・4 の族 B/C、`FUN_00ad319c` の自己相似
  (32×32 SAD で 12+12 オフセット) と `FUN_00ad1ae0` の分位点 0.5/0.85)。
- **検証**: 乱数面 100+ (方向ビンは libm の atan2/__sincos_stret に依存するので
  C++ は macOS libm をそのまま呼んでよい — Python も ctypes で同じ関数を呼んでいる。
  REPORT に明記) と実写 8 フレームの切り出し。set_=-1 と 0..4 の全組。
- **分割の目安**: 大きすぎる場合は「族 A まで」「族 B/C + 自己相似」の 2 件に分けてよい
  (その場合は N1e1/N1e2 として REPORT を分ける)。
- **成果物**: `native/nv_model.{h,cpp}`、`native/verify_model.py`、REPORT.md。

## N1f: プロファイル構築 (nv_profile)

- **依存**: N1c + N1d + N1e。
- **移植元**: `repro/nv_profile.py` — `build_profile` (0xc300 バイトの組立全部:
  面モデル配置・tile_dev (reference_floor は `data/ref_floor.npy` を読むか計算)・
  mc_stats (`nv_mcstats`)・curve 初期化・filled ビン・flags・size class・LZMA 部 (N1c)・
  W/H・0xc2c8 の既定値)。
- **検証**: 実写 6 フレーム × 2 種の領域 (128²・192²) で Python の出力 blob と
  **全バイト一致** (ヒープポインタ 0x00..0x17 は双方 0 でよい)。
- **成果物**: `native/nv_profile.{h,cpp}`、`native/verify_profile.py`、REPORT.md。

## N1g: v5 時間カーネル (temporal_v5)

- **依存**: N1a。
- **移植元**: `repro/nvfilter.py` の `temporal_v5` とその部品 (類似度ピラミッド
  `pyramid_weight`・画素単位重み・残留ノイズ追跡・`v5_params` の時間関連の既定値)。
- **検証**: 実写の 5 フレーム窓 (test_v02 n=310..316) 10+ と乱数窓 50+ で
  出力 3 面 + 残留ノイズ配列をビット比較。
- **成果物**: `native/nv_temporal.{h,cpp}`、`native/verify_temporal.py`、REPORT.md。

## N1h: v5 空間カーネル (spatial_v5)

- **依存**: N1a (なしでも書けるが検証に N1a の語を使う場面があるため後でよい)。
- **移植元**: `repro/nvfilter.py` の `spatial_v5` (a-trous 5 帯域・局所 Wiener 7×7・
  noise_level/channel_noise/帯域 Amount・mix) と `process_frame_v5` の空間部分。
- **検証**: 実写フレーム (1080p 3 フレーム) × 既定パラメータと ±変則パラメータ、
  乱数面 20+。出力 3 面をビット比較。
- **成果物**: `native/nv_spatial.{h,cpp}`、`native/verify_spatial.py`、REPORT.md。

## N1i: AutoFineTune (nv_aft / nv_aft_v5)

- **依存**: N1a + N1e。
- **移植元**: `neatvideo_ofx_614/analysis/noise/aft_port/src/nv_aft.py` (類似度
  `FUN_009b6338`・`curve_post`/`curve_fill`・タイル順 `FUN_009d39ec`) と
  `repro/nv_aft_v5.py` の係数結合 (`FUN_009aae90` の色マトリクス合成)。
- **検証**: 実写 6 フレーム ( 全面 or 128² 領域) で Python の `nv_aft.auto_fine_tune`
  の出力バイト (3 面の曲線語・旗) とビット一致。タイル順 194 件 (`nvh aftshuffle` 相当) も含む。
- **成果物**: `native/nv_aft.{h,cpp}`、`native/verify_aft.py`、REPORT.md。

---

## 統括側の流れ (ワーカーは実施しない)

ワーカーの REPORT 受領後、統括が (1) verify スクリプトを自分の環境で再実行、
(2) 独立検証者への回し (必要に応じて)、(3) native/ へのマージ + スナップショット更新 + コミット、
(4) MIGRATION.md の現状更新、を行う。不具合・疑義は REPORT.md の「Python 側への不具合報告」節に
まとめてもらえれば統括が判断する。

## 各タスクの貼り付け用プロンプト (例: N1a)

```
.claude/tasks/native_n1_split.md の「N1a」を読んで実行してください。
repro/nv_band.py の model_levels とその部品を C++ (native/nv_band.{h,cpp}) に移植し、
native/verify_band.py で Python とビット一致させてください。共通規約 (ソースは Python、
repro 変更禁止、ビット一致のみ合格、定数はビットパターンから、fast-math 禁止) を厳守。
完了したら sandbox/omo/wp_native_band/REPORT.md に書いてください。コミットはしないでください。
```

(他のタスクも同様に「N1x」の節を読ませる。)
