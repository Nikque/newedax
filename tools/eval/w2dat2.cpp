/**
 * @file w2dat2.cpp
 *
 * Convert weights dumped by evsearch (-dump: <cfg>.pat, <cfg>.scl, <cfg>_<ply>.w) for a configuration made of
 * Edax's 46 features (+ optional "mob" or "mobp") into an Edax eval.dat of form 1; the mobility weights are appended to
 * the file ("MOBW" then 61 x 64 shorts: side to move [0..31], opponent [32..63]), as read by eval2.
 * usage: w2dat2 <template eval.dat> <dir> <cfg> <out.dat> [version x.y.z]
 */
#include <cmath>

#include "evtool.hpp"

using namespace evtool;

int main(int argc, char **argv)
{
	if (argc < 5) { fprintf(stderr, "usage\n"); return 1; }
	et_init();
	ET_Eval *e = et_read(argv[1]);
	const char *dir = argv[2], *cfg = argv[3];
	char name[1024];
	int n_inst, nw, sc[32] = {0};
	sprintf(name, "%s/%s.pat", dir, cfg);
	FILE *f = fopen(name, "rb");
	if (!f) { fprintf(stderr, "cannot open %s\n", name); return 1; }
	xfread(&n_inst, 4, 1, f, name); xfread(&nw, 4, 1, f, name);
	if (n_inst < 1 || n_inst > 256 || nw < 1) die("bad instances in %s", name);
	int *in = (int *) xmalloc(n_inst * sizeof *in), *io = (int *) xmalloc(n_inst * sizeof *io);
	unsigned char (*isq)[16] = (unsigned char (*)[16]) xmalloc(n_inst * 16);
	for (int j = 0; j < n_inst; ++j) {
		xfread(in + j, 4, 1, f, name); xfread(io + j, 4, 1, f, name); xfread(isq[j], 1, 16, f, name);
		int size = 1;
		for (int i = 0; i < in[j] && i < 16; ++i) size *= 3;
		if (in[j] < 1 || in[j] > 16 || io[j] < 1 || io[j] > nw - size) die("bad instances in %s", name);
		for (int i = 0; i < in[j]; ++i) if (isq[j][i] > 63) die("bad instances in %s", name);
	}
	fclose(f);
	sprintf(name, "%s/%s.scl", dir, cfg);
	if ((f = fopen(name, "rb"))) {
		const size_t r = fread(sc, 4, 32, f);	// 11 scalar features of evsearch: (used, offset) each
		fclose(f);
		if (r < 4 || r % 2) die("bad scalar features in %s", name);
	}
	const int has_mob = sc[0] | sc[2], mob_ofs = sc[0] ? sc[1] : sc[3], mob_n = sc[0] ? 64 : 32;	// mob (both sides) or mobp (side to move only)
	for (int i = 4; i < 32; i += 2) if (sc[i]) { fprintf(stderr, "only the mob / mobp scalars are supported\n"); return 1; }
	if (has_mob && (mob_ofs < 1 || mob_ofs > nw - mob_n)) die("bad scalar features in %s", name);
	static short mob[61][64];
	double *w = (double *) xmalloc(nw * sizeof *w);
	double maxdiff = 0;
	int covered[13] = {0};
	for (int ply = 2; ply <= 53; ++ply) {
		sprintf(name, "%s/%s_%02d.w", dir, cfg, ply);
		if (!(f = fopen(name, "rb"))) { fprintf(stderr, "cannot open %s\n", name); return 1; }
		xfread(w, 8, nw, f, name);
		fclose(f);
		short *ew = e->w[ply];
		ew[ET_NW - 1] = (short) lrint(w[0] * 128);
		for (int j = 0; j < n_inst; ++j) {
			if (j > 0 && io[j] == io[j - 1]) continue;
			const int n = in[j];
			uint64_t set = 0;
			for (int i = 0; i < n; ++i) set |= 1ULL << isq[j][i];
			int found = 0;
			for (int ft = 0; ft < ET_NF && !found; ++ft) {
				const ET_F2X *x = ET_FEAT + ft;
				uint64_t es = 0;
				for (int i = 0; i < x->n; ++i) es |= 1ULL << x->x[i];
				if (x->n != n || es != set) continue;
				int pw[16], size = 1;
				for (int i = 0; i < n; ++i) {
					int k = 0;
					while (x->x[k] != isq[j][i]) ++k;
					pw[i] = 1;
					for (int m = k + 1; m < n; ++m) pw[i] *= 3;
					size *= 3;
				}
				const int cl = ET_CLASS[ft];
				char *seen = (char *) calloc(ET_PSIZE[cl], 1);
				for (int v = 0; v < size; ++v) {
					int y = v, xe = 0;
					for (int i = n - 1; i >= 0; --i) { xe += (y % 3) * pw[i]; y /= 3; }
					long q = lrint(w[io[j] + v] * 128);
					if (q > 32767) q = 32767;
					if (q < -32768) q = -32768;
					const int pi = ET_PACK[cl][xe];
					if (seen[pi]) { const double d = fabs((double) ew[pi + ET_POFS[cl]] - q); if (d > maxdiff) maxdiff = d; }
					seen[pi] = 1;
					ew[pi + ET_POFS[cl]] = (short) q;
				}
				free(seen);
				covered[cl] = 1; found = 1;
			}
			if (!found) { fprintf(stderr, "instance %d is not an Edax feature\n", j); return 1; }
		}
		if (has_mob) for (int i = 0; i < mob_n; ++i) mob[ply][i] = (short) lrint(w[mob_ofs + i] * 128);
	}
	for (int c = 0; c < 12; ++c) if (!covered[c]) { fprintf(stderr, "Edax class %d is not in the configuration\n", c); return 1; }
	for (int ply = 0; ply < 2; ++ply) { memcpy(e->w[ply], e->w[ply + 2], sizeof e->w[0]); memcpy(mob[ply], mob[ply + 2], sizeof mob[0]); }
	for (int ply = 54; ply < ET_NPLY; ++ply) { memcpy(e->w[ply], e->w[ply - 2], sizeof e->w[0]); memcpy(mob[ply], mob[ply - 2], sizeof mob[0]); }
	if (argc > 5) {	// version x.y.z of the evaluation file
		unsigned v = 0, r = 0, b = 0;
		sscanf(argv[5], "%u.%u.%u", &v, &r, &b);
		e->version = v; e->release = r; e->build = b;
	}
	et_write(e, argv[4]);
	printf("%s written (largest difference between symmetric entries: %.0f)\n", argv[4], maxdiff);
	if (has_mob) {	// mobility weights appended to the evaluation file: "MOBW" then 61 x 64 shorts (same byte order as the file)
		const unsigned int tag = 0x4d4f4257;
		f = xfopen(argv[4], "ab");
		fwrite(&tag, 4, 1, f);
		fwrite(mob, sizeof mob, 1, f);
		if (fclose(f) != 0) die("cannot write %s", argv[4]);
		printf("mobility weights appended\n");
	}
	return 0;
}
