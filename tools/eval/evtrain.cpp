/**
 * @file evtrain.cpp
 *
 * Train Edax eval.dat weights (same patterns) by regularized least squares, one ply at a time.
 *
 * For each trained ply p, minimize  sum_s c_s (x_s . w - y_s)^2 + lambda * sum_i (w_i - w0_i)^2
 * where x_s counts the packed pattern indices of sample s (side-to-move view), y_s its score in discs,
 * w0 the prior weights (from -init, in discs = short / 128), by Jacobi-preconditioned conjugate gradient.
 * Samples of ply q != p are used with weight c = decay^|q - p| (window option).
 *
 * usage: evtrain -init old.dat -out new.dat -src prefix:dir:weight[:max] [-src ...] -plies a-b [options]
 *   -src           data files dir/<prefix>NN.bin, sample weight, at most max training samples per ply
 *                  (default 30000000); errors are reported for each source's test samples
 *   -window k      use plies p-k..p+k (default 0)     -decay d   weight per ply distance (default 0.5)
 *   -lambda l      L2 toward the prior (default 1.0)    -iter n    CG iterations (default 60)
 *   -threads n     (default 24)                         -eval      only report the prior's errors
 *   -test m        1 in m samples held out for test (by hash, default 32)
 *   -calib d       shift the constant weight so that the bias on the test samples of source d is 0
 */
#include <cmath>
#include <omp.h>

#include "evtool.hpp"

using namespace evtool;

struct Set {
	int n, cap;
	unsigned int *idx;  // ET_NF per sample
	float *y, *c;
};

// more room for the samples of a set (outside set_push, which is called for each sample)
static EVTOOL_NOINLINE void set_grow(Set *s)
{
	s->cap = grow(s->cap, 1 << 20);
	s->idx = (unsigned int *) xrealloc(s->idx, (size_t) s->cap * ET_NF * sizeof *s->idx);
	s->y = (float *) xrealloc(s->y, (size_t) s->cap * sizeof *s->y);
	s->c = (float *) xrealloc(s->c, (size_t) s->cap * sizeof *s->c);
}

static void set_push(Set *s, const Rec *r, float c)
{
	if (s->n == s->cap) set_grow(s);
	int f[ET_NF];
	et_features(r->P, r->O, f);
	for (int i = 0; i < ET_NF; ++i) s->idx[(size_t) s->n * ET_NF + i] = (unsigned int) f[i];
	s->y[s->n] = r->v;
	s->c[s->n] = c;
	++s->n;
}

static Source src[8];
static int n_src;
static int test_mod = 32;

// load samples of one ply of every source into train (weight c * source weight) and per-source test sets
static void load_ply(int q, double c, Set *train, Set *test)
{
	for (int d = 0; d < n_src; ++d) {
		char name[1024];
		sprintf(name, "%s/%s%02d.bin", src[d].dir, src[d].prefix, q);
		FILE *f = fopen(name, "rb");
		if (!f) continue;
		const long long n = file_size(f) / (long long) sizeof (Rec);
		// keep 1 in 'keep' training samples to respect the maximum (deterministic by hash)
		const double keep = n > src[d].max ? (double) src[d].max / n : 1.0;
		const uint64_t thr = keep >= 1.0 ? ~0ULL : (uint64_t) (keep * 18446744073709551615.0);	// (2^64 as a double does not fit)
		const float w = (float) (c * src[d].weight);
		Rec *buf = (Rec *) xmalloc(sizeof (Rec) << 20);
		size_t m;
		while ((m = fread(buf, sizeof (Rec), 1 << 20, f)) > 0) {
			for (size_t i = 0; i < m; ++i) {
				const uint64_t h = mix64(buf[i].P * 0x9E3779B97F4A7C15ULL ^ buf[i].O);
				if (h % test_mod == 0) { if (test && (n < 4000000 || (mix64(h) >> 40) % 4 == 0)) set_push(test + d, buf + i, 1.0f); }
				else if (w > 0 && mix64(h ^ 0x1234567ULL) <= thr) set_push(train, buf + i, w);
			}
		}
		free(buf);
		fclose(f);
	}
}

// errors of weights w (discs) on a set: mse of the raw prediction, mae of Edax's rounded score
static void errors(const Set *s, const double *w, double *mse, double *mae, double *bias)
{
	double se = 0, ae = 0, b = 0;
	int k;
	#pragma omp parallel for reduction(+:se,ae,b) schedule(static)
	for (k = 0; k < s->n; ++k) {
		const unsigned int *x = s->idx + (size_t) k * ET_NF;
		double p = w[ET_NW - 1];
		for (int i = 0; i < ET_NF; ++i) p += w[x[i]];
		const double e = p - s->y[k];
		se += e * e; b += e;
		// Edax rounding of the 1/128 integer sum
		const int sum = (int) floor(p * 128 + 0.5);
		ae += fabs((double) et_score(sum) - s->y[k]);
	}
	*mse = se / s->n; *mae = ae / s->n; *bias = b / s->n;
}

