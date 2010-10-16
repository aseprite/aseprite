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
 *      Defines for 15 bit color graphics primitives.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __bma_cdefs15_h
#define __bma_cdefs15_h

#define PP_DEPTH               15

#define PIXEL_PTR              unsigned short*
#define OFFSET_PIXEL_PTR(p,x)  ((PIXEL_PTR) (p) + (x))
#define INC_PIXEL_PTR(p)       ((p)++)
#define INC_PIXEL_PTR_N(p,d)   ((p) += d)
#define DEC_PIXEL_PTR(p)       ((p)--)

#define PUT_PIXEL(p,c)         bmp_write15((uintptr_t) (p), (c))
#define PUT_MEMORY_PIXEL(p,c)  (*(p) = (c))
#define PUT_RGB(p,r,g,b)       bmp_write15((uintptr_t) (p), makecol15((r), (g), (b)))
#define GET_PIXEL(p)           bmp_read15((uintptr_t) (p))
#define GET_MEMORY_PIXEL(p)    (*(p))

#define IS_MASK(c)             ((unsigned long) (c) == MASK_COLOR_15)
#define IS_SPRITE_MASK(b,c)    ((unsigned long) (c) == MASK_COLOR_15)

/* Blender for putpixel (DRAW_MODE_TRANS).  */
#define PP_BLENDER             BLENDER_FUNC
#define MAKE_PP_BLENDER(c)     _blender_func15
#define PP_BLEND(b,o,n)        ((*(b))((n), (o), _blender_alpha))

/* Blender for draw_trans_*_sprite.  */
#define DTS_BLENDER            BLENDER_FUNC
#define MAKE_DTS_BLENDER()     _blender_func15
#define DTS_BLEND(b,o,n)       ((*(b))((n), (o), _blender_alpha))

/* Blender for draw_lit_*_sprite.  */
#define DLS_BLENDER            BLENDER_FUNC
#define MAKE_DLS_BLENDER(a)    _blender_func15
#define DLS_BLEND(b,a,n)       ((*(b))(_blender_col_15, (n), (a)))
#define DLSX_BLEND(b,n)        ((*(b))(_blender_col_15, (n), _blender_alpha))

/* Blender for RGBA sprites.  */
#define RGBA_BLENDER           BLENDER_FUNC
#define MAKE_RGBA_BLENDER()    _blender_func15x
#define RGBA_BLEND(b,o,n)      ((*(b))((n), (o), _blender_alpha))

/* Blender for poly_scanline_*_lit.  */
#define PS_BLENDER             BLENDER_FUNC
#define MAKE_PS_BLENDER()      _blender_func15
#define PS_BLEND(b,o,c)        ((*(b))((c), _blender_col_15, (o)))
#define PS_ALPHA_BLEND(b,o,c)  ((*(b))((o), (c), _blender_alpha))

#define PATTERN_LINE(y)        (PIXEL_PTR) (_drawing_pattern->line[((y) - _drawing_y_anchor) \
								   & _drawing_y_mask])
#define GET_PATTERN_PIXEL(x,y) GET_MEMORY_PIXEL(OFFSET_PIXEL_PTR(PATTERN_LINE(y), \
                                                ((x) - _drawing_x_anchor) & _drawing_x_mask))

#define RLE_PTR                signed short*
#define RLE_IS_EOL(c)          ((unsigned short) (c) == MASK_COLOR_15)

#define FUNC_LINEAR_CLEAR_TO_COLOR          _linear_clear_to_color15
#define FUNC_LINEAR_BLIT                    _linear_blit15
#define FUNC_LINEAR_BLIT_BACKWARD           _linear_blit_backward15
#define FUNC_LINEAR_MASKED_BLIT             _linear_masked_blit15

#define FUNC_LINEAR_PUTPIXEL                _linear_putpixel15
#define FUNC_LINEAR_GETPIXEL                _linear_getpixel15
#define FUNC_LINEAR_HLINE                   _linear_hline15
#define FUNC_LINEAR_VLINE                   _linear_vline15

#define FUNC_LINEAR_DRAW_SPRITE             _linear_draw_sprite15
#define FUNC_LINEAR_DRAW_SPRITE_EX          _linear_draw_sprite_ex15
#define FUNC_LINEAR_DRAW_256_SPRITE         _linear_draw_256_sprite15
#define FUNC_LINEAR_DRAW_SPRITE_V_FLIP      _linear_draw_sprite_v_flip15
#define FUNC_LINEAR_DRAW_SPRITE_H_FLIP      _linear_draw_sprite_h_flip15
#define FUNC_LINEAR_DRAW_SPRITE_VH_FLIP     _linear_draw_sprite_vh_flip15
#define FUNC_LINEAR_DRAW_TRANS_SPRITE       _linear_draw_trans_sprite15
#define FUNC_LINEAR_DRAW_TRANS_RGBA_SPRITE  _linear_draw_trans_rgba_sprite15
#define FUNC_LINEAR_DRAW_LIT_SPRITE         _linear_draw_lit_sprite15
#define FUNC_LINEAR_DRAW_CHARACTER          _linear_draw_character15
#define FUNC_LINEAR_DRAW_RLE_SPRITE         _linear_draw_rle_sprite15
#define FUNC_LINEAR_DRAW_TRANS_RLE_SPRITE   _linear_draw_trans_rle_sprite15
#define FUNC_LINEAR_DRAW_TRANS_RGBA_RLE_SPRITE _linear_draw_trans_rgba_rle_sprite15
#define FUNC_LINEAR_DRAW_LIT_RLE_SPRITE     _linear_draw_lit_rle_sprite15

#define FUNC_LINEAR_DRAW_SPRITE_END         _linear_draw_sprite15_end
#define FUNC_LINEAR_BLIT_END                _linear_blit15_end

#define FUNC_POLY_SCANLINE_GRGB             _poly_scanline_grgb15
#define FUNC_POLY_SCANLINE_ATEX             _poly_scanline_atex15
#define FUNC_POLY_SCANLINE_ATEX_MASK        _poly_scanline_atex_mask15
#define FUNC_POLY_SCANLINE_ATEX_LIT         _poly_scanline_atex_lit15
#define FUNC_POLY_SCANLINE_ATEX_MASK_LIT    _poly_scanline_atex_mask_lit15
#define FUNC_POLY_SCANLINE_PTEX             _poly_scanline_ptex15
#define FUNC_POLY_SCANLINE_PTEX_MASK        _poly_scanline_ptex_mask15
#define FUNC_POLY_SCANLINE_PTEX_LIT         _poly_scanline_ptex_lit15
#define FUNC_POLY_SCANLINE_PTEX_MASK_LIT    _poly_scanline_ptex_mask_lit15
#define FUNC_POLY_SCANLINE_ATEX_TRANS       _poly_scanline_atex_trans15
#define FUNC_POLY_SCANLINE_ATEX_MASK_TRANS  _poly_scanline_atex_mask_trans15
#define FUNC_POLY_SCANLINE_PTEX_TRANS       _poly_scanline_ptex_trans15
#define FUNC_POLY_SCANLINE_PTEX_MASK_TRANS  _poly_scanline_ptex_mask_trans15

#endif /* !__bma_cdefs15_h */

