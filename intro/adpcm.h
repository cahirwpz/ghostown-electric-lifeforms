#ifndef __ADPCM_H__
#define __ADPCM_H__

#include <types.h>

void AdpcmInit(void);
void AdpcmDecode(uint8_t *input, int n, int8_t *output);

#endif /* !__ADPCM_H__ */
