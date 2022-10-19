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

ArrayList<Span> spans = new ArrayList<Span>();

void rasterize() {
  ArrayList<Span> opened = new ArrayList<Span>();

  int next = 0;

  Collections.sort(spans);

  loadPixels();

  while (opened.size() > 0 || next < spans.size()) {
    while (next < spans.size() && (opened.size() == 0 || spans.get(next).ys == opened.get(0).ys)) {
      opened.add(spans.get(next++));
    }

    ListIterator<Span> iter = opened.listIterator();

    // println("***");

    while (iter.hasNext()) {
      Span s = iter.next();

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
      s.ys += 1;

      if (s.ys >= s.ye) {
        iter.remove();
      }
    }
  }

  updatePixels();

  spans.clear();
}
