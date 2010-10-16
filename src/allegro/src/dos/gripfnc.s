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
 *      Joystick driver for the Gravis GrIP.
 *      (Xterminator, GamepadPro, Stinger, ?)
 *
 *      By Robert J. Ragno, using the Gravis SDK and the other
 *      Allegro drivers as references.
 */


#include "../i386/asmdefs.inc"


#define GRIP_CALL             lcall *GLOBL(GRIP_Thunk) + 8


.extern _GRIP_Thunk

.text



FUNC(_GrInitialize)
	movw    $0x84A1,%ax
	GRIP_CALL
ret

FUNC(_GrShutdown)
	pushl   %eax
	movw    $0x84A2,%ax
	GRIP_CALL
	popl %eax
ret

FUNC(_GrRefresh)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %edx

	movl    8(%ebp), %edx

	movw    $0x84A0,%ax
	GRIP_CALL

	popl    %edx
	popl    %ebp
ret

FUNC(_GrGetSlotMap)
	movw    $0x84B0,%ax
	GRIP_CALL
ret

FUNC(_GrGetClassMap)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx

	movb    8(%ebp), %bh

	movw    $0x84B1,%ax
	GRIP_CALL

	popl    %ebx
	popl    %ebp
ret

FUNC(_GrGetOEMClassMap)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx

	movb    8(%ebp),%bh

	movw    $0x84B2,%ax
	GRIP_CALL

	popl    %ebx
	popl    %ebp
ret

FUNC(_GrGetMaxIndex)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx

	movb    8(%ebp),%bh
	movb    12(%ebp),%bl

	movw    $0x84B3,%ax
	GRIP_CALL

	popl    %ebx
	popl    %ebp
ret

FUNC(_GrGetMaxValue)
	pushl   %ebp
	movl    %esp,%ebp

	movb    8(%ebp),%bh
	movb    12(%ebp),%bl

	movw    $0x84B4,%ax
	GRIP_CALL

	popl    %ebp
ret

FUNC(_GrGetValue)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx
	pushl   %ecx

	movb    8(%ebp),%bh
	movb    12(%ebp),%bl
	movb    16(%ebp),%ch

	movw    $0x84C0,%ax
	GRIP_CALL

	popl    %ecx
	popl    %ebx
	popl    %ebp
ret

FUNC(_GrGetPackedValues)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx
	pushl   %ecx

	movb    8(%ebp),%bh
	movb    12(%ebp),%bl
	movb    16(%ebp),%ch
	movb    20(%ebp),%cl

	movw    $0x84C1,%ax
	GRIP_CALL

	popl    %ecx
	popl    %ebx
	popl    %ebp
ret

FUNC(_GrSetValue)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %eax
	pushl   %ebx
	pushl   %ecx
	pushl   %edx

	movb    8(%ebp),%bh
	movb    12(%ebp),%bl
	movb    16(%ebp),%ch
	movw    20(%ebp),%dx

	movw    $0x84C2,%ax
	GRIP_CALL

	popl    %edx
	popl    %ecx
	popl    %ebx
	popl    %eax
	popl    %ebp
ret

FUNC(_GrGetVendorName)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %esi
	pushl   %edi
	pushl   %eax
	pushl   %ebx
	pushl   %ecx
	pushl   %edx

	movb    8(%ebp),%bh
	movl    12(%ebp),%edi

	movw    $0x84D0,%ax
	GRIP_CALL

	movl    %ebx,0x00(%edi)
	movl    %ecx,0x04(%edi)
	movl    %edx,0x08(%edi)
	movl    %esi,0x0C(%edi)

	popl    %edx
	popl    %ecx
	popl    %ebx
	popl    %eax
	popl    %edi
	popl    %esi
	popl    %ebp
ret

FUNC(_GrGetProductName)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %esi
	pushl   %edi
	pushl   %ebx
	pushl   %ecx
	pushl   %edx

	movb    8(%ebp),%bh
	movl    12(%ebp),%edi

	movw    $0x84D1,%ax
	GRIP_CALL

	movl    %ebx,0x00(%edi)
	movl    %ecx,0x04(%edi)
	movl    %edx,0x08(%edi)
	movl    %esi,0x0C(%edi)

	popl    %edx
	popl    %ecx
	popl    %ebx
	popl    %edi
	popl    %esi
	popl    %ebp
ret

FUNC(_GrGetControlName)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %esi
	pushl   %edi
	pushl   %eax
	pushl   %ebx
	pushl   %ecx
	pushl   %edx

	movb    8(%ebp),%bh
	movb    12(%ebp),%bl
	movb    16(%ebp),%ch
	movl    20(%ebp),%edi

	movw    $0x84D2,%ax
	GRIP_CALL

	movl    %ebx,0x00(%edi)
	movl    %ecx,0x04(%edi)
	movl    %edx,0x08(%edi)
	movl    %esi,0x0C(%edi)

	popl    %edx
	popl    %ecx
	popl    %ebx
	popl    %eax
	popl    %edi
	popl    %esi
	popl    %ebp
ret

FUNC(_GrGetCaps)
   xorl         %eax, %eax
ret

FUNC(_Gr__Link)
	movw    $0x8490,%ax
	GRIP_CALL
ret

FUNC(_Gr__Unlink)
	pushl   %eax
	movw    $0x8491,%ax
	GRIP_CALL
	popl    %eax
ret

FUNC(_GrGetSWVer)
	movw    $0x84F0,%ax
	GRIP_CALL
ret

FUNC(_GrGetHWVer)
	movw    $0x84F1,%ax
	GRIP_CALL
ret

FUNC(_GrGetDiagCnt)
	movw    $0x84F2,%ax
	GRIP_CALL
ret

FUNC(_GrGetDiagReg)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx

	movb    8(%ebp),%bh

	movw    $0x84F3,%ax
	GRIP_CALL

	popl    %ebx
	popl    %ebp
ret

/*
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; These functions call the DPMI host to set access rights for the ;;
;; descriptor whose selector is sent as a parameter.               ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
*/
FUNC(_DPMI_SetCodeAR)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx
	pushl   %ecx
	pushf 

	movw    8(%ebp),%bx

	movw    %cs,%cx
	larl    %ecx,%ecx
	shrw    $8,%cx
	movl    $0x09,%eax
	int             $0x31
	setcb   %al

	popf 
	popl    %ecx
	popl    %ebx
	popl    %ebp
ret

FUNC(_DPMI_SetDataAR)
	pushl   %ebp
	movl    %esp,%ebp
	pushl   %ebx
	pushl   %ecx
	pushf 

	movw    8(%ebp),%bx

	movw    %ds,%cx
	larl    %ecx,%ecx
	shrw    $8,%cx
	movl    $0x09,%eax
	int             $0x31
	setcb   %al

	popf 
	popl    %ecx
	popl    %ebx
	popl    %ebp
ret

