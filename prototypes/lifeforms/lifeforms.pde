static final int w = 320;
static final int h = 256;
static final int scale = 3;
static final int tilesize = 8;
static final int tilew = w/tilesize;
static final int tileh = h/tilesize;

static final int orgcount = 30;
static final int groupcount = 2; // more than 2 groups starts to look off
static final int batchsize = orgcount/groupcount;

PGraphics screen;

Organism buckets[];
Organism organisms[];
Flowfield ff;
int weights[] = {6, 0, 0, 3, 2};

boolean heatmap = false;
boolean wallBounce = false;

int adds;
int muls;
int divs;
int invSqrts;

Vec2 normtab[] = new Vec2[1<<14];

void init_normtab() {
  // x and y assumed to be in range 0-7.875 in 3.4 format
  for (int y = 0; y < 128; y++) {
    for (int x = 0; x < 128; x++) {
      PVector v = new PVector((float)x, (float)y);
      v.normalize();
      // convert back to 3.12
      int nx = (int)(v.x*4096);
      int ny = (int)(v.y*4096);
      normtab[(y<<7)+x] = new Vec2(nx, ny);
    }
  }
}

class Vec2 {
  int x;
  int y;
  
  Vec2() {
    x = 0;
    y = 0;
  }
  
  Vec2(int _x, int _y) {
    x = _x;
    y = _y;
  }
  
  Vec2(Vec2 other) {
    x = other.x;
    y = other.y;
  }
  
  void add(Vec2 other) {
    x += other.x;
    y += other.y;
  }
  
  void sub(Vec2 other) {
    x -= other.x;
    y -= other.y;
  }
  
  void mult(int coeff) {
    x *= coeff;
    y *= coeff;
  }
  
  void div(int coeff) {
    x /= coeff;
    y /= coeff;
  }
  
  // input int 12.4 format
  void normalize() {
    int sgnx = Integer.signum(x);
    int sgny = Integer.signum(y);
    x = abs(x);
    y = abs(y);
    while (x > 127 || y > 127) {
      x >>= 1;
      y >>= 1;
    }
    Vec2 norm = new Vec2(normtab[(y<<7) + x]);
    // fix sign according to the quadrant
    x = sgnx * norm.x; //(norm.x ^ (sgnx < 0 ? 0xffffffff : 0)) + (sgnx < 0 ? 1 : 0);
    y = sgny * norm.y; //(norm.y ^ (sgny < 0 ? 0xffffffff : 0)) + (sgny < 0 ? 1 : 0);
  }
};

class Flowfield {
  PVector[][] vectors;
  int ffw;
  int ffh;
  int resolution;
  
  Flowfield(int res) {
    resolution = res;
    ffw = w/resolution;
    ffh = h/resolution;
    vectors = new PVector[w][h];
    for (int i = 0; i < ffw; i++) {
      for (int j = 0; j < ffh; j++) {
        float sample = noise(map(i, 0, ffw-1, 0, 1), map(j, 0, ffh-1, 0, 1)/*, float(frameCount)/100.0*/);
        float theta = map(sample, 0, 1, -TWO_PI, TWO_PI);
        vectors[i][j] = new PVector(cos(theta), sin(theta));
        float sample2 = noise(map(i, 0, ffw-1, 3, 6), map(j, 0, ffh-1, 3, 6)/*, float(frameCount)/100.0*/);
        vectors[i][j].limit(sample2);
      }
    }
  }
  
  PVector get(int x, int y) {
    return vectors[constrain(x, 0, ffw - 1)][constrain(y, 0, ffh - 1)];
  }
  
  void draw() {
    PVector reference = new PVector(1, 0);
    for (int y = 0; y < ffh; y++) {
      for (int x = 0; x < ffw; x++) {
        if (heatmap) {
          float sx = x * resolution;
          float sy = y * resolution;
          screen.colorMode(HSB, TWO_PI, 1.0, 1.0);
          float h = PVector.angleBetween(reference, vectors[x][y]);
          screen.stroke(h, 0.5, 0.5);
          screen.fill(h, 0.5, 0.5);
          screen.rect(sx, sy, resolution, resolution);
          screen.colorMode(RGB);
        } else {
          PVector v = vectors[x][y];
          float sx = x * resolution + resolution/2;
          float sy = y * resolution + resolution/2;
          float dx = v.x * resolution;
          float dy = v.y * resolution;
  
          screen.stroke(128); screen.line(sx, sy, sx+dx, sy+dy);
          screen.stroke(255); screen.point(sx, sy);
          screen.stroke(255, 0, 0); screen.point(sx+dx, sy+dy);
        }
      }
    }
  }
}

