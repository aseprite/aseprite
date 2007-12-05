/***************************************************************************/
/*                                                                         */
/*  ftraster.c                                                             */
/*                                                                         */
/*    The FreeType glyph rasterizer (body).                                */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
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
  /* This is a rewrite of the FreeType 1.x scan-line converter             */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include "ftraster.h"
#include FT_INTERNAL_CALC_H   /* for FT_MulDiv only */


  /*************************************************************************/
  /*                                                                       */
  /* A simple technical note on how the raster works                       */
  /* -----------------------------------------------                       */
  /*                                                                       */
  /*   Converting an outline into a bitmap is achieved in several steps:   */
  /*                                                                       */
  /*   1 - Decomposing the outline into successive `profiles'.  Each       */
  /*       profile is simply an array of scanline intersections on a given */
  /*       dimension.  A profile's main attributes are                     */
  /*                                                                       */
  /*       o its scanline position boundaries, i.e. `Ymin' and `Ymax'.     */
  /*                                                                       */
  /*       o an array of intersection coordinates for each scanline        */
  /*         between `Ymin' and `Ymax'.                                    */
  /*                                                                       */
  /*       o a direction, indicating whether it was built going `up' or    */
  /*         `down', as this is very important for filling rules.          */
  /*                                                                       */
  /*   2 - Sweeping the target map's scanlines in order to compute segment */
  /*       `spans' which are then filled.  Additionally, this pass         */
  /*       performs drop-out control.                                      */
  /*                                                                       */
  /*   The outline data is parsed during step 1 only.  The profiles are    */
  /*   built from the bottom of the render pool, used as a stack.  The     */
  /*   following graphics shows the profile list under construction:       */
  /*                                                                       */
  /*     ____________________________________________________________ _ _  */
  /*    |         |                   |         |                 |        */
  /*    | profile | coordinates for   | profile | coordinates for |-->     */
  /*    |    1    |  profile 1        |    2    |  profile 2      |-->     */
  /*    |_________|___________________|_________|_________________|__ _ _  */
  /*                                                                       */
  /*    ^                                                         ^        */
  /*    |                                                         |        */
  /*  start of render pool                                       top       */
  /*                                                                       */
  /*   The top of the profile stack is kept in the `top' variable.         */
  /*                                                                       */
  /*   As you can see, a profile record is pushed on top of the render     */
  /*   pool, which is then followed by its coordinates/intersections.  If  */
  /*   a change of direction is detected in the outline, a new profile is  */
  /*   generated until the end of the outline.                             */
  /*                                                                       */
  /*   Note that when all profiles have been generated, the function       */
  /*   Finalize_Profile_Table() is used to record, for each profile, its   */
  /*   bottom-most scanline as well as the scanline above its upmost       */
  /*   boundary.  These positions are called `y-turns' because they (sort  */
  /*   of) correspond to local extrema.  They are stored in a sorted list  */
  /*   built from the top of the render pool as a downwards stack:         */
  /*                                                                       */
  /*      _ _ _______________________________________                      */
  /*                            |                    |                     */
  /*                         <--| sorted list of     |                     */
  /*                         <--|  extrema scanlines |                     */
  /*      _ _ __________________|____________________|                     */
  /*                                                                       */
  /*                            ^                    ^                     */
  /*                            |                    |                     */
  /*                         maxBuff           sizeBuff = end of pool      */
  /*                                                                       */
  /*   This list is later used during the sweep phase in order to          */
  /*   optimize performance (see technical note on the sweep below).       */
  /*                                                                       */
  /*   Of course, the raster detects whether the two stacks collide and    */
  /*   handles the situation propertly.                                    */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*************************************************************************/
  /**                                                                     **/
  /**  CONFIGURATION MACROS                                               **/
  /**                                                                     **/
  /*************************************************************************/
  /*************************************************************************/

  /* define DEBUG_RASTER if you want to compile a debugging version */
#define xxxDEBUG_RASTER

  /* The default render pool size in bytes */
#define RASTER_RENDER_POOL  8192

  /* undefine FT_RASTER_OPTION_ANTI_ALIASING if you do not want to support */
  /* 5-levels anti-aliasing                                                */
#ifdef FT_CONFIG_OPTION_5_GRAY_LEVELS
#define FT_RASTER_OPTION_ANTI_ALIASING
#endif

  /* The size of the two-lines intermediate bitmap used */
  /* for anti-aliasing, in bytes.                       */
#define RASTER_GRAY_LINES  2048


  /*************************************************************************/
  /*************************************************************************/
  /**                                                                     **/
  /**  OTHER MACROS (do not change)                                       **/
  /**                                                                     **/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_raster


#ifdef _STANDALONE_


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

#define Raster_Err_None          0
#define Raster_Err_Not_Ini      -1
#define Raster_Err_Overflow     -2
#define Raster_Err_Neg_Height   -3
#define Raster_Err_Invalid      -4
#define Raster_Err_Unsupported  -5


#else /* _STANDALONE_ */


#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H        /* for FT_TRACE() and FT_ERROR() */

#include "rasterrs.h"

#define Raster_Err_None         Raster_Err_Ok
#define Raster_Err_Not_Ini      Raster_Err_Raster_Uninitialized
#define Raster_Err_Overflow     Raster_Err_Raster_Overflow
#define Raster_Err_Neg_Height   Raster_Err_Raster_Negative_Height
#define Raster_Err_Invalid      Raster_Err_Invalid_Outline
#define Raster_Err_Unsupported  Raster_Err_Cannot_Render_Glyph


#endif /* _STANDALONE_ */


#ifndef MEM_Set
#define MEM_Set( d, s, c )  memset( d, s, c )
#endif


  /* FMulDiv means `Fast MulDiv'; it is used in case where `b' is       */
  /* typically a small value and the result of a*b is known to fit into */
  /* 32 bits.                                                           */
#define FMulDiv( a, b, c )  ( (a) * (b) / (c) )

  /* On the other hand, SMulDiv means `Slow MulDiv', and is used typically */
  /* for clipping computations.  It simply uses the FT_MulDiv() function   */
  /* defined in `ftcalc.h'.                                                */
#define SMulDiv  FT_MulDiv

  /* The rasterizer is a very general purpose component; please leave */
  /* the following redefinitions there (you never know your target    */
  /* environment).                                                    */

#ifndef TRUE
#define TRUE   1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef NULL
#define NULL  (void*)0
#endif

#ifndef SUCCESS
#define SUCCESS  0
#endif

#ifndef FAILURE
#define FAILURE  1
#endif


#define MaxBezier  32   /* The maximum number of stacked Bezier curves. */
                        /* Setting this constant to more than 32 is a   */
                        /* pure waste of space.                         */

#define Pixel_Bits  6   /* fractional bits of *input* coordinates */


  /*************************************************************************/
  /*************************************************************************/
  /**                                                                     **/
  /**  SIMPLE TYPE DECLARATIONS                                           **/
  /**                                                                     **/
  /*************************************************************************/
  /*************************************************************************/

  typedef int             Int;
  typedef unsigned int    UInt;
  typedef short           Short;
  typedef unsigned short  UShort, *PUShort;
  typedef long            Long, *PLong;
  typedef unsigned long   ULong;

  typedef unsigned char   Byte, *PByte;
  typedef char            Bool;

  typedef struct  TPoint_
  {
    Long  x;
    Long  y;

  } TPoint;


  typedef enum  TFlow_
  {
    Flow_None = 0,
    Flow_Up   = 1,
    Flow_Down = -1

  } TFlow;


  /* States of each line, arc, and profile */
  typedef enum  TStates_
  {
    Unknown,
    Ascending,
    Descending,
    Flat

  } TStates;


  typedef struct TProfile_  TProfile;
  typedef TProfile*         PProfile;

  struct  TProfile_
  {
    FT_F26Dot6  X;           /* current coordinate during sweep        */
    PProfile    link;        /* link to next profile - various purpose */
    PLong       offset;      /* start of profile's data in render pool */
    int         flow;        /* Profile orientation: Asc/Descending    */
    long        height;      /* profile's height in scanlines          */
    long        start;       /* profile's starting scanline            */

    unsigned    countL;      /* number of lines to step before this    */
                             /* profile becomes drawable               */

    PProfile    next;        /* next profile in same contour, used     */
                             /* during drop-out control                */
  };

  typedef PProfile   TProfileList;
  typedef PProfile*  PProfileList;


  /* Simple record used to implement a stack of bands, required */
  /* by the sub-banding mechanism                               */
  typedef struct  TBand_
  {
    Short  y_min;   /* band's minimum */
    Short  y_max;   /* band's maximum */

  } TBand;


#define AlignProfileSize \
          ( ( sizeof ( TProfile ) + sizeof ( long ) - 1 ) / sizeof ( long ) )


#ifdef TT_STATIC_RASTER


#define RAS_ARGS       /* void */
#define RAS_ARG        /* void */

#define RAS_VARS       /* void */
#define RAS_VAR        /* void */

#define FT_UNUSED_RASTER  do ; while ( 0 )


#else /* TT_STATIC_RASTER */


#define RAS_ARGS       TRaster_Instance*  raster,
#define RAS_ARG        TRaster_Instance*  raster

#define RAS_VARS       raster,
#define RAS_VAR        raster

#define FT_UNUSED_RASTER  FT_UNUSED( raster )


#endif /* TT_STATIC_RASTER */


  typedef struct TRaster_Instance_  TRaster_Instance;


  /* prototypes used for sweep function dispatch */
  typedef void
  Function_Sweep_Init( RAS_ARGS Short*  min,
                                Short*  max );

  typedef void
  Function_Sweep_Span( RAS_ARGS Short       y,
                                FT_F26Dot6  x1,
                                FT_F26Dot6  x2,
                                PProfile    left,
                                PProfile    right );

  typedef void
  Function_Sweep_Step( RAS_ARG );


  /* NOTE: These operations are only valid on 2's complement processors */

