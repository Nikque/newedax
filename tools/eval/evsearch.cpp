/**
 * @file evsearch.cpp
 *
 * Fast search of evaluation feature sets.
 * Loads training/test data of some plies once, then trains (from zero, regularized least squares, CG) and tests
 * many feature configurations read from a file.
 *
 * usage: evsearch -lib patterns.txt -configs configs.txt -plies 30,38,46 -src prefix:dir:weight[:max] ... [-iter n] [-lambda l] [-threads n]
 *   patterns.txt : lines "name n sq1 sq2 ... sqn" (squares as a1..h8); every pattern is used with its distinct
 *                  symmetric instances (8 symmetries, deduplicated by square set), sharing one weight table.
 *   configs.txt  : lines "config_name item item ..." where item = pattern name or a scalar feature:
 *                  mob (mobility of both sides), mobp, cmob (corner moves of both sides), qpar (quadrant parity),
 *                  stab (stable discs of both sides, edges + full lines), front (frontier discs), disc,
 *                  estab (edge-stable discs), mobhv, mobd (mobility along some directions), mob2d (joint table of both mobilities)
 *   output       : "config_name lanes table_entries | ply source mse ..." one line per configuration
 *   other options: -lambda-prior l, -prior eval.dat, -teacher eval.dat -alpha a, -base dir:cfg, -dump dir, -notie,
 *                  -window k -decay d (training samples also from the plies p +- 2, ..., p +- 2k, weighted by d^j)
 */
#include <cmath>
#include <omp.h>

#include "evtool.hpp"

using namespace evtool;

// ---------------- pattern library ----------------
struct Pattern { char name[32]; int n; unsigned char sq[16]; };
static Pattern lib[8192];
static int n_lib;

static int sym(int x, int s)
{
	int r = x >> 3, c = x & 7, t;
	if (s & 1) c = 7 - c;
	if (s & 2) r = 7 - r;
	if (s & 4) { t = r; r = c; c = t; }
	return r * 8 + c;
}

static void read_lib(const char *file)
{
	if (file == nullptr) die("missing -lib");
	FILE *f = fopen(file, "r");
	char line[1024], copy[1024];
	if (!f) { fprintf(stderr, "cannot open %s\n", file); exit(1); }
	while (fgets(line, sizeof line, f)) {
		strcpy(copy, line);
		char *tok = strtok(line, " \t\r\n");
		if (!tok || tok[0] == '#') continue;
		if (n_lib == (int) (sizeof lib / sizeof lib[0])) die("too many patterns in the library (at most 8192)");
		Pattern *p = lib + n_lib;
		strncpy(p->name, tok, 31);
		const char *n = strtok(nullptr, " \t\r\n");
		p->n = n ? atoi(n) : 0;
		if (p->n < 1 || p->n > 16) die("bad pattern line (1 to 16 squares): %s", copy);
		uint64_t used = 0;
		for (int i = 0; i < p->n; ++i) {
			char *s = strtok(nullptr, " \t\r\n");
			if (s == nullptr || s[0] < 'a' || s[0] > 'h' || s[1] < '1' || s[1] > '8' || s[2] != '\0') die("bad pattern line (squares a1 to h8): %s", copy);
			p->sq[i] = (unsigned char) ((s[0] - 'a') + 8 * (s[1] - '1'));
			if (used >> p->sq[i] & 1) die("bad pattern line (a square twice): %s", copy);
			used |= 1ULL << p->sq[i];
		}
		++n_lib;
	}
	fclose(f);
}

// Canonical index of a pattern configuration under the board symmetries that map the pattern's square set
// onto itself (e.g. the mirror of an edge pattern): the weights of symmetric configurations are shared, as in Edax.
static int *lib_canon[8192];
static int tie = 1;	// -notie: no sharing

static const int *canon_table(int p)
{
	if (lib_canon[p]) return lib_canon[p];
	const int n = lib[p].n;
	int size = 1;
	for (int i = 0; i < n; ++i) size *= 3;
	int *t = (int *) xmalloc(size * sizeof *t);
	for (int x = 0; x < size; ++x) t[x] = x;
	uint64_t set = 0;
	for (int i = 0; i < n; ++i) set |= 1ULL << lib[p].sq[i];
	for (int sy = 1; sy < 8 && tie; ++sy) {
		uint64_t img = 0;
		int perm[16];	// position i goes to position perm[i]
		for (int i = 0; i < n; ++i) img |= 1ULL << sym(lib[p].sq[i], sy);
		if (img != set) continue;
		for (int i = 0; i < n; ++i) { int j = 0; while (lib[p].sq[j] != sym(lib[p].sq[i], sy)) ++j; perm[i] = j; }
		for (int x = 0; x < size; ++x) {
			int d[16], y = x, xi = 0;
			for (int i = n - 1; i >= 0; --i) { d[i] = y % 3; y /= 3; }
			int e[16];
			for (int i = 0; i < n; ++i) e[perm[i]] = d[i];
			for (int i = 0; i < n; ++i) xi = xi * 3 + e[i];
			int ra = x, rb = xi;	// union of the two classes, the smaller index is the root
			while (t[ra] != ra) ra = t[ra];
			while (t[rb] != rb) rb = t[rb];
			if (ra < rb) t[rb] = ra; else t[ra] = rb;
		}
	}
	for (int x = 0; x < size; ++x) { int r = x; while (t[r] != r) r = t[r]; t[x] = r; }
	return lib_canon[p] = t;
}

