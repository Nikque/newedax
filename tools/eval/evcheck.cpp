/**
 * @file evcheck.cpp
 *
 * Check the features and sums of evtool.hpp against Edax's own evaluation (accumlate_eval, and mobility_eval when the
 * file has the mobility weights of eval2: a build with EVAL_MOBILITY) on the positions of random games, and the eval.dat
 * round trip of evtool (written to roundtrip.dat, the pattern weights).
 * usage: evcheck [eval.dat [n_games]]
 */
#define main edax_original_main
#include "all.cpp"
#undef main

#include "evtool.hpp"

int main(int argc, char **argv)
{
	const char *file = argc > 1 ? argv[1] : "eval.dat";
	const int n_games = argc > 2 ? atoi(argv[2]) : 20000;
	fprintf(stderr, "open\n");
	bit_init(); edge_stability_init();
	eval_open(file);
	fprintf(stderr, "opened\n");
	evtool::et_init();
	evtool::ET_Eval *e = evtool::et_read(file);
	evtool::et_write(e, "roundtrip.dat");
	fprintf(stderr, "written\n");
#if EVAL_MOBILITY
	if (e->has_mob != EVAL_HAS_MOBILITY) { printf("mobility weights: Edax %d, evtool %d\n", (int) EVAL_HAS_MOBILITY, (int) e->has_mob); return 1; }
#else
	if (e->has_mob) { printf("the mobility weights need a build with EVAL_MOBILITY\n"); return 1; }
#endif
	if (e->has_mob) fprintf(stderr, "mobility weights compared too\n");

	unsigned long long seed = 12345, bad = 0, n = 0;
	for (int game = 0; game < n_games; ++game) {
		Board b; board_init(&b);
		for (;;) {
			unsigned long long moves = board_get_moves(&b);
			if (!moves) {
				board_pass(&b);
				if (!board_get_moves(&b)) break;
				continue;
			}

			Eval ev; ev.n_empties = board_count_empties(&b);
			eval_set(&ev, &b);
			int ref = accumlate_eval(60 - ev.n_empties, &ev);
#if EVAL_MOBILITY
			if (EVAL_HAS_MOBILITY) ref += mobility_eval(60 - ev.n_empties, b.player, b.opponent);
#endif

			int idx[evtool::ET_NF];
			evtool::et_features(b.player, b.opponent, idx);
			int mine = evtool::et_sum(e->w[evtool::et_ply(ev.n_empties)], idx);
			if (e->has_mob) mine += evtool::et_mob(e, b.player, b.opponent);
			if (ref != mine && bad++ < 10) printf("mismatch empties %d ref %d mine %d\n", ev.n_empties, ref, mine);
			++n;
			seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
			int r = (int) ((seed >> 33) % (unsigned) bit_count(moves));
			while (r--) moves &= moves - 1;
			const int x = first_bit(moves);
			Board nb; board_next(&b, x, &nb); b = nb;
		}
	}
	fprintf(stderr, "checked %llu positions, %llu mismatches\n", n, bad);
	return bad != 0;
}
