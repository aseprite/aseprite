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
 *      16 bit color polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *
 *      Original routines by Michael Bukin.
 *      Modified to support z-buffered polygon drawing by Bertrand Coconnier
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"

#ifdef ALLEGRO_COLOR16

#include "allegro/internal/aintern.h"
#include "cdefs16.h"

#define FUNC_POLY_ZBUF_FLAT			_poly_zbuf_flat16
#define FUNC_POLY_ZBUF_GRGB			_poly_zbuf_grgb16
#define FUNC_POLY_ZBUF_ATEX			_poly_zbuf_atex16
#define FUNC_POLY_ZBUF_ATEX_MASK		_poly_zbuf_atex_mask16
#define FUNC_POLY_ZBUF_ATEX_LIT			_poly_zbuf_atex_lit16
#define FUNC_POLY_ZBUF_ATEX_MASK_LIT		_poly_zbuf_atex_mask_lit16
#define FUNC_POLY_ZBUF_PTEX			_poly_zbuf_ptex16
#define FUNC_POLY_ZBUF_PTEX_MASK		_poly_zbuf_ptex_mask16
#define FUNC_POLY_ZBUF_PTEX_LIT			_poly_zbuf_ptex_lit16
#define FUNC_POLY_ZBUF_PTEX_MASK_LIT		_poly_zbuf_ptex_mask_lit16
#define FUNC_POLY_ZBUF_ATEX_TRANS		_poly_zbuf_atex_trans16
#define FUNC_POLY_ZBUF_ATEX_MASK_TRANS		_poly_zbuf_atex_mask_trans16
#define FUNC_POLY_ZBUF_PTEX_TRANS		_poly_zbuf_ptex_trans16
#define FUNC_POLY_ZBUF_PTEX_MASK_TRANS		_poly_zbuf_ptex_mask_trans16

#undef _bma_zbuf_gcol

#include "czscan.h"

#endif
