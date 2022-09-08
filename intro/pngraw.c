#include <types.h>

#define PNG_GRAYSCALE 0
#define PNG_TRUECOLOR 2
#define PNG_INDEXED 3
#define PNG_GRAYSCALE_ALPHA 4
#define PNG_TRUECOLOR_ALPHA 6

typedef struct RawPng {
  short width;
  short height;
  short row_size;
  short pix_size;
  u_char data[0];
} RawPngT;

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

#define BYTES(x) (((x) + 7) >> 3)

void RawPngDecode(RawPngT *rawpng) {
  u_char *pixels = rawpng->data;
  u_char *encoded = rawpng->data;
  register short n asm("d6") = rawpng->height - 1;
  register short j asm("d7");

  do {
    u_char method = *encoded++;

    /*
     * Filters are applied to corresponding (!) bytes, not to pixels, regardless
     * of the bit depth or colour type of the image. The filters operate on the
     * byte sequence formed by a scanline.
     */

    if (method == 0) {
      j = rawpng->row_size - 1;
      do
        *pixels++ = *encoded++;
      while (--j != -1);
    } else if (method == 1) {
      u_char *left = pixels;
      j = rawpng->pix_size - 1;
      do
        *pixels++ = *encoded++;
      while (--j != -1);
      j = rawpng->row_size - rawpng->pix_size - 1;
      do
        *pixels++ = *encoded++ + *left++;
      while (--j != -1);
    } else if (method == 2) {
      u_char *up = pixels - rawpng->row_size;
      j = rawpng->row_size - 1;
      do
        *pixels++ = *encoded++ + *up++;
      while (--j != -1);
    } else if (method == 3) {
      u_char *left = pixels;
      u_char *up = pixels - rawpng->row_size;
      j = rawpng->pix_size - 1;
      do
        *pixels++ = *encoded++ + ((short)*up++ >> 1);
      while (--j != -1);
      j = rawpng->row_size - rawpng->pix_size - 1;
      do
        *pixels++ = *encoded++ + ((short)(*left++ + *up++) >> 1);
      while (--j != -1);
    } else if (method == 4) {
      u_char *left = pixels;
      u_char *leftup = pixels - rawpng->row_size;
      u_char *up = pixels - rawpng->row_size;
      j = rawpng->pix_size - 1;
      do
        *pixels++ = *encoded++ + *up++;
      while (--j != -1);
      j = rawpng->row_size - rawpng->pix_size - 1;
      do {
        *pixels++ = *encoded++ + PaethPredictor(*left++, *up++, *leftup++);
      } while (--j != -1);
    }
  } while (--n != -1);
}
