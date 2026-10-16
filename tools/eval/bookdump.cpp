/**
 * @file bookdump.cpp
 *
 * Dump an Edax book of form 1 (nikque, 48-byte header) into per-ply training files <outdir>/bookNN.bin:
 * records of player(8) opponent(8) value(1); positions whose value is outside -64..64 are skipped.
 * usage: bookdump book.dat outdir
 */
#include "evtool.hpp"

using namespace evtool;

int main(int argc, char **argv)
{
	if (argc < 3) die("usage: bookdump book.dat outdir");
	FILE *f = xfopen(argv[1], "rb");
	setvbuf(f, nullptr, _IOFBF, 1 << 24);
	unsigned char hdr[42];
	if (fread(hdr, 1, 42, f) != 42) die("cannot read %s", argv[1]);
	book_form1_check(hdr, argv[1]);
	unsigned int n; memcpy(&n, hdr + 38, 4);
	FILE *out[61] = {};
	long long cnt[61] = {}, skipped = 0;
	unsigned char h[40], buf[520];
	char name[512];
	for (unsigned int i = 0; i < n; ++i) {
		if (fread(h, 1, 40, f) != 40) { fprintf(stderr, "truncated at %u\n", i); return 1; }
		const int nl = h[38];
		if (fread(buf, 2, nl + 1, f) != (size_t) nl + 1) { fprintf(stderr, "truncated at %u\n", i); return 1; }
		Rec r; short v;
		memcpy(&r.P, h, 8); memcpy(&r.O, h + 8, 8); memcpy(&v, h + 32, 2);
		if (v < -64 || v > 64) { ++skipped; continue; }
		r.v = (signed char) v;
		const int ply = 60 - (64 - popc(r.P | r.O));
		if (!out[ply]) {
			sprintf(name, "%s/book%02d.bin", argv[2], ply);
			out[ply] = xfopen(name, "wb");
			setvbuf(out[ply], nullptr, _IOFBF, 1 << 20);
		}
		fwrite(&r, sizeof r, 1, out[ply]);
		++cnt[ply];
	}
	for (int p = 0; p <= 60; ++p) if (out[p]) { if (fclose(out[p]) != 0) die("cannot write the files of %s", argv[2]); printf("ply %2d %lld\n", p, cnt[p]); }
	printf("skipped %lld\n", skipped);
	return 0;
}
