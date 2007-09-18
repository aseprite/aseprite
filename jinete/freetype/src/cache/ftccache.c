/***************************************************************************/
/*                                                                         */
/*  ftccache.c                                                             */
/*                                                                         */
/*    The FreeType internal cache interface (body).                        */
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
#include FT_CACHE_MANAGER_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H

#include "ftcerror.h"


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE NODE DEFINITIONS                      *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  FT_EXPORT_DEF( void )
  ftc_node_done( FTC_Node   node,
                 FTC_Cache  cache )
  {
    FTC_Family       family;
    FTC_FamilyEntry  entry;


    entry  = cache->manager->families.entries + node->fam_index;
    family = entry->family;

    /* remove from parent set table - eventually destroy the set */
    if ( --family->num_nodes <= 0 )
      FT_LruList_Remove( cache->families, (FT_LruNode) family );
  }


  /* add a new node to the head of the manager's circular MRU list */
  static void
  ftc_node_mru_link( FTC_Node     node,
                     FTC_Manager  manager )
  {
    FTC_Node  first = manager->nodes_list;


    if ( first )
    {
      node->mru_prev = first->mru_prev;
      node->mru_next = first;

      first->mru_prev->mru_next = node;
      first->mru_prev           = node;
    }
    else
    {
      node->mru_next = node;
      node->mru_prev = node;
    }

    manager->nodes_list = node;
    manager->num_nodes++;
  }


  /* remove a node from the manager's MRU list */
  static void
  ftc_node_mru_unlink( FTC_Node     node,
                       FTC_Manager  manager )
  {
    FTC_Node  prev  = node->mru_prev;
    FTC_Node  next  = node->mru_next;
    FTC_Node  first = manager->nodes_list;


    prev->mru_next = next;
    next->mru_prev = prev;

    if ( node->mru_next == first )
    {
      /* this is the last node in the list; update its head pointer */
      if ( node == first )
        manager->nodes_list = NULL;
      else
        first->mru_prev = prev;
    }

    node->mru_next = NULL;
    node->mru_prev = NULL;
    manager->num_nodes--;
  }


  /* move a node to the head of the manager's MRU list */
  static void
  ftc_node_mru_up( FTC_Node     node,
                   FTC_Manager  manager )
  {
    FTC_Node  first = manager->nodes_list;


    if ( node != first )
    {
      ftc_node_mru_unlink( node, manager );
      ftc_node_mru_link( node, manager );
    }
  }


  /* remove a node from its cache's hash table */
  static void
  ftc_node_hash_unlink( FTC_Node   node,
                        FTC_Cache  cache )
  {
    FTC_Node  *pnode = cache->buckets + ( node->hash % cache->size );


    for (;;)
    {
      if ( *pnode == NULL )
      {
        FT_ERROR(( "FreeType.cache.hash_unlink: unknown node!\n" ));
        return;
      }

      if ( *pnode == node )
      {
        *pnode     = node->link;
        node->link = NULL;

        cache->nodes--;
        return;
      }

      pnode = &(*pnode)->link;
    }
  }


  /* add a node to the "top" of its cache's hash table */
  static void
  ftc_node_hash_link( FTC_Node   node,
                      FTC_Cache  cache )
  {
    FTC_Node  *pnode = cache->buckets + ( node->hash % cache->size );


    node->link = *pnode;
    *pnode     = node;

    cache->nodes++;
  }


  /* remove a node from the cache manager */
  FT_EXPORT_DEF( void )
  ftc_node_destroy( FTC_Node     node,
                    FTC_Manager  manager )
  {
    FT_Memory        memory  = manager->library->memory;
    FTC_Cache        cache;
    FTC_FamilyEntry  entry;
    FTC_Cache_Class  clazz;


#ifdef FT_DEBUG_ERROR
    /* find node's cache */
    if ( node->fam_index >= manager->families.count )
    {
      FT_ERROR(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    entry = manager->families.entries + node->fam_index;
    cache = entry->cache;

#ifdef FT_DEBUG_ERROR
    if ( cache == NULL )
    {
      FT_ERROR(( "ftc_node_destroy: invalid node handle\n" ));
      return;
    }
#endif

    clazz = cache->clazz;

    manager->cur_weight -= clazz->node_weight( node, cache );

    /* remove node from mru list */
    ftc_node_mru_unlink( node, manager );

    /* remove node from cache's hash table */
    ftc_node_hash_unlink( node, cache );

    /* now finalize it */
    if ( clazz->node_done )
      clazz->node_done( node, cache );

    FREE( node );

    /* check, just in case of general corruption :-) */
    if ( manager->num_nodes <= 0 )
      FT_ERROR(( "ftc_node_destroy: invalid cache node count! = %d\n",
                  manager->num_nodes ));
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   CACHE FAMILY DEFINITIONS                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  FT_EXPORT_DEF( FT_Error )
  ftc_family_init( FTC_Family  family,
                   FTC_Query   query,
                   FTC_Cache   cache )
  {
    FT_Error         error;
    FTC_Manager      manager = cache->manager;
    FT_Memory        memory  = manager->library->memory;
    FTC_FamilyEntry  entry;


    family->cache     = cache;
    family->num_nodes = 0;

    /* now add to manager's family table */
    error = ftc_family_table_alloc( &manager->families, memory, &entry );
    if ( !error )
    {
      entry->cache      = cache;
      entry->family     = family;
      family->fam_index = entry->index;

      query->family = family;   /* save family in query */
    }

    return error;
  }


  FT_EXPORT_DEF( void )
  ftc_family_done( FTC_Family  family )
  {
    FTC_Manager  manager = family->cache->manager;


    /* remove from manager's family table */
    ftc_family_table_free( &manager->families, family->fam_index );
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    ABSTRACT CACHE CLASS                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

#define FTC_PRIMES_MIN  7
#define FTC_PRIMES_MAX  13845163

  static const FT_UInt  ftc_primes[] =
  {
    7,
    11,
    19,
    37,
    73,
    109,
    163,
    251,
    367,
    557,
    823,
    1237,
    1861,
    2777,
    4177,
    6247,
    9371,
    14057,
    21089,
    31627,
    47431,
    71143,
    106721,
    160073,
    240101,
    360163,
    540217,
    810343,
    1215497,
    1823231,
    2734867,
    4102283,
    6153409,
    9230113,
    13845163,
  };


  static FT_UFast
  ftc_prime_closest( FT_UFast  num )
  {
    FT_UInt  i;


    for ( i = 0; i < sizeof ( ftc_primes ) / sizeof ( ftc_primes[0] ); i++ )
      if ( ftc_primes[i] > num )
        return ftc_primes[i];

    return FTC_PRIMES_MAX;
  }


#define FTC_CACHE_RESIZE_TEST( c )       \
          ( (c)->nodes*3 < (c)->size  || \
            (c)->size*3  < (c)->nodes )


  static void
  ftc_cache_resize( FTC_Cache  cache )
  {
    FT_UFast  new_size;


    new_size = ftc_prime_closest( cache->nodes );
    if ( new_size != cache->size )
    {
      FT_Memory  memory = cache->memory;
      FT_Error   error;
      FTC_Node*  new_buckets ;
      FT_ULong   i;


      /* no need to report an error; we'll simply keep using the same */
      /* buckets number / size                                        */
      if ( ALLOC_ARRAY( new_buckets, new_size, FTC_Node ) )
        return;

      for ( i = 0; i < cache->size; i++ )
      {
        FTC_Node   node, next, *pnode;
        FT_UFast   hash;


        node = cache->buckets[i];
        while ( node )
        {
          next  = node->link;
          hash  = node->hash % new_size;
          pnode = new_buckets + hash;

          node->link = pnode[0];
          pnode[0]   = node;

          node = next;
        }
      }

      if ( cache->buckets )
        FREE( cache->buckets );

      cache->buckets = new_buckets;
      cache->size    = new_size;
    }
  }


  FT_EXPORT_DEF( FT_Error )
  ftc_cache_init( FTC_Cache  cache )
  {
    FT_Memory        memory = cache->memory;
    FTC_Cache_Class  clazz  = cache->clazz;
    FT_Error         error;


    cache->nodes = 0;
    cache->size  = FTC_PRIMES_MIN;

    if ( ALLOC_ARRAY( cache->buckets, cache->size, FTC_Node ) )
      goto Exit;

    /* now, initialize the lru list of families for this cache */
    if ( clazz->family_size > 0 )
    {
      FT_LruList_ClassRec*  lru_class = &cache->family_class;


      lru_class->list_size = sizeof( FT_LruListRec );
      lru_class->list_init = NULL;
      lru_class->list_done = NULL;

      lru_class->node_size    = clazz->family_size;
      lru_class->node_init    = (FT_LruNode_InitFunc)   clazz->family_init;
      lru_class->node_done    = (FT_LruNode_DoneFunc)   clazz->family_done;
      lru_class->node_flush   = (FT_LruNode_FlushFunc)  NULL;
      lru_class->node_compare = (FT_LruNode_CompareFunc)clazz->family_compare;

      error = FT_LruList_New( (FT_LruList_Class) lru_class,
                              0,    /* max items == 0 => unbounded list */
                              cache,
                              memory,
                              &cache->families );
      if ( error )
        FREE( cache->buckets );
    }

  Exit:
    return error;
  }


  FT_EXPORT_DEF( void )
  ftc_cache_clear( FTC_Cache  cache )
  {
    if ( cache )
    {
      FT_Memory        memory  = cache->memory;
      FTC_Cache_Class  clazz   = cache->clazz;
      FTC_Manager      manager = cache->manager;
      FT_UFast         i;


      for ( i = 0; i < cache->size; i++ )
      {
        FTC_Node  *pnode = cache->buckets + i, next, node = *pnode;


        while ( node )
        {
          next        = node->link;
          node->link  = NULL;

          /* remove node from mru list */
          ftc_node_mru_unlink( node, manager );

          /* now finalize it */
          manager->cur_weight -= clazz->node_weight( node, cache );

          if ( clazz->node_done )
            clazz->node_done( node, cache );

          FREE( node );
          node = next;
        }
        cache->buckets[i] = NULL;
      }

      cache->nodes = 0;

      /* destroy the families */
      if ( cache->families )
        FT_LruList_Reset( cache->families );
    }
  }


  FT_EXPORT_DEF( void )
  ftc_cache_done( FTC_Cache  cache )
  {
    if ( cache )
    {
      FT_Memory  memory = cache->memory;


      ftc_cache_clear( cache );

      FREE( cache->buckets );
      cache->size = 0;

      if ( cache->families )
      {
        FT_LruList_Destroy( cache->families );
        cache->families = NULL;
      }
    }
  }


  /* Look up a node in "top" of its cache's hash table. */
  /* If not found, create a new node.                   */
  /*                                                    */
  FT_EXPORT_DEF( FT_Error )
  ftc_cache_lookup( FTC_Cache   cache,
                    FTC_Query   query,
                    FTC_Node   *anode )
  {
    FT_Error    error;
    FT_LruNode  lru;


    /* some argument checks are delayed to ftc_glyph_cache_lookup */
    if ( !cache || !query || !anode )
      return FTC_Err_Invalid_Argument;

    *anode = NULL;

    query->hash   = 0;
    query->family = NULL;

    error = FT_LruList_Lookup( cache->families, query, &lru );
    if ( !error )
    {
      FTC_Family  family = (FTC_Family) lru;
      FT_UFast    hash    = query->hash;
      FTC_Node*   bucket  = cache->buckets + (hash % cache->size);


      if ( query->family     != family                        ||
           family->fam_index >= cache->manager->families.size )
      {
        FT_ERROR((
          "ftc_cache_lookup: invalid query (bad 'family' field)\n" ));
        return FT_Err_Invalid_Argument;
      }

      if ( *bucket )
      {
        FTC_Node*             pnode   = bucket;
        FTC_Node_CompareFunc  compare = cache->clazz->node_compare;


        for ( ;; )
        {
          FTC_Node  node;


          node = *pnode;
          if ( node == NULL )
            break;

          if ( (FT_UInt)node->fam_index == family->fam_index &&
               compare( node, query, cache ) )
          {
            /* move to head of bucket list */
            if ( pnode != bucket )
            {
              *pnode     = node->link;
              node->link = *bucket;
              *bucket    = node;
            }

            /* move to head of MRU list */
            if ( node != cache->manager->nodes_list )
              ftc_node_mru_up( node, cache->manager );

            *anode = node;
            goto Exit;
          }

          pnode = &(*pnode)->link;
        }
      }

      /* didn't find a node, create a new one */
      {
        FTC_Cache_Class  clazz   = cache->clazz;
        FTC_Manager      manager = cache->manager;
        FT_Memory        memory  = cache->memory;
        FTC_Node         node;


        if ( ALLOC( node, clazz->node_size ) )
          goto Exit;

        node->fam_index = (FT_UShort) family->fam_index;
        node->hash      = query->hash;
        node->ref_count = 0;

        error = clazz->node_init( node, query, cache );
        if ( error )
        {
          FREE( node );
          goto Exit;
        }

        ftc_node_hash_link( node, cache );
        ftc_node_mru_link( node, cache->manager );

        cache->manager->cur_weight += clazz->node_weight( node, cache );

        /* now try to compress the node pool when necessary */
        if ( manager->cur_weight >= manager->max_weight )
        {
          node->ref_count++;
          FTC_Manager_Compress( manager );
          node->ref_count--;
        }

        /* try to resize the hash table if appropriate */
        if ( FTC_CACHE_RESIZE_TEST( cache ) )
          ftc_cache_resize( cache );

        *anode = node;
      }
    }

  Exit:
    return error;
  }


/* END */
