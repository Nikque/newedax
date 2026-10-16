/**
 * @file evtool.hpp
 *
 * Common parts of the evaluation tools: training records, data sources, eval.dat files of form 1, and the 46 features
 * of Edax 4.5 in the side-to-move view, packed as in src/eval/eval.cpp (patterns C9 x4, C10 x4, S100 x4, S101 x4,
 * S8 x16, S7654 x12, S0).
 */
#pragma once

#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifdef _MSC_VER
	#define EVTOOL_NOINLINE __declspec(noinline)
#else
	#define EVTOOL_NOINLINE __attribute__((noinline))
#endif

namespace evtool {

/** print a message to stderr and exit with 1 */
[[noreturn]] inline void die(const char *message, const char *what = "")
{
	fprintf(stderr, message, what);
	fputc('\n', stderr);
	exit(1);
}

/** allocation that exits with "out of memory" when it fails */
inline void *xmalloc(size_t size)
{
	void *p = malloc(size ? size : 1);
	if (p == nullptr) die("out of memory");
	return p;
}

inline void *xcalloc(size_t n, size_t size)
{
	void *p = calloc(n ? n : 1, size ? size : 1);
	if (p == nullptr) die("out of memory");
	return p;
}

inline void *xrealloc(void *old, size_t size)
{
	void *p = realloc(old, size ? size : 1);
	if (p == nullptr) die("out of memory");
	return p;
}

/** fopen that exits with "cannot open <file>" (reading) or "cannot create <file>" (writing) when it fails */
inline FILE *xfopen(const char *file, const char *mode)
{
	FILE *f = fopen(file, mode);
	if (f == nullptr) die(mode[0] == 'r' ? "cannot open %s" : "cannot create %s", file);
	return f;
}

/** fread of exactly n items, or exit with "cannot read <file>" */
inline void xfread(void *p, size_t size, size_t n, FILE *f, const char *file)
{
	if (fread(p, size, n, f) != n) die("cannot read %s (file too short)", file);
}

/** the number of records of a growing array that can still be doubled (the counts are int) */
inline int grow(int cap, int first)
{
	if (cap == 0) return first;
	if (cap > INT_MAX / 2) {
		if (cap == INT_MAX) die("too many records (more than %s) for a ply", "2^31 - 1");
		return INT_MAX;
	}
	return 2 * cap;
}

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

/** length of a leading drive letter ("C:\", "C:/") of a path, else 0 */
inline int drive_length(const char *s)
{
	return (isalpha((unsigned char) s[0]) && s[1] == ':' && (s[2] == '\\' || s[2] == '/')) ? 2 : 0;
}

/**
 * @brief Read a data source "prefix:dir:weight[:max]"; dir may begin with a drive letter ("D:\data").
 * @return the number of fields read (as sscanf: 3 or 4 for a whole source).
 */
inline int source_parse(const char *s, Source *o)
{
	const char *c = strchr(s, ':');
	if (c == nullptr || c == s || c - s > 31) return 0;
	memcpy(o->prefix, s, c - s); o->prefix[c - s] = '\0';
	const char *d = c + 1, *e = strchr(d + drive_length(d), ':');
	if (e == nullptr || e == d || e - d > 511) return 1;
	memcpy(o->dir, d, e - d); o->dir[e - d] = '\0';
	const int n = sscanf(e + 1, "%lf:%lld", &o->weight, &o->max);
	return 2 + (n > 0 ? n : 0);
}

/**
 * @brief Read "dir:name" (the name has no ':'; dir may begin with a drive letter).
 * @return true if both are read.
 */
inline bool dir_name_parse(const char *s, char *dir, size_t dir_size, char *name, size_t name_size)
{
	const char *c = strrchr(s, ':');
	if (c == nullptr || c == s || c[1] == '\0' || (size_t) (c - s) >= dir_size || strlen(c + 1) >= name_size || (c == s + 1 && drive_length(s))) return false;
	memcpy(dir, s, c - s); dir[c - s] = '\0';
	strcpy(name, c + 1);
	return true;
}

/** CRC-32C (Castagnoli, reflected, as crc32c_bytes of src/util): crc of the bytes so far (0 at the start) */
inline uint32_t crc32c_update(uint32_t crc, const unsigned char *p, size_t n)
{
	static uint32_t table[256];
	if (table[1] == 0) {
		for (uint32_t i = 0; i < 256; ++i) {
			uint32_t c = i;
			for (int k = 0; k < 8; ++k) c = c & 1 ? (c >> 1) ^ 0x82F63B78U : c >> 1;
			table[i] = c;
		}
	}
	crc = ~crc;
	for (size_t i = 0; i < n; ++i) crc = table[(crc ^ p[i]) & 0xff] ^ (crc >> 8);
	return ~crc;
}

/** a position of a book file (the fields the tools read) */
struct BookRecord {
	uint64_t P, O;                  // player, opponent
	int value, lower, upper;        // score of the position and its bounds
	int n_link, level;              // number of moves, level of the search
	int leaf_score, leaf_move;      // best move not in the links
};

/**
 * Reading the positions of a book file of form 1 (Edax 4, nikque) or form 2 ("NEDXBOOK", docs/port/DESIGN-format.md)
 * one after the other. A file of form 2 is checked: its header CRC, the number of positions, the trailer (size and
 * CRC of the positions, "BEND") and nothing after it.
 */
struct BookReader {
	FILE *f;
	const char *file;
	unsigned char *buf;             // the file is read in this buffer, 16 MB at a time
	size_t pos, n;                  // next byte, bytes in the buffer
	int form;                       // 1 or 2
	uint64_t n_positions, read;
	int level, n_empties;           // settings of the book (header)
	uint32_t crc;                   // form 2: CRC of the positions read so far
	uint64_t bytes;                 // form 2: bytes of the positions read so far
};

enum { BOOK_READER_BUFFER = 1 << 24 };

/** the next k bytes of the file (k at most 4096) in the buffer, or fewer at the end of the file */
inline const unsigned char *book_reader_fill(BookReader *r, size_t k)
{
	if (r->n - r->pos < k) {	// (the rare case: the buffer is filled again)
		memmove(r->buf, r->buf + r->pos, r->n - r->pos);
		r->n -= r->pos; r->pos = 0;
		r->n += fread(r->buf + r->n, 1, BOOK_READER_BUFFER - r->n, r->f);
	}
	return r->buf + r->pos;
}

/** the next k bytes of the file (k at most 4096) in the buffer, or exit if the file ends before them */
inline const unsigned char *book_reader_need(BookReader *r, size_t k)
{
	const unsigned char *p = book_reader_fill(r, k);
	if (r->n - r->pos < k) die("%s is truncated", r->file);
	return p;
}

/** k bytes of the positions read (form 2: counted in the CRC) */
inline void book_reader_take(BookReader *r, size_t k)
{
	if (r->form == 2) { r->crc = crc32c_update(r->crc, r->buf + r->pos, k); r->bytes += k; }
	r->pos += k;
}

inline void book_reader_open(BookReader *r, const char *file)
{
	memset(r, 0, sizeof *r);
	r->file = file;
	r->f = xfopen(file, "rb");
	setvbuf(r->f, nullptr, _IONBF, 0);	// (read in BookReader::buf)
	r->buf = (unsigned char *) xmalloc(BOOK_READER_BUFFER);
	const unsigned char *h = book_reader_need(r, 8);
	if (!memcmp(h, "XADEKOOB", 8)) {	// form 1: version, release, date (8), options (20), number of positions (4)
		h = book_reader_need(r, 42);
		uint32_t n;
		memcpy(&n, h + 38, 4);
		r->form = 1; r->n_positions = n;
		r->level = h[18]; r->n_empties = h[22];
		r->pos += 42;
	} else if (!memcmp(h, "NEDXBOOK", 8)) {	// form 2: header of header_size bytes (64 known)
		h = book_reader_need(r, 12);
		uint16_t form, size;
		memcpy(&form, h + 8, 2); memcpy(&size, h + 10, 2);
		if (form != 2 || size < 64 || size > 4096) die("%s is a book of another form", file);
		h = book_reader_need(r, size);
		uint32_t crc, flags;
		memcpy(&crc, h + size - 4, 4); memcpy(&flags, h + 12, 4);
		if (crc32c_update(0, h, size - 4) != crc) die("%s has a damaged header", file);
		if (flags != 0 || h[18] != 1) die("%s is a book of a later form", file);
		int32_t level, n_empties;
		memcpy(&level, h + 28, 4); memcpy(&n_empties, h + 32, 4);
		memcpy(&r->n_positions, h + 48, 8);
		r->form = 2; r->level = level; r->n_empties = n_empties;
		r->pos += size;
	} else die("%s is not a book", file);
}

/** the next position, or false after the last one (then a file of form 2 has been checked to its end) */
inline bool book_reader_next(BookReader *r, BookRecord *p)
{
	if (r->read == r->n_positions) {
		if (r->form == 2) {
			const unsigned char *t = book_reader_need(r, 16);
			uint64_t bytes;
			uint32_t crc;
			memcpy(&bytes, t, 8); memcpy(&crc, t + 8, 4);
			r->pos += 16;
			if (bytes != r->bytes || crc != r->crc || memcmp(t + 12, "BEND", 4) || r->pos != r->n || fgetc(r->f) != EOF)
				die("%s is damaged (the check of the positions fails)", r->file);
		}
		return false;
	}
	++r->read;
	if (r->form == 1) {	// board (16), 4 counts (16), value, lower, upper (2 each), n_link, level; links; leaf
		const unsigned char *h = book_reader_fill(r, 40 + 2 * 256);	// (at most: the file may end before)
		if (r->n - r->pos < 40 || r->n - r->pos < (size_t) 40 + 2 * (h[38] + 1)) die("%s is truncated", r->file);
		int16_t v[3];
		memcpy(&p->P, h, 8); memcpy(&p->O, h + 8, 8); memcpy(v, h + 32, 6);
		p->value = v[0]; p->lower = v[1]; p->upper = v[2];
		p->n_link = h[38]; p->level = h[39];
		p->leaf_score = (signed char) h[40 + 2 * p->n_link]; p->leaf_move = h[40 + 2 * p->n_link + 1];
		book_reader_take(r, 40 + 2 * (p->n_link + 1));
	} else {	// board (16), n_link, level, value, lower, upper, leaf score, leaf move; 4 counts (LEB128); links
		const unsigned char *h = book_reader_fill(r, 23 + 4 * 5 + 2 * 255);	// (at most: the file may end before)
		const size_t left = r->n - r->pos;
		if (left < 23 + 4) die("%s is truncated", r->file);
		memcpy(&p->P, h, 8); memcpy(&p->O, h + 8, 8);
		p->n_link = h[16]; p->level = h[17];
		p->value = (signed char) h[18]; p->lower = (signed char) h[19]; p->upper = (signed char) h[20];
		p->leaf_score = (signed char) h[21]; p->leaf_move = h[22];
		size_t k = 23;
		for (int i = 0; i < 4; ++i) {
			int b = 0;
			do {
				if (k >= left) die("%s is truncated", r->file);
				if (++b > 5) die("%s is damaged (a count of more than 32 bits)", r->file);
			} while (h[k++] & 0x80);
		}
		k += 2 * p->n_link;
		if (k > left) die("%s is truncated", r->file);
		book_reader_take(r, k);
	}
	return true;
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
	if (r >= 2 && !(e->edax == 0x45444158 && e->eval == 0x4556414c)) {	// "EDAX" "EVAL"
		if (!memcmp(&e->edax, "NEDXEVAL", 8)) die("%s is an evaluation file of form 2: convert it to form 1 (edax -eval-convert)", file);
		die("%s is not an evaluation file of form 1", file);
	}
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
	if (fclose(f) != 0) die("cannot write %s", file);
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
