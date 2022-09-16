
final int WIDTH = 320;
final int HEIGHT = 256;

final int CENTERX = WIDTH / 2;
final int CENTERY = HEIGHT / 2;

PGraphics screen;

void setup() {
  size(640, 512);
  noSmooth();

  screen = createGraphics(WIDTH, HEIGHT);
  screen.noStroke();
}

float getAngle() {
  return PI/3.0 + frameCount / 400.;
}

float getSize(int index) {
  return (-cos(frameCount/30.)+1) * 120.0 + (150 - index*25.0);
}

void drawFlower(PGraphics s, float size, int i) {
  float circleSize = size*i/30.0;

  s.rotate(getAngle());
  if (size > 0) {
    s.rect(-size/2.0, -size/2.0, size, size);
    s.circle(-size/2.0, -size/2.0, circleSize);
    s.circle(size/2.0, -size/2.0, circleSize);
    s.circle(size/2.0, size/2.0, circleSize);
    s.circle(-size/2.0, size/2.0, circleSize);

    s.rotate(PI/4.0);
    s.rect(-size/2.0, -size/2.0, size, size);
    s.circle(-size/2.0, -size/2.0, circleSize);
    s.circle(size/2.0, -size/2.0, circleSize);
    s.circle(size/2.0, size/2.0, circleSize);
    s.circle(-size/2.0, size/2.0, circleSize);
  }
}

void draw() {
  screen.beginDraw();
  screen.translate(width/4, height/4);
  screen.background(0);
  screen.noStroke();

  float timeFactor = constrain(abs(-cos(frameCount/30.)), 0.9, 1);
  int paletteOffset = int(map(timeFactor, 0.9, 1, 0, 4));

  for (int i = 0; i <= 15; i++) {
    screen.fill(paletteOffset != 0 ? palette[i < 12 ? i+paletteOffset : 15] : palette[i]);
    drawFlower(screen, getSize(i), i);
  }
  screen.endDraw();

  amigaPixels();
}
