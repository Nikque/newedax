/**
 * @file sample.cpp
 *
 * Sample held-out (test split: hash % 32 == 0) positions of a data file as OBF lines (the values go to stderr).
 * usage: sample <file.bin> <n> <seed>  > out.obf
 */
#include "evtool.hpp"

using namespace evtool;

int main(int argc, char **argv)
{
	(void) argc;
	FILE *f = fopen(argv[1], "rb");
	const int n = atoi(argv[2]);
	const uint64_t seed = strtoull(argv[3], nullptr, 10);
	uint64_t k = 0;
	Rec *res = (Rec *) malloc(n * sizeof *res), r;
	setvbuf(f, nullptr, _IOFBF, 1 << 24);
	while (fread(&r, sizeof r, 1, f) == 1) {
		const uint64_t h = mix64(r.P * 0x9E3779B97F4A7C15ULL ^ r.O);
		if (h % 32) continue;
		if (k < (uint64_t) n) res[k] = r;
		else { const uint64_t j = mix64(h ^ seed ^ k) % (k + 1); if (j < (uint64_t) n) res[j] = r; }
		++k;
	}
	for (int i = 0; i < n && i < (int) k; ++i) {
		char s[65];
		for (int x = 0; x < 64; ++x) s[x] = (res[i].P >> x) & 1 ? 'X' : (res[i].O >> x) & 1 ? 'O' : '-';
		s[64] = 0;
		printf("%s X;\n", s); fprintf(stderr, "%d\n", res[i].v);
	}
	return 0;
}
