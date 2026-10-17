/**
 * @file sprepair.cpp
 *
 * Repair self-play record files whose 17-byte record alignment may be broken (two writers appended to one file):
 * scan with resynchronisation, keep only valid records (disjoint bitboards, disc count = ply + 4, centre occupied, |v| <= 64).
 * usage: sprepair <indir> <outdir> <prefix>
 */
#include "evtool.hpp"

using namespace evtool;

static int valid(const unsigned char *p, int ply)
{
	uint64_t P, O;
	memcpy(&P, p, 8); memcpy(&O, p + 8, 8);
	const signed char v = (signed char) p[16];
	return !(P & O) && popc(P | O) == ply + 4 && ((P | O) & 0x0000001818000000ULL) == 0x0000001818000000ULL && v >= -64 && v <= 64;
}

int main(int argc, char **argv)
{
	long long tot_in = 0, tot_out = 0, tot_skip = 0;
	if (argc < 4) die("usage: sprepair <indir> <outdir> <prefix>");
	for (int ply = 0; ply <= 60; ++ply) {
		char name[1024];
		sprintf(name, "%s/%s%02d.bin", argv[1], argv[3], ply);
		FILE *f = fopen(name, "rb");
		if (!f) continue;
		long long n = file_size(f);
		unsigned char *d = (unsigned char *) xmalloc(n + 17);
		n = (long long) fread(d, 1, n, f);
		fclose(f);
		sprintf(name, "%s/%s%02d.bin", argv[2], argv[3], ply);
		FILE *g = xfopen(name, "wb");
		long long pos = 0, kept = 0, skipped = 0;
		while (pos + 17 <= n) {
			if (valid(d + pos, ply)) { fwrite(d + pos, 1, 17, g); ++kept; pos += 17; }
			else { ++pos; ++skipped; }
		}
		skipped += n - pos;
		if (fclose(g) != 0) die("cannot write %s", name);
		free(d);
		if (n) printf("ply %2d: %lld bytes, %lld records kept, %lld bytes skipped\n", ply, n, kept, skipped);
		tot_in += n; tot_out += kept; tot_skip += skipped;
	}
	printf("total: %lld bytes in, %lld records kept, %lld bytes skipped\n", tot_in, tot_out, tot_skip);
	return 0;
}
