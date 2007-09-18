/***************************************************************************/
/*                                                                         */
/*  ftcache.h                                                              */
/*                                                                         */
/*    FreeType Cache subsystem (specification).                            */
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
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*********                                                       *********/
  /*********             WARNING, THIS IS BETA CODE.               *********/
  /*********                                                       *********/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#ifndef __FTCACHE_H__
#define __FTCACHE_H__


#include <ft2build.h>
#include FT_GLYPH_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    cache_subsystem                                                    */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Cache Sub-System                                                   */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    How to cache face, size, and glyph data with FreeType 2.           */
  /*                                                                       */
  /* <Description>                                                         */
  /*   This section describes the FreeType 2 cache sub-system which is     */
  /*   stile in beta.                                                      */
  /*                                                                       */
  /* <Order>                                                               */
  /*   FTC_Manager                                                         */
  /*   FTC_FaceID                                                          */
  /*   FTC_Face_Requester                                                  */
  /*                                                                       */
  /*   FTC_Manager_New                                                     */
  /*   FTC_Manager_Lookup_Face                                             */
  /*   FTC_Manager_Lookup_Size                                             */
  /*                                                                       */
  /*   FTC_Node                                                            */
  /*   FTC_Node_Ref                                                        */
  /*   FTC_Node_Unref                                                      */
  /*                                                                       */
  /*   FTC_Font                                                            */
  /*   FTC_ImageDesc                                                       */
  /*   FTC_ImageCache                                                      */
  /*   FTC_ImageCache_New                                                  */
  /*   FTC_ImageCache_Lookup                                               */
  /*                                                                       */
  /*   FTC_SBit                                                            */
  /*   FTC_SBitCache                                                       */
  /*   FTC_SBitCache_New                                                   */
  /*   FTC_SBitCache_Lookup                                                */
  /*                                                                       */
  /*                                                                       */
  /*   FTC_Image_Desc                                                      */
  /*   FTC_Image_Cache                                                     */
  /*   FTC_Image_Cache_Lookup                                              */
  /*                                                                       */
  /*   FTC_SBit_Cache                                                      */
  /*   FTC_SBit_Cache_Lookup                                               */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    BASIC TYPE DEFINITIONS                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_FaceID                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A generic pointer type that is used to identity face objects.  The */
  /*    contents of such objects is application-dependent.                 */
  /*                                                                       */
  typedef FT_Pointer  FTC_FaceID;


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FTC_Face_Requester                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A callback function provided by client applications.  It is used   */
  /*    to translate a given @FTC_FaceID into a new valid @FT_Face object. */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face_id :: The face ID to resolve.                                 */
  /*                                                                       */
  /*    library :: A handle to a FreeType library object.                  */
  /*                                                                       */
  /*    data    :: Application-provided request data.                      */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A new @FT_Face handle.                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The face requester should not perform funny things on the returned */
  /*    face object, like creating a new @FT_Size for it, or setting a     */
  /*    transformation through @FT_Set_Transform!                          */
  /*                                                                       */
  typedef FT_Error
  (*FTC_Face_Requester)( FTC_FaceID  face_id,
                         FT_Library  library,
                         FT_Pointer  request_data,
                         FT_Face*    aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_FontRec                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to describe a given `font' to the cache    */
  /*    manager.  Note that a `font' is the combination of a given face    */
  /*    with a given character size.                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face_id    :: The ID of the face to use.                           */
  /*                                                                       */
  /*    pix_width  :: The character width in integer pixels.               */
  /*                                                                       */
  /*    pix_height :: The character height in integer pixels.              */
  /*                                                                       */
  typedef struct  FTC_FontRec_
  {
    FTC_FaceID  face_id;
    FT_UShort   pix_width;
    FT_UShort   pix_height;

  } FTC_FontRec;


  /* */


#define FTC_FONT_COMPARE( f1, f2 )                  \
          ( (f1)->face_id    == (f2)->face_id    && \
            (f1)->pix_width  == (f2)->pix_width  && \
            (f1)->pix_height == (f2)->pix_height )

#define FTC_FACE_ID_HASH( i )  ((FT_UInt32)(FT_Pointer)( i ))

#define FTC_FONT_HASH( f )                              \
          (FT_UInt32)( FTC_FACE_ID_HASH((f)->face_id) ^ \
                       ((f)->pix_width << 8)          ^ \
                       ((f)->pix_height)              )


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Font                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple handle to an @FTC_FontRec structure.                      */
  /*                                                                       */
  typedef FTC_FontRec*  FTC_Font;


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      CACHE MANAGER OBJECT                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Manager                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This object is used to cache one or more @FT_Face objects, along   */
  /*    with corresponding @FT_Size objects.                               */
  /*                                                                       */
  typedef struct FTC_ManagerRec_*  FTC_Manager;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Node                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to a cache node object.  Each cache node is       */
  /*    reference-counted.  A node with a count of 0 might be flushed      */
  /*    out of a full cache whenever a lookup request is performed.        */
  /*                                                                       */
  /*    If you lookup nodes, you have the ability to "acquire" them, i.e., */
  /*    to increment their reference count.  This will prevent the node    */
  /*    from being flushed out of the cache until you explicitly "release" */
  /*    it (see @FTC_Node_Release).                                        */
  /*                                                                       */
  /*    See also @FTC_BitsetCache_Lookup and @FTC_ImageCache_Lookup.       */
  /*                                                                       */
  typedef struct FTC_NodeRec_*  FTC_Node;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_New                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new cache manager.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library   :: The parent FreeType library handle to use.            */
  /*                                                                       */
  /*    max_faces :: Maximum number of faces to keep alive in manager.     */
  /*                 Use 0 for defaults.                                   */
  /*                                                                       */
  /*    max_sizes :: Maximum number of sizes to keep alive in manager.     */
  /*                 Use 0 for defaults.                                   */
  /*                                                                       */
  /*    max_bytes :: Maximum number of bytes to use for cached data.       */
  /*                 Use 0 for defaults.                                   */
  /*                                                                       */
  /*    requester :: An application-provided callback used to translate    */
  /*                 face IDs into real @FT_Face objects.                  */
  /*                                                                       */
  /*    req_data  :: A generic pointer that is passed to the requester     */
  /*                 each time it is called (see @FTC_Face_Requester).     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    amanager  :: A handle to a new manager object.  0 in case of       */
  /*                 failure.                                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Manager_New( FT_Library          library,
                   FT_UInt             max_faces,
                   FT_UInt             max_sizes,
                   FT_ULong            max_bytes,
                   FTC_Face_Requester  requester,
                   FT_Pointer          req_data,
                   FTC_Manager        *amanager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Reset                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Empties a given cache manager.  This simply gets rid of all the    */
  /*    currently cached @FT_Face and @FT_Size objects within the manager. */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    manager :: A handle to the manager.                                */
  /*                                                                       */
  FT_EXPORT( void )
  FTC_Manager_Reset( FTC_Manager  manager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Done                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given manager after emptying it.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the target cache manager object.            */
  /*                                                                       */
  FT_EXPORT( void )
  FTC_Manager_Done( FTC_Manager  manager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Lookup_Face                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the @FT_Face object that corresponds to a given face ID  */
  /*    through a cache manager.                                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /*    face_id :: The ID of the face object.                              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A handle to the face object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned @FT_Face object is always owned by the manager.  You  */
  /*    should never try to discard it yourself.                           */
  /*                                                                       */
  /*    The @FT_Face object doesn't necessarily have a current size object */
  /*    (i.e., face->size can be 0).  If you need a specific `font size',  */
  /*    use @FTC_Manager_Lookup_Size instead.                              */
  /*                                                                       */
  /*    Never change the face's transformation matrix (i.e., never call    */
  /*    the @FT_Set_Transform function) on a returned face!  If you need   */
  /*    to transform glyphs, do it yourself after glyph loading.           */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Manager_Lookup_Face( FTC_Manager  manager,
                           FTC_FaceID   face_id,
                           FT_Face     *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Lookup_Size                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the @FT_Face and @FT_Size objects that correspond to a   */
  /*    given @FTC_SizeID.                                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /*    size_id :: The ID of the `font size' to use.                       */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aface   :: A pointer to the handle of the face object.  Set it to  */
  /*               zero if you don't need it.                              */
  /*                                                                       */
  /*    asize   :: A pointer to the handle of the size object.  Set it to  */
  /*               zero if you don't need it.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned @FT_Face object is always owned by the manager.  You  */
  /*    should never try to discard it yourself.                           */
  /*                                                                       */
  /*    Never change the face's transformation matrix (i.e., never call    */
  /*    the @FT_Set_Transform function) on a returned face!  If you need   */
  /*    to transform glyphs, do it yourself after glyph loading.           */
  /*                                                                       */
  /*    Similarly, the returned @FT_Size object is always owned by the     */
  /*    manager.  You should never try to discard it, and never change its */
  /*    settings with @FT_Set_Pixel_Sizes or @FT_Set_Char_Size!            */
  /*                                                                       */
  /*    The returned size object is the face's current size, which means   */
  /*    that you can call @FT_Load_Glyph with the face if you need to.     */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Manager_Lookup_Size( FTC_Manager  manager,
                           FTC_Font     font,
                           FT_Face     *aface,
                           FT_Size     *asize );


FT_END_HEADER

#endif /* __FTCACHE_H__ */


/* END */
