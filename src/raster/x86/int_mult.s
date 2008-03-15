/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

.text

/*
 * Implementations of:
 *   #define INT_MULT(a, b, t) \
 *     ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))
 */

/* Function: int _int_mult(int a, int b) */
.globl __int_mult
	.def	__int_mult;	.scl	2;	.type	32;	.endef
	.align 4
__int_mult:
	/*
	 * b = a*b + 128;
	 * return ((b>>8) + b) >> 8;
	 */

	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax	/* eax = a */
	movl	12(%ebp), %edx	/* edx = b */
	popl	%ebp

	imull	%eax, %edx	/* b *= a */
	subl	$-128, %edx	/* b -= 128 */
	movl	%edx, %eax	/* a = b */
	sarl	$8, %eax	/* a >>= 8 */
	addl	%edx, %eax	/* a += b */
	sarl	$8, %eax	/* a >>= 8 */
	ret

/* Function: int _int_mult_fast(int %ecx, int %edx) */
.globl @_int_mult_fast@8
	.def	@_int_mult_fast@8;	.scl	2;	.type	32;	.endef
	.align 4
@_int_mult_fast@8:
	imull	%ecx, %edx	/* edx *= ecx */
	subl	$-128, %edx	/* edx -= 128 */
	movl	%edx, %eax	/* eax = edx */
	sarl	$8, %eax	/* eax >>= 8 */
	addl	%edx, %eax	/* eax += edx */
	sarl	$8, %eax	/* eax >>= 8 */
	ret
