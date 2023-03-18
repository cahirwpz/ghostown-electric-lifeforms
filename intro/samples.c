#include <debug.h>

#define _SYSTEM
#include <system/memory.h>

extern u_char Samples[];
#if VQ == 1 || DELTA == 1 || KLANG == 0
extern u_char SamplesSize[];
#endif

#if KLANG == 1
extern u_int AK_Progress;
void AK_Generate(void *TmpBuf asm("a1"));
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

void InitSamples(void) {
#if VQ == 1 || DELTA == 1
  Log("[Init] Decoding samples\n");
  DecodeSamples(Samples, (int)SamplesSize);
#endif

#if KLANG == 1
  Log("[Init] Generating samples\n");
  {
    void *TmpBuf = MemAlloc(32768, MEMF_PUBLIC);
    AK_Generate(TmpBuf);
    MemFree(TmpBuf);
  }
#endif
}
