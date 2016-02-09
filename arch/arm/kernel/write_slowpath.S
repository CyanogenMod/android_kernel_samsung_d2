#include <linux/linkage.h>
#include <asm/assembler.h>

.macro spinhead
.endm

.macro spintest loop
	ldrex	r1, [r0]
	teq	r1, #0
	bne	\loop

	mov	r1, #0x80000000
	strex	r2, r1, [r0]
	teq	r2, #0
	bne	\loop
.endm

.macro spintail
.endm

.macro spinskipoptim skip
	ldr	r1, [r0]
	bics	r1, r1, #0x80000001
	bne	\skip
.endm

ENTRY(__arch_write_lock_slowpath)

#include "lock_template.S"

ENDPROC(__arch_write_lock_slowpath)
