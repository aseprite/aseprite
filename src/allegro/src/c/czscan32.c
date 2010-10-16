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
 *      32 bit color polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *
 *      Original routines by Michael Bukin.
 *      Modified to support z-buffered polygon drawing by Bertrand Coconnier
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"

#ifdef ALLEGRO_COLOR32

#include "allegro/internal/aintern.h"
#include "cdefs32.h"

#define FUNC_POLY_ZBUF_FLAT			_poly_zbuf_flat32
#define FUNC_POLY_ZBUF_GRGB			_poly_zbuf_grgb32
#define FUNC_POLY_ZBUF_ATEX			_poly_zbuf_atex32
#define FUNC_POLY_ZBUF_ATEX_MASK		_poly_zbuf_atex_mask32
#define FUNC_POLY_ZBUF_ATEX_LIT			_poly_zbuf_atex_lit32
#define FUNC_POLY_ZBUF_ATEX_MASK_LIT		_poly_zbuf_atex_mask_lit32
#define FUNC_POLY_ZBUF_PTEX			_poly_zbuf_ptex32
#define FUNC_POLY_ZBUF_PTEX_MASK		_poly_zbuf_ptex_mask32
#define FUNC_POLY_ZBUF_PTEX_LIT			_poly_zbuf_ptex_lit32
#define FUNC_POLY_ZBUF_PTEX_MASK_LIT		_poly_zbuf_ptex_mask_lit32
#define FUNC_POLY_ZBUF_ATEX_TRANS		_poly_zbuf_atex_trans32
#define FUNC_POLY_ZBUF_ATEX_MASK_TRANS		_poly_zbuf_atex_mask_trans32
#define FUNC_POLY_ZBUF_PTEX_TRANS		_poly_zbuf_ptex_trans32
#define FUNC_POLY_ZBUF_PTEX_MASK_TRANS		_poly_zbuf_ptex_mask_trans32

#undef _bma_zbuf_gcol

#include "czscan.h"

#endif
