import java.util.Comparator;
import java.util.Collections;
import java.util.function.Predicate;

class Segment {
  int ys, ye;
  float xs, xe;
  float dxs, dxe;
  color c;

  Segment(float xs, float xe, float dxs, float dxe, int ys, int ye, color c) {
    this.xs = xs;
    this.xe = xe;
    this.dxs = dxs;
    this.dxe = dxe;
    this.ys = ys;
    this.ye = ye;
    this.c = c;
  }
};

class SegmentFinished implements Predicate<Segment> {
  @Override
    public boolean test(Segment s) {
    return s.ys >= s.ye;
  }
}

class SegmentDepth implements Comparator<Segment>{  
  @Override public int compare(Segment s1, Segment s2) {
    return s1.c - s2.c;
  }
}

class SegmentBuffer {
  ArrayList<Segment> segments[]; // as many as lines on the screen

  SegmentBuffer() {
    segments = new ArrayList[HEIGHT];

    for (int i = 0; i < HEIGHT; i++) {
      segments[i] = new ArrayList<Segment>();
    }
  }

  void add(Segment s) {
    if (s.ye < 0 || s.ys >= HEIGHT) {
      return;
    }

    // clip against top of the screen
    if (s.ys < 0) {
      s.xs -= s.ys * s.dxs;
      s.xe -= s.ys * s.dxe;
      s.ys = 0;
    }

    // clip against bottom of the screen
    if (s.ye > HEIGHT) {
      s.ye = HEIGHT;
    }

    segments[s.ys].add(s);
  }

  void rasterize() {
    ArrayList<Segment> active = new ArrayList<Segment>();

    loadPixels();

    for (int y = 0; y < HEIGHT; y++) {
      active.addAll(segments[y]);
      active.sort(new SegmentDepth());

      for (Segment s : active) {
        int xs = floor(s.xs + 0.5);
        int xe = floor(s.xe + 0.5);
        
        if (xe > 0 && xs < WIDTH) {
          xs = max(xs, 0);
          xe = min(xe, WIDTH);
 
          for (int x = xs; x < xe; x++) {
            pixels[s.ys * width + x] = s.c;
          }
        }

        s.xs += s.dxs;
        s.xe += s.dxe;
        s.ys += 1;
      }

      active.removeIf(new SegmentFinished());

      segments[y].clear();
    }

    updatePixels();
  }
}

SegmentBuffer sbuf = new SegmentBuffer();
