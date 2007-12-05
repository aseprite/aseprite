/***************************************************************************/
/*                                                                         */
/*  ftccmap.h                                                              */
/*                                                                         */
/*    FreeType charmap cache (specification).                              */
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


#ifndef __FTCCMAP_H__
#define __FTCCMAP_H__

#include <ft2build.h>
#include FT_CACHE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* @type:                                                                */
  /*    FTC_CmapCache                                                      */
  /*                                                                       */
  /* @description:                                                         */
  /*    An opaque handle used to manager a charmap cache.  This cache is   */
  /*    to hold character codes -> glyph indices mappings.                 */
  /*                                                                       */
  typedef struct FTC_CMapCacheRec_*  FTC_CMapCache;


  /*************************************************************************/
  /*                                                                       */
  /* @type:                                                                */
  /*    FTC_CMapDesc                                                       */
  /*                                                                       */
  /* @description:                                                         */
  /*    A handle to an @FTC_CMapDescRec structure used to describe a given */
  /*    charmap in a charmap cache.                                        */
  /*                                                                       */
  /*    Each @FTC_CMapDesc describes which charmap (of which @FTC_Face) we */
  /*    want to use in @FTC_CMapCache_Lookup.                              */
  /*                                                                       */
  typedef struct FTC_CMapDescRec_*  FTC_CMapDesc;


  /*************************************************************************/
  /*                                                                       */
  /* @enum:                                                                */
  /*    FTC_CMapType                                                       */
  /*                                                                       */
  /* @description:                                                         */
  /*    The list of valid @FTC_CMap types.  They indicate how we want to   */
  /*    address a charmap within an @FTC_FaceID.                           */
  /*                                                                       */
  /* @values:                                                              */
  /*    FTC_CMAP_BY_INDEX ::                                               */
  /*      Address a charmap by its index in the corresponding @FT_Face.    */
  /*                                                                       */
  /*    FTC_CMAP_BY_ENCODING ::                                            */
  /*      Use a @FT_Face charmap that corresponds to a given encoding.     */
  /*                                                                       */
  /*    FTC_CMAP_BY_ID ::                                                  */
  /*      Use an @FT_Face charmap that corresponds to a given              */
  /*      (platform,encoding) ID.  See @FTC_CMapIdRec.                     */
  /*                                                                       */
  typedef enum  FTC_CMapType_
  {
    FTC_CMAP_BY_INDEX    = 0,
    FTC_CMAP_BY_ENCODING = 1,
    FTC_CMAP_BY_ID       = 2

  } FTC_CMapType;


  /*************************************************************************/
  /*                                                                       */
  /* @struct:                                                              */
  /*    FTC_CMapIdRec                                                      */
  /*                                                                       */
  /* @description:                                                         */
  /*    A short structure to identify a charmap by a (platform,encoding)   */
  /*    pair of values.                                                    */
  /*                                                                       */
  /* @fields:                                                              */
  /*    platform :: The platform ID.                                       */
  /*                                                                       */
  /*    encoding :: The encoding ID.                                       */
  /*                                                                       */
  typedef struct  FTC_CMapIdRec_
  {
    FT_UInt  platform;
    FT_UInt  encoding;

  } FTC_CMapIdRec;


  /*************************************************************************/
  /*                                                                       */
  /* @struct:                                                              */
  /*    FTC_CMapDescRec                                                    */
  /*                                                                       */
  /* @description:                                                         */
  /*    A structure to describe a given charmap to @FTC_CMapCache.         */
  /*                                                                       */
  /* @fields:                                                              */
  /*    face_id    :: @FTC_FaceID of the face this charmap belongs to.     */
  /*                                                                       */
  /*    type       :: The type of charmap, see @FTC_CMapType.              */
  /*                                                                       */
  /*    u.index    :: For @FTC_CMAP_BY_INDEX types, this is the charmap    */
  /*                  index (within a @FT_Face) we want to use.            */
  /*                                                                       */
  /*    u.encoding :: For @FTC_CMAP_BY_ENCODING types, this is the charmap */
  /*                  encoding we want to use. see @FT_Encoding.           */
  /*                                                                       */
  /*    u.id       :: For @FTC_CMAP_BY_ID types, this is the               */
  /*                  (platform,encoding) pair we want to use. see         */
  /*                  @FTC_CMapIdRec and @FT_CharMapRec.                   */
  /*                                                                       */
  typedef struct  FTC_CMapDescRec_
  {
    FTC_FaceID    face_id;
    FTC_CMapType  type;

    union
    {
      FT_UInt        index;
      FT_Encoding    encoding;
      FTC_CMapIdRec  id;

    } u;

  } FTC_CMapDescRec;


  /*************************************************************************/
  /*                                                                       */
  /* @function:                                                            */
  /*    FTC_CMapCache_New                                                  */
  /*                                                                       */
  /* @description:                                                         */
  /*    Creates a new charmap cache.                                       */
  /*                                                                       */
  /* @input:                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /* @output:                                                              */
  /*    acache  :: A new cache handle.  NULL in case of error.             */
  /*                                                                       */
  /* @return:                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* @note:                                                                */
  /*    Like all other caches, this one will be destroyed with the cache   */
  /*    manager.                                                           */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_CMapCache_New( FTC_Manager     manager,
                     FTC_CMapCache  *acache );


  /*************************************************************************/
  /*                                                                       */
  /* @function:                                                            */
  /*    FTC_CMapCache_Lookup                                               */
  /*                                                                       */
  /* @description:                                                         */
  /*    Translates a character code into a glyph index, using the charmap  */
  /*    cache.                                                             */
  /*                                                                       */
  /* @input:                                                               */
  /*    cache     :: A charmap cache handle.                               */
  /*                                                                       */
  /*    cmap_desc :: A charmap descriptor handle.                          */
  /*                                                                       */
  /*    char_code :: The character code (in the corresponding charmap).    */
  /*                                                                       */
  /* @return:                                                              */
  /*    Glyph index.  0 means "no glyph".                                  */
  /*                                                                       */
  /* @note:                                                                */
  /*    This function doesn't return @FTC_Node handles, since there is no  */
  /*    real use for them with typical uses of charmaps.                   */
  /*                                                                       */
  FT_EXPORT( FT_UInt )
  FTC_CMapCache_Lookup( FTC_CMapCache  cache,
                        FTC_CMapDesc   cmap_desc,
                        FT_UInt32      char_code );

  /* */


FT_END_HEADER


#endif /* __FTCCMAP_H__ */


/* END */
