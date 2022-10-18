import java.util.*; 

class Span implements Comparable<Span> {
  int ys, ye;
  float xs, xe;
  float dxs, dxe;
  color c;
  
  Span(float xs, float xe, float dxs, float dxe, int ys, int ye, color c) {
    this.xs = xs;
    this.xe = xe;
    this.dxs = dxs;
    this.dxe = dxe;
    this.ys = ys;
    this.ye = ye;
    this.c = c;
  }
  
  @Override public int compareTo(Span other) {
    return this.ys - other.ys;
  }
};

ArrayList<Span> inactive = new ArrayList<Span>();
ArrayList<Span> active = new ArrayList<Span>();

void rasterize() {
  Collections.sort(inactive);

  loadPixels();
  
  while (active.size() > 0 || inactive.size() > 0) {
    if (active.size() == 0) {
      active.add(inactive.remove(0));
    }
    
    while (inactive.size() > 0 && inactive.get(0).ys == active.get(0).ys) {
      active.add(inactive.remove(0));
    }
    
    // println(active.get(0).ys, active.size(), inactive.size());
    
    for (Span s : active) {
      if (s.ys >= 0 && s.ys < height) {
        int xsi = floor(s.xs + 0.5);
        int xei = floor(s.xe + 0.5);
  
        // println(xsi, xei);
  
        for (int x = xsi; x < xei; x++) {
          if (x >= 0 && x < width) {
            pixels[s.ys * width + x] = s.c;
          }
        }
      }  
    
      s.xs += s.dxs;
      s.xe += s.dxe;
      s.ys++;
    }
    
    while (active.size() > 0 && active.get(0).ys >= active.get(0).ye) {
      active.remove(0);
    }
  }
  
  updatePixels();
}

void addSegment(float xs, float xe, float dxs, float dxe, float ys, float ye) {
  assert(ys <= ye);
  assert(xs <= xe);

  int ysi = ceil(ys);
  int yei = ceil(ye);
  float prestep = ysi - ys; 

  xs += prestep * dxs;
  xe += prestep * dxe;
  
  inactive.add(new Span(xs, ys, dxs, dxe, ysi, yei, g.fillColor));
}


void addTriangle(PVector p1, PVector p2, PVector p3) {
  PVector pt;

  if (p1.y > p2.y) {
    pt = p1;
    p1 = p2;
    p2 = pt;
  }

  if (p1.y > p3.y) {
    pt = p1;
    p1 = p3;
    p3 = pt;
  }

  if (p2.y > p3.y) {
    pt = p2;
    p2 = p3;
    p3 = pt;
  }

  float dy21 = p2.y - p1.y;
  float dy31 = p3.y - p1.y;
  float dy32 = p3.y - p2.y;

  float dx21 = (dy21 > 0.0) ? (p2.x - p1.x) / dy21 : 0.0;
  float dx31 = (dy31 > 0.0) ? (p3.x - p1.x) / dy31 : 0.0;
  float dx32 = (dy32 > 0.0) ? (p3.x - p2.x) / dy32 : 0.0;

  float x_mid = p1.x + dx31 * dy21;

  // Split the triangle into two segments by p2.y
  if (p2.x <= x_mid) {
    addSegment(p1.x, p1.x, dx21, dx31, p1.y, p2.y);
    addSegment(p2.x, x_mid, dx32, dx31, p2.y, p3.y);
  } else {
    addSegment(p1.x, p1.x, dx31, dx21, p1.y, p2.y);
    addSegment(x_mid, p2.x, dx31, dx32, p2.y, p3.y);
  }
}
