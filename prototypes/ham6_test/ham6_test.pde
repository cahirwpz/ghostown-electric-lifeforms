EasyOCS ocs;

void setup() {
  /* Size should be twice the size of Amiga resolution. */
  size(640, 512);
  /* Turn off antialiasing since A500 is way too slow. */
  noSmooth();
  
  ocs = new EasyOCS(6, true);
  for (int i = 0; i < 16; i++) {
    ocs.setColor(i, color(i * 16));
  } 
}

void draw() {
  /* 
   * You're allowed to use normal drawing routines below.
   *
   * Colors that normally would be displayed as shade of grays
   * will be translated to real colors using color registers
   * (aka palette) that are part of `ocs` object.
   *
   * You can even change color registers mid-screen by using
   * color change functions.
   */
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
