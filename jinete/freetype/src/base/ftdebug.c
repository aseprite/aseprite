/***************************************************************************/
/*                                                                         */
/*  ftdebug.c                                                              */
/*                                                                         */
/*    Debugging and logging component (body).                              */
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
  /* This component contains various macros and functions used to ease the */
  /* debugging of the FreeType engine.  Its main purpose is in assertion   */
  /* checking, tracing, and error detection.                               */
  /*                                                                       */
  /* There are now three debugging modes:                                  */
  /*                                                                       */
  /* - trace mode                                                          */
  /*                                                                       */
  /*   Error and trace messages are sent to the log file (which can be the */
  /*   standard error output).                                             */
  /*                                                                       */
  /* - error mode                                                          */
  /*                                                                       */
  /*   Only error messages are generated.                                  */
  /*                                                                       */
  /* - release mode:                                                       */
  /*                                                                       */
  /*   No error message is sent or generated.  The code is free from any   */
  /*   debugging parts.                                                    */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H


#ifdef FT_DEBUG_LEVEL_TRACE
  char  ft_trace_levels[trace_max];
#endif


#if defined( FT_DEBUG_LEVEL_ERROR ) || defined( FT_DEBUG_LEVEL_TRACE )


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


  FT_EXPORT_DEF( void )
  FT_Message( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );
  }


  FT_EXPORT_DEF( void )
  FT_Panic( const char*  fmt, ... )
  {
    va_list  ap;


    va_start( ap, fmt );
    vprintf( fmt, ap );
    va_end( ap );

    exit( EXIT_FAILURE );
  }


#ifdef FT_DEBUG_LEVEL_TRACE

  FT_EXPORT_DEF( void )
  FT_SetTraceLevel( FT_Trace  component,
                    char      level )
  {
    if ( component >= trace_max )
      return;

    /* if component is `trace_any', change _all_ levels at once */
    if ( component == trace_any )
    {
      int  n;


      for ( n = trace_any; n < trace_max; n++ )
        ft_trace_levels[n] = level;
    }
    else        /* otherwise, only change individual component */
      ft_trace_levels[component] = level;
  }

#endif /* FT_DEBUG_LEVEL_TRACE */

#endif /* FT_DEBUG_LEVEL_TRACE || FT_DEBUG_LEVEL_ERROR */


  /* ANSI C doesn't allow empty files, so we insert a dummy symbol */
  extern const int  ft_debug_dummy;


/* END */
