#include <custom.h>
#include <effect.h>
#include <cinter.h>
#include <sync.h>
#include <system/interrupt.h>
#include <system/task.h>
#include <system/memory.h>

#define _SYSTEM
#include <system/cia.h>

static CinterPlayerT CinterPlayer[1];
extern u_char CinterModule[];
extern u_char CinterSamples[];
extern u_char CinterSamplesSize[];

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

static int CinterMusic(CinterPlayerT *player) {
  CinterPlay1(player);
  CinterPlay2(player);
  return 0;
}

INTSERVER(CinterMusicServer, 10, (IntFuncT)CinterMusic, CinterPlayer);

static void LoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectLoad(effect);
  }
}

static void UnLoadEffects(EffectT **effects) {
  EffectT *effect;
  for (effect = *effects; effect; effect = *effects++) { 
    EffectUnLoad(effect);
  }
}

static void RunEffects(void) {
  /* Reset frame counter and wait for all time actions to finish. */
  SetFrameCounter(0);
  frameCount = 0;

  for (;;) {
    static short prev = -1;
    short curr = TrackValueGet(&EffectNumber, frameCount);

    // Log("prev: %d, curr: %d, frameCount: %d\n", prev, curr, frameCount);

    if (prev != curr) {
      if (prev >= 0)
        EffectKill(AllEffects[prev]);
      if (curr == -1)
        break;
      EffectInit(AllEffects[curr]);
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

  Log("[Init] Generating Cinter samples\n");
  CinterInit(CinterModule, CinterSamples, CinterPlayer);
#if VQ == 1 || ADPCM == 1 || DELTA == 1
  DecodeSamples(CinterSamples, (int)CinterSamplesSize);
#endif

  TrackInit(&EffectNumber);
  LoadEffects(AllEffects);

  EnableDMA(DMAF_AUDIO);
  AddIntServer(INTB_VERTB, CinterMusicServer);

  RunEffects();

  RemIntServer(INTB_VERTB, CinterMusicServer);
  DisableDMA(DMAF_AUDIO);

  UnLoadEffects(AllEffects);

  return 0;
}