#define FLOOR( x )    ( (x) & -ras.precision )
#define CEILING( x )  ( ( (x) + ras.precision - 1 ) & -ras.precision )
#define TRUNC( x )    ( (signed long)(x) >> ras.precision_bits )
#define FRAC( x )     ( (x) & ( ras.precision - 1 ) )
#define SCALED( x )   ( ( (x) << ras.scale_shift ) - ras.precision_half )

  /* Note that I have moved the location of some fields in the */
  /* structure to ensure that the most used variables are used */
  /* at the top.  Thus, their offset can be coded with less    */
  /* opcodes, and it results in a smaller executable.          */

  struct  TRaster_Instance_
  {
    Int       precision_bits;       /* precision related variables         */
    Int       precision;
    Int       precision_half;
    Long      precision_mask;
    Int       precision_shift;
    Int       precision_step;
    Int       precision_jitter;

    Int       scale_shift;          /* == precision_shift   for bitmaps    */
                                    /* == precision_shift+1 for pixmaps    */

    PLong     buff;                 /* The profiles buffer                 */
    PLong     sizeBuff;             /* Render pool size                    */
    PLong     maxBuff;              /* Profiles buffer size                */
    PLong     top;                  /* Current cursor in buffer            */

    FT_Error  error;

    Int       numTurns;             /* number of Y-turns in outline        */

    TPoint*   arc;                  /* current Bezier arc pointer          */

    UShort    bWidth;               /* target bitmap width                 */
    PByte     bTarget;              /* target bitmap buffer                */
    PByte     gTarget;              /* target pixmap buffer                */

    Long      lastX, lastY, minY, maxY;

    UShort    num_Profs;            /* current number of profiles          */

    Bool      fresh;                /* signals a fresh new profile which   */
                                    /* 'start' field must be completed     */
    Bool      joint;                /* signals that the last arc ended     */
                                    /* exactly on a scanline.  Allows      */
                                    /* removal of doublets                 */
    PProfile  cProfile;             /* current profile                     */
    PProfile  fProfile;             /* head of linked list of profiles     */
    PProfile  gProfile;             /* contour's first profile in case     */
                                    /* of impact                           */

    TStates   state;                /* rendering state                     */

    FT_Bitmap   target;             /* description of target bit/pixmap    */
    FT_Outline  outline;

    Long      traceOfs;             /* current offset in target bitmap     */
    Long      traceG;               /* current offset in target pixmap     */

    Short     traceIncr;            /* sweep's increment in target bitmap  */

    Short     gray_min_x;           /* current min x during gray rendering */
    Short     gray_max_x;           /* current max x during gray rendering */

    /* dispatch variables */

    Function_Sweep_Init*  Proc_Sweep_Init;
    Function_Sweep_Span*  Proc_Sweep_Span;
    Function_Sweep_Span*  Proc_Sweep_Drop;
    Function_Sweep_Step*  Proc_Sweep_Step;

    Byte      dropOutControl;       /* current drop_out control method     */

    Bool      second_pass;          /* indicates wether a horizontal pass  */
                                    /* should be performed to control      */
                                    /* drop-out accurately when calling    */
                                    /* Render_Glyph.  Note that there is   */
                                    /* no horizontal pass during gray      */
                                    /* rendering.                          */

    TPoint    arcs[3 * MaxBezier + 1]; /* The Bezier stack                 */

    TBand     band_stack[16];       /* band stack used for sub-banding     */
    Int       band_top;             /* band stack top                      */

    Int       count_table[256];     /* Look-up table used to quickly count */
                                    /* set bits in a gray 2x2 cell         */

    void*     memory;

#ifdef FT_RASTER_OPTION_ANTI_ALIASING

    Byte      grays[5];             /* Palette of gray levels used for     */
                                    /* render.                             */

    Byte      gray_lines[RASTER_GRAY_LINES];
                                /* Intermediate table used to render the   */
                                /* graylevels pixmaps.                     */
                                /* gray_lines is a buffer holding two      */
                                /* monochrome scanlines                    */

    Short     gray_width;       /* width in bytes of one monochrome        */
                                /* intermediate scanline of gray_lines.    */
                                /* Each gray pixel takes 2 bits long there */

                       /* The gray_lines must hold 2 lines, thus with size */
                       /* in bytes of at least `gray_width*2'.             */

#endif /* FT_RASTER_ANTI_ALIASING */

#if 0
    PByte       flags;              /* current flags table                 */
    PUShort     outs;               /* current outlines table              */
    FT_Vector*  coords;

    UShort      nPoints;            /* number of points in current glyph   */
    Short       nContours;          /* number of contours in current glyph */
#endif

  };


#ifdef FT_CONFIG_OPTION_STATIC_RASTER

  static TRaster_Instance  cur_ras;
#define ras  cur_ras

#else

#define ras  (*raster)