// sum the per-thread buffers
static void reduce(double *out, double **t, int nt)
{
	int i;
	#pragma omp parallel for schedule(static)
	for (i = 0; i < ET_NW; ++i) {
		double a = 0;
		for (int j = 0; j < nt; ++j) a += t[j][i];
		out[i] = a;
	}
}

// q = X^T C X p   (accumulated in per-thread buffers)
static void apply_A(const Set *s, const double *p, double *q, double **tq, int nt)
{
	#pragma omp parallel num_threads(nt)
	{
		double *lq = tq[omp_get_thread_num()];
		int k;
		memset(lq, 0, ET_NW * sizeof *lq);
		#pragma omp for schedule(static)
		for (k = 0; k < s->n; ++k) {
			const unsigned int *x = s->idx + (size_t) k * ET_NF;
			double t = p[ET_NW - 1];
			for (int i = 0; i < ET_NF; ++i) t += p[x[i]];
			t *= s->c[k];
			for (int i = 0; i < ET_NF; ++i) lq[x[i]] += t;
			lq[ET_NW - 1] += t;
		}
	}
	reduce(q, tq, nt);
}

// g = X^T C (y - X w), diag = X^T C 1 (count of each weight)
static void residual(const Set *s, const double *w, double *g, double *diag, double **tq, double **td, int nt)
{
	#pragma omp parallel num_threads(nt)
	{
		double *lq = tq[omp_get_thread_num()], *ld = td[omp_get_thread_num()];
		int k;
		memset(lq, 0, ET_NW * sizeof *lq);
		memset(ld, 0, ET_NW * sizeof *ld);
		#pragma omp for schedule(static)
		for (k = 0; k < s->n; ++k) {
			const unsigned int *x = s->idx + (size_t) k * ET_NF;
			double t = w[ET_NW - 1];
			for (int i = 0; i < ET_NF; ++i) t += w[x[i]];
			t = (s->y[k] - t) * s->c[k];
			for (int i = 0; i < ET_NF; ++i) { lq[x[i]] += t; ld[x[i]] += s->c[k]; }
			lq[ET_NW - 1] += t; ld[ET_NW - 1] += s->c[k];
		}
	}
	reduce(g, tq, nt);
	reduce(diag, td, nt);
}

