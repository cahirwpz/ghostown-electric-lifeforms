#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <math.h>
#include <stdbool.h>

#ifndef TEST
#define TEST 0
#endif

#if TEST
static const int WIDTH = 640;
static const int HEIGHT = 360;
#else
static const int WIDTH = 160;
static const int HEIGHT = 100;
#endif
static const int TEXSIZE = 64;

static const int MAX_STEPS = 1000;
static const float MAX_DIST = 100.0;
static const float SURF_DIST = 0.01;

static float iTime;
static SDL_Surface *iChannel0;
static SDL_Surface *iChannel1;

static bool mapDone = false;

typedef uint8_t uvmap_t[2][WIDTH * HEIGHT];

static uvmap_t uvmap;

typedef struct vec3 {
  float x, y, z, w;
} vec3;

typedef struct mat4 {
  float m00, m01, m02, m03;
  float m10, m11, m12, m13;
  float m20, m21, m22, m23;
  float m30, m31, m32, m33;
} mat4;

/* https://en.wikipedia.org/wiki/Fast_inverse_square_root */
float Q_rsqrt(float number) {
  union {
    float f;
    uint32_t i;
  } conv = {.f = number};
  conv.i = 0x5f3759df - (conv.i >> 1);
  conv.f *= 1.5f - (number * 0.5f * conv.f * conv.f);
  return conv.f;
}

float clamp(float v, float min, float max) {
  return fmin(fmax(v, min), max);
}

vec3 v3_abs(vec3 p) {
  return (vec3){fabs(p.x), fabs(p.y), fabs(p.z)};
}

vec3 v3_max(vec3 p, float v) {
  return (vec3){fmax(p.x, v), fmax(p.y, v), fmax(p.z, v)};
}

