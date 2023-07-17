#include <sprite.h>

CopInsPairT *CopSetupSprites(CopListT *list) {
  SprDataT *spr = NullSprData;
  CopInsPairT *sprptr = CopInsPtr(list);
  short i;

  for (i = 0; i < 8; i++)
    CopMove32(list, sprpt[i], spr);

  return sprptr;
}
