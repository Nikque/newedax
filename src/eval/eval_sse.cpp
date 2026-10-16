/**
 * @file eval_sse.cpp
 *
 * SSE/AVX translation of some eval.c functions
 *
 * @date 2018 - 2023
 * @author Toshihiko Okuhara
 * @version 4.5
 */

#include <assert.h>

#include "util/cpp_headers.h"
#include "core/bit.h"
#include "core/board.h"
#include "core/move.h"
#include "eval/eval.h"

#ifdef __ARM_NEON
#define __m128i		int16x8_t
#define	_mm_add_epi16	vaddq_s16
#define _mm_sub_epi16	vsubq_s16
#define _mm_slli_epi16	vshlq_n_s16
#endif

#if defined(hasSSE2) || defined(__ARM_NEON) || defined(USE_MSVC_X86)

/**
 * @brief Update the features after a move, with the parity of the empty squares given.
 *
 * eval_update_sse() with parity = eval_in->n_empties & 1. A loop over the moves of a position computes the
 * parity once (search_eval_2): the compiler then keeps fewer values across the calls of the loop.
 */
static EDAX_FORCE_INLINE void eval_update_sse_parity(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in, const int parity)
{
  #ifdef __AVX2__
	__m256i	f0 = eval_in->feature.v16[0];
	__m256i	f1 = eval_in->feature.v16[1];
	__m256i	f2 = eval_in->feature.v16[2];
  #if EVAL_SLOTS > 48
	__m256i	f3 = eval_in->feature.v16[3];
  #endif

	if (parity) {
		f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
  #if EVAL_SLOTS > 48
		f3 = _mm256_sub_epi16(f3, EVAL_FEATURE[x].v16[3]);
  #endif

		foreach_bit (x, f) {
			f0 = _mm256_add_epi16(f0, EVAL_FEATURE[x].v16[0]);
			f1 = _mm256_add_epi16(f1, EVAL_FEATURE[x].v16[1]);
			f2 = _mm256_add_epi16(f2, EVAL_FEATURE[x].v16[2]);
  #if EVAL_SLOTS > 48
			f3 = _mm256_add_epi16(f3, EVAL_FEATURE[x].v16[3]);
  #endif
		}

	} else {
		f0 = _mm256_sub_epi16(f0, _mm256_slli_epi16(EVAL_FEATURE[x].v16[0], 1));
		f1 = _mm256_sub_epi16(f1, _mm256_slli_epi16(EVAL_FEATURE[x].v16[1], 1));
		f2 = _mm256_sub_epi16(f2, _mm256_slli_epi16(EVAL_FEATURE[x].v16[2], 1));
  #if EVAL_SLOTS > 48
		f3 = _mm256_sub_epi16(f3, _mm256_slli_epi16(EVAL_FEATURE[x].v16[3], 1));
  #endif

		foreach_bit (x, f) {
			f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
			f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
			f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
  #if EVAL_SLOTS > 48
			f3 = _mm256_sub_epi16(f3, EVAL_FEATURE[x].v16[3]);
  #endif
		}
	}

	eval_out->feature.v16[0] = f0;
	eval_out->feature.v16[1] = f1;
	eval_out->feature.v16[2] = f2;
  #if EVAL_SLOTS > 48
	eval_out->feature.v16[3] = f3;
  #endif

  #else
	__m128i	f0 = eval_in->feature.v8[0];
	__m128i	f1 = eval_in->feature.v8[1];
	__m128i	f2 = eval_in->feature.v8[2];
	__m128i	f3 = eval_in->feature.v8[3];
	__m128i	f4 = eval_in->feature.v8[4];
	__m128i	f5 = eval_in->feature.v8[5];
  #if EVAL_SLOTS > 48
	__m128i	f6 = eval_in->feature.v8[6];
	__m128i	f7 = eval_in->feature.v8[7];
  #endif

	if (parity) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
  #if EVAL_SLOTS > 48
		f6 = _mm_sub_epi16(f6, EVAL_FEATURE[x].v8[6]);
		f7 = _mm_sub_epi16(f7, EVAL_FEATURE[x].v8[7]);
  #endif

		foreach_bit (x, f) {
			f0 = _mm_add_epi16(f0, EVAL_FEATURE[x].v8[0]);
			f1 = _mm_add_epi16(f1, EVAL_FEATURE[x].v8[1]);
			f2 = _mm_add_epi16(f2, EVAL_FEATURE[x].v8[2]);
			f3 = _mm_add_epi16(f3, EVAL_FEATURE[x].v8[3]);
			f4 = _mm_add_epi16(f4, EVAL_FEATURE[x].v8[4]);
			f5 = _mm_add_epi16(f5, EVAL_FEATURE[x].v8[5]);
  #if EVAL_SLOTS > 48
			f6 = _mm_add_epi16(f6, EVAL_FEATURE[x].v8[6]);
			f7 = _mm_add_epi16(f7, EVAL_FEATURE[x].v8[7]);
  #endif
		}

	} else {
		f0 = _mm_sub_epi16(f0, _mm_slli_epi16(EVAL_FEATURE[x].v8[0], 1));
		f1 = _mm_sub_epi16(f1, _mm_slli_epi16(EVAL_FEATURE[x].v8[1], 1));
		f2 = _mm_sub_epi16(f2, _mm_slli_epi16(EVAL_FEATURE[x].v8[2], 1));
		f3 = _mm_sub_epi16(f3, _mm_slli_epi16(EVAL_FEATURE[x].v8[3], 1));
		f4 = _mm_sub_epi16(f4, _mm_slli_epi16(EVAL_FEATURE[x].v8[4], 1));
		f5 = _mm_sub_epi16(f5, _mm_slli_epi16(EVAL_FEATURE[x].v8[5], 1));
  #if EVAL_SLOTS > 48
		f6 = _mm_sub_epi16(f6, _mm_slli_epi16(EVAL_FEATURE[x].v8[6], 1));
		f7 = _mm_sub_epi16(f7, _mm_slli_epi16(EVAL_FEATURE[x].v8[7], 1));
  #endif

		foreach_bit (x, f) {
			f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
			f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
			f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
			f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
			f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
			f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
  #if EVAL_SLOTS > 48
			f6 = _mm_sub_epi16(f6, EVAL_FEATURE[x].v8[6]);
			f7 = _mm_sub_epi16(f7, EVAL_FEATURE[x].v8[7]);
  #endif
		}
	}

	eval_out->feature.v8[0] = f0;
	eval_out->feature.v8[1] = f1;
	eval_out->feature.v8[2] = f2;
	eval_out->feature.v8[3] = f3;
	eval_out->feature.v8[4] = f4;
	eval_out->feature.v8[5] = f5;
  #if EVAL_SLOTS > 48
	eval_out->feature.v8[6] = f6;
	eval_out->feature.v8[7] = f7;
  #endif
  #endif
}

