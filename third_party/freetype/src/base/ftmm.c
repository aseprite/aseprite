/***************************************************************************/
/*                                                                         */
/*  ftmm.c                                                                 */
/*                                                                         */
/*    Multiple Master font support (body).                                 */
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


#include <ft2build.h>
#include FT_MULTIPLE_MASTERS_H
#include FT_INTERNAL_OBJECTS_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_mm


  /* documentation is in ftmm.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Get_Multi_Master( FT_Face           face,
                       FT_Multi_Master  *amaster )
  {
    FT_Error  error;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    error = FT_Err_Invalid_Argument;

    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
    {
      FT_Driver       driver = face->driver;
      FT_Get_MM_Func  func;


      func = (FT_Get_MM_Func)driver->root.clazz->get_interface(
                               FT_MODULE( driver ), "get_mm" );
      if ( func )
        error = func( face, amaster );
    }

    return error;
  }


  /* documentation is in ftmm.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_MM_Design_Coordinates( FT_Face   face,
                                FT_UInt   num_coords,
                                FT_Long*  coords )
  {
    FT_Error  error;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    error = FT_Err_Invalid_Argument;

    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
    {
      FT_Driver              driver = face->driver;
      FT_Set_MM_Design_Func  func;


      func = (FT_Set_MM_Design_Func)driver->root.clazz->get_interface(
                                      FT_MODULE( driver ), "set_mm_design" );
      if ( func )
        error = func( face, num_coords, coords );
    }

    return error;
  }


  /* documentation is in ftmm.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Set_MM_Blend_Coordinates( FT_Face    face,
                               FT_UInt    num_coords,
                               FT_Fixed*  coords )
  {
    FT_Error  error;


    if ( !face )
      return FT_Err_Invalid_Face_Handle;

    error = FT_Err_Invalid_Argument;

    if ( FT_HAS_MULTIPLE_MASTERS( face ) )
    {
      FT_Driver             driver = face->driver;
      FT_Set_MM_Blend_Func  func;


      func = (FT_Set_MM_Blend_Func)driver->root.clazz->get_interface(
                                     FT_MODULE( driver ), "set_mm_blend" );
      if ( func )
        error = func( face, num_coords, coords );
    }

    return error;
  }


/* END */
