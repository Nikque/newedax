/**
 * @file bookscan.cpp
 *
 * Scan an Edax book (form 1 or form 2): per-ply statistics (positions, positions with a leaf, exact values, levels).
 * usage: bookscan book.dat
 */
#include "evtool.hpp"

using namespace evtool;

int main(int argc, char **argv)
{
	if (argc < 2) die("usage: bookscan book.dat");
	BookReader reader;
	book_reader_open(&reader, argv[1]);
	printf("positions %llu level %d n_empties %d\n", (unsigned long long) reader.n_positions, reader.level, reader.n_empties);
	static long long cnt[65], leafok[65], lv[65][61], exact[65];	// index: the discs of the board (the ply + 4; 0 to 3 in a damaged book)
	BookRecord p;
	while (book_reader_next(&reader, &p)) {
		const int d = popc(p.P | p.O);
		cnt[d]++; lv[d][p.level > 60 ? 60 : p.level]++;
		if (p.leaf_score > -65) leafok[d]++;
		if (p.lower == p.upper) exact[d]++;
	}
	if (cnt[0] + cnt[1] + cnt[2] + cnt[3]) die("%s is damaged (a board of fewer than 4 discs)", argv[1]);
	for (int ply = 0; ply <= 60; ++ply) if (cnt[ply + 4]) {
		const int d = ply + 4;
		printf("ply %2d n %11lld leaf %11lld lo==up %11lld  levels:", ply, cnt[d], leafok[d], exact[d]);
		for (int l = 0; l <= 60; ++l) if (lv[d][l]) printf(" %d:%lld", l, lv[d][l]);
		printf("\n");
	}
	return 0;
}
