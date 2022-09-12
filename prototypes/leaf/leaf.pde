
final int WIDTH = 320;
final int HEIGHT = 256;

final int CENTERX = WIDTH / 2;
final int CENTERY = HEIGHT / 2;

PGraphics screen;

void drawTriangle(PGraphics screen, float x_center, float y_center, float r, int i) {
  float tf = sin(frameCount/150.0);
  float x1 = r*cos(0) + x_center;
  float y1 = r*sin(0) + y_center;
  float x2 = r*cos(1./3*2*PI) + x_center;
  float y2 = r*sin(1./3*2*PI) + y_center;
  float x3 = r*cos(2./3*2*PI) + x_center;
  float y3 = r*sin(2./3*2*PI) + y_center;
  screen.pushMatrix();
  screen.rotate(radians((pow(cos(PI * tf/2.0), 0.5+i/15.0)+1)*120.0));
  screen.triangle(x1, y1, x2, y2, x3, y3);
  screen.popMatrix();
  screen.pushMatrix();
  screen.rotate(-radians((pow(cos(PI * tf/2.0), 0.5+i/15.0)+1)*120.0));
  screen.triangle(x1, y1, x2, y2, x3, y3);
  screen.popMatrix();
}

void setup() {
  size(640, 512);
  noSmooth();

  screen = createGraphics(WIDTH, HEIGHT);
}
void draw() {

  screen.beginDraw();
  screen.background(0);
  screen.translate(width/4., height/4.);
  screen.noStroke();
  screen.rotate(radians(-90));
  for (int i = 0; i <= 15; i++) {
    screen.fill(map(i, 0, 15, 16, 256));
    drawTriangle(screen, 0, 0, 190-i*12, i);
  }

  screen.endDraw();

  amigaPixels();
}
