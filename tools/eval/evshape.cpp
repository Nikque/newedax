/**
 * @file evshape.cpp
 *
 * Errors of an evaluation by edge shape: the records are classified by the shapes of their 4 edges (mountain, wing,
 * block, bomb, with the corners and the X squares empty), for the player who owns the shape, by the state of the 4
 * inner squares c2-f2, the adjacent edges and the game phase; a table of the evaluation minus the label is printed.
 * usage: evshape <eval.dat> <threads> [-sample out.raw n] file ...
 *   -sample: writes at most n records of each cell of the table and of each phase (records without a label),
 *            to give them a deeper label (label) and to read the result again with evshape.
 *   the table: "shape mid owner adj phase n mean sd mean_label"; the errors and labels are in the view of the owner
 *   of the shape ("all" rows: all the records of a phase, in the view of the side to move).
 */
#include <cmath>
#include <omp.h>

#include "evtool.hpp"

using namespace evtool;

enum { SH_MOUNTAIN, SH_WING, SH_BLOCK, SH_BOMB, SH_N };
enum { MID_PURE, MID_MIXED, MID_OPEN, MID_N };
enum { ADJ_N = 3, PHASE_N = 7, CELL_N = SH_N * MID_N * 2 * ADJ_N * PHASE_N };
static const char *SHAPE_NAME[SH_N] = { "mountain", "wing", "block", "bomb" };
static const char *MID_NAME[MID_N] = { "pure", "mixed", "open" };

struct Stat { long long n; double e, e2, v; };

static inline void stat_add(Stat *s, double e, double v) { ++s->n; s->e += e; s->e2 += e * e; s->v += v; }

static inline int cell_of(int shape, int mid, int owner, int adj, int phase)
{
	return ((((shape * MID_N + mid) * 2 + owner) * ADJ_N + adj) * PHASE_N) + phase;
}

/** a column of the board as a byte, a1 side in the low bit */
static inline int column(uint64_t b, int c)
{
	return (int) ((((b >> c) & 0x0101010101010101ULL) * 0x0102040810204080ULL) >> 56);
}

/** shape of an edge for the player x against y (bytes of the edge and of the line next to it), or -1 */
static inline int edge_shape(int xe, int ye, int xs, int ys)
{
	if (((xe | ye) & 0x81) || ((xs | ys) & 0x42)) return -1;
	if (ye == 0) {
		if (xe == 0x7e) return SH_MOUNTAIN;
		if (xe == 0x7c || xe == 0x3e) return SH_WING;
		if (xe == 0x3c) return SH_BLOCK;
	} else if (xe == 0x24 && ye == 0x18) return SH_BOMB;
	return -1;
}

static inline int mid_state(int xs, int ys)
{
	if ((xs & 0x3c) == 0x3c) return MID_PURE;
	return ((xs | ys) & 0x3c) == 0x3c ? MID_MIXED : MID_OPEN;
}

/** the cells of a position (at most 4 edges x 2 owners; a position has at most 4 shapes); returns their number */
static int cells_of(uint64_t P, uint64_t O, int phase, int *cell, int *owner)
{
	const uint64_t bd[2] = { P, O };
	int e[2][4], s[2][4], n = 0;
	for (int k = 0; k < 2; ++k) {
		e[k][0] = (int) (bd[k] & 0xff); s[k][0] = (int) ((bd[k] >> 8) & 0xff);
		e[k][1] = (int) (bd[k] >> 56); s[k][1] = (int) ((bd[k] >> 48) & 0xff);
		e[k][2] = column(bd[k], 0); s[k][2] = column(bd[k], 1);
		e[k][3] = column(bd[k], 7); s[k][3] = column(bd[k], 6);
	}
	for (int k = 0; k < 2; ++k) for (int i = 0; i < 4; ++i) {
		const int shape = edge_shape(e[k][i], e[1 - k][i], s[k][i], s[1 - k][i]);
		if (shape < 0) continue;
		const int a = i < 2 ? 2 : 0;	// the adjacent edges
		const int adj = (popc((uint64_t) e[k][a]) >= 4) + (popc((uint64_t) e[k][a + 1]) >= 4);
		cell[n] = cell_of(shape, mid_state(s[k][i], s[1 - k][i]), k, adj, phase);
		owner[n++] = k;
	}
	return n;
}

static inline int phase_of(int ply) { int p = (ply - 8) / 8; return p < 0 ? 0 : p >= PHASE_N ? PHASE_N - 1 : p; }

static void print_stat(const char *head, const Stat *s, int phase)
{
	if (s->n == 0) return;
	const double m = s->e / s->n, var = s->e2 / s->n - m * m;
	printf("%s\t%d\t%lld\t%.3f\t%.3f\t%.3f\n", head, 8 + 8 * phase, s->n, m, sqrt(var > 0 ? var : 0), s->v / s->n);
}

