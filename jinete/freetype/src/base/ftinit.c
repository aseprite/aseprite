/***************************************************************************/
/*                                                                         */
/*  ftinit.c                                                               */
/*                                                                         */
/*    FreeType initialization layer (body).                                */
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
  /*  The purpose of this file is to implement the following two           */
  /*  functions:                                                           */
  /*                                                                       */
  /*  FT_Add_Default_Modules():                                            */
  /*     This function is used to add the set of default modules to a      */
  /*     fresh new library object.  The set is taken from the header file  */
  /*     `freetype/config/ftmodule.h'.  See the document `FreeType 2.0     */
  /*     Build System' for more information.                               */
  /*                                                                       */
  /*  FT_Init_FreeType():                                                  */
  /*     This function creates a system object for the current platform,   */
  /*     builds a library out of it, then calls FT_Default_Drivers().      */
  /*                                                                       */
  /*  Note that even if FT_Init_FreeType() uses the implementation of the  */
  /*  system object defined at build time, client applications are still   */
  /*  able to provide their own `ftsystem.c'.                              */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_MODULE_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_init

#undef  FT_USE_MODULE
#ifdef __cplusplus
#define FT_USE_MODULE( x )  extern "C" const FT_Module_Class*  x;
#else
#define FT_USE_MODULE( x )  extern const FT_Module_Class*  x;
#endif


#include FT_CONFIG_MODULES_H


#undef  FT_USE_MODULE
#define FT_USE_MODULE( x )  (const FT_Module_Class*)&x,

  static
  const FT_Module_Class*  const ft_default_modules[] =
  {
#include FT_CONFIG_MODULES_H
    0
  };


  /* documentation is in ftmodule.h */

  FT_EXPORT_DEF( void )
  FT_Add_Default_Modules( FT_Library  library )
  {
    FT_Error                       error;
    const FT_Module_Class* const*  cur;


    /* test for valid `library' delayed to FT_Add_Module() */

    cur = ft_default_modules;
    while ( *cur )
    {
      error = FT_Add_Module( library, *cur );
      /* notify errors, but don't stop */
      if ( error )
      {
        FT_ERROR(( "FT_Add_Default_Module: Cannot install `%s', error = %x\n",
                   (*cur)->module_name, error ));
      }
      cur++;
    }
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Init_FreeType( FT_Library  *alibrary )
  {
    FT_Error   error;
    FT_Memory  memory;


    /* First of all, allocate a new system object -- this function is part */
    /* of the system-specific component, i.e. `ftsystem.c'.                */

    memory = FT_New_Memory();
    if ( !memory )
    {
      FT_ERROR(( "FT_Init_FreeType: cannot find memory manager\n" ));
      return FT_Err_Unimplemented_Feature;
    }

    /* build a library out of it, then fill it with the set of */
    /* default drivers.                                        */

    error = FT_New_Library( memory, alibrary );
    if ( !error )
      FT_Add_Default_Modules( *alibrary );

    return error;
  }


  /* documentation is in freetype.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Done_FreeType( FT_Library  library )
  {
    if ( library )
    {
      FT_Memory  memory = library->memory;


      /* Discard the library object */
      FT_Done_Library( library );

      /* discard memory manager */
      FT_Done_Memory( memory );
    }

    return FT_Err_Ok;
  }


/* END */