int main(int argc, char **argv)
{
	const char *init = nullptr, *out = nullptr;
	int p0 = -1, p1 = -1, window = 0, iter = 60, nt = 24, only_eval = 0, calib = -1;
	double decay = 0.5, lambda = 1.0;
	auto value = [&](int &i) -> char * { if (i + 1 >= argc) die("missing value of %s", argv[i]); return argv[++i]; };
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-init")) init = value(i);
		else if (!strcmp(argv[i], "-out")) out = value(i);
		else if (!strcmp(argv[i], "-src")) {
			if (n_src == 8) die("too many -src (at most 8)");
			Source *o = src + n_src++;
			o->max = 30000000;
			if (source_parse(value(i), o) < 3) { fprintf(stderr, "bad -src\n"); return 1; }
		}
		else if (!strcmp(argv[i], "-plies")) { if (sscanf(value(i), "%d-%d", &p0, &p1) == 1) p1 = p0; }
		else if (!strcmp(argv[i], "-window")) window = atoi(value(i));
		else if (!strcmp(argv[i], "-decay")) decay = atof(value(i));
		else if (!strcmp(argv[i], "-lambda")) lambda = atof(value(i));
		else if (!strcmp(argv[i], "-iter")) iter = atoi(value(i));
		else if (!strcmp(argv[i], "-threads")) nt = atoi(value(i));
		else if (!strcmp(argv[i], "-test")) test_mod = atoi(value(i));
		else if (!strcmp(argv[i], "-eval")) only_eval = 1;
		else if (!strcmp(argv[i], "-calib")) calib = atoi(value(i));
		else { fprintf(stderr, "unknown option %s\n", argv[i]); return 1; }
	}
	if (!init || p0 < 0 || !n_src || (!out && (!only_eval || calib >= 0))) { fprintf(stderr, "missing options\n"); return 1; }
	if (p1 < p0 || p1 > 60) die("bad -plies (a-b, 0 <= a <= b <= 60)");
	if (nt < 1 || test_mod < 1 || window < 0 || calib >= n_src) die("bad -threads, -test, -window or -calib");
	omp_set_num_threads(nt);
	et_init();
	ET_Eval *e = et_read(init);

	double *w0 = (double *) xmalloc(ET_NW * sizeof *w0), *w = (double *) xmalloc(ET_NW * sizeof *w);
	double *r = (double *) xmalloc(ET_NW * sizeof *r), *z = (double *) xmalloc(ET_NW * sizeof *z);
	double *p = (double *) xmalloc(ET_NW * sizeof *p), *q = (double *) xmalloc(ET_NW * sizeof *q);
	double *M = (double *) xmalloc(ET_NW * sizeof *M);
	double **tq = (double **) xmalloc(nt * sizeof *tq), **td = (double **) xmalloc(nt * sizeof *td);
	for (int t = 0; t < nt; ++t) { tq[t] = (double *) xmalloc(ET_NW * sizeof (double)); td[t] = (double *) xmalloc(ET_NW * sizeof (double)); }

	printf("ply    train |");
	for (int d = 0; d < n_src; ++d) printf(" %-8s    n  prior -> new mse (mae)     |", src[d].prefix);
	printf(" iters time\n");
	for (int ply = p0; ply <= p1; ++ply) {
		const double t0 = omp_get_wtime();
		Set train = {}, test[8] = {};
		for (int d = -window; d <= window; ++d) {
			const int q_ = ply + d;
			if (q_ < 0 || q_ > 60) continue;
			load_ply(q_, pow(decay, abs(d)), &train, d == 0 ? test : nullptr);
		}
		double pm[8], pa[8], pb[8], nm[8], na[8], nb[8];
		for (int i = 0; i < ET_NW; ++i) w[i] = w0[i] = e->w[ply][i] / 128.0;
		for (int d = 0; d < n_src; ++d) if (test[d].n) errors(test + d, w0, pm + d, pa + d, pb + d);
		int it = 0;
		if (!only_eval && train.n) {
			residual(&train, w, r, M, tq, td, nt);	// r = b - A w (prior term is 0 at w = w0)
			double rz = 0;
			for (int i = 0; i < ET_NW; ++i) { M[i] += lambda; z[i] = r[i] / M[i]; p[i] = z[i]; rz += r[i] * z[i]; }
			const double rz0 = rz;
			for (it = 0; it < iter && rz > rz0 * 1e-12; ++it) {
				apply_A(&train, p, q, tq, nt);
				double pq = 0;
				for (int i = 0; i < ET_NW; ++i) { q[i] += lambda * p[i]; pq += p[i] * q[i]; }
				const double step = rz / pq;
				double rz_new = 0;
				for (int i = 0; i < ET_NW; ++i) { w[i] += step * p[i]; r[i] -= step * q[i]; z[i] = r[i] / M[i]; rz_new += r[i] * z[i]; }
				const double beta = rz_new / rz;
				rz = rz_new;
				for (int i = 0; i < ET_NW; ++i) p[i] = z[i] + beta * p[i];
			}
			for (int i = 0; i < ET_NW; ++i) {
				double v = floor(w[i] * 128 + 0.5);
				if (v > 32767) v = 32767; else if (v < -32767) v = -32767;
				e->w[ply][i] = (short) v;
				w[i] = v / 128.0;
			}
		}
		if (calib >= 0 && test[calib].n) {	// shift the constant weight so that the bias on source 'calib' is 0
			double b;
			errors(test + calib, w, &b, &b, &b);
			const double v = e->w[ply][ET_NW - 1] - floor(b * 128 + 0.5);
			e->w[ply][ET_NW - 1] = (short) (v > 32767 ? 32767 : v < -32767 ? -32767 : v);
			w[ET_NW - 1] = e->w[ply][ET_NW - 1] / 128.0;
		}
		printf("%3d %9d |", ply, train.n);
		for (int d = 0; d < n_src; ++d) {
			if (test[d].n) {
				errors(test + d, w, nm + d, na + d, nb + d);
				printf(" %-8s %6d %7.2f -> %7.2f (%5.2f) b %+5.2f -> %+5.2f |", src[d].prefix, test[d].n, pm[d], nm[d], na[d], pb[d], nb[d]);
			} else printf(" %-8s %6s %7s    %7s  %5s  |", src[d].prefix, "-", "", "", "");
			free(test[d].idx); free(test[d].y); free(test[d].c);
		}
		printf(" %5d %4.0fs\n", it, omp_get_wtime() - t0);
		fflush(stdout);
		free(train.idx); free(train.y); free(train.c);
	}
	if (out && (!only_eval || calib >= 0)) et_write(e, out);
	return 0;
}