vec3 v3_add(vec3 a, vec3 b) {
  return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 v3_sub(vec3 a, vec3 b) {
  return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3 v3_mul(vec3 a, float v) {
  return (vec3){a.x * v, a.y * v, a.z * v};
}

float v3_dot(vec3 a, vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 v3_cross(vec3 a, vec3 b) {
  return (vec3){a.y * b.z - b.y * a.z,
                a.z * b.x - b.z * a.x,
                a.x * b.y - b.x * a.y};
}

float v3_length(vec3 p) {
  return sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
}

vec3 v3_normalize(vec3 p) {
#if 1
  return v3_mul(p, 1.0f / v3_length(p));
#else
  return v3_mul(p, Q_rsqrt(p.x * p.x + p.y * p.y + p.z * p.z));
#endif
}

mat4 m4_mul(mat4 a, mat4 b) {
  mat4 c;

  float *ma = (float *)&a.m00;
  float *mb = (float *)&b.m00;
  float *mc = (float *)&c.m00;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      mc[i * 4 + j] = 0;
      for (int k = 0; k < 4; k++) {
        mc[i * 4 + j] += ma[i * 4 + k] * mb[k * 4 + j];
      }
    }
  }

  return c;
}

/* @brief Rotate around X axis */
mat4 m4_rotate_x(float angle) {
  float s = sinf(angle);
  float c = cosf(angle);
  return (mat4){1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0, 0, 0, 0, 1};
}

/* @brief Rotate around Y axis */
mat4 m4_rotate_y(float angle) {
  float s = sinf(angle);
  float c = cosf(angle);
  return (mat4){c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1};
}

/* @brief Rotate around Z axis */
mat4 m4_rotate_z(float angle) {
  float s = sinf(angle);
  float c = cosf(angle);
  return (mat4){c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

mat4 m4_move(float x, float y, float z) {
  return (mat4){1, 0, 0, -x, 0, 1, 0, -y, 0, 0, 1, -z, 0, 0, 0, 1};
}

mat4 m4_rotate(float x, float y, float z) {
  return m4_mul(m4_mul(m4_rotate_x(x), m4_rotate_y(y)), m4_rotate_z(z));
}

vec3 m4_translate(vec3 p, mat4 a) {
  vec3 r;

  p.w = 1.0;

  float *vp = (float *)&p.x;
  float *vr = (float *)&r.x;
  float *ma = (float *)&a.m00;

  for (int i = 0; i < 4; i++) {
    vr[i] = 0;
    for (int j = 0; j < 4; j++) {
      vr[i] += ma[i * 4 + j] * vp[j];
    }
  }

  return r;
}

// Position:
//  x: left (-) <=> right (+)
//  y: down (-) <=> up (+)
//  z: back (-) <=> front (+)
mat4 m4_camera_lookat(vec3 pos, vec3 target, float rot) {
  // camera direction aka forward
	vec3 f = v3_normalize(v3_sub(target, pos));
  // camera up vector
	vec3 up = (vec3){sinf(rot), cosf(rot), 0.0};

	vec3 r = v3_normalize(v3_cross(f, up));
	vec3 u = v3_cross(r, f);

  return (mat4){
    r.x, u.x, f.x, 0.0,
    r.y, u.y, f.y, 0.0,
    r.z, u.z, f.z, 0.0,
    0.0, 0.0, 0.0, 1.0};
}

static int texpos(float x, int n) {
  return (int)((x - floorf(x)) * (float)n);
}

static Uint32 texture(SDL_Surface *img, vec3 uv) {
  Uint32 *tex = (Uint32 *)img->pixels;
  int u = (uv.x - floorf(uv.x)) * img->w;
  int v = (uv.y - floorf(uv.y)) * img->h;
  return tex[v * img->w + u];
}

typedef struct hit {
  float dist;
  int obj;
  vec3 uv;
} hit;

static hit GetDist(vec3 p);

static hit RayMarch(vec3 ro, vec3 rd) {
  // distance from origin 
  float dO = 0.0;
  hit h;

  for (int i = 0; i < MAX_STEPS; i++) {
    vec3 p = v3_add(ro, v3_mul(rd, dO));
    // distance to the scene
    h = GetDist(p);
    if (h.dist < SURF_DIST)
      return (hit){dO, h.obj};
    dO += h.dist;
    // prevent ray from escaping the scene
    if (dO > MAX_DIST)
      return (hit){INFINITY, -1};
  }

  SDL_Log("Too many steps!\n");

  return (hit){dO, h.obj}; 
}

static hit ScenePixel(vec3 uv);

static void Render(SDL_Surface *canvas) {
  Uint32 *buffer = (Uint32 *)canvas->pixels;

  uint32_t start = SDL_GetTicks();

  for (int y = 0; y < HEIGHT; y++) {
    // SDL_Log("y = %d\n", y);
    for (int x = 0; x < WIDTH; x++) {
      // Normalized pixel coordinates (Y from -1.0 to 1.0)
      vec3 uv = (vec3){
        (2.0f * (float)x - WIDTH) / HEIGHT, 1.0f - (float)y / HEIGHT * 2.0f};

      hit h = ScenePixel(uv);

      if (h.obj == -1) {
        SDL_Log("Missed the surface at (%d,%d)!", x, y);
        continue;
      } 

      Uint32 col = 0;

      if (h.obj == 0) {
        col = texture(iChannel0, h.uv);
      } else {
        col = texture(iChannel1, h.uv);
      }

      int pos = y * WIDTH + x;

      if (!mapDone) {
        uvmap[0][pos] = (texpos(h.uv.x, TEXSIZE) << 1) | h.obj;
        uvmap[1][pos] = texpos(h.uv.y, TEXSIZE);
      }

      buffer[pos] = col;
    }
  }

  uint32_t end = SDL_GetTicks();

  SDL_Log("Render took %dms\n", end - start);

  mapDone = true;
}

static SDL_Surface *LoadTexture(const char *path) {
  SDL_Surface *loaded;
  SDL_Surface *native;
  SDL_PixelFormat *pixelfmt;

  if (!(loaded = IMG_Load(path))) {
    SDL_Log("Unable to load image %s! SDL_image Error: %s\n",
            path, IMG_GetError());
    exit(EXIT_FAILURE);
  }

  pixelfmt = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
  native = SDL_ConvertSurface(loaded, pixelfmt, 0);
  SDL_FreeFormat(pixelfmt);
  SDL_FreeSurface(loaded);
  return native;
}

static void DeltaEncoder(uint8_t *out, uint8_t *data, int width, int height) {
  *out++ = data[0];
  for (int x = 1; x < width; x++)
    *out++ = data[x] - data[x - 1];

  for (int y = 1; y < height; y++) {
    int row = y * width;

    *out++ = data[row] - data[row - width];
    for (int x = 1; x < width; x++)
      *out++ = data[row + x] - data[row + x - 1];
  }
}

void SaveMap(const char *path, const char *name, uvmap_t uvmap) {
  FILE *f = fopen(path, "wb");
  uint8_t *output = malloc(WIDTH * HEIGHT);

  fprintf(f, "static u_char %s[2][%d] = {\n", name, WIDTH * HEIGHT);

  for (int i = 0; i < 2; i++) {
    DeltaEncoder(output, uvmap[i], WIDTH, HEIGHT);

    fprintf(f, "  {\n");
    for (int y = 0; y < HEIGHT; y++) {
      fprintf(f, "    /* y = %2d */ ", y);
      for (int x = 0; x < WIDTH; x++) {
        int c = output[y * WIDTH + x];
        fprintf(f, "%d, ", c);
      }
      fprintf(f, "\n");
    }
    fprintf(f, "  },\n");
  }

  fprintf(f, "};\n");
  fclose(f);

  free(output);
}

static void PerFrame(void) {
  static float iTimeStart;
  static bool iTimeFirst = true;

  if (iTimeFirst) {
    iTimeStart = SDL_GetTicks() / 1000.0f;
    iTimeFirst = false;
  }

  iTime = SDL_GetTicks() / 1000.0 - iTimeStart;
}

int main(void) {
  SDL_Init(SDL_INIT_EVERYTHING);
  IMG_Init(IMG_INIT_PNG);

  SDL_Window *window =
    SDL_CreateWindow("Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                     WIDTH, HEIGHT, 0);

  SDL_Surface *window_surface = SDL_GetWindowSurface(window);

  SDL_Surface *canvas = SDL_CreateRGBSurfaceWithFormat(
    0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBA8888);
  iChannel0 = LoadTexture("slabs.png");
  iChannel1 = LoadTexture("lava.png");

  int quit = 0;
  SDL_Event event;
  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        quit = 1;
      }
    }

    PerFrame();

    SDL_LockSurface(canvas);
    SDL_LockSurface(iChannel0);
    SDL_LockSurface(iChannel1);
    Render(canvas);
    SDL_UnlockSurface(iChannel1);
    SDL_UnlockSurface(iChannel0);
    SDL_UnlockSurface(canvas);

    SDL_BlitSurface(canvas, 0, window_surface, 0);
    SDL_UpdateWindowSurface(window);

    SDL_Delay(10);
  }

  SDL_FreeSurface(iChannel0);
  SDL_FreeSurface(iChannel1);
  IMG_Quit();
  SDL_Quit();

#if !TEST
  SaveMap(NAME "-uv.c", NAME, uvmap);
#else
  (void)SaveMap;
#endif

  return 0;
}
