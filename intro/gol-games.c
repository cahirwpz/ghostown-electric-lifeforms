#include "bitmap.h"
#include "blitter.h"

typedef void(BlitterPhaseFunc)(const BitmapT *, const BitmapT *,
                               const BitmapT *, const BitmapT *,
                               u_short minterms);

typedef struct BlitterPhase {
  BlitterPhaseFunc *blitfunc;
  u_short minterm;
  u_int srcc;
  u_int srcb;
  u_int srca;
  u_int dst;
  u_char pad[10];
} BlitterPhaseT;

typedef struct GameDefinition {
  int num_phases;
  const BlitterPhaseT *phases;
} GameDefinitionT;

static void BlitAdjacentHorizontal(const BitmapT *sourceA,
                                   const BitmapT *sourceB,
                                   const BitmapT *sourceC,
                                   const BitmapT *target, u_short minterms);

static void BlitAdjacentVertical(const BitmapT *sourceA, const BitmapT *sourceB,
                                 const BitmapT *sourceC, const BitmapT *target,
                                 u_short minterms);

static void BlitFunc(const BitmapT *sourceA, const BitmapT *sourceB,
                     const BitmapT *sourceC, const BitmapT *target,
                     u_short minterms);

#define PHASE(sa, sb, sc, d, mt, bf)                                           \
  (BlitterPhaseT) {                                                            \
    .blitfunc = bf, .minterm = mt, .srca = sa * sizeof(void *),                \
    .srcb = sb * sizeof(void *), .srcc = sc * sizeof(void *),                  \
    .dst = d * sizeof(void *), .pad = {}                                       \
  }
#define PHASE_SIMPLE(s, d, mt, bf) PHASE(s, 0, 0, d, mt, bf)

