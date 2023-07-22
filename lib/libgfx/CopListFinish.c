#include <copper.h>
#include <debug.h>

CopListT *CopListFinish(CopListT *list) {
  CopInsT *ins = list->curr;
  *((u_int *)ins)++ = 0xfffffffe;
  list->curr = ins;
  Log("Used copper list slots: %ld\n", (ptrdiff_t)(list->curr - list->entry));
  return list;
}
