# wp_motion6 — 動き経路の解読と照合 (2026-11-03 開始、統括)

E9 の残差の主因が時間段・動き補償 (v5 のまま) と推定されたため
(`work/omo/wp_spatial6/REPORT.md` §9.4)、本家の動き経路の残タスクを解決する作業枠。

## 1. FUN_0098adbc の難読化ゲートの安全性分析 (統括、2026-11-03) — 解決

`decode_motion.md` §2.3-5 と §6.3 の「未調査」だった 32×32 枝の入口の判定を読んだ。

### 構造 (観測、逆アセンブル)

```
0x987d78 (インスタンス初期化):  *(this+0x5a8) = 0; *(this+0x5b0) = 0;
0x987d80:   bl 0x9ec324                    ; FUN_009ec324 = gettimeofday(&tv, NULL); return tv_usec
0x987d84:   *(this+0x2f8) = 戻り値 (tv_usec の下位 32 ビット)
0x987d90:   *(this+0x2fc) = 0x1769eb53     ; 固定値

0x98b860 (32×32 枝の入口、16 個に 1 個のブロックだけ):
0x98b860:   tst x19, #0xf; b.ne skip       ; (ブロック番号 & 0xf) != 0 ならゲートを評価しない
0x98b868:   ldrh w8, [x26, #0x2f8]         ; tv_usec の下位 16 ビット
0x98b86c:   cmp w8, #0x1
0x98b874:   csel w22, [sp+0x1c], wzr, eq   ; w22 = (tv_usec & 0xffff == 1) ? ハッシュ値 : 0
0x98b904:   bl 0x98bf18                    ; 32×32 探索はゲートと無関係に必ず実行される
0x98b908:   cbz x22, skip                  ; w22 == 0 (ゲート不発火) なら書き込まない
0x98b90c:   *(this+0x5a8) = 0x400          ; 発火時だけ
```

### 意味

- **ゲートは時間ランダム**: `tv_usec & 0xffff == 1` はインスタンス作成時に 1 回決まり、
  成立確率 16/1000000 ≈ 1/62500。成立すると**そのインスタンスの全実行**で w22 が非 0 になる。
- **発火時の動作**: `this+0x5a8` に 0x400 を書く。この値は `FUN_00988470` の 0x988c24 で読まれ、
  `FUN_00989cd8` の第 6 引数 = `*(this+0x4a0) + *(this+0x5a8)` として渡され、
  0x98a030 の `ldr q0, [x8]` で**ポインタとして逆参照される**。つまり発火すると
  ポインタが +0x400 ずれて破壊される (クラッシュまたは不正な出力)。= **改ざん検知の罠**。
  解析者が「ゲートを強制通過させる」(常に 1 を書く) と破壊が常時起こる。
- **正規の決定的経路 = ゲート不発火**: 32×32 探索は必ず実行され、`this+0x5a8` は 0 のまま。
  旧記載「難読化値を 0 とするとこの枝は実行されない」は誤読 (探索は実行される。
  制御されるのは 0x5a8 への書き込みだけ)。
- `nv_motion.py` の移植 (`refine32_every=16`) は 32×32 枝を条件どおりに実行しており**構造は正しい**。
- エントリと 0x98b098/0x98b3cc の 3 か所のハッシュ更新ブロックは、いずれも
  ROOT スロットの負のオフセット定数 + 即値で解決される __DATA 内の語への記帳だけ
  (0x14897E0・0x1489F24・0x1489C24 に解決されることをファイル実値で確認)。数値出力には入らない。
  `[sp+0x1c]` のハッシュ値はゲート発火時の書き込み値になるだけで、不発火では使われない。

### ハーネスでの扱い

- `FUN_0098adbc` をハーネスで呼ぶときは `this+0x2f8 = 0` に初期化すれば
  ゲートは確実に不発火 (正規の決定的経路) で、安全に照合できる。
- ユーザー承認 (2026-11-03「Claudeが拒否しなければハーネスで扱えるようにしてもいい」) 済み。

## 2. FUN_0098b9a4 (16×16 パターン探索) の完全解読 (統括、2026-11-03)

### シグネチャ (観測)

