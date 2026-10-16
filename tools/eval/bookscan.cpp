/**
 * @file bookscan.cpp
 *
 * Scan an Edax book of form 1 (nikque): per-ply statistics (positions, positions with a leaf, exact values, levels).
 * usage: bookscan book.dat
 */
#include "evtool.hpp"

using namespace evtool;

int main(int argc, char **argv)
{
	if (argc < 2) die("usage: bookscan book.dat");
	FILE *f = xfopen(argv[1], "rb");
	setvbuf(f, nullptr, _IOFBF, 1 << 24);
	unsigned char hdr[42];
	if (fread(hdr, 1, 42, f) != 42) die("cannot read %s", argv[1]);
	book_form1_check(hdr, argv[1]);
	unsigned int n; memcpy(&n, hdr + 38, 4);
	printf("positions %u level %d n_empties %d\n", n, hdr[18], hdr[22]);
	static long long cnt[61], leafok[61], lv[61][61], exact[61];
	unsigned char h[40], buf[520];
	for (unsigned int i = 0; i < n; ++i) {
		if (fread(h, 1, 40, f) != 40) { printf("trunc %u\n", i); break; }
		uint64_t P, O; memcpy(&P, h, 8); memcpy(&O, h + 8, 8);
		const int nl = h[38]; fread(buf, 2, nl + 1, f);
		short lo, up; memcpy(&lo, h + 34, 2); memcpy(&up, h + 36, 2);
		const int e = 64 - popc(P | O), ply = 60 - e;
		cnt[ply]++; lv[ply][h[39] > 60 ? 60 : h[39]]++;
		const signed char ls = (signed char) buf[2 * nl];
		if (ls > -65) leafok[ply]++;
		if (lo == up) exact[ply]++;
	}
	for (int p = 0; p <= 60; ++p) if (cnt[p]) {
		printf("ply %2d n %11lld leaf %11lld lo==up %11lld  levels:", p, cnt[p], leafok[p], exact[p]);
		for (int l = 0; l <= 60; ++l) if (lv[p][l]) printf(" %d:%lld", l, lv[p][l]);
		printf("\n");
	}
	return 0;
}
