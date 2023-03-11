#include <custom.h>
#include <effect.h>
#include <ptplayer.h>
#include <sync.h>
#include <system/task.h>

#define _SYSTEM
#include <system/memory.h>
#include <system/cia.h>

extern u_char Module[];
extern u_char Samples[];
#if VQ == 1 || DELTA == 1 || KLANG == 0
extern u_char SamplesSize[];
#endif

extern EffectT LogoEffect;
extern EffectT WeaveEffect;
extern EffectT TextScrollEffect;
extern EffectT TurmiteEffect;
extern EffectT GrowingTreeEffect;
extern EffectT TileMoverEffect;
extern EffectT SeaAnemoneEffect;
extern EffectT WireworldEffect;
extern EffectT GameOfLifeEffect;
extern EffectT VitruvianEffect;

short frameFromStart;
short frameTillEnd;

#include "data/intro.c"

static EffectT *AllEffects[] = {
  &LogoEffect,
  &TextScrollEffect,
  &WeaveEffect,
  &TurmiteEffect,
  &GrowingTreeEffect,
  &TileMoverEffect,
  &SeaAnemoneEffect,
  &WireworldEffect,
  &GameOfLifeEffect,
  &VitruvianEffect,
  NULL,
};

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
void AK_Generate(void *TmpBuf asm("a1"));
#endif

static void ShowMemStats(void) {
  Log("[Memory] CHIP: %d FAST: %d\n", MemAvail(MEMF_CHIP), MemAvail(MEMF_FAST));
}

static void LoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectLoad(effect);
    if (effect->Load)
      ShowMemStats();
  }
}

static void UnLoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectUnLoad(effect);
  }
}

#define SYNCPOS(pos) (((((pos) & 0xff00) >> 2) | ((pos) & 0x3f)) * 6)

static void RunEffects(void) {
  /* Set the beginning of intro. Useful for effect synchronization! */
  short pos = 0;

  frameCount = SYNCPOS(pos);
  SetFrameCounter(frameCount);
  PtData.mt_SongPos = pos >> 8;
  PtData.mt_PatternPos = (pos & 0x3f) << 4;
  PtEnable = -1;

  for (;;) {
    static short prev = -1;
    short curr = TrackValueGet(&EffectNumber, frameCount);

    // Log("prev: %d, curr: %d, frameCount: %d\n", prev, curr, frameCount);

    if (prev != curr) {
      if (prev >= 0) {
        EffectKill(AllEffects[prev]);
        ShowMemStats();
      }
      if (curr == -1)
        break;
      EffectInit(AllEffects[curr]);
      ShowMemStats();
    }

    lastFrameCount = ReadFrameCounter() - 1;

    {
      EffectT *effect = AllEffects[curr];
      short t = ReadFrameCounter();
      frameCount = t;
      frameFromStart = t - CurrKeyFrame(&EffectNumber);
      frameTillEnd = NextKeyFrame(&EffectNumber) - t;
      if (effect->Render)
        effect->Render();
      lastFrameCount = t;
    }

    prev = curr;
  }
}

int main(void) {
  /* NOP that triggers fs-uae debugger to stop and inform GDB that it should
   * fetch segments locations to relocate symbol information read from file. */
  asm volatile("exg %d7,%d7");

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

  PtInstallCIA();
  PtInit(Module, Samples, 0);

  TrackInit(&EffectNumber);
  LoadEffects(AllEffects);

  RunEffects();

  PtEnd();
  PtRemoveCIA();

  UnLoadEffects(AllEffects);

  return 0;
}
