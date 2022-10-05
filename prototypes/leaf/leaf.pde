final color[] palette = new color[] {
  color(17, 0, 17), 
  color(21, 17, 30), 
  color(25, 34, 43), 
  color(29, 51, 56), 
  color(33, 68, 69), 
  color(37, 85, 82), 
  color(41, 102, 95), 
  color(45, 119, 108), 
  color(49, 136, 122), 
  color(51, 153, 136), 
  color(79, 170, 147), 
  color(107, 187, 158), 
  color(136, 204, 170), 
  color(164, 221, 181), 
  color(192, 238, 192), 
  color(221, 255, 204), 
};

EasyOCS ocs;

void drawTriangle(float xc, float yc, float r, float angle) {
  angle -= radians(90);

  float x1 = r * cos(angle + TWO_PI * 0 / 3) + xc;
  float y1 = r * sin(angle + TWO_PI * 0 / 3) + yc;
  float x2 = r * cos(angle + TWO_PI * 1 / 3) + xc;
  float y2 = r * sin(angle + TWO_PI * 1 / 3) + yc;
  float x3 = r * cos(angle + TWO_PI * 2 / 3) + xc;
  float y3 = r * sin(angle + TWO_PI * 2 / 3) + yc;

  triangle(x1, y1, x2, y2, x3, y3);
}

void setup() {
  size(640, 512);
  noSmooth();

  ocs = new EasyOCS(4, false);
  ocs.setPalette(palette);
}

void draw() {
  background(0);
  noStroke();

  float step = sin(radians(frameCount / PI)) * PI;

  for (int i = 0; i < 16; i++) {
    float angle = radians(frameCount + i * step);
    float radius = (16 - i) * 12;
    fill(i);
    drawTriangle(WIDTH / 2, HEIGHT / 2, radius, angle);
    drawTriangle(WIDTH / 2, HEIGHT / 2, radius, -angle);
  }

  ocs.update();
}
