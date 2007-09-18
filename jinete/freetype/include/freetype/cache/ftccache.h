/***************************************************************************/
/*                                                                         */
/*  ftccache.h                                                             */
/*                                                                         */
/*    FreeType internal cache interface (specification).                   */
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


#ifndef __FTCCACHE_H__
#define __FTCCACHE_H__


FT_BEGIN_HEADER

  /* handle to cache object */
  typedef struct FTC_CacheRec_*  FTC_Cache;

  /* handle to cache class */
  typedef const struct FTC_Cache_ClassRec_*  FTC_Cache_Class;

  /* handle to cache node family */
  typedef struct FTC_FamilyRec_*  FTC_Family;

  /* handle to cache root query */
  typedef struct FTC_QueryRec_*  FTC_Query;


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE NODE DEFINITIONS                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* Each cache controls one or more cache nodes.  Each node is part of    */
  /* the global_lru list of the manager.  Its `data' field however is used */
  /* as a reference count for now.                                         */
  /*                                                                       */
  /* A node can be anything, depending on the type of information held by  */
  /* the cache.  It can be an individual glyph image, a set of bitmaps     */
  /* glyphs for a given size, some metrics, etc.                           */
  /*                                                                       */
  /*************************************************************************/

  /* structure size should be 20 bytes on 32-bits machines */
  typedef struct  FTC_NodeRec_
  {
    FTC_Node   mru_next;     /* circular mru list pointer           */
    FTC_Node   mru_prev;     /* circular mru list pointer           */
    FTC_Node   link;         /* used for hashing                    */
    FT_UInt32  hash;         /* used for hashing too                */
    FT_UShort  fam_index;    /* index of family the node belongs to */
    FT_Short   ref_count;    /* reference count for this node       */

  } FTC_NodeRec;


#define FTC_NODE( x )    ( (FTC_Node)(x) )
#define FTC_NODE_P( x )  ( (FTC_Node*)(x) )


  /*************************************************************************/
  /*                                                                       */
  /* These functions are exported so that they can be called from          */
  /* user-provided cache classes; otherwise, they are really part of the   */
  /* cache sub-system internals.                                           */
  /*                                                                       */

  /* can be used as a FTC_Node_DoneFunc */
  FT_EXPORT( void )
  ftc_node_done( FTC_Node   node,
                 FTC_Cache  cache );

  /* reserved for manager's use */
  FT_EXPORT( void )
  ftc_node_destroy( FTC_Node     node,
                    FTC_Manager  manager );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE QUERY DEFINITIONS                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* A structure modelling a cache node query.  The following fields must  */
  /* all be set by the @FTC_Family_CompareFunc method of a cache's family  */
  /* list.                                                                 */
  /*                                                                       */
  typedef struct  FTC_QueryRec_
  {
    FTC_Family  family;
    FT_UFast    hash;

  } FTC_QueryRec;


#define FTC_QUERY( x )    ( (FTC_Query)(x) )
#define FTC_QUERY_P( x )  ( (FTC_Query*)(x) )


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE FAMILY DEFINITIONS                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct  FTC_FamilyRec_
  {
    FT_LruNodeRec  lru;
    FTC_Cache      cache;
    FT_UInt        num_nodes;
    FT_UInt        fam_index;

  } FTC_FamilyRec;


#define FTC_FAMILY( x )    ( (FTC_Family)(x) )
#define FTC_FAMILY_P( x )  ( (FTC_Family*)(x) )


  /*************************************************************************/
  /*                                                                       */
  /* These functions are exported so that they can be called from          */
  /* user-provided cache classes; otherwise, they are really part of the   */
  /* cache sub-system internals.                                           */
  /*                                                                       */

  /* must be called by any FTC_Node_InitFunc routine */
  FT_EXPORT( FT_Error )
  ftc_family_init( FTC_Family  family,
                   FTC_Query   query,
                   FTC_Cache   cache );


  /* can be used as a FTC_Family_DoneFunc; otherwise, must be called */
  /* by any family finalizer function                                */
  FT_EXPORT( void )
  ftc_family_done( FTC_Family  family );


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                       CACHE DEFINITIONS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* each cache really implements a dynamic hash table to manage its nodes */
  typedef struct  FTC_CacheRec_
  {
    FTC_Manager          manager;
    FT_Memory            memory;
    FTC_Cache_Class      clazz;

    FT_UInt              cache_index;  /* in manager's table         */
    FT_Pointer           cache_data;   /* used by cache node methods */

    FT_UFast             nodes;
    FT_UFast             size;
    FTC_Node*            buckets;

    FT_LruList_ClassRec  family_class;
    FT_LruList           families;

  } FTC_CacheRec;


