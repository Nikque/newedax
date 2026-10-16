/**
 * @file evcheck.cpp
 *
 * Check the features and sums of evtool.hpp against Edax's own evaluation (accumlate_eval) on the positions of random
 * games, and the eval.dat round trip of evtool (written to roundtrip.dat).
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
			const int ref = accumlate_eval(60 - ev.n_empties, &ev);

			int idx[evtool::ET_NF];
			evtool::et_features(b.player, b.opponent, idx);
			const int mine = evtool::et_sum(e->w[evtool::et_ply(ev.n_empties)], idx);
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
