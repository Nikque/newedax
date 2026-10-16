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
	static long long cnt[61], leafok[61], lv[61][61], exact[61];
	BookRecord p;
	while (book_reader_next(&reader, &p)) {
		const int e = 64 - popc(p.P | p.O), ply = 60 - e;
		cnt[ply]++; lv[ply][p.level > 60 ? 60 : p.level]++;
		if (p.leaf_score > -65) leafok[ply]++;
		if (p.lower == p.upper) exact[ply]++;
	}
	for (int ply = 0; ply <= 60; ++ply) if (cnt[ply]) {
		printf("ply %2d n %11lld leaf %11lld lo==up %11lld  levels:", ply, cnt[ply], leafok[ply], exact[ply]);
		for (int l = 0; l <= 60; ++l) if (lv[ply][l]) printf(" %d:%lld", l, lv[ply][l]);
		printf("\n");
	}
	return 0;
}