// ---------------- scalar features ----------------
// The scalar features and features() (the hot code of the feature indices) are in the code section ".text$a1", at the
// start of the code: their place, and the time they take, do not depend on the size of the other code.
#ifdef _MSC_VER
#pragma code_seg(".text$a1")
#endif
static uint64_t get_moves(uint64_t P, uint64_t O)
{
	uint64_t moves = 0, mO = O & 0x7e7e7e7e7e7e7e7eULL, flip, pre;
	const int dir[4] = { 1, 7, 8, 9 };
	const uint64_t mask[4] = { mO, mO, O, mO };
	for (int d = 0; d < 4; ++d) {
		const int s = dir[d]; const uint64_t m = mask[d];
		flip = m & (P << s); flip |= m & (flip << s); pre = m & (m << s);
		flip |= pre & (flip << 2 * s); flip |= pre & (flip << 2 * s);
		moves |= flip << s;
		flip = m & (P >> s); flip |= m & (flip >> s); pre = m & (m >> s);
		flip |= pre & (flip >> 2 * s); flip |= pre & (flip >> 2 * s);
		moves |= flip >> s;
	}
	return moves & ~(P | O);
}

static uint64_t get_moves_dirs(uint64_t P, uint64_t O, int d0, int d1)
{	// moves along the directions dir[d0..d1] only (0: horizontal, 1: diagonal 7, 2: vertical, 3: diagonal 9)
	uint64_t moves = 0, mO = O & 0x7e7e7e7e7e7e7e7eULL, flip, pre;
	const int dir[4] = { 1, 7, 8, 9 };
	const uint64_t mask[4] = { mO, mO, O, mO };
	for (int d = d0; d <= d1; ++d) {
		if (d0 == 0 && d1 == 2 && d == 1) continue;	// horizontal + vertical
		const int s = dir[d]; const uint64_t m = mask[d];
		flip = m & (P << s); flip |= m & (flip << s); pre = m & (m << s);
		flip |= pre & (flip << 2 * s); flip |= pre & (flip << 2 * s);
		moves |= flip << s;
		flip = m & (P >> s); flip |= m & (flip >> s); pre = m & (m >> s);
		flip |= pre & (flip >> 2 * s); flip |= pre & (flip >> 2 * s);
		moves |= flip >> s;
	}
	return moves & ~(P | O);
}

static uint64_t neighbours(uint64_t b)
{
	const uint64_t l = b & 0xfefefefefefefefeULL, r = b & 0x7f7f7f7f7f7f7f7fULL;
	return (b << 8) | (b >> 8) | (l >> 1) | (l << 7) | (l >> 9) | (r << 1) | (r >> 7) | (r << 9);
}

// full lines in the 4 directions (occupied squares whose line is completely filled)
static uint64_t full_h(uint64_t f) { f &= f >> 1; f &= f >> 2; f &= f >> 4; f &= 0x0101010101010101ULL; return f * 0xff; }
static uint64_t full_v(uint64_t f) { f &= (f >> 8) | (f << 56); f &= (f >> 16) | (f << 48); f &= (f >> 32) | (f << 32); return f; }
static uint64_t full_d(uint64_t f, int s)
{	// along direction s (7 or 9): a square is full if the whole diagonal is filled
	uint64_t full = 0;
	for (int x = 0; x < 64; ++x) {
		if (!((f >> x) & 1)) continue;
		int ok = 1;
		const int c = x & 7, rr = x >> 3;
		const int dc = (s == 9) ? 1 : -1;
		for (int k = 1; ok; ++k) { const int cc = c + dc * k, r2 = rr + k; if (cc < 0 || cc > 7 || r2 > 7) break; if (!((f >> (r2 * 8 + cc)) & 1)) ok = 0; }
		for (int k = 1; ok; ++k) { const int cc = c - dc * k, r2 = rr - k; if (cc < 0 || cc > 7 || r2 < 0) break; if (!((f >> (r2 * 8 + cc)) & 1)) ok = 0; }
		if (ok) full |= 1ULL << x;
	}
	return full;
}

// stable discs (approximation): corners grown along edges, plus discs on 4 full lines, grown from stable neighbours
static uint64_t stable_discs(uint64_t P, uint64_t O)
{
	const uint64_t occ = P | O;
	const uint64_t fh = full_h(occ), fv = full_v(occ), f9 = full_d(occ, 9), f7 = full_d(occ, 7);
	uint64_t st = (P & 0x8100000000000081ULL) | (P & fh & fv & f9 & f7), old;
	do {	// a disc is stable if, in each of the 4 axes, one neighbour is a stable disc or the border, or the line is full
		old = st;
		const uint64_t sh = (st << 1 & 0xfefefefefefefefeULL) | (st >> 1 & 0x7f7f7f7f7f7f7f7fULL) | 0x8181818181818181ULL | fh;
		const uint64_t sv = (st << 8) | (st >> 8) | 0xff000000000000ffULL | fv;
		const uint64_t sd9 = (st << 9 & 0xfefefefefefefefeULL) | (st >> 9 & 0x7f7f7f7f7f7f7f7fULL) | 0xff818181818181ffULL | f9;
		const uint64_t sd7 = (st << 7 & 0x7f7f7f7f7f7f7f7fULL) | (st >> 7 & 0xfefefefefefefefeULL) | 0xff818181818181ffULL | f7;
		st |= P & sh & sv & sd9 & sd7;
	} while (st != old);
	return st;
}

