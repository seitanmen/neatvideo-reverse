# 統括注記 (2026-10-20): 検証との整合

この wp の文書には、独立検証 (`coord/verify_spatial/VERDICT.md`) で否定・訂正された記述が残っている。原文は記録として残し、ここに正しい記述をまとめる。
確定した事実は AGENTS.md と `neatvideo_ofx_614/analysis/blackbox/exp_results.txt` の対応する節にある。両者が食い違うときは、AGENTS.md・exp_results.txt・本書を正とする。

| 箇所 | 残っている記述 | 正しい記述 (根拠) |
|---|---|---|
| REPORT.md:16・:98、notes/spatial_analysis.md:89、MERGE_PLAN.md:36、src・base の nv_spatial.py | ε = 9.4433e-4 | ε = 0x38d1b717 = 1e-4 (float32 9.99999747e-05)。白色化は面ごとの `FUN_0098edb0` (0x98f86c〜) で行う (VERDICT:9)。src・base の定数は 2026-10-20 に直した。REPORT・exp_results の PSNR は旧定数で測った値 (1e-4 にした検証者の再測定は区間 A t=1.5 で 46.996/50.624/50.474、差 0.002 dB 未満) |
| REPORT.md:17-18・:49-50・:88-89 (§6-3)、notes:174-179 (§5 の「訂正」)、MERGE_PLAN.md:37・:40、harness_request.md:27、verify/verify_shrink.py:7・:69 | 通常経路の出力乗数は −model[+0x14+4k]、Amount は本体に入らない | 通常経路の乗数は −cfg[+0x14+4k] (Amount 系。0x991748〜0x991758 で読み 0x9917a0 で fneg)。cfg+0xb8 > 0 かつ値 ≠ 1 のときだけ `FUN_009ee7bc` の適応経路。notes §2.4・§2.5 の元の記述「scale = −amount」が正しく、§5 の訂正段落は撤回 (VERDICT:14)。model[+0x14+4k] は t の因子 |
| REPORT.md:17、harness_request.md:27 | t = 1.1·cfg[k]·\|model[+0x780+組·5+k]\| | 通常経路は t = (cfg[k] + 1e-6)·\|model[+0x14+4k]\|·1.1 (0x99155c〜0x991574)。+0x780+組·5+k は方向経路の添字 (VERDICT:13)。notes:104 の `model[k]` は model+0x14+4k (語 5+k) の意味 |
| src/nv_spatial.py:40 (halve)、:118 (ランプ) | 2×2 縮小 ((a+c)+b)+d、np.clip のランプ | 本家は先頭 W&~7 列 (a+b)+(c+d)、残り ((a+b)+c)+d。ランプは FMLA→FMIN→FMAX と端数部の非融合 + fcsel (AGENTS.md §6.1)。移植は未修正 (参考実装。数 ulp の差、VERDICT:19-23) |
| REPORT.md、src | P["spatial"] (κ) 未使用、t の倍率 genuine_t の意味 | どちらも未検証の仮定 (VERDICT:39・:41) |
| REPORT §5・harness_request.md | `nvh shrink` を要求 | `FUN_00990f74` の入口に難読化様のコードがあり、統括が却下 (harness_response.md)。verify_shrink.py は動かない |

## 追記 (2026-10-21)
- 上の表の「src/nv_spatial.py:40 (halve)、:118 (ランプ)」の行は、2026-10-21 に解消した。halve は FUN_009f15b4 の式、縮小式は FUN_009eebf8 (wmul) と同じ演算にした (実装者 1,785 件・独立検証者 2,484 件で本家と不一致 0。`coord/verify_spatial2/VERDICT.md`)。変更前の版は `tmp/nv_spatial.py.before_bitexact`。
- 残る差: 白色化 (本家は FRECPE+FRECPS の逆数と積とみられる)、t・σ を v5 の値で代用している点、高さ 2 未満・幅 1 の段。ランプを面全体で 1 回呼ぶというのは推定。
