/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Preliminary driver for Microsoft Sidewinder 3D/Precision/
 *      Force Feedback Pro Joysticks.
 *
 *      By Acho A. Tang.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"


#define PARAM1 40(%esp)
#define PARAM2 44(%esp)
#define PARAM3 48(%esp)
#define PARAM4 52(%esp)
#define PARAM5 56(%esp)
#define PARAM6 60(%esp)
#define PARAM7 64(%esp)
#define PARAM8 68(%esp)


.text


/* void _swpp_get_portspeed(unsigned int port, unsigned int npasses, unsigned int *nqueries, unsigned int *elapsed) */
FUNC(_swpp_get_portspeed)
	pushfl
	pushal
	movl   PARAM1,%edx       // edx <- port
	movl   PARAM2,%eax       // eax <- npasses
	movl   PARAM3,%esi       // esi <- *nqueries
	movl   PARAM4,%edi       // edi <- *elapsed
	xorl   %ebp,%ebp
	movb   %al,%ah
	cli

	movb   $0xb4,%al         // select ch.2 to mode 2
	outb   %al,$0x43
	xorb   %al,%al           // reset divisor and start counting
	outb   %al,$0x42
	outb   %al,$0x42

	.p2align 2
1:
	movl   (%esi,%ebp),%ecx  // load innerloop counter

	movb   $0x84,%al         // latch and read start timer value
	outb   %al,$0x43
	inb    $0x42,%al
	movb   %al,%bl
	inb    $0x42,%al
	movb   %al,%bh

	.p2align 2
0:
	inb    %dx,%al           // query game port
	decl   %ecx
	jnz    0b

	movb   $0x84,%al         // latch and read end timer value
	outb   %al,$0x43
	inb    $0x42,%al
	movb   %al,%cl
	inb    $0x42,%al
	movb   %al,%ch

	subw   %cx,%bx           // store number of ticks elapsed
	movswl %bx,%ecx
	movl   %ecx,(%edi,%ebp)
	addl   $4,%ebp
	decb   %ah
	jnz    1b

	sti
	popal
	popfl
ret


/* void _swpp_init_digital(unsigned int port, unsigned int ncmds, unsigned char *cmdiv, unsigned int *timeout) */
FUNC(_swpp_init_digital)
	pushfl
	pushal
	movl   PARAM1,%edx       // edx <- port addr
	movl   PARAM2,%eax       // eax <- ncmds
	movl   PARAM3,%esi       // esi <- *cmdiv
	movl   PARAM4,%edi       // ecx <- *timeout
	movl   (%edi),%ecx
	movb   %al,%ah
	cli

	movb   $0xb0,%al         // set ch.2 to mode 0
	outb   %al,$0x43
	outb   %al,%dx           // trigger game port

	// wait for axis 0 to fall, time-out if necessary
	.p2align 2
0:                          // outter looptop
	inb    %dx,%al
	decl   %ecx
	jz 2f
	testb  $1,%al
	jnz 0b

	// load counter 2 and start counting
	movb   (%esi),%al
	outb   %al,$0x42
	movb   1(%esi),%al
	addl   $2,%esi
	outb   %al,$0x42

	// wait for terminal state(output pin going high) *** NO TIME-OUT ***
	.p2align 2
1:
	movb   $0xe8,%al         // set ch.2 to read-back mode
	outb   %al,$0x43
	inb    $0x42,%al         // read output pin status
	testb  $0x80,%al
	jz 1b

	// send the next/last command
	outb   %al,%dx
	decb   %ah
	jnz 0b

2:
	sti
	movl %ecx,(%edi)         // save time-out status
	popal
	popfl
ret


/* void _swpp_read_analogue(unsigned int port, unsigned int *outbuf, unsigned int pollout) */
FUNC(_swpp_read_analogue)
	pushfl
	pushal
	movl   PARAM1,%edx
	movl   PARAM3,%ecx
	xorl   %eax,%eax
	xorl   %ebx,%ebx
	xorl   %ebp,%ebp
	xorl   %esi,%esi
	xorl   %edi,%edi
	cli

	outb   %al,%dx

	.p2align 4
0:
	inb    %dx,%al
	rorb   $1,%al
	adcl   $0,%ebx
	rorb   $1,%al
	adcl   $0,%ebp
	rorb   $1,%al
	adcl   $0,%esi
	rorb   $1,%al
	adcl   $0,%edi
	testb  $0xf0,%al
	jz 1f
	decl   %ecx
	jnz 0b

1:
	sti
	movl   PARAM2,%edx
	movzbl %al,%eax
	movl   %eax,(%edx)
	movl   %ebx,4(%edx)
	movl   %ebp,8(%edx)
	movl   %esi,12(%edx)
	movl   %edi,16(%edx)
	movl   %ecx,20(%edx)
	popal
	popfl
ret
