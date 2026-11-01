/* nvcore_c_match - C ABI of the reproduction's own Auto Match score (N4e).  The
 * genuine Auto Match is an empty slot (work/omo/wp_automatch/DECODE.md); this score is the repro definition, built
 * from the bit-exact ports only: nvm::model_full (FUN_00995250), nvc::is_clip_genuine (FUN_009c6f88),
 * nva::similarity (FUN_009b6338) and nva::penalty (FUN_009d49b4).  Same conventions as nvcore_c.h.  Checked bit for
 * bit by native/verify_match.py (in `make verify`) against the Python reference native/match_ref.py.
 *
 * A match model is what the AFT reads of a 0xc300-byte profile (nvc_profile_build): per R, G, B face the first
 * NVC_MATCH_FACE words of the plane model (profile + 0x4c / 0x1fa0 / 0x3ef4) and the NVC_MATCH_STATS words of the
 * tile deviations (profile + 0xbc44 / 0xbd10 / 0xbddc).
 *
 * nvc_match_model: copies the model out of prof (NVC_PROF_BYTES): face[3 * NVC_MATCH_FACE], stats[3 * NVC_MATCH_STATS].
 *
 * nvc_match_score: rgb H x W x 3 float32 (the AFT's input: np.clip(yuv_to_rgb(..), 0, 255)); a reference model
 *   (ref_face 3 x NVC_MATCH_FACE, ref_stats 3 x NVC_MATCH_STATS: the frame's own model, BuildNoiseModel of its
 *   FindFlatArea square) and n models (faces n x 3 x NVC_MATCH_FACE, stats n x 3 x NVC_MATCH_STATS; R, G, B order).
 *   The 32 x 32 tiles of the AFT grid ((32 ty, 32 tx), ty < H / 32, tx < W / 32, row-major) whose R, G and B IsClip
 *   are all <= 0.33f (NaN passes, as in the AFT) are the used tiles; each gets the 2-level models
 *   m_c = model_full(plane_c, set 2, levels 2).  For a model (F, S) and a used tile, with s_lvl = 3, s_mult = 4 (the
 *   AFT's mode 0) and float32 operations:
 *     q_c  = float32(similarity(m_c, F_c, S_c, 3, 4) * float32(penalty(|F_c[5] / m_c[5]|, 3) * penalty(|F_c[6] / m_c[6]|, 3)))
 *     p    = float32(float32(q_G * q_R) * q_B), then p = 0 when NaN
 *   (the AFT gate's similarity product, with the reverse level test added: the AFT only fails a tile that is noisier
 *   than the profile).  With r = p of the reference model and p_k = p of model k, in tile order and in double:
 *     scores[k] = sum min(p_k, r) / sum max(p_k, r)   (0 when the denominator is 0; a fuzzy Jaccard index: 1 when
 *                 model k accepts exactly the tiles the frame's own model accepts)
 *   accept[k] = the number of used tiles with p_k > 0.5f.  info[3] = (tiles, used tiles, tiles with r > 0.5f).
 *   threads > 1 computes the tile models in parallel; the sums are taken in tile order afterwards, so the result
 *   does not depend on it.  cancel: asked before each tile.  Returns 0 ok, 1 too small (W or H < 48, as the AFT),
 *   2 cancelled, 3 model error, 4 no used tile (all clipped; scores 0), < 0 bad arguments.
 */
#ifndef NVCORE_C_MATCH_H
#define NVCORE_C_MATCH_H

#include <stdint.h>

#include "nvcore_c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NVC_MATCH_FACE 492
#define NVC_MATCH_STATS 51

void nvc_match_model(const uint8_t* prof, float* face, float* stats);
int nvc_match_score(const float* rgb, int W, int H, const float* ref_face, const float* ref_stats, int n,
                    const float* faces, const float* stats, int threads, nvc_cancel_fn cancel, double* scores,
                    int* accept, int* info);

#ifdef __cplusplus
}
#endif

#endif