// edge-stable discs (edge discs can only be flipped along the edge): full edges, and runs anchored at an own corner
static int edge_stable(uint64_t P, uint64_t O)
{
	static const unsigned char E[4][8] = { {0,1,2,3,4,5,6,7}, {56,57,58,59,60,61,62,63}, {0,8,16,24,32,40,48,56}, {7,15,23,31,39,47,55,63} };
	uint64_t st = 0;
	for (int e = 0; e < 4; ++e) {
		int full = 1;
		for (int i = 0; i < 8; ++i) if (!(((P | O) >> E[e][i]) & 1)) full = 0;
		if (full) { for (int i = 0; i < 8; ++i) if ((P >> E[e][i]) & 1) st |= 1ULL << E[e][i]; continue; }
		for (int i = 0; i < 8 && ((P >> E[e][i]) & 1); ++i) st |= 1ULL << E[e][i];
		for (int i = 7; i >= 0 && ((P >> E[e][i]) & 1); --i) st |= 1ULL << E[e][i];
	}
	return popc(st);
}
#ifdef _MSC_VER
#pragma code_seg()
#endif

// ---------------- configuration ----------------
enum { S_MOB, S_MOBP, S_CMOB, S_QPAR, S_STAB, S_FRONT, S_DISC, S_ESTAB, S_MOBHV, S_MOBD, S_MOB2D, S_N };
static const char *SCALAR_NAME[S_N] = { "mob", "mobp", "cmob", "qpar", "stab", "front", "disc", "estab", "mobhv", "mobd", "mob2d" };
static const int SCALAR_LANES[S_N] = { 2, 1, 1, 1, 2, 2, 1, 2, 2, 2, 1 };
static const int SCALAR_SIZE[S_N] = { 64, 32, 25, 16, 130, 64, 65, 64, 64, 64, 1024 };	// mob2d: joint table of both mobilities (1 lane)

struct Config {
	char name[64];
	int n_inst, inst_n[256], inst_ofs[256];
	unsigned char inst_sq[256][16];
	const int *inst_canon[256];
	int scalar[S_N], scalar_ofs[S_N];
	int nf, nw, table;	// features per sample (lanes), weights (incl. constant at 0)
};

static int parse_config(char *line, Config *c)
{
	memset(c, 0, sizeof *c);
	char *tok = strtok(line, " \t\r\n");
	if (!tok || tok[0] == '#') return 0;
	strncpy(c->name, tok, 63);
	c->nw = 1;	// weight 0 = constant
	while ((tok = strtok(nullptr, " \t\r\n"))) {
		int found = 0;
		for (int s = 0; s < S_N; ++s) if (!strcmp(tok, SCALAR_NAME[s])) {
			if (c->scalar[s]) { fprintf(stderr, "%s twice in %s\n", tok, c->name); exit(1); }
			c->scalar[s] = 1; c->scalar_ofs[s] = c->nw; c->nw += SCALAR_SIZE[s]; c->nf += SCALAR_LANES[s]; c->table += SCALAR_SIZE[s]; found = 1;
		}
		for (int p = 0; p < n_lib && !found; ++p) if (!strcmp(tok, lib[p].name)) {
			int size = 1;
			const int first = c->n_inst;
			for (int i = 0; i < lib[p].n; ++i) size *= 3;
			for (int s = 0; s < 8; ++s) {
				uint64_t set = 0;
				int dup = 0;
				const int k = c->n_inst;
				if (k == 256) {	// no room: only a duplicate instance is accepted
					for (int i = 0; i < lib[p].n; ++i) set |= 1ULL << sym(lib[p].sq[i], s);
				} else {	// written in the next place (kept there only if it is not a duplicate)
					c->inst_n[k] = lib[p].n; c->inst_ofs[k] = c->nw; c->inst_canon[k] = canon_table(p);
					for (int i = 0; i < lib[p].n; ++i) { c->inst_sq[k][i] = (unsigned char) sym(lib[p].sq[i], s); set |= 1ULL << c->inst_sq[k][i]; }
				}
				for (int j = first; j < k; ++j) { uint64_t o = 0; for (int i = 0; i < c->inst_n[j]; ++i) o |= 1ULL << c->inst_sq[j][i]; if (o == set) dup = 1; }
				if (!dup) {
					if (k == 256) die("too many pattern places in configuration %s (at most 256)", c->name);
					++c->n_inst;
				}
			}
			c->nf += c->n_inst - first; c->nw += size; c->table += size; found = 1;
		}
		if (!found) { fprintf(stderr, "unknown item %s in %s\n", tok, c->name); exit(1); }
	}
	return 1;
}

