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

## 5. FUN_0098adbc : structure complète décodée + harnais opérationnel (2026-11-03, suite)

### Corrections majeures issues de la confrontation harnais ↔ transcription

1. **Disposition des candidats dans l'enregistrement (entrelacée)** : les notes (+0x10,+0x20 = candidat 0)
   étaient correctes et le port les lisait mal. Layout réel :
   - candidat k.x = +0x10 + 4k, candidat k.y = +0x20 + 4k
   - soit : les 4 X à +0x10..+0x1c, les 4 Y à +0x20..+0x2c
   Preuve : avec le layout par paires adjacentes, la transcription divergeait dès le bloc 0
   (le genuine convergeait vers (3,−7) avec SAD 20428, inexplicable) ; avec le layout entrelacé,
   cand2 = (2,−4) → second pattern search → (3,−7) SAD 20428 = exactement la sortie du genuine.

2. **Le raffinement 32×32 tourne sur CHAQUE bloc** (dès que W > 31 et H > 31), pas 1 sur 16.
   Preuve empirique : 704/704 flags +0x40 montés sur plans identiques ; 128/704 avec mouvement réel
   (les autres ont été acceptés avant, l'acceptation réussie court-circuite le 32×32).
   Le `tst x19, #0xf` ne contrôle que l'évaluation du piège tv_usec.

3. **L'acceptation réussie court-circuite le 32×32** : accepté → écriture vecteur+SAD → bloc suivant ;
   refusé → SAD = SAD_first, puis 32×32 (ancre = clamp(bloc−8, 0, W−32), thr = s9·weight·1.1,
   succès → vecteur = out − ancre, SAD = out_sad>>2, flag32 = 1).

4. **L'indirection des settings** : settings = *([this+8] + 0x3d00) — un POINTEUR stocké dans l'image,
   pas les settings en ligne (découvert par le SIGSEGV à 0x140 = lecture settings+0x140 sur NULL).

5. **Les facteurs s11/s9 viennent de la table image** : s11 = *(image + bs·4 + 0x15bd390),
   s9 = *(image + bs·4 + 0x15bd390 + 0x40). Subtilité d'aliasing : `add x8, x8, x10` réutilise x10
   dont les 32 bits bas viennent d'être écrasés par `ldr s10, [x9]` (x10/s10 aliasent le même
   registre physique) — le coefficient de niveau settings+0x150 sert donc AUSSI d'offset entier
   dans l'image. Avec slvl = 1.0 (bits 0x3f800000), l'offset est ~1 Go → lecture hors bornes (0).
   Dans l'instance réelle, settings+0x150 doit contenir une valeur dont le motif de bits est un
   petit offset (dénominal ≈ 0 comme float → les produits s0 = s8·s10 ≈ 0, d'où slack = 0 observé
   et facteur d'acceptation ≈ 0).

6. **Le planificateur FUN_0097f894 = fetch-and-add** (retourne l'ancienne valeur, avance de chunk) :
   un seul appel traite tous les blocs en mono-thread. Le chunking n'influence pas la sortie
   (le test 1/16 ne contrôle que le piège inerte).

### État du harnais

- `nvh adbc W H ref.f32 cur.f32 rec.bin nblocks [s140 s168 sd4 slvl sslack s11 chunkdiv weights.f32]`
  fonctionne de bout en bout (704 blocs, plans identiques et mouvement réel testés).
  Écrit rec.out.bin. Instance minimale : image 22 Mo + settings séparés (indirection) + table image.
- Transcription Python : `tools/verify_adbc.py` — les 20 premiers enregistrements du cas 0 correspondent
  bit à bit ; les écarts restants viennent des valeurs de settings (slack/thr) dont l'adresse exacte
  de lecture reste à épingler (voir §6).

### Comportement observé (mouvement réel, 2 images de test.mp4)

- 576/704 blocs acceptés (v ≠ 0, flag32 = 0), 128/704 en 32×32 (flag32 = 1, dont 96 à vecteur nul),
  0 bloc sans vecteur ni 32×32. Cohérent avec le modèle décodé.

## 6. Prochaines étapes (mises à jour)

1. **Épingler les valeurs de settings** par sondage avec le harnais (varier une valeur à la fois,
   observer acceptation/slack) — ou décoder l'initialisation (qui écrit settings+0x140/0x150
   et la table image +0x15bd390 ?). Une fois épinglées : fuzz bit-exact complet de la transcription.
2. Puis a94c/a79c (lissage) et c90c (régression + warp).
3. Port C++ (nv_motion6) avec la structure corrigée (32×32 sur chaque bloc, candidats entrelacés).
4. **Conséquence pour nv_motion.py** : `refine32_every=16` est structurellement faux (le 32×32
   tourne sur chaque bloc) — à corriger dans le port v6.

## 7. FUN_0098adbc : transcription bit-exacte (2026-11-03, suite)

### Ce qui a été épinglé cette session

1. **FUN_0097f894 = fetch-and-add atomique** (retourne l'ancienne valeur, avance de `chunk`) :
   vérifié empiriquement (x1=5 → +5, x1=100 → +100). Un seul appel de FUN_0098adbc traite donc
   tous les blocs [0, nblocks) en mono-thread ; la fonction re-saisit un chunk à la fin de chaque
   chunk et sort quand l'index atteint `total`.

2. **Indirection des settings confirmée** : settings = *([this+8] + 0x3d00) — un POINTEUR stocké
   dans l'image (SIGSEGV à 0x140 = lecture settings+0x140 sur un pointeur NULL).

3. **Arithmétique corrigée** : la constante de la table image est `mov w9, #0xd390; movk w9, #0x1, lsl #16`
   = **0x0001D390** (119 952) et NON 0x15bd390. Donc :
   - s11 (facteur d'acceptation) = *(image + bits(settings+0x150) + 0x1D390)
   - s9 (facteur de slack 32×32) = *(image + bits(settings+0x150) + 0x1D390 + 0x40)
   Confirmé par sondage : image remplie de 1.0 → slack = s0 (= s8·s10) ; settings+0x140 = 2.0 →
   slack ×2 (bits 0x80) ; sslack = 1e30 dans la table → slack = s0·1e30 (bits 2a49f2ca).

4. **Le SAD 32×32 compare la fenêtre ref au départ avec la fenêtre cur à l'ANCRE**
   (clamp(bloc−8, 0, W−32) / clamp(bloc−8, 0, H−32)), pas au bloc lui-même (0x98b8a0 :
   `add x2, x8, x20` avec x8 = origine_cur + ancre_y·stride et x20 = ancre_x). Conséquence :
   pas de débordement du plan paddé, les fenêtres 32×32 tiennent toujours.

5. **slvl (settings+0x150) est utilisé deux fois** : comme multiplicateur float (s10 dans
   s0 = s8·s10) et comme offset entier dans la table image (l'aliasing x10/s10 : `ldr s10, [x9]`
   écrit les 32 bits bas de x10). Le harnais prend donc slvl comme MOTIF DE BITS (décimal ou 0x hex)
   pour garantir la cohérence des deux usages.

### Chaîne complète des coefficients (définitif)

```
s8   = settings[+0x140]            (× settings[+0xd4] si settings[+0x168] != 1)
s10  = float_bits(settings[+0x150])          (bs >= 4 ; pour bs < 4 : settings[+0x144 + bs*4])
s11  = image[bits(s10) + 0x1D390]            (facteur d'acceptation)
s9i  = image[bits(s10) + 0x1D390 + 0x40]     (facteur de slack 32×32)
s0   = s8 * s10
s8a  = s11 * s0        # acceptation : 2 * (s8a * weight) + best < SAD_first
s9r  = s0 * s9i        # slack = s9r * weight ; thr32 = slack * 1.1
```

### Harnais

```
nvh adbc W H ref.f32 cur.f32 rec.bin nblocks [s140 s168 sd4 slvlbits sslack s11 chunkdiv [weights.f32]]
```
- records : nblocks × 0x44 octets {i32 x0, i32 y0, f32 vx, f32 vy, f32 c[8] entrelacés
  (4 X à +0x10..+0x1c, 4 Y à +0x20..+0x2c), f32 sx, sy, slack, i32 sad, u8 flag32, u8 flag41}
- écrit rec.bin.out.bin ; affiche l'index final et this+0x5a8.

### Vérification

- Transcription Python (`tools/verify_adbc.py`) vs `nvh adbc` :
  seed 1, 3 cas : **0 écart** ; seed 42, 12 cas (W 48..512, blocs de bord inclus) : **0 écart**.
  Records comparés champ par champ (x0, y0, vx, vy, candidats, slack, sad, flag32).
- Bugs corrigés en route (tous côté transcription) : conversion (unsigned char)(v+0.5f),
  offset +8 du padding dans le SAD, layout entrelacé des candidats, SAD 32×32 (pas 16×16),
  fenêtre cur à l'ancre, sortie anticipée du 32×32 hors bornes.

## 8. Prochaines étapes

1. FUN_0098a94c / FUN_0098a79c (lissage niveau 0) puis FUN_0098c90c (régression + warp) — même méthode.
2. Valeurs réelles de settings+0x140/0x150 et de la table image (+0x1D390) : à décoder dans
   l'initialisation ou à calibrer sur E9 (comme les coefficients de v5).
3. Port C++ (nv_motion6) puis v6 temporel (compensation + similarité) et comparaison E9.

## 9. FUN_0098a79c / FUN_0098a94c / FUN_0098ab5c (平滑化) : bit-exact (2026-11-03)

### FUN_0098a79c (5×5 近傍平均) — 完全解読

```
a79c(records, nx, ny, col, row, &out_x, &out_y) -> 0/1
```
- 5×5 ボックス: cols [max(col−2,0), min(col+2,nx−1)], rows [max(row−2,0), min(row+2,ny−1)]。
  **注意: 0x98a894 の `sub x17, x17, #0x1` で列カウンタ x21 は c0−1 から始まり、
  ループ入口の x21++ で c0 に揃う** (これを見落とすと方向カウンタが 1 ずれで常に失敗になる)。
- 各近傍の条件: SAD (+0x3c, i32) != 0x7fffffff、|vx − 自身のvx| ≤ 2.0、|vy − 自身のvy| ≤ 2.0
  (x20 = record+8 基点なので [x20+0x34] = +0x3c = SAD)。
- 方向カウンタ: left (c < col), right (c > col), up (r < row), down (r > row)。
- 受理条件: **up ≥ 4 && down ≥ 4 && left ≥ 4 && right > 3** (4 方向すべて)。
- 出力: out_x = Σvx/count, out_y = Σvy/count (float32 の除算)。失敗時は書かない。

### FUN_0098a94c (平滑化 1 パス) — 完全解読

```
a94c(s0=scale, x0=descA, x1=descB, x2=records, x3=nx, x4=ny, x5=chunk)
```
- チャンク状態: [x5+0x10] = 開始行, **[x5+8] = 1 パスの行進幅 (1 = 全行を順に)**。
  ループは 1 グリッド行を処理するとチャンク末尾に飛び、行カウンタに [x5+8] を足す
  (ハーネスで [x5+8] = ny にすると 1 行だけ処理して残りがスキップされる — 経験的に確定)。
  [x5+0] = バリアオブジェクトへのポインタ (その +8 が 0 ならスピン即退出)。
- 各レコード: (1) (sx,sy) = (vx,vy) (64 ビット複製、flag32 に関係なく全レコード);
  (2) flag32 == 0 → 次へ; (3) a79c → 失敗なら次へ;
  (4) rint(vx)==rint(smx) && rint(vy)==rint(smy) → 採用 (sx=smx, sy=smy);
  (5) それ以外: アンカー = clamp(ブロック−8, 0, W−32)/(…H−32)、候補位置 = アンカー + rint(smx/smy)、
  範囲 [−8, W−24]×[−8, H−24]、SAD32(ref[候補], cur[アンカー]) ≤ slack·scale + (SAD<<2) なら採用。
- SAD32 は 9f9290 (32×32 バイト SAD)。descA = 候補位置でサンプルされる面 (ref)、
  descB = アンカー位置の面 (cur) — adbc の 32×32 と同じ規約。

### FUN_0098ab5c (レベル 0 の追加パス)

smooth(scale=1.0) → **全レコードで (vx,vy) = (sx,sy)** → smooth(scale=2.0)。
内部のバリア (97f8e0) は並列処理の帳簿なので、ハーネスは 3 ステップを直接再現。

### ハーネス

```
nvh smooth     W H ref.f32 cur.f32 rec.bin nblocks [scale]   # a94c 1 回
nvh smoothfull W H ref.f32 cur.f32 rec.bin nblocks           # ab5c 相当 (1.0 → 複製 → 2.0)
nvh a79c       W H rec.bin nblocks col row                    # a79c 単体
```

### 検証

- 転写 (`tools/verify_smooth.py`) vs `nvh smooth`/`nvh smoothfull`:
  seed 1, 2 ケース不一致 0 / seed 42, 6 ケース不一致 0 (smoothfull と scale 1.0/2.0 の両方)。
- 過程で判明したこと: a79c が 5×5 ボックスの平均を正しく返すことの単体確認
  (`nvh a79c`: vx=100/vy=50 の一様レコードで ok=1 smx=100 smy=50)。

## 10. 残タスク

1. FUN_0098c90c (ゲイン/オフセット回帰 + ワープ) の照合 — 最大の未検証部品。
2. settings/テーブルの実行時値 (adbc の s11/s9i、a94c の scale 1.0/2.0 は観測済み)。
3. C++ ポート (nv_motion6) → v6 時間段 → E9。

## 11. FUN_0098c90c (回帰 + ワープ) : 入口の解読 (2026-11-03、途中まで)

### シグネチャ (観測、入口 0x98c90c〜0x98ca00)

- 引数: x0, x1 → [sp+0x90]/[sp+0xd0]、x2 = x24、x3 = x28、x4 → [sp+0x78]、x5 = x27 (モード)、
  x6 = x22、x7 → [sp+0x60]、スタック第 1 引数 (x19) = [x29+0x18]。
- x27 (arg6) の値で分岐: **0 → bs=16、それ以外 → bs=32** (`csel x5, #0x10, #0x20, eq`)。
  さらに (x27−1) < 2 (つまり x27 ∈ {1,2}) で elem = 4、それ以外 elem = 0 —
  この elem と align を FUN_0097eebc に渡して**記述子を内部で構築する**
  (margin=8、align = bs)。スタック上の記述子 (sp+0xf8) を 97eef8 で平面に結合。
- 呼び出し元 FUN_00989cd8 の適用経路 (+0xf4 != 0) から: x1 = [フレーム集合+0xb0]、
  x2 = [別記述子+0xb0]、x3/x4 = [+0xd0/+0xd8]、w5 = [+0xf0] (モード 0/1/2)。

### 構造 (呼び出し先から)

- 計測ループ: レコードごとに位置 = (x0 + rint(vx) − 8, y0 + rint(vy) − 8) とみられる
  (0x98cdbc〜0x98cdd8)。範囲外は FUN_0098d3f8/9d8e8 (枠外取得)、内部は FUN_009f94ac (mode 0/1) と
  FUN_009f9e38 (mode 2) で 32×32 窓を累積し、FUN_009fa980 (/4096) と FUN_009fab3c (/256) で平均。
  無効の印 = 0xff800001 (−3.4e38 のビット) を書く (0x98cde4: `mov w8, #-0x800001`)。
  記録 +0x41 (flag41) を見て再計測をスキップ (0x98ce84: `ldrb w27, [x20, #0x41]`)。
- ワープ: 行ごとに FUN_009fa6b0 (mode 1) / FUN_009fa83c (mode 2) を呼び
  (dst 行テーブル, src 行テーブル, param, stride)、mode 0 は stride·行で進めるだけ (0x98d17c)。
- 回帰: 0x98d188 からアキュムレータ (v8..v11 + [sp+0x98]) を 0 にして有効ブロックを集計。
  エントリに難読化ブロック 2 件 (0x98c988〜0x98c9c4、0x98d088〜0x98d0e0 — 記帳のみとみられるが未検証)。

### 残り (次セッション)

1. 計測ループの 3 モードの正確な意味 (mode 0/1/2 = 8bit/int16/float 面と SAD/窓平均の対応)。
2. 回帰の完全な式 (a, b, 平均|測定−参照|, 平均|(b+a·測定)−参照|) と有効率の閾値 30%。
3. ワープ行テーブル (9fa6b0/9fa83c) の出力形式と、時間フィルタ側での使用法。
4. ハーネスサブコマンド `nvh c90c` と転写のビット照合。

## 12. FUN_0098c90c : 回帰式の完全解読 (2026-11-03、続き)

### 回帰 (0x98d188〜0x98d340) — 確定

アキュムレータ (double): d8 = Σx (参照), d9 = Σy (測定), d10 = Σxy, d11 = Σyy、x15 = 有効ブロック数。

```
total = [sp+0x58] · [sp+0x60]            # 全ブロック数 (nx·ny とみられる)
validity% = x15 / total × 100.0          # float32、出力 [x8+0x10] に書く
if validity% <= 30.0 → 既定値を出力して終了
denom = n·Σyy − Σy²   (n = x15, double)
if denom == 0 → 既定値
a_d = (n·Σxy − Σy·Σx) / denom
a   = float32(a_d) に丸めてから double に戻し、clamp(a, 0.66, 1.5)
b_d = (Σx − a·Σy) / n    # ここでの a は float32 に丸めた値
b   = clamp(b_d, −64, 64)
```

出力構造体 (x8):
- [0] = a (clamp 0.66..1.5)、[1] = b (clamp −64..64)
- [2] = Σ|測定 − 参照| / n、[3] = Σ|(b + a·測定) − 参照| / n (有効ブロックのみ、
  測定値 == 0xff800001 (無効印) のブロックはスキップ)
- [0x10] = 有効率 %
- 既定値 (有効率 ≤ 30% かつ denom == 0): [0] = 1.0, [1] = 0.0 (DAT_01244888 の 8 バイト)、
  [2] = 0, [3] = 0。旧ノートの「+4 に有効率」は誤りで、+0x10 が正。

モデル: **参照 ≈ a·測定 + b** (測定 = 隣接フレームのブロック平均、参照 = 中心フレームのブロック平均)。
補正後の残差 [3] が時間フィルタの類似度に使われるとみられる (未確認)。

### ワープ (0x98d104〜0x98d184) — 構造

行ごとに FUN_009fa6b0 (mode 1) / FUN_009fa83c (mode 2) を呼ぶ:
`9fa6b0(dst_row_table[i], src_row_table[i], x24 (出力ポインタ), x28)` — x24 は stride ずつ進む。
mode 0 は `x24 += stride` のみ。行テーブルは入口のループ (0x98ca84〜0x98cac8) が作る:
x20[i·8] = (ベース + i·刻み) の列 — 詳細は未確定。

### 計測ループ (0x98cdb8〜) — 部分的に解読

- レコードごとに位置 = (x0 + rint(vx) − 8, y0 + rint(vy) − 8) とみられる
- 範囲外 → FUN_0098d3f8/9d8e8、内部 → FUN_009f94ac (mode 0/1) / FUN_009f9e38 (mode 2)
- 平均: FUN_009fa980 (/4096, mode 0/1) / FUN_009fab3c (/256, mode 2)
- 無効印 = 0xff800001、flag41 (+0x41) で再計測をスキップ
- flag41 が 0 のときだけ測り、測ったら flag41 = 1 を書くとみられる (未確認)

### 次のステップ

1. 計測ループの 3 モードの正確な意味と、測定値/参照値の配列の配置
2. ハーネス `nvh c90c` (引数の構成が複雑: インスタンス +0x538 の旗、2 記述子、
   行テーブル出力、測定配列、出力構造体) と転写のビット照合
3. ワープ行テーブルの正確な形式

## 13. FUN_0098c90c : 計測ループの累積構造 (2026-11-03、続き)

### 測定値から回帰アキュムレータへの累積 (0x98cf58〜0x98cf8c) — 確定

```
測定値 d12 (float) を測定配列 [x25] に書く (x25 は 4 バイトずつ進む)
参照値 = [x26] (参照配列、同じく 4 バイトずつ)
d8 += 参照          # Σx
d9 += 測定          # Σy
d10 += 測定·参照    # Σxy
d11 += 測定²        # Σyy
n++ ([sp+0x98])
```

### 除外条件 (0x98cf2c〜0x98cf54) — 確定

```
max(|測定|, |参照|) >= 2048.0 (0x40a0000000000000) なら累積をスキップ
(b.pl = 「以上かつ順序あり」。NaN もスキップ)
```
内部スケール (16 ビット固定小数点 ×16?) の飽和保護とみられる。

### 計測のモード分岐 (0x98ce28〜) — 確定

- mode 2: 測定 = FUN_009f9e38 (内部) / FUN_0098d8e8+9f9e38 (枠外)、平均 = FUN_009fab3c (/256)
- mode 1: 測定 = FUN_009f94ac (内部) / FUN_0098d3f8+9f94ac (枠外)、平均 = FUN_009fa980 (/4096)
- mode 0: 測定なしから既定値 (0x98cda0 へ直接)
- mode 2 の枠外パスは参照値 [x26] を 32×32 の種に複製してから 9f9e38 (0x98cfd0〜0x98cfe0)

### 残り (次セッション)

1. FUN_009f94ac / 9f9e38 の引数の正確な意味 (窓累積の単位)
2. 参照配列 [x26] の作られ方 (中心フレームのブロック平均とみられる)
3. 測定配列/参照配列のサイズと、時間フィルタ側での使用法
4. ワープ行テーブルの正確な形式
5. ハーネス `nvh c90c` と転写のビット照合

## 14. FUN_0099afb4 (時間類似度の段ごと処理) : しきい値式を命令レベルで確認 (2026-11-04)

### しきい値の計算 (0x99b370〜0x99b3d4) — 確定

```
s11 = [x9 + x27·4]                    ; 段係数 (DAT_012445a8[k] とみられる)
s0  = s11 × 1.1                       ; 0x3f8ccccd
s0  = s0 × s8                         ; s8 = v8 = 第 1 float 引数 (count 型鎖の値、未確定)
s0  = s0 × 1.2806249                  ; 0x3fa3eb84 = √1.64
s0  = s0 × 0.9                        ; 0x3f666666
s14 = scvtf(x22 − w8)                 ; w8 = (popcount 関係) + (w19 >> 31) − 16
s15 = s0 × s14                        ; ← 整数係数が掛かる (旧ノートの読みどおり)
s0  = |noise構造体[+0x14 + 4k]|       ; σ_k (モデル面の語 5+k = 帯域 q99)
t   = s15 × σ_k + s10                 ; s10 = 第 2 float 引数 (最粗段の T²)
2t  = t + t
w    = FUN_009eebf8(t, 2t, R, W, n)   ; w = clamp(2 − R/t, 0, 1) × 前段の重み
```

- 旧ノートの式 `s15 = (C_k × 1.1) × s8 × 1.2806 × 0.9 × (x22 − (popcount(w19) − 16))` を
  命令レベルで再確認。w19 = *(0x1488030 + 0x763648) (静的値 0xbc5368a5、popcount 16 → 減算 0)。
- σ_k は noise 構造体の +0x14+4k を fabs で読む ✓ (モデル面の語 5+k)。
- **未確定の 2 点**: s8 (第 1 float 引数 = count 型鎖) と x22 (第 3 引数) の意味。

### 関数内の処理構造 (呼び出し先から)

- 9eebf8 (ramp·mul) ×2、9ee93c (ランプ)、9ef0d0 (頑健化)、9ed4e0 (√正規化) ×2、
  **9944c0 (線フィルタ) ×3**、9ed014 (3×3 箱)、99bf40 (合成)、9efc94 (最大統合) —
  **空間フィルタの方向経路と同じ葉のセット** = 時間類似度にも方向フィルタによる
  構造差の検出が入る (§6.4 の「方向フィルタによる構造的な差の検出」の実体)。
- 組テーブル (0x1243700) の +0x4d8 の語を読む (方向セットの選択とみられる)。
- FUN_0097bbb0 (mode 2 の差の合成) = (A+B+C)/3 は解読済み (§13 系)。

### 次のステップ

1. s8 (count 型鎖) と x22 の意味の確定 — 99a76c の呼び出し側の引数を遡る。
2. 99afb4 の方向経路の完全な転写 (空間の nvs6 と同じ葉なので照合は可能)。
3. ノイズ分散ピラミッド (99a76c 側) の構成の解読。

## 15. s8 (第 1 float 引数) の正体 (2026-11-04、続き)

### 99a76c から 99afb4 への引数 (0x99ac3c〜0x99aca0)

```
w8  = count型鎖(~w22 − ror(w22,31)) の結果 − 14      # w22 = 大域状態語
x8  = 1 − w8                                          # = 15 − popcount系の値
s0  = scvtf(x8) × 1.3 (0x3fa66666) × [x24+0xc]        # x24 = noise 構造体
v8  = s0   (99afb4 の第 1 float 引数 = s8)
v1  = v8   (第 2 float 引数も同じ値 → T² として加算されるのは s8 自身)
x2  = [sp+0x68] の第 1 (99afb4 の arg3 = x22)
x4  = x26 + レベル·4 (noise 構造体のレベル位置)
stack: [sp]=x8, [sp+8]=3, [sp+0x10]=x27, [sp+0x18]=0, [sp+0x20]=1(byte)
```

- つまり **s8 = (15 − count型鎖(w22)) × 1.3 × noise[+0xc]** で、両 float 引数は同値。
  しきい値は t = C_k·1.1·s8·√1.64·0.9·(x22 − pc(w19)+16)·σ_k + s8。
- count 型鎖は「~w22 − ror(w22,31)」の SWAR popcount + [sp+0x64]>>1 レーンの合成
  (旧ノートの「popcount(w22) の単純関数ではない」を命令レベルで確認)。
- **残る未確定**: 実行時の w22 の値 (改ざん検知の状態語 — 正規ホストでの値) と
  x22 (arg3) の意味。w22 が静的初期値のままなら count 型鎖の値は計算可能
  (ネイティブ再実行または Python で命令列を再現)。

### 次のステップ

1. w22 の静的初期値から count 型鎖を Python で再現し、s8 の理論値を出す。
   (検証者の手順 `coord/verify_protect/VERDICT.md` の「命令列のネイティブ再実行」を参照)
2. x22 の意味 (99a76c の [sp+0x68] の由来を遡る)。
3. pyrgen2: しきい値に σ_k を掛ける本家構造 (t ∝ σ_k) で pyramid を再実装し E9 で再評価。

## 16. count 型鎖の Python 再現 — s8 の理論値が確定 (2026-11-04)

### 状態語の導出 (0x99a8a8〜0x99a8cc)

```
x9 = *(0x1487fc8) = +0x384b1c → 状態語は image+0x1489e64
w10 (静的値) = 0x1cf01a7d
w8 = (w10 ^ w10>>1) & 0x44014054; w8 |= w8 << 1
w22 = w8 ^ w10 = 0x1cf01abd (popcount 16)
[sp+0x64] = w10 (「w10(raw)>>1 レーン」の源)
```

### count 型鎖のエミュレーション (0x99aabc〜0x99ac38、Python で命令列を再現)

```
w9 = ror(w22, 31)
w8 = w10raw >> 1
w10 = bfi(w22>>30, w9); w9 = ~w22 − w9
w11 = bfi(w22>>29, w10); w9 −= w10
w10 = bfi(w22>>28, w11); w9 −= w11
... (sh 27..4 の往復) ...
w11 = bfi(w22>>3, w10); w9 −= w10
w12 = bfi(w22>>2, w11); w9 −= w11; w9 −= w12
w8 = bfi(w8, w12); w8 = ~w8; w8 = w9 + w8; w8 −= 14
→ **w8 = 0** (静的状態語で)
```

- つまり count 型鎖は**正規の状態語で 0 になるように設計された改ざん検知**
  (値が変わると非ゼロになりしきい値が崩壊する)。
- よって **s8 = scvtf(1 − 0) × 1.3 × noise[+0xc] = 1.3 × noise[+0xc]**。
- x22 (99afb4 の arg3) = 99a76c の x26 = 呼び出し側で 0 → しきい値の整数因子
  (x22 − (popcount(w19) − 16)) = 0 (w19 の静的値の popcount は 16)。
- **結論: しきい値 t = s8 = 1.3·noise[+0xc] (全レベルで一定、+ T² も同値)**。
  負の語 (小さすぎる段のマーカー) のときは t < 0 → (2t−R)/t ≥ 1 にクランプされ
  その段は重みに寄与しない (スキップ) とみられる。
- noise[+0xc] = モデル面の語 3 (第 1 統計のレベル 3 値)。sintel 96² の領域では
  レベル 3 が「小さすぎ」で −1.71 (無効マーカー)。

### 残る未解決

1. ノイズ分散ピラミッド (正規化 X = 差/(ノイズピラミッド+1e-4)) の構成 —
   レベルごとの正規化値の由来 (モデルの語 0..4 か、ピクセルごとの σ マップか)。
2. 99afb4 の方向経路 (線フィルタ ×3 → 頑健化 → 箱 → 合成 → √正規化 → 最大統合) の転写。
3. 4 つの 99afb4 呼び出し (0x99aca0/ad64/ae0c/aeb4) のレベル割り当てと段係数 s11 の表。
