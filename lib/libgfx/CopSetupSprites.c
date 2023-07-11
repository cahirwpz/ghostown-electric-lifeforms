#include <sprite.h>

CopInsPairT *CopSetupSprites(CopListT *list) {
  SprDataT *spr = NullSprData;
  CopInsPairT *sprptr = NULL;
  short i;

  for (i = 0; i < 8; i++) {
    CopInsPairT *ins = CopMove32(list, sprpt[i], spr);
    if (!sprptr)
      sprptr = ins;
  }

  return sprptr;
}
