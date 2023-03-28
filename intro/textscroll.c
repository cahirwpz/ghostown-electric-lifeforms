#include <intro.h>
#include <blitter.h>
#include <copper.h>
#include <gfx.h>
#include <system/memory.h>

#define WIDTH 640
#define HEIGHT 256
#define DEPTH 1

#define SIZE 8

#define COLUMNS (WIDTH / SIZE)
#define LINES   (HEIGHT / SIZE)

static __code short active = 0;

typedef CopInsT *CopInsPtrT;

static CopListT *cp[2];
static CopInsPtrT (*linebpl)[2][HEIGHT];
static BitmapT *scroll;

static __code short last_line = -1;
static __code char *line_start;

extern uint8_t Text[];

#include "data/text-scroll-font.c"

static CopListT *MakeCopperList(CopInsPtrT *linebpl) {
  CopListT *cp = NewCopList(100 + 3 * HEIGHT);
  CopInit(cp);
  CopSetupBitplanes(cp, NULL, scroll, DEPTH);
  {
    u_short i;
    void *ptr = scroll->planes[0];

    for (i = 0; i < HEIGHT; i++, ptr += scroll->bytesPerRow) {
      CopWaitSafe(cp, Y(i), 0);
      linebpl[i] = CopMove32(cp, bplpt[0], ptr);
    }
  }
  CopEnd(cp);
  return cp;
}

static void Init(void) {
  scroll = NewBitmap(WIDTH, HEIGHT + 16, 1);

  line_start = Text;

  SetupPlayfield(MODE_HIRES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadPalette(&font_pal, 0);

  linebpl = MemAlloc(sizeof(CopInsPtrT) * 2 * HEIGHT, MEMF_PUBLIC);
  cp[0] = MakeCopperList((*linebpl)[0]);
  cp[1] = MakeCopperList((*linebpl)[1]);

  CopListActivate(cp[active]);

  EnableDMA(DMAF_RASTER|DMAF_BLITTER);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER|DMAF_BLITTER|DMAF_COPPER);

  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
  MemFree(linebpl);
  DeleteBitmap(scroll);
}

static void RenderLine(u_char *dst, char *line, short size) {
  short dwidth = scroll->bytesPerRow;
  short swidth = font.bytesPerRow;
  u_char *src = font.planes[0];
  short x = 0;

  while (--size >= 0) {
    short i = (*line++) - 32;
    short j = x++;
    short h = 8;

    if (i < 0)
      continue;

    while (--h >= 0) {
      dst[j] = src[i];
      i += swidth;
      j += dwidth;
    }
  }
}

static void SetupLinePointers(void) {
  CopInsT **ins = (*linebpl)[active];
  void *plane = scroll->planes[0];
  int stride = scroll->bytesPerRow;
  int bplsize = scroll->bplSize;
  short y = (int)(frameCount / 2 + 8) % (short)scroll->height;
  void *start = plane + (short)stride * y;
  void *end = plane + bplsize;
  short n = HEIGHT;

  while (--n >= 0) {
    if (start >= end)
      start -= bplsize;
    CopInsSet32(*ins++, start);
    start += stride;
  }
}

static char *NextLine(char *str) {
  for (; *str; str++)
    if (*str == '\n')
      return ++str;
  return str;
}

static void RenderNextLineIfNeeded(void) {
  Area2D rect = {0, 0, WIDTH, SIZE};
  short s = frameCount / 16;

  if (s > last_line) {
    void *ptr = scroll->planes[0];
    short line_num = (s % (LINES + 2)) * SIZE;
    char *line_end;
    short size;

    line_end = NextLine(line_start);
    size = (line_end - line_start) - 1;

    ptr += line_num * scroll->bytesPerRow;

    rect.y = line_num;
    BitmapClearArea(scroll, &rect);
    WaitBlitter();
    RenderLine(ptr, line_start, min(size, COLUMNS));

    last_line = s;
    line_start = line_end;
  }
}

static void Render(void) {
  SetupLinePointers();
  RenderNextLineIfNeeded();

  CopListRun(cp[active]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(TextScroll, NULL, NULL, Init, Kill, Render);
