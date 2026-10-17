/**
 * @file bookdump.cpp
 *
 * Dump an Edax book (form 1 or form 2) into per-ply training files <outdir>/bookNN.bin:
 * records of player(8) opponent(8) value(1); positions whose value is outside -64..64 are skipped.
 * usage: bookdump book.dat outdir
 */
#include "evtool.hpp"

using namespace evtool;

int main(int argc, char **argv)
{
	if (argc < 3) die("usage: bookdump book.dat outdir");
	BookReader reader;
	book_reader_open(&reader, argv[1]);
	FILE *out[61] = {};
	long long cnt[61] = {}, skipped = 0;
	char name[512];
	BookRecord p;
	while (book_reader_next(&reader, &p)) {
		if (p.value < -64 || p.value > 64) { ++skipped; continue; }
		Rec r;
		r.P = p.P; r.O = p.O; r.v = (signed char) p.value;
		const int ply = 60 - (64 - popc(r.P | r.O));
		if (!out[ply]) {
			sprintf(name, "%s/book%02d.bin", argv[2], ply);
			out[ply] = xfopen(name, "wb");
			setvbuf(out[ply], nullptr, _IOFBF, 1 << 20);
		}
		fwrite(&r, sizeof r, 1, out[ply]);
		++cnt[ply];
	}
	for (int ply = 0; ply <= 60; ++ply) if (out[ply]) { if (fclose(out[ply]) != 0) die("cannot write the files of %s", argv[2]); printf("ply %2d %lld\n", ply, cnt[ply]); }
	printf("skipped %lld\n", skipped);
	return 0;
}
