/***************************************************************************/
/*                                                                         */
/*  ftcsbits.c                                                             */
/*                                                                         */
/*    FreeType sbits manager (body).                                       */
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


#include <ft2build.h>
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H
#include FT_CACHE_INTERNAL_GLYPH_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_ERRORS_H

#include "ftcerror.h"

#include <string.h>         /* memcmp() */


#define FTC_SBIT_ITEMS_PER_NODE  16


  typedef struct FTC_SBitNodeRec_*  FTC_SBitNode;

  typedef struct  FTC_SBitNodeRec_
  {
    FTC_GlyphNodeRec  gnode;
    FTC_SBitRec       sbits[FTC_SBIT_ITEMS_PER_NODE];

  } FTC_SBitNodeRec;


#define FTC_SBIT_NODE( x )  ( (FTC_SBitNode)( x ) )


  typedef struct  FTC_SBitQueryRec_
  {
    FTC_GlyphQueryRec  gquery;
    FTC_ImageDesc      desc;

  } FTC_SBitQueryRec, *FTC_SBitQuery;


#define FTC_SBIT_QUERY( x ) ( (FTC_SBitQuery)( x ) )


  typedef struct FTC_SBitFamilyRec_*  FTC_SBitFamily;

  /* sbit family structure */
  typedef struct  FTC_SBitFamilyRec_
  {
    FTC_GlyphFamilyRec  gfam;
    FTC_ImageDesc       desc;

  } FTC_SBitFamilyRec;
 
  
