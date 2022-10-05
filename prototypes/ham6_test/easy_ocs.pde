final int WIDTH = 320;
final int HEIGHT = 256;
final int CCSLOTS = 13;
final int NCOLORS = 32;

class EasyOCS {
  class ColorChange {
    int idx;
    color col;
  };
  
  color[] colorRegister;
  ColorChange[][] colorChange;
  color[] palette;
  
  int depth = 6;
  boolean ham6 = true;

  EasyOCS(int depth, boolean ham6) {
    assert(width == WIDTH * 2);
    assert(height == HEIGHT * 2);
    
    this.depth = depth;
    this.ham6 = ham6;
    
    palette = new color[NCOLORS];
    colorRegister = new color[NCOLORS];
    
    colorChange = new ColorChange[HEIGHT][CCSLOTS];
    for (int i = 0; i < HEIGHT; i++) {
      for (int j = 0; j < CCSLOTS; j++) {
        colorChange[i][j] = new ColorChange();
        colorChange[i][j].idx = -1;
      }
    }
  }
  
  void setColor(int i, color c) {
    assert(i >= 0 && i < 32);
    palette[i] = c & 0xf0f0f0;
  }
  
  void addColorChange(int h, int i, color c) {
  }
  
  void removeColorChange(int h, int i) {
    for (int j = 0; j < CCSLOTS; j++) {
    }
  }
  
  void update() {
    copy(0, 0, WIDTH, HEIGHT, WIDTH, HEIGHT, WIDTH, HEIGHT);
    
    for (int i = 0; i < NCOLORS; i++) {
      colorRegister[i] = palette[i];
    }
    
    loadPixels();
    for (int j = 0; j < HEIGHT; j++) {
      // emulates palette color exchange with copper between two raster lines
      // (we don't perform color swapping while drawing the pixels)
      for (int i = 0; i < CCSLOTS; i++) {
        if (colorChange[j][i].idx >= 0) {
          colorRegister[i] = colorChange[j][i].col;
        }
      }
  
      // HAM-6 emulation
      color p = colorRegister[0];
      for (int i = 0; i < WIDTH; i++) {
        int v = pixels[(j + HEIGHT) * width + (i + WIDTH)];
        if (ham6) {
          int c = (v & 0x0f) << 4;
          int d = (v & 0x30) >> 4;
          if (d == 0) {
            p = palette[v & 15];
          } else if (d == 1) /* 01 -> blue */ {
            p = color(red(p), green(p), c);
          } else if (d == 2) /* 10 -> red */ {
            p = color(c, green(p), blue(p));
          } else if (d == 3) /* 11 -> green */ {
            p = color(red(p), c, blue(p));
          }
        } else {
          p = palette[v & 31];
        }
        p &= 0x00f0f0f0;
        if ((v & 32) != 0 && !ham6) {
          p >>= 1; /* extra half brite emulation */
        }
        p |= 0xff000000;
        pixels[(j * 2 + 0) * width + (i * 2 + 0)] = p;
        pixels[(j * 2 + 1) * width + (i * 2 + 0)] = p;
        pixels[(j * 2 + 0) * width + (i * 2 + 1)] = p;
        pixels[(j * 2 + 1) * width + (i * 2 + 1)] = p;
      }
    }
    updatePixels();
  }
}
