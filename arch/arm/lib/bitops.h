#include <asm/unwind.h>

#if __LINUX_ARM_ARCH__ >= 6
/* revb: branch with reversed logic (eq and ne only) */
	.macro	revb, cond, tgt
.ifc \cond,eq
	bne	\tgt
.else
.ifc \cond,ne
	beq	\tgt
.endif
.endif
	.endm

	.macro	bitop, name, instr, cond
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	bne	3f			@ assert word-aligned
	mov	r2, #1
	and	r3, r0, #31		@ Get bit offset
	mov	r0, r0, lsr #5
	add	r1, r1, r0, lsl #2	@ Get word offset
#if __LINUX_ARM_ARCH__ >= 7
	.arch_extension	mp
	ALT_SMP(W(pldw)	[r1])
	ALT_UP(W(nop))
#endif
	mov	r3, r2, lsl r3
.ifnc \cond,
	ldr	r2, [r1]
	tst	r2, r3
	revb	\cond, 2f
.endif
1:	ldrex	r2, [r1]
.ifnc \cond,
	tst	r2, r3
.endif
	\instr	r2, r2, r3
	revb	\cond, 2f
	strex	r0, r2, [r1]
	cmp	r0, #0
	bne	1b
2:	bx	lr
3:	strb	r1, [ip]
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm

	.macro	testop, name, instr, cond
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	bne	3f			@ assert word-aligned
	mov	r2, #1
	and	r3, r0, #31		@ Get bit offset
	mov	r0, r0, lsr #5
	add	r1, r1, r0, lsl #2	@ Get word offset
	mov	r3, r2, lsl r3		@ create mask
	smp_dmb
#if __LINUX_ARM_ARCH__ >= 7 && defined(CONFIG_SMP)
	.arch_extension	mp
	ALT_SMP(W(pldw)	[r1])
	ALT_UP(W(nop))
#endif
.ifnc \cond,
	ldr	r2, [r1]
	ands	r0, r2, r3
	revb	\cond, 2f
.endif
1:	ldrex	r2, [r1]
	ands	r0, r2, r3		@ save old value of bit
	\instr	r2, r2, r3		@ toggle bit
	revb	\cond, 2f
	strex	ip, r2, [r1]
	cmp	ip, #0
	bne	1b
	smp_dmb
2:	cmp	r0, #0
	movne	r0, #1
	bx	lr
3:	strb	r1, [ip]
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm
#else
	.macro	bitop, name, instr, cond
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	strneb	r1, [ip]		@ assert word-aligned
	and	r2, r0, #31
	mov	r0, r0, lsr #5
	mov	r3, #1
	mov	r3, r3, lsl r2
	save_and_disable_irqs ip
	ldr	r2, [r1, r0, lsl #2]
	\instr	r2, r2, r3
	str	r2, [r1, r0, lsl #2]
	restore_irqs ip
	mov	pc, lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm

/**
 * testop - implement a test_and_xxx_bit operation.
 * @instr: operational instruction
 * @store: store instruction
 *
 * Note: we can trivially conditionalise the store instruction
 * to avoid dirtying the data cache.
 */
	.macro	testop, name, instr, cond
ENTRY(	\name		)
UNWIND(	.fnstart	)
	ands	ip, r1, #3
	strneb	r1, [ip]		@ assert word-aligned
	and	r3, r0, #31
	mov	r0, r0, lsr #5
	save_and_disable_irqs ip
	ldr	r2, [r1, r0, lsl #2]!
	mov	r0, #1
	tst	r2, r0, lsl r3
	\instr	r2, r2, r0, lsl r3
	str\cond	r2, [r1]
	moveq	r0, #0
	restore_irqs ip
	mov	pc, lr
UNWIND(	.fnend		)
ENDPROC(\name		)
	.endm
#endif
