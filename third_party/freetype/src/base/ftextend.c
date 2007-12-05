/***************************************************************************/
/*                                                                         */
/*  ftextend.c                                                             */
/*                                                                         */
/*    FreeType extensions implementation (body).                           */
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
  /*  This is an updated version of the extension component, now located   */
  /*  in the main library's source directory.  It allows the dynamic       */
  /*  registration/use of various face object extensions through a simple  */
  /*  API.                                                                 */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_EXTEND_H
#include FT_INTERNAL_DEBUG_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_extend


  typedef struct  FT_Extension_Registry_
  {
    FT_Int              num_extensions;
    FT_Long             cur_offset;
    FT_Extension_Class  classes[FT_MAX_EXTENSIONS];

  } FT_Extension_Registry;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Init_Extensions                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes the extension component.                               */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    driver :: A handle to the driver object.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  FT_Init_Extensions( FT_Driver  driver )
  {
    FT_Error                error;
    FT_Memory               memory;
    FT_Extension_Registry*  registry;


    memory = driver->root.library->memory;
    if ( ALLOC( registry, sizeof ( *registry ) ) )
      return error;

    registry->num_extensions = 0;
    registry->cur_offset     = 0;
    driver->extensions       = registry;

    FT_TRACE2(( "FT_Init_Extensions: success\n" ));

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Extensions                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes the extension component.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    driver :: A handle to the driver object.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  FT_Done_Extensions( FT_Driver  driver )
  {
    FT_Memory  memory = driver->root.memory;


    FREE( driver->extensions );
    return FT_Err_Ok;
  }


  /* documentation is in ftextend.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Register_Extension( FT_Driver            driver,
                         FT_Extension_Class*  clazz )
  {
    FT_Extension_Registry*  registry;


    if ( !driver )
      return FT_Err_Invalid_Driver_Handle;

    if ( !clazz )
      return FT_Err_Invalid_Argument;

    registry = (FT_Extension_Registry*)driver->extensions;
    if ( registry )
    {
      FT_Int               n   = registry->num_extensions;
      FT_Extension_Class*  cur = registry->classes + n;


      if ( n >= FT_MAX_EXTENSIONS )
        return FT_Err_Too_Many_Extensions;

      *cur = *clazz;

      cur->offset  = registry->cur_offset;

      registry->num_extensions++;
      registry->cur_offset +=
        ( cur->size + FT_ALIGNMENT - 1 ) & -FT_ALIGNMENT;

      FT_TRACE1(( "FT_Register_Extension: `%s' successfully registered\n",
                  cur->id ));
    }

    return FT_Err_Ok;
  }


  /* documentation is in ftextend.h */

  FT_EXPORT_DEF( void* )
  FT_Get_Extension( FT_Face      face,
                    const char*  extension_id,
                    void**       extension_interface )
  {
    FT_Extension_Registry*  registry;


    if ( !face || !extension_id || !extension_interface )
      return 0;

    registry = (FT_Extension_Registry*)face->driver->extensions;
    if ( registry && face->extensions )
    {
      FT_Extension_Class*  cur   = registry->classes;
      FT_Extension_Class*  limit = cur + registry->num_extensions;


      for ( ; cur < limit; cur++ )
        if ( strcmp( cur->id, extension_id ) == 0 )
        {
          *extension_interface = cur->interface;

          FT_TRACE1(( "FT_Get_Extension: got `%s'\n", extension_id ));

          return (void*)((char*)face->extensions + cur->offset);
        }
    }

    /* could not find the extension id */

    FT_ERROR(( "FT_Get_Extension: couldn't find `%s'\n", extension_id ));

    *extension_interface = 0;

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Destroy_Extensions                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys all extensions within a face object.                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face :: A handle to the face object.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Called by the face object destructor.                              */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  FT_Destroy_Extensions( FT_Face  face )
  {
    FT_Extension_Registry*  registry;
    FT_Memory               memory;


    registry = (FT_Extension_Registry*)face->driver->extensions;
    if ( registry && face->extensions )
    {
      FT_Extension_Class*  cur   = registry->classes;
      FT_Extension_Class*  limit = cur + registry->num_extensions;


      for ( ; cur < limit; cur++ )
      {
        char*  ext = (char*)face->extensions + cur->offset;

        if ( cur->finalize )
          cur->finalize( ext, face );
      }

      memory = face->driver->root.memory;
      FREE( face->extensions );
    }

    return FT_Err_Ok;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Create_Extensions                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates an extension object within a face object for all           */
  /*    registered extensions.                                             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face :: A handle to the face object.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Called by the face object constructor.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  FT_Create_Extensions( FT_Face  face )
  {
    FT_Extension_Registry*  registry;
    FT_Memory               memory;
    FT_Error                error;


    face->extensions = 0;

    /* load extensions registry; exit successfully if none is there */

    registry = (FT_Extension_Registry*)face->driver->extensions;
    if ( !registry )
      return FT_Err_Ok;

    memory = face->driver->root.memory;
    if ( ALLOC( face->extensions, registry->cur_offset ) )
      return error;

    {
      FT_Extension_Class*  cur   = registry->classes;
      FT_Extension_Class*  limit = cur + registry->num_extensions;


      for ( ; cur < limit; cur++ )
      {
        char*  ext = (char*)face->extensions + cur->offset;

        if ( cur->init )
        {
          error = cur->init( ext, face );
          if ( error )
            break;
        }
      }
    }

    return error;
  }


/* END */