int main(int argc, char **argv)
{
	if (argc < 4) die("usage: evshape <eval.dat> <threads> [-sample out.raw n] file ...");
	et_init();
	const ET_Eval *ev = et_read(argv[1]);
	const int nt = atoi(argv[2]);
	if (nt < 1) die("bad threads %s", argv[2]);
	const char *sample = nullptr;
	long long n_sample = 0;
	int first = 3;
	if (!strcmp(argv[first], "-sample")) {
		if (argc < 7) die("-sample needs a file and a number");
		sample = argv[first + 1]; n_sample = atoll(argv[first + 2]);
		first += 3;
	}
	omp_set_num_threads(nt);
	Stat *cells = (Stat *) xcalloc((size_t) nt * CELL_N, sizeof *cells), *base = (Stat *) xcalloc((size_t) nt * PHASE_N, sizeof *base);
	Rec *buf = (Rec *) xmalloc(sizeof (Rec) << 20);
	for (int i = first; i < argc; ++i) {
		FILE *f = xfopen(argv[i], "rb");
		size_t m;
		while ((m = fread(buf, sizeof (Rec), 1 << 20, f)) > 0) {
			int k;
			#pragma omp parallel for schedule(static)
			for (k = 0; k < (int) m; ++k) {
				const Rec *r = buf + k;
				const int ply = popc(r->P | r->O) - 4, phase = phase_of(ply), t = omp_get_thread_num();
				int idx[ET_NF], cell[8], owner[8];
				et_features(r->P, r->O, idx);
				const double value = (et_sum(ev->w[et_ply(60 - ply)], idx) + (ev->has_mob ? et_mob(ev, r->P, r->O) : 0)) / 128.0;
				const double e = value - r->v;
				stat_add(base + t * PHASE_N + phase, e, r->v);
				const int n = cells_of(r->P, r->O, phase, cell, owner);
				for (int c = 0; c < n; ++c) stat_add(cells + t * CELL_N + cell[c], owner[c] ? -e : e, owner[c] ? -r->v : r->v);
			}
		}
		fclose(f);
	}
	for (int t = 1; t < nt; ++t) {
		for (int c = 0; c < CELL_N; ++c) { Stat *a = cells + c, *b = cells + t * CELL_N + c; a->n += b->n; a->e += b->e; a->e2 += b->e2; a->v += b->v; }
		for (int c = 0; c < PHASE_N; ++c) { Stat *a = base + c, *b = base + t * PHASE_N + c; a->n += b->n; a->e += b->e; a->e2 += b->e2; a->v += b->v; }
	}
	printf("shape\tmid\towner\tadj\tphase\tn\tmean\tsd\tmean_label\n");
	for (int p = 0; p < PHASE_N; ++p) print_stat("all\t-\t-\t-", base + p, p);
	for (int s = 0; s < SH_N; ++s) for (int d = 0; d < MID_N; ++d) for (int o = 0; o < 2; ++o) for (int a = 0; a < ADJ_N; ++a) for (int p = 0; p < PHASE_N; ++p) {
		char head[64];
		sprintf(head, "%s\t%s\t%s\t%d", SHAPE_NAME[s], MID_NAME[d], o ? "opponent" : "player", a);
		print_stat(head, cells + cell_of(s, d, o, a, p), p);
	}
	if (sample) {	// a second pass: records kept with the probability that gives n_sample records a cell
		FILE *g = xfopen(sample, "wb");
		long long written = 0;
		for (int i = first; i < argc; ++i) {
			FILE *f = xfopen(argv[i], "rb");
			size_t m;
			while ((m = fread(buf, sizeof (Rec), 1 << 20, f)) > 0) {
				for (size_t k = 0; k < m; ++k) {
					const Rec *r = buf + k;
					const int ply = popc(r->P | r->O) - 4, phase = phase_of(ply);
					int cell[8], owner[8];
					const int n = cells_of(r->P, r->O, phase, cell, owner);
					double keep = (double) n_sample / (double) (base[phase].n > 0 ? base[phase].n : 1);
					for (int c = 0; c < n; ++c) { const double q = (double) n_sample / (double) cells[cell[c]].n; if (q > keep) keep = q; }
					if ((double) (mix64(r->P * 0x9E3779B97F4A7C15ULL ^ r->O) >> 11) * (1.0 / 9007199254740992.0) < keep) { fwrite(r, sizeof *r, 1, g); ++written; }
				}
			}
			fclose(f);
		}
		if (fclose(g) != 0) die("cannot write %s", sample);
		fprintf(stderr, "%lld records written to %s\n", written, sample);
	}
	return 0;
}