```
FUN_0098b9a4(x0=未使用, x1=記述子(ref), x2=中心ブロック(cur)ポインタ, w3=bs(16),
             x4=開始x, x5=開始y, x6=ラウンド数(呼び出し元は 2), x7=オフセット表,
             [sp]=out_x(long*), [sp+8]=out_y(long*), [sp+0x10]=out_sad(long*))
戻り値 = 開始位置の SAD。out_x/out_y は long (整数、絶対位置)。out_sad = 最良 SAD。
```

- **out_x/out_y は float ではなく long** (呼び出し元が `sub x9, x9, x20` で整数差分を取り、
  `scvtf` で float 化して記録に書く)。b32 (FUN_0098bf18) は float だったので混同しないこと。
- x0 は関数内で一切読まれない (ハーネスから NULL で呼べる)。
- 呼び出し先は 0x9f862c (SAD16) と 0x9f8ddc (2 候補同時評価) だけ。大域は __stack_chk_guard のみ。

### オフセット表 (呼び出し元 FUN_0098adbc がスタックに構築、観測)

- ページ 0 (ラウンド 0): 8 近傍 (DAT_014877f8 = {(-1,-1),(0,-1),(1,-1),(-1,0),(1,0),(-1,1),(0,1),(1,1)}) を
  **×2** した {int32 バイトオフセット = 2·dx + 2·dy·stride, short dx=2·dx, short dy=2·dy}
- ページ 1 (ラウンド 1): 同じ 8 近傍を **×1**。エントリ 8 バイト、ページ 0x40 バイト。
- 呼び出し元はページ 0 を sp+0x100、ページ 1 を sp+0x140 に置き、x7 = sp+0x100 を渡す。

### ラウンドループ (観測)

- ラウンドごとに: 8 候補を 0x9f8ddc で 2 個ずつ 4 回評価。同点は 0x9f8ddc が 0 を返す
  (候補 0 側が勝ち) = スキャン順・厳密改善・先勝ち。
  改善したら中心を勝者の (dx, dy) だけ動かす (1 ラウンド 1 移動)。
- 継続条件: `x9 = 旧x24 + 1` が 3 以上の間。x24 は各ラウンドで 1 ビット右シフト。
  rounds=2 → 2 ラウンド、rounds=1 → 1 ラウンド、rounds=3 → 2 ラウンド (3→1→0)。
- ラウンド終了ごとに x21 += 0x40 (次のページ)。

### 速い経路と遅い経路 (観測)

- 速い経路の条件: `y + r ≤ H + 8 − bs`、`y − r ≥ −8`、`x − r ≥ −8`、`x + r ≤ W + 8 − bs`
  (r = 2·rounds − 1。rounds=2 なら ±3)。成立すると候補ごとの範囲検査を省略する。
- 遅い経路: 開始位置の検査は**符号なし** `(x + 8) ≤ W + 16 − bs` かつ `(y + 8) ≤ H + 16 − bs`
  (負は wrap して落せる)。失敗なら ret = 0x7fffffff、out = 開始位置、out_sad = 0x7fffffff。
  候補ごとに `(x + dx + 8) ≤ W + 16 − bs` かつ `(y + dy + 8) ≤ H + 16 − bs` (符号なし) を検査し、
  範囲外の候補は評価しない (スキップ)。

### 記述子の実際の値 (観測、NVH_DBG_PAD)

- stride は **16 バイト境界に切り上げ**: W=97 → 128 (W+16 = 113 ではない)、W=32 → 48。
- origin (desc+0x10) = mem + 8·stride + 8·elem を 16 バイト境界に切り上げた位置
  (W=97: 1032 → 1040)。座標は origin 相対 (= 平面座標)。

## 3. 照合 (観測)

- `nvh b9a4 W H ref.f32 cur.f32 cx cy vx vy [rounds]` をハーネスに追加した
  (nvh.c。テーブル 2 ページの構築と開始位置の rint は FUN_0098adbc と同じ式)。
- 移植 (Python 転写、`tools/verify_b9a4.py` の `genuine_b9a4`) との比較:
  - seed 1, 40 件: 不一致 0 (初回は変換の丸めと pad8 の +8 オフセットで 9 件不一致だったが、
    いずれも照合側のバグ。本家の変換は `(unsigned char)(v + 0.5f)`、座標は平面座標)
  - seed 777, 300 件: **不一致 0** (ret・out_x・out_y・out_sad の完全一致。
    境界ブロック・範囲外開始 (失敗経路)・rounds 1/2/3 を含む)
