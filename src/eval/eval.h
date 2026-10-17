/**
 * @file eval.h
 *
 * Evaluation function's header.
 *
 * @date 1998 - 2025
 * @author Richard Delorme
 * @version 4.5
 */

#ifndef EDAX_EVAL_H
#define EDAX_EVAL_H

#include "core/bit.h"

/**
 * Feature slots: 48 (the 47 features of Edax 4.5, the default) or 64 (EVAL_SLOTS=64: 16 more features,
 * slots 48 to 63, see eval_feature64.h). The features fill whole vectors: 3 or 4 of 256 bits.
 */
#ifndef EVAL_SLOTS
	#define EVAL_SLOTS 48
#endif
#if EVAL_SLOTS != 48 && EVAL_SLOTS != 64
	#error "EVAL_SLOTS is 48 or 64"
#endif
#if EVAL_SLOTS > 48 && !defined(hasSSE2) && !defined(__ARM_NEON) && !defined(VECTOR_EVAL_UPDATE)
	#define VECTOR_EVAL_UPDATE	// (the update square by square of eval.cpp knows the 47 features of Edax 4.5 only)
#endif

/**
 * Mobility weights (EVAL_MOBILITY=1, as v4.5.5-eval2): the weights of the numbers of legal moves (0 to 31) of the side
 * to move and of the opponent, for each ply, are read from the end of an evaluation file of form 1 ("MOBW", then 61 x 64
 * shorts, or the mobility group of form 2) and added to the evaluation. Without them in the file, the evaluation is the
 * one of Edax 4.5. 1 is the default (bin/data/eval.dat is the one of eval2). 0: they are not read (a file of form 1
 * is read without them, a file of form 2 with them is refused).
 */
#ifndef EVAL_MOBILITY
	#define EVAL_MOBILITY 1
#endif

/** number of features */
enum { EVAL_N_FEATURE = 47 };

/**
 * struct Eval
 * @brief evaluation function
 */
typedef union {
	unsigned short us[EVAL_SLOTS];
	unsigned long long ull[EVAL_SLOTS / 4];	// SWAR
#ifdef __ARM_NEON
	int16x8_t v8[EVAL_SLOTS / 8];
#elif defined(hasSSE2) || defined(USE_MSVC_X86)
	__m128i	v8[EVAL_SLOTS / 8];
#endif
#ifdef __AVX2__
	__m256i	v16[EVAL_SLOTS / 16];
#endif
} EVAL_FEATURE_V;

/* defined in eval.cpp; declared here so that they are visible from the other translation units (eval_sse.cpp on Android) */
extern const EVAL_FEATURE_V EVAL_FEATURE[65];
extern const EVAL_FEATURE_V EVAL_FEATURE_all_opponent;

typedef struct Eval {
	EVAL_FEATURE_V feature;                       /**!< discs' features (96, or 128 with 64 slots) */
	int n_empties;                                /**< number of empty squares (4) */
	unsigned int parity;                          /**< parity (4) */
} Eval;

struct Board;
struct Move;

/** unpacked weights */
// enum { EVAL_N_WEIGHT = 226315 };
// [0] for even ply, [1] for odd, interleaved for aligned VPGATHERDD access (4.5.5)
typedef struct Eval_weight {
	short	S0[2];
	short	C9[19683][2];
	short	C10[59049][2];
	short	S100[59049][2];
	short	S101[59049][2];
	short	S8x4[6561*4][2];
	short	S7654[2187+729+243+81][2];
#if EVAL_SLOTS > 48
	short	X[4][59049][2];	// extra groups of 10 squares (slots 48-63, 4 places each)
#endif
} Eval_weight;

/** number of plies */
enum { EVAL_N_2PLY = 54 / 2 };	// decreased from 60 in 4.5.1

extern Eval_weight (*EVAL_WEIGHT)[EVAL_N_2PLY - 1];	// for 2..53, interleaved

#if EVAL_MOBILITY
extern short EVAL_MOBILITY_WEIGHT[61][64];	// for each ply: side to move [0..31], opponent [32..63]
extern bool EVAL_HAS_MOBILITY;	// the evaluation file has mobility weights
#endif

/* function declaration */
void eval_open(const char*);
void eval_close(void);
bool eval_convert(const char*, const char*);
// void eval_init(Eval*);
// void eval_free(Eval*);
void eval_set(Eval*, const struct Board*);
void eval_restore(Eval*, const struct Move*);
void eval_pass(Eval*);
double eval_sigma(const int, const int, const int);
double probcut_sigma(const int, const int, const int);

#if defined(hasSSE2) || defined(__ARM_NEON) || defined(USE_MSVC_X86) || defined(ANDROID)
void eval_update_sse(int, unsigned long long, Eval *, const Eval *);
#endif
#if defined(hasSSE2) || defined(__ARM_NEON)
#define	eval_update(x, f, eval)	eval_update_sse(x, f, eval, eval)
#define	eval_update_leaf(x, f, eval_out, eval_in)	eval_update_sse(x, f, eval_out, eval_in)
#else
void eval_update(int, unsigned long long, Eval*);
void eval_update_leaf(int, unsigned long long, Eval*, const Eval*);
#endif

#endif