#endif /* FT_CONFIG_OPTION_STATIC_RASTER */


  /*************************************************************************/
  /*************************************************************************/
  /**                                                                     **/
  /**  PROFILES COMPUTATION                                               **/
  /**                                                                     **/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Set_High_Precision                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets precision variables according to param flag.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    High :: Set to True for high precision (typically for ppem < 18),  */
  /*            false otherwise.                                           */
  /*                                                                       */
  static void
  Set_High_Precision( RAS_ARGS Int  High )
  {
    if ( High )
    {
      ras.precision_bits   = 10;
      ras.precision_step   = 128;
      ras.precision_jitter = 24;
    }
    else
    {
      ras.precision_bits   = 6;
      ras.precision_step   = 32;
      ras.precision_jitter = 2;
    }

    FT_TRACE6(( "Set_High_Precision(%s)\n", High ? "true" : "false" ));

    ras.precision       = 1L << ras.precision_bits;
    ras.precision_half  = ras.precision / 2;
    ras.precision_shift = ras.precision_bits - Pixel_Bits;
    ras.precision_mask  = -ras.precision;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    New_Profile                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new profile in the render pool.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    aState :: The state/orientation of the new profile.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*   SUCCESS on success.  FAILURE in case of overflow or of incoherent   */
  /*   profile.                                                            */
  /*                                                                       */
  static Bool
  New_Profile( RAS_ARGS TStates  aState )
  {
    if ( !ras.fProfile )
    {
      ras.cProfile  = (PProfile)ras.top;
      ras.fProfile  = ras.cProfile;
      ras.top      += AlignProfileSize;
    }

    if ( ras.top >= ras.maxBuff )
    {
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    switch ( aState )
    {
    case Ascending:
      ras.cProfile->flow = Flow_Up;
      FT_TRACE6(( "New ascending profile = %lx\n", (long)ras.cProfile ));
      break;

    case Descending:
      ras.cProfile->flow = Flow_Down;
      FT_TRACE6(( "New descending profile = %lx\n", (long)ras.cProfile ));
      break;

    default:
      FT_ERROR(( "New_Profile: invalid profile direction!\n" ));
      ras.error = Raster_Err_Invalid;
      return FAILURE;
    }

    ras.cProfile->start  = 0;
    ras.cProfile->height = 0;
    ras.cProfile->offset = ras.top;
    ras.cProfile->link   = (PProfile)0;
    ras.cProfile->next   = (PProfile)0;

    if ( !ras.gProfile )
      ras.gProfile = ras.cProfile;

    ras.state = aState;
    ras.fresh = TRUE;
    ras.joint = FALSE;

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    End_Profile                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes the current profile.                                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success.  FAILURE in case of overflow or incoherency.   */
  /*                                                                       */
  static Bool
  End_Profile( RAS_ARG )
  {
    Long      h;
    PProfile  oldProfile;


    h = (Long)( ras.top - ras.cProfile->offset );

    if ( h < 0 )
    {
      FT_ERROR(( "End_Profile: negative height encountered!\n" ));
      ras.error = Raster_Err_Neg_Height;
      return FAILURE;
    }

    if ( h > 0 )
    {
      FT_TRACE6(( "Ending profile %lx, start = %ld, height = %ld\n",
                  (long)ras.cProfile, ras.cProfile->start, h ));

      oldProfile           = ras.cProfile;
      ras.cProfile->height = h;
      ras.cProfile         = (PProfile)ras.top;

      ras.top             += AlignProfileSize;

      ras.cProfile->height = 0;
      ras.cProfile->offset = ras.top;
      oldProfile->next     = ras.cProfile;
      ras.num_Profs++;
    }

    if ( ras.top >= ras.maxBuff )
    {
      FT_TRACE1(( "overflow in End_Profile\n" ));
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    ras.joint = FALSE;

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Insert_Y_Turn                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Inserts a salient into the sorted list placed on top of the render */
  /*    pool.                                                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    New y scanline position.                                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success.  FAILURE in case of overflow.                  */
  /*                                                                       */
  static Bool
  Insert_Y_Turn( RAS_ARGS Int  y )
  {
    PLong     y_turns;
    Int       y2, n;


    n       = ras.numTurns - 1;
    y_turns = ras.sizeBuff - ras.numTurns;

    /* look for first y value that is <= */
    while ( n >= 0 && y < y_turns[n] )
      n--;

    /* if it is <, simply insert it, ignore if == */
    if ( n >= 0 && y > y_turns[n] )
      while ( n >= 0 )
      {
        y2 = y_turns[n];
        y_turns[n] = y;
        y = y2;
        n--;
      }

    if ( n < 0 )
    {
      if ( ras.maxBuff <= ras.top )
      {
        ras.error = Raster_Err_Overflow;
        return FAILURE;
      }
      ras.maxBuff--;
      ras.numTurns++;
      ras.sizeBuff[-ras.numTurns] = y;
    }

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Finalize_Profile_Table                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Adjusts all links in the profiles list.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success.  FAILURE in case of overflow.                  */
  /*                                                                       */
  static Bool
  Finalize_Profile_Table( RAS_ARG )
  {
    Int       bottom, top;
    UShort    n;
    PProfile  p;


    n = ras.num_Profs;

    if ( n > 1 )
    {
      p = ras.fProfile;
      while ( n > 0 )
      {
        if ( n > 1 )
          p->link = (PProfile)( p->offset + p->height );
        else
          p->link = NULL;

        switch ( p->flow )
        {
        case Flow_Down:
          bottom     = p->start - p->height+1;
          top        = p->start;
          p->start   = bottom;
          p->offset += p->height - 1;
          break;

        case Flow_Up:
        default:
          bottom = p->start;
          top    = p->start + p->height - 1;
        }

        if ( Insert_Y_Turn( RAS_VARS bottom )   ||
             Insert_Y_Turn( RAS_VARS top + 1 )  )
          return FAILURE;

        p = p->link;
        n--;
      }
    }
    else
      ras.fProfile = NULL;

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Split_Conic                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Subdivides one conic Bezier into two joint sub-arcs in the Bezier  */
  /*    stack.                                                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    None (subdivided Bezier is taken from the top of the stack).       */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This routine is the `beef' of this component.  It is  _the_ inner  */
  /*    loop that should be optimized to hell to get the best performance. */
  /*                                                                       */
  static void
  Split_Conic( TPoint*  base )
  {
    Long  a, b;


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

    /* hand optimized.  gcc doesn't seem to be too good at common      */
    /* expression substitution and instruction scheduling ;-)          */
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Split_Cubic                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Subdivides a third-order Bezier arc into two joint sub-arcs in the */
  /*    Bezier stack.                                                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This routine is the `beef' of the component.  It is one of _the_   */
  /*    inner loops that should be optimized like hell to get the best     */
  /*    performance.                                                       */
  /*                                                                       */
  static void
  Split_Cubic( TPoint*  base )
  {
    Long  a, b, c, d;


    base[6].x = base[3].x;
    c = base[1].x;
    d = base[2].x;
    base[1].x = a = ( base[0].x + c + 1 ) >> 1;
    base[5].x = b = ( base[3].x + d + 1 ) >> 1;
    c = ( c + d + 1 ) >> 1;
    base[2].x = a = ( a + c + 1 ) >> 1;
    base[4].x = b = ( b + c + 1 ) >> 1;
    base[3].x = ( a + b + 1 ) >> 1;

    base[6].y = base[3].y;
    c = base[1].y;
    d = base[2].y;
    base[1].y = a = ( base[0].y + c + 1 ) >> 1;
    base[5].y = b = ( base[3].y + d + 1 ) >> 1;
    c = ( c + d + 1 ) >> 1;
    base[2].y = a = ( a + c + 1 ) >> 1;
    base[4].y = b = ( b + c + 1 ) >> 1;
    base[3].y = ( a + b + 1 ) >> 1;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Line_Up                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the x-coordinates of an ascending line segment and stores */
  /*    them in the render pool.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x1   :: The x-coordinate of the segment's start point.             */
  /*                                                                       */
  /*    y1   :: The y-coordinate of the segment's start point.             */
  /*                                                                       */
  /*    x2   :: The x-coordinate of the segment's end point.               */
  /*                                                                       */
  /*    y2   :: The y-coordinate of the segment's end point.               */
  /*                                                                       */
  /*    miny :: A lower vertical clipping bound value.                     */
  /*                                                                       */
  /*    maxy :: An upper vertical clipping bound value.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success, FAILURE on render pool overflow.               */
  /*                                                                       */
  static Bool
  Line_Up( RAS_ARGS Long  x1,
                    Long  y1,
                    Long  x2,
                    Long  y2,
                    Long  miny,
                    Long  maxy )
  {
    Long   Dx, Dy;
    Int    e1, e2, f1, f2, size;     /* XXX: is `Short' sufficient? */
    Long   Ix, Rx, Ax;

    PLong  top;


    Dx = x2 - x1;
    Dy = y2 - y1;

    if ( Dy <= 0 || y2 < miny || y1 > maxy )
      return SUCCESS;

    if ( y1 < miny )
    {
      /* Take care: miny-y1 can be a very large value; we use     */
      /*            a slow MulDiv function to avoid clipping bugs */
      x1 += SMulDiv( Dx, miny - y1, Dy );
      e1  = TRUNC( miny );
      f1  = 0;
    }
    else
    {
      e1 = TRUNC( y1 );
      f1 = FRAC( y1 );
    }

    if ( y2 > maxy )
    {
      /* x2 += FMulDiv( Dx, maxy - y2, Dy );  UNNECESSARY */
      e2  = TRUNC( maxy );
      f2  = 0;
    }
    else
    {
      e2 = TRUNC( y2 );
      f2 = FRAC( y2 );
    }

    if ( f1 > 0 )
    {
      if ( e1 == e2 )
        return SUCCESS;
      else
      {
        x1 += FMulDiv( Dx, ras.precision - f1, Dy );
        e1 += 1;
      }
    }
    else
      if ( ras.joint )
      {
        ras.top--;
        ras.joint = FALSE;
      }

    ras.joint = (char)( f2 == 0 );

    if ( ras.fresh )
    {
      ras.cProfile->start = e1;
      ras.fresh           = FALSE;
    }

    size = e2 - e1 + 1;
    if ( ras.top + size >= ras.maxBuff )
    {
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    if ( Dx > 0 )
    {
      Ix = ( ras.precision * Dx ) / Dy;
      Rx = ( ras.precision * Dx ) % Dy;
      Dx = 1;
    }
    else
    {
      Ix = -( ( ras.precision * -Dx ) / Dy );
      Rx =    ( ras.precision * -Dx ) % Dy;
      Dx = -1;
    }

    Ax  = -Dy;
    top = ras.top;

    while ( size > 0 )
    {
      *top++ = x1;

      x1 += Ix;
      Ax += Rx;
      if ( Ax >= 0 )
      {
        Ax -= Dy;
        x1 += Dx;
      }
      size--;
    }

    ras.top = top;
    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Line_Down                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the x-coordinates of an descending line segment and       */
  /*    stores them in the render pool.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    x1   :: The x-coordinate of the segment's start point.             */
  /*                                                                       */
  /*    y1   :: The y-coordinate of the segment's start point.             */
  /*                                                                       */
  /*    x2   :: The x-coordinate of the segment's end point.               */
  /*                                                                       */
  /*    y2   :: The y-coordinate of the segment's end point.               */
  /*                                                                       */
  /*    miny :: A lower vertical clipping bound value.                     */
  /*                                                                       */
  /*    maxy :: An upper vertical clipping bound value.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success, FAILURE on render pool overflow.               */
  /*                                                                       */
  static Bool
  Line_Down( RAS_ARGS Long  x1,
                      Long  y1,
                      Long  x2,
                      Long  y2,
                      Long  miny,
                      Long  maxy )
  {
    Bool  result, fresh;


    fresh  = ras.fresh;

    result = Line_Up( RAS_VARS x1, -y1, x2, -y2, -maxy, -miny );

    if ( fresh && !ras.fresh )
      ras.cProfile->start = -ras.cProfile->start;

    return result;
  }


  /* A function type describing the functions used to split Bezier arcs */
  typedef void  (*TSplitter)( TPoint*  base );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Bezier_Up                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the x-coordinates of an ascending Bezier arc and stores   */
  /*    them in the render pool.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    degree   :: The degree of the Bezier arc (either 2 or 3).          */
  /*                                                                       */
  /*    splitter :: The function to split Bezier arcs.                     */
  /*                                                                       */
  /*    miny     :: A lower vertical clipping bound value.                 */
  /*                                                                       */
  /*    maxy     :: An upper vertical clipping bound value.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success, FAILURE on render pool overflow.               */
  /*                                                                       */
  static Bool
  Bezier_Up( RAS_ARGS Int        degree,
                      TSplitter  splitter,
                      Long       miny,
                      Long       maxy )
  {
    Long   y1, y2, e, e2, e0;
    Short  f1;

    TPoint*  arc;
    TPoint*  start_arc;

    PLong top;


    arc = ras.arc;
    y1  = arc[degree].y;
    y2  = arc[0].y;
    top = ras.top;

    if ( y2 < miny || y1 > maxy )
      goto Fin;

    e2 = FLOOR( y2 );

    if ( e2 > maxy )
      e2 = maxy;

    e0 = miny;

    if ( y1 < miny )
      e = miny;
    else
    {
      e  = CEILING( y1 );
      f1 = (Short)( FRAC( y1 ) );
      e0 = e;

      if ( f1 == 0 )
      {
        if ( ras.joint )
        {
          top--;
          ras.joint = FALSE;
        }

        *top++ = arc[degree].x;

        e += ras.precision;
      }
    }

    if ( ras.fresh )
    {
      ras.cProfile->start = TRUNC( e0 );
      ras.fresh = FALSE;
    }

    if ( e2 < e )
      goto Fin;

    if ( ( top + TRUNC( e2 - e ) + 1 ) >= ras.maxBuff )
    {
      ras.top   = top;
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    start_arc = arc;

    while ( arc >= start_arc && e <= e2 )
    {
      ras.joint = FALSE;

      y2 = arc[0].y;

      if ( y2 > e )
      {
        y1 = arc[degree].y;
        if ( y2 - y1 >= ras.precision_step )
        {
          splitter( arc );
          arc += degree;
        }
        else
        {
          *top++ = arc[degree].x + FMulDiv( arc[0].x-arc[degree].x,
                                            e - y1, y2 - y1 );
          arc -= degree;
          e   += ras.precision;
        }
      }
      else
      {
        if ( y2 == e )
        {
          ras.joint  = TRUE;
          *top++     = arc[0].x;

          e += ras.precision;
        }
        arc -= degree;
      }
    }

  Fin:
    ras.top  = top;
    ras.arc -= degree;
    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Bezier_Down                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Computes the x-coordinates of an descending Bezier arc and stores  */
  /*    them in the render pool.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    degree   :: The degree of the Bezier arc (either 2 or 3).          */
  /*                                                                       */
  /*    splitter :: The function to split Bezier arcs.                     */
  /*                                                                       */
  /*    miny     :: A lower vertical clipping bound value.                 */
  /*                                                                       */
  /*    maxy     :: An upper vertical clipping bound value.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success, FAILURE on render pool overflow.               */
  /*                                                                       */
  static Bool
  Bezier_Down( RAS_ARGS Int        degree,
                        TSplitter  splitter,
                        Long       miny,
                        Long       maxy )
  {
    TPoint*  arc = ras.arc;
    Bool     result, fresh;


    arc[0].y = -arc[0].y;
    arc[1].y = -arc[1].y;
    arc[2].y = -arc[2].y;
    if ( degree > 2 )
      arc[3].y = -arc[3].y;

    fresh = ras.fresh;

    result = Bezier_Up( RAS_VARS degree, splitter, -maxy, -miny );

    if ( fresh && !ras.fresh )
      ras.cProfile->start = -ras.cProfile->start;

    arc[0].y = -arc[0].y;
    return result;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Line_To                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Injects a new line segment and adjusts Profiles list.              */
  /*                                                                       */
  /* <Input>                                                               */
  /*   x :: The x-coordinate of the segment's end point (its start point   */
  /*        is stored in `LastX').                                         */
  /*                                                                       */
  /*   y :: The y-coordinate of the segment's end point (its start point   */
  /*        is stored in `LastY').                                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*   SUCCESS on success, FAILURE on render pool overflow or incorrect    */
  /*   profile.                                                            */
  /*                                                                       */
  static Bool
  Line_To( RAS_ARGS Long  x,
                    Long  y )
  {
    /* First, detect a change of direction */

    switch ( ras.state )
    {
    case Unknown:
      if ( y > ras.lastY )
      {
        if ( New_Profile( RAS_VARS Ascending ) )
          return FAILURE;
      }
      else
      {
        if ( y < ras.lastY )
          if ( New_Profile( RAS_VARS Descending ) )
            return FAILURE;
      }
      break;

    case Ascending:
      if ( y < ras.lastY )
      {
        if ( End_Profile( RAS_VAR )             ||
             New_Profile( RAS_VARS Descending ) )
          return FAILURE;
      }
      break;

    case Descending:
      if ( y > ras.lastY )
      {
        if ( End_Profile( RAS_VAR )            ||
             New_Profile( RAS_VARS Ascending ) )
          return FAILURE;
      }
      break;

    default:
      ;
    }

    /* Then compute the lines */

    switch ( ras.state )
    {
    case Ascending:
      if ( Line_Up( RAS_VARS ras.lastX, ras.lastY,
                    x, y, ras.minY, ras.maxY ) )
        return FAILURE;
      break;

    case Descending:
      if ( Line_Down( RAS_VARS ras.lastX, ras.lastY,
                      x, y, ras.minY, ras.maxY ) )
        return FAILURE;
      break;

    default:
      ;
    }

    ras.lastX = x;
    ras.lastY = y;

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Conic_To                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Injects a new conic arc and adjusts the profile list.              */
  /*                                                                       */
  /* <Input>                                                               */
  /*   cx :: The x-coordinate of the arc's new control point.              */
  /*                                                                       */
  /*   cy :: The y-coordinate of the arc's new control point.              */
  /*                                                                       */
  /*   x  :: The x-coordinate of the arc's end point (its start point is   */
  /*         stored in `LastX').                                           */
  /*                                                                       */
  /*   y  :: The y-coordinate of the arc's end point (its start point is   */
  /*         stored in `LastY').                                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*   SUCCESS on success, FAILURE on render pool overflow or incorrect    */
  /*   profile.                                                            */
  /*                                                                       */
  static Bool
  Conic_To( RAS_ARGS Long  cx,
                     Long  cy,
                     Long  x,
                     Long  y )
  {
    Long     y1, y2, y3, x3, ymin, ymax;
    TStates  state_bez;


    ras.arc      = ras.arcs;
    ras.arc[2].x = ras.lastX;
    ras.arc[2].y = ras.lastY;
    ras.arc[1].x = cx; ras.arc[1].y = cy;
    ras.arc[0].x = x;  ras.arc[0].y = y;

    do
    {
      y1 = ras.arc[2].y;
      y2 = ras.arc[1].y;
      y3 = ras.arc[0].y;
      x3 = ras.arc[0].x;

      /* first, categorize the Bezier arc */

      if ( y1 <= y3 )
      {
        ymin = y1;
        ymax = y3;
      }
      else
      {
        ymin = y3;
        ymax = y1;
      }

      if ( y2 < ymin || y2 > ymax )
      {
        /* this arc has no given direction, split it! */
        Split_Conic( ras.arc );
        ras.arc += 2;
      }
      else if ( y1 == y3 )
      {
        /* this arc is flat, ignore it and pop it from the Bezier stack */
        ras.arc -= 2;
      }
      else
      {
        /* the arc is y-monotonous, either ascending or descending */
        /* detect a change of direction                            */
        state_bez = y1 < y3 ? Ascending : Descending;
        if ( ras.state != state_bez )
        {
          /* finalize current profile if any */
          if ( ras.state != Unknown   &&
               End_Profile( RAS_VAR ) )
            goto Fail;

          /* create a new profile */
          if ( New_Profile( RAS_VARS state_bez ) )
            goto Fail;
        }

        /* now call the appropriate routine */
        if ( state_bez == Ascending )
        {
          if ( Bezier_Up( RAS_VARS 2, Split_Conic, ras.minY, ras.maxY ) )
            goto Fail;
        }
        else
          if ( Bezier_Down( RAS_VARS 2, Split_Conic, ras.minY, ras.maxY ) )
            goto Fail;
      }

    } while ( ras.arc >= ras.arcs );

    ras.lastX = x3;
    ras.lastY = y3;

    return SUCCESS;

  Fail:
    return FAILURE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Cubic_To                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Injects a new cubic arc and adjusts the profile list.              */
  /*                                                                       */
  /* <Input>                                                               */
  /*   cx1 :: The x-coordinate of the arc's first new control point.       */
  /*                                                                       */
  /*   cy1 :: The y-coordinate of the arc's first new control point.       */
  /*                                                                       */
  /*   cx2 :: The x-coordinate of the arc's second new control point.      */
  /*                                                                       */
  /*   cy2 :: The y-coordinate of the arc's second new control point.      */
  /*                                                                       */
  /*   x   :: The x-coordinate of the arc's end point (its start point is  */
  /*          stored in `LastX').                                          */
  /*                                                                       */
  /*   y   :: The y-coordinate of the arc's end point (its start point is  */
  /*          stored in `LastY').                                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*   SUCCESS on success, FAILURE on render pool overflow or incorrect    */
  /*   profile.                                                            */
  /*                                                                       */
  static Bool
  Cubic_To( RAS_ARGS Long  cx1,
                     Long  cy1,
                     Long  cx2,
                     Long  cy2,
                     Long  x,
                     Long  y )
  {
    Long     y1, y2, y3, y4, x4, ymin1, ymax1, ymin2, ymax2;
    TStates  state_bez;


    ras.arc      = ras.arcs;
    ras.arc[3].x = ras.lastX;
    ras.arc[3].y = ras.lastY;
    ras.arc[2].x = cx1; ras.arc[2].y = cy1;
    ras.arc[1].x = cx2; ras.arc[1].y = cy2;
    ras.arc[0].x = x;   ras.arc[0].y = y;

    do
    {
      y1 = ras.arc[3].y;
      y2 = ras.arc[2].y;
      y3 = ras.arc[1].y;
      y4 = ras.arc[0].y;
      x4 = ras.arc[0].x;

      /* first, categorize the Bezier arc */

      if ( y1 <= y4 )
      {
        ymin1 = y1;
        ymax1 = y4;
      }
      else
      {
        ymin1 = y4;
        ymax1 = y1;
      }

      if ( y2 <= y3 )
      {
        ymin2 = y2;
        ymax2 = y3;
      }
      else
      {
        ymin2 = y3;
        ymax2 = y2;
      }

      if ( ymin2 < ymin1 || ymax2 > ymax1 )
      {
        /* this arc has no given direction, split it! */
        Split_Cubic( ras.arc );
        ras.arc += 3;
      }
      else if ( y1 == y4 )
      {
        /* this arc is flat, ignore it and pop it from the Bezier stack */
        ras.arc -= 3;
      }
      else
      {
        state_bez = ( y1 <= y4 ) ? Ascending : Descending;

        /* detect a change of direction */
        if ( ras.state != state_bez )
        {
          if ( ras.state != Unknown   &&
               End_Profile( RAS_VAR ) )
            goto Fail;

          if ( New_Profile( RAS_VARS state_bez ) )
            goto Fail;
        }

        /* compute intersections */
        if ( state_bez == Ascending )
        {
          if ( Bezier_Up( RAS_VARS 3, Split_Cubic, ras.minY, ras.maxY ) )
            goto Fail;
        }
        else
          if ( Bezier_Down( RAS_VARS 3, Split_Cubic, ras.minY, ras.maxY ) )
            goto Fail;
      }

    } while ( ras.arc >= ras.arcs );

    ras.lastX = x4;
    ras.lastY = y4;

    return SUCCESS;

  Fail:
    return FAILURE;
  }


#undef  SWAP_
#define SWAP_( x, y )  do                \
                       {                 \
                         Long  swap = x; \
                                         \
                                         \
                         x = y;          \
                         y = swap;       \
                       } while ( 0 )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Decompose_Curve                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Scans the outline arays in order to emit individual segments and   */
  /*    Beziers by calling Line_To() and Bezier_To().  It handles all      */
  /*    weird cases, like when the first point is off the curve, or when   */
  /*    there are simply no `on' points in the contour!                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    first   :: The index of the first point in the contour.            */
  /*                                                                       */
  /*    last    :: The index of the last point in the contour.             */
  /*                                                                       */
  /*    flipped :: If set, flip the direction of the curve.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success, FAILURE on error.                              */
  /*                                                                       */
  static Bool
  Decompose_Curve( RAS_ARGS UShort  first,
                            UShort  last,
                            int     flipped )
  {
    FT_Vector   v_last;
    FT_Vector   v_control;
    FT_Vector   v_start;

    FT_Vector*  points;
    FT_Vector*  point;
    FT_Vector*  limit;
    char*       tags;

    unsigned    tag;       /* current point's state           */


    points = ras.outline.points;
    limit  = points + last;

    v_start.x = SCALED( points[first].x );
    v_start.y = SCALED( points[first].y );
    v_last.x  = SCALED( points[last].x );
    v_last.y  = SCALED( points[last].y );

    if ( flipped )
    {
      SWAP_( v_start.x, v_start.y );
      SWAP_( v_last.x, v_last.y );
    }

    v_control = v_start;

    point = points + first;
    tags  = ras.outline.tags  + first;
    tag   = FT_CURVE_TAG( tags[0] );

    /* A contour cannot start with a cubic control point! */
    if ( tag == FT_Curve_Tag_Cubic )
      goto Invalid_Outline;

    /* check first point to determine origin */
    if ( tag == FT_Curve_Tag_Conic )
    {
      /* first point is conic control.  Yes, this happens. */
      if ( FT_CURVE_TAG( ras.outline.tags[last] ) == FT_Curve_Tag_On )
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

    ras.lastX = v_start.x;
    ras.lastY = v_start.y;

    while ( point < limit )
    {
      point++;
      tags++;

      tag = FT_CURVE_TAG( tags[0] );

      switch ( tag )
      {
      case FT_Curve_Tag_On:  /* emit a single line_to */
        {
          Long  x, y;


          x = SCALED( point->x );
          y = SCALED( point->y );
          if ( flipped )
            SWAP_( x, y );

          if ( Line_To( RAS_VARS x, y ) )
            goto Fail;
          continue;
        }

      case FT_Curve_Tag_Conic:  /* consume conic arcs */
        v_control.x = SCALED( point[0].x );
        v_control.y = SCALED( point[0].y );

        if ( flipped )
          SWAP_( v_control.x, v_control.y );

      Do_Conic:
        if ( point < limit )
        {
          FT_Vector  v_middle;
          Long       x, y;


          point++;
          tags++;
          tag = FT_CURVE_TAG( tags[0] );

          x = SCALED( point[0].x );
          y = SCALED( point[0].y );

          if ( flipped )
            SWAP_( x, y );

          if ( tag == FT_Curve_Tag_On )
          {
            if ( Conic_To( RAS_VARS v_control.x, v_control.y, x, y ) )
              goto Fail;
            continue;
          }

          if ( tag != FT_Curve_Tag_Conic )
            goto Invalid_Outline;

          v_middle.x = ( v_control.x + x ) / 2;
          v_middle.y = ( v_control.y + y ) / 2;

          if ( Conic_To( RAS_VARS v_control.x, v_control.y,
                                  v_middle.x,  v_middle.y ) )
            goto Fail;

          v_control.x = x;
          v_control.y = y;

          goto Do_Conic;
        }

        if ( Conic_To( RAS_VARS v_control.x, v_control.y,
                                v_start.x,   v_start.y ) )
          goto Fail;

        goto Close;

      default:  /* FT_Curve_Tag_Cubic */
        {
          Long  x1, y1, x2, y2, x3, y3;


          if ( point + 1 > limit                             ||
               FT_CURVE_TAG( tags[1] ) != FT_Curve_Tag_Cubic )
            goto Invalid_Outline;

          point += 2;
          tags  += 2;

          x1 = SCALED( point[-2].x );
          y1 = SCALED( point[-2].y );
          x2 = SCALED( point[-1].x );
          y2 = SCALED( point[-1].y );
          x3 = SCALED( point[ 0].x );
          y3 = SCALED( point[ 0].y );

          if ( flipped )
          {
            SWAP_( x1, y1 );
            SWAP_( x2, y2 );
            SWAP_( x3, y3 );
          }

          if ( point <= limit )
          {
            if ( Cubic_To( RAS_VARS x1, y1, x2, y2, x3, y3 ) )
              goto Fail;
            continue;
          }

          if ( Cubic_To( RAS_VARS x1, y1, x2, y2, v_start.x, v_start.y ) )
            goto Fail;
          goto Close;
        }
      }
    }

    /* close the contour with a line segment */
    if ( Line_To( RAS_VARS v_start.x, v_start.y ) )
      goto Fail;

  Close:
    return SUCCESS;

  Invalid_Outline:
    ras.error = Raster_Err_Invalid;

  Fail:
    return FAILURE;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Convert_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts a glyph into a series of segments and arcs and makes a    */
  /*    profiles list with them.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    flipped :: If set, flip the direction of curve.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    SUCCESS on success, FAILURE if any error was encountered during    */
  /*    rendering.                                                         */
  /*                                                                       */
  static Bool
  Convert_Glyph( RAS_ARGS int  flipped )
  {
    int       i;
    unsigned  start;

    PProfile  lastProfile;


    ras.fProfile = NULL;
    ras.joint    = FALSE;
    ras.fresh    = FALSE;

    ras.maxBuff  = ras.sizeBuff - AlignProfileSize;

    ras.numTurns = 0;

    ras.cProfile         = (PProfile)ras.top;
    ras.cProfile->offset = ras.top;
    ras.num_Profs        = 0;

    start = 0;

    for ( i = 0; i < ras.outline.n_contours; i++ )
    {
      ras.state    = Unknown;
      ras.gProfile = NULL;

      if ( Decompose_Curve( RAS_VARS (unsigned short)start,
                            ras.outline.contours[i],
                            flipped ) )
        return FAILURE;

      start = ras.outline.contours[i] + 1;

      /* We must now see whether the extreme arcs join or not */
      if ( FRAC( ras.lastY ) == 0 &&
           ras.lastY >= ras.minY  &&
           ras.lastY <= ras.maxY  )
        if ( ras.gProfile && ras.gProfile->flow == ras.cProfile->flow )
          ras.top--;
        /* Note that ras.gProfile can be nil if the contour was too small */
        /* to be drawn.                                                   */

      lastProfile = ras.cProfile;
      if ( End_Profile( RAS_VAR ) )
        return FAILURE;

      /* close the `next profile in contour' linked list */
      if ( ras.gProfile )
        lastProfile->next = ras.gProfile;
    }

    if ( Finalize_Profile_Table( RAS_VAR ) )
      return FAILURE;

    return (Bool)( ras.top < ras.maxBuff ? SUCCESS : FAILURE );
  }


  /*************************************************************************/
  /*************************************************************************/
  /**                                                                     **/
  /**  SCAN-LINE SWEEPS AND DRAWING                                       **/
  /**                                                                     **/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /*  Init_Linked                                                          */
  /*                                                                       */
  /*    Initializes an empty linked list.                                  */
  /*                                                                       */
  static void
  Init_Linked( TProfileList*  l )
  {
    *l = NULL;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  InsNew                                                               */
  /*                                                                       */
  /*    Inserts a new profile in a linked list.                            */
  /*                                                                       */
  static void
  InsNew( PProfileList  list,
          PProfile      profile )
  {
    PProfile  *old, current;
    Long       x;


    old     = list;
    current = *old;
    x       = profile->X;

    while ( current )
    {
      if ( x < current->X )
        break;
      old     = &current->link;
      current = *old;
    }

    profile->link = current;
    *old          = profile;
  }


  /*************************************************************************/
  /*                                                                       */
  /*  DelOld                                                               */
  /*                                                                       */
  /*    Removes an old profile from a linked list.                         */
  /*                                                                       */
  static void
  DelOld( PProfileList  list,
          PProfile      profile )
  {
    PProfile  *old, current;


    old     = list;
    current = *old;

    while ( current )
    {
      if ( current == profile )
      {
        *old = current->link;
        return;
      }

      old     = &current->link;
      current = *old;
    }

    /* we should never get there, unless the profile was not part of */
    /* the list.                                                     */
  }


  /*************************************************************************/
  /*                                                                       */
  /*  Sort                                                                 */
  /*                                                                       */
  /*    Sorts a trace list.  In 95%, the list is already sorted.  We need  */
  /*    an algorithm which is fast in this case.  Bubble sort is enough    */
  /*    and simple.                                                        */
  /*                                                                       */
  static void
  Sort( PProfileList  list )
  {
    PProfile  *old, current, next;


    /* First, set the new X coordinate of each profile */
    current = *list;
    while ( current )
    {
      current->X       = *current->offset;
      current->offset += current->flow;
      current->height--;
      current = current->link;
    }

    /* Then sort them */
    old     = list;
    current = *old;

    if ( !current )
      return;

    next = current->link;

    while ( next )
    {
      if ( current->X <= next->X )
      {
        old     = &current->link;
        current = *old;

        if ( !current )
          return;
      }
      else
      {
        *old          = next;
        current->link = next->link;
        next->link    = current;

        old     = list;
        current = *old;
      }

      next = current->link;
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /*  Vertical Sweep Procedure Set                                         */
  /*                                                                       */
  /*  These four routines are used during the vertical black/white sweep   */
  /*  phase by the generic Draw_Sweep() function.                          */
  /*                                                                       */
  /*************************************************************************/

  static void
  Vertical_Sweep_Init( RAS_ARGS Short*  min,
                                Short*  max )
  {
    Long  pitch = ras.target.pitch;

    FT_UNUSED( max );


    ras.traceIncr = (Short)-pitch;
    ras.traceOfs  = -*min * pitch;
    if ( pitch > 0 )
      ras.traceOfs += ( ras.target.rows - 1 ) * pitch;

    ras.gray_min_x = 0;
    ras.gray_max_x = 0;
  }


  static void
  Vertical_Sweep_Span( RAS_ARGS Short       y,
                                FT_F26Dot6  x1,
                                FT_F26Dot6  x2,
                                PProfile    left,
                                PProfile    right )
  {
    Long   e1, e2;
    int    c1, c2;
    Byte   f1, f2;
    Byte*  target;

    FT_UNUSED( y );
    FT_UNUSED( left );
    FT_UNUSED( right );


    /* Drop-out control */

    e1 = TRUNC( CEILING( x1 ) );

    if ( x2 - x1 - ras.precision <= ras.precision_jitter )
      e2 = e1;
    else
      e2 = TRUNC( FLOOR( x2 ) );

    if ( e2 >= 0 && e1 < ras.bWidth )
    {
      if ( e1 < 0 )
        e1 = 0;
      if ( e2 >= ras.bWidth )
        e2 = ras.bWidth - 1;

      c1 = (Short)( e1 >> 3 );
      c2 = (Short)( e2 >> 3 );

      f1 = (Byte)  ( 0xFF >> ( e1 & 7 ) );
      f2 = (Byte) ~( 0x7F >> ( e2 & 7 ) );

      if ( ras.gray_min_x > c1 ) ras.gray_min_x = (short)c1;
      if ( ras.gray_max_x < c2 ) ras.gray_max_x = (short)c2;

      target = ras.bTarget + ras.traceOfs + c1;
      c2 -= c1;

      if ( c2 > 0 )
      {
        target[0] |= f1;

        /* memset() is slower than the following code on many platforms. */
        /* This is due to the fact that, in the vast majority of cases,  */
        /* the span length in bytes is relatively small.                 */
        c2--;
        while ( c2 > 0 )
        {
          *(++target) = 0xFF;
          c2--;
        }
        target[1] |= f2;
      }
      else
        *target |= ( f1 & f2 );
    }
  }


  static void
  Vertical_Sweep_Drop( RAS_ARGS Short       y,
                                FT_F26Dot6  x1,
                                FT_F26Dot6  x2,
                                PProfile    left,
                                PProfile    right )
  {
    Long   e1, e2;
    Short  c1, f1;


    /* Drop-out control */

    e1 = CEILING( x1 );
    e2 = FLOOR  ( x2 );

    if ( e1 > e2 )
    {
      if ( e1 == e2 + ras.precision )
      {
        switch ( ras.dropOutControl )
        {
        case 1:
          e1 = e2;
          break;

        case 4:
          e1 = CEILING( (x1 + x2 + 1) / 2 );
          break;

        case 2:
        case 5:
          /* Drop-out Control Rule #4 */

          /* The spec is not very clear regarding rule #4.  It      */
          /* presents a method that is way too costly to implement  */
          /* while the general idea seems to get rid of `stubs'.    */
          /*                                                        */
          /* Here, we only get rid of stubs recognized if:          */
          /*                                                        */
          /*  upper stub:                                           */
          /*                                                        */
          /*   - P_Left and P_Right are in the same contour         */
          /*   - P_Right is the successor of P_Left in that contour */
          /*   - y is the top of P_Left and P_Right                 */
          /*                                                        */
          /*  lower stub:                                           */
          /*                                                        */
          /*   - P_Left and P_Right are in the same contour         */
          /*   - P_Left is the successor of P_Right in that contour */
          /*   - y is the bottom of P_Left                          */
          /*                                                        */

          /* FIXXXME: uncommenting this line solves the disappearing */
          /*          bit problem in the `7' of verdana 10pts, but   */
          /*          makes a new one in the `C' of arial 14pts      */

#if 0
          if ( x2 - x1 < ras.precision_half )
#endif
          {
            /* upper stub test */
            if ( left->next == right && left->height <= 0 )
              return;

            /* lower stub test */
            if ( right->next == left && left->start == y )
              return;
          }

          /* check that the rightmost pixel isn't set */

          e1 = TRUNC( e1 );

          c1 = (Short)( e1 >> 3 );
          f1 = (Short)( e1 &  7 );

          if ( e1 >= 0 && e1 < ras.bWidth                      &&
               ras.bTarget[ras.traceOfs + c1] & ( 0x80 >> f1 ) )
            return;

          if ( ras.dropOutControl == 2 )
            e1 = e2;
          else
            e1 = CEILING( ( x1 + x2 + 1 ) / 2 );

          break;

        default:
          return;  /* unsupported mode */
        }
      }
      else
        return;
    }

    e1 = TRUNC( e1 );

    if ( e1 >= 0 && e1 < ras.bWidth )
    {
      c1 = (Short)( e1 >> 3 );
      f1 = (Short)( e1 & 7 );

      if ( ras.gray_min_x > c1 ) ras.gray_min_x = c1;
      if ( ras.gray_max_x < c1 ) ras.gray_max_x = c1;

      ras.bTarget[ras.traceOfs + c1] |= (char)( 0x80 >> f1 );
    }
  }


  static void
  Vertical_Sweep_Step( RAS_ARG )
  {
    ras.traceOfs += ras.traceIncr;
  }


  /***********************************************************************/
  /*                                                                     */
  /*  Horizontal Sweep Procedure Set                                     */
  /*                                                                     */
  /*  These four routines are used during the horizontal black/white     */
  /*  sweep phase by the generic Draw_Sweep() function.                  */
  /*                                                                     */
  /***********************************************************************/

  static void
  Horizontal_Sweep_Init( RAS_ARGS Short*  min,
                                  Short*  max )
  {
    /* nothing, really */
    FT_UNUSED( raster );
    FT_UNUSED( min );
    FT_UNUSED( max );
  }


  static void
  Horizontal_Sweep_Span( RAS_ARGS Short       y,
                                  FT_F26Dot6  x1,
                                  FT_F26Dot6  x2,
                                  PProfile    left,
                                  PProfile    right )
  {
    Long   e1, e2;
    PByte  bits;
    Byte   f1;

    FT_UNUSED( left );
    FT_UNUSED( right );


    if ( x2 - x1 < ras.precision )
    {
      e1 = CEILING( x1 );
      e2 = FLOOR  ( x2 );

      if ( e1 == e2 )
      {
        bits = ras.bTarget + ( y >> 3 );
        f1   = (Byte)( 0x80 >> ( y & 7 ) );

        e1 = TRUNC( e1 );

        if ( e1 >= 0 && e1 < ras.target.rows )
        {
          PByte  p;


          p = bits - e1*ras.target.pitch;
          if ( ras.target.pitch > 0 )
            p += ( ras.target.rows - 1 ) * ras.target.pitch;

          p[0] |= f1;
        }
      }
    }
  }


  static void
  Horizontal_Sweep_Drop( RAS_ARGS Short       y,
                                  FT_F26Dot6  x1,
                                  FT_F26Dot6  x2,
                                  PProfile    left,
                                  PProfile    right )
  {
    Long   e1, e2;
    PByte  bits;
    Byte   f1;


    /* During the horizontal sweep, we only take care of drop-outs */

    e1 = CEILING( x1 );
    e2 = FLOOR  ( x2 );

    if ( e1 > e2 )
    {
      if ( e1 == e2 + ras.precision )
      {
        switch ( ras.dropOutControl )
        {
        case 1:
          e1 = e2;
          break;

        case 4:
          e1 = CEILING( ( x1 + x2 + 1 ) / 2 );
          break;

        case 2:
        case 5:

          /* Drop-out Control Rule #4 */

          /* The spec is not very clear regarding rule #4.  It      */
          /* presents a method that is way too costly to implement  */
          /* while the general idea seems to get rid of `stubs'.    */
          /*                                                        */

          /* rightmost stub test */
          if ( left->next == right && left->height <= 0 )
            return;

          /* leftmost stub test */
          if ( right->next == left && left->start == y )
            return;

          /* check that the rightmost pixel isn't set */

          e1 = TRUNC( e1 );

          bits = ras.bTarget + ( y >> 3 );
          f1   = (Byte)( 0x80 >> ( y & 7 ) );

          bits -= e1 * ras.target.pitch;
          if ( ras.target.pitch > 0 )
            bits += ( ras.target.rows - 1 ) * ras.target.pitch;

          if ( e1 >= 0              &&
               e1 < ras.target.rows &&
               *bits & f1 )
            return;

          if ( ras.dropOutControl == 2 )
            e1 = e2;
          else
            e1 = CEILING( ( x1 + x2 + 1 ) / 2 );

          break;

        default:
          return;  /* unsupported mode */
        }
      }
      else
        return;
    }

    bits = ras.bTarget + ( y >> 3 );
    f1   = (Byte)( 0x80 >> ( y & 7 ) );

    e1 = TRUNC( e1 );

    if ( e1 >= 0 && e1 < ras.target.rows )
    {
      bits -= e1 * ras.target.pitch;
      if ( ras.target.pitch > 0 )
        bits += ( ras.target.rows - 1 ) * ras.target.pitch;

      bits[0] |= f1;
    }
  }


  static void
  Horizontal_Sweep_Step( RAS_ARG )
  {
    /* Nothing, really */
    FT_UNUSED( raster );
  }


#ifdef FT_RASTER_OPTION_ANTI_ALIASING


  /*************************************************************************/
  /*                                                                       */
  /*  Vertical Gray Sweep Procedure Set                                    */
  /*                                                                       */
  /*  These two routines are used during the vertical gray-levels sweep    */
  /*  phase by the generic Draw_Sweep() function.                          */
  /*                                                                       */
  /*  NOTES                                                                */
  /*                                                                       */
  /*  - The target pixmap's width *must* be a multiple of 4.               */
  /*                                                                       */
  /*  - You have to use the function Vertical_Sweep_Span() for the gray    */
  /*    span call.                                                         */
  /*                                                                       */
  /*************************************************************************/

  static void
  Vertical_Gray_Sweep_Init( RAS_ARGS Short*  min,
                                     Short*  max )
  {
    Long  pitch, byte_len;


    *min = *min & -2;
    *max = ( *max + 3 ) & -2;

    ras.traceOfs  = 0;
    pitch         = ras.target.pitch;
    byte_len      = -pitch;
    ras.traceIncr = (Short)byte_len;
    ras.traceG    = ( *min / 2 ) * byte_len;

    if ( pitch > 0 )
    {
      ras.traceG += ( ras.target.rows - 1 ) * pitch;
      byte_len    = -byte_len;
    }

    ras.gray_min_x =  (Short)byte_len;
    ras.gray_max_x = -(Short)byte_len;
  }


  static void
  Vertical_Gray_Sweep_Step( RAS_ARG )
  {
    Int    c1, c2;
    PByte  pix, bit, bit2;
    Int*   count = ras.count_table;
    Byte*  grays;


    ras.traceOfs += ras.gray_width;

    if ( ras.traceOfs > ras.gray_width )
    {
      pix   = ras.gTarget + ras.traceG + ras.gray_min_x * 4;
      grays = ras.grays;

      if ( ras.gray_max_x >= 0 )
      {
        Long   last_pixel = ras.target.width - 1;
        Int    last_cell  = last_pixel >> 2;
        Int    last_bit   = last_pixel & 3;
        Bool   over       = 0;


        if ( ras.gray_max_x >= last_cell && last_bit != 3 )
        {
          ras.gray_max_x = last_cell - 1;
          over = 1;
        }

        if ( ras.gray_min_x < 0 )
          ras.gray_min_x = 0;

        bit   = ras.bTarget + ras.gray_min_x;
        bit2  = bit + ras.gray_width;

        c1 = ras.gray_max_x - ras.gray_min_x;

        while ( c1 >= 0 )
        {
          c2 = count[*bit] + count[*bit2];

          if ( c2 )
          {
            pix[0] = grays[(c2 >> 12) & 0x000F];
            pix[1] = grays[(c2 >> 8 ) & 0x000F];
            pix[2] = grays[(c2 >> 4 ) & 0x000F];
            pix[3] = grays[ c2        & 0x000F];

            *bit  = 0;
            *bit2 = 0;
          }

          bit++;
          bit2++;
          pix += 4;
          c1--;
        }

        if ( over )
        {
          c2 = count[*bit] + count[*bit2];
          if ( c2 )
          {
            switch ( last_bit )
            {
            case 2:
              pix[2] = grays[(c2 >> 4 ) & 0x000F];
            case 1:
              pix[1] = grays[(c2 >> 8 ) & 0x000F];
            default:
              pix[0] = grays[(c2 >> 12) & 0x000F];
            }

            *bit  = 0;
            *bit2 = 0;
          }
        }
      }

      ras.traceOfs = 0;
      ras.traceG  += ras.traceIncr;

      ras.gray_min_x =  32000;
      ras.gray_max_x = -32000;
    }
  }


  static void
  Horizontal_Gray_Sweep_Span( RAS_ARGS Short       y,
                                       FT_F26Dot6  x1,
                                       FT_F26Dot6  x2,
                                       PProfile    left,
                                       PProfile    right )
  {
    /* nothing, really */
    FT_UNUSED( raster );
    FT_UNUSED( y );
    FT_UNUSED( x1 );
    FT_UNUSED( x2 );
    FT_UNUSED( left );
    FT_UNUSED( right );
  }


  static void
  Horizontal_Gray_Sweep_Drop( RAS_ARGS Short       y,
                                       FT_F26Dot6  x1,
                                       FT_F26Dot6  x2,
                                       PProfile    left,
                                       PProfile    right )
  {
    Long   e1, e2;
    PByte  pixel;
    Byte   color;


    /* During the horizontal sweep, we only take care of drop-outs */
    e1 = CEILING( x1 );
    e2 = FLOOR  ( x2 );

    if ( e1 > e2 )
    {
      if ( e1 == e2 + ras.precision )
      {
        switch ( ras.dropOutControl )
        {
        case 1:
          e1 = e2;
          break;

        case 4:
          e1 = CEILING( ( x1 + x2 + 1 ) / 2 );
          break;

        case 2:
        case 5:

          /* Drop-out Control Rule #4 */

          /* The spec is not very clear regarding rule #4.  It      */
          /* presents a method that is way too costly to implement  */
          /* while the general idea seems to get rid of `stubs'.    */
          /*                                                        */

          /* rightmost stub test */
          if ( left->next == right && left->height <= 0 )
            return;

          /* leftmost stub test */
          if ( right->next == left && left->start == y )
            return;

          if ( ras.dropOutControl == 2 )
            e1 = e2;
          else
            e1 = CEILING( ( x1 + x2 + 1 ) / 2 );

          break;

        default:
          return;  /* unsupported mode */
        }
      }
      else
        return;
    }

    if ( e1 >= 0 )
    {
      if ( x2 - x1 >= ras.precision_half )
        color = ras.grays[2];
      else
        color = ras.grays[1];

      e1 = TRUNC( e1 ) / 2;
      if ( e1 < ras.target.rows )
      {
        pixel = ras.gTarget - e1 * ras.target.pitch + y / 2;
        if ( ras.target.pitch > 0 )
          pixel += ( ras.target.rows - 1 ) * ras.target.pitch;

        if ( pixel[0] == ras.grays[0] )
          pixel[0] = color;
      }
    }
  }


#endif /* FT_RASTER_OPTION_ANTI_ALIASING */


  /*************************************************************************/
  /*                                                                       */
  /*  Generic Sweep Drawing routine                                        */
  /*                                                                       */
  /*************************************************************************/

  static Bool
  Draw_Sweep( RAS_ARG )
  {
    Short         y, y_change, y_height;

    PProfile      P, Q, P_Left, P_Right;

    Short         min_Y, max_Y, top, bottom, dropouts;

    Long          x1, x2, xs, e1, e2;

    TProfileList  wait;
    TProfileList  draw_left, draw_right;


    /* Init empty linked lists */

    Init_Linked( &wait );

    Init_Linked( &draw_left  );
    Init_Linked( &draw_right );

    /* first, compute min and max Y */

    P     = ras.fProfile;
    max_Y = (Short)TRUNC( ras.minY );
    min_Y = (Short)TRUNC( ras.maxY );

    while ( P )
    {
      Q = P->link;

      bottom = (Short)P->start;
      top    = (Short)( P->start + P->height - 1 );

      if ( min_Y > bottom ) min_Y = bottom;
      if ( max_Y < top    ) max_Y = top;

      P->X = 0;
      InsNew( &wait, P );

      P = Q;
    }

    /* Check the Y-turns */
    if ( ras.numTurns == 0 )
    {
      ras.error = Raster_Err_Invalid;
      return FAILURE;
    }

    /* Now inits the sweep */

    ras.Proc_Sweep_Init( RAS_VARS &min_Y, &max_Y );

    /* Then compute the distance of each profile from min_Y */

    P = wait;

    while ( P )
    {
      P->countL = (UShort)( P->start - min_Y );
      P = P->link;
    }

    /* Let's go */

    y        = min_Y;
    y_height = 0;

    if ( ras.numTurns > 0 &&
         ras.sizeBuff[-ras.numTurns] == min_Y )
      ras.numTurns--;

    while ( ras.numTurns > 0 )
    {
      /* look in the wait list for new activations */

      P = wait;

      while ( P )
      {
        Q = P->link;
        P->countL -= y_height;
        if ( P->countL == 0 )
        {
          DelOld( &wait, P );

          switch ( P->flow )
          {
          case Flow_Up:
            InsNew( &draw_left,  P );
            break;

          case Flow_Down:
            InsNew( &draw_right, P );
            break;
          }
        }

        P = Q;
      }

      /* Sort the drawing lists */

      Sort( &draw_left );
      Sort( &draw_right );

      y_change = (Short)ras.sizeBuff[-ras.numTurns--];
      y_height = (Short)( y_change - y );

      while ( y < y_change )
      {
        /* Let's trace */

        dropouts = 0;

        P_Left  = draw_left;
        P_Right = draw_right;

        while ( P_Left )
        {
          x1 = P_Left ->X;
          x2 = P_Right->X;

          if ( x1 > x2 )
          {
            xs = x1;
            x1 = x2;
            x2 = xs;
          }

          if ( x2 - x1 <= ras.precision )
          {
            e1 = FLOOR( x1 );
            e2 = CEILING( x2 );

            if ( ras.dropOutControl != 0                 &&
                 ( e1 > e2 || e2 == e1 + ras.precision ) )
            {
              /* a drop out was detected */

              P_Left ->X = x1;
              P_Right->X = x2;

              /* mark profile for drop-out processing */
              P_Left->countL = 1;
              dropouts++;

              goto Skip_To_Next;
            }
          }

          ras.Proc_Sweep_Span( RAS_VARS y, x1, x2, P_Left, P_Right );

        Skip_To_Next:

          P_Left  = P_Left->link;
          P_Right = P_Right->link;
        }

        /* now perform the dropouts _after_ the span drawing -- */
        /* drop-outs processing has been moved out of the loop  */
        /* for performance tuning                               */
        if ( dropouts > 0 )
          goto Scan_DropOuts;

      Next_Line:

        ras.Proc_Sweep_Step( RAS_VAR );

        y++;

        if ( y < y_change )
        {
          Sort( &draw_left  );
          Sort( &draw_right );
        }
      }

      /* Now finalize the profiles that needs it */

      P = draw_left;
      while ( P )
      {
        Q = P->link;
        if ( P->height == 0 )
          DelOld( &draw_left, P );
        P = Q;
      }

      P = draw_right;
      while ( P )
      {
        Q = P->link;
        if ( P->height == 0 )
          DelOld( &draw_right, P );
        P = Q;
      }
    }

    /* for gray-scaling, flushes the bitmap scanline cache */
    while ( y <= max_Y )
    {
      ras.Proc_Sweep_Step( RAS_VAR );
      y++;
    }

    return SUCCESS;

  Scan_DropOuts:

    P_Left  = draw_left;
    P_Right = draw_right;

    while ( P_Left )
    {
      if ( P_Left->countL )
      {
        P_Left->countL = 0;
#if 0
        dropouts--;  /* -- this is useful when debugging only */
#endif
        ras.Proc_Sweep_Drop( RAS_VARS y,
                                      P_Left->X,
                                      P_Right->X,
                                      P_Left,
                                      P_Right );
      }

      P_Left  = P_Left->link;
      P_Right = P_Right->link;
    }

    goto Next_Line;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Render_Single_Pass                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Performs one sweep with sub-banding.                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    flipped :: If set, flip the direction of the outline.              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Renderer error code.                                               */
  /*                                                                       */
  static int
  Render_Single_Pass( RAS_ARGS Bool  flipped )
  {
    Short  i, j, k;


    while ( ras.band_top >= 0 )
    {
      ras.maxY = (Long)ras.band_stack[ras.band_top].y_max * ras.precision;
      ras.minY = (Long)ras.band_stack[ras.band_top].y_min * ras.precision;

      ras.top = ras.buff;

      ras.error = Raster_Err_None;

      if ( Convert_Glyph( RAS_VARS flipped ) )
      {
        if ( ras.error != Raster_Err_Overflow )
          return FAILURE;

        ras.error = Raster_Err_None;

        /* sub-banding */

#ifdef DEBUG_RASTER
        ClearBand( RAS_VARS TRUNC( ras.minY ), TRUNC( ras.maxY ) );
#endif

        i = ras.band_stack[ras.band_top].y_min;
        j = ras.band_stack[ras.band_top].y_max;

        k = (Short)( ( i + j ) / 2 );

        if ( ras.band_top >= 7 || k < i )
        {
          ras.band_top = 0;
          ras.error    = Raster_Err_Invalid;

          return ras.error;
        }

        ras.band_stack[ras.band_top + 1].y_min = k;
        ras.band_stack[ras.band_top + 1].y_max = j;

        ras.band_stack[ras.band_top].y_max = (Short)( k - 1 );

        ras.band_top++;
      }
      else
      {
        if ( ras.fProfile )
          if ( Draw_Sweep( RAS_VAR ) )
             return ras.error;
        ras.band_top--;
      }
    }

    return SUCCESS;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Render_Glyph                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Renders a glyph in a bitmap.  Sub-banding if needed.               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  Render_Glyph( RAS_ARG )
  {
    FT_Error  error;


    Set_High_Precision( RAS_VARS ras.outline.flags &
                        ft_outline_high_precision );
    ras.scale_shift    = ras.precision_shift;
    ras.dropOutControl = 2;
    ras.second_pass    = (FT_Byte)( !( ras.outline.flags &
                                       ft_outline_single_pass ) );

    /* Vertical Sweep */
    ras.Proc_Sweep_Init = Vertical_Sweep_Init;
    ras.Proc_Sweep_Span = Vertical_Sweep_Span;
    ras.Proc_Sweep_Drop = Vertical_Sweep_Drop;
    ras.Proc_Sweep_Step = Vertical_Sweep_Step;

    ras.band_top            = 0;
    ras.band_stack[0].y_min = 0;
    ras.band_stack[0].y_max = (short)( ras.target.rows - 1 );

    ras.bWidth  = (unsigned short)ras.target.width;
    ras.bTarget = (Byte*)ras.target.buffer;

    if ( ( error = Render_Single_Pass( RAS_VARS 0 ) ) != 0 )
      return error;

    /* Horizontal Sweep */
    if ( ras.second_pass && ras.dropOutControl != 0 )
    {
      ras.Proc_Sweep_Init = Horizontal_Sweep_Init;
      ras.Proc_Sweep_Span = Horizontal_Sweep_Span;
      ras.Proc_Sweep_Drop = Horizontal_Sweep_Drop;
      ras.Proc_Sweep_Step = Horizontal_Sweep_Step;

      ras.band_top            = 0;
      ras.band_stack[0].y_min = 0;
      ras.band_stack[0].y_max = (short)( ras.target.width - 1 );

      if ( ( error = Render_Single_Pass( RAS_VARS 1 ) ) != 0 )
        return error;
    }

    return Raster_Err_Ok;
  }


#ifdef FT_RASTER_OPTION_ANTI_ALIASING


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Render_Gray_Glyph                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Renders a glyph with grayscaling.  Sub-banding if needed.          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  Render_Gray_Glyph( RAS_ARG )
  {
    Long      pixel_width;
    FT_Error  error;


    Set_High_Precision( RAS_VARS ras.outline.flags &
                        ft_outline_high_precision );
    ras.scale_shift    = ras.precision_shift + 1;
    ras.dropOutControl = 2;
    ras.second_pass    = !( ras.outline.flags & ft_outline_single_pass );

    /* Vertical Sweep */

    ras.band_top            = 0;
    ras.band_stack[0].y_min = 0;
    ras.band_stack[0].y_max = 2 * ras.target.rows - 1;

    ras.bWidth  = ras.gray_width;
    pixel_width = 2 * ( ( ras.target.width + 3 ) >> 2 );

    if ( ras.bWidth > pixel_width )
      ras.bWidth = pixel_width;

    ras.bWidth  = ras.bWidth * 8;
    ras.bTarget = (Byte*)ras.gray_lines;
    ras.gTarget = (Byte*)ras.target.buffer;

    ras.Proc_Sweep_Init = Vertical_Gray_Sweep_Init;
    ras.Proc_Sweep_Span = Vertical_Sweep_Span;
    ras.Proc_Sweep_Drop = Vertical_Sweep_Drop;
    ras.Proc_Sweep_Step = Vertical_Gray_Sweep_Step;

    error = Render_Single_Pass( RAS_VARS 0 );
    if ( error )
      return error;

    /* Horizontal Sweep */
    if ( ras.second_pass && ras.dropOutControl != 0 )
    {
      ras.Proc_Sweep_Init = Horizontal_Sweep_Init;
      ras.Proc_Sweep_Span = Horizontal_Gray_Sweep_Span;
      ras.Proc_Sweep_Drop = Horizontal_Gray_Sweep_Drop;
      ras.Proc_Sweep_Step = Horizontal_Sweep_Step;

      ras.band_top            = 0;
      ras.band_stack[0].y_min = 0;
      ras.band_stack[0].y_max = ras.target.width * 2 - 1;

      error = Render_Single_Pass( RAS_VARS 1 );
      if ( error )
        return error;
    }

    return Raster_Err_Ok;
  }

#else /* FT_RASTER_OPTION_ANTI_ALIASING */

  FT_LOCAL_DEF
  FT_Error  Render_Gray_Glyph( RAS_ARG )
  {
    FT_UNUSED_RASTER;

    return Raster_Err_Cannot_Render_Glyph;
  }

#endif /* FT_RASTER_OPTION_ANTI_ALIASING */


  static void
  ft_black_init( TRaster_Instance*  raster )
  {
    FT_UInt  n;
    FT_ULong c;


    /* setup count table */
    for ( n = 0; n < 256; n++ )
    {
      c = ( n & 0x55 ) + ( ( n & 0xAA ) >> 1 );

      c = ( ( c << 6 ) & 0x3000 ) |
          ( ( c << 4 ) & 0x0300 ) |
          ( ( c << 2 ) & 0x0030 ) |
                   (c  & 0x0003 );

      raster->count_table[n] = c;
    }

#ifdef FT_RASTER_OPTION_ANTI_ALIASING

    /* set default 5-levels gray palette */
    for ( n = 0; n < 5; n++ )
      raster->grays[n] = n * 255 / 4;

    raster->gray_width = RASTER_GRAY_LINES / 2;

#endif
  }


  /**** RASTER OBJECT CREATION: In standalone mode, we simply use *****/
  /****                         a static object.                  *****/


#ifdef _STANDALONE_


  static int
  ft_black_new( void*      memory,
                FT_Raster  *araster )
  {
     static FT_RasterRec_  the_raster;


     *araster = &the_raster;
     MEM_Set( &the_raster, sizeof ( the_raster ), 0 );
     ft_black_init( &the_raster );

     return 0;
  }


  static void
  ft_black_done( FT_Raster  raster )
  {
    /* nothing */
    raster->init = 0;
  }


#else /* _STANDALONE_ */


  static int
  ft_black_new( FT_Memory           memory,
                TRaster_Instance**  araster )
  {
    FT_Error           error;
    TRaster_Instance*  raster;


    *araster = 0;
    if ( !ALLOC( raster, sizeof ( *raster ) ) )
    {
      raster->memory = memory;
      ft_black_init( raster );

      *araster = raster;
    }

    return error;
  }


  static void
  ft_black_done( TRaster_Instance*  raster )
  {
    FT_Memory  memory = (FT_Memory)raster->memory;
    FREE( raster );
  }


#endif /* _STANDALONE_ */


  static void
  ft_black_reset( TRaster_Instance*  raster,
                  const char*        pool_base,
                  long               pool_size )
  {
    if ( raster && pool_base && pool_size >= 4096 )
    {
      /* save the pool */
      raster->buff     = (PLong)pool_base;
      raster->sizeBuff = raster->buff + pool_size / sizeof ( Long );
    }
  }


  static void
  ft_black_set_mode( TRaster_Instance* raster,
                     unsigned long     mode,
                     const char*       palette )
  {
#ifdef FT_RASTER_OPTION_ANTI_ALIASING

    if ( mode == FT_MAKE_TAG( 'p', 'a', 'l', '5' ) )
    {
      /* set 5-levels gray palette */
      raster->grays[0] = palette[0];
      raster->grays[1] = palette[1];
      raster->grays[2] = palette[2];
      raster->grays[3] = palette[3];
      raster->grays[4] = palette[4];
    }

#else

    FT_UNUSED( raster );
    FT_UNUSED( mode );
    FT_UNUSED( palette );

#endif
  }


  static int
  ft_black_render( TRaster_Instance*  raster,
                   FT_Raster_Params*  params )
  {
    FT_Outline*  outline    = (FT_Outline*)params->source;
    FT_Bitmap*   target_map = params->target;


    if ( !raster || !raster->buff || !raster->sizeBuff )
      return Raster_Err_Not_Ini;

    /* return immediately if the outline is empty */
    if ( outline->n_points == 0 || outline->n_contours <= 0 )
      return Raster_Err_None;

    if ( !outline || !outline->contours || !outline->points )
      return Raster_Err_Invalid;

    if ( outline->n_points != outline->contours[outline->n_contours - 1] + 1 )
      return Raster_Err_Invalid;

    /* this version of the raster does not support direct rendering, sorry */
    if ( params->flags & ft_raster_flag_direct )
      return Raster_Err_Unsupported;

    if ( !target_map || !target_map->buffer )
      return Raster_Err_Invalid;

    ras.outline  = *outline;
    ras.target   = *target_map;

    return ( ( params->flags & ft_raster_flag_aa )
               ? Render_Gray_Glyph( raster )
               : Render_Glyph( raster ) );
  }


  const FT_Raster_Funcs  ft_standard_raster =
  {
    ft_glyph_format_outline,
    (FT_Raster_New_Func)     ft_black_new,
    (FT_Raster_Reset_Func)   ft_black_reset,
    (FT_Raster_Set_Mode_Func)ft_black_set_mode,
    (FT_Raster_Render_Func)  ft_black_render,
    (FT_Raster_Done_Func)    ft_black_done
  };


/* END */
