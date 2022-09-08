#ifndef __ADPCM_H__
#define __ADPCM_H__

#include <types.h>

void ADPCMDecoder_InitTables(void);
void ADPCMDecoder(uint8_t *input asm("a0"), int size asm("d0"),
                  int8_t *output asm("a1"));

#endif /* !__ADPCM_H__ */
