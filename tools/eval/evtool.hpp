/**
 * @file evtool.hpp
 *
 * Common parts of the evaluation tools: training records, data sources, eval.dat files of form 1, and the 46 features
 * of Edax 4.5 in the side-to-move view, packed as in src/eval/eval.cpp (patterns C9 x4, C10 x4, S100 x4, S101 x4,
 * S8 x16, S7654 x12, S0).
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace evtool {

#ifdef _MSC_VER
inline int popc(uint64_t x) { return (int) __popcnt64(x); }
#else
inline int popc(uint64_t x) { return __builtin_popcountll(x); }
#endif

/** training record: player's discs, opponent's discs (side to move), value (score of the side to move, or another label) */
#pragma pack(push, 1)
struct Rec { uint64_t P, O; signed char v; };
#pragma pack(pop)
static_assert(sizeof (Rec) == 17, "17-byte records");

/** hash used to split the records into training and test samples */
inline uint64_t mix64(uint64_t x)
{
	x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL; x ^= x >> 27; x *= 0x94d049bb133111ebULL; x ^= x >> 31;
	return x;
}

/** size of an open file in bytes; the file is left at its start */
inline long long file_size(FILE *f)
{
#ifdef _WIN32
	_fseeki64(f, 0, SEEK_END);
	const long long n = _ftelli64(f);
	_fseeki64(f, 0, SEEK_SET);
#else
	fseeko(f, 0, SEEK_END);
	const long long n = ftello(f);
	fseeko(f, 0, SEEK_SET);
#endif
	return n;
}

/** data source: files <dir>/<prefix>NN.bin, weight of their samples, at most max training samples per ply */
struct Source { char prefix[32], dir[512]; double weight; long long max; };

/**
 * @brief Read a data source "prefix:dir:weight[:max]".
 * @return the number of fields read (as sscanf).
 */
inline int source_parse(const char *s, Source *o)
{
	return sscanf(s, "%31[^:]:%511[^:]:%lf:%lld", o->prefix, o->dir, &o->weight, &o->max);
}

enum { ET_NW = 114364, ET_NF = 46, ET_NPLY = 61 };

/** square index 0 = A1, 1 = B1, ..., 63 = H8 (as src/core/const.h) */
enum {
	A1, B1, C1, D1, E1, F1, G1, H1, A2, B2, C2, D2, E2, F2, G2, H2, A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4, A5, B5, C5, D5, E5, F5, G5, H5, A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7, A8, B8, C8, D8, E8, F8, G8, H8
};

struct ET_F2X { int n; unsigned char x[10]; };
inline constexpr ET_F2X ET_FEAT[ET_NF] = {
	{ 9, {A1, B1, A2, B2, C1, A3, C2, B3, C3}},
	{ 9, {H1, G1, H2, G2, F1, H3, F2, G3, F3}},
	{ 9, {A8, A7, B8, B7, A6, C8, B6, C7, C6}},
	{ 9, {H8, H7, G8, G7, H6, F8, G6, F7, F6}},
	{10, {A5, A4, A3, A2, A1, B2, B1, C1, D1, E1}},
	{10, {H5, H4, H3, H2, H1, G2, G1, F1, E1, D1}},
	{10, {A4, A5, A6, A7, A8, B7, B8, C8, D8, E8}},
	{10, {H4, H5, H6, H7, H8, G7, G8, F8, E8, D8}},
	{10, {B2, A1, B1, C1, D1, E1, F1, G1, H1, G2}},
	{10, {B7, A8, B8, C8, D8, E8, F8, G8, H8, G7}},
	{10, {B2, A1, A2, A3, A4, A5, A6, A7, A8, B7}},
	{10, {G2, H1, H2, H3, H4, H5, H6, H7, H8, G7}},
	{10, {A1, C1, D1, C2, D2, E2, F2, E1, F1, H1}},
	{10, {A8, C8, D8, C7, D7, E7, F7, E8, F8, H8}},
	{10, {A1, A3, A4, B3, B4, B5, B6, A5, A6, A8}},
	{10, {H1, H3, H4, G3, G4, G5, G6, H5, H6, H8}},
	{ 8, {A2, B2, C2, D2, E2, F2, G2, H2}},
	{ 8, {A7, B7, C7, D7, E7, F7, G7, H7}},
	{ 8, {B1, B2, B3, B4, B5, B6, B7, B8}},
	{ 8, {G1, G2, G3, G4, G5, G6, G7, G8}},
	{ 8, {A3, B3, C3, D3, E3, F3, G3, H3}},
	{ 8, {A6, B6, C6, D6, E6, F6, G6, H6}},
	{ 8, {C1, C2, C3, C4, C5, C6, C7, C8}},
	{ 8, {F1, F2, F3, F4, F5, F6, F7, F8}},
	{ 8, {A4, B4, C4, D4, E4, F4, G4, H4}},
	{ 8, {A5, B5, C5, D5, E5, F5, G5, H5}},
	{ 8, {D1, D2, D3, D4, D5, D6, D7, D8}},
	{ 8, {E1, E2, E3, E4, E5, E6, E7, E8}},
	{ 8, {A1, B2, C3, D4, E5, F6, G7, H8}},
	{ 8, {A8, B7, C6, D5, E4, F3, G2, H1}},
	{ 7, {B1, C2, D3, E4, F5, G6, H7}},
	{ 7, {H2, G3, F4, E5, D6, C7, B8}},
	{ 7, {A2, B3, C4, D5, E6, F7, G8}},
	{ 7, {G1, F2, E3, D4, C5, B6, A7}},
	{ 6, {C1, D2, E3, F4, G5, H6}},
	{ 6, {A3, B4, C5, D6, E7, F8}},
	{ 6, {F1, E2, D3, C4, B5, A6}},
	{ 6, {H3, G4, F5, E6, D7, C8}},
	{ 5, {D1, E2, F3, G4, H5}},
	{ 5, {A4, B5, C6, D7, E8}},
	{ 5, {E1, D2, C3, B4, A5}},
	{ 5, {H4, G5, F6, E7, D8}},
	{ 4, {D1, C2, B3, A4}},
	{ 4, {A5, B6, C7, D8}},
	{ 4, {E1, F2, G3, H4}},
	{ 4, {H5, G6, F7, E8}},
};

/** packed class offsets: C9, C10, S100, S101, S8 (4 groups), S7, S6, S5, S4, S0 */
inline constexpr int ET_POFS[13] = { 0, 10206, 40095, 69741, 99387, 102708, 106029, 109350, 112671, 113805, 114183, 114318, 114363 };
inline constexpr int ET_PSIZE[13] = { 10206, 29889, 29646, 29646, 3321, 3321, 3321, 3321, 1134, 378, 135, 45, 1 };
/** packed class of each feature */
inline constexpr unsigned char ET_CLASS[ET_NF] = {
	0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4, 5,5,5,5, 6,6,6,6, 7,7, 8,8,8,8, 9,9,9,9, 10,10,10,10, 11,11,11,11 };

/** packing tables: raw pattern index (digit 0 = side to move, 1 = opponent, 2 = empty) -> packed index within its class */
inline short ET_P_C9[19683], ET_P_C10[59049], ET_P_S10[59049], ET_P_S8[6561], ET_P_S7[2187], ET_P_S6[729], ET_P_S5[243], ET_P_S4[81];
inline short *ET_PACK[12];

inline int et_set_packing(short *pe, int *T, const int *kd, int l, int k, int n, int d)
{
	int i = 0, q0, q1, q2, q3;
	if (--d > 3) {
		l *= 3;
		n = et_set_packing(pe, T, kd, l, k, n, d);
		k += kd[d];
		n = et_set_packing(pe, T, kd, l + 3, k, n, d);
		k += kd[d];
		n = et_set_packing(pe, T, kd, l + 6, k, n, d);
	} else {
		l *= 27;
		for (q3 = 0; q3 < 3; ++q3) {
			for (q2 = 0; q2 < 3; ++q2) {
				for (q1 = 0; q1 < 3; ++q1) {
					for (q0 = 0; q0 < 3; ++q0) {
						if (k < l) i = T[k];
						else T[l] = i = n++;
						pe[l++] = (short) i;
						k += kd[0];
					}
					k += (kd[1] - kd[0] * 3);
				}
				k += (kd[2] - kd[1] * 3);
			}
			k += (kd[3] - kd[2] * 3);
		}
	}
	return n;
}

