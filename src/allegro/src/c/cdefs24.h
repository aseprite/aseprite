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
 *      Defines for 24 bit color graphics primitives.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __bma_cdefs24_h
#define __bma_cdefs24_h

#define PP_DEPTH               24

#define PIXEL_PTR              unsigned char*
#define PTR_PER_PIXEL          3
#define OFFSET_PIXEL_PTR(p,x)  ((PIXEL_PTR) (p) + 3 * (x))
#define INC_PIXEL_PTR(p)       ((p) += 3)
#define INC_PIXEL_PTR_N(p,d)   ((p) += 3 * d)
#define DEC_PIXEL_PTR(p)       ((p) -= 3)

#define PUT_PIXEL(p,c)         bmp_write24((uintptr_t) (p), (c))
#define PUT_MEMORY_PIXEL(p,c)  WRITE3BYTES((p), (c))
#define PUT_RGB(p,r,g,b)       bmp_write24((uintptr_t) (p), makecol24((r), (g), (b)))
#define GET_PIXEL(p)           bmp_read24((uintptr_t) (p))
#define GET_MEMORY_PIXEL(p)    READ3BYTES((p))
#define IS_MASK(c)             ((unsigned long) (c) == MASK_COLOR_24)
#define IS_SPRITE_MASK(b,c)    ((unsigned long) (c) == MASK_COLOR_24)

/* Blender for putpixel (DRAW_MODE_TRANS).  */
#define PP_BLENDER             BLENDER_FUNC
#define MAKE_PP_BLENDER(c)     _blender_func24
#define PP_BLEND(b,o,n)        ((*(b))((n), (o), _blender_alpha))

/* Blender for draw_trans_*_sprite.  */
#define DTS_BLENDER            BLENDER_FUNC
#define MAKE_DTS_BLENDER()     _blender_func24
#define DTS_BLEND(b,o,n)       ((*(b))((n), (o), _blender_alpha))

/* Blender for draw_lit_*_sprite.  */
#define DLS_BLENDER            BLENDER_FUNC
#define MAKE_DLS_BLENDER(a)    _blender_func24
#define DLS_BLEND(b,a,n)       ((*(b))(_blender_col_24, (n), (a)))
#define DLSX_BLEND(b,n)        ((*(b))(_blender_col_24, (n), _blender_alpha))

/* Blender for RGBA sprites.  */
#define RGBA_BLENDER           BLENDER_FUNC
#define MAKE_RGBA_BLENDER()    _blender_func24x
#define RGBA_BLEND(b,o,n)      ((*(b))((n), (o), _blender_alpha))

/* Blender for poly_scanline_*_lit.  */
#define PS_BLENDER             BLENDER_FUNC
#define MAKE_PS_BLENDER()      _blender_func24
#define PS_BLEND(b,o,c)        ((*(b))((c), _blender_col_24, (o)))
#define PS_ALPHA_BLEND(b,o,c)  ((*(b))((o), (c), _blender_alpha))

#define PATTERN_LINE(y)        (PIXEL_PTR) (_drawing_pattern->line[((y) - _drawing_y_anchor) \
								   & _drawing_y_mask])
#define GET_PATTERN_PIXEL(x,y) GET_MEMORY_PIXEL(OFFSET_PIXEL_PTR(PATTERN_LINE(y), \
                                                ((x) - _drawing_x_anchor) & _drawing_x_mask))

#define RLE_PTR                int32_t*
#define RLE_IS_EOL(c)          ((unsigned long) (c) == MASK_COLOR_24)

#define FUNC_LINEAR_CLEAR_TO_COLOR          _linear_clear_to_color24
#define FUNC_LINEAR_BLIT                    _linear_blit24
#define FUNC_LINEAR_BLIT_BACKWARD           _linear_blit_backward24
#define FUNC_LINEAR_MASKED_BLIT             _linear_masked_blit24

#define FUNC_LINEAR_PUTPIXEL                _linear_putpixel24
#define FUNC_LINEAR_GETPIXEL                _linear_getpixel24
#define FUNC_LINEAR_HLINE                   _linear_hline24
#define FUNC_LINEAR_VLINE                   _linear_vline24

#define FUNC_LINEAR_DRAW_SPRITE             _linear_draw_sprite24
#define FUNC_LINEAR_DRAW_SPRITE_EX          _linear_draw_sprite_ex24
#define FUNC_LINEAR_DRAW_256_SPRITE         _linear_draw_256_sprite24
#define FUNC_LINEAR_DRAW_SPRITE_V_FLIP      _linear_draw_sprite_v_flip24
#define FUNC_LINEAR_DRAW_SPRITE_H_FLIP      _linear_draw_sprite_h_flip24
#define FUNC_LINEAR_DRAW_SPRITE_VH_FLIP     _linear_draw_sprite_vh_flip24
#define FUNC_LINEAR_DRAW_TRANS_SPRITE       _linear_draw_trans_sprite24
#define FUNC_LINEAR_DRAW_TRANS_RGBA_SPRITE  _linear_draw_trans_rgba_sprite24
#define FUNC_LINEAR_DRAW_LIT_SPRITE         _linear_draw_lit_sprite24
#define FUNC_LINEAR_DRAW_CHARACTER          _linear_draw_character24
#define FUNC_LINEAR_DRAW_RLE_SPRITE         _linear_draw_rle_sprite24
#define FUNC_LINEAR_DRAW_TRANS_RLE_SPRITE   _linear_draw_trans_rle_sprite24
#define FUNC_LINEAR_DRAW_TRANS_RGBA_RLE_SPRITE _linear_draw_trans_rgba_rle_sprite24
#define FUNC_LINEAR_DRAW_LIT_RLE_SPRITE     _linear_draw_lit_rle_sprite24

#define FUNC_LINEAR_DRAW_SPRITE_END         _linear_draw_sprite24_end
#define FUNC_LINEAR_BLIT_END                _linear_blit24_end

#define FUNC_POLY_SCANLINE_GRGB             _poly_scanline_grgb24
#define FUNC_POLY_SCANLINE_ATEX             _poly_scanline_atex24
#define FUNC_POLY_SCANLINE_ATEX_MASK        _poly_scanline_atex_mask24
#define FUNC_POLY_SCANLINE_ATEX_LIT         _poly_scanline_atex_lit24
#define FUNC_POLY_SCANLINE_ATEX_MASK_LIT    _poly_scanline_atex_mask_lit24
#define FUNC_POLY_SCANLINE_PTEX             _poly_scanline_ptex24
#define FUNC_POLY_SCANLINE_PTEX_MASK        _poly_scanline_ptex_mask24
#define FUNC_POLY_SCANLINE_PTEX_LIT         _poly_scanline_ptex_lit24
#define FUNC_POLY_SCANLINE_PTEX_MASK_LIT    _poly_scanline_ptex_mask_lit24
#define FUNC_POLY_SCANLINE_ATEX_TRANS       _poly_scanline_atex_trans24
#define FUNC_POLY_SCANLINE_ATEX_MASK_TRANS  _poly_scanline_atex_mask_trans24
#define FUNC_POLY_SCANLINE_PTEX_TRANS       _poly_scanline_ptex_trans24
#define FUNC_POLY_SCANLINE_PTEX_MASK_TRANS  _poly_scanline_ptex_mask_trans24

#endif /* !__bma_cdefs24_h */

