#include <asm.h>
#include <custom_regdef.h>

# [a0] custom
ENTRY(KPutChar)
        ext.w   d0
        or.w    #256,d0         /* add stop bit (?) */

.Wait1: btst.b  #5,serdatr(a0)  /* loop until transmit buffer empty */
        beq.s   .Wait1
        move.w  d0,serdat(a0)   /* send the character */

        cmp.b   #10,d0          /* check if it was `\n` */
        bne.s   .Quit

.Wait2: btst.b  #5,serdatr(a0)  /* loop until transmit buffer empty */
        beq.s   .Wait2
        move.w  #269,serdat(a0) /* send '\r' after `\n` */

.Quit:  rts
END(KPutChar)

# vim: ft=gas:ts=8:sw=8:noet:
