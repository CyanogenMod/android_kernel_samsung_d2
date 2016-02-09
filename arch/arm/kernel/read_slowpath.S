#include <linux/linkage.h>
#include <asm/assembler.h>

.macro spinhead
.endm

.macro spintest loop
	ldr	r1, [r0]
	tst	r1, #0x80000000
	bne	\loop
.endm

.macro spintail
.endm

.macro spinskipoptim skip
.endm

ENTRY(__arch_read_lock_slowpath)

#include "lock_template.S"

ENDPROC(__arch_read_lock_slowpath)
