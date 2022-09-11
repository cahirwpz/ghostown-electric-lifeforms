#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

#define VECLEN 4
#define MAXVECNUM 4096
#define CODEWORDS 256

typedef float vec_t __attribute__((vector_size(VECLEN * sizeof(float))));

typedef struct codestat {
  int count;
  float dist;
} codestat_t;

static inline float Euclid(vec_t a, vec_t b) {
  vec_t d = a - b;
  vec_t sq = d * d;
  return sqrtf(sq[0] + sq[1] + sq[2] + sq[3]);
}

static inline vec_t DivVector(vec_t vec, long n) {
  return vec / (vec_t){n, n, n, n};
}

static vec_t CenterVector(vec_t *vec, long n) {
  vec_t center = (vec_t){0};
  for (long i = 0; i < n; i++)
    center += vec[i];
  return DivVector(center, n);
}

static float AverageDistortion(vec_t *codebook, int *nearest,
                               vec_t *vec, long n) {
  float d = 0.0;

  for (long i = 0; i < n; i++)
    d += Euclid(codebook[nearest[i]], vec[i]);

  return d / n;
}

void DumpCodeBook(vec_t *codebook, codestat_t *codestat, long n) {
  for (long i = 0; i < n; i++) {
    vec_t vec = codebook[i];
    codestat_t stat = codestat[i];
    if (!stat.count)
      continue;
    printf("codebook[%ld] = <%f %f %f %f> (count: %d, dist: %f)\n",
           i, vec[0], vec[1], vec[2], vec[3], stat.count, stat.dist);
  }
}

// Linde-Buzo-Gray / Generalized Lloyd algorithm
// Loosely based on: https://github.com/internaut/py-lbg/blob/master/lbg.py
static float EncodeChunk(vec_t *vec, size_t n, double epsilon) {
  static vec_t codebook[CODEWORDS];
  static codestat_t codestat[CODEWORDS];
  static int nearest[MAXVECNUM];

  memset(codebook, 0, sizeof(codebook));
  memset(codestat, 0, sizeof(codestat));
  memset(nearest, 0, sizeof(nearest));

  codebook[0] = CenterVector(vec, n);

  float avg_dist = AverageDistortion(codebook, nearest, vec, n);
  int niter;

  codestat[0] = (codestat_t){.count = n, .dist = avg_dist};

  for (int codewords = 1; codewords < CODEWORDS;) {
    int i, j;

    for (int c = 0; c < codewords; c++) {
      codebook[codewords + c] = codebook[c] + (vec_t){1,1,1,1};
      codebook[c] -= (vec_t){1,1,1,1};
    }

    codewords *= 2;

    // Try to reach a convergence by minimizing the average distortion.
    // This is done by moving the codewords step by step to the center
    // of the points in their proximity.
    double err = epsilon + 1;

    for (niter = 0; err > epsilon; niter++) {
      memset(nearest, 0, sizeof(nearest));
      memset(codestat, 0, sizeof(codestat));

      // For each vector find its nearest codeword
      for (i = 0; i < n; i++) {
        float min_dist = FLT_MAX;
        int best_idx = -1;
        for (j = 0; j < codewords; j++) {
          float dist = Euclid(codebook[j], vec[i]);
          if (dist < min_dist) {
            min_dist = dist;
            best_idx = j;
          }
        }

        nearest[i] = best_idx;
        codestat[best_idx].count++;
        codestat[best_idx].dist += min_dist;
      }

      // Update codebook based on fresh vectors classification
      memset(codebook, 0, codewords * sizeof(vec_t));

      for (i = 0; i < n; i++)
        codebook[nearest[i]] += vec[i];

      for (i = 0; i < codewords; i++)
        if (codestat[i].count)
          codebook[i] = DivVector(codebook[i], codestat[i].count);

      // Recalculate average distortion value & error value
      float prev_avg_dist = avg_dist;

      avg_dist = AverageDistortion(codebook, nearest, vec, n);
      // assert(avg_dist <= prev_avg_dist);
      err = fabs(prev_avg_dist - avg_dist) / prev_avg_dist;
    }
  }

  DumpCodeBook(codebook, codestat, CODEWORDS);

  return avg_dist;
}

static void EncodeFile(FILE *f) {
  static int8_t data[MAXVECNUM * VECLEN];
  static vec_t vec[MAXVECNUM];

  double sum_avg_dist = 0.0;
  size_t len;
  int nchunks;

  for (nchunks = 0; (len = fread(data, 1, sizeof(data), f)); nchunks++) {
    if (len < sizeof(data))
      memset(data + len, 0, sizeof(data) - len);

    size_t n = (len + 3) / 4;

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < VECLEN; j++) {
        vec[i][j] = data[i * 4 + j] + 128;
      }
    }

    double avg_dist = EncodeChunk(vec, n, 0.0001);

    printf("chunk[%d] avg_dist %g\n", nchunks, avg_dist);

    sum_avg_dist += avg_dist;
  }

  printf("average distortion: %g\n", sum_avg_dist / nchunks);
}

int main(int argc, char **argv) {
  if (argc != 2)
    return EXIT_FAILURE;
  FILE *f = fopen(argv[1], "r");
  EncodeFile(f);
  fclose(f);
  return 0;
}
