#include <SDL.h>
#include <SDL_image.h>
#include <math.h>

static const int WIDTH = 160;
static const int HEIGHT = 100;
static const int TEXSIZE = 64;

static const int MAX_STEPS = 100;
static const float MAX_DIST = 10.0;
static const float SURF_DIST = 0.001;

static float iTime;
static SDL_Surface *iChannel0;
static SDL_Surface *iChannel1;

static uint8_t umap[WIDTH * HEIGHT];
static uint8_t vmap[WIDTH * HEIGHT];

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

Uint32 texture(SDL_Surface *img, vec3 uv) {
  Uint32 *tex = (Uint32 *)img->pixels;
  int u = (uv.x - floorf(uv.x)) * img->w;
  int v = (uv.y - floorf(uv.y)) * img->h;
  return tex[v * img->w + u];
}

float PipeInnerDist(vec3 p, float r) {
  return sqrtf(p.x * p.x + p.y * p.y) - r;
}

float PipeOuterDist(vec3 p, float r) {
  return r - sqrtf(p.x * p.x + p.y * p.y);
}

void PerFrame(void) {
  iTime = SDL_GetTicks() / 1000.0;
}

typedef struct hit {
  float dist;
  int obj;
} hit;

const float pipeOuterRadius = 2.0;
const float pipeInnerRadius = 0.6;

hit GetDist(vec3 p) {
  float p0 = PipeOuterDist(p, pipeOuterRadius);
  float p1 = PipeInnerDist(p, pipeInnerRadius);

  if (p0 < p1)
    return (hit){p0, 0};
  else
    return (hit){p1, 1};
}

hit RayMarch(vec3 ro, vec3 rd) {
  // distance from origin 
  float dO = 0.0;

  for (int i = 0; i < MAX_STEPS; i++) {
    vec3 p = v3_add(ro, v3_mul(rd, dO));
    // distance to the scene
    hit h = GetDist(p);
    if (h.dist < SURF_DIST)
      return (hit){dO, h.obj}; 
    dO += h.dist;
    // prevent ray from escaping the scene
    if (dO > MAX_DIST)
      break;
  }

  return (hit){INFINITY, -1};
}

void Render(SDL_Surface *canvas) {
  Uint32 *buffer = (Uint32 *)canvas->pixels;

  uint32_t start = SDL_GetTicks();

  for (int y = 0; y < HEIGHT; y++) {
    // SDL_Log("y = %d\n", y);
    for (int x = 0; x < WIDTH; x++) {
      // Normalized pixel coordinates (from -1.0 to 1.0)
      vec3 uv =
        (vec3){(float)x / WIDTH * 2.0 - 1.0, 1.0 - (float)y / HEIGHT * 2.0};

      // camera-to-world transformation
      vec3 lookat = (vec3){0.0, 0.0, -1.0};
      vec3 ro = (vec3){1.9, 0.0, -2.5};

      mat4 ca = m4_camera_lookat(ro, lookat, 0.);
      vec3 co = v3_normalize((vec3){uv.x, uv.y, 2.0});
      vec3 rd = m4_translate(co, ca);

      hit h = RayMarch(ro, rd);

      vec3 p = v3_add(ro, v3_mul(rd, h.dist));

      vec3 tuv = v3_mul((vec3){atan2(p.x, p.y), p.z}, .5 / M_PI);

      Uint32 col = 0;

      // fetch a color from texture for given object
      switch (h.obj) {
        case 0:
          tuv.x *= 2.;
          tuv.y *= 1.;
          tuv = v3_add(tuv, (vec3){iTime * 0.1, iTime * 0.1});
          col = texture(iChannel1, tuv);
          break;
          
        case 1:
          tuv.x *= 1.;
          tuv.y *= 2.;
          tuv = v3_add(tuv, (vec3){iTime * 0.1, -iTime * 0.25});
          col = texture(iChannel0, tuv);
          break;

        default:
          break;
      }

      umap[y * WIDTH + x] = texpos(tuv.x, TEXSIZE);
      vmap[y * WIDTH + x] = texpos(tuv.y, TEXSIZE);

      buffer[y * WIDTH + x] = col;
    }
  }

  uint32_t end = SDL_GetTicks();

  SDL_Log("Render took %dms\n", end - start);
}

SDL_Surface *LoadTexture(const char *path) {
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

void SaveMap(const char *path, const char *name, uint8_t *map) {
  FILE *f = fopen(path, "wb");

  fprintf(f, "static u_char %s[%d] = {\n", name, WIDTH * HEIGHT);
  for (int y = 0; y < HEIGHT; y++) {
    fprintf(f, "  ");
    for (int x = 0; x < WIDTH; x++) {
      fprintf(f, "%d, ", map[y * WIDTH + x]);
    }
    fprintf(f, "\n");
  }
  fprintf(f, "};\n");
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
  iChannel0 = LoadTexture("lava.png");
  iChannel1 = LoadTexture("slabs.png");

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

  SaveMap("map-u.c", "umap", umap);
  SaveMap("map-v.c", "vmap", vmap);

  return 0;
}