#ifdef _MSC_VER
#pragma code_seg(".text$a1")
#endif
static void features(const Config *c, uint64_t P, uint64_t O, unsigned int *f)
{
	int k = 0;
	for (int j = 0; j < c->n_inst; ++j) {
		int x = 0;
		for (int i = 0; i < c->inst_n[j]; ++i) {
			const int s = c->inst_sq[j][i];
			x = x * 3 + ((P >> s) & 1 ? 0 : (O >> s) & 1 ? 1 : 2);
		}
		f[k++] = (unsigned int) (c->inst_ofs[j] + c->inst_canon[j][x]);
	}
	if (c->scalar[S_MOB] || c->scalar[S_MOBP] || c->scalar[S_CMOB] || c->scalar[S_MOB2D]) {
		const uint64_t mp = get_moves(P, O), mo = get_moves(O, P);
		int a = popc(mp), b = popc(mo);
		if (a > 31) a = 31;
		if (b > 31) b = 31;
		if (c->scalar[S_MOB]) { f[k++] = c->scalar_ofs[S_MOB] + a; f[k++] = c->scalar_ofs[S_MOB] + 32 + b; }
		if (c->scalar[S_MOBP]) f[k++] = c->scalar_ofs[S_MOBP] + a;
		if (c->scalar[S_MOB2D]) f[k++] = c->scalar_ofs[S_MOB2D] + a * 32 + b;
		if (c->scalar[S_CMOB]) f[k++] = c->scalar_ofs[S_CMOB] + popc(mp & 0x8100000000000081ULL) * 5 + popc(mo & 0x8100000000000081ULL);
	}
	if (c->scalar[S_QPAR]) {
		const uint64_t E = ~(P | O);
		const int q = (popc(E & 0x000000000f0f0f0fULL) & 1) | (popc(E & 0x00000000f0f0f0f0ULL) & 1) << 1
		            | (popc(E & 0x0f0f0f0f00000000ULL) & 1) << 2 | (popc(E & 0xf0f0f0f000000000ULL) & 1) << 3;
		f[k++] = c->scalar_ofs[S_QPAR] + q;
	}
	if (c->scalar[S_STAB]) {
		f[k++] = c->scalar_ofs[S_STAB] + popc(stable_discs(P, O));
		f[k++] = c->scalar_ofs[S_STAB] + 65 + popc(stable_discs(O, P));
	}
	if (c->scalar[S_FRONT]) {
		const uint64_t fr = neighbours(~(P | O));
		int a = popc(P & fr), b = popc(O & fr);
		if (a > 31) a = 31;
		if (b > 31) b = 31;
		f[k++] = c->scalar_ofs[S_FRONT] + a; f[k++] = c->scalar_ofs[S_FRONT] + 32 + b;
	}
	if (c->scalar[S_DISC]) f[k++] = c->scalar_ofs[S_DISC] + popc(P);
	if (c->scalar[S_MOBHV]) {
		int a = popc(get_moves_dirs(P, O, 0, 2)), b = popc(get_moves_dirs(O, P, 0, 2));
		if (a > 31) a = 31;
		if (b > 31) b = 31;
		f[k++] = c->scalar_ofs[S_MOBHV] + a; f[k++] = c->scalar_ofs[S_MOBHV] + 32 + b;
	}
	if (c->scalar[S_MOBD]) {
		int a = popc(get_moves_dirs(P, O, 1, 1) | get_moves_dirs(P, O, 3, 3)), b = popc(get_moves_dirs(O, P, 1, 1) | get_moves_dirs(O, P, 3, 3));
		if (a > 31) a = 31;
		if (b > 31) b = 31;
		f[k++] = c->scalar_ofs[S_MOBD] + a; f[k++] = c->scalar_ofs[S_MOBD] + 32 + b;
	}
	if (c->scalar[S_ESTAB]) { f[k++] = c->scalar_ofs[S_ESTAB] + edge_stable(P, O); f[k++] = c->scalar_ofs[S_ESTAB] + 32 + edge_stable(O, P); }
}
#ifdef _MSC_VER
#pragma code_seg()
#endif

// ---------------- data ----------------
static Source src[8];
static int n_src;
struct Data { int n, cap; Rec *r; float *c, *t; double *te; };	// t: training target (label, or mixed with the teacher); te: the teacher's evaluation of a test position
static Data train[64], test[64][8];
static int plies[64], n_plies;

static ET_Eval *teacher;	// -teacher eval.dat: target = alpha * label + (1 - alpha) * teacher's evaluation (-alpha a)
static double alpha = 1.0;
static ET_Eval *prior;	// -prior eval.dat: L2 toward Edax's weights for the patterns that are Edax features (others toward 0)

// prior weights of configuration c at ply (in discs), 0 where the pattern is not an Edax feature
static void prior_weights(const Config *c, int ply, double *w0, char *cov)
{
	memset(cov, 0, c->nw); cov[0] = 1;	// cov: weights that have a prior from Edax
	const short *ew = prior->w[et_ply(60 - ply)];
	memset(w0, 0, c->nw * sizeof *w0);
	w0[0] = ew[ET_NW - 1] / 128.0;
	for (int j = 0; j < c->n_inst; ++j) {
		if (j > 0 && c->inst_ofs[j] == c->inst_ofs[j - 1]) continue;	// same table as the previous instance
		const int n = c->inst_n[j];
		uint64_t set = 0;
		for (int i = 0; i < n; ++i) set |= 1ULL << c->inst_sq[j][i];
		for (int f = 0; f < ET_NF; ++f) {
			const ET_F2X *e = ET_FEAT + f;
			uint64_t es = 0;
			int pw[16];
			const int ok = (e->n == n);
			for (int i = 0; i < e->n; ++i) es |= 1ULL << e->x[i];
			if (!ok || es != set) continue;
			for (int i = 0; i < n; ++i) {	// weight of our digit i in Edax's index
				int k = 0;
				while (e->x[k] != c->inst_sq[j][i]) ++k;
				pw[i] = 1;
				for (int m = k + 1; m < n; ++m) pw[i] *= 3;
			}
			int size = 1;
			for (int i = 0; i < n; ++i) size *= 3;
			const int cl = ET_CLASS[f];
			for (int x = 0; x < size; ++x) {
				int y = x, xe = 0;
				for (int i = n - 1; i >= 0; --i) { xe += (y % 3) * pw[i]; y /= 3; }
				w0[c->inst_ofs[j] + x] = ew[ET_PACK[cl][xe] + ET_POFS[cl]] / 128.0;
				cov[c->inst_ofs[j] + x] = 1;
			}
			break;
		}
	}
	if (prior->has_mob) {	// the mobility weights of eval2 (of the ply itself) for mob and mobp
		const short *m = prior->mob[ply];
		if (c->scalar[S_MOB]) for (int a = 0; a < 64; ++a) { w0[c->scalar_ofs[S_MOB] + a] = m[a] / 128.0; cov[c->scalar_ofs[S_MOB] + a] = 1; }
		if (c->scalar[S_MOBP]) for (int a = 0; a < 32; ++a) { w0[c->scalar_ofs[S_MOBP] + a] = m[a] / 128.0; cov[c->scalar_ofs[S_MOBP] + a] = 1; }
	}
}