static const GameDefinitionT games[] = {
  /* classic gol */
  {
    .num_phases = 9,
    .phases = (BlitterPhaseT[]){
      PHASE_SIMPLE(0, 1, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 2, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, NANBC | NABNC | NABC | ANBNC | ANBC | ABNC,
                   BlitAdjacentVertical),
      PHASE_SIMPLE(1, 4, NANBNC | NABC | ANBC | ABNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NANBNC | NABC | ANBC | ABNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 6, NANBNC | ABC, BlitAdjacentVertical),
      PHASE(5, 0, 6, 7, NABNC | ANBNC | ANBC | ABC, BlitFunc),
      PHASE(4, 7, 6, 8, NANBC | NABC | ANBNC | ANBC, BlitFunc),
      PHASE(5, 3, 8, 0, NABNC | ANBC, BlitFunc),
    }
  }, 
  /* coagulation */
  {
    .num_phases = 10,
    .phases = (BlitterPhaseT[]){
      PHASE_SIMPLE(0, 1, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 2, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(1, 4, NABC | ANBC | ABNC | NANBNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NABC | ANBC | ABNC | NANBNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 6, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE(3, 5, 4, 7, NANBC | NABNC | NABC | ANBNC | ABC, BlitFunc),
      PHASE(4, 6, 3, 8, NANBNC | NANBC | NABNC | ABNC | ABC, BlitFunc),
      PHASE(8, 0, 4, 9, NANBC | NABNC | ABC, BlitFunc),
      PHASE(7, 9, 0, 0, NANBNC | NANBC | NABC | ABC, BlitFunc),
    }
  },
  /* maze */
  {
    .num_phases = 10,
    .phases = (BlitterPhaseT[]){
      PHASE_SIMPLE(0, 1, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 2, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, NANBC | NABNC | ANBNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(1, 4, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NANBNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 6, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE(4, 6, 5, 7, NANBC | NABNC | ANBNC | ABC, BlitFunc),
      PHASE(3, 5, 7, 8, NANBNC | NANBC | NABNC | ANBC, BlitFunc),
      PHASE(6, 7, 6, 9, NANBC | NABNC | ANBNC | ANBC | ABC, BlitFunc),
      PHASE(9, 0, 8, 0, NANBNC | NABNC | NABC | ABC, BlitFunc),
    }
  },
  /* diamoeba */
  {
    .num_phases = 9,
    .phases = (BlitterPhaseT[]){
      PHASE_SIMPLE(0, 1, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 2, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, NANBNC | NABC | ANBC | ABNC, BlitAdjacentVertical),
      PHASE_SIMPLE(1, 4, NANBNC | NANBC | NABNC | ANBNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NANBNC | NANBC | NABNC | ANBNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 6, NANBNC | NABC | ANBC | ABNC, BlitAdjacentVertical),
      PHASE(3, 0, 5, 7, NANBC | NABNC | ANBNC | ABNC, BlitFunc),
      PHASE(6, 4, 7, 8, NABNC | ANBNC | ABNC | ABC, BlitFunc),
      PHASE(5, 7, 8, 0, NANBC | NABNC | ABNC, BlitFunc),
    }
  },
  /* stains */
  {
    .num_phases = 10,
    .phases = (BlitterPhaseT[]){
      PHASE_SIMPLE(0, 1, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 2, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(1, 4, NANBNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 6, NANBC | NABNC | ANBNC | ABC, BlitAdjacentVertical),
      PHASE(3, 6, 0, 7, NANBC | ABNC, BlitFunc),
      PHASE(7, 5, 4, 8, NANBC | ANBNC | ANBC | ABC, BlitFunc),
      PHASE(8, 3, 5, 9, NANBNC | NANBC | NABNC | ANBNC, BlitFunc),
      PHASE(8, 9, 6, 0, NANBNC | NANBC | NABC | ANBNC, BlitFunc),
    }
  },
  /* day and night */
  {
    .num_phases = 10,
    .phases = (BlitterPhaseT[]){
      PHASE_SIMPLE(0, 1, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 2, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, NANBNC | NANBC | NABNC | ANBNC, BlitAdjacentVertical),
      PHASE_SIMPLE(1, 4, NANBNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 6, NANBC | NABNC | ANBNC | ABC, BlitAdjacentVertical),
      PHASE(3, 0, 6, 7, NANBNC | NABC | ANBC | ABNC, BlitFunc),
      PHASE(7, 4, 0, 8, NABNC | NABC | ABNC, BlitFunc),
      PHASE(8, 6, 3, 9, NANBNC | NANBC | ANBC | ABNC | ABC, BlitFunc),
      PHASE(5, 9, 7, 0, NANBC | ANBNC | ANBC | ABC, BlitFunc),
    }
  },
  /* three four */
  {
    .num_phases = 9,
    .phases = (BlitterPhaseT[]){
      PHASE_SIMPLE(0, 1, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 2, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, NANBNC | NABC | ANBC | ABNC, BlitAdjacentVertical),
      PHASE_SIMPLE(1, 4, NANBNC | NANBC | NABNC | ANBNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NANBC | NABNC | ANBNC | NABC | ANBC | ABNC,
                   BlitAdjacentVertical),
      PHASE_SIMPLE(2, 6, NANBC | NABNC | ANBNC | ABC, BlitAdjacentVertical),
      PHASE(3, 0, 6, 7, NANBC | NABNC | ANBNC | ABNC, BlitFunc),
      PHASE(5, 6, 7, 8, NANBNC | ANBC | ABNC | ABC, BlitFunc),
      PHASE(7, 8, 4, 0, NABNC | ABC, BlitFunc),
    }
  }
};

// take GOL board for rule B12345678/S12345678, only leave pixels which lie on
// the circuit, and remove pixels which were lit one and two generations ago
static const GameDefinitionT wireworlds[] = {
  {
    .num_phases = 10,
    .phases = (BlitterPhaseT[]){
      // 0 = state from one generation ago
      // 1 = state from two generations ago
      // 11 = circuit
      PHASE_SIMPLE(0, 2, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(0, 3, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(2, 4, NANBNC | NABC | ANBC | ABNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(3, 6, NANBNC | NANBC | NABNC | ANBNC | NABC | ANBC | ABNC,
                   BlitAdjacentVertical),
      PHASE_SIMPLE(3, 7, NANBNC | ABC, BlitAdjacentVertical),
      PHASE(7, 4, 0, 8, ANBC | ABNC | ABC, BlitFunc),
      PHASE(6, 5, 8, 9, NANBNC | NANBC | NABC | ANBNC | ABNC | ABC, BlitFunc),
      PHASE(11, 9, 0, 10, ABNC, BlitFunc),
      PHASE(1, 10, 10, 1, NABC, BlitFunc),
    }
  }, {
    .num_phases = 10,
    .phases = (BlitterPhaseT[]){
      // 1 = state from one generation ago
      // 0 = state from two generations ago
      // 11 = circuit
      PHASE_SIMPLE(1, 2, FULL_ADDER, BlitAdjacentHorizontal),
      PHASE_SIMPLE(1, 3, FULL_ADDER_CARRY, BlitAdjacentHorizontal),
      PHASE_SIMPLE(2, 4, NANBNC | NABC | ANBC | ABNC, BlitAdjacentVertical),
      PHASE_SIMPLE(2, 5, NABC | ANBC | ABNC | ABC, BlitAdjacentVertical),
      PHASE_SIMPLE(3, 6, NANBNC | NANBC | NABNC | ANBNC | NABC | ANBC | ABNC,
                   BlitAdjacentVertical),
      PHASE_SIMPLE(3, 7, NANBNC | ABC, BlitAdjacentVertical),
      PHASE(7, 4, 1, 8, ANBC | ABNC | ABC, BlitFunc),
      PHASE(6, 5, 8, 9, NANBNC | NANBC | NABC | ANBNC | ABNC | ABC, BlitFunc),
      PHASE(11, 9, 1, 10, ABNC, BlitFunc),
      PHASE(0, 10, 10, 0, NABC, BlitFunc),
    }
  }
};
