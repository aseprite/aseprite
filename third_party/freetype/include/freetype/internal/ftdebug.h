/***************************************************************************/
/*                                                                         */
/*  ftdebug.h                                                              */
/*                                                                         */
/*    Debugging and logging component (specification).                     */
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


#ifndef __FTDEBUG_H__
#define __FTDEBUG_H__


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H


FT_BEGIN_HEADER


#ifdef FT_DEBUG_LEVEL_TRACE


  /* note that not all levels are used currently */

  typedef enum  FT_Trace_
  {
    /* the first level must always be `trace_any' */
    trace_any = 0,

    /* base components */
    trace_aaraster,  /* anti-aliasing raster    (ftgrays.c)  */
    trace_calc,      /* calculations            (ftcalc.c)   */
    trace_extend,    /* extension manager       (ftextend.c) */
    trace_glyph,     /* glyph manager           (ftglyph.c)  */
    trace_io,        /* i/o monitoring          (ftsystem.c) */
    trace_init,      /* initialization          (ftinit.c)   */
    trace_list,      /* list manager            (ftlist.c)   */
    trace_memory,    /* memory manager          (ftobjs.c)   */
    trace_mm,        /* MM interface            (ftmm.c)     */
    trace_objs,      /* base objects            (ftobjs.c)   */
    trace_outline,   /* outline management      (ftoutln.c)  */
    trace_raster,    /* rasterizer              (ftraster.c) */
    trace_stream,    /* stream manager          (ftstream.c) */

    /* Cache sub-system */
    trace_cache,

    /* SFNT driver components */
    trace_sfobjs,    /* SFNT object handler     (sfobjs.c)   */
    trace_ttcmap,    /* charmap handler         (ttcmap.c)   */
    trace_ttload,    /* basic TrueType tables   (ttload.c)   */
    trace_ttpost,    /* PS table processing     (ttpost.c)   */
    trace_ttsbit,    /* TrueType sbit handling  (ttsbit.c)   */

    /* TrueType driver components */
    trace_ttdriver,  /* TT font driver          (ttdriver.c) */
    trace_ttgload,   /* TT glyph loader         (ttgload.c)  */
    trace_ttinterp,  /* bytecode interpreter    (ttinterp.c) */
    trace_ttobjs,    /* TT objects manager      (ttobjs.c)   */
    trace_ttpload,   /* TT data/program loader  (ttpload.c)  */

    /* Type 1 driver components */
    trace_t1driver,
    trace_t1gload,
    trace_t1hint,
    trace_t1load,
    trace_t1objs,
    trace_t1parse,

    /* PostScript helper module `psaux' */
    trace_t1decode,
    trace_psobjs,

    /* PostScript hinting module `pshinter' */
    trace_pshrec,
    trace_pshalgo1,
    trace_pshalgo2,

    /* Type 2 driver components */
    trace_cffdriver,
    trace_cffgload,
    trace_cffload,
    trace_cffobjs,
    trace_cffparse,

    /* CID driver components */
    trace_cidafm,
    trace_ciddriver,
    trace_cidgload,
    trace_cidload,
    trace_cidobjs,
    trace_cidparse,

    /* Windows fonts component */
    trace_winfnt,

    /* PCF fonts component */
    trace_pcfdriver,
    trace_pcfread,

    /* the last level must always be `trace_max' */
    trace_max

  } FT_Trace;


  /* declared in ftdebug.c */
  extern char  ft_trace_levels[trace_max];


  /*************************************************************************/
  /*                                                                       */
  /* IMPORTANT!                                                            */
  /*                                                                       */
  /* Each component must define the macro FT_COMPONENT to a valid FT_Trace */
  /* value before using any TRACE macro.                                   */
  /*                                                                       */
  /*************************************************************************/


#define FT_TRACE( level, varformat )                      \
          do                                              \
          {                                               \
            if ( ft_trace_levels[FT_COMPONENT] >= level ) \
              FT_Message varformat;                       \
          } while ( 0 )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_SetTraceLevel                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the trace level for debugging.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    component :: The component which should be traced.  See above for  */
  /*                 a complete list.  If set to `trace_any', all          */
  /*                 components will be traced.                            */
  /*                                                                       */
  /*    level     :: The tracing level.                                    */
  /*                                                                       */
  FT_EXPORT( void )
  FT_SetTraceLevel( FT_Trace  component,
                    char      level );


#elif defined( FT_DEBUG_LEVEL_ERROR )


#define FT_TRACE( level, varformat )  do ; while ( 0 )      /* nothing */


#else  /* release mode */


#define FT_Assert( condition )        do ; while ( 0 )      /* nothing */

#define FT_TRACE( level, varformat )  do ; while ( 0 )      /* nothing */
#define FT_ERROR( varformat )         do ; while ( 0 )      /* nothing */


#endif /* FT_DEBUG_LEVEL_TRACE, FT_DEBUG_LEVEL_ERROR */


  /*************************************************************************/
  /*                                                                       */
  /* Define macros and functions that are common to the debug and trace    */
  /* modes.                                                                */
  /*                                                                       */
  /* You need vprintf() to be able to compile ftdebug.c.                   */
  /*                                                                       */
  /*************************************************************************/


#if defined( FT_DEBUG_LEVEL_TRACE ) || defined( FT_DEBUG_LEVEL_ERROR )


#include "stdio.h"  /* for vprintf() */


#define FT_Assert( condition )                                      \
          do                                                        \
          {                                                         \
            if ( !( condition ) )                                   \
              FT_Panic( "assertion failed on line %d of file %s\n", \
                        __LINE__, __FILE__ );                       \
          } while ( 0 )

  /* print a message */
  FT_EXPORT( void )
  FT_Message( const char*  fmt, ... );

  /* print a message and exit */
  FT_EXPORT( void )
  FT_Panic( const char*  fmt, ... );

#define FT_ERROR( varformat )  FT_Message varformat


#endif /* FT_DEBUG_LEVEL_TRACE || FT_DEBUG_LEVEL_ERROR */


  /*************************************************************************/
  /*                                                                       */
  /* You need two opening resp. closing parentheses!                       */
  /*                                                                       */
  /* Example: FT_TRACE0(( "Value is %i", foo ))                            */
  /*                                                                       */
  /*************************************************************************/

#define FT_TRACE0( varformat )  FT_TRACE( 0, varformat )
#define FT_TRACE1( varformat )  FT_TRACE( 1, varformat )
#define FT_TRACE2( varformat )  FT_TRACE( 2, varformat )
#define FT_TRACE3( varformat )  FT_TRACE( 3, varformat )
#define FT_TRACE4( varformat )  FT_TRACE( 4, varformat )
#define FT_TRACE5( varformat )  FT_TRACE( 5, varformat )
#define FT_TRACE6( varformat )  FT_TRACE( 6, varformat )
#define FT_TRACE7( varformat )  FT_TRACE( 7, varformat )


#if defined( _MSC_VER )      /* Visual C++ (and Intel C++) */

  /* we disable the warning `conditional expression is constant' here */
  /* in order to compile cleanly with the maximum level of warnings   */
#pragma warning( disable : 4127 )

#endif /* _MSC_VER */


FT_END_HEADER

#endif /* __FTDEBUG_H__ */


/* END */