void eval_update_sse(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
{
	eval_update_sse_parity(x, f, eval_out, eval_in, eval_in->n_empties & 1);
}

#else	// SSE dispatch (Eval may not be aligned)

#include <emmintrin.h>

/*
 * The SSE2 version of a build that does not assume SSE2 (USE_GAS_X86), chosen when the CPU has it.
 * Up to v4.5.5-nikque.15 it was written as separate asm statements that kept their values in xmm2-xmm7 from one
 * statement to the next, and stored eval_out through input operands: the compiler is told nothing of that, and
 * may use these registers, or the memory, in between. The intrinsics let it know.
 */
__attribute__((target("sse2")))
void eval_update_sse(int x, unsigned long long f, Eval *eval_out, const Eval *eval_in)
{
	__m128i	f0 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[0]);
	__m128i	f1 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[8]);
	__m128i	f2 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[16]);
	__m128i	f3 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[24]);
	__m128i	f4 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[32]);
	__m128i	f5 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[40]);
  #if EVAL_SLOTS > 48
	__m128i	f6 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[48]);
	__m128i	f7 = _mm_loadu_si128((const __m128i *) &eval_in->feature.us[56]);
  #endif
	#define	EVAL_FEATURE_SSE(x, i)	_mm_loadu_si128((const __m128i *) &EVAL_FEATURE[x].us[(i) * 8])

	if (eval_in->n_empties & 1) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE_SSE(x, 0));
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE_SSE(x, 1));
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE_SSE(x, 2));
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE_SSE(x, 3));
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE_SSE(x, 4));
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE_SSE(x, 5));
  #if EVAL_SLOTS > 48
		f6 = _mm_sub_epi16(f6, EVAL_FEATURE_SSE(x, 6));
		f7 = _mm_sub_epi16(f7, EVAL_FEATURE_SSE(x, 7));
  #endif

		foreach_bit (x, f) {
			f0 = _mm_add_epi16(f0, EVAL_FEATURE_SSE(x, 0));
			f1 = _mm_add_epi16(f1, EVAL_FEATURE_SSE(x, 1));
			f2 = _mm_add_epi16(f2, EVAL_FEATURE_SSE(x, 2));
			f3 = _mm_add_epi16(f3, EVAL_FEATURE_SSE(x, 3));
			f4 = _mm_add_epi16(f4, EVAL_FEATURE_SSE(x, 4));
			f5 = _mm_add_epi16(f5, EVAL_FEATURE_SSE(x, 5));
  #if EVAL_SLOTS > 48
			f6 = _mm_add_epi16(f6, EVAL_FEATURE_SSE(x, 6));
			f7 = _mm_add_epi16(f7, EVAL_FEATURE_SSE(x, 7));
  #endif
		}

	} else {
		f0 = _mm_sub_epi16(f0, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 0), 1));
		f1 = _mm_sub_epi16(f1, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 1), 1));
		f2 = _mm_sub_epi16(f2, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 2), 1));
		f3 = _mm_sub_epi16(f3, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 3), 1));
		f4 = _mm_sub_epi16(f4, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 4), 1));
		f5 = _mm_sub_epi16(f5, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 5), 1));
  #if EVAL_SLOTS > 48
		f6 = _mm_sub_epi16(f6, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 6), 1));
		f7 = _mm_sub_epi16(f7, _mm_slli_epi16(EVAL_FEATURE_SSE(x, 7), 1));
  #endif

		foreach_bit (x, f) {
			f0 = _mm_sub_epi16(f0, EVAL_FEATURE_SSE(x, 0));
			f1 = _mm_sub_epi16(f1, EVAL_FEATURE_SSE(x, 1));
			f2 = _mm_sub_epi16(f2, EVAL_FEATURE_SSE(x, 2));
			f3 = _mm_sub_epi16(f3, EVAL_FEATURE_SSE(x, 3));
			f4 = _mm_sub_epi16(f4, EVAL_FEATURE_SSE(x, 4));
			f5 = _mm_sub_epi16(f5, EVAL_FEATURE_SSE(x, 5));
  #if EVAL_SLOTS > 48
			f6 = _mm_sub_epi16(f6, EVAL_FEATURE_SSE(x, 6));
			f7 = _mm_sub_epi16(f7, EVAL_FEATURE_SSE(x, 7));
  #endif
		}
	}
	#undef	EVAL_FEATURE_SSE

	_mm_storeu_si128((__m128i *) &eval_out->feature.us[0], f0);
	_mm_storeu_si128((__m128i *) &eval_out->feature.us[8], f1);
	_mm_storeu_si128((__m128i *) &eval_out->feature.us[16], f2);
	_mm_storeu_si128((__m128i *) &eval_out->feature.us[24], f3);
	_mm_storeu_si128((__m128i *) &eval_out->feature.us[32], f4);
	_mm_storeu_si128((__m128i *) &eval_out->feature.us[40], f5);
  #if EVAL_SLOTS > 48
	_mm_storeu_si128((__m128i *) &eval_out->feature.us[48], f6);
	_mm_storeu_si128((__m128i *) &eval_out->feature.us[56], f7);
  #endif
}

