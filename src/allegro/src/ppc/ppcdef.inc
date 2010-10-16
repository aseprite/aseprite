	MACRO
	MakeFunction &fnName
		EXPORT &fnName[DS]
 		EXPORT .&fnName[PR]
		
		TC &fnName[TC], &fnName[DS]
			
		CSECT &fnName[DS]
			DC.L .&fnName[PR]
 			DC.L TOC[tc0]
		
		CSECT .&fnName[PR]
		FUNCTION .&fnName[PR]	
	ENDM

ARG1	EQU	r3
ARG2	EQU	r4
ARG3	EQU	r5
ARG4	EQU	r6
ARG5	EQU	r7
ARG6	EQU	r8
ARG7	EQU	r9
ARG8	EQU	r10
TMP1	EQU	r11
TMP2	EQU	r12

LOCAL1	EQU	r31
LOCAL2	EQU	r30
LOCAL3	EQU	r29
LOCAL4	EQU	r28
LOCAL5	EQU	r27
LOCAL6	EQU	r26
LOCAL7	EQU	r25
LOCAL8	EQU	r24
LOCAL10	EQU	r23
LOCAL11	EQU	r22
LOCAL12	EQU	r21
LOCAL13	EQU	r20
LOCAL14	EQU	r19
LOCAL15	EQU	r18
LOCAL16	EQU	r17

RESULT	EQU	r3

;typedef struct BITMAP            /* a bitmap structure */
;{
;   int w, h;                     /* width and height in pixels */
;   int clip;                     /* flag if clipping is turned on */
;   int cl, cr, ct, cb;           /* clip left, right, top and bottom values */
;   GFX_VTABLE *vtable;           /* drawing functions */
;   void *write_bank;             /* C func on some machines, asm on i386 */
;   void *read_bank;              /* C func on some machines, asm on i386 */
;   void *dat;                    /* the memory we allocated for the bitmap */
;   unsigned long id;             /* for identifying sub-bitmaps */
;   void *extra;                  /* points to a structure with more info */
;   int x_ofs;                    /* horizontal offset (for sub-bitmaps) */
;   int y_ofs;                    /* vertical offset (for sub-bitmaps) */
;   int seg;                      /* bitmap segment */
;   ZERO_SIZE_ARRAY(unsigned char *,line);
;} BITMAP;

BITMAP     record 
w          ds.l 1
h          ds.l 1
clip       ds.l 1
cl         ds.l 1
cr         ds.l 1
ct         ds.l 1
cb         ds.l 1
vtable     ds.l 1
write_bank ds.l 1
read_bank  ds.l 1
dat        ds.l 1
id         ds.l 1
extra      ds.l 1
x_ofs      ds.l 1
y_ofs      ds.l 1
_seg       ds.l 1
sizeof     EQU *
_line      ds.l 1
_line1     ds.l 1
           endr

DRAW_MODE_SOLID          set   0
DRAW_MODE_XOR            set   1
DRAW_MODE_COPY_PATTERN   set   2
DRAW_MODE_SOLID_PATTERN  set   3
DRAW_MODE_MASKED_PATTERN set   4
DRAW_MODE_TRANS          set   5
DRAW_MODE_MAX_SUPORTED   set   5

	toc
	tc	_linear_vline8[TC],_linear_vline8[DS]
	import	._linear_vline8[PR]
	import	_linear_vline8[DS]
	toc
	tc	_linear_hline8[TC],_linear_hline8[DS]
	import	._linear_hline8[PR]
	import	_linear_hline8[DS]
	toc
	tc	_normal_rectfill[TC],_normal_rectfill[DS]
	import	._normal_rectfill[PR]
	import	_normal_rectfill[DS]
	toc
	tc	_linear_putpixel8[TC],_linear_putpixel8[DS]
	import	._linear_putpixel8[PR] 
	import	_linear_putpixel8[DS] 

	toc
	tc	_drawing_mode[TC],_drawing_mode[RW]
	toc
	tc	color_map[TC],color_map[RW]
	toc
	tc	_drawing_x_mask[TC],_drawing_x_mask[RW]
	toc
	tc	_drawing_x_anchor[TC],_drawing_x_anchor[RW]
	toc
	tc	_drawing_y_mask[TC],_drawing_y_mask[RW]
	toc
	tc	_drawing_y_anchor[TC],_drawing_y_anchor[RW]
	toc
	tc	_drawing_pattern[TC],_drawing_pattern[RW]
	import	_drawing_mode[RW]
	import	color_map[RW]
	import	_drawing_x_mask[RW]
	import	_drawing_x_anchor[RW]
	import	_drawing_y_mask[RW]
	import	_drawing_y_anchor[RW]
	import	_drawing_pattern[RW]