// more room for the records of a data set (outside push, which is called for each record)
static EVTOOL_NOINLINE void data_grow(Data *d)
{
	d->cap = grow(d->cap, 1 << 16);
	d->r = (Rec *) xrealloc(d->r, d->cap * sizeof *d->r);
	d->c = (float *) xrealloc(d->c, d->cap * sizeof *d->c);
	d->t = (float *) xrealloc(d->t, d->cap * sizeof *d->t);
}

static void push(Data *d, const Rec *r, float c)
{
	if (d->n == d->cap) data_grow(d);
	d->r[d->n] = *r; d->c[d->n] = c; d->t[d->n] = r->v; ++d->n;
}

// Places of the hot code (MSVC). The code sections .text$a<n> come first in the code, in the order of their names:
//   .text$a1  the scalar features and features() (3,728 bytes, from the start of the code)
//   .text$a2  pad a (368 bytes): what follows starts a 4 KiB page
//   .text$a3  apply_base and its OpenMP loop, from the start of a page (the loop over the records took 4 to 6 % more
//             cycles at other places of the page, the same machine code, on an AMD Ryzen 9 9950X: docs/port/EVTOOLS-3.md)
//   .text$a4  pad b (48 bytes)
//   .text$a5  run_config and its OpenMP loops (the first one at 0 in its 64-byte line, as before the sections)
//   .text$a6  pad c (48 bytes)
//   .text$a7  load (at 48 in its 64-byte line, as before the sections)
// A pad of n nops takes n + 9 bytes (with the store and "ret 0") rounded up to 16. A change of the size of the code
// before a pad moves what follows it: build-evtools.ps1 checks these places in the link map of evsearch. The pads are
// called by main when argc < 0 (never), so that the linker keeps them.
#ifdef _MSC_VER
#define EVS_NOP8 __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
#define EVS_NOP64 EVS_NOP8 EVS_NOP8 EVS_NOP8 EVS_NOP8 EVS_NOP8 EVS_NOP8 EVS_NOP8 EVS_NOP8
#pragma code_seg(".text$a2")
static EVTOOL_NOINLINE void hot_pad_a(int *x) { EVS_NOP64 EVS_NOP64 EVS_NOP64 EVS_NOP64 EVS_NOP64 EVS_NOP8 EVS_NOP8 EVS_NOP8 *x = 1; }	// 344 nops: 368 bytes
#pragma code_seg(".text$a4")
static EVTOOL_NOINLINE void hot_pad_b(int *x) { EVS_NOP8 EVS_NOP8 EVS_NOP8 EVS_NOP8 *x = 2; }	// 32 nops: 48 bytes
#pragma code_seg(".text$a3")
#endif
// -base dir:cfg : screening mode. The weights of configuration cfg (dumped in dir: cfg.pat, cfg_<ply>.w, patterns only)
// are a fixed base evaluation; training targets and test labels become the residuals (label - base), so that each
// configuration of the -configs file is fitted alone on what the base does not explain.
static char base_dir[512], base_cfg[64];

static void apply_base(int pi, int ply)
{
	char name[1024];
	int n_inst, nw, k;
	sprintf(name, "%s/%s.pat", base_dir, base_cfg);
	FILE *f = fopen(name, "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", name); exit(1); }
	xfread(&n_inst, 4, 1, f, name); xfread(&nw, 4, 1, f, name);
	if (n_inst < 0 || n_inst > 256 || nw < 1) die("bad instances in %s", name);
	int *bn = (int *) xmalloc(n_inst * sizeof *bn), *bo = (int *) xmalloc(n_inst * sizeof *bo);
	unsigned char (*bs)[16] = (unsigned char (*)[16]) xmalloc(n_inst * 16);
	for (int j = 0; j < n_inst; ++j) {
		xfread(bn + j, 4, 1, f, name); xfread(bo + j, 4, 1, f, name); xfread(bs[j], 1, 16, f, name);
		int size = 1;
		for (int i = 0; i < bn[j] && i < 16; ++i) size *= 3;
		if (bn[j] < 1 || bn[j] > 16 || bo[j] < 1 || bo[j] > nw - size) die("bad instances in %s", name);
		for (int i = 0; i < bn[j]; ++i) if (bs[j][i] > 63) die("bad instances in %s", name);
	}
	fclose(f);
	sprintf(name, "%s/%s_%02d.w", base_dir, base_cfg, ply);
	f = fopen(name, "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", name); exit(1); }
	double *w = (double *) xmalloc(nw * sizeof *w);
	xfread(w, 8, nw, f, name);
	fclose(f);
	for (int set = -1; set < n_src; ++set) {
		Data *D = set < 0 ? train + pi : &test[pi][set];
		#pragma omp parallel for schedule(static)
		for (k = 0; k < D->n; ++k) {
			const uint64_t P = D->r[k].P, O = D->r[k].O;
			double pr = w[0];
			for (int j = 0; j < n_inst; ++j) {
				int x = 0;
				for (int i = 0; i < bn[j]; ++i) { const int q = bs[j][i]; x = x * 3 + ((P >> q) & 1 ? 0 : (O >> q) & 1 ? 1 : 2); }
				pr += w[bo[j] + x];
			}
			if (set < 0) D->t[k] -= (float) pr;
			else { double e = D->r[k].v - pr; if (e > 127) e = 127; if (e < -127) e = -127; D->r[k].v = (signed char) (e >= 0 ? e + 0.5 : e - 0.5); }
		}
	}
	free(bn); free(bo); free(bs); free(w);
}

