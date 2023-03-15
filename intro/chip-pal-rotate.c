typedef struct PalRotT {
  const PaletteT *pal;
  short head;
  short rate;
  short step;
  short len;
  char *indices;
} PalRotT;

static PalRotT rot[3] = {
  {
    .pal = &wireworld_chip_pal,
    .head = 0,
    .rate = 2560,
    .step = 0,
    .len = 5,
    .indices = (char[5]){1, 3, 5, 7, 9}
  }, {
    .pal = &wireworld_chip_pal,
    .head = 0,
    .rate = 2560,
    .step = 0,
    .len = 5,
    .indices = (char[5]){2, 4, 6, 8, 10}
  }, {
    .pal = &wireworld_chip_pal,
    .head = 0,
    .rate = 1664,
    .step = 0,
    .len = 6,
    .indices = (char[6]){11, 12, 13, 14, 13, 12}
  }
};

static void SpriteCyclePal(CopInsT *ins, PalRotT *rot, short nrot) {
  // From https://wiki.amigaos.net/wiki/ILBM_IFF_Interleaved_Bitmap#CRNG
  // "The field rate determines the speed at which the colors will step when
  // color cycling is on. The units are such that a rate of 60 steps per
  // second is represented as 2^14 = 16384. Slower rates can be obtained
  // by linear scaling: for 30 steps/second, rate = 8192; for 1 step/second,
  // rate = 16384/60 ~273."

  // frameCount - lastFrameCount gives wrong frame delta so just trust me
  // on this one (this function gets called every 2 frames in this effect)

  do {
    short i, n;

    rot->step += 2 * rot->rate;
    if (rot->step < (1 << 14))
      continue;

    n = rot->head;
    for (i = 0; i < rot->len; i++) {
      short cn = rot->indices[n];
      short ci = rot->indices[i];
      CopInsSet16(&ins[ci], rot->pal->colors[cn]);

      n++;
      if (n >= rot->len)
        n -= rot->len;
    }

    rot->step -= 1 << 14;
    rot->head++;
    if (rot->head >= rot->len)
      rot->head -= rot->len;

    rot++;
  } while (--nrot);
}
