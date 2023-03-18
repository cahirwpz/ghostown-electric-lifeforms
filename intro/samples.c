#include <intro.h>
#include <debug.h>
#include <copper.h>
#include <gfx.h>
#include <line.h>

#define _SYSTEM
#include <system/memory.h>
#include <system/interrupt.h>

extern u_char Samples[];
#if VQ == 1 || DELTA == 1 || KLANG == 0
extern u_char SamplesSize[];
#endif

#if DELTA == 1
static void DecodeSamples(u_char *smp, int size) {
  u_char data = *smp++;
  short n = (size + 7) / 8 - 1;
  short k = size & 7;

  Log("[Init] Decoding delta samples (%d bytes)\n", size);

  switch (k) {
  case 0: do { data += *smp; *smp++ = data;
  case 7:      data += *smp; *smp++ = data;
  case 6:      data += *smp; *smp++ = data;
  case 5:      data += *smp; *smp++ = data;
  case 4:      data += *smp; *smp++ = data;
  case 3:      data += *smp; *smp++ = data;
  case 2:      data += *smp; *smp++ = data;
  case 1:      data += *smp; *smp++ = data;
          } while (--n != -1);
  }
}
#endif

#if VQ == 1
static void DecodeSamples(u_char *smp, int size) {
  char *copy = MemAlloc(size, MEMF_PUBLIC);
  memcpy(copy, smp, size);

  Log("[Init] Decoding VQ samples (%d bytes)\n", size);
  
  {
    u_char *data = copy;
    u_char *out = smp;

    for (;;) {
      u_char *codebook;
      short n;
      
      n = (*data++) << 8;
      n |= *data++;
      
      if (n == 0)
        break;

      codebook = data;
      data += 1024;
      n--;

      do {
        short cwi = *data++ << 2;
        u_char *cw = codebook + cwi;
        *out++ = *cw++;
        *out++ = *cw++;
        *out++ = *cw++;
        *out++ = *cw++;
      } while (--n != -1);
    }
  }

  MemFree(copy);
}
#endif

#if KLANG == 1
extern u_int AK_Progress;
extern u_int AK_ProgressLen;
void AK_Generate(void *TmpBuf asm("a1"));

#define X1 32
#define Y1 120
#define X2 288
#define Y2 136

static int ProgressBarUpdate(void) {
  static __code short x = 0;
  short newX = div16(AK_Progress, AK_ProgressLen >> 8);
  for (; x < newX; x++) {
    CpuLine(X1 + x, Y1, X1 + x, Y2);
  }
  return 0;
}

INTSERVER(ProgressBarInterrupt, 0, (IntFuncT)ProgressBarUpdate, NULL);

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 1

static void GenerateSamples(void *buf) {
  BitmapT *screen = NewBitmap(WIDTH, HEIGHT, DEPTH);
  CopListT *cp = NewCopList(40);

  CpuLineSetup(screen, 0);
  CpuLine(X1 - 2, Y1 - 2, X2 + 2, Y1 - 2);
  CpuLine(X1 - 2, Y2 + 1, X2 + 2, Y2 + 1);
  CpuLine(X1 - 2, Y1 - 2, X1 - 2, Y2 + 1);
  CpuLine(X2 + 2, Y1 - 2, X2 + 2, Y2 + 1);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  SetColor(0, 0x000);
  SetColor(1, 0xfff);

  CopInit(cp);
  CopSetupBitplanes(cp, NULL, screen, DEPTH);
  CopEnd(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);

  AddIntServer(INTB_VERTB, ProgressBarInterrupt);
  AK_Generate(buf);
  RemIntServer(INTB_VERTB, ProgressBarInterrupt);

  DisableDMA(DMAF_RASTER);
  DeleteCopList(cp);

  DeleteBitmap(screen);
}
#endif

void InitSamples(void) {
#if VQ == 1 || DELTA == 1
  Log("[Init] Decoding samples\n");
  DecodeSamples(Samples, (int)SamplesSize);
#endif

#if KLANG == 1
  Log("[Init] Generating samples\n");
  {
    void *TmpBuf = MemAlloc(32768, MEMF_PUBLIC);
    GenerateSamples(TmpBuf);
    MemFree(TmpBuf);
  }
#endif
}