final float MIN_Q12 = -8.0;
final float MAX_Q12 = 7.999755859375;

final int MAX_USHORT = (1<<16)-1;
final int MIN_USHORT = 0;
final int MAX_SHORT = (1<<15)-1;
final int MIN_SHORT = -(1<<15);

final int maxspeed = MAX_SHORT/2;
final int maxspeedSq = maxspeed*maxspeed;
final int maxforce = MAX_SHORT/32;
final int maxforceSq = maxforce*maxforce;
final int orgSize = 10;

void limitMaxspeed(Vec2 vel) { //<>//
  int dsq = vel.x * vel.x + vel.y * vel.y;
  if (dsq > maxspeedSq) {
    vel.normalize();
    vel.x <<= 2;
    vel.y <<= 2; // vel.mult(maxspeed);
  }
}

void limitMaxforce(Vec2 force) {
  int dsq = force.x * force.x + force.y * force.y;
  if (dsq > maxforceSq) {
    force.normalize();
    force.x >>= 2;
    force.y >>= 2; // force.mult(maxforce);
  }
}

int alignCoheseCircle[] = {
  -2, -4,
  -1, -4,
  0, -4,
  1, -4,
  2, -4,

  -3, -3,
  -2, -3,
  -1, -3,
  0, -3,
  1, -3,
  2, -3,
  3, -3,

  -4, -2,
  -3, -2,
  -2, -2,
  -1, -2,
  0, -2,
  1, -2,
  2, -2,
  3, -2,
  4, -2,

  -4, -1,
  -3, -1,
  -2, -1,
  -1, -1,
  0, -1,
  1, -1,
  2, -1,
  3, -1,
  4, -1,

  -4, 0,
  -3, 0,
  -2, 0,
  -1, 0,
  0, 0,
  1, 0,
  2, 0,
  3, 0,
  4, 0,

  -4, 1,
  -3, 1,
  -2, 1,
  -1, 1,
  0, 1,
  1, 1,
  2, 1,
  3, 1,
  4, 1,

  -4, 2,
  -3, 2,
  -2, 2,
  -1, 2,
  0, 2,
  1, 2,
  2, 2,
  3, 2,
  4, 2,

  -3, 3,
  -2, 3,
  -1, 3,
  0, 3,
  1, 3,
  2, 3,
  3, 3,

  -2, 4,
  -1, 4,
  0, 4,
  1, 4,
  2, 4,
};

int separationCircle[] = {
  0, -2,

  -1, -1,
  0, -1,
  1, -1,

  -2, 0,
  -1, 0,
  0, 0,
  1, 0,
  2, 0,

  -1, 1,
  0, 1,
  1, 1,

  0, 2,
};

ArrayList<Organism> getNeighbours(Organism self, int[] sepArea) {
  ArrayList<Organism> res = new ArrayList<Organism>();
  for (int i = 0; i < sepArea.length; i += 2) {
    int neighbour = self.bucketid + sepArea[i] + (sepArea[i+1] * tilew);
    //if (neighbour < 0)
    //  neighbour += tilew*tileh;
    //else if (neighbour >= tilew*tileh)
    //  neighbour -= tilew*tileh;
    if (neighbour < 0 || neighbour >= tilew*tileh)
      continue;
    Organism org = buckets[neighbour];
    if (org != null) {
      while (org.next != null) {
        if (org != self)
          res.add(org);
        org = org.next;
      }
      if (org != self)
        res.add(org);
    }
  }
  return res;
}

float fastInvSqrt(float x) {
  return 1.0 / sqrt(x);
}

class Organism {
  Vec2 pos;   // 12.4
  Vec2 vel;   // 4.12
  Vec2 accel; // 4.12
  int bucketid;
  Organism next;
  
  Organism(int x, int y, int id) {
    pos = new Vec2(x<<4, y<<4);
    vel = new Vec2((int)random(-maxspeed/2, maxspeed/2), (int)random(-maxspeed/2, maxspeed/2));
    accel = new Vec2(0, 0);
    bucketid = id;
    next = null;
  }
  