- 入力は実フレーム風の合成 (sin 構造 + ガウス雑音 + roll 位移動)。
- `verify_leaves.py`・`compare_python.py` (spatial6) への影響なし (nvh.c はサブコマンド追加のみ)。

## 4. 次のタスク

1. `FUN_0098adbc` (候補の検証と受理) のハーネス照合 — インスタンスの最小構築が必要
   (settings +0x140/+0x168/+0xd4/+0x144・+0x150/+0x184・+0x190、this+0x540、this+0x2f8 = 0、
   this+0x5a8、記録配列、重み配列)。`[sp+0x1c]` のハッシュは不発火なので定数でよい。
2. `FUN_0098a94c`/`FUN_0098a79c` (平滑化) と `FUN_0098c90c` (回帰とワープ) の照合。
3. 実行時値の確定: レベル数・settings の係数 (`FUN_0098a368`/`FUN_00988470` の設定経路)。
4. C++ への移植 (nv_motion6) と E9 での比較。


## 5. FUN_0098adbc : harnais opérationnel + découverte majeure (2026-11-03, suite)

### Harnais `nvh adbc` (fonctionnel de bout en bout)

```
nvh adbc W H ref.f32 cur.f32 rec.bin nblocks [s140 s168 sd4 slvl simg s11 chunkdiv weights.f32]
```

- Instance minimale construite par le harnais :
  - `[this+8]` = base image (22 Mo calloc) ; **settings = *([this+8] + 0x3d00)** — c'est un
    **pointeur indirect** : le mot à image+0x3d00 CONTIENT l'adresse des settings (découvert par le
    SIGSEGV à 0x140 = lecture de settings+0x140 sur un pointeur NULL). Settings alloués à part.
  - settings : +0x140 (float), +0x168 (int, si != 1 alors +0xd4 est multiplié), +0xd4,
    +0x150 (coef de niveau pour bs >= 4 ; pour bs < 4 c'est +0x144 + bs*4), +0x190 (= +0x150 + 0x40)
  - table s11 : image + 0x15bd390 + bs*4 (valeur lue comme coefficient dans la formule d'acceptation)
  - this+0x540 (diviseur de chunk), this+0x2f8 = 0 (piège désactivé), this+0x5a8 = 0
- Descripteurs ref et cur séparés (97eebc + 97eef8), plans paddés 8 px, même convention que b32/b9a4.
- Le planificateur FUN_0097f894 est un **fetch-and-add** (retourne l'ancienne valeur, avance de chunk) :
  un seul appel traite tous les blocs [0, nblocks) en mono-thread. Sortie : rec.out.bin + idx final.
- Pièges du harnais résolus : le typage `unsigned char**` de l'instance (décalage ×8),
  l'indirection des settings, le chemin absolu du bundle.

### Découverte majeure : le raffinement 32×32 tourne sur CHAQUE bloc

Test : 704 blocs (512×360), plans identiques (SAD 0 partout). Résultat : **flag +0x40 = 1 sur 704/704**.
Le test `tst x19, #0xf` (1 bloc sur 16) ne contrôle que l'évaluation du piège tv_usec, pas la recherche.
La lecture « 16 个に 1 個 » des notes (issue du décompilateur) était fausse. Conséquences :
- le port `nv_motion.py` (`refine32_every=16`) est structurellement faux sur ce point ;
- dans le port v6, le raffinement 32×32 s'applique à chaque bloc dès que W > 31 et H > 31 ;
- l'ancre 32×32 = clamp(bloc − 8, 0, W−32) / clamp(bloc − 8, 0, H−32) confirmée (vecteur = out − ancre).

### Comportement observé sur plans identiques (sanity check)

- SAD_first = 0 partout, acceptation jamais déclenchée (2·weight + 0 < 0 est faux),
  SAD final = SAD_first = 0, vecteurs = 0, flag 32×32 = 1 partout (succès trivial de la recherche 32×32).

## 6. Prochaines étapes

1. Transcription Python complète de FUN_0098adbc (boucle candidats + acceptation + branche 32×32)
   et fuzz bit-exact contre `nvh adbc` (même méthode que verify_b9a4).
2. Puis a94c/a79c (lissage) et c90c (régression + warp).
3. Valeurs d'exécution : coef de niveau (+0x144/+0x150/+0x184/+0x190), s11, nombre de niveaux —
   à observer sur le produit réel ou à décoder dans l'initialisation (FUN_00987f88/988470).
