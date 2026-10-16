/**
 * @file selfplay3.cpp
 *
 * Self-play training data: games from the start position, the first k plies random (k uniform in [rmin, rmax]),
 * then both sides play Edax at level L (exactly when at most E empties). Every position after the random part is
 * recorded with the final result of the game (disc difference of the side to move, empties to the winner) and with
 * the score of the search that chose the move.
 * usage: selfplay3 <outdir> <n_games> <seed> <threads> [-level L] [-rmin a] [-rmax b] [-exact E] [-eval file]
 * output: <outdir>/sp<ply>.bin (records player(8) opponent(8) final result(1), appended)
 *         <outdir>/ss<ply>.bin (same positions, with the score of the search that chose the move)
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
	if (argc < 5) evtool::die("usage: selfplay3 <outdir> <n_games> <seed> <threads> [-level L] [-rmin a] [-rmax b] [-exact E] [-eval file]");
	const char *outdir = argv[1];
	const long long n_games = atoll(argv[2]);
	const unsigned long long seed = strtoull(argv[3], nullptr, 10);
	int nt = atoi(argv[4]), level = 8, rmin = 4, rmax = 32, exact = 0;	// -exact E: solve exactly when at most E empties
	const char *eval_file = "pw/eval.dat";
	auto value = [&](int &i) -> char * { if (i + 1 >= argc) evtool::die("missing value of %s", argv[i]); return argv[++i]; };
	for (int i = 5; i < argc; ++i) {
		if (!strcmp(argv[i], "-level")) level = atoi(value(i));
		else if (!strcmp(argv[i], "-rmin")) rmin = atoi(value(i));
		else if (!strcmp(argv[i], "-rmax")) rmax = atoi(value(i));
		else if (!strcmp(argv[i], "-eval")) eval_file = value(i);
		else if (!strcmp(argv[i], "-exact")) exact = atoi(value(i));
	}
	if (rmin < 0 || rmax < rmin || rmax > 60) evtool::die("bad -rmin / -rmax (0 <= rmin <= rmax <= 60)");
	if (nt < 1) evtool::die("bad threads %s", argv[4]);

	options.n_task = 1;
	options.hash_table_size = 18;
	options.verbosity = 0;
	options.eval_file = (char *) eval_file;
	bit_init();
	edge_stability_init();
	statistics_init();
	eval_open(options.eval_file);
	search_global_init();

	FILE *out[61] = {}, *outs[61] = {};
	long long count[61] = {};
	for (int p = 0; p <= 60; ++p) {
		char name[1024];
		sprintf(name, "%s/sp%02d.bin", outdir, p);
		out[p] = evtool::xfopen(name, "ab");
		sprintf(name, "%s/ss%02d.bin", outdir, p);
		outs[p] = evtool::xfopen(name, "ab");
	}
	const double t0 = omp_get_wtime();
	long long done = 0;

	#pragma omp parallel num_threads(nt)
	{
		Search search;
		search_init(&search);
		search.options.verbosity = 0;
		Rec buf[64];
		signed char side[64], sscore[64];
		long long g;
		#pragma omp for schedule(dynamic, 16)
		for (g = 0; g < n_games; ++g) {
			unsigned long long r = mix64(seed * 0x9E3779B97F4A7C15ULL + g);
			const int k = rmin + (int) (mix64(r ^ 0x5555) % (unsigned) (rmax - rmin + 1));
			Board b;
			int nb = 0, colour = 1;	// 1: black to move
			board_init(&b);
			for (;;) {
				unsigned long long moves = board_get_moves(&b);
				if (!moves) {
					board_pass(&b); colour = -colour;
					if (!board_get_moves(&b)) break;	// game over
					continue;
				}
				const int ply = 60 - board_count_empties(&b);
				int x;
				if (ply < k) {
					r = mix64(r);
					int j = (int) (r % (unsigned) bit_count(moves));
					while (j--) moves &= moves - 1;
					x = first_bit(moves);
				} else {
					buf[nb].P = b.player; buf[nb].O = b.opponent; side[nb] = (signed char) colour;
					search_cleanup(&search);
					search_set_board(&search, &b, BLACK);
					search_set_level(&search, search.eval.n_empties <= exact ? 60 : level, search.eval.n_empties);
					search_set_game_time(&search, TIME_MAX);
					search_run(&search);
					x = search.result->move;
					sscore[nb] = (signed char) search.result->score;
					++nb;
				}
				Board next; board_next(&b, x, &next); b = next; colour = -colour;
			}
			// final result from the point of view of the side "colour" (b.player)
			int diff = bit_count(b.player) - bit_count(b.opponent);
			const int e = board_count_empties(&b);
			if (diff > 0) diff += e; else if (diff < 0) diff -= e;
			#pragma omp critical
			{
				for (int i = 0; i < nb; ++i) {
					const int ply = 60 - (64 - bit_count(buf[i].P | buf[i].O));
					buf[i].v = (signed char) (side[i] == colour ? diff : -diff);
					fwrite(buf + i, sizeof *buf, 1, out[ply]);
					buf[i].v = sscore[i];
					fwrite(buf + i, sizeof *buf, 1, outs[ply]);
					++count[ply];
				}
				if (++done % 20000 == 0) { fprintf(stderr, "%lld games, %.0f s\n", done, omp_get_wtime() - t0); for (int p = 0; p <= 60; ++p) { fflush(out[p]); fflush(outs[p]); } }
			}
		}
		search_free(&search);
	}
	long long total = 0;
	bool written = true;
	for (int p = 0; p <= 60; ++p) { written &= fclose(out[p]) == 0; written &= fclose(outs[p]) == 0; total += count[p]; }
	if (!written) evtool::die("cannot write the files of %s", outdir);
	printf("%lld games, %lld positions in %.0f s\n", n_games, total, omp_get_wtime() - t0);
	return 0;
}
