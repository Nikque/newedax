/**
 * @file gen.cpp
 *
 * Generate exactly solved positions beyond the book: walks from book positions of a ply (training split only),
 * each move being the best move (probability -best) or a random move, every position solved by Edax.
 * usage: gen <startfile.bin> <outdir> <n_walks> <seed> <threads> [-best p] [-last ply] [-eval file (default eval_orig.dat)]
 * output: <outdir>/gen<ply>.bin (records player(8) opponent(8) score(1), appended); the files of plies 41 to last are
 * created at the start, the files of the other plies when they get a record.
 */
#define main edax_original_main
#include "all.cpp"
#undef main
#include <omp.h>

#include "evtool.hpp"

using evtool::Rec;
using evtool::mix64;

static int solve(Search *search, const Board *b, int *best)
{
	search_cleanup(search);
	search_set_board(search, b, BLACK);
	search_set_level(search, 60, search->eval.n_empties);
	search_set_game_time(search, TIME_MAX);
	search_run(search);
	*best = search->result->move;
	return search->result->score;
}

int main(int argc, char **argv)
{
	if (argc < 6) evtool::die("usage: gen <startfile.bin> <outdir> <n_walks> <seed> <threads> [-best p] [-last ply] [-eval file]");
	const char *start = argv[1], *outdir = argv[2];
	const long long n_walks = atoll(argv[3]);
	const unsigned long long seed = strtoull(argv[4], nullptr, 10);
	int nt = atoi(argv[5]), last = 53;
	double p_best = 0.5;
	const char *eval_file = "eval_orig.dat";
	auto value = [&](int &i) -> char * { if (i + 1 >= argc) evtool::die("missing value of %s", argv[i]); return argv[++i]; };
	for (int i = 6; i < argc; ++i) {
		if (!strcmp(argv[i], "-best")) p_best = atof(value(i));
		else if (!strcmp(argv[i], "-last")) last = atoi(value(i));
		else if (!strcmp(argv[i], "-eval")) eval_file = value(i);
	}
	if (nt < 1) evtool::die("bad threads %s", argv[5]);
	if (last > 60) last = 60;

	options.n_task = 1;
	options.hash_table_size = 18;
	options.verbosity = 0;
	options.eval_file = (char *) eval_file;
	bit_init();
	edge_stability_init();
	statistics_init();
	eval_open(options.eval_file);
	search_global_init();

	// start positions: training split only (hash % 32 != 0, as in evtrain)
	FILE *f = evtool::xfopen(start, "rb");
	long long n = evtool::file_size(f) / (long long) sizeof (Rec);
	Rec *S = (Rec *) evtool::xmalloc(n * sizeof *S);
	n = (long long) fread(S, sizeof *S, n, f);
	fclose(f);
	long long n_train = 0;
	for (long long i = 0; i < n; ++i) n_train += mix64(S[i].P * 0x9E3779B97F4A7C15ULL ^ S[i].O) % 32 != 0;
	if (n_train == 0) evtool::die("%s has no position of the training split", start);

	FILE *out[61] = {};
	long long count[61] = {};
	char name[61][1024];
	for (int p = 0; p <= 60; ++p) {
		sprintf(name[p], "%s/gen%02d.bin", outdir, p);
		if (p > 40 && p <= last) out[p] = evtool::xfopen(name[p], "ab");
	}
	const double t0 = omp_get_wtime();
	long long done = 0;

	#pragma omp parallel num_threads(nt)
	{
		Search search;
		search_init(&search);
		search.options.verbosity = 0;
		Rec buf[64];
		int nb = 0;
		long long w;
		#pragma omp for schedule(dynamic, 16)
		for (w = 0; w < n_walks; ++w) {
			unsigned long long r = mix64(seed * 0x9E3779B97F4A7C15ULL + w);
			const Rec *s;
			do { r = mix64(r); s = S + r % n; } while (mix64(s->P * 0x9E3779B97F4A7C15ULL ^ s->O) % 32 == 0);
			Board b; b.player = s->P; b.opponent = s->O;
			bool first = true;
			for (;;) {
				unsigned long long moves = board_get_moves(&b);
				if (!moves) {
					board_pass(&b);
					if (!board_get_moves(&b)) break;	// game over
					continue;
				}
				int best = NOMOVE, x;
				const int ply = 60 - board_count_empties(&b);
				if (!first) {
					if (ply > last) break;
					const int v = solve(&search, &b, &best);
					buf[nb].P = b.player; buf[nb].O = b.opponent; buf[nb].v = (signed char) v;
					++nb;
				}
				r = mix64(r);
				if (!first && best != NOMOVE && (double) (r >> 11) / 9007199254740992.0 < p_best && (moves & x_to_bit(best))) x = best;
				else {
					r = mix64(r);
					int k = (int) (r % (unsigned) bit_count(moves));
					while (k--) moves &= moves - 1;
					x = first_bit(moves);
				}
				Board next; board_next(&b, x, &next); b = next;
				first = false;
			}
			#pragma omp critical
			{
				for (int i = 0; i < nb; ++i) {
					const int ply = 60 - (64 - bit_count(buf[i].P | buf[i].O));
					if (!out[ply]) out[ply] = evtool::xfopen(name[ply], "ab");
					fwrite(buf + i, sizeof *buf, 1, out[ply]);
					++count[ply];
				}
				if (++done % 20000 == 0) { fprintf(stderr, "%lld walks, %.0f s\n", done, omp_get_wtime() - t0); }
			}
			nb = 0;
		}
		search_free(&search);
	}
	for (int p = 0; p <= 60; ++p) if (out[p]) { if (fclose(out[p]) != 0) evtool::die("cannot write %s", name[p]); if (count[p]) printf("ply %d: %lld\n", p, count[p]); }
	printf("%lld walks in %.0f s\n", n_walks, omp_get_wtime() - t0);
	return 0;
}
