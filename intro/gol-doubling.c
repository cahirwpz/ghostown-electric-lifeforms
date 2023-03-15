// Used by CPU to quickly transform 1x1 pixels into 2x1 pixels.
static u_short double_pixels[256];

static void MakeDoublePixels(void) {
  u_short *data = double_pixels;
  u_short w = 0;
  short n = 255;

  do {
    *data++ = w;
    w |= 0xAAAA; /* let carry propagate through odd bits */
    w++;
    w &= 0x5555;
    w |= w << 1;
  } while (--n != -1);
}

static void (*PixelDouble)(u_char *source asm("a0"), u_short *target asm("a1"),
                           u_short *lut asm("a2"));

#define PixelDoubleSize \
  ((BOARD_WIDTH / 8) * BOARD_HEIGHT * 10 + BOARD_HEIGHT * 2 + 6)

static void MakePixelDoublingCode(void) {
  u_short *code = (void *)PixelDouble;
  u_short x, y;

  *code++ = 0x7200 | (EXT_BOARD_MODULO & 0xFF); // moveq #EXT_BOARD_MODULO,d1
  for (y = 0; y < BOARD_HEIGHT; y++) {
    for (x = 0; x < BOARD_WIDTH / 8; x++) {
      *code++ = 0x7000; // moveq.l #0,d0           # 4
      *code++ = 0x1018; // move.b (a0)+,d0         # 8
      *code++ = 0xd040; // add.w  d0,d0            # 4
      *code++ = 0x32f2;
      *code++ = 0x0000; // move.w (a2,d0.w),(a1)+  # 18
      // perform a lookup in the pixel doubling lookup table
      // (e.g. 00100110 -> 0000110000111100)
      // *double_target++ = double_pixels[*double_src++];
    }
    *code++ = 0xD1C1; // adda.l d1,a0
    // double_src += EXT_BOARD_MODULO & 0xFF;
    // bitmap modulo - skip the extra EXT_BOARD_MODULO bytes on the edges
    // (EXT_WIDTH_LEFT/8 bytes on the left, EXT_WIDTH_RIGHT/8 bytes on the right
    // on the next row)
  }
  *code++ = 0x4e75; // rts
}