#define FTC_CACHE( x )    ( (FTC_Cache)(x) )
#define FTC_CACHE_P( x )  ( (FTC_Cache*)(x) )


  /* initialize a given cache */
  typedef FT_Error
  (*FTC_Cache_InitFunc)( FTC_Cache  cache );

  /* clear a cache */
  typedef void
  (*FTC_Cache_ClearFunc)( FTC_Cache  cache );

  /* finalize a given cache */
  typedef void
  (*FTC_Cache_DoneFunc)( FTC_Cache  cache );


  typedef FT_Error
  (*FTC_Family_InitFunc)( FTC_Family  family,
                          FTC_Query   query,
                          FTC_Cache   cache );

  typedef FT_Int
  (*FTC_Family_CompareFunc)( FTC_Family  family,
                             FTC_Query   query );

  typedef void
  (*FTC_Family_DoneFunc)( FTC_Family  family,
                          FTC_Cache   cache );

  /* initialize a new cache node */
  typedef FT_Error
  (*FTC_Node_InitFunc)( FTC_Node    node,
                        FT_Pointer  type,
                        FTC_Cache   cache );

  /* compute the weight of a given cache node */
  typedef FT_ULong
  (*FTC_Node_WeightFunc)( FTC_Node   node,
                          FTC_Cache  cache );

  /* compare a node to a given key pair */
  typedef FT_Bool
  (*FTC_Node_CompareFunc)( FTC_Node    node,
                           FT_Pointer  key,
                           FTC_Cache   cache );

  /* finalize a given cache node */
  typedef void
  (*FTC_Node_DoneFunc)( FTC_Node   node,
                        FTC_Cache  cache );


  typedef struct  FTC_Cache_ClassRec_
  {
    FT_UInt                 cache_size;
    FTC_Cache_InitFunc      cache_init;
    FTC_Cache_ClearFunc     cache_clear;
    FTC_Cache_DoneFunc      cache_done;

    FT_UInt                 family_size;
    FTC_Family_InitFunc     family_init;
    FTC_Family_CompareFunc  family_compare;
    FTC_Family_DoneFunc     family_done;

    FT_UInt                 node_size;
    FTC_Node_InitFunc       node_init;
    FTC_Node_WeightFunc     node_weight;
    FTC_Node_CompareFunc    node_compare;
    FTC_Node_DoneFunc       node_done;

  } FTC_Cache_ClassRec;


  /* */


  /*************************************************************************/
  /*                                                                       */
  /* These functions are exported so that they can be called from          */
  /* user-provided cache classes; otherwise, they are really part of the   */
  /* cache sub-system internals.                                           */
  /*                                                                       */

  /* can be used directly as FTC_Cache_DoneFunc(), or called by custom */
  /* cache finalizers                                                  */
  FT_EXPORT( void )
  ftc_cache_done( FTC_Cache  cache );

  /* can be used directly as FTC_Cache_ClearFunc(), or called by custom */
  /* cache clear routines                                               */
  FT_EXPORT( void )
  ftc_cache_clear( FTC_Cache  cache );

  /* initalize the hash table within the cache */
  FT_EXPORT( FT_Error )
  ftc_cache_init( FTC_Cache  cache );

  /* can be called when the key's hash value has been computed */
  FT_EXPORT( FT_Error )
  ftc_cache_lookup( FTC_Cache  cache,
                    FTC_Query  query,
                    FTC_Node  *anode );

 /* */


FT_END_HEADER


#endif /* __FTCCACHE_H__ */


/* END */
