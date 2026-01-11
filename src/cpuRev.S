    INCDIR  "include:"
    INCLUDE "lvo/exec_lib.i"

    SECTION Code,CODE
    MACHINE 68060
	XDEF	_GetCPUReg

_GetCPUReg:
	movem.l	a5/a6,-(sp)
	lea.l	_readreg,a5
	move.l	$4.w,a6
	jsr	_LVOSupervisor(a6)
	movem.l	(sp)+,a5/a6
	rts

_readreg:
	movec	PCR,d0
	rte
    END
