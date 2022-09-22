PGraphics screen;
final static int scale = 3;
final static int w = 320;
final static int h = 256;

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
};

Vec2 normtab[] = new Vec2[1<<14];

void init_normtab() {
  // x and y assumed to be in range 0-7.875 in 3.4 format
  for (int y = 0; y < 128; y++) {
    for (int x = 0; x < 128; x++) {
      PVector v = new PVector((float)x, (float)y);
      v.normalize();
      // convert back to 3.4
      int nx = (int)(v.x*16);
      int ny = (int)(v.y*16);
      normtab[(y<<7)+x] = new Vec2(nx, ny);
    }
  }
}

void setup() {
  size(960, 768);
  noSmooth();
  screen = createGraphics(scale*w, scale*h);
  init_normtab();
}

// input in 12.4 format
Vec2 normalize(int x, int y) {
  Vec2 res = new Vec2();

  float d = ((int)sqrt(x*x + y*y));
  // scale x and y a bit so the result is normalized vector*16
  res.x = (int)((float)(x<<4)/d);
  res.y = (int)((float)(y<<4)/d);

  return res;
}

// input int 12.4 format
Vec2 normalize2(int x, int y) {
  int sgnx = Integer.signum(x);
  int sgny = Integer.signum(y);
  x = abs(x);
  y = abs(y);
  screen.strokeWeight(2);
  screen.stroke(255, 0, 0);
  // while (x >= 1.0 || y >= 1.0) in 12.4
  while (x >= 16 || y >= 16) {
    screen.point(x+w/2, y+h/2);
    x >>= 1;
    y >>= 1;
  }
  screen.point(x+w/2, y+h/2);
  Vec2 norm = new Vec2(normtab[(y<<7) + x]);
  // fix sign according to the quadrant
  norm.x = sgnx * norm.x; //(norm.x ^ (sgnx < 0 ? 0xffffffff : 0)) + (sgnx < 0 ? 1 : 0);
  norm.y = sgny * norm.y; //(norm.y ^ (sgny < 0 ? 0xffffffff : 0)) + (sgny < 0 ? 1 : 0);
  return norm;
}

void draw() {
  clear();
  screen.beginDraw();
  screen.clear();
  screen.scale(scale);
  
  int mx = mouseX/scale;
  int my = mouseY/scale;
  int cx = w/2;
  int cy = h/2;
  
  screen.strokeWeight(1);
  screen.stroke(255);
  screen.line(cx, cy, mx, my);
  
  Vec2 mouse = new Vec2(mx-cx, my-cy);
  Vec2 norm = normalize(mouse.x, mouse.y);
  screen.stroke(0, 0, 255);
  screen.line(cx, cy, cx+norm.x, cy+norm.y);
  
  Vec2 norm2 = normalize2(mouse.x, mouse.y);
  screen.strokeWeight(1);
  screen.stroke(0, 255, 0);
  screen.line(cx, cy, cx+norm2.x/1.5, cy+norm2.y/1.5);

  screen.endDraw();
  image(screen, 0, 0);
}
