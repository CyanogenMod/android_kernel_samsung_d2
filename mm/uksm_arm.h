#ifndef _UKSM_ARM_H
#define _UKSM_ARM_H

#include <asm/page.h>

#undef memcmp
#define memcmp uksm_memcmp

/* uksm_memcmp:
 * 461 MB/s @ 384 MHz
 * 1584 MB/s @ 1512 MHz
 */
static inline int uksm_memcmp(const void *s1, const void *s2, size_t n)
{
	register uint64_t r1, r2, r3;
	register uint32_t ret = 1;
	asm volatile (
	"	ldrd	%3, %H3, [%0], #8\n"
	"	ldrd	%4, %H4, [%1], #8\n"
	"	pld	[%0, #56]\n"
	"	pld	[%1, #56]\n"
	"	pld	[%0, #120]\n"
	"	pld	[%1, #120]\n"

	"1:	pld	[%0, #184]\n"
	"	pld	[%1, #184]\n"
	"	teq	%3, %4\n"
	"	ldrd	%5, %H5, [%1], #8\n"
	"	teqeq	%H3, %H4\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	bne	5f\n"
	"	teq	%3, %5\n"
	"	ldrd	%4, %H4, [%1], #8\n"
	"	teqeq	%H3, %H5\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	bne	5f\n"
	"	teq	%3, %4\n"
	"	ldrd	%5, %H5, [%1], #8\n"
	"	teqeq	%H3, %H4\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	bne	5f\n"
	"	teq	%3, %5\n"
	"	ldrd	%4, %H4, [%1], #8\n"
	"	teqeq	%H3, %H5\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	bne	5f\n"
	"	teq	%3, %4\n"
	"	ldrd	%5, %H5, [%1], #8\n"
	"	teqeq	%H3, %H4\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	bne	5f\n"
	"	teq	%3, %5\n"
	"	ldrd	%4, %H4, [%1], #8\n"
	"	teqeq	%H3, %H5\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	bne	5f\n"
	"	teq	%3, %4\n"
	"	ldrd	%5, %H5, [%1], #8\n"
	"	teqeq	%H3, %H4\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	bne	5f\n"
	"	teq	%3, %5\n"
	"	teqeq	%H3, %H5\n"
	"	bne	5f\n"

	"	tst	%0, #4064\n"
	"	beq	4f\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	ldrd	%4, %H4, [%1], #8\n"
	"	b	1b\n"

	"4:	mov	%2, #0\n"
	"5:"
	: "+Qr" (s1), "+Qr" (s2), "+r" (ret),
	  "=&r" (r1), "=&r" (r2), "=&r" (r3)
	: : "cc");

	return ret;
}

/* is_full_zero:
 * 750 MB/s @ 384 MHz
 * 2543 MB/s @ 1512 MHz
 */
static inline int is_full_zero(void *s1, size_t n)
{
	register uint64_t r1, r2;
	register uint32_t ret = 0;
	asm volatile (
	"	ldrd	%2, %H2, [%0], #8\n"
	"	pld	[%0, #56]\n"
	"	pld	[%0, #120]\n"
	"	pld	[%0, #184]\n"
	"	pld	[%0, #248]\n"
	"	pld	[%0, #312]\n"
	"	pld	[%0, #376]\n"

	"1:	pld	[%0, #440]\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	orrs	%2, %2, %H2\n"
	"	bne	5f\n"
	"	ldrd	%2, %H2, [%0], #8\n"
	"	orrs	%3, %3, %H3\n"
	"	bne	5f\n"
	"	ldrd	%3, %H3, [%0], #8\n"
	"	orrs	%2, %2, %H2\n"
	"	bne	5f\n"
	"	orrs	%3, %3, %H3\n"
	"	bne	5f\n"

	"	tst	%0, #4064\n"
	"	beq	4f\n"
	"	ldrd	%2, %H2, [%0], #8\n"
	"	b	1b\n"

	"4:	mov	%1, #1\n"
	"5:"
	: "+Qr" (s1), "+r" (ret),
	  "=&r" (r1), "=&r" (r2)
	: : "cc");

	return ret;
}

#endif
