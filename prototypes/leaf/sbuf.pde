import java.util.Comparator;
import java.util.Collections;
import java.util.function.Predicate;

class Span {
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
};

class SpanFinished implements Predicate<Span> {
  @Override
    public boolean test(Span s) {
    return s.ys >= s.ye;
  }
}

class DepthComparator implements Comparator<Span>{  
  @Override public int compare(Span s1, Span s2) {
    return s1.c - s2.c;
  }
}

class SpanBuffer {
  ArrayList<Span> spans[]; // as many as lines on the screen

  SpanBuffer(int n) {
    spans = new ArrayList[n];

    for (int i = 0; i < n; i++) {
      spans[i] = new ArrayList<Span>();
    }
  }

  void add(Span s) {
    if (s.ye < 0 || s.ys >= spans.length) {
      return;
    }

    // clip against top of the screen
    if (s.ys < 0) {
      s.xs -= s.ys * s.dxs;
      s.xe -= s.ys * s.dxe;
      s.ys = 0;
    }

    // clip against bottom of the screen
    if (s.ye > spans.length) {
      s.ye = spans.length;
    }

    spans[s.ys].add(s);
  }

  void rasterize() {
    ArrayList<Span> opened = new ArrayList<Span>();

    loadPixels();

    for (int y = 0; y < spans.length; y++) {
      opened.addAll(spans[y]);
      opened.sort(new DepthComparator());

      for (Span s : opened) {
        int xsi = floor(s.xs + 0.5);
        int xei = floor(s.xe + 0.5);

        for (int x = xsi; x < xei; x++) {
          if (x >= 0 && x < width) {
            pixels[s.ys * width + x] = s.c;
          }
        }

        s.xs += s.dxs;
        s.xe += s.dxe;
        s.ys += 1;
      }

      opened.removeIf(new SpanFinished());

      spans[y].clear();
    }

    updatePixels();
  }
}

SpanBuffer sbuf = new SpanBuffer(HEIGHT);