  void update() {
    vel.add(accel);
    limitMaxspeed(vel);
    Vec2 velQ4 = new Vec2(vel.x>>8, vel.y>>8);
    pos.add(velQ4);

    Organism _buckets[] = buckets; // force to show up in debugger
    
    if (wallBounce) {
      //Vec2 desiredVel = new Vec2(vel);
      //if (pos.x > 0)
      //  desiredVel.x = maxspeed;
      //else if (pos.x > w)
      //  desiredVel.x = -maxspeed;
      //if (pos.y < 0)
      //  desiredVel.y = maxspeed;
      //else if (pos.y > h)
      //  desiredVel.y = -maxspeed;
      
      //applyDesiredVel(desiredVel);
    } else {
      if (pos.x < 0)
        pos.x += w<<4;
      if (pos.y < 0)
        pos.y += h<<4;
      if (pos.x > w<<4)
        pos.x -= w<<4;
      if (pos.y > h<<4)
        pos.y -= h<<4;
    }

    int tx = constrain((pos.x>>4) / tilesize, 0, tilew-1);
    int ty = constrain((pos.y>>4) / tilesize, 0, tileh-1);
    int bucket = ty*tilew + tx;

    Organism org = buckets[bucketid];
    while (org.next != null) {
      assert(org.bucketid == bucketid);
      org = org.next;
    }

    if (bucket != bucketid) {
      Organism self = buckets[bucketid];
      Organism prev = buckets[bucketid];
      while (self != this) {
        assert(self.next != null);
        prev = self;
        self = self.next;
      }
      assert(self.next != prev);
      prev.next = self.next;
      // only one boid in the bucket
      if (self == prev)
        buckets[bucketid] = self.next;

      if (_buckets[bucket] == null)
        buckets[bucket] = self;
      else {
        Organism cur = buckets[bucket];
        while (cur.next != null)
          cur = cur.next;
        cur.next = self;
      }
      self.next = null;
      self.bucketid = bucket;
    }
  }

