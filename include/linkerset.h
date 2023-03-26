#ifndef __LINKERSET_H__
#define __LINKERSET_H__

#include <asm.h>
#include <stab.h>

/*
 * Macros for handling symbol table information (aka linker set elements).
 *
 * https://sourceware.org/gdb/onlinedocs/stabs/Non_002dStab-Symbol-Types.html
 */

/* Add symbol 's' to list 'l' (type 't': N_SETT, N_SETD, N_SETB). */
#define ADD2LIST(s, l, t) STABS(_L(l), t, 0, 0, _L(s))

/*
 * Install private constructors and destructors pri MUST be in [-127, 127]
 * Constructors are called in ascending order of priority,
 * while destructors in descending.
 */
#if 0
#define ADD2INIT(ctor, pri)                                                    \
  ADD2LIST(ctor, __INIT_LIST__, N_SETT);                                       \
  STABS(_L(__INIT_LIST__), N_SETA, 0, 0, pri + 128)
#else
#define ADD2INIT(ctor, pri)
#endif

#if 0
#define ADD2EXIT(dtor, pri)                                                    \
  ADD2LIST(dtor, __EXIT_LIST__, N_SETT);                                       \
  STABS(_L(__EXIT_LIST__), N_SETA, 0, 0, 128 - pri)
#else
#define ADD2EXIT(dtor, pri)
#endif

#endif /* !__LINKERSET_H__ */
