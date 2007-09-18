/***************************************************************************/
/*                                                                         */
/*  ftcimage.h                                                             */
/*                                                                         */
/*    FreeType Image cache (specification).                                */
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
  /* Each image cache really manages FT_Glyph objects.                     */
  /*                                                                       */
  /*************************************************************************/


#ifndef __FTCIMAGE_H__
#define __FTCIMAGE_H__


#include <ft2build.h>
#include FT_CACHE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    cache_subsystem                                                    */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       IMAGE CACHE OBJECT                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define FTC_IMAGE_FORMAT( x )  ( (x) & 7 )


#define ftc_image_format_bitmap    0x0000
#define ftc_image_format_outline   0x0001

#define ftc_image_format_mask      0x000F

#define ftc_image_flag_monochrome  0x0010
#define ftc_image_flag_unhinted    0x0020
#define ftc_image_flag_autohinted  0x0040
#define ftc_image_flag_unscaled    0x0080
#define ftc_image_flag_no_sbits    0x0100

  /* monochrome bitmap */
#define ftc_image_mono             ftc_image_format_bitmap | \
                                   ftc_image_flag_monochrome

  /* anti-aliased bitmap */
#define ftc_image_grays            ftc_image_format_bitmap

  /* scaled outline */
#define ftc_image_outline          ftc_image_format_outline


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_ImageDesc                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to describe a given glyph image category.  */
  /*    Note that this is different from @FTC_Image_Desc.                  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    size    :: An @FTC_SizeRec used to describe the glyph's face and   */
  /*               size.                                                   */
  /*                                                                       */
  /*    type    :: The glyph image's type.  Note that it is a 32-bit uint. */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This type deprecates @FTC_Image_Desc.                              */
  /*                                                                       */
  typedef struct  FTC_ImageDesc_
  {
    FTC_FontRec  font;
    FT_UInt32    type;

  } FTC_ImageDesc;

 /* */

#define FTC_IMAGE_DESC_COMPARE( d1, d2 )                    \
          ( FTC_FONT_COMPARE( &(d1)->font, &(d2)->font ) && \
            (d1)->type == (d2)->type                     )

#define FTC_IMAGE_DESC_HASH( d )                    \
          (FT_UFast)( FTC_FONT_HASH( &(d)->font ) ^ \
                      ( (d)->type << 4 )          )


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_ImageCache                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to an glyph image cache object.  They are designed to     */
  /*    hold many distinct glyph images while not exceeding a certain      */
  /*    memory threshold.                                                  */
  /*                                                                       */
  typedef struct FTC_ImageCacheRec_*  FTC_ImageCache;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_ImageCache_New                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new glyph image cache.                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: The parent manager for the image cache.                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    acache  :: A handle to the new glyph image cache object.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_ImageCache_New( FTC_Manager      manager,
                      FTC_ImageCache  *acache );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_ImageCache_Lookup                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves a given glyph image from a glyph image cache.            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    cache  :: A handle to the source glyph image cache.                */
  /*                                                                       */
  /*    desc   :: A pointer to a glyph image descriptor.                   */
  /*                                                                       */
  /*    gindex :: The glyph index to retrieve.                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aglyph :: The corresponding @FT_Glyph object.  0 in case of        */
  /*              failure.                                                 */
  /*                                                                       */
  /*    anode  :: Used to return the address of of the corresponding cache */
  /*              node after incrementing its reference count (see note    */
  /*              below).                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned glyph is owned and managed by the glyph image cache.  */
  /*    Never try to transform or discard it manually!  You can however    */
  /*    create a copy with @FT_Glyph_Copy and modify the new one.          */
  /*                                                                       */
  /*    If "anode" is _not_ NULL, it receives the address of the cache     */
  /*    node containing the glyph image, after increasing its reference    */
  /*    count.  This ensures that the node (as well as the FT_Glyph) will  */
  /*    always be kept in the cache until you call @FTC_Node_Unref to      */
  /*    "release" it.                                                      */
  /*                                                                       */
  /*    If "anode" is NULL, the cache node is left unchanged, which means  */
  /*    that the FT_Glyph could be flushed out of the cache on the next    */
  /*    call to one of the caching sub-system APIs.  Don't assume that it  */
  /*    is persistent!                                                     */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_ImageCache_Lookup( FTC_ImageCache  cache,
                         FTC_ImageDesc*  desc,
                         FT_UInt         gindex,
                         FT_Glyph       *aglyph,
                         FTC_Node       *anode );

  /* */


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_Image_Desc                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    THIS TYPE IS DEPRECATED.  Use @FTC_ImageDesc instead.              */
  /*                                                                       */
  /*    A simple structure used to describe a given glyph image category.  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    size       :: An @FTC_SizeRec used to describe the glyph's face    */
  /*                  and size.                                            */
  /*                                                                       */
  /*    image_type :: The glyph image's type.                              */
  /*                                                                       */
  typedef struct  FTC_Image_Desc_
  {
    FTC_FontRec  font;
    FT_UInt      image_type;

  } FTC_Image_Desc;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FTC_Image_Cache                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    THIS TYPE IS DEPRECATED.  Use @FTC_ImageCache instead.             */
  /*                                                                       */
  typedef FTC_ImageCache  FTC_Image_Cache;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Image_Cache_New                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    THIS FUNCTION IS DEPRECATED.  Use @FTC_ImageCache_New instead.     */
  /*                                                                       */
  /*    Creates a new glyph image cache.                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    manager :: The parent manager for the image cache.                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    acache  :: A handle to the new glyph image cache object.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Image_Cache_New( FTC_Manager       manager,
                       FTC_Image_Cache  *acache );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Image_Cache_Lookup                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    THIS FUNCTION IS DEPRECATED.  Use @FTC_ImageCache_Lookup instead.  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    cache  :: A handle to the source glyph image cache.                */
  /*                                                                       */
  /*    desc   :: A pointer to a glyph image descriptor.                   */
  /*                                                                       */
  /*    gindex :: The glyph index to retrieve.                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    aglyph :: The corresponding @FT_Glyph object.  0 in case of        */
  /*              failure.                                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned glyph is owned and managed by the glyph image cache.  */
  /*    Never try to transform or discard it manually!  You can however    */
  /*    create a copy with @FT_Glyph_Copy and modify the new one.          */
  /*                                                                       */
  /*    Because the glyph image cache limits the total amount of memory    */
  /*    taken by the glyphs it holds, the returned glyph might disappear   */
  /*    on a later invocation of this function!  It is a cache after       */
  /*    all...                                                             */
  /*                                                                       */
  /*    Use this function to "lock" the glyph as long as it is needed.     */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FTC_Image_Cache_Lookup( FTC_Image_Cache  cache,
                          FTC_Image_Desc*  desc,
                          FT_UInt          gindex,
                          FT_Glyph        *aglyph );

 /* */

FT_END_HEADER


#endif /* __FTCIMAGE_H__ */


/* END */
