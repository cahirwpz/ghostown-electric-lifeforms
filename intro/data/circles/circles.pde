final int DIAMETER = 32;
final boolean DRAWBG = false;

PrintWriter output;

void setup() {
  size(33, 305);
  noStroke();
  noSmooth();

  output = createWriter("circles.txt"); 

  noLoop();
}

void draw() {
  background(0);
  
  for (int r = 1, y = 0; r <= DIAMETER / 2; r++) {
    boolean odd = r % 2 == 1;
    int d = r * 2 + 1;
    if (DRAWBG) {
      fill(odd ? 64 : 96);
      rect(0, y, (d + 15) & -15, d);
    }
    fill(255);
    circle(r, y + r, d);
    println(y, d);
    output.println(String.format("--bitmap circle%d,%dx%dx1,extract_at=%dx%d \\", r, d, d, 0, y));
    y += d + 1;
  }

  output.flush();
  output.close();
 
  save("circles.png");
}
