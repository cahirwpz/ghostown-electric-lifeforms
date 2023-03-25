#define TEST 0
#define NAME "tit"
#include "uvmap.h"

float TorusInnerDist(vec3 p, float r1, float r2) {
  float q = sqrtf(p.x * p.x + p.z * p.z) - r2;
  return sqrtf(q * q + p.y * p.y) - r1;
}

float TorusOuterDist(vec3 p, float r1, float r2) {
  float q = sqrtf(p.x * p.x + p.z * p.z) - r2;
  return r1 - sqrtf(q * q + p.y * p.y);
}

const float torusRadius = 2.;
const float torusOuter = .8;
const float torusInner = .25;

static hit GetDist(vec3 p) {
	float t0 = TorusOuterDist(p, torusOuter, torusRadius);
  float t1 = TorusInnerDist(p, torusInner, torusRadius);

  if (t0 < t1)
    return (hit){t0, 0};
  else
    return (hit){t1, 1};
}

static hit ScenePixel(vec3 uv) {
  // camera-to-world transformation
  vec3 lookat = (vec3){0.0, -0.0, -1.5};
  vec3 ro = (vec3){1.0, .5, -2.2};

  mat4 ca = m4_camera_lookat(ro, lookat, 0.);
  vec3 co = v3_normalize((vec3){uv.x, uv.y, 2.0});
  vec3 rd = m4_translate(co, ca);

  hit h = RayMarch(ro, rd);

  vec3 p = v3_add(ro, v3_mul(rd, h.dist));

  vec3 tuv = (vec3){
    atan2(sqrtf(p.x * p.x + p.z * p.z) - torusRadius, p.y),
    atan2(p.x, p.z),
  };

  tuv = v3_mul(tuv, .5 / M_PI);

    // fetch a color from texture for given object
  if (h.obj == 0) {
    tuv.x *= 4.;
    tuv.y *= 8.;
    h.uv = v3_add(tuv, (vec3){0.0, iTime * 0.1});
  } else if (h.obj == 1) {
    tuv.x *= 2.;
    tuv.y *= 16.;
    h.uv = v3_add(tuv, (vec3){0.0, -iTime * 0.25});
  } else {
    h.obj = -1;
  }

  return h;
}