#ifdef _MSC_VER
#pragma code_seg()
#endif
// the teacher's evaluation of a position: the 46 features, and the mobility weights of eval2 (outside the loops that call it)
static EVTOOL_NOINLINE double teacher_value(uint64_t P, uint64_t O)
{
	int idx[ET_NF];
	et_features(P, O, idx);
	return (et_sum(teacher->w[et_ply(64 - popc(P | O))], idx) + (teacher->has_mob ? et_mob(teacher, P, O) : 0)) / 128.0;
}

static void apply_teacher(Data *d)
{
	int k;
	if (!teacher || alpha >= 1.0) return;
	#pragma omp parallel for schedule(static)
	for (k = 0; k < d->n; ++k) {
		const double te = teacher_value(d->r[k].P, d->r[k].O);
		d->t[k] = (float) (alpha * d->r[k].v + (1.0 - alpha) * te);
	}
}

// -window k -decay d: the training samples of a ply p also come from the plies p - 2k .. p + 2k of the same parity,
// with their weights multiplied by d^j at a distance of 2j plies; the test samples come from p only.
static int window = 0;
static double decay = 0.5;

// the teacher's evaluation of the test positions, once (run_config uses it for each configuration); the arrays c and t
// of a test set are not used: their room is given back first
static void teacher_test(int pi)
{
	if (!teacher) return;
	for (int d = 0; d < n_src; ++d) {
		Data *T = &test[pi][d];
		int k;
		free(T->c); free(T->t); T->c = T->t = nullptr;
		T->te = (double *) xmalloc((size_t) (T->n > 0 ? T->n : 1) * sizeof *T->te);
		#pragma omp parallel for schedule(static)
		for (k = 0; k < T->n; ++k) T->te[k] = teacher_value(T->r[k].P, T->r[k].O);
	}
}

#ifdef _MSC_VER
#pragma code_seg(".text$a6")
static EVTOOL_NOINLINE void hot_pad_c(int *x) { EVS_NOP8 EVS_NOP8 EVS_NOP8 EVS_NOP8 *x = 3; }	// 32 nops: 48 bytes (see the places of the hot code above apply_base)
#pragma code_seg(".text$a7")
#endif
static void load(int pi, int ply)
{
	for (int j = -window; j <= window; ++j) {
		const int q = ply + 2 * j;
		if (q < 0 || q > 60) continue;
		const double factor = pow(decay, abs(j));
		for (int d = 0; d < n_src; ++d) {
			char name[1024];
			sprintf(name, "%s/%s%02d.bin", src[d].dir, src[d].prefix, q);
			FILE *f = fopen(name, "rb");
			if (!f) continue;
			const long long n = file_size(f) / (long long) sizeof (Rec);
			const double keep = n > src[d].max ? (double) src[d].max / n : 1.0;
			const uint64_t thr = keep >= 1.0 ? ~0ULL : (uint64_t) (keep * 18446744073709551615.0);	// (2^64 as a double does not fit)
			const float weight = (float) (src[d].weight * factor);
			Rec *buf = (Rec *) xmalloc(sizeof (Rec) << 20);
			size_t m;
			while ((m = fread(buf, sizeof (Rec), 1 << 20, f)) > 0) {
				for (size_t i = 0; i < m; ++i) {
					const uint64_t h = mix64(buf[i].P * 0x9E3779B97F4A7C15ULL ^ buf[i].O);
					if (h % 32 == 0) { if (j == 0 && ((mix64(h) >> 40) % 4 == 0 || n < 4000000)) push(&test[pi][d], buf + i, 1.0f); }
					else if (mix64(h ^ 0x1234567ULL) <= thr) push(&train[pi], buf + i, weight);
				}
			}
			free(buf);
			fclose(f);
		}
	}
}

#ifdef _MSC_VER
#pragma code_seg()
#endif
// ---------------- training ----------------
static int nt = 24, iter = 30;
static const char *dumpdir;	// -dump dir: write the trained weights (dir/<config>_<ply>.w, doubles) and instances (dir/<config>.pat)
static double lambda = 300, lambda_prior = -1;	// -lambda-prior: L2 strength for the weights that have a prior (default: lambda; 1e30 freezes them)

