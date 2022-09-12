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
float weights[] = {0.9, 0, 0, 0.5, 0.3};

boolean heatmap = false;
boolean wallBounce = false;

int adds;
int muls;
int divs;
int invSqrts;

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

final float maxspeed = MAX_Q12/2;
final float maxforce = 0.5/2;
final float orgSize = 10;

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
  PVector pos;
  PVector vel;
  PVector accel;
  int bucketid;
  Organism next;
  
  Organism(float x, float y, int id) {
    pos = new PVector(x, y);
    vel = new PVector(random(MIN_Q12, MAX_Q12), random(MIN_Q12, MAX_Q12));
    
    accel = new PVector(random(-0.1, 0.1), random(-0.1, 0.1));
    
    vel.limit(maxspeed);
    bucketid = id;
    next = null;
  }
  
  void update() {
    vel.add(accel);
    vel.limit(maxspeed);
    pos.add(vel);
    Organism _buckets[] = buckets; // force to show up in debugger
    
    if (wallBounce) {
      PVector desiredVel = new PVector(vel.x, vel.y);
      if (pos.x < 0)
        desiredVel.x = maxspeed;
      else if (pos.x > w)
        desiredVel.x = -maxspeed;
      if (pos.y < 0)
        desiredVel.y = maxspeed;
      else if (pos.y > h)
        desiredVel.y = -maxspeed;
      
      applyDesiredVel(desiredVel);
    } else {
      if (pos.x < 0)
        pos.x += w;
      if (pos.y < 0)
        pos.y += h;
      pos.x = pos.x % w;
      pos.y = pos.y % h;
    }

    int tx = constrain(floor(pos.x / tilesize), 0, tilew-1);
    int ty = constrain(floor(pos.y / tilesize), 0, tileh-1);
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
    screen.stroke(#FFFFFF);
    screen.fill(#AAAAAA);
    screen.circle(pos.x, pos.y, orgSize);
    
    PVector vel_graph = PVector.mult(vel, 4);
    screen.stroke(#FF0000);
    screen.line(pos.x, pos.y, pos.x + vel_graph.x, pos.y + vel_graph.y);
    screen.fill(#0000FF);
  }
  
  void applyForce(PVector force) {
    accel.add(force);
    adds += 2;
  }
  
  void applyDesiredVel(PVector desired) {
    PVector steer = PVector.sub(desired, vel);
    steer.limit(maxforce);
    applyForce(steer);
  }
  
  PVector steerForce(PVector desiredVel) {
    PVector steer = PVector.sub(desiredVel, vel);
    steer.limit(maxforce);
    adds += 2;
    divs += 2;
    muls += 4;
    return steer;
  }
  
  PVector steerForce(PVector desiredVel, float max) {
    PVector steer = PVector.sub(desiredVel, vel);
    steer.limit(max);
    return steer;
  }
  
  PVector seek(PVector target) {
    PVector desired = PVector.sub(target, pos);
    // potentially not neeed normalization step? - it starts to look off though
    // space between boids isn't maintained and they tend to lump together more often
    desired.normalize();
    desired.mult(maxspeed);
    adds += 2;
    divs += 2;
    muls += 4;
    return steerForce(desired);
  }
  
  PVector arrive(PVector target) {
    PVector desired = PVector.sub(target, pos);
    float dist = desired.mag();
    float speed = maxspeed;
    if (dist < 100) {
      speed = map(dist, 0, 100, 0, maxspeed);
    }
    desired.normalize();
    desired.mult(speed);
    return steerForce(desired);
  }
  
  PVector separate() {
    PVector avg = new PVector();
    int cnt = 0;
    
    for (Organism other : getNeighbours(this, separationCircle)) {
      float diffx = pos.x - other.pos.x;
      float diffy = pos.y - other.pos.y;
      float d = abs(diffx) + abs(diffy);
      PVector away = new PVector(diffx, diffy);
      // 32 - maximum manhattan distance between two boids in neighbourhood area
      if (d <= 32) {
        avg.add(away);
        cnt++;
      }
      adds += 6;
    }
    
    if (cnt > 0) {
      // dividing by count and normalization step not needed - works well enough
      //avg.div(cnt);
      float norm = avg.x*avg.x + avg.y*avg.y;
      norm = fastInvSqrt(norm);
      avg.x *= norm;
      avg.y *= norm;
      avg.mult(maxspeed); // * 4
      
      PVector steer = PVector.sub(avg, vel);      
      // this imitates steer.limit(maxforce) but instead of
      // limiting vector length it remaps its lenght linearly
      // from 0-8 to 0-maxforce (0.25)
      steer.x /= 32;
      steer.y /= 32;
      adds += 3;
      muls += 4;
      invSqrts += 1;
      avg = steer;
    }
    return avg;
  }
  
  PVector align() {
    PVector avg = new PVector();
    int cnt = 0;
    
    for (Organism other : getNeighbours(this, alignCoheseCircle)) {
      avg.add(other.vel);
      cnt++;
      adds += 3;
    }
    
    if (cnt > 0) {
      float norm = avg.x*avg.x + avg.y*avg.y;
      norm = fastInvSqrt(norm);
      avg.x *= norm;
      avg.y *= norm;
      avg.mult(maxspeed); // * 4

      adds += 1;
      muls += 2;
      invSqrts += 1;
      avg = steerForce(avg);
    }
    return avg;
  }
  
  PVector cohese() {
    PVector avg = new PVector();
    int cnt = 0;
    
    for (Organism other : getNeighbours(this, alignCoheseCircle)) {
      avg.add(other.pos);
      cnt++;
      adds += 3;
    }
    
    if (cnt > 0) {
      avg.div(cnt);
      avg = seek(avg);
      divs += 2;
    }
    return avg;
  }
  
  void applyBehaviors() {
    accel.mult(0);
    PVector sep = separate();
    PVector mouse = seek(new PVector(mouseX/scale, mouseY/scale));
    
    int x = int(pos.x / ff.ffw);
    int y = int(pos.y / ff.ffh);
    PVector ffForce = steerForce(PVector.mult(ff.get(x, y), 20), maxforce);
    
    PVector alignment = align();
    PVector cohesion = cohese();
    
    sep.mult(weights[0]);
    mouse.mult(weights[1]);
    ffForce.mult(weights[2]);
    alignment.mult(weights[3]);
    cohesion.mult(weights[4]);
    
    muls += 4; // don't count mouse, can potentially be replaced with shifts and adds
    
    applyForce(sep);
    applyForce(alignment);
    applyForce(cohesion);
    applyForce(mouse);
    applyForce(ffForce);
  }
  
}

void setup() {
  size(960, 768); //<>//
  noSmooth();
  randomSeed(42);
  noiseSeed(38);
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
  screen.text(String.format("Separation: %.2f\nMouse: %.2f\nFlowfield: %.2f\nAlignment: %.2f\nCohesion: %.2f",
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
  
  print(String.format("adds: %d, muls: %d, divs: %d, raster lines: %d-%d-%d\n", 
                       adds, muls, divs, rasterBest, rasterlines, rasterWorst));

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
    weights[0] -= 0.05;
  if (key == '2')
    weights[0] += 0.05;
  if (key == '3')
    weights[1] -= 0.05;
  if (key == '4')
    weights[1] += 0.05;
  if (key == '5')
    weights[2] -= 0.05;
  if (key == '6')
    weights[2] += 0.05;
  if (key == '7')
    weights[3] -= 0.05;
  if (key == '8')
    weights[3] += 0.05;
  if (key == '9')
    weights[4] -= 0.05;
  if (key == '0')
    weights[4] += 0.05;
}
