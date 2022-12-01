final int WIDTH = 640/4;
final int HEIGHT = 512/4;

final int MAX_STEPS = 100;
final float MAX_DIST = 100.0;
final float SURF_DIST = 0.01;

float iTime;

void settings() {
  size(WIDTH, HEIGHT);
  noSmooth();
}

void setup() {
  frameRate(50);
}

PVector abs(PVector p) {
  return new PVector(abs(p.x), abs(p.y), abs(p.z));
}

PVector max(PVector p, float v) {
  return new PVector(max(p.x, v), max(p.y, v), max(p.z, v));
}

/* @brief Rotate around X axis */
PMatrix3D RotationX(float angle) {
  PMatrix3D m = new PMatrix3D();
  float s = sin(angle);
  float c = cos(angle);
  m.set(
    1, 0, 0, 0, 
    0, c, -s, 0, 
    0, s, c, 0, 
    0, 0, 0, 1);
  return m;
}

/* @brief Rotate around Y axis */
PMatrix3D RotationY(float angle) {
  PMatrix3D m = new PMatrix3D();
  float s = sin(angle);
  float c = cos(angle);
  m.set(
    c, 0, s, 0, 
    0, 1, 0, 0, 
    -s, 0, c, 0, 
    0, 0, 0, 1);
  return m;
}

/* @brief Rotate around Z axis */
PMatrix3D RotationZ(float angle) {
  PMatrix3D m = new PMatrix3D();
  float s = sin(angle);
  float c = cos(angle);
  m.set(
    c, -s, 0, 0, 
    s, c, 0, 0, 
    0, 0, 1, 0, 
    0, 0, 0, 1);
  return m;
}

PMatrix3D Rotate(float x, float y, float z) {
  PMatrix3D m = RotationX(x);
  m.apply(RotationY(y));
  m.apply(RotationZ(z));
  return m;
}

PMatrix3D Move(float x, float y, float z) {
  PMatrix3D m = new PMatrix3D();
  m.set(
    1, 0, 0, -x, 
    0, 1, 0, -y, 
    0, 0, 1, -z, 
    0, 0, 0, 1);
  return m;
}


/* @brief Calculate distance from sphere
 * @param p point of interest
 * @param r sphere radius
 * @return distance of s from p
 */
float SphereDist(PVector p, float r) {
  return p.mag() - r;
}

/* @brief Calculate distance from plane at (0,0,0) origin
 * @param p point of interest
 * @return distance of plane from p
 */
float PlaneDist(PVector p) {
  return p.y;
}

/* @brief Calculate distance from a capsule */
float CapsuleDist(PVector p, PVector a, PVector b, float r) {
  PVector ab = PVector.sub(b, a);
  PVector ap = PVector.sub(p, a);

  float t = PVector.dot(ab, ap) / PVector.dot(ab, ab);

  t = constrain(t, 0.0, 1.0);

  PVector c = PVector.add(a, PVector.mult(ab, t));

  return PVector.sub(p, c).mag() - r;
}

/* @brief Calculate distance from a torus */
float TorusDist(PVector p, float r1, float r2) {
  PVector r = new PVector(p.x, p.z);
  PVector q = new PVector(r.mag() - r1, p.y);
  return q.mag() - r2;
}

/* @brief Calculate distance from a box */
float BoxDist(PVector p, PVector s) {
  PVector d = PVector.sub(abs(p), s);
  float e = max(d, 0.0).mag();
  float i = min(max(d.x, max(d.y, d.z)), 0.0);
  return e + i;
}

/* @brief Calculate distance from a cylinder */
float CylinderDist(PVector p, PVector a, PVector b, float r) {
  PVector ab = PVector.sub(b, a);
  PVector ap = PVector.sub(p, a);

  float t = PVector.dot(ab, ap) / PVector.dot(ab, ab);

  PVector c = PVector.add(a, PVector.mult(ab, t));
  float x = PVector.sub(p, c).mag() - r;
  float y = (abs(t - 0.5) - 0.5) * ab.mag();
  float e = max(new PVector(x, y), 0.0).mag();
  float i = min(max(x, y), 0.0);

  return e + i;
}

float GetDist(PVector p) {
  float sd, pd, cd, ccd, td, bd;
  
  PMatrix3D so = Move(0, 1, 5);
  PVector sp = so.mult(p, null);
  sd = SphereDist(sp, 1.0);

  pd = PlaneDist(p);
  
  cd = CapsuleDist(p, new PVector(3, 1, 8), new PVector(3, 3, 8), .5);
  ccd = CylinderDist(p, new PVector(2, 1, 6), new PVector(3, 1, 4), .5);

  PMatrix3D to = Move(-3, 1, 6);
  PMatrix3D tr = Rotate(iTime, 0., 0.);
  to.preApply(tr);
  PVector tp = to.mult(p, null);
  td = TorusDist(tp, 1.0, 0.25);

  PMatrix3D bo = Move(0, 1, 8);
  PMatrix3D br = Rotate(0., iTime * .2, 0.);
  bo.preApply(br);
  PVector bp = bo.mult(p, null);
  bd = BoxDist(bp, new PVector(1, 1, 1));

  return min(min(min(ccd, bd), min(sd, td)), min(pd, cd));
}

PVector GetNormal(PVector p) {
  PVector e = new PVector(0.01, 0.0);
  float d = GetDist(p);
  PVector px = new PVector(e.x, e.y, e.y);
  PVector py = new PVector(e.y, e.x, e.y);
  PVector pz = new PVector(e.y, e.y, e.x);
  PVector n = new PVector(
    d - GetDist(PVector.sub(p, px)), 
    d - GetDist(PVector.sub(p, py)), 
    d - GetDist(PVector.sub(p, pz)));
  n.normalize();
  return n;
}

float RayMarch(PVector ro, PVector rd) {
  // distance from origin
  float dO = 0.0;

  for (int i = 0; i < MAX_STEPS; i++) {
    PVector p = PVector.add(ro, PVector.mult(rd, dO));
    // distance to the scene
    float dS = GetDist(p);
    if (dS < SURF_DIST)
      break;
    dO += dS;
    // prevent ray from escaping the scene
    if (dO > MAX_DIST)
      break;
  }

  return dO;
}

float GetLight(PVector p) {
  PVector lightPos = new PVector(0.0, 5.0, 6.0);

  lightPos.x += sin(iTime) * 2.0;
  lightPos.z += cos(iTime) * 2.0;

  PVector l = PVector.sub(lightPos, p);
  l.normalize();

  PVector n = GetNormal(p);

  float diff = PVector.dot(n, l);

  float d = RayMarch(PVector.add(p, PVector.mult(n, SURF_DIST)), l);

  if (d < lightPos.mag())
    diff *= 0.1;

  return constrain(diff, 0.0, 1.0);
}

void draw() {
  iTime = frameCount / 50.0;

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      // Normalized pixel coordinates (from -1.0 to 1.0)
      PVector uv = new PVector(
        (float)x * 2.0 / WIDTH - 1.0, 
        1.0 - (float)y * 2.0 / HEIGHT);

      // Simple camera model
      PVector ro = new PVector(0.0, 4.0, 0.0);
      PVector rd = new PVector(uv.x, uv.y - 0.5, 1.0);
      rd.normalize();

      float d = RayMarch(ro, rd);

      PVector p = PVector.add(ro, PVector.mult(rd, d));

      float diff = GetLight(p);

      diff = constrain(diff, 0.0, 1.0);

      set(x, y, color(int(diff * 255.0)));
    }
  }
}
