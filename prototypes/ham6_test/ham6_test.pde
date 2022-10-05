EasyOCS ocs;

void setup() {
  size(640, 512);
  noSmooth();
  
  ocs = new EasyOCS(6, true);
  for (int i = 0; i < 16; i++) {
    ocs.setColor(i, color(i * 16));
  } 
}

void draw() {
  background(0);
  stroke(15);
  fill(0+48);
  rect(64, 64, 320 - 128, 256 - 128, 28);

  stroke(0+16);
  line(128, 64, 128, 192);
  stroke(15+16);
  line(192, 64, 192, 192);

  ocs.update();
}