#ifdef _MSC_VER
#pragma code_seg(".text$a5")
#endif
static void run_config(const Config *c, int pi, double *mse)
{
	const Data *D = train + pi;
	const int nf = c->nf, nw = c->nw;
	unsigned int *idx = (unsigned int *) xmalloc((size_t) D->n * nf * sizeof *idx);
	int k;
	#pragma omp parallel for schedule(static)
	for (k = 0; k < D->n; ++k) features(c, D->r[k].P, D->r[k].O, idx + (size_t) k * nf);
	double *w0 = (double *) xcalloc(nw, 8);
	float *tt = (float *) xmalloc((size_t) D->n * sizeof *tt);
	double *lam = (double *) xmalloc(nw * sizeof *lam);
	{
		char *cov = (char *) xcalloc(nw, 1);
		if (prior) prior_weights(c, plies[pi], w0, cov);
		for (int a = 0; a < nw; ++a) lam[a] = (prior && cov[a] && lambda_prior >= 0) ? lambda_prior : lambda;
		free(cov);
	}
	#pragma omp parallel for schedule(static)
	for (k = 0; k < D->n; ++k) {
		const unsigned int *x = idx + (size_t) k * nf;
		double p0 = w0[0];
		for (int j = 0; j < nf; ++j) p0 += w0[x[j]];
		tt[k] = (float) (D->t[k] - p0);
	}
	double *w = (double *) xcalloc(nw, 8), *r = (double *) xcalloc(nw, 8), *z = (double *) xcalloc(nw, 8);
	double *p = (double *) xcalloc(nw, 8), *q = (double *) xcalloc(nw, 8), *M = (double *) xcalloc(nw, 8);
	double **tq = (double **) xmalloc(nt * sizeof *tq), **td = (double **) xmalloc(nt * sizeof *td);
	for (int t = 0; t < nt; ++t) { tq[t] = (double *) xmalloc(nw * 8); td[t] = (double *) xmalloc(nw * 8); }
	// residual & diagonal at w = 0
	#pragma omp parallel num_threads(nt)
	{
		double *lq = tq[omp_get_thread_num()], *ld = td[omp_get_thread_num()];
		memset(lq, 0, nw * 8); memset(ld, 0, nw * 8);
		int kk;
		#pragma omp for schedule(static)
		for (kk = 0; kk < D->n; ++kk) {
			const unsigned int *x = idx + (size_t) kk * nf;
			const double t = tt[kk] * D->c[kk];
			for (int i = 0; i < nf; ++i) { lq[x[i]] += t; ld[x[i]] += D->c[kk]; }
			lq[0] += t; ld[0] += D->c[kk];
		}
	}
	int i;
	#pragma omp parallel for
	for (i = 0; i < nw; ++i) { double a = 0, b = 0; for (int t = 0; t < nt; ++t) { a += tq[t][i]; b += td[t][i]; } r[i] = a; M[i] = b + lam[i]; if (lam[i] >= 1e20) { r[i] = 0; M[i] = 1; } }	// lam >= 1e20: frozen at the prior
	double rz = 0;
	for (i = 0; i < nw; ++i) { z[i] = r[i] / M[i]; p[i] = z[i]; rz += r[i] * z[i]; }
	for (int it = 0; it < iter; ++it) {
		#pragma omp parallel num_threads(nt)
		{
			double *lq = tq[omp_get_thread_num()];
			memset(lq, 0, nw * 8);
			int kk;
			#pragma omp for schedule(static)
			for (kk = 0; kk < D->n; ++kk) {
				const unsigned int *x = idx + (size_t) kk * nf;
				double t = p[0];
				for (int j = 0; j < nf; ++j) t += p[x[j]];
				t *= D->c[kk];
				for (int j = 0; j < nf; ++j) lq[x[j]] += t;
				lq[0] += t;
			}
		}
		double pq = 0;
		#pragma omp parallel for reduction(+:pq)
		for (i = 0; i < nw; ++i) { double a = 0; for (int t = 0; t < nt; ++t) a += tq[t][i]; q[i] = lam[i] >= 1e20 ? 0 : a + lam[i] * p[i]; pq += p[i] * q[i]; }
		if (!(pq > 0) || !(rz > 1e-30)) break;	// converged (e.g. the prior fits exactly)
		const double step = rz / pq;
		double rz_new = 0;
		for (i = 0; i < nw; ++i) { w[i] += step * p[i]; r[i] -= step * q[i]; z[i] = r[i] / M[i]; rz_new += r[i] * z[i]; }
		const double beta = rz_new / rz; rz = rz_new;
		for (i = 0; i < nw; ++i) p[i] = z[i] + beta * p[i];
	}
	free(idx);
	for (i = 0; i < nw; ++i) w[i] += w0[i];
	for (int j = 0; j < c->n_inst; ++j) {	// expand the shared weights to the whole tables
		if (j > 0 && c->inst_ofs[j] == c->inst_ofs[j - 1]) continue;
		int size = 1;
		for (int a = 0; a < c->inst_n[j]; ++a) size *= 3;
		for (int x = 0; x < size; ++x) w[c->inst_ofs[j] + x] = w[c->inst_ofs[j] + c->inst_canon[j][x]];
	}
	free(w0); free(tt); free(lam);
	if (dumpdir) {
		char name[1024];
		sprintf(name, "%s/%s_%02d.w", dumpdir, c->name, plies[pi]);
		FILE *g = xfopen(name, "wb");
		fwrite(w, 8, nw, g);
		if (fclose(g) != 0) die("cannot write %s", name);
	}
	// test errors
	for (int d = 0; d < n_src; ++d) {
		const Data *T = &test[pi][d];
		double se = 0, st = 0;
		mse[8 + d] = -1;
		if (!T->n) { mse[d] = -1; continue; }
		#pragma omp parallel for reduction(+:se,st)
		for (k = 0; k < T->n; ++k) {
			unsigned int f[512];
			features(c, T->r[k].P, T->r[k].O, f);
			double pr = w[0];
			for (int j = 0; j < nf; ++j) pr += w[f[j]];
			se += (pr - T->r[k].v) * (pr - T->r[k].v);
			if (teacher) {	// distance to the teacher's evaluation
				const double te = T->te[k];
				st += (pr - te) * (pr - te);
			}
		}
		mse[d] = se / T->n;
		if (teacher) mse[8 + d] = st / T->n;
	}
	free(w); free(r); free(z); free(p); free(q); free(M);
	for (int t = 0; t < nt; ++t) { free(tq[t]); free(td[t]); }
	free(tq); free(td);
}

