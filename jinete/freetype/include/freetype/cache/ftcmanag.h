/***************************************************************************/
/*                                                                         */
/*  ftcmanag.h                                                             */
/*                                                                         */
/*    FreeType Cache Manager (specification).                              */
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
  /* A cache manager is in charge of the following:                        */
  /*                                                                       */
  /*  - Maintain a mapping between generic FTC_FaceIDs and live FT_Face    */
  /*    objects.  The mapping itself is performed through a user-provided  */
  /*    callback.  However, the manager maintains a small cache of FT_Face */
  /*    and FT_Size objects in order to speed up things considerably.      */
  /*                                                                       */
  /*  - Manage one or more cache objects.  Each cache is in charge of      */
  /*    holding a varying number of `cache nodes'.  Each cache node        */
  /*    represents a minimal amount of individually accessible cached      */
  /*    data.  For example, a cache node can be an FT_Glyph image          */
  /*    containing a vector outline, or some glyph metrics, or anything    */
  /*    else.                                                              */
  /*                                                                       */
  /*    Each cache node has a certain size in bytes that is added to the   */
  /*    total amount of `cache memory' within the manager.                 */
  /*                                                                       */
  /*    All cache nodes are located in a global LRU list, where the oldest */
  /*    node is at the tail of the list.                                   */
  /*                                                                       */
  /*    Each node belongs to a single cache, and includes a reference      */
  /*    count to avoid destroying it (due to caching).                     */
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


#ifndef __FTCMANAG_H__
#define __FTCMANAG_H__


#include <ft2build.h>
#include FT_CACHE_H
#include FT_CACHE_INTERNAL_LRU_H
#include FT_CACHE_INTERNAL_CACHE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    cache_subsystem                                                    */
  /*                                                                       */
  /*************************************************************************/


#define FTC_MAX_FACES_DEFAULT  2
#define FTC_MAX_SIZES_DEFAULT  4
#define FTC_MAX_BYTES_DEFAULT  200000L  /* ~200kByte by default */

  /* maximum number of caches registered in a single manager */
#define FTC_MAX_CACHES         16


  typedef struct  FTC_FamilyEntryRec_
  {
    FTC_Family  family;
    FTC_Cache   cache;
    FT_UInt     index;
    FT_UInt     link;

  } FTC_FamilyEntryRec, *FTC_FamilyEntry;


#define FTC_FAMILY_ENTRY_NONE  ( (FT_UInt)-1 )


  typedef struct  FTC_FamilyTableRec_
  {
    FT_UInt          count;
    FT_UInt          size;
    FTC_FamilyEntry  entries;
    FT_UInt          free;
  
  } FTC_FamilyTableRec, *FTC_FamilyTable;


  FT_EXPORT( FT_Error )
  ftc_family_table_alloc( FTC_FamilyTable   table,
                          FT_Memory         memory,
                          FTC_FamilyEntry  *aentry );

  FT_EXPORT( void )
  ftc_family_table_free( FTC_FamilyTable  table,
                         FT_UInt          index );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FTC_ManagerRec                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The cache manager structure.                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    library      :: A handle to a FreeType library instance.           */
  /*                                                                       */
  /*    faces_list   :: The lru list of @FT_Face objects in the cache.     */
  /*                                                                       */
  /*    sizes_list   :: The lru list of @FT_Size objects in the cache.     */
  /*                                                                       */
  /*    max_weight   :: The maximum cache pool weight.                     */
  /*                                                                       */
  /*    cur_weight   :: The current cache pool weight.                     */
  /*                                                                       */
  /*    num_nodes    :: The current number of nodes in the manager.        */
  /*                                                                       */
  /*    nodes_list   :: The global lru list of all cache nodes.            */
  /*                                                                       */
  /*    caches       :: A table of installed/registered cache objects.     */
  /*                                                                       */
  /*    request_data :: User-provided data passed to the requester.        */
  /*                                                                       */
  /*    request_face :: User-provided function used to implement a mapping */
  /*                    between abstract @FTC_FaceID values and real       */
  /*                    @FT_Face objects.                                  */
  /*                                                                       */
  /*    families     :: Global table of families.                          */
  /*                                                                       */
  typedef struct  FTC_ManagerRec_
  {
    FT_Library          library;
    FT_LruList          faces_list;
    FT_LruList          sizes_list;

    FT_ULong            max_weight;
    FT_ULong            cur_weight;
    
    FT_UInt             num_nodes;
    FTC_Node            nodes_list;
    
    FTC_Cache           caches[FTC_MAX_CACHES];

    FT_Pointer          request_data;
    FTC_Face_Requester  request_face;

    FTC_FamilyTableRec  families;

  } FTC_ManagerRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Manager_Compress                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to check the state of the cache manager if   */
  /*    its `num_bytes' field is greater than its `max_bytes' field.  It   */
  /*    will flush as many old cache nodes as possible (ignoring cache     */
  /*    nodes with a non-zero reference count).                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    manager :: A handle to the cache manager.                          */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Client applications should not call this function directly.  It is */
  /*    normally invoked by specific cache implementations.                */
  /*                                                                       */
  /*    The reason this function is exported is to allow client-specific   */
  /*    cache classes.                                                     */
  /*                                                                       */
  FT_EXPORT( void )
  FTC_Manager_Compress( FTC_Manager  manager );


  /* this must be used internally for the moment */
  FT_EXPORT( FT_Error )
  FTC_Manager_Register_Cache( FTC_Manager      manager,
                              FTC_Cache_Class  clazz,
                              FTC_Cache       *acache );


  /* can be called to increment a node's reference count */
  FT_EXPORT( void )
  FTC_Node_Ref( FTC_Node     node,
                FTC_Manager  manager );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FTC_Node_Unref                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Decrement a cache node's internal reference count.  When the count */
  /*    reaches 0, it is not destroyed but becomes eligible for subsequent */
  /*    cache flushes.                                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    node    :: The cache node handle.                                  */
  /*                                                                       */
  /*    manager :: The cache manager handle.                               */
  /*                                                                       */
  FT_EXPORT( void )
  FTC_Node_Unref( FTC_Node     node,
                  FTC_Manager  manager );

 /* */

FT_END_HEADER


#endif /* __FTCMANAG_H__ */


/* END */
