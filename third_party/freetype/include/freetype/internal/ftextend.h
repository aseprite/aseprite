/***************************************************************************/
/*                                                                         */
/*  ftextend.h                                                             */
/*                                                                         */
/*    FreeType extensions implementation (specification).                  */
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


#ifndef __FTEXTEND_H__
#define __FTEXTEND_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* The extensions don't need to be integrated at compile time into the   */
  /* engine, only at link time.                                            */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FT_Extension_Initializer                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Each new face object can have several extensions associated with   */
  /*    it at creation time.  This function is used to initialize given    */
  /*    extension data for a given face.                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    ext  :: A typeless pointer to the extension data.                  */
  /*                                                                       */
  /*    face :: A handle to the source face object the extension is        */
  /*            associated with.                                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    In case of error, the initializer should not destroy the extension */
  /*    data, as the finalizer will get called later by the function's     */
  /*    caller.                                                            */
  /*                                                                       */
  typedef FT_Error
  (*FT_Extension_Initializer)( void*    ext,
                               FT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FT_Extension_Finalizer                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Each new face object can have several extensions associated with   */
  /*    it at creation time.  This function is used to finalize given      */
  /*    extension data for a given face; it occurs before the face object  */
  /*    itself is finalized.                                               */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    ext  :: A typeless pointer to the extension data.                  */
  /*                                                                       */
  /*    face :: A handle to the source face object the extension is        */
  /*            associated with.                                           */
  /*                                                                       */
  typedef void
  (*FT_Extension_Finalizer)( void*    ext,
                             FT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Extension_Class                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to describe a given extension to the       */
  /*    FreeType base layer.  An FT_Extension_Class is used as a parameter */
  /*    for FT_Register_Extension().                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    id        :: The extension's ID.  This is a normal C string that   */
  /*                 is used to uniquely reference the extension's         */
  /*                 interface.                                            */
  /*                                                                       */
  /*    size      :: The size in bytes of the extension data that must be  */
  /*                 associated with each face object.                     */
  /*                                                                       */
  /*    init      :: A pointer to the extension data's initializer.        */
  /*                                                                       */
  /*    finalize  :: A pointer to the extension data's finalizer.          */
  /*                                                                       */
  /*    interface :: This pointer can be anything, but should usually      */
  /*                 point to a table of function pointers which implement */
  /*                 the extension's interface.                            */
  /*                                                                       */
  /*    offset    :: This field is set and used within the base layer and  */
  /*                 should be set to 0 when registering an extension      */
  /*                 through FT_Register_Extension().  It contains an      */
  /*                 offset within the face's extension block for the      */
  /*                 current extension's data.                             */
  /*                                                                       */
  typedef struct  FT_Extension_Class_
  {
    const char*               id;
    FT_ULong                  size;
    FT_Extension_Initializer  init;
    FT_Extension_Finalizer    finalize;
    void*                     interface;

    FT_ULong                  offset;

  } FT_Extension_Class;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Register_Extension                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Registers a new extension.                                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    driver :: A handle to the driver object.                           */
  /*                                                                       */
  /*    class  :: A pointer to a class describing the extension.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Register_Extension( FT_Driver            driver,
                         FT_Extension_Class*  clazz );


#ifdef FT_CONFIG_OPTION_EXTEND_ENGINE


  /* Initialize the extension component */
  FT_LOCAL FT_Error
  FT_Init_Extensions( FT_Library  library );

  /* Finalize the extension component */
  FT_LOCAL FT_Error
  FT_Done_Extensions( FT_Library  library );

  /* Create an extension within a face object.  Called by the */
  /* face object constructor.                                 */
  FT_LOCAL FT_Error
  FT_Create_Extensions( FT_Face  face );

  /* Destroy all extensions within a face object.  Called by the */
  /* face object destructor.                                     */
  FT_LOCAL FT_Error
  FT_Destroy_Extensions( FT_Face  face );


#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Extension                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Queries an extension block by an extension ID string.              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face                :: A handle to the face object.                */
  /*    extension_id        :: An ID string identifying the extension.     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    extension_interface :: A generic pointer, usually pointing to a    */
  /*                           table of functions implementing the         */
  /*                           extension interface.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A generic pointer to the extension block.                          */
  /*                                                                       */
  FT_EXPORT( void* )
  FT_Get_Extension( FT_Face      face,
                    const char*  extension_id,
                    void**       extension_interface );


FT_END_HEADER

#endif /* __FTEXTEND_H__ */


/* END */
