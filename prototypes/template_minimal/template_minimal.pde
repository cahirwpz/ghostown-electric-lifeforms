
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
void draw() {
  screen.beginDraw();
  
  screen.fill(255);
  screen.circle(CENTERX, CENTERY, 120);
  
  screen.endDraw();

  amigaPixels();
}
