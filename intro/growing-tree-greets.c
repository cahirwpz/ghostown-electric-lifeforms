#include "growing-tree-greets-data.c"

static int hashTable[] = {
  0x011bad37, 0x7a6433ee, // 3
  0x4ffa0d80, 0x23743a06, // 1
  0x273f164b, 0x9ffa9d90, // 2
  0x74a7beec, 0xb2818113, // 4 
};

// EMPTY PLACEHOLDER
static u_char emptyPlaceholder[] = {
  0,0,
  1,
  0,0, 255,255,
  128,128
};

static u_char *greetsSet0[] = {
  grAltair,
  grAW,
  grDesire,
  emptyPlaceholder,
  NULL,
};

static u_char *greetsSet1[] = {
  grAppendix,
  grCapsule,
  grDreamweb,
  grContinue,
  NULL,
};

static u_char *greetsSet2[] = {
  grArtway,
  grDekadence,
  grElude,
  grTobe,
  NULL,
};
