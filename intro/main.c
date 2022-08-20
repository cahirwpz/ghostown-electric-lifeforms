#include <custom.h>
#include <effect.h>
#include <cinter.h>
#include <system/interrupt.h>
#include <system/task.h>
#include <system/memory.h>

extern u_char binary_data_JazzCat_RitchieGlitchie_ctr_start[];
#define module binary_data_JazzCat_RitchieGlitchie_ctr_start

extern u_char binary_data_JazzCat_RitchieGlitchie_smp_start[];
#define samples binary_data_JazzCat_RitchieGlitchie_smp_start

static CinterPlayerT player[1];
extern EffectT Effect;

static int CinterMusic(void) {
  CinterPlay1(player);
  CinterPlay2(player);
  return 0;
}

INTSERVER(CinterMusicServer, 10, (IntFuncT)CinterMusic, NULL);

int main(void) {
  /* NOP that triggers fs-uae debugger to stop and inform GDB that it should
   * fetch segments locations to relocate symbol information read from file. */
  asm volatile("exg %d7,%d7");

  CinterInit(module, samples, player);

  EffectLoad(&Effect);

  EnableDMA(DMAF_AUDIO);
  AddIntServer(INTB_VERTB, CinterMusicServer);

  EffectInit(&Effect);
  EffectRun(&Effect);
  EffectKill(&Effect);

  RemIntServer(INTB_VERTB, CinterMusicServer);
  DisableDMA(DMAF_AUDIO);

  EffectUnLoad(&Effect);

  return 0;
}
