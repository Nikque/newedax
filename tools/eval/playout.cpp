/**
 * @file playout.cpp
 *
 * Label given positions by a playout: both sides play Edax at level L (exactly when at most E empties) to the end;
 * the start position and every later position are recorded with the final result (side-to-move view).
 * usage: playout <startfile> <outdir> <n> <seed> <threads> [-level L] [-exact E] [-eval file]
 *   startfile: records player(8) opponent(8) x(1); n of them are picked at random (all if n >= count)
 *   output: <outdir>/lf<ply>.bin (start positions), <outdir>/lc<ply>.bin (the positions that follow), appended
 */
#define main edax_original_main
#include "all.cpp"
#undef main
#include <omp.h>

#include "evtool.hpp"

using evtool::Rec;
using evtool::mix64;

int main(int argc, char **argv)
{
	if (argc < 6) evtool::die("usage: playout <startfile> <outdir> <n> <seed> <threads> [-level L] [-exact E] [-eval file]");
	const char *start = argv[1], *outdir = argv[2];
	long long n_pick = atoll(argv[3]);
	const unsigned long long seed = strtoull(argv[4], nullptr, 10);
	int nt = atoi(argv[5]), level = 6, exact = 18;
	const char *eval_file = "pw/eval.dat";
	auto value = [&](int &i) -> char * { if (i + 1 >= argc) evtool::die("missing value of %s", argv[i]); return argv[++i]; };
	for (int i = 6; i < argc; ++i) {
		if (!strcmp(argv[i], "-level")) level = atoi(value(i));
		else if (!strcmp(argv[i], "-exact")) exact = atoi(value(i));
		else if (!strcmp(argv[i], "-eval")) eval_file = value(i);
	}
	if (nt < 1) evtool::die("bad threads %s", argv[5]);
	options.n_task = 1;
	options.hash_table_size = 18;
	options.verbosity = 0;
	options.eval_file = (char *) eval_file;
	bit_init();
	edge_stability_init();
	statistics_init();
	eval_open(options.eval_file);
	search_global_init();

	FILE *f = fopen(start, "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", start); return 1; }
	long long n = evtool::file_size(f) / (long long) sizeof (Rec);
	Rec *S = (Rec *) evtool::xmalloc(n * sizeof *S);
	n = (long long) fread(S, sizeof *S, n, f);
	fclose(f);
	if (n_pick > n) n_pick = n;
	// partial Fisher-Yates: the first n_pick records are a random sample
	for (long long i = 0; i < n_pick; ++i) {
		const long long j = i + (long long) (mix64(seed * 0x9E3779B97F4A7C15ULL + i) % (unsigned long long) (n - i));
		const Rec t = S[i]; S[i] = S[j]; S[j] = t;
	}

	FILE *outf[61] = {}, *outc[61] = {};
	for (int p = 0; p <= 60; ++p) {
		char name[1024];
		sprintf(name, "%s/lf%02d.bin", outdir, p); outf[p] = evtool::xfopen(name, "ab");
		sprintf(name, "%s/lc%02d.bin", outdir, p); outc[p] = evtool::xfopen(name, "ab");
	}
	const double t0 = omp_get_wtime();
	long long done = 0, total = 0;

	#pragma omp parallel num_threads(nt)
	{
		Search search;
		search_init(&search);
		search.options.verbosity = 0;
		Rec buf[64];
		signed char side[64];
		long long g;
		#pragma omp for schedule(dynamic, 16)
		for (g = 0; g < n_pick; ++g) {
			Board b;
			int nb = 0, colour = 1;
			b.player = S[g].P; b.opponent = S[g].O;
			if ((b.player & b.opponent) || ((b.player | b.opponent) & 0x0000001818000000ULL) != 0x0000001818000000ULL) continue;
			for (;;) {
				unsigned long long moves = board_get_moves(&b);
				if (!moves) {
					board_pass(&b); colour = -colour;
					if (!board_get_moves(&b)) break;	// game over
					continue;
				}
				int x;
				buf[nb].P = b.player; buf[nb].O = b.opponent; side[nb] = (signed char) colour; ++nb;
				if ((moves & (moves - 1)) == 0) x = first_bit(moves);
				else {
					search_cleanup(&search);
					search_set_board(&search, &b, BLACK);
					search_set_level(&search, search.eval.n_empties <= exact ? 60 : level, search.eval.n_empties);
					search_set_game_time(&search, TIME_MAX);
					search_run(&search);
					x = search.result->move;
				}
				Board next; board_next(&b, x, &next); b = next; colour = -colour;
			}
			int diff = bit_count(b.player) - bit_count(b.opponent);
			const int e = board_count_empties(&b);
			if (diff > 0) diff += e; else if (diff < 0) diff -= e;
			#pragma omp critical
			{
				for (int i = 0; i < nb; ++i) {
					const int ply = 60 - (64 - bit_count(buf[i].P | buf[i].O));
					buf[i].v = (signed char) (side[i] == colour ? diff : -diff);
					// the first record is the start position unless it had to pass (then the passed position is first)
					fwrite(buf + i, sizeof *buf, 1, i == 0 ? outf[ply] : outc[ply]);
				}
				total += nb;
				if (++done % 20000 == 0) { fprintf(stderr, "%lld playouts, %.0f s\n", done, omp_get_wtime() - t0); for (int p = 0; p <= 60; ++p) { fflush(outf[p]); fflush(outc[p]); } }
			}
		}
		search_free(&search);
	}
	bool written = true;
	for (int p = 0; p <= 60; ++p) { written &= fclose(outf[p]) == 0; written &= fclose(outc[p]) == 0; }
	if (!written) evtool::die("cannot write the files of %s", outdir);
	printf("%lld playouts, %lld positions in %.0f s\n", done, total, omp_get_wtime() - t0);
	return 0;
}
