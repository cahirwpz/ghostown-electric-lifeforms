
final int WIDTH = 320;
final int HEIGHT = 256;

final int CENTERX = WIDTH / 2;
final int CENTERY = HEIGHT / 2;

PGraphics screen;
float maxSize;

void setup() {
  size(640, 512);
  noSmooth();

  screen = createGraphics(WIDTH, HEIGHT);
  screen.noStroke();
  frameRate(5);
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


  for (int i = 0; i <= 15; i++) {
    //abs(cos(frameCount/30.)) > 0.9
    screen.fill(frameCount % 120 <= 5 ? palette[i < 12 ? i+4 : 15] : palette[i]);

    drawFlower(screen, getSize(i), i);
  }
  screen.text(frameCount, 100, 100);
  screen.endDraw();

  amigaPixels();
}
