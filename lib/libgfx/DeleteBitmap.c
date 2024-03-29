#include <bitmap.h>
#include <system/memory.h>

void DeleteBitmap(BitmapT *bitmap) {
  if (bitmap) {
    if (!(bitmap->flags & BM_MINIMAL))
      MemFree(bitmap->planes[0]);
    MemFree(bitmap);
  }
}
