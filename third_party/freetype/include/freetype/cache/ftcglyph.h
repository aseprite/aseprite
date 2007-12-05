/***************************************************************************/
/*                                                                         */
/*  ftcglyph.h                                                             */
/*                                                                         */
/*    FreeType abstract glyph cache (specification).                       */
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
  /* Important: The functions defined in this file are only used to        */
  /*            implement an abstract glyph cache class.  You need to      */
  /*            provide additional logic to implement a complete cache.    */
  /*            For example, see `ftcimage.h' and `ftcimage.c' which       */
  /*            implement a FT_Glyph cache based on this code.             */
  /*                                                                       */
  /*  NOTE: For now, each glyph set is implemented as a static hash table. */
  /*        It would be interesting to experiment with dynamic hashes to   */
  /*        see whether this improves performance or not (I don't know why */
  /*        but something tells me it won't).                              */
  /*                                                                       */
  /*        In all cases, this change should not affect any derived glyph  */
  /*        cache class.                                                   */
  /*                                                                       */
  /*************************************************************************/


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


#ifndef __FTCGLYPH_H__
#define __FTCGLYPH_H__


#include <ft2build.h>
#include FT_CACHE_H
#include FT_CACHE_MANAGER_H

#include <stddef.h>


FT_BEGIN_HEADER


  /* each glyph set is characterized by a "glyph set type" which must be */
  /* defined by sub-classes                                              */
  typedef struct FTC_GlyphFamilyRec_*  FTC_GlyphFamily;

  /* handle to a glyph cache node */
  typedef struct FTC_GlyphNodeRec_*  FTC_GlyphNode;


  /* size should be 24 + chunk size on 32-bit machines;                 */
  /* note that the node's hash is ((gfam->hash << 16) | glyph_index) -- */
  /* this _must_ be set properly by the glyph node initializer          */
  /*                                                                    */
  typedef struct  FTC_GlyphNodeRec_
  {
    FTC_NodeRec   node;
    FT_UShort     item_count;
    FT_UShort     item_start;

  } FTC_GlyphNodeRec;


#define FTC_GLYPH_NODE( x )    ( (FTC_GlyphNode)(x) )
#define FTC_GLYPH_NODE_P( x )  ( (FTC_GlyphNode*)(x) )


  typedef struct  FTC_GlyphQueryRec_
  {
    FTC_QueryRec  query;
    FT_UInt       gindex;

  } FTC_GlyphQueryRec, *FTC_GlyphQuery;


#define FTC_GLYPH_QUERY( x )  ( (FTC_GlyphQuery)(x) )


  /* a glyph set is used to categorize glyphs of a given type */
  typedef struct  FTC_GlyphFamilyRec_
  {
    FTC_FamilyRec  family;
    FT_UInt32      hash;
    FT_UInt        item_total;   /* total number of glyphs in family */
    FT_UInt        item_count;   /* number of glyph items per node   */

  } FTC_GlyphFamilyRec;


#define FTC_GLYPH_FAMILY( x )         ( (FTC_GlyphFamily)(x) )
#define FTC_GLYPH_FAMILY_P( x )       ( (FTC_GlyphFamily*)(x) )

#define FTC_GLYPH_FAMILY_MEMORY( x )  FTC_FAMILY(x)->cache->memory


  /* each glyph node contains a 'chunk' of glyph items; */
  /* translate a glyph index into a chunk index         */
#define FTC_GLYPH_FAMILY_CHUNK( gfam, gindex )                  \
          ( ( gindex ) / FTC_GLYPH_FAMILY( gfam )->item_count )

  /* find a glyph index's chunk, and return its start index */
#define FTC_GLYPH_FAMILY_START( gfam, gindex )       \
          ( FTC_GLYPH_FAMILY_CHUNK( gfam, gindex ) * \
            FTC_GLYPH_FAMILY( gfam )->item_count )

  /* compute a glyph request's hash value */
#define FTC_GLYPH_FAMILY_HASH( gfam, gindex )                         \
          ( (FT_UFast)(                                               \
              ( FTC_GLYPH_FAMILY( gfam )->hash << 16 ) |              \
              ( FTC_GLYPH_FAMILY_CHUNK( gfam, gindex ) & 0xFFFF ) ) )

  /* must be called in an FTC_Family_CompareFunc to update the query */
  /* whenever a glyph set is matched in the lookup, or when it       */
  /* is created                                                      */
#define FTC_GLYPH_FAMILY_FOUND( gfam, gquery )                            \
          do                                                              \
          {                                                               \
            FTC_QUERY( gquery )->family = FTC_FAMILY( gfam );             \
            FTC_QUERY( gquery )->hash =                                   \
              FTC_GLYPH_FAMILY_HASH( gfam,                                \
                                     FTC_GLYPH_QUERY( gquery )->gindex ); \
          } while ( 0 )

  /* retrieve glyph index of glyph node */
#define FTC_GLYPH_NODE_GINDEX( x )                                 \
          ( (FT_UInt)( FTC_GLYPH_NODE( x )->node.hash & 0xFFFF ) )


  /*************************************************************************/
  /*                                                                       */
  /* These functions are exported so that they can be called from          */
  /* user-provided cache classes; otherwise, they are really part of the   */
  /* cache sub-system internals.                                           */
  /*                                                                       */

  /* must be called by derived FTC_Node_InitFunc routines */
  FT_EXPORT( void )
  ftc_glyph_node_init( FTC_GlyphNode    node,
                       FT_UInt          gindex,  /* glyph index for node */
                       FTC_GlyphFamily  gfam );

  /* returns TRUE iff the query's glyph index correspond to the node;  */
  /* this assumes that the "family" and "hash" fields of the query are */
  /* already correctly set                                             */
  FT_EXPORT( FT_Bool )
  ftc_glyph_node_compare( FTC_GlyphNode   gnode,
                          FTC_GlyphQuery  gquery );

  /* must be called by derived FTC_Node_DoneFunc routines */
  FT_EXPORT( void )
  ftc_glyph_node_done( FTC_GlyphNode  node,
                       FTC_Cache      cache );


  /* must be called by derived FTC_Family_InitFunc; */
  /* calls "ftc_family_init"                        */
  FT_EXPORT( FT_Error )
  ftc_glyph_family_init( FTC_GlyphFamily  gfam,
                         FT_UInt32        hash,
                         FT_UInt          item_count,
                         FT_UInt          item_total,
                         FTC_GlyphQuery   gquery,
                         FTC_Cache        cache );

  FT_EXPORT( void )
  ftc_glyph_family_done( FTC_GlyphFamily  gfam );


  /* */
 
FT_END_HEADER


#endif /* __FTCGLYPH_H__ */


/* END */
