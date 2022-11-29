# Reliable transfers may only happen 3 or more E-cycles basis.
# http://lallafa.de/blog/2015/09/amiga-parallel-port-how-fast-can-you-go/

#include <asm.h>
#include <custom_regdef.h>

# [a0] custom
ENTRY(KPutChar)
        ext.w   d0
        or.w    #256,d0

.Wait1: btst.b  #5,serdatr(a0)  /* check SERDATF_TBE */
        beq.s   .Wait1
        move.w  d0,serdat(a0)

        cmp.b   #10,d0          /* check if `\n` was sent */
        bne.s   .Quit

.Wait2: btst.b  #5,serdatr(a0)  /* check SERDATF_TBE */
        beq.s   .Wait2
        move.w  #269,serdat(a0) /* send '\r' */

.Quit:  rts
END(KPutChar)

# vim: ft=gas:ts=8:sw=8:noet:
