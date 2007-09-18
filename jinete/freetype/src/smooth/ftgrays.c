/***************************************************************************/
/*                                                                         */
/*  ftgrays.c                                                              */
/*                                                                         */
/*    A new `perfect' anti-aliasing renderer (body).                       */
/*                                                                         */
/*  Copyright 2000-2001 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /*  This file can be compiled without the rest of the FreeType engine,   */
  /*  by defining the _STANDALONE_ macro when compiling it.  You also need */
  /*  to put the files `ftgrays.h' and `ftimage.h' into the current        */
  /*  compilation directory.  Typically, you could do something like       */
  /*                                                                       */
  /*  - copy `src/base/ftgrays.c' to your current directory                */
  /*                                                                       */
  /*  - copy `include/freetype/ftimage.h' and                              */
  /*    `include/freetype/ftgrays.h' to the same directory                 */
  /*                                                                       */
  /*  - compile `ftgrays' with the _STANDALONE_ macro defined, as in       */
  /*                                                                       */
  /*      cc -c -D_STANDALONE_ ftgrays.c                                   */
  /*                                                                       */
  /*  The renderer can be initialized with a call to                       */
  /*  `ft_gray_raster.gray_raster_new'; an anti-aliased bitmap can be      */
  /*  generated with a call to `ft_gray_raster.gray_raster_render'.        */
  /*                                                                       */
  /*  See the comments and documentation in the file `ftimage.h' for       */
  /*  more details on how the raster works.                                */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /*  This is a new anti-aliasing scan-converter for FreeType 2.  The      */
  /*  algorithm used here is _very_ different from the one in the standard */
  /*  `ftraster' module.  Actually, `ftgrays' computes the _exact_         */
  /*  coverage of the outline on each pixel cell.                          */
  /*                                                                       */
  /*  It is based on ideas that I initially found in Raph Levien's         */
  /*  excellent LibArt graphics library (see http://www.levien.com/libart  */
  /*  for more information, though the web pages do not tell anything      */
  /*  about the renderer; you'll have to dive into the source code to      */
  /*  understand how it works).                                            */
  /*                                                                       */
  /*  Note, however, that this is a _very_ different implementation        */
  /*  compared to Raph's.  Coverage information is stored in a very        */
  /*  different way, and I don't use sorted vector paths.  Also, it        */
  /*  doesn't use floating point values.                                   */
  /*                                                                       */
  /*  This renderer has the following advantages:                          */
  /*                                                                       */
  /*  - It doesn't need an intermediate bitmap.  Instead, one can supply   */
  /*    a callback function that will be called by the renderer to draw    */
  /*    gray spans on any target surface.  You can thus do direct          */
  /*    composition on any kind of bitmap, provided that you give the      */
  /*    renderer the right callback.                                       */
  /*                                                                       */
  /*  - A perfect anti-aliaser, i.e., it computes the _exact_ coverage on  */
  /*    each pixel cell                                                    */
  /*                                                                       */
  /*  - It performs a single pass on the outline (the `standard' FT2       */
  /*    renderer makes two passes).                                        */
  /*                                                                       */
  /*  - It can easily be modified to render to _any_ number of gray levels */
  /*    cheaply.                                                           */
  /*                                                                       */
  /*  - For small (< 20) pixel sizes, it is faster than the standard       */
  /*    renderer.                                                          */
  /*                                                                       */
  /*************************************************************************/


#include <string.h>             /* for memcpy() */
#include <setjmp.h>


/* experimental support for gamma correction within the rasterizer */
#define xxxGRAYS_USE_GAMMA


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_aaraster


#define ErrRaster_MemoryOverflow   -4

#ifdef _STANDALONE_


#define ErrRaster_Invalid_Mode     -2
#define ErrRaster_Invalid_Outline  -1

#include "ftimage.h"
#include "ftgrays.h"

  /* This macro is used to indicate that a function parameter is unused. */
  /* Its purpose is simply to reduce compiler warnings.  Note also that  */
  /* simply defining it as `(void)x' doesn't avoid warnings with certain */
  /* ANSI compilers (e.g. LCC).                                          */
#define FT_UNUSED( x )  (x) = (x)

  /* Disable the tracing mechanism for simplicity -- developers can      */
  /* activate it easily by redefining these two macros.                  */
#ifndef FT_ERROR
#define FT_ERROR( x )  do ; while ( 0 )     /* nothing */
#endif

#ifndef FT_TRACE
#define FT_TRACE( x )  do ; while ( 0 )     /* nothing */
#endif


#else /* _STANDALONE_ */


#include <ft2build.h>
#include "ftgrays.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_OUTLINE_H

#include "ftsmerrs.h"

#define ErrRaster_Invalid_Mode     Smooth_Err_Cannot_Render_Glyph
#define ErrRaster_Invalid_Outline  Smooth_Err_Invalid_Outline


#endif /* _STANDALONE_ */


#ifndef MEM_Set
#define  MEM_Set( d, s, c )  memset( d, s, c )
#endif

  /* define this to dump debugging information */
#define xxxDEBUG_GRAYS

  /* as usual, for the speed hungry :-) */

#ifndef FT_STATIC_RASTER


#define RAS_ARG   PRaster  raster
#define RAS_ARG_  PRaster  raster,

#define RAS_VAR   raster
#define RAS_VAR_  raster,

#define ras       (*raster)


#else /* FT_STATIC_RASTER */


#define RAS_ARG   /* empty */
#define RAS_ARG_  /* empty */
#define RAS_VAR   /* empty */
#define RAS_VAR_  /* empty */

  static TRaster  ras;


#endif /* FT_STATIC_RASTER */


  /* must be at least 6 bits! */
#define PIXEL_BITS  8

#define ONE_PIXEL       ( 1L << PIXEL_BITS )
#define PIXEL_MASK      ( -1L << PIXEL_BITS )
#define TRUNC( x )      ( (x) >> PIXEL_BITS )
#define SUBPIXELS( x )  ( (x) << PIXEL_BITS )
#define FLOOR( x )      ( (x) & -ONE_PIXEL )
#define CEILING( x )    ( ( (x) + ONE_PIXEL - 1 ) & -ONE_PIXEL )
#define ROUND( x )      ( ( (x) + ONE_PIXEL / 2 ) & -ONE_PIXEL )

#if PIXEL_BITS >= 6
#define UPSCALE( x )    ( (x) << ( PIXEL_BITS - 6 ) )
#define DOWNSCALE( x )  ( (x) >> ( PIXEL_BITS - 6 ) )
#else
#define UPSCALE( x )    ( (x) >> ( 6 - PIXEL_BITS ) )
#define DOWNSCALE( x )  ( (x) << ( 6 - PIXEL_BITS ) )
#endif

  /* Define this if you want to use a more compact storage scheme.  This   */
  /* increases the number of cells available in the render pool but slows  */
  /* down the rendering a bit.  It is useful if you have a really tiny     */
  /* render pool.                                                          */
#define xxxGRAYS_COMPACT


  /*************************************************************************/
  /*                                                                       */
  /*   TYPE DEFINITIONS                                                    */
  /*                                                                       */

  /* don't change the following types to FT_Int or FT_Pos, since we might */
  /* need to define them to "float" or "double" when experimenting with   */
  /* new algorithms                                                       */

  typedef int   TScan;   /* integer scanline/pixel coordinate */
  typedef long  TPos;    /* sub-pixel coordinate              */

  /* determine the type used to store cell areas.  This normally takes at */
  /* least PIXEL_BYTES*2 + 1.  On 16-bit systems, we need to use `long'   */
  /* instead of `int', otherwise bad things happen                        */

#if PIXEL_BITS <= 7

  typedef int   TArea;

#else /* PIXEL_BITS >= 8 */

  /* approximately determine the size of integers using an ANSI-C header */
#include <limits.h>

#if UINT_MAX == 0xFFFFU
  typedef long  TArea;
#else
  typedef int  TArea;
#endif

#endif /* PIXEL_BITS >= 8 */


  /* maximal number of gray spans in a call to the span callback */
#define FT_MAX_GRAY_SPANS  32


#ifdef GRAYS_COMPACT

  typedef struct  TCell_
  {
    short  x     : 14;
    short  y     : 14;
    int    cover : PIXEL_BITS + 2;
    int    area  : PIXEL_BITS * 2 + 2;

  } TCell, *PCell;

#else /* GRAYS_COMPACT */

  typedef struct  TCell_
  {
    TScan  x;
    TScan  y;
    int    cover;
    TArea  area;

  } TCell, *PCell;

#endif /* GRAYS_COMPACT */


  typedef struct  TRaster_
  {
    PCell  cells;
    int    max_cells;
    int    num_cells;

    TScan  min_ex, max_ex;
    TScan  min_ey, max_ey;

    TArea  area;
    int    cover;
    int    invalid;

    TScan  ex, ey;
    TScan  cx, cy;
    TPos   x,  y;

    TScan  last_ey;

    FT_Vector   bez_stack[32 * 3 + 1];
    int         lev_stack[32];

    FT_Outline  outline;
    FT_Bitmap   target;
    FT_BBox     clip_box;

    FT_Span     gray_spans[FT_MAX_GRAY_SPANS];
    int         num_gray_spans;

    FT_Raster_Span_Func  render_span;
    void*                render_span_data;
    int                  span_y;

    int  band_size;
    int  band_shoot;
    int  conic_level;
    int  cubic_level;

    void*    memory;
    jmp_buf  jump_buffer;

#ifdef GRAYS_USE_GAMMA
    FT_Byte  gamma[257];
#endif

  } TRaster, *PRaster;


  /*************************************************************************/
  /*                                                                       */
  /* Initialize the cells table.                                           */
  /*                                                                       */
  static void
  gray_init_cells( RAS_ARG_ void*  buffer,
                   long            byte_size )
  {
    ras.cells     = (PCell)buffer;
    ras.max_cells = byte_size / sizeof ( TCell );
    ras.num_cells = 0;
    ras.area      = 0;
    ras.cover     = 0;
    ras.invalid   = 1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Compute the outline bounding box.                                     */
  /*                                                                       */
  static void
  gray_compute_cbox( RAS_ARG )
  {
    FT_Outline*  outline = &ras.outline;
    FT_Vector*   vec     = outline->points;
    FT_Vector*   limit   = vec + outline->n_points;


    if ( outline->n_points <= 0 )
    {
      ras.min_ex = ras.max_ex = 0;
      ras.min_ey = ras.max_ey = 0;
      return;
    }

    ras.min_ex = ras.max_ex = vec->x;
    ras.min_ey = ras.max_ey = vec->y;

    vec++;

    for ( ; vec < limit; vec++ )
    {
      TPos  x = vec->x;
      TPos  y = vec->y;


      if ( x < ras.min_ex ) ras.min_ex = x;
      if ( x > ras.max_ex ) ras.max_ex = x;
      if ( y < ras.min_ey ) ras.min_ey = y;
      if ( y > ras.max_ey ) ras.max_ey = y;
    }

    /* truncate the bounding box to integer pixels */
    ras.min_ex = ras.min_ex >> 6;
    ras.min_ey = ras.min_ey >> 6;
    ras.max_ex = ( ras.max_ex + 63 ) >> 6;
    ras.max_ey = ( ras.max_ey + 63 ) >> 6;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Record the current cell in the table.                                 */
  /*                                                                       */
  static void
  gray_record_cell( RAS_ARG )
  {
    PCell  cell;


    if ( !ras.invalid && ( ras.area | ras.cover ) )
    {
      if ( ras.num_cells >= ras.max_cells )
        longjmp( ras.jump_buffer, 1 );

      cell        = ras.cells + ras.num_cells++;
      cell->x     = ras.ex - ras.min_ex;
      cell->y     = ras.ey - ras.min_ey;
      cell->area  = ras.area;
      cell->cover = ras.cover;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* Set the current cell to a new position.                               */
  /*                                                                       */
  static void
  gray_set_cell( RAS_ARG_ TScan  ex,
                          TScan  ey )
  {
    int  invalid, record, clean;


    /* Move the cell pointer to a new position.  We set the `invalid'      */
    /* flag to indicate that the cell isn't part of those we're interested */
    /* in during the render phase.  This means that:                       */
    /*                                                                     */
    /* . the new vertical position must be within min_ey..max_ey-1.        */
    /* . the new horizontal position must be strictly less than max_ex     */
    /*                                                                     */
    /* Note that if a cell is to the left of the clipping region, it is    */
    /* actually set to the (min_ex-1) horizontal position.                 */

    record  = 0;
    clean   = 1;

    invalid = ( ey < ras.min_ey || ey >= ras.max_ey || ex >= ras.max_ex );
    if ( !invalid )
    {
      /* All cells that are on the left of the clipping region go to the */
      /* min_ex - 1 horizontal position.                                 */
      if ( ex < ras.min_ex )
        ex = ras.min_ex - 1;

      /* if our position is new, then record the previous cell */
      if ( ex != ras.ex || ey != ras.ey )
        record = 1;
      else
        clean = ras.invalid;  /* do not clean if we didn't move from */
                              /* a valid cell                        */
    }

    /* record the previous cell if needed (i.e., if we changed the cell */
    /* position, of changed the `invalid' flag)                         */
    if ( ras.invalid != invalid || record )
      gray_record_cell( RAS_VAR );

    if ( clean )
    {
      ras.area  = 0;
      ras.cover = 0;
    }

    ras.invalid = invalid;
    ras.ex      = ex;
    ras.ey      = ey;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Start a new contour at a given cell.                                  */
  /*                                                                       */
  static void
  gray_start_cell( RAS_ARG_  TScan  ex,
                             TScan  ey )
  {
    if ( ex < ras.min_ex )
      ex = ras.min_ex - 1;

    ras.area    = 0;
    ras.cover   = 0;
    ras.ex      = ex;
    ras.ey      = ey;
    ras.last_ey = SUBPIXELS( ey );
    ras.invalid = 0;

    gray_set_cell( RAS_VAR_ ex, ey );
  }


  /*************************************************************************/
  /*                                                                       */
  /* Render a scanline as one or more cells.                               */
  /*                                                                       */
  static void
  gray_render_scanline( RAS_ARG_  TScan  ey,
                                  TPos   x1,
                                  TScan  y1,
                                  TPos   x2,
                                  TScan  y2 )
  {
    TScan  ex1, ex2, fx1, fx2, delta;
    long   p, first, dx;
    int    incr, lift, mod, rem;


    dx = x2 - x1;

    ex1 = TRUNC( x1 ); /* if (ex1 >= ras.max_ex) ex1 = ras.max_ex-1; */
    ex2 = TRUNC( x2 ); /* if (ex2 >= ras.max_ex) ex2 = ras.max_ex-1; */
    fx1 = x1 - SUBPIXELS( ex1 );
    fx2 = x2 - SUBPIXELS( ex2 );

    /* trivial case.  Happens often */
    if ( y1 == y2 )
    {
      gray_set_cell( RAS_VAR_ ex2, ey );
      return;
    }

    /* everything is located in a single cell.  That is easy! */
    /*                                                        */
    if ( ex1 == ex2 )
    {
      delta      = y2 - y1;
      ras.area  += (TArea)( fx1 + fx2 ) * delta;
      ras.cover += delta;
      return;
    }

    /* ok, we'll have to render a run of adjacent cells on the same */
    /* scanline...                                                  */
    /*                                                              */
    p     = ( ONE_PIXEL - fx1 ) * ( y2 - y1 );
    first = ONE_PIXEL;
    incr  = 1;

    if ( dx < 0 )
    {
      p     = fx1 * ( y2 - y1 );
      first = 0;
      incr  = -1;
      dx    = -dx;
    }

    delta = p / dx;
    mod   = p % dx;
    if ( mod < 0 )
    {
      delta--;
      mod += dx;
    }

    ras.area  += (TArea)( fx1 + first ) * delta;
    ras.cover += delta;

    ex1 += incr;
    gray_set_cell( RAS_VAR_ ex1, ey );
    y1  += delta;

    if ( ex1 != ex2 )
    {
      p     = ONE_PIXEL * ( y2 - y1 );
      lift  = p / dx;
      rem   = p % dx;
      if ( rem < 0 )
      {
        lift--;
        rem += dx;
      }

      mod -= dx;

      while ( ex1 != ex2 )
      {
        delta = lift;
        mod  += rem;
        if ( mod >= 0 )
        {
          mod -= dx;
          delta++;
        }

        ras.area  += (TArea)ONE_PIXEL * delta;
        ras.cover += delta;
        y1        += delta;
        ex1       += incr;
        gray_set_cell( RAS_VAR_ ex1, ey );
      }
    }

    delta      = y2 - y1;
    ras.area  += (TArea)( fx2 + ONE_PIXEL - first ) * delta;
    ras.cover += delta;
  }


  /*************************************************************************/
  /*                                                                       */
  /* Render a given line as a series of scanlines.                         */
  /*                                                                       */
  static void
  gray_render_line( RAS_ARG_ TPos  to_x,
                             TPos  to_y )
  {
    TScan  ey1, ey2, fy1, fy2;
    TPos   dx, dy, x, x2;
    int    p, rem, mod, lift, delta, first, incr;


    ey1 = TRUNC( ras.last_ey );
    ey2 = TRUNC( to_y ); /* if (ey2 >= ras.max_ey) ey2 = ras.max_ey-1; */
    fy1 = ras.y - ras.last_ey;
    fy2 = to_y - SUBPIXELS( ey2 );

    dx = to_x - ras.x;
    dy = to_y - ras.y;

    /* XXX: we should do something about the trivial case where dx == 0, */
    /*      as it happens very often!                                    */

    /* perform vertical clipping */
    {
      TScan  min, max;


      min = ey1;
      max = ey2;
      if ( ey1 > ey2 )
      {
        min = ey2;
        max = ey1;
      }
      if ( min >= ras.max_ey || max < ras.min_ey )
        goto End;
    }

    /* everything is on a single scanline */
    if ( ey1 == ey2 )
    {
      gray_render_scanline( RAS_VAR_ ey1, ras.x, fy1, to_x, fy2 );
      goto End;
    }

    /* ok, we have to render several scanlines */
    p     = ( ONE_PIXEL - fy1 ) * dx;
    first = ONE_PIXEL;
    incr  = 1;

    if ( dy < 0 )
    {
      p     = fy1 * dx;
      first = 0;
      incr  = -1;
      dy    = -dy;
    }

    delta = p / dy;
    mod   = p % dy;
    if ( mod < 0 )
    {
      delta--;
      mod += dy;
    }

    x = ras.x + delta;
    gray_render_scanline( RAS_VAR_ ey1, ras.x, fy1, x, first );

    ey1 += incr;
    gray_set_cell( RAS_VAR_ TRUNC( x ), ey1 );

    if ( ey1 != ey2 )
    {
      p     = ONE_PIXEL * dx;
      lift  = p / dy;
      rem   = p % dy;
      if ( rem < 0 )
      {
        lift--;
        rem += dy;
      }
      mod -= dy;

      while ( ey1 != ey2 )
      {
        delta = lift;
        mod  += rem;
        if ( mod >= 0 )
        {
          mod -= dy;
          delta++;
        }

        x2 = x + delta;
        gray_render_scanline( RAS_VAR_ ey1, x, ONE_PIXEL - first, x2, first );
        x = x2;

        ey1 += incr;
        gray_set_cell( RAS_VAR_ TRUNC( x ), ey1 );
      }
    }

    gray_render_scanline( RAS_VAR_ ey1, x, ONE_PIXEL - first, to_x, fy2 );

  End:
    ras.x       = to_x;
    ras.y       = to_y;
    ras.last_ey = SUBPIXELS( ey2 );
  }


  static void
  gray_split_conic( FT_Vector*  base )
  {
    TPos  a, b;


    base[4].x = base[2].x;
    b = base[1].x;
    a = base[3].x = ( base[2].x + b ) / 2;
    b = base[1].x = ( base[0].x + b ) / 2;
    base[2].x = ( a + b ) / 2;

    base[4].y = base[2].y;
    b = base[1].y;
    a = base[3].y = ( base[2].y + b ) / 2;
    b = base[1].y = ( base[0].y + b ) / 2;
    base[2].y = ( a + b ) / 2;
  }


  static void
  gray_render_conic( RAS_ARG_ FT_Vector*  control,
                              FT_Vector*  to )
  {
    TPos        dx, dy;
    int         top, level;
    int*        levels;
    FT_Vector*  arc;


    dx = DOWNSCALE( ras.x ) + to->x - ( control->x << 1 );
    if ( dx < 0 )
      dx = -dx;
    dy = DOWNSCALE( ras.y ) + to->y - ( control->y << 1 );
    if ( dy < 0 )
      dy = -dy;
    if ( dx < dy )
      dx = dy;

    level = 1;
    dx = dx / ras.conic_level;
    while ( dx > 0 )
    {
      dx >>= 2;
      level++;
    }

    /* a shortcut to speed things up */
    if ( level <= 1 )
    {
      /* we compute the mid-point directly in order to avoid */
      /* calling gray_split_conic()                          */
      TPos   to_x, to_y, mid_x, mid_y;


      to_x  = UPSCALE( to->x );
      to_y  = UPSCALE( to->y );
      mid_x = ( ras.x + to_x + 2 * UPSCALE( control->x ) ) / 4;
      mid_y = ( ras.y + to_y + 2 * UPSCALE( control->y ) ) / 4;

      gray_render_line( RAS_VAR_ mid_x, mid_y );
      gray_render_line( RAS_VAR_ to_x, to_y );
      return;
    }

    arc       = ras.bez_stack;
    levels    = ras.lev_stack;
    top       = 0;
    levels[0] = level;

    arc[0].x = UPSCALE( to->x );
    arc[0].y = UPSCALE( to->y );
    arc[1].x = UPSCALE( control->x );
    arc[1].y = UPSCALE( control->y );
    arc[2].x = ras.x;
    arc[2].y = ras.y;

    while ( top >= 0 )
    {
      level = levels[top];
      if ( level > 1 )
      {
        /* check that the arc crosses the current band */
        TPos  min, max, y;


        min = max = arc[0].y;

        y = arc[1].y;
        if ( y < min ) min = y;
        if ( y > max ) max = y;

        y = arc[2].y;
        if ( y < min ) min = y;
        if ( y > max ) max = y;

        if ( TRUNC( min ) >= ras.max_ey || TRUNC( max ) < 0 )
          goto Draw;

        gray_split_conic( arc );
        arc += 2;
        top++;
        levels[top] = levels[top - 1] = level - 1;
        continue;
      }

    Draw:
      {
        TPos  to_x, to_y, mid_x, mid_y;


        to_x  = arc[0].x;
        to_y  = arc[0].y;
        mid_x = ( ras.x + to_x + 2 * arc[1].x ) / 4;
        mid_y = ( ras.y + to_y + 2 * arc[1].y ) / 4;

        gray_render_line( RAS_VAR_ mid_x, mid_y );
        gray_render_line( RAS_VAR_ to_x, to_y );

        top--;
        arc -= 2;
      }
    }
    return;
  }


  static void
  gray_split_cubic( FT_Vector*  base )
  {
    TPos  a, b, c, d;


    base[6].x = base[3].x;
    c = base[1].x;
    d = base[2].x;
    base[1].x = a = ( base[0].x + c ) / 2;
    base[5].x = b = ( base[3].x + d ) / 2;
    c = ( c + d ) / 2;
    base[2].x = a = ( a + c ) / 2;
    base[4].x = b = ( b + c ) / 2;
    base[3].x = ( a + b ) / 2;

    base[6].y = base[3].y;
    c = base[1].y;
    d = base[2].y;
    base[1].y = a = ( base[0].y + c ) / 2;
    base[5].y = b = ( base[3].y + d ) / 2;
    c = ( c + d ) / 2;
    base[2].y = a = ( a + c ) / 2;
    base[4].y = b = ( b + c ) / 2;
    base[3].y = ( a + b ) / 2;
  }


  static void
  gray_render_cubic( RAS_ARG_ FT_Vector*  control1,
                              FT_Vector*  control2,
                              FT_Vector*  to )
  {
    TPos        dx, dy, da, db;
    int         top, level;
    int*        levels;
    FT_Vector*  arc;


    dx = DOWNSCALE( ras.x ) + to->x - ( control1->x << 1 );
    if ( dx < 0 )
      dx = -dx;
    dy = DOWNSCALE( ras.y ) + to->y - ( control1->y << 1 );
    if ( dy < 0 )
      dy = -dy;
    if ( dx < dy )
      dx = dy;
    da = dx;

    dx = DOWNSCALE( ras.x ) + to->x - 3 * ( control1->x + control2->x );
    if ( dx < 0 )
      dx = -dx;
    dy = DOWNSCALE( ras.y ) + to->y - 3 * ( control1->x + control2->y );
    if ( dy < 0 )
      dy = -dy;
    if ( dx < dy )
      dx = dy;
    db = dx;

    level = 1;
    da    = da / ras.cubic_level;
    db    = db / ras.conic_level;
    while ( da > 0 || db > 0 )
    {
      da >>= 2;
      db >>= 3;
      level++;
    }

    if ( level <= 1 )
    {
      TPos   to_x, to_y, mid_x, mid_y;


      to_x  = UPSCALE( to->x );
      to_y  = UPSCALE( to->y );
      mid_x = ( ras.x + to_x +
                3 * UPSCALE( control1->x + control2->x ) ) / 8;
      mid_y = ( ras.y + to_y +
                3 * UPSCALE( control1->y + control2->y ) ) / 8;

      gray_render_line( RAS_VAR_ mid_x, mid_y );
      gray_render_line( RAS_VAR_ to_x, to_y );
      return;
    }

    arc      = ras.bez_stack;
    arc[0].x = UPSCALE( to->x );
    arc[0].y = UPSCALE( to->y );
    arc[1].x = UPSCALE( control2->x );
    arc[1].y = UPSCALE( control2->y );
    arc[2].x = UPSCALE( control1->x );
    arc[2].y = UPSCALE( control1->y );
    arc[3].x = ras.x;
    arc[3].y = ras.y;

    levels    = ras.lev_stack;
    top       = 0;
    levels[0] = level;

    while ( top >= 0 )
    {
      level = levels[top];
      if ( level > 1 )
      {
        /* check that the arc crosses the current band */
        TPos  min, max, y;


        min = max = arc[0].y;
        y = arc[1].y;
        if ( y < min ) min = y;
        if ( y > max ) max = y;
        y = arc[2].y;
        if ( y < min ) min = y;
        if ( y > max ) max = y;
        y = arc[3].y;
        if ( y < min ) min = y;
        if ( y > max ) max = y;
        if ( TRUNC( min ) >= ras.max_ey || TRUNC( max ) < 0 )
          goto Draw;
        gray_split_cubic( arc );
        arc += 3;
        top ++;
        levels[top] = levels[top - 1] = level - 1;
        continue;
      }

    Draw:
      {
        TPos  to_x, to_y, mid_x, mid_y;


        to_x  = arc[0].x;
        to_y  = arc[0].y;
        mid_x = ( ras.x + to_x + 3 * ( arc[1].x + arc[2].x ) ) / 8;
        mid_y = ( ras.y + to_y + 3 * ( arc[1].y + arc[2].y ) ) / 8;

        gray_render_line( RAS_VAR_ mid_x, mid_y );
        gray_render_line( RAS_VAR_ to_x, to_y );
        top --;
        arc -= 3;
      }
    }
    return;
  }


  /* a macro comparing two cell pointers.  Returns true if a <= b. */
#if 1

#define PACK( a )          ( ( (long)(a)->y << 16 ) + (a)->x )
#define LESS_THAN( a, b )  ( PACK( a ) < PACK( b ) )

#else /* 1 */

#define LESS_THAN( a, b )  ( (a)->y < (b)->y || \
                             ( (a)->y == (b)->y && (a)->x < (b)->x ) )

#endif /* 1 */

#define SWAP_CELLS( a, b, temp )  do             \
                                  {              \
                                    temp = *(a); \
                                    *(a) = *(b); \
                                    *(b) = temp; \
                                  } while ( 0 )
#define DEBUG_SORT
#define QUICK_SORT

#ifdef SHELL_SORT

  /* a simple shell sort algorithm that works directly on our */
  /* cells table                                              */
  static void
  gray_shell_sort ( PCell  cells,
                    int    count )
  {
    PCell  i, j, limit = cells + count;
    TCell  temp;
    int    gap;


    /* compute initial gap */
    for ( gap = 0; ++gap < count; gap *= 3 )
      ;

    while ( gap /= 3 )
    {
      for ( i = cells + gap; i < limit; i++ )
      {
        for ( j = i - gap; ; j -= gap )
        {
          PCell  k = j + gap;


          if ( LESS_THAN( j, k ) )
            break;

          SWAP_CELLS( j, k, temp );

          if ( j < cells + gap )
            break;
        }
      }
    }
  }

#endif /* SHELL_SORT */


#ifdef QUICK_SORT

  /* This is a non-recursive quicksort that directly process our cells     */
  /* array.  It should be faster than calling the stdlib qsort(), and we   */
  /* can even tailor our insertion threshold...                            */

#define QSORT_THRESHOLD  9  /* below this size, a sub-array will be sorted */
                            /* through a normal insertion sort             */

  static void
  gray_quick_sort( PCell  cells,
                   int    count )
  {
    PCell   stack[40];  /* should be enough ;-) */
    PCell*  top;        /* top of stack */
    PCell   base, limit;
    TCell   temp;


    limit = cells + count;
    base  = cells;
    top   = stack;

    for (;;)
    {
      int    len = (int)( limit - base );
      PCell  i, j, pivot;


      if ( len > QSORT_THRESHOLD )
      {
        /* we use base + len/2 as the pivot */
        pivot = base + len / 2;
        SWAP_CELLS( base, pivot, temp );

        i = base + 1;
        j = limit - 1;

        /* now ensure that *i <= *base <= *j */
        if ( LESS_THAN( j, i ) )
          SWAP_CELLS( i, j, temp );

        if ( LESS_THAN( base, i ) )
          SWAP_CELLS( base, i, temp );

        if ( LESS_THAN( j, base ) )
          SWAP_CELLS( base, j, temp );

        for (;;)
        {
          do i++; while ( LESS_THAN( i, base ) );
          do j--; while ( LESS_THAN( base, j ) );

          if ( i > j )
            break;

          SWAP_CELLS( i, j, temp );
        }

        SWAP_CELLS( base, j, temp );

        /* now, push the largest sub-array */
        if ( j - base > limit - i )
        {
          top[0] = base;
          top[1] = j;
          base   = i;
        }
        else
        {
          top[0] = i;
          top[1] = limit;
          limit  = j;
        }
        top += 2;
      }
      else
      {
        /* the sub-array is small, perform insertion sort */
        j = base;
        i = j + 1;

        for ( ; i < limit; j = i, i++ )
        {
          for ( ; LESS_THAN( j + 1, j ); j-- )
          {
            SWAP_CELLS( j + 1, j, temp );
            if ( j == base )
              break;
          }
        }
        if ( top > stack )
        {
          top  -= 2;
          base  = top[0];
          limit = top[1];
        }
        else
          break;
      }
    }
  }

#endif /* QUICK_SORT */


#ifdef DEBUG_GRAYS
#ifdef DEBUG_SORT

  static int
  gray_check_sort( PCell  cells,
                   int    count )
  {
    PCell  p, q;


    for ( p = cells + count - 2; p >= cells; p-- )
    {
      q = p + 1;
      if ( !LESS_THAN( p, q ) )
        return 0;
    }
    return 1;
  }

#endif /* DEBUG_SORT */
#endif /* DEBUG_GRAYS */


  static int
  gray_move_to( FT_Vector*  to,
                FT_Raster   raster )
  {
    TPos  x, y;


    /* record current cell, if any */
    gray_record_cell( (PRaster)raster );

    /* start to a new position */
    x = UPSCALE( to->x );
    y = UPSCALE( to->y );

    gray_start_cell( (PRaster)raster, TRUNC( x ), TRUNC( y ) );

    ((PRaster)raster)->x = x;
    ((PRaster)raster)->y = y;
    return 0;
  }


  static int
  gray_line_to( FT_Vector*  to,
                FT_Raster   raster )
  {
    gray_render_line( (PRaster)raster,
                      UPSCALE( to->x ), UPSCALE( to->y ) );
    return 0;
  }


  static int
  gray_conic_to( FT_Vector*  control,
                 FT_Vector*  to,
                 FT_Raster   raster )
  {
    gray_render_conic( (PRaster)raster, control, to );
    return 0;
  }


  static int
  gray_cubic_to( FT_Vector*  control1,
                 FT_Vector*  control2,
                 FT_Vector*  to,
                 FT_Raster   raster )
  {
    gray_render_cubic( (PRaster)raster, control1, control2, to );
    return 0;
  }


  static void
  gray_render_span( int       y,
                    int       count,
                    FT_Span*  spans,
                    PRaster   raster )
  {
    unsigned char*  p;
    FT_Bitmap*      map = &raster->target;


    /* first of all, compute the scanline offset */
    p = (unsigned char*)map->buffer - y * map->pitch;
    if ( map->pitch >= 0 )
      p += ( map->rows - 1 ) * map->pitch;

    for ( ; count > 0; count--, spans++ )
    {
      FT_UInt  coverage = spans->coverage;


#ifdef GRAYS_USE_GAMMA
      coverage = raster->gamma[(FT_Byte)coverage];
#endif

      if ( coverage )
#if 1
        MEM_Set( p + spans->x, (unsigned char)coverage, spans->len );
#else /* 1 */
      {
        q     = p + spans->x;
        limit = q + spans->len;
        for ( ; q < limit; q++ )
          q[0] = (unsigned char)coverage;
      }
#endif /* 1 */
    }
  }


#ifdef DEBUG_GRAYS

#include <stdio.h>

  static void
  gray_dump_cells( RAS_ARG )
  {
    PCell  cell, limit;
    int    y = -1;


    cell  = ras.cells;
    limit = cell + ras.num_cells;

    for ( ; cell < limit; cell++ )
    {
      if ( cell->y != y )
      {
        fprintf( stderr, "\n%2d: ", cell->y );
        y = cell->y;
      }
      fprintf( stderr, "[%d %d %d]",
               cell->x, cell->area, cell->cover );
    }
    fprintf(stderr, "\n" );
  }

#endif /* DEBUG_GRAYS */


  static void
  gray_hline( RAS_ARG_ TScan  x,
                       TScan  y,
                       TPos   area,
                       int    acount )
  {
    FT_Span*   span;
    int        count;
    int        coverage;


    /* compute the coverage line's coverage, depending on the    */
    /* outline fill rule                                         */
    /*                                                           */
    /* the coverage percentage is area/(PIXEL_BITS*PIXEL_BITS*2) */
    /*                                                           */
    coverage = area >> ( PIXEL_BITS * 2 + 1 - 8);  /* use range 0..256 */

    if ( ras.outline.flags & ft_outline_even_odd_fill )
    {
      if ( coverage < 0 )
        coverage = -coverage;

      while ( coverage >= 512 )
        coverage -= 512;

      if ( coverage > 256 )
        coverage = 512 - coverage;
      else if ( coverage == 256 )
        coverage = 255;
    }
    else
    {
      /* normal non-zero winding rule */
      if ( coverage < 0 )
        coverage = -coverage;

      if ( coverage >= 256 )
        coverage = 255;
    }

    y += ras.min_ey;
    x += ras.min_ex;

    if ( coverage )
    {
      /* see if we can add this span to the current list */
      count = ras.num_gray_spans;
      span  = ras.gray_spans + count - 1;
      if ( count > 0                          &&
           ras.span_y == y                    &&
           (int)span->x + span->len == (int)x &&
           span->coverage == coverage )
      {
        span->len = (unsigned short)( span->len + acount );
        return;
      }

      if ( ras.span_y != y || count >= FT_MAX_GRAY_SPANS )
      {
        if ( ras.render_span && count > 0 )
          ras.render_span( ras.span_y, count, ras.gray_spans,
                           ras.render_span_data );
        /* ras.render_span( span->y, ras.gray_spans, count ); */

#ifdef DEBUG_GRAYS

        if ( ras.span_y >= 0 )
        {
          int  n;


          fprintf( stderr, "y=%3d ", ras.span_y );
          span = ras.gray_spans;
          for ( n = 0; n < count; n++, span++ )
            fprintf( stderr, "[%d..%d]:%02x ",
                     span->x, span->x + span->len - 1, span->coverage );
          fprintf( stderr, "\n" );
        }

#endif /* DEBUG_GRAYS */

        ras.num_gray_spans = 0;
        ras.span_y         = y;

        count = 0;
        span  = ras.gray_spans;
      }
      else
        span++;

      /* add a gray span to the current list */
      span->x        = (short)x;
      span->len      = (unsigned short)acount;
      span->coverage = (unsigned char)coverage;
      ras.num_gray_spans++;
    }
  }


  static void
  gray_sweep( RAS_ARG_ FT_Bitmap*  target )
  {
    TScan  x, y, cover;
    TArea  area;
    PCell  start, cur, limit;

    FT_UNUSED( target );

    if ( ras.num_cells == 0 )
      return;

    cur   = ras.cells;
    limit = cur + ras.num_cells;

    cover              = 0;
    ras.span_y         = -1;
    ras.num_gray_spans = 0;

    for (;;)
    {
      start  = cur;
      y      = start->y;
      x      = start->x;

      area   = start->area;
      cover += start->cover;

      /* accumulate all start cells */
      for (;;)
      {
        ++cur;
        if ( cur >= limit || cur->y != start->y || cur->x != start->x )
          break;

        area  += cur->area;
        cover += cur->cover;
      }

      /* if the start cell has a non-null area, we must draw an */
      /* individual gray pixel there                            */
      if ( area && x >= 0 )
      {
        gray_hline( RAS_VAR_ x, y, cover * ( ONE_PIXEL * 2 ) - area, 1 );
        x++;
      }

      if ( x < 0 )
        x = 0;

      if ( cur < limit && start->y == cur->y )
      {
        /* draw a gray span between the start cell and the current one */
        if ( cur->x > x )
          gray_hline( RAS_VAR_ x, y,
                       cover * ( ONE_PIXEL * 2 ), cur->x - x );
      }
      else
      {
        /* draw a gray span until the end of the clipping region */
        if ( cover && x < ras.max_ex - ras.min_ex )
          gray_hline( RAS_VAR_ x, y,
                       cover * ( ONE_PIXEL * 2 ),
                       ras.max_ex - x - ras.min_ex );
        cover = 0;
      }

      if ( cur >= limit )
        break;
    }

    if ( ras.render_span && ras.num_gray_spans > 0 )
      ras.render_span( ras.span_y, ras.num_gray_spans,
                       ras.gray_spans, ras.render_span_data );

#ifdef DEBUG_GRAYS

    {
      int       n;
      FT_Span*  span;


      fprintf( stderr, "y=%3d ", ras.span_y );
      span = ras.gray_spans;
      for ( n = 0; n < ras.num_gray_spans; n++, span++ )
        fprintf( stderr, "[%d..%d]:%02x ",
                 span->x, span->x + span->len - 1, span->coverage );
      fprintf( stderr, "\n" );
    }

#endif /* DEBUG_GRAYS */

  }


#ifdef _STANDALONE_

  /*************************************************************************/
  /*                                                                       */
  /*  The following function should only compile in stand_alone mode,      */
  /*  i.e., when building this component without the rest of FreeType.     */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Outline_Decompose                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Walks over an outline's structure to decompose it into individual  */
  /*    segments and Bezier arcs.  This function is also able to emit      */
  /*    `move to' and `close to' operations to indicate the start and end  */
  /*    of new contours in the outline.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    outline   :: A pointer to the source target.                       */
  /*                                                                       */
  /*    interface :: A table of `emitters', i.e,. function pointers called */
  /*                 during decomposition to indicate path operations.     */
  /*                                                                       */
  /*    user      :: A typeless pointer which is passed to each emitter    */
  /*                 during the decomposition.  It can be used to store    */
  /*                 the state during the decomposition.                   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Error code.  0 means sucess.                                       */
  /*                                                                       */
  static
  int  FT_Outline_Decompose( FT_Outline*              outline,
                             const FT_Outline_Funcs*  interface,
                             void*                    user )
  {
#undef SCALED
#if 0
#define SCALED( x )  ( ( (x) << shift ) - delta )
#else
#define SCALED( x )  (x)
#endif

    FT_Vector   v_last;
    FT_Vector   v_control;
    FT_Vector   v_start;

    FT_Vector*  point;
    FT_Vector*  limit;
    char*       tags;

    int     n;         /* index of contour in outline     */
    int     first;     /* index of first point in contour */
    int     error;
    char    tag;       /* current point's state           */

#if 0
    int     shift = interface->shift;
    FT_Pos  delta = interface->delta;
#endif


    first = 0;

    for ( n = 0; n < outline->n_contours; n++ )
    {
      int  last;  /* index of last point in contour */


      last  = outline->contours[n];
      limit = outline->points + last;

      v_start = outline->points[first];
      v_last  = outline->points[last];

      v_start.x = SCALED( v_start.x ); v_start.y = SCALED( v_start.y );
      v_last.x  = SCALED( v_last.x );  v_last.y  = SCALED( v_last.y );

      v_control = v_start;

      point = outline->points + first;
      tags  = outline->tags  + first;
      tag   = FT_CURVE_TAG( tags[0] );

      /* A contour cannot start with a cubic control point! */
      if ( tag == FT_Curve_Tag_Cubic )
        goto Invalid_Outline;

      /* check first point to determine origin */
      if ( tag == FT_Curve_Tag_Conic )
      {
        /* first point is conic control.  Yes, this happens. */
        if ( FT_CURVE_TAG( outline->tags[last] ) == FT_Curve_Tag_On )
        {
          /* start at last point if it is on the curve */
          v_start = v_last;
          limit--;
        }
        else
        {
          /* if both first and last points are conic,         */
          /* start at their middle and record its position    */
          /* for closure                                      */
          v_start.x = ( v_start.x + v_last.x ) / 2;
          v_start.y = ( v_start.y + v_last.y ) / 2;

          v_last = v_start;
        }
        point--;
        tags--;
      }

      error = interface->move_to( &v_start, user );
      if ( error )
        goto Exit;

      while ( point < limit )
      {
        point++;
        tags++;

        tag = FT_CURVE_TAG( tags[0] );
        switch ( tag )
        {
        case FT_Curve_Tag_On:  /* emit a single line_to */
          {
            FT_Vector  vec;


            vec.x = SCALED( point->x );
            vec.y = SCALED( point->y );

            error = interface->line_to( &vec, user );
            if ( error )
              goto Exit;
            continue;
          }

        case FT_Curve_Tag_Conic:  /* consume conic arcs */
          {
            v_control.x = SCALED( point->x );
            v_control.y = SCALED( point->y );

          Do_Conic:
            if ( point < limit )
            {
              FT_Vector  vec;
              FT_Vector  v_middle;


              point++;
              tags++;
              tag = FT_CURVE_TAG( tags[0] );

              vec.x = SCALED( point->x );
              vec.y = SCALED( point->y );

              if ( tag == FT_Curve_Tag_On )
              {
                error = interface->conic_to( &v_control, &vec, user );
                if ( error )
                  goto Exit;
                continue;
              }

              if ( tag != FT_Curve_Tag_Conic )
                goto Invalid_Outline;

              v_middle.x = ( v_control.x + vec.x ) / 2;
              v_middle.y = ( v_control.y + vec.y ) / 2;

              error = interface->conic_to( &v_control, &v_middle, user );
              if ( error )
                goto Exit;

              v_control = vec;
              goto Do_Conic;
            }

            error = interface->conic_to( &v_control, &v_start, user );
            goto Close;
          }

        default:  /* FT_Curve_Tag_Cubic */
          {
            FT_Vector  vec1, vec2;


            if ( point + 1 > limit                             ||
                 FT_CURVE_TAG( tags[1] ) != FT_Curve_Tag_Cubic )
              goto Invalid_Outline;

            point += 2;
            tags  += 2;

            vec1.x = SCALED( point[-2].x ); vec1.y = SCALED( point[-2].y );
            vec2.x = SCALED( point[-1].x ); vec2.y = SCALED( point[-1].y );

            if ( point <= limit )
            {
              FT_Vector  vec;


              vec.x = SCALED( point->x );
              vec.y = SCALED( point->y );

              error = interface->cubic_to( &vec1, &vec2, &vec, user );
              if ( error )
                goto Exit;
              continue;
            }

            error = interface->cubic_to( &vec1, &vec2, &v_start, user );
            goto Close;
          }
        }
      }

      /* close the contour with a line segment */
      error = interface->line_to( &v_start, user );

   Close:
      if ( error )
        goto Exit;

      first = last + 1;
    }

    return 0;

  Exit:
    return error;

  Invalid_Outline:
    return ErrRaster_Invalid_Outline;
  }

#endif /* _STANDALONE_ */


  typedef struct  TBand_
  {
    FT_Pos  min, max;

  } TBand;


  static int
  gray_convert_glyph_inner( RAS_ARG )
  {
    static
    const FT_Outline_Funcs  interface =
    {
      (FT_Outline_MoveTo_Func) gray_move_to,
      (FT_Outline_LineTo_Func) gray_line_to,
      (FT_Outline_ConicTo_Func)gray_conic_to,
      (FT_Outline_CubicTo_Func)gray_cubic_to,
      0,
      0
    };

    volatile int  error = 0;

    if ( setjmp( ras.jump_buffer ) == 0 )
    {
      error = FT_Outline_Decompose( &ras.outline, &interface, &ras );
      gray_record_cell( RAS_VAR );
    }
    else
    {
      error = ErrRaster_MemoryOverflow;
    }

    return error;
  }


  static int
  gray_convert_glyph( RAS_ARG )
  {
    TBand             bands[40];
    volatile  TBand*  band;
    volatile  int     n, num_bands;
    volatile  TPos    min, max, max_y;
    FT_BBox*          clip;


    /* Set up state in the raster object */
    gray_compute_cbox( RAS_VAR );

    /* clip to target bitmap, exit if nothing to do */
    clip = &ras.clip_box;

    if ( ras.max_ex <= clip->xMin || ras.min_ex >= clip->xMax ||
         ras.max_ey <= clip->yMin || ras.min_ey >= clip->yMax )
      return 0;

    if ( ras.min_ex < clip->xMin ) ras.min_ex = clip->xMin;
    if ( ras.min_ey < clip->yMin ) ras.min_ey = clip->yMin;

    if ( ras.max_ex > clip->xMax ) ras.max_ex = clip->xMax;
    if ( ras.max_ey > clip->yMax ) ras.max_ey = clip->yMax;

    /* simple heuristic used to speed-up the bezier decomposition -- see */
    /* the code in gray_render_conic() and gray_render_cubic() for more  */
    /* details                                                           */
    ras.conic_level = 32;
    ras.cubic_level = 16;

    {
      int level = 0;


      if ( ras.max_ex > 24 || ras.max_ey > 24 )
        level++;
      if ( ras.max_ex > 120 || ras.max_ey > 120 )
        level++;

      ras.conic_level <<= level;
      ras.cubic_level <<= level;
    }

    /* setup vertical bands */
    num_bands = ( ras.max_ey - ras.min_ey ) / ras.band_size;
    if ( num_bands == 0 )  num_bands = 1;
    if ( num_bands >= 39 ) num_bands = 39;

    ras.band_shoot = 0;

    min   = ras.min_ey;
    max_y = ras.max_ey;

    for ( n = 0; n < num_bands; n++, min = max )
    {
      max = min + ras.band_size;
      if ( n == num_bands - 1 || max > max_y )
        max = max_y;

      bands[0].min = min;
      bands[0].max = max;
      band         = bands;

      while ( band >= bands )
      {
        FT_Pos  bottom, top, middle;
        int     error;


        ras.num_cells = 0;
        ras.invalid   = 1;
        ras.min_ey    = band->min;
        ras.max_ey    = band->max;

#if 1
        error = gray_convert_glyph_inner( RAS_VAR );
#else
        error = FT_Outline_Decompose( outline, &interface, &ras ) ||
                gray_record_cell( RAS_VAR );
#endif

        if ( !error )
        {
#ifdef SHELL_SORT
          gray_shell_sort( ras.cells, ras.num_cells );
#else
          gray_quick_sort( ras.cells, ras.num_cells );
#endif

#ifdef DEBUG_GRAYS
          gray_check_sort( ras.cells, ras.num_cells );
          gray_dump_cells( RAS_VAR );
#endif

          gray_sweep( RAS_VAR_  &ras.target );
          band--;
          continue;
        }
        else if ( error != ErrRaster_MemoryOverflow )
          return 1;

        /* render pool overflow, we will reduce the render band by half */
        bottom = band->min;
        top    = band->max;
        middle = bottom + ( ( top - bottom ) >> 1 );

        /* waoow! This is too complex for a single scanline, something */
        /* must be really rotten here!                                 */
        if ( middle == bottom )
        {
#ifdef DEBUG_GRAYS
          fprintf( stderr, "Rotten glyph!\n" );
#endif
          return 1;
        }

        if ( bottom-top >= ras.band_size )
          ras.band_shoot++;

        band[1].min = bottom;
        band[1].max = middle;
        band[0].min = middle;
        band[0].max = top;
        band++;
      }
    }

    if ( ras.band_shoot > 8 && ras.band_size > 16 )
      ras.band_size = ras.band_size / 2;

    return 0;
  }


  extern int
  gray_raster_render( PRaster            raster,
                      FT_Raster_Params*  params )
  {
    FT_Outline*  outline = (FT_Outline*)params->source;
    FT_Bitmap*   target_map = params->target;


    if ( !raster || !raster->cells || !raster->max_cells )
      return -1;

    /* return immediately if the outline is empty */
    if ( outline->n_points == 0 || outline->n_contours <= 0 )
      return 0;

    if ( !outline || !outline->contours || !outline->points )
      return ErrRaster_Invalid_Outline;

    if ( outline->n_points !=
           outline->contours[outline->n_contours - 1] + 1 )
      return ErrRaster_Invalid_Outline;

    /* if direct mode is not set, we must have a target bitmap */
    if ( ( params->flags & ft_raster_flag_direct ) == 0 &&
         ( !target_map || !target_map->buffer )         )
      return -1;

    /* this version does not support monochrome rendering */
    if ( !( params->flags & ft_raster_flag_aa ) )
      return ErrRaster_Invalid_Mode;

    /* compute clipping box */
    if ( ( params->flags & ft_raster_flag_direct ) == 0 )
    {
      /* compute clip box from target pixmap */
      ras.clip_box.xMin = 0;
      ras.clip_box.yMin = 0;
      ras.clip_box.xMax = target_map->width;
      ras.clip_box.yMax = target_map->rows;
    }
    else if ( params->flags & ft_raster_flag_clip )
    {
      ras.clip_box = params->clip_box;
    }
    else
    {
      ras.clip_box.xMin = -32768L;
      ras.clip_box.yMin = -32768L;
      ras.clip_box.xMax =  32767L;
      ras.clip_box.yMax =  32767L;
    }

    ras.outline   = *outline;
    ras.num_cells = 0;
    ras.invalid   = 1;

    if ( target_map )
      ras.target = *target_map;

    ras.render_span      = (FT_Raster_Span_Func)gray_render_span;
    ras.render_span_data = &ras;

    if ( params->flags & ft_raster_flag_direct )
    {
      ras.render_span      = (FT_Raster_Span_Func)params->gray_spans;
      ras.render_span_data = params->user;
    }

    return gray_convert_glyph( (PRaster)raster );
  }


  /**** RASTER OBJECT CREATION: In standalone mode, we simply use *****/
  /****                         a static object.                  *****/

#ifdef GRAYS_USE_GAMMA

  /* initialize the "gamma" table. Yes, this is really a crummy function */
  /* but the results look pretty good for something that simple.         */
  /*                                                                     */
#define M_MAX  255
#define M_X    128
#define M_Y    192

  static void
  grays_init_gamma( PRaster  raster )
  {
    FT_UInt  x, a;


    for ( x = 0; x < 256; x++ )
    {
      if ( x <= M_X )
        a = ( x * M_Y + M_X / 2) / M_X;
      else
        a = M_Y + ( ( x - M_X ) * ( M_MAX - M_Y ) +
            ( M_MAX - M_X ) / 2 ) / ( M_MAX - M_X );

      raster->gamma[x] = (FT_Byte)a;
    }
  }

#endif /* GRAYS_USE_GAMMA */

#ifdef _STANDALONE_

  static int
  gray_raster_new( void*       memory,
                   FT_Raster*  araster )
  {
    static TRaster  the_raster;

    FT_UNUSED( memory );


    *araster = (FT_Raster)&the_raster;
    MEM_Set( &the_raster, 0, sizeof ( the_raster ) );

#ifdef GRAYS_USE_GAMMA
    grays_init_gamma( (PRaster)*araster );
#endif

    return 0;
  }


  static void
  gray_raster_done( FT_Raster  raster )
  {
    /* nothing */
    FT_UNUSED( raster );
  }

#else /* _STANDALONE_ */

  static int
  gray_raster_new( FT_Memory   memory,
                   FT_Raster*  araster )
  {
    FT_Error  error;
    PRaster   raster;


    *araster = 0;
    if ( !ALLOC( raster, sizeof ( TRaster ) ) )
    {
      raster->memory = memory;
      *araster = (FT_Raster)raster;

#ifdef GRAYS_USE_GAMMA
      grays_init_gamma( raster );
#endif
    }

    return error;
  }


  static void
  gray_raster_done( FT_Raster  raster )
  {
    FT_Memory  memory = (FT_Memory)((PRaster)raster)->memory;


    FREE( raster );
  }

#endif /* _STANDALONE_ */


  static void
  gray_raster_reset( FT_Raster    raster,
                     const char*  pool_base,
                     long         pool_size )
  {
    PRaster  rast = (PRaster)raster;


    if ( raster && pool_base && pool_size >= 4096 )
      gray_init_cells( rast, (char*)pool_base, pool_size );

    rast->band_size  = ( pool_size / sizeof ( TCell ) ) / 8;
  }


  const FT_Raster_Funcs  ft_grays_raster =
  {
    ft_glyph_format_outline,

    (FT_Raster_New_Func)      gray_raster_new,
    (FT_Raster_Reset_Func)    gray_raster_reset,
    (FT_Raster_Set_Mode_Func) 0,
    (FT_Raster_Render_Func)   gray_raster_render,
    (FT_Raster_Done_Func)     gray_raster_done
  };


/* END */
