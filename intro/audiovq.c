#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

#define VECLEN 4
#define MAXVECNUM 4096
#define CODEWORDS 256

typedef float vec_t __attribute__((vector_size(VECLEN * sizeof(float))));

typedef struct codestat {
  double dist;
  int count;
} codestat_t;

typedef struct nearest {
  double dist;
  int idx;
  bool chosen;
} nearest_t;

static inline float Euclid(vec_t a, vec_t b) {
  vec_t d = a - b;
  vec_t sq = d * d;
  return sq[0] + sq[1] + sq[2] + sq[3];
}

static float AverageDistortion(const nearest_t *nearest, const vec_t *vec,
                               long n, const vec_t *codebook) {
  float d = 0.0;

  for (long i = 0; i < n; i++)
    d += Euclid(codebook[nearest[i].idx], vec[i]);

  return d / n;
}

// For each vector find its nearest codeword
static void FindNearest(nearest_t *nearest, const vec_t *vec, long n,
                        const vec_t *codebook, long codewords) {
  for (long i = 0; i < n; i++) {
    float best_dist = FLT_MAX;
    int best_idx = -1;
    for (long j = 0; j < codewords; j++) {
      float dist = Euclid(codebook[j], vec[i]);
      if (dist < best_dist) {
        best_dist = dist;
        best_idx = j;
      }
    }

    assert(best_idx >= 0 && best_idx < codewords);

    nearest[i].idx = best_idx;
    nearest[i].dist = best_dist;
  }
}

void DumpCodeBook(vec_t *codebook, codestat_t *codestat, long n) {
  for (long i = 0; i < n; i++) {
    vec_t vec = codebook[i];
    codestat_t stat = codestat[i];
    if (!stat.count)
      continue;
    printf("codebook[%ld] = <%d %d %d %d> (count: %d, dist: %f)\n", i,
           (int)vec[0], (int)vec[1], (int)vec[2], (int)vec[3], stat.count,
           stat.dist);
  }
}

// Update codebook based on fresh vectors classification
static void UpdateCodeBook(vec_t *codebook, codestat_t *codestat,
                           long codewords, const nearest_t *nearest,
                           const vec_t *vec, long n) {
  long i;

  memset(codebook, 0, codewords * sizeof(vec_t));
  memset(codestat, 0, codewords * sizeof(codestat_t));

  for (i = 0; i < n; i++) {
    int idx = nearest[i].idx;
    float dist = nearest[i].dist;
    codestat[idx].count++;
    codestat[idx].dist += dist;
    codebook[idx] += vec[i];
  }

  for (i = 0; i < codewords; i++) {
    long cnt = codestat[i].count;
    if (cnt)
      codebook[i] /= (vec_t){cnt, cnt, cnt, cnt};
  }
}

// K-Means++ algorithm adapted to 8-bit 4-element vectors
// http://ilpubs.stanford.edu:8090/778/1/2006-13.pdf
static float EncodeChunk(const vec_t *vec, size_t n, double epsilon,
                         FILE *f_out) {
  static vec_t codebook[CODEWORDS];
  static codestat_t codestat[CODEWORDS];
  static nearest_t nearest[MAXVECNUM];

  long i, j, codewords;

  memset(codebook, 0, sizeof(codebook));
  memset(codestat, 0, sizeof(codestat));
  memset(nearest, 0, sizeof(nearest));

  // Take one center `c` chosen uniformly at random from `X`.
  i = random() % n;
  nearest[i].chosen = true;
  codebook[0] = vec[i];

  // Take a new center `c_i`, choosing `x` in `X`
  // with probability D(x)^2 / sum(D(x)^2) for x in X)
  for (codewords = 1; codewords < CODEWORDS;) {
    FindNearest(nearest, vec, n, codebook, codewords);

    long last = 0;
    for (i = 0; i < n; i++) {
      if (nearest[i].chosen)
        nearest[i].dist = 0;
      nearest[i].dist += last;
      last = nearest[i].dist;
    }

    if (last == 0)
      break;

    long x = random() % last;

    for (i = 0; i < n; i++)
      if (x < nearest[i].dist)
        break;

    assert(!nearest[i].chosen);

    nearest[i].chosen = true;
    codebook[codewords++] = vec[i];
  }

  // Try to reach a convergence by minimizing the average distortion.
  // This is done by moving the codewords step by step to the center
  // of the points in their proximity.
  double err = epsilon + 1;
  float avg_dist = AverageDistortion(nearest, vec, n, codebook);
  int niter;

  for (niter = 0; err > epsilon; niter++) {
    FindNearest(nearest, vec, n, codebook, codewords);
    UpdateCodeBook(codebook, codestat, codewords, nearest, vec, n);

    // Recalculate average distortion value & error value
    float prev_avg_dist = avg_dist;

    avg_dist = AverageDistortion(nearest, vec, n, codebook);
    // printf("[%d] avg_dist = %f\n", niter, avg_dist);
    assert(avg_dist <= prev_avg_dist);
    err = fabs(prev_avg_dist - avg_dist) / prev_avg_dist;
  }

  static uint8_t cb[CODEWORDS][VECLEN];
  for (i = 0; i < CODEWORDS; i++)
    for (j = 0; j < VECLEN; j++)
      cb[i][j] = codebook[i][j];
  fwrite(cb, sizeof(cb), 1, f_out);

  static uint8_t ni[MAXVECNUM];
  for (i = 0; i < n; i++)
    ni[i] = nearest[i].idx;
  fwrite(ni, n, 1, f_out);

  // DumpCodeBook(codebook, codestat, CODEWORDS);

  return sqrt(avg_dist);
}

static void WriteWord(short n, FILE *f_out) {
  uint8_t bin[2];
  bin[0] = n >> 8;
  bin[1] = n;
  fwrite(bin, sizeof(bin), 1, f_out);
}

static void EncodeFile(FILE *f_in, FILE *f_out) {
  static int8_t data[MAXVECNUM * VECLEN];
  static vec_t vec[MAXVECNUM];

  double sum_avg_dist = 0.0;
  size_t len;
  int nchunks;

  for (nchunks = 0; (len = fread(data, 1, sizeof(data), f_in)); nchunks++) {
    if (len < sizeof(data))
      memset(data + len, 0, sizeof(data) - len);

    size_t n = (len + 3) / 4;

    for (int i = 0; i < n; i++)
      for (int j = 0; j < VECLEN; j++)
        vec[i][j] = data[i * 4 + j];

    WriteWord(n, f_out);

    double avg_dist = EncodeChunk(vec, n, 0.001, f_out);

    printf("chunk[%d] avg_dist %g\n", nchunks, avg_dist);

    sum_avg_dist += avg_dist;
  }

  WriteWord(0, f_out);

  printf("average distortion: %g\n", sum_avg_dist / nchunks);
}

int main(int argc, char **argv) {
  if (argc != 3)
    return EXIT_FAILURE;
  FILE *f_in = fopen(argv[1], "r");
  FILE *f_out = fopen(argv[2], "w");
  EncodeFile(f_in, f_out);
  fclose(f_out);
  fclose(f_in);
  return 0;
}