#ifdef _MSC_VER
#pragma code_seg()
#endif
int main(int argc, char **argv)
{
	const char *libf = nullptr, *conff = nullptr;
	auto value = [&](int &i) -> char * { if (i + 1 >= argc) die("missing value of %s", argv[i]); return argv[++i]; };
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-lib")) libf = value(i);
		else if (!strcmp(argv[i], "-configs")) conff = value(i);
		else if (!strcmp(argv[i], "-plies")) {
			char *s = value(i), *t;
			for (t = strtok(s, ","); t; t = strtok(nullptr, ",")) {
				if (n_plies == 64) die("too many plies (at most 64)");
				plies[n_plies] = atoi(t);
				if (plies[n_plies] < 0 || plies[n_plies] > 60) die("plies are 0 to 60: %s", t);
				++n_plies;
			}
		}
		else if (!strcmp(argv[i], "-src")) {
			if (n_src == 8) die("too many -src (at most 8)");
			Source *o = src + n_src++;
			o->max = 1LL << 40;
			if (source_parse(value(i), o) < 3) die("bad -src %s (prefix:dir:weight[:max])", argv[i]);
		}
		else if (!strcmp(argv[i], "-iter")) iter = atoi(value(i));
		else if (!strcmp(argv[i], "-lambda")) lambda = atof(value(i));
		else if (!strcmp(argv[i], "-lambda-prior")) lambda_prior = atof(value(i));
		else if (!strcmp(argv[i], "-threads")) nt = atoi(value(i));
		else if (!strcmp(argv[i], "-dump")) dumpdir = value(i);
		else if (!strcmp(argv[i], "-notie")) tie = 0;
		else if (!strcmp(argv[i], "-window")) { window = atoi(value(i)); if (window < 0 || window > 30) die("bad -window (0 to 30)"); }
		else if (!strcmp(argv[i], "-decay")) { decay = atof(value(i)); if (!(decay > 0 && decay <= 1)) die("bad -decay (0 < d <= 1)"); }
		else if (!strcmp(argv[i], "-base")) { if (!dir_name_parse(value(i), base_dir, sizeof base_dir, base_cfg, sizeof base_cfg)) die("bad -base %s (dir:cfg)", argv[i]); }
		else if (!strcmp(argv[i], "-teacher")) { et_init(); teacher = et_read(value(i)); }
		else if (!strcmp(argv[i], "-alpha")) alpha = atof(value(i));
		else if (!strcmp(argv[i], "-prior")) { et_init(); prior = et_read(value(i)); }
	}
#ifdef _MSC_VER
	if (argc < 0) { hot_pad_a(&argc); hot_pad_b(&argc); hot_pad_c(&argc); }
#endif
	if (nt < 1) die("bad -threads");
	omp_set_num_threads(nt);
	read_lib(libf);
	if (conff == nullptr) die("missing -configs");
	FILE *f = xfopen(conff, "r");
	for (int pi = 0; pi < n_plies; ++pi) { load(pi, plies[pi]); apply_teacher(train + pi); teacher_test(pi); if (base_cfg[0]) apply_base(pi, plies[pi]); }
	fprintf(stderr, "%d patterns; data loaded\n", n_lib);
	char line[4096], copy[4096];
	while (fgets(line, sizeof line, f)) {
		Config c;
		strcpy(copy, line);
		if (!parse_config(copy, &c)) continue;
		if (dumpdir) {	// instances: n_inst, nw, then n, ofs, sq[16] per instance (ints, bytes)
			char name[1024];
			sprintf(name, "%s/%s.pat", dumpdir, c.name);
			FILE *g = xfopen(name, "wb");
			fwrite(&c.n_inst, 4, 1, g); fwrite(&c.nw, 4, 1, g);
			for (int j = 0; j < c.n_inst; ++j) { fwrite(&c.inst_n[j], 4, 1, g); fwrite(&c.inst_ofs[j], 4, 1, g); fwrite(c.inst_sq[j], 1, 16, g); }
			if (fclose(g) != 0) die("cannot write %s", name);
			sprintf(name, "%s/%s.scl", dumpdir, c.name);	// scalar features: S_N x (used, offset)
			g = xfopen(name, "wb");
			for (int a = 0; a < S_N; ++a) { fwrite(&c.scalar[a], 4, 1, g); fwrite(&c.scalar_ofs[a], 4, 1, g); }
			if (fclose(g) != 0) die("cannot write %s", name);
		}
		printf("%-24s lanes %3d table %7d |", c.name, c.nf, c.table);
		for (int pi = 0; pi < n_plies; ++pi) {
			double mse[16];
			run_config(&c, pi, mse);
			for (int d = 0; d < n_src; ++d) if (mse[d] >= 0) printf(" %d:%s %.3f", plies[pi], src[d].prefix, mse[d]);
			for (int d = 0; d < n_src; ++d) if (mse[d] >= 0 && mse[8 + d] >= 0) printf(" %d:%sT %.3f", plies[pi], src[d].prefix, mse[8 + d]);
		}
		printf("\n");
		fflush(stdout);
	}
	fclose(f);
	return 0;
}