inline void et_init()
{
	static const int kd_S10[] = { 19683, 6561, 2187, 729, 243, 81, 27, 9, 3, 1 };
	static const int kd_C10[] = { 19683, 6561, 2187, 729, 81, 243, 27, 9, 3, 1 };
	static const int kd_C9[] = { 1, 9, 3, 81, 27, 243, 2187, 729, 6561 };
	int *T = (int *) malloc(2 * 59049 * sizeof *T), n[8];
	if (T == nullptr) { fprintf(stderr, "out of memory\n"); exit(1); }
	n[0] = et_set_packing(ET_P_S8, T, kd_S10 + 2, 0, 0, 0, 8);
	n[1] = et_set_packing(ET_P_S7, T, kd_S10 + 3, 0, 0, 0, 7);
	n[2] = et_set_packing(ET_P_S6, T, kd_S10 + 4, 0, 0, 0, 6);
	n[3] = et_set_packing(ET_P_S5, T, kd_S10 + 5, 0, 0, 0, 5);
	n[4] = et_set_packing(ET_P_S4, T, kd_S10 + 6, 0, 0, 0, 4);
	n[5] = et_set_packing(ET_P_C9, T, kd_C9, 0, 0, 0, 9);
	n[6] = et_set_packing(ET_P_S10, T, kd_S10, 0, 0, 0, 10);
	n[7] = et_set_packing(ET_P_C10, T, kd_C10, 0, 0, 0, 10);
	free(T);
	if (n[0] != 3321 || n[1] != 1134 || n[2] != 378 || n[3] != 135 || n[4] != 45 || n[5] != 10206 || n[6] != 29646 || n[7] != 29889) {
		fprintf(stderr, "packing size mismatch\n"); exit(1);
	}
	ET_PACK[0] = ET_P_C9; ET_PACK[1] = ET_P_C10; ET_PACK[2] = ET_P_S10; ET_PACK[3] = ET_P_S10;
	ET_PACK[4] = ET_PACK[5] = ET_PACK[6] = ET_PACK[7] = ET_P_S8;
	ET_PACK[8] = ET_P_S7; ET_PACK[9] = ET_P_S6; ET_PACK[10] = ET_P_S5; ET_PACK[11] = ET_P_S4;
}

/** packed weight indices of the 46 features (the constant S0 is index ET_NW - 1) */
inline void et_features(uint64_t P, uint64_t O, int *idx)
{
	for (int i = 0; i < ET_NF; ++i) {
		const ET_F2X *f = ET_FEAT + i;
		int x = 0;
		for (int j = 0; j < f->n; ++j) {
			const int s = f->x[j];
			x = x * 3 + ((P >> s) & 1 ? 0 : (O >> s) & 1 ? 1 : 2);
		}
		const int c = ET_CLASS[i];
		idx[i] = ET_PACK[c][x] + ET_POFS[c];
	}
}

/** eval.dat of form 1: header (5 ints, a double), then the packed weights of 61 plies */
struct ET_Eval {
	unsigned int edax, eval, version, release, build;
	double date;
	short w[ET_NPLY][ET_NW];
};

/** read an eval.dat of form 1 (at least 54 plies; the missing plies are copies of the last one) */
inline ET_Eval *et_read(const char *file)
{
	FILE *f = fopen(file, "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", file); exit(1); }
	ET_Eval *e = (ET_Eval *) calloc(1, sizeof *e);
	if (e == nullptr) { fprintf(stderr, "out of memory\n"); exit(1); }
	size_t r = fread(&e->edax, 4, 5, f);
	r += fread(&e->date, 8, 1, f);
	int np = 0;
	while (np < ET_NPLY && fread(e->w[np], 2, ET_NW, f) == ET_NW) ++np;
	fclose(f);
	if (r != 6 || np < 54) { fprintf(stderr, "bad eval file %s (%d plies)\n", file, np); exit(1); }
	for (; np < ET_NPLY; ++np) memcpy(e->w[np], e->w[np - 1], sizeof e->w[0]);
	return e;
}

inline void et_write(const ET_Eval *e, const char *file)
{
	FILE *f = fopen(file, "wb");
	if (!f) { fprintf(stderr, "cannot create %s\n", file); exit(1); }
	fwrite(&e->edax, 4, 5, f); fwrite(&e->date, 8, 1, f);
	fwrite(e->w, 2, (size_t) ET_NW * ET_NPLY, f);
	fclose(f);
}

/** Edax's weight ply for a position with n_empties (accumlate_eval: plies 0/1 use 2/3, >= 54 use 52/53) */
inline int et_ply(int n_empties)
{
	int ply = 60 - n_empties;
	if (ply < 2) ply += 2;
	while (ply > 53) ply -= 2;
	return ply;
}

/** raw sum (score * 128) as in accumlate_eval */
inline int et_sum(const short *w, const int *idx)
{
	int s = w[ET_NW - 1];
	for (int i = 0; i < ET_NF; ++i) s += w[idx[i]];
	return s;
}

/** search_eval_0 rounding and bounds */
inline int et_score(int sum)
{
	if (sum > 0) sum += 64; else sum -= 64;
	sum /= 128;
	if (sum < -63) sum = -63;
	if (sum > 63) sum = 63;
	return sum;
}

} // namespace evtool
