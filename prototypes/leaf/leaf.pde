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

void drawTriangle(float x_center, float y_center, float r, float angle) {

  float x1 = r * cos(0) + x_center;
  float y1 = r * sin(0) + y_center;
  float x2 = r * cos(1./3*2*PI) + x_center;
  float y2 = r * sin(1./3*2*PI) + y_center;
  float x3 = r * cos(2./3*2*PI) + x_center;
  float y3 = r * sin(2./3*2*PI) + y_center;
  pushMatrix();
  rotate(angle);
  triangle(x1, y1, x2, y2, x3, y3);
  popMatrix();
  pushMatrix();
  rotate(-angle);
  triangle(x1, y1, x2, y2, x3, y3);
  popMatrix();
}

void setup() {
  size(640, 512);
  noSmooth();
  
  ocs = new EasyOCS(4, false);
  ocs.setPalette(palette);
}

void draw() {
  background(0);
  translate(WIDTH / 2, HEIGHT / 2);
  noStroke();
  rotate(radians(-90));
  for (int i = 0; i < 16; i++) {
    float angle = radians((frameCount/100.0 + sin(frameCount/1000.0*i/10.0))*60.0);
    fill(i);
    drawTriangle(0, 0, 190 - i * 12, angle);
  }

  ocs.update();
}