#define FTC_SBIT_FAMILY( x )         ( (FTC_SBitFamily)( x ) )
#define FTC_SBIT_FAMILY_MEMORY( x )  FTC_GLYPH_FAMILY_MEMORY( &( x )->cset )


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     SBIT CACHE NODES                          *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  ftc_sbit_copy_bitmap( FTC_SBit    sbit,
                        FT_Bitmap*  bitmap,
                        FT_Memory   memory )
  {
    FT_Error  error;
    FT_Int    pitch = bitmap->pitch;
    FT_ULong  size;


    if ( pitch < 0 )
      pitch = -pitch;

    size = (FT_ULong)( pitch * bitmap->rows );

    if ( !ALLOC( sbit->buffer, size ) )
      MEM_Copy( sbit->buffer, bitmap->buffer, size );

    return error;
  }


  FT_CALLBACK_DEF( void )
  ftc_sbit_node_done( FTC_SBitNode  snode,
                      FTC_Cache     cache )
  {
    FTC_SBit   sbit  = snode->sbits;
    FT_UInt    count = FTC_GLYPH_NODE( snode )->item_count;
    FT_Memory  memory = cache->memory;


    for ( ; count > 0; sbit++, count-- )
      FREE( sbit->buffer );

    ftc_glyph_node_done( FTC_GLYPH_NODE( snode ), cache );
  }


  static FT_Error
  ftc_sbit_node_load( FTC_SBitNode    snode,
                      FTC_Manager     manager,
                      FTC_SBitFamily  sfam,
                      FT_UInt         gindex,
                      FT_ULong       *asize )
  {
    FT_Error       error;
    FTC_GlyphNode  gnode = FTC_GLYPH_NODE( snode );
    FT_Memory      memory;
    FT_Face        face;
    FT_Size        size;
    FTC_SBit       sbit;


    if ( gindex <  (FT_UInt)gnode->item_start                     ||
         gindex >= (FT_UInt)gnode->item_start + gnode->item_count )
    {
      FT_ERROR(( "ftc_sbit_node_load: invalid glyph index" ));
      return FTC_Err_Invalid_Argument;
    }

    memory = manager->library->memory;

    sbit = snode->sbits + ( gindex - gnode->item_start );

    error = FTC_Manager_Lookup_Size( manager, &sfam->desc.font,
                                     &face, &size );
    if ( !error )
    {
      FT_UInt  load_flags = FT_LOAD_DEFAULT;
      FT_UInt  type       = sfam->desc.type;


      /* determine load flags, depending on the font description's */
      /* image type                                                */

      if ( FTC_IMAGE_FORMAT( type ) == ftc_image_format_bitmap )
      {
        if ( type & ftc_image_flag_monochrome )
          load_flags |= FT_LOAD_MONOCHROME;

        /* disable embedded bitmaps loading if necessary */
        if ( type & ftc_image_flag_no_sbits )
          load_flags |= FT_LOAD_NO_BITMAP;
      }
      else
      {
        FT_ERROR((
          "ftc_sbit_node_load: cannot load scalable glyphs in an"
          " sbit cache, please check your arguments!\n" ));
        error = FTC_Err_Invalid_Argument;
        goto Exit;
      }

      /* always render glyphs to bitmaps */
      load_flags |= FT_LOAD_RENDER;

      if ( type & ftc_image_flag_unhinted )
        load_flags |= FT_LOAD_NO_HINTING;

      if ( type & ftc_image_flag_autohinted )
        load_flags |= FT_LOAD_FORCE_AUTOHINT;

      /* by default, indicates a `missing' glyph */
      sbit->buffer = 0;

      error = FT_Load_Glyph( face, gindex, load_flags );
      if ( !error )
      {
        FT_Int        temp;
        FT_GlyphSlot  slot   = face->glyph;
        FT_Bitmap*    bitmap = &slot->bitmap;
        FT_Int        xadvance, yadvance;


        /* check that our values fit into 8-bit containers!       */
        /* If this is not the case, our bitmap is too large       */
        /* and we will leave it as `missing' with sbit.buffer = 0 */

#define CHECK_CHAR( d )  ( temp = (FT_Char)d, temp == d )
#define CHECK_BYTE( d )  ( temp = (FT_Byte)d, temp == d )

        /* XXX: FIXME: add support for vertical layouts maybe */

        /* horizontal advance in pixels */
        xadvance = ( slot->metrics.horiAdvance + 32 ) >> 6;
        yadvance = ( slot->metrics.vertAdvance + 32 ) >> 6;

        if ( CHECK_BYTE( bitmap->rows  )     &&
             CHECK_BYTE( bitmap->width )     &&
             CHECK_CHAR( bitmap->pitch )     &&
             CHECK_CHAR( slot->bitmap_left ) &&
             CHECK_CHAR( slot->bitmap_top  ) &&
             CHECK_CHAR( xadvance )          &&
             CHECK_CHAR( yadvance )          )
        {
          sbit->width    = (FT_Byte)bitmap->width;
          sbit->height   = (FT_Byte)bitmap->rows;
          sbit->pitch    = (FT_Char)bitmap->pitch;
          sbit->left     = (FT_Char)slot->bitmap_left;
          sbit->top      = (FT_Char)slot->bitmap_top;
          sbit->xadvance = (FT_Char)xadvance;
          sbit->yadvance = (FT_Char)yadvance;
          sbit->format   = (FT_Byte)bitmap->pixel_mode;

          /* grab the bitmap when possible - this is a hack !! */
          if ( slot->flags & ft_glyph_own_bitmap )
          {
            slot->flags &= ~ft_glyph_own_bitmap;
            sbit->buffer = bitmap->buffer;
          }
          else
          {
            /* copy the bitmap into a new buffer -- ignore error */
            error = ftc_sbit_copy_bitmap( sbit, bitmap, memory );
          }

          /* now, compute size */
          if ( asize )
            *asize = ABS( sbit->pitch ) * sbit->height;

        }  /* glyph dimensions ok */

      } /* glyph loading successful */

      /* ignore the errors that might have occurred --   */
      /* we mark unloaded glyphs with `sbit.buffer == 0' */
      /* and 'width == 255', 'height == 0'               */
      /*                                                 */
      if ( error )
      {
        sbit->width = 255;
        error       = 0;
        /* sbit->buffer == NULL too! */
      }
    }

  Exit:
    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_sbit_node_init( FTC_SBitNode    snode,
                      FTC_GlyphQuery  gquery,
                      FTC_Cache       cache )
  {
    FT_Error  error;


    ftc_glyph_node_init( FTC_GLYPH_NODE( snode ),
                         gquery->gindex,
                         FTC_GLYPH_FAMILY( gquery->query.family ) );

    error = ftc_sbit_node_load( snode,
                                cache->manager,
                                FTC_SBIT_FAMILY( FTC_QUERY( gquery )->family ),
                                gquery->gindex,
                                NULL );
    if ( error )
      ftc_glyph_node_done( FTC_GLYPH_NODE( snode ), cache );

    return error;
  }


  FT_CALLBACK_DEF( FT_ULong )
  ftc_sbit_node_weight( FTC_SBitNode  snode )
  {
    FTC_GlyphNode  gnode = FTC_GLYPH_NODE( snode );
    FT_UInt        count = gnode->item_count;
    FTC_SBit       sbit  = snode->sbits;
    FT_Int         pitch;
    FT_ULong       size;


    /* the node itself */
    size = sizeof ( *snode );

    /* the sbit records */
    size += FTC_GLYPH_NODE( snode )->item_count * sizeof ( FTC_SBitRec );

    for ( ; count > 0; count--, sbit++ )
    {
      if ( sbit->buffer )
      {
        pitch = sbit->pitch;
        if ( pitch < 0 )
          pitch = -pitch;

        /* add the size of a given glyph image */
        size += pitch * sbit->height;
      }
    }

    return size;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_sbit_node_compare( FTC_SBitNode   snode,
                         FTC_SBitQuery  squery,
                         FTC_Cache      cache )
  {
    FTC_GlyphQuery  gquery = FTC_GLYPH_QUERY( squery );
    FTC_GlyphNode   gnode  = FTC_GLYPH_NODE( snode );
    FT_Bool         result;


    result = ftc_glyph_node_compare( gnode, gquery );
    if ( result )
    {
      /* check if we need to load the glyph bitmap now */
      FT_UInt   gindex = gquery->gindex;
      FTC_SBit  sbit   = snode->sbits + ( gindex - gnode->item_start );


      if ( sbit->buffer == NULL && sbit->width != 255 )
      {
        FT_ULong  size;


        /* yes, it's safe to ignore errors here */
        ftc_sbit_node_load( snode,
                            cache->manager,
                            FTC_SBIT_FAMILY( FTC_QUERY( squery )->family ),
                            gindex,
                            &size );

        cache->manager->cur_weight += size;
      }
    }

    return result;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     SBITS FAMILIES                            *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_CALLBACK_DEF( FT_Error )
  ftc_sbit_family_init( FTC_SBitFamily  sfam,
                        FTC_SBitQuery   squery,
                        FTC_Cache       cache )
  {
    FTC_Manager  manager = cache->manager;
    FT_Error     error;
    FT_Face      face;


    sfam->desc = squery->desc;

    /* we need to compute "cquery.item_total" now */
    error = FTC_Manager_Lookup_Face( manager,
                                     squery->desc.font.face_id,
                                     &face );
    if ( !error )
    {
      error = ftc_glyph_family_init( FTC_GLYPH_FAMILY( sfam ),
                                     FTC_IMAGE_DESC_HASH( &sfam->desc ),
                                     FTC_SBIT_ITEMS_PER_NODE,
                                     face->num_glyphs,
                                     FTC_GLYPH_QUERY( squery ),
                                     cache );
    }

    return error;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_sbit_family_compare( FTC_SBitFamily  sfam,
                           FTC_SBitQuery   squery )
  {
    FT_Bool  result;


    /* we need to set the "cquery.cset" field or our query for */
    /* faster glyph comparisons in ftc_sbit_node_compare       */
    /*                                                         */
    result = FT_BOOL( FTC_IMAGE_DESC_COMPARE( &sfam->desc, &squery->desc ) );
    if ( result )
      FTC_GLYPH_FAMILY_FOUND( sfam, squery );

    return result;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                     SBITS CACHE                               *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_CALLBACK_TABLE_DEF
  const FTC_Cache_ClassRec  ftc_sbit_cache_class =
  {
    sizeof ( FTC_CacheRec ),
    (FTC_Cache_InitFunc) ftc_cache_init,
    (FTC_Cache_ClearFunc)ftc_cache_clear,
    (FTC_Cache_DoneFunc) ftc_cache_done,

    sizeof ( FTC_SBitFamilyRec ),
    (FTC_Family_InitFunc)   ftc_sbit_family_init,
    (FTC_Family_CompareFunc)ftc_sbit_family_compare,
    (FTC_Family_DoneFunc)   ftc_glyph_family_done,

    sizeof ( FTC_SBitNodeRec ),
    (FTC_Node_InitFunc)   ftc_sbit_node_init,
    (FTC_Node_WeightFunc) ftc_sbit_node_weight,
    (FTC_Node_CompareFunc)ftc_sbit_node_compare,
    (FTC_Node_DoneFunc)   ftc_sbit_node_done
  };


  /* documentation is in ftcsbits.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_SBitCache_New( FTC_Manager     manager,
                     FTC_SBitCache  *acache )
  {
    return FTC_Manager_Register_Cache( manager,
                                       &ftc_sbit_cache_class,
                                       (FTC_Cache*)acache );
  }


  /* documentation is in ftcsbits.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_SBitCache_Lookup( FTC_SBitCache   cache,
                        FTC_ImageDesc*  desc,
                        FT_UInt         gindex,
                        FTC_SBit       *ansbit,
                        FTC_Node       *anode )
  {
    FT_Error          error;
    FTC_SBitQueryRec  squery;
    FTC_SBitNode      node;


    /* other argument checks delayed to ftc_cache_lookup */
    if ( !ansbit )
      return FTC_Err_Invalid_Argument;

    *ansbit = NULL;

    if ( anode )
      *anode = NULL;

    squery.gquery.gindex = gindex;
    squery.desc          = *desc;

    error = ftc_cache_lookup( FTC_CACHE( cache ),
                              FTC_QUERY( &squery ),
                              (FTC_Node*)&node );
    if ( !error )
    {
      *ansbit = node->sbits + ( gindex - FTC_GLYPH_NODE( node )->item_start );

      if ( anode )
      {
        *anode = FTC_NODE( node );
        FTC_NODE( node )->ref_count++;
      }
    }
    return error;
  }


  /* backwards-compatibility functions */

  FT_EXPORT_DEF( FT_Error )
  FTC_SBit_Cache_New( FTC_Manager      manager,
                      FTC_SBit_Cache  *acache )
  {
    return FTC_SBitCache_New( manager, (FTC_SBitCache*)acache );
  }


  FT_EXPORT_DEF( FT_Error )
  FTC_SBit_Cache_Lookup( FTC_SBit_Cache   cache,
                         FTC_Image_Desc*  desc,
                         FT_UInt          gindex,
                         FTC_SBit        *ansbit )
  {
    FTC_ImageDesc  desc0;
    

    if ( !desc )
      return FT_Err_Invalid_Argument;
      
    desc0.font = desc->font;
    desc0.type = (FT_UInt32)desc->image_type;

    return FTC_SBitCache_Lookup( (FTC_SBitCache)cache,
                                  &desc0,
                                  gindex,
                                  ansbit,
                                  NULL );
  }


/* END */
