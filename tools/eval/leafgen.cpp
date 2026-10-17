/**
 * @file leafgen.cpp
 *
 * Collect the positions evaluated during level-L searches of book positions (training split).
 * usage: leafgen <datadir> <ply_from> <ply_to> <n_per_ply> <rate> <threads> <outdir> [-level L] [-seed s] [-eval file]
 *   each evaluated position is kept with probability 1/rate; output <outdir>/leafNN.raw (player, opponent, root ply)
 * Built with EVAL_LEAF_HOOK (the search calls eval_leaf_hook with each evaluated position).
 */
#ifndef EVAL_LEAF_HOOK
	#error "leafgen needs a build with EVAL_LEAF_HOOK"
#endif
#define main edax_original_main
#include "all.cpp"
#undef main
#include <omp.h>

#include "evtool.hpp"

using evtool::Rec;
using evtool::mix64;

struct LeafBuf { Rec *r; int n, cap; unsigned long long rng; int rate; signed char root; };
static thread_local LeafBuf *tl_buf;

static void hook(const Board *b)
{
	LeafBuf *L = tl_buf;
	if (!L) return;
	L->rng = L->rng * 6364136223846793005ULL + 1442695040888963407ULL;
	if ((L->rng >> 33) % (unsigned) L->rate) return;
	if (L->n == L->cap) { L->cap = L->cap ? 2 * L->cap : 4096; L->r = (Rec *) realloc(L->r, L->cap * sizeof *L->r); }
	L->r[L->n].P = b->player; L->r[L->n].O = b->opponent; L->r[L->n].v = L->root;
	++L->n;
}

int main(int argc, char **argv)
{
	if (argc < 8) evtool::die("usage: leafgen <datadir> <ply_from> <ply_to> <n_per_ply> <rate> <threads> <outdir> [-level L] [-seed s] [-eval file]");
	const char *datadir = argv[1], *outdir = argv[7];
	const int p0 = atoi(argv[2]), p1 = atoi(argv[3]), per_ply = atoi(argv[4]), rate = atoi(argv[5]), nt = atoi(argv[6]);
	int level = 18;
	unsigned long long seed = 1;
	options.eval_file = (char *) "eval_orig.dat";
	auto value = [&](int &i) -> char * { if (i + 1 >= argc) evtool::die("missing value of %s", argv[i]); return argv[++i]; };
	for (int i = 8; i < argc; ++i) {
		if (!strcmp(argv[i], "-level")) level = atoi(value(i));
		else if (!strcmp(argv[i], "-seed")) seed = strtoull(value(i), nullptr, 10);
		else if (!strcmp(argv[i], "-eval")) options.eval_file = value(i);
	}
	if (p0 < 0 || p1 < p0 || p1 > 60 || per_ply < 0 || rate < 1 || nt < 1) evtool::die("bad plies, n_per_ply, rate or threads");
	options.n_task = 1;
	options.hash_table_size = 22;
	options.verbosity = 0;
	bit_init();
	edge_stability_init();
	statistics_init();
	eval_open(options.eval_file);
	search_global_init();
	eval_leaf_hook = hook;

	// pick start positions: training split of book plies p0..p1, about per_ply each (hash sampling)
	Rec *S = nullptr;
	long long nS = 0, capS = 0;
	for (int p = p0; p <= p1; ++p) {
		char name[1024];
		sprintf(name, "%s/book%02d.bin", datadir, p);
		FILE *f = fopen(name, "rb");
		if (!f) continue;
		const long long n = evtool::file_size(f) / (long long) sizeof (Rec);
		const double keep = n > per_ply ? (double) per_ply / n : 1.0;
		const unsigned long long thr = keep >= 1.0 ? ~0ULL : (unsigned long long) (keep * 18446744073709551615.0);	// (2^64 as a double does not fit)
		Rec *buf = (Rec *) evtool::xmalloc(sizeof (Rec) << 20);
		size_t m;
		while ((m = fread(buf, sizeof (Rec), 1 << 20, f)) > 0) {
			for (size_t i = 0; i < m; ++i) {
				const unsigned long long h = mix64(buf[i].P * 0x9E3779B97F4A7C15ULL ^ buf[i].O);
				if (h % 32 == 0) continue;	// test split
				if (mix64(h ^ seed * 0x5851F42D4C957F2DULL) > thr) continue;
				if (nS == capS) { capS = capS ? 2 * capS : 1 << 16; S = (Rec *) evtool::xrealloc(S, capS * sizeof *S); }
				S[nS++] = buf[i];
			}
		}
		free(buf);
		fclose(f);
	}
	// shuffle so that the threads get mixed plies
	for (long long i = nS - 1; i > 0; --i) {
		const long long j = (long long) (mix64(seed + i) % (unsigned long long) (i + 1));
		const Rec t = S[i]; S[i] = S[j]; S[j] = t;
	}
	fprintf(stderr, "%lld start positions\n", nS);

	FILE *out[61] = {};
	long long count[61] = {}, done = 0, nodes = 0;
	for (int p = 0; p <= 60; ++p) {
		char name[1024];
		sprintf(name, "%s/leaf%02d.raw", outdir, p);
		out[p] = evtool::xfopen(name, "ab");
	}
	const double t0 = omp_get_wtime();

	#pragma omp parallel num_threads(nt)
	{
		Search search;
		LeafBuf L = {};
		L.rng = mix64(seed * 1000 + omp_get_thread_num() + 1);
		L.rate = rate;
		search_init(&search);
		search.options.verbosity = 0;
		tl_buf = &L;
		long long k;
		#pragma omp for schedule(dynamic, 1)
		for (k = 0; k < nS; ++k) {
			Board b; b.player = S[k].P; b.opponent = S[k].O;
			if (!board_get_moves(&b)) continue;
			L.root = (signed char) (60 - board_count_empties(&b));
			search_cleanup(&search);
			search_set_board(&search, &b, BLACK);
			search_set_level(&search, level, search.eval.n_empties);
			search_set_game_time(&search, TIME_MAX);
			search_run(&search);
			#pragma omp critical
			{
				for (int i = 0; i < L.n; ++i) {
					const int ply = 60 - (64 - bit_count(L.r[i].P | L.r[i].O));
					fwrite(L.r + i, sizeof *L.r, 1, out[ply]);
					++count[ply];
				}
				nodes += search_count_nodes(&search);
				if (++done % 200 == 0) fprintf(stderr, "%lld searches, %.0f s\n", done, omp_get_wtime() - t0);
			}
			L.n = 0;
		}
		tl_buf = nullptr;
		search_free(&search);
	}
	bool written = true;
	for (int p = 0; p <= 60; ++p) { written &= fclose(out[p]) == 0; if (count[p]) printf("ply %d: %lld\n", p, count[p]); }
	if (!written) evtool::die("cannot write the files of %s", outdir);
	printf("%lld searches, %lld nodes, %.0f s\n", done, nodes, omp_get_wtime() - t0);
	return 0;
}