#endif // hasSSE2

#if defined(hasSSE2) || (defined(__ARM_NEON) && !defined(DISPATCH_NEON))

/**
 * @brief Set up evaluation features from a board.
 *
 * @param eval  Evaluation function.
 * @param board Board to setup features from.
 */
void eval_set(Eval *eval, const Board *board)
{
	int x;
	unsigned long long b = (eval->n_empties & 1) ? board->opponent : board->player;
  #ifdef __AVX2__
	__m256i	f0 = EVAL_FEATURE_all_opponent.v16[0];
	__m256i	f1 = EVAL_FEATURE_all_opponent.v16[1];
	__m256i	f2 = EVAL_FEATURE_all_opponent.v16[2];
  #if EVAL_SLOTS > 48
	__m256i	f3 = EVAL_FEATURE_all_opponent.v16[3];
  #endif

	foreach_bit (x, b) {
		f0 = _mm256_sub_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_sub_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_sub_epi16(f2, EVAL_FEATURE[x].v16[2]);
  #if EVAL_SLOTS > 48
		f3 = _mm256_sub_epi16(f3, EVAL_FEATURE[x].v16[3]);
  #endif
	}

	b = ~(board->opponent | board->player);
	foreach_bit (x, b) {
		f0 = _mm256_add_epi16(f0, EVAL_FEATURE[x].v16[0]);
		f1 = _mm256_add_epi16(f1, EVAL_FEATURE[x].v16[1]);
		f2 = _mm256_add_epi16(f2, EVAL_FEATURE[x].v16[2]);
  #if EVAL_SLOTS > 48
		f3 = _mm256_add_epi16(f3, EVAL_FEATURE[x].v16[3]);
  #endif
	}
	eval->feature.v16[0] = f0;
	eval->feature.v16[1] = f1;
	eval->feature.v16[2] = f2;
  #if EVAL_SLOTS > 48
	eval->feature.v16[3] = f3;
  #endif

  #else
	__m128i	f0 = EVAL_FEATURE_all_opponent.v8[0];
	__m128i	f1 = EVAL_FEATURE_all_opponent.v8[1];
	__m128i	f2 = EVAL_FEATURE_all_opponent.v8[2];
	__m128i	f3 = EVAL_FEATURE_all_opponent.v8[3];
	__m128i	f4 = EVAL_FEATURE_all_opponent.v8[4];
	__m128i	f5 = EVAL_FEATURE_all_opponent.v8[5];
  #if EVAL_SLOTS > 48
	__m128i	f6 = EVAL_FEATURE_all_opponent.v8[6];
	__m128i	f7 = EVAL_FEATURE_all_opponent.v8[7];
  #endif

	foreach_bit (x, b) {
		f0 = _mm_sub_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_sub_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_sub_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_sub_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_sub_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_sub_epi16(f5, EVAL_FEATURE[x].v8[5]);
  #if EVAL_SLOTS > 48
		f6 = _mm_sub_epi16(f6, EVAL_FEATURE[x].v8[6]);
		f7 = _mm_sub_epi16(f7, EVAL_FEATURE[x].v8[7]);
  #endif
	}

	b = ~(board->opponent | board->player);
	foreach_bit (x, b) {
		f0 = _mm_add_epi16(f0, EVAL_FEATURE[x].v8[0]);
		f1 = _mm_add_epi16(f1, EVAL_FEATURE[x].v8[1]);
		f2 = _mm_add_epi16(f2, EVAL_FEATURE[x].v8[2]);
		f3 = _mm_add_epi16(f3, EVAL_FEATURE[x].v8[3]);
		f4 = _mm_add_epi16(f4, EVAL_FEATURE[x].v8[4]);
		f5 = _mm_add_epi16(f5, EVAL_FEATURE[x].v8[5]);
  #if EVAL_SLOTS > 48
		f6 = _mm_add_epi16(f6, EVAL_FEATURE[x].v8[6]);
		f7 = _mm_add_epi16(f7, EVAL_FEATURE[x].v8[7]);
  #endif
	}

	eval->feature.v8[0] = f0;
	eval->feature.v8[1] = f1;
	eval->feature.v8[2] = f2;
	eval->feature.v8[3] = f3;
	eval->feature.v8[4] = f4;
	eval->feature.v8[5] = f5;
  #if EVAL_SLOTS > 48
	eval->feature.v8[6] = f6;
	eval->feature.v8[7] = f7;
  #endif
  #endif
}

#endif // hasSSE2
