#include <custom.h>
#include <effect.h>
#include <cinter.h>
#include <system/interrupt.h>
#include <system/task.h>
#include <system/memory.h>

static CinterPlayerT CinterPlayer[1];
extern u_char CinterModule[];
extern u_char CinterSamples[];

extern EffectT WeaveEffect;
extern EffectT TextScrollEffect;

static int CinterMusic(CinterPlayerT *player) {
  CinterPlay1(player);
  CinterPlay2(player);
  return 0;
}

INTSERVER(CinterMusicServer, 10, (IntFuncT)CinterMusic, CinterPlayer);

int main(void) {
  EffectT *effect = &WeaveEffect;

  /* NOP that triggers fs-uae debugger to stop and inform GDB that it should
   * fetch segments locations to relocate symbol information read from file. */
  asm volatile("exg %d7,%d7");

  CinterInit(CinterModule, CinterSamples, CinterPlayer);

  EffectLoad(effect);

  EnableDMA(DMAF_AUDIO);
  AddIntServer(INTB_VERTB, CinterMusicServer);

  EffectInit(effect);
  EffectRun(effect);
  EffectKill(effect);

  RemIntServer(INTB_VERTB, CinterMusicServer);
  DisableDMA(DMAF_AUDIO);

  EffectUnLoad(effect);

  return 0;
}
