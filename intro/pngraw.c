#include <types.h>

#define PNG_GRAYSCALE 0
#define PNG_TRUECOLOR 2
#define PNG_INDEXED 3
#define PNG_GRAYSCALE_ALPHA 4
#define PNG_TRUECOLOR_ALPHA 6

typedef struct {
  int width;
  int height;
  char bit_depth;
  char colour_type;
  char compression_method;
  char filter_method;
  char interlace_method;
} IHDR;

#define BYTES(x) (((x) + 7) >> 3)

static __regargs short GetBitsPerPixel(IHDR *ihdr) {
  short bpp = ihdr->bit_depth;

  if (ihdr->colour_type == PNG_GRAYSCALE_ALPHA)
    bpp *= 2;
  else if (ihdr->colour_type == PNG_TRUECOLOR)
    bpp *= 3;
  else if (ihdr->colour_type == PNG_TRUECOLOR_ALPHA)
    bpp *= 4;

  return bpp;
}

static inline short PaethPredictor(short a, short b, short c) {
  short p = a + b - c;
  short pa, pb, pc;

  pa = p - a;
  if (pa < 0)
    pa = -pa;
  pb = p - b;
  if (pb < 0)
    pb = -pb;
  pc = p - c;
  if (pc < 0)
    pc = -pc;

  if ((pa <= pb) && (pa <= pc))
    return a;
  if (pb <= pc)
    return b;
  return c;
}

void ReconstructImage(u_char *pixels, u_char *encoded, IHDR *ihdr) {
  short bpp = GetBitsPerPixel(ihdr);
  short row = BYTES(bpp * (short)ihdr->width);
  short pixel = BYTES(bpp);
  short n = ihdr->height;
  short i, j;

  for (i = 0; i < n; i++) {
    u_char method = *encoded++;

    if (method == 2 && i == 0)
      method = 0;

    if (method == 4 && i == 0)
      method = 1;

    /*
     * Filters are applied to corresponding (!) bytes, not to pixels, regardless
     * of the bit depth or colour type of the image. The filters operate on the
     * byte sequence formed by a scanline.
     */

    if (method == 0) {
      j = row - 1;
      do
        *pixels++ = *encoded++;
      while (--j != -1);
    } else if (method == 1) {
      u_char *left = pixels;
      j = pixel - 1;
      do
        *pixels++ = *encoded++;
      while (--j != -1);
      j = row - pixel - 1;
      do
        *pixels++ = *encoded++ + *left++;
      while (--j != -1);
    } else if (method == 2) {
      u_char *up = pixels - row;
      j = row - 1;
      do
        *pixels++ = *encoded++ + *up++;
      while (--j != -1);
    } else if (method == 3) {
      u_char *left = pixels;
      if (i > 0) {
        u_char *up = pixels - row;
        j = pixel - 1;
        do
          *pixels++ = *encoded++ + *up++ / 2;
        while (--j != -1);
        j = row - pixel - 1;
        do
          *pixels++ = *encoded++ + (*left++ + *up++) / 2;
        while (--j != -1);
      } else {
        j = pixel - 1;
        do
          *pixels++ = *encoded++;
        while (--j != -1);
        j = row - pixel - 1;
        do
          *pixels++ = *encoded++ + *left++ / 2;
        while (--j != -1);
      }
    } else if (method == 4) {
      u_char *left = pixels;
      u_char *leftup = pixels - row;
      u_char *up = pixels - row;
      j = pixel - 1;
      do
        *pixels++ = *encoded++ + *up++;
      while (--j != -1);
      j = row - pixel - 1;
      do {
        *pixels++ = *encoded++ + PaethPredictor(*left++, *up++, *leftup++);
      } while (--j != -1);
    }
  }
}
