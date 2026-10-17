/**
 * @file trans2rec.cpp
 *
 * Training records from game transcripts: one game a line, the moves as "f5d6c3..." (a-h or A-H, then 1-8) from the
 * start position; what follows the moves on the line is not read; empty lines and lines beginning with '#' are skipped.
 * Every position from ply <from> on where the side to move has a legal move is recorded with the final result of the
 * game (disc difference of the side to move, empties to the winner). A game that does not end (no more moves for both)
 * is recorded with the result of its last position.
 * usage: trans2rec <outdir> <prefix> [-from n] [-to n] <transcript file> ...
 *   -from n, -to n: plies of the recorded positions (default 0 and 60)
 * output: <outdir>/<prefix><ply>.bin (records player(8) opponent(8) result(1), appended)
 */
#define main edax_original_main
#include "all.cpp"
#undef main

#include "evtool.hpp"

using evtool::Rec;

int main(int argc, char **argv)
{
	if (argc < 4) evtool::die("usage: trans2rec <outdir> <prefix> [-from n] [-to n] <transcript file> ...");
	const char *outdir = argv[1], *prefix = argv[2];
	int from = 0, to = 60, first_file = 3;
	for (; first_file < argc && argv[first_file][0] == '-'; ++first_file) {
		if (first_file + 1 >= argc) evtool::die("missing value of %s", argv[first_file]);
		if (!strcmp(argv[first_file], "-from")) from = atoi(argv[++first_file]);
		else if (!strcmp(argv[first_file], "-to")) to = atoi(argv[++first_file]);
		else evtool::die("unknown option %s", argv[first_file]);
	}
	if (from < 0 || to < from || to > 60) evtool::die("bad -from / -to (0 <= from <= to <= 60)");
	if (first_file == argc) evtool::die("no transcript file");
	bit_init();

	FILE *out[61] = {};
	long long count[61] = {}, games = 0, unfinished = 0;
	char name[1024];
	static char line[1 << 16];
	for (int a = first_file; a < argc; ++a) {
		FILE *f = evtool::xfopen(argv[a], "r");
		setvbuf(f, nullptr, _IOFBF, 1 << 24);
		long long line_number = 0;
		while (fgets(line, sizeof line, f)) {
			++line_number;
			if (line[0] == '\n' || line[0] == '\r' || line[0] == '#' || line[0] == '\0') continue;
			if (!strchr(line, '\n') && !feof(f)) { fprintf(stderr, "%s:%lld: line too long\n", argv[a], line_number); return 1; }
			Board b;
			board_init(&b);
			Rec rec[64];
			signed char side[64];
			int n = 0, colour = 1;	// 1: black to move
			const char *s = line;
			for (;;) {
				unsigned long long moves = board_get_moves(&b);
				if (!moves) {
					board_pass(&b); colour = -colour;
					moves = board_get_moves(&b);
					if (!moves) break;	// game over
				}
				const int c = s[0] | 0x20;	// (lower case)
				if (c < 'a' || c > 'h' || s[1] < '1' || s[1] > '8') break;	// end of the moves
				const int x = (c - 'a') + 8 * (s[1] - '1');
				if (!(moves & x_to_bit(x))) { fprintf(stderr, "%s:%lld: illegal move %.2s\n", argv[a], line_number, s); return 1; }
				const int ply = 60 - board_count_empties(&b);
				if (ply >= from && ply <= to) { rec[n].P = b.player; rec[n].O = b.opponent; side[n] = (signed char) colour; ++n; }
				Board next; board_next(&b, x, &next); b = next; colour = -colour;
				s += 2;
			}
			if (board_get_moves(&b)) ++unfinished;
			else {
				Board q = { b.opponent, b.player };
				if (board_get_moves(&q)) ++unfinished;
			}
			// result from the point of view of the side "colour" (b.player)
			int diff = bit_count(b.player) - bit_count(b.opponent);
			const int e = board_count_empties(&b);
			if (diff > 0) diff += e; else if (diff < 0) diff -= e;
			for (int i = 0; i < n; ++i) {
				const int ply = 60 - (64 - bit_count(rec[i].P | rec[i].O));
				rec[i].v = (signed char) (side[i] == colour ? diff : -diff);
				if (!out[ply]) {
					sprintf(name, "%s/%s%02d.bin", outdir, prefix, ply);
					out[ply] = evtool::xfopen(name, "ab");
					setvbuf(out[ply], nullptr, _IOFBF, 1 << 20);
				}
				fwrite(rec + i, sizeof *rec, 1, out[ply]);
				++count[ply];
			}
			++games;
		}
		fclose(f);
	}
	long long total = 0;
	for (int p = 0; p <= 60; ++p) if (out[p]) { if (fclose(out[p]) != 0) evtool::die("cannot write the files of %s", outdir); total += count[p]; }
	printf("%lld games (%lld not ended), %lld positions\n", games, unfinished, total);
	return 0;
}
