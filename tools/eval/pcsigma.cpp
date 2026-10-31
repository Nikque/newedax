/**
 * @file pcsigma.cpp
 *
 * Fixed-depth scores without selectivity, to measure the error of the shallow search of probcut.
 * For each record of <in.raw>: the evaluation (depth 0), then the scores at the depths 2 to dmax.
 * usage: pcsigma <in.raw> <out.tsv> <threads> [-eval file] [-dmax d (default 18)] [-h bits (default 22)]
 * output: one line per position: n_empties, s0, s2, s3, ..., s<dmax>
 */
#define main edax_original_main
#include "all.cpp"
#undef main
#include <omp.h>

#include "evtool.hpp"

using evtool::Rec;

static int fixed_score(Search *search, const Board *b, const int depth)
{
	search_cleanup(search);
	search_set_board(search, b, BLACK);
	search->options.depth = depth;
	search->options.selectivity = NO_SELECTIVITY;
	search_set_game_time(search, TIME_MAX);
	search_run(search);
	return search->result->score;
}

int main(int argc, char **argv)
{
	if (argc < 4) evtool::die("usage: pcsigma <in.raw> <out.tsv> <threads> [-eval file] [-dmax d] [-h bits]");
	const char *in = argv[1], *outname = argv[2];
	const int nt = atoi(argv[3]);
	int dmax = 18, hash_bits = 22;
	const char *eval_file = "eval_orig.dat";
	auto value = [&](int &i) -> char * { if (i + 1 >= argc) evtool::die("missing value of %s", argv[i]); return argv[++i]; };
	for (int i = 4; i < argc; ++i) {
		if (!strcmp(argv[i], "-eval")) eval_file = value(i);
		else if (!strcmp(argv[i], "-dmax")) dmax = atoi(value(i));
		else if (!strcmp(argv[i], "-h")) hash_bits = atoi(value(i));
	}
	if (nt < 1) evtool::die("bad threads %s", argv[3]);
	if (dmax < 2 || dmax > 40) evtool::die("bad -dmax (2 to 40)");

	options.n_task = 1;
	options.hash_table_size = hash_bits;
	options.verbosity = 0;
	options.eval_file = (char *) eval_file;
	bit_init();
	edge_stability_init();
	statistics_init();
	eval_open(options.eval_file);
	search_global_init();

	FILE *f = evtool::xfopen(in, "rb");
	long long n = evtool::file_size(f) / (long long) sizeof (Rec);
	Rec *R = (Rec *) evtool::xmalloc(n * sizeof *R);
	n = (long long) fread(R, sizeof *R, n, f);
	fclose(f);
	FILE *g = evtool::xfopen(outname, "w");

	const double t0 = omp_get_wtime();
	#pragma omp parallel num_threads(nt)
	{
		Search search;
		search_init(&search);
		search.options.verbosity = 0;
		long long k;
		#pragma omp for schedule(dynamic, 1)
		for (k = 0; k < n; ++k) {
			Board b; b.player = R[k].P; b.opponent = R[k].O;
			if (!board_get_moves(&b)) continue;
			char line[512];
			search_set_board(&search, &b, BLACK);
			int len = sprintf(line, "%d\t%d", (int) search.eval.n_empties, search_eval_0(&search));
			for (int d = 2; d <= dmax; ++d) len += sprintf(line + len, "\t%d", fixed_score(&search, &b, d));
			#pragma omp critical
			fprintf(g, "%s\n", line);
		}
		search_free(&search);
	}
	if (fclose(g) != 0) evtool::die("cannot write %s", outname);
	printf("%s: %lld positions in %.0f s\n", outname, n, omp_get_wtime() - t0);
	return 0;
}