  void draw() {
    PVector posf = new PVector(float(pos.x)/16, float(pos.y)/16);
    PVector velf = new PVector(float(vel.x)/(1<<12), float(vel.y)/(1<<12));
    screen.stroke(#FFFFFF);
    screen.fill(#AAAAAA);
    screen.circle(posf.x, posf.y, orgSize);
    
    screen.stroke(#FF0000);
    screen.line(posf.x, posf.y, posf.x + velf.x*4, posf.y + velf.y*4);
    screen.fill(#0000FF);
    //screen.text(String.format("%.2f %.2f", velf.x, velf.y), posf.x, posf.y);
  }
  
  void applyForce(Vec2 force) {
    accel.add(force);
  }
  
  void applyDesiredVel(Vec2 desired) {
    Vec2 steer = new Vec2(desired);
    steer.sub(vel);
    limitMaxforce(steer);
    applyForce(steer);
  } //<>//
  
  Vec2 steerForce(Vec2 desiredVel) {
    Vec2 steer = new Vec2(desiredVel);
    steer.sub(vel);
    limitMaxforce(steer);
    return steer;
  }
  
  Vec2 seek(Vec2 target) {
    Vec2 desired = new Vec2(target);
    desired.sub(pos);
    // potentially not neeed normalization step? - it starts to look off though
    // space between boids isn't maintained and they tend to lump together more often
    desired.normalize();
    desired.x <<= 2;
    desired.y <<= 2; // desired.mult(maxspeed);
    return steerForce(desired);
  }
  
  Vec2 separate() {
    Vec2 avg = new Vec2();
    int cnt = 0;
    
    for (Organism other : getNeighbours(this, separationCircle)) {
      int diffx = pos.x - other.pos.x;
      int diffy = pos.y - other.pos.y;
      Vec2 away = new Vec2(diffx, diffy);
      avg.add(away);
      cnt++;
      adds += 6;
    }
    
    if (cnt > 0) {
      avg.normalize();
      avg.x <<= 2;
      avg.y <<= 2; // avg.mult(maxspeed);
      avg = steerForce(avg);
    }
    return avg;
  }
  
  Vec2 align() {
    Vec2 avg = new Vec2();
    int cnt = 0;
    
    for (Organism other : getNeighbours(this, alignCoheseCircle)) {
      avg.add(other.vel);
      cnt++;
      adds += 3;
    }
    
    if (cnt > 0) {
      avg.normalize();
      avg.x <<= 2;
      avg.y <<= 2; // avg.mult(maxspeed);
      avg = steerForce(avg);
    }
    return avg;
  }
  
  Vec2 cohese() {
    Vec2 avg = new Vec2();
    int cnt = 0;
    
    for (Organism other : getNeighbours(this, alignCoheseCircle)) {
      avg.add(other.pos);
      cnt++;
      adds += 3;
    }
    
    if (cnt > 0) {
      avg.div(cnt);
      avg = seek(avg);
    }
    return avg;
  }
  
  void applyBehaviors() {
    accel.x = 0;
    accel.y = 0;
    Vec2 sep = separate();
    sep.div(10);
    Vec2 mouse = seek(new Vec2(16*mouseX/scale, 16*mouseY/scale));
    
    int x = pos.x / ff.ffw;
    int y = pos.y / ff.ffh;
    //PVector ffForce = steerForce(PVector.mult(ff.get(x, y), 20), maxforce);
    
    Vec2 alignment = align();
    alignment.div(10);
    Vec2 cohesion = cohese();
    cohesion.div(10);
    
    sep.mult(weights[0]);
    mouse.mult(weights[1]);
    //ffForce.mult(weights[2]);
    alignment.mult(weights[3]);
    cohesion.mult(weights[4]);
    
    muls += 4; // don't count mouse, can potentially be replaced with shifts and adds
    
    applyForce(sep);
    applyForce(alignment);
    applyForce(cohesion);
    applyForce(mouse);
    //applyForce(ffForce);
  }
  
}

void setup() {
  size(960, 768); //<>//
  noSmooth();
  randomSeed(42);
  noiseSeed(38);
  init_normtab();
  screen = createGraphics(scale*w, scale*h);

  buckets = new Organism[tilew * tileh];
  organisms = new Organism[orgcount];

  int i = 0;
  while (i < orgcount) {
    int x = floor(random(0, float(tilew)));
    int y = floor(random(0, float(tileh)));
    int bucket = y*tilew + x;
    if (buckets[bucket] == null) {
      organisms[i] = new Organism(x*tilesize - tilesize/2, y*tilesize - tilesize/2, bucket);
      buckets[bucket] = organisms[i];
      i++;
    }
  }
}

int rasterBest = Integer.MAX_VALUE;
int rasterWorst = Integer.MIN_VALUE;
int group = 0;

void draw() {
  clear();
  screen.beginDraw();
  screen.clear();
  screen.scale(scale);

  ff = new Flowfield(16);
  ff.draw();
  
  screen.fill(#ffffff);
  screen.textSize(10);
  screen.text(String.format("Separation: %d\nMouse: %d\nFlowfield: %d\nAlignment: %d\nCohesion: %d",
              weights[0], weights[1], weights[2], weights[3], weights[4]), 0, 10);

  adds = 0;
  muls = 0;
  divs = 0;
  invSqrts = 0;
  
  for (int i = group * batchsize; i < (group+1) * batchsize; i++)
    organisms[i].applyBehaviors();

  for (Organism org : organisms) {
    org.update();
    org.draw();
  }
  
  group = (group+1) % groupcount;
  
  int rasterlines = (adds*12+muls*70+divs*140)/450;
  if (rasterlines < rasterBest)
    rasterBest = rasterlines;
  if (rasterlines > rasterWorst)
    rasterWorst = rasterlines;
  
  //print(String.format("adds: %d, muls: %d, divs: %d, raster lines: %d-%d-%d\n", 
  //                     adds, muls, divs, rasterBest, rasterlines, rasterWorst));

  int count = 0;
  for (int i = 0; i < buckets.length; i++) {
    if (buckets[i] != null) {
      Organism org = buckets[i];
      while (org.next != null) {
        org = org.next;
        count++;
      }
      count++;
    }
  }
  assert(count == orgcount);

  screen.endDraw();
  image(screen, 0, 0);
}

void keyPressed() {
  if (key == '1')
    weights[0] -= 1;
  if (key == '2')
    weights[0] += 1;
  if (key == '3')
    weights[1] -= 1;
  if (key == '4')
    weights[1] += 1;
  if (key == '5')
    weights[2] -= 1;
  if (key == '6')
    weights[2] += 1;
  if (key == '7')
    weights[3] -= 1;
  if (key == '8')
    weights[3] += 1;
  if (key == '9')
    weights[4] -= 1;
  if (key == '0')
    weights[4] += 1;
}
