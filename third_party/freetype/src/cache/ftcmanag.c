/***************************************************************************/
/*                                                                         */
/*  ftcmanag.c                                                             */
/*                                                                         */
/*    FreeType Cache Manager (body).                                       */
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
#include FT_CACHE_MANAGER_H
#include FT_CACHE_INTERNAL_LRU_H
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_SIZES_H

#include "ftcerror.h"


#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cache

#define FTC_LRU_GET_MANAGER( lru )  ( (FTC_Manager)(lru)->user_data )


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    FACE LRU IMPLEMENTATION                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  typedef struct FTC_FaceNodeRec_*  FTC_FaceNode;
  typedef struct FTC_SizeNodeRec_*  FTC_SizeNode;


  typedef struct  FTC_FaceNodeRec_
  {
    FT_LruNodeRec  lru;
    FT_Face        face;

  } FTC_FaceNodeRec;


  typedef struct  FTC_SizeNodeRec_
  {
    FT_LruNodeRec  lru;
    FT_Size        size;

  } FTC_SizeNodeRec;


  FT_CALLBACK_DEF( FT_Error )
  ftc_face_node_init( FTC_FaceNode  node,
                      FTC_FaceID    face_id,
                      FTC_Manager   manager )
  {
    FT_Error  error;


    error = manager->request_face( face_id,
                                   manager->library,
                                   manager->request_data,
                                   &node->face );
    if ( !error )
    {
      /* destroy initial size object; it will be re-created later */
      if ( node->face->size )
        FT_Done_Size( node->face->size );
    }

    return error;
  }


  /* helper function for ftc_face_node_done() */
  FT_CALLBACK_DEF( FT_Bool )
  ftc_size_node_select( FTC_SizeNode  node,
                        FT_Face       face )
  {
    return FT_BOOL( node->size->face == face );
  }


  FT_CALLBACK_DEF( void )
  ftc_face_node_done( FTC_FaceNode  node,
                      FTC_Manager   manager )
  {
    FT_Face  face    = node->face;


    /* we must begin by removing all sizes for the target face */
    /* from the manager's list                                 */
    FT_LruList_Remove_Selection( manager->sizes_list,
                                 (FT_LruNode_SelectFunc)ftc_size_node_select,
                                 face );

    /* all right, we can discard the face now */
    FT_Done_Face( face );
    node->face = NULL;
  }


  FT_CALLBACK_TABLE_DEF
  const FT_LruList_ClassRec  ftc_face_list_class =
  {
    sizeof ( FT_LruListRec ),
    (FT_LruList_InitFunc)0,
    (FT_LruList_DoneFunc)0,

    sizeof ( FTC_FaceNodeRec ),
    (FT_LruNode_InitFunc)   ftc_face_node_init,
    (FT_LruNode_DoneFunc)   ftc_face_node_done,
    (FT_LruNode_FlushFunc)  0,  /* no flushing needed                      */
    (FT_LruNode_CompareFunc)0,  /* direct comparison of FTC_FaceID handles */
  };


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_Lookup_Face( FTC_Manager  manager,
                           FTC_FaceID   face_id,
                           FT_Face     *aface )
  {
    FT_Error      error;
    FTC_FaceNode  node;


    if ( aface == NULL )
      return FTC_Err_Bad_Argument;

    *aface = NULL;

    if ( !manager )
      return FTC_Err_Invalid_Cache_Handle;

    error = FT_LruList_Lookup( manager->faces_list,
                               (FT_LruKey)face_id,
                               (FT_LruNode*)&node );
    if ( !error )
      *aface = node->face;

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                      SIZES LRU IMPLEMENTATION                 *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  typedef struct  FTC_SizeQueryRec_
  {
    FT_Face  face;
    FT_UInt  width;
    FT_UInt  height;

  } FTC_SizeQueryRec, *FTC_SizeQuery;


  FT_CALLBACK_DEF( FT_Error )
  ftc_size_node_init( FTC_SizeNode   node,
                      FTC_SizeQuery  query )
  {
    FT_Face   face = query->face;
    FT_Size   size;
    FT_Error  error;


    node->size = NULL;
    error = FT_New_Size( face, &size );
    if ( !error )
    {
      FT_Activate_Size( size );
      error = FT_Set_Pixel_Sizes( query->face,
                                  query->width,
                                  query->height );
      if ( error )
        FT_Done_Size( size );
      else
        node->size = size;
    }
    return error;
  }


  FT_CALLBACK_DEF( void )
  ftc_size_node_done( FTC_SizeNode  node )
  {
    if ( node->size )
    {
      FT_Done_Size( node->size );
      node->size = NULL;
    }
  }


  FT_CALLBACK_DEF( FT_Error )
  ftc_size_node_flush( FTC_SizeNode   node,
                       FTC_SizeQuery  query )
  {
    FT_Size   size = node->size;
    FT_Error  error;


    if ( size->face == query->face )
    {
      FT_Activate_Size( size );
      error = FT_Set_Pixel_Sizes( query->face, query->width, query->height );
      if ( error )
      {
        FT_Done_Size( size );
        node->size = NULL;
      }
    }
    else
    {
      FT_Done_Size( size );
      node->size = NULL;

      error = ftc_size_node_init( node, query );
    }
    return error;
  }


  FT_CALLBACK_DEF( FT_Bool )
  ftc_size_node_compare( FTC_SizeNode   node,
                         FTC_SizeQuery  query )
  {
    FT_Size  size = node->size;


    return FT_BOOL( size->face                    == query->face   &&
                    (FT_UInt)size->metrics.x_ppem == query->width  &&
                    (FT_UInt)size->metrics.y_ppem == query->height );
  }


  FT_CALLBACK_TABLE_DEF
  const FT_LruList_ClassRec  ftc_size_list_class =
  {
    sizeof ( FT_LruListRec ),
    (FT_LruList_InitFunc)0,
    (FT_LruList_DoneFunc)0,

    sizeof ( FTC_SizeNodeRec ),
    (FT_LruNode_InitFunc)   ftc_size_node_init,
    (FT_LruNode_DoneFunc)   ftc_size_node_done,
    (FT_LruNode_FlushFunc)  ftc_size_node_flush,
    (FT_LruNode_CompareFunc)ftc_size_node_compare
  };


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_Lookup_Size( FTC_Manager  manager,
                           FTC_Font     font,
                           FT_Face     *aface,
                           FT_Size     *asize )
  {
    FT_Error  error;


    /* check for valid `manager' delayed to FTC_Manager_Lookup_Face() */
    if ( aface )
      *aface = 0;

    if ( asize )
      *asize = 0;

    error = FTC_Manager_Lookup_Face( manager, font->face_id, aface );
    if ( !error )
    {
      FTC_SizeQueryRec  query;
      FTC_SizeNode      node;


      query.face   = *aface;
      query.width  = font->pix_width;
      query.height = font->pix_height;

      error = FT_LruList_Lookup( manager->sizes_list,
                                 (FT_LruKey)&query,
                                 (FT_LruNode*)&node );
      if ( !error )
      {
        /* select the size as the current one for this face */
        FT_Activate_Size( node->size );

        if ( asize )
          *asize = node->size;
      }
    }

    return error;
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    SET TABLE MANAGEMENT                       *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  static void
  ftc_family_table_init( FTC_FamilyTable  table )
  {
    table->count   = 0;
    table->size    = 0;
    table->entries = NULL;
    table->free    = FTC_FAMILY_ENTRY_NONE;
  }


  static void
  ftc_family_table_done( FTC_FamilyTable  table,
                         FT_Memory        memory )
  {
    FREE( table->entries );
    table->free  = 0;
    table->count = 0;
    table->size  = 0;
  }


  FT_EXPORT_DEF( FT_Error )
  ftc_family_table_alloc( FTC_FamilyTable   table,
                          FT_Memory         memory,
                          FTC_FamilyEntry  *aentry )
  {
    FTC_FamilyEntry  entry;
    FT_Error         error = 0;


    /* re-allocate table size when needed */
    if ( table->free == FTC_FAMILY_ENTRY_NONE && table->count >= table->size )
    {
      FT_UInt  old_size = table->size;
      FT_UInt  new_size, index;


      if ( old_size == 0 )
        new_size = 8;
      else
      {
        new_size = old_size * 2;

        /* check for (unlikely) overflow */
        if ( new_size < old_size )
          new_size = 65534;
      }

      if ( REALLOC_ARRAY( table->entries, old_size, new_size,
                          FTC_FamilyEntryRec ) )
        return error;

      table->size = new_size;

      entry       = table->entries + old_size;
      table->free = old_size;

      for ( index = old_size; index + 1 < new_size; index++, entry++ )
      {
        entry->link  = index + 1;
        entry->index = index;
      }

      entry->link  = FTC_FAMILY_ENTRY_NONE;
      entry->index = index;
    }

    if ( table->free != FTC_FAMILY_ENTRY_NONE )
    {
      entry       = table->entries + table->free;
      table->free = entry->link;
    }
    else if ( table->count < table->size )
    {
      entry = table->entries + table->count++;
    }
    else
    {
      FT_ERROR(( "ftc_family_table_alloc: internal bug!" ));
      return FT_Err_Invalid_Argument;
    }

    entry->link = FTC_FAMILY_ENTRY_NONE;
    table->count++;

    *aentry = entry;
    return error;
  }


  FT_EXPORT_DEF( void )
  ftc_family_table_free( FTC_FamilyTable  table,
                         FT_UInt          index )
  {
    /* simply add it to the linked list of free entries */
    if ( index < table->count )
    {
      FTC_FamilyEntry  entry = table->entries + index;


      if ( entry->link != FTC_FAMILY_ENTRY_NONE )
        FT_ERROR(( "ftc_family_table_free: internal bug!\n" ));
      else
      {
        entry->link = table->free;
        table->free = entry->index;
        table->count--;
      }
    }
  }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    CACHE MANAGER ROUTINES                     *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_New( FT_Library          library,
                   FT_UInt             max_faces,
                   FT_UInt             max_sizes,
                   FT_ULong            max_bytes,
                   FTC_Face_Requester  requester,
                   FT_Pointer          req_data,
                   FTC_Manager        *amanager )
  {
    FT_Error     error;
    FT_Memory    memory;
    FTC_Manager  manager = 0;


    if ( !library )
      return FTC_Err_Invalid_Library_Handle;

    memory = library->memory;

    if ( ALLOC( manager, sizeof ( *manager ) ) )
      goto Exit;

    if ( max_faces == 0 )
      max_faces = FTC_MAX_FACES_DEFAULT;

    if ( max_sizes == 0 )
      max_sizes = FTC_MAX_SIZES_DEFAULT;

    if ( max_bytes == 0 )
      max_bytes = FTC_MAX_BYTES_DEFAULT;

    error = FT_LruList_New( &ftc_face_list_class,
                            max_faces,
                            manager,
                            memory,
                            &manager->faces_list );
    if ( error )
      goto Exit;

    error = FT_LruList_New( &ftc_size_list_class,
                            max_sizes,
                            manager,
                            memory,
                            &manager->sizes_list );
    if ( error )
      goto Exit;

    manager->library      = library;
    manager->max_weight   = max_bytes;
    manager->cur_weight   = 0;

    manager->request_face = requester;
    manager->request_data = req_data;

    ftc_family_table_init( &manager->families );

    *amanager = manager;

  Exit:
    if ( error && manager )
    {
      FT_LruList_Destroy( manager->faces_list );
      FT_LruList_Destroy( manager->sizes_list );
      FREE( manager );
    }

    return error;
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Done( FTC_Manager  manager )
  {
    FT_Memory  memory;
    FT_UInt    index;


    if ( !manager || !manager->library )
      return;

    memory = manager->library->memory;

    /* now discard all caches */
    for (index = 0; index < FTC_MAX_CACHES; index++ )
    {
      FTC_Cache  cache = manager->caches[index];


      if ( cache )
      {
        cache->clazz->cache_done( cache );
        FREE( cache );
        manager->caches[index] = 0;
      }
    }

    /* discard families table */
    ftc_family_table_done( &manager->families, memory );

    /* discard faces and sizes */
    FT_LruList_Destroy( manager->faces_list );
    manager->faces_list = 0;

    FT_LruList_Destroy( manager->sizes_list );
    manager->sizes_list = 0;

    FREE( manager );
  }


  /* documentation is in ftcache.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Reset( FTC_Manager  manager )
  {
    if ( manager )
    {
      FT_LruList_Reset( manager->sizes_list );
      FT_LruList_Reset( manager->faces_list );
    }
    /* XXX: FIXME: flush the caches? */
  }


#ifdef FT_DEBUG_ERROR

  FT_EXPORT_DEF( void )
  FTC_Manager_Check( FTC_Manager  manager )
  {
    FTC_Node  node, first;
    

    first = manager->nodes_list;

    /* check node weights */
    if ( first )
    {
      FT_ULong  weight = 0;
      

      node = first;

      do
      {
        FTC_FamilyEntry  entry = manager->families.entries + node->fam_index;
        FTC_Cache     cache;

        if ( (FT_UInt)node->fam_index >= manager->families.count ||
             entry->link              != FTC_FAMILY_ENTRY_NONE  )
          FT_ERROR(( "FTC_Manager_Check: invalid node (family index = %ld\n",
                     node->fam_index ));
        else
        {
          cache   = entry->cache;
          weight += cache->clazz->node_weight( node, cache );
        }

        node = node->mru_next;

      } while ( node != first );

      if ( weight != manager->cur_weight )
        FT_ERROR(( "FTC_Manager_Check: invalid weight %ld instead of %ld\n",
                   manager->cur_weight, weight ));
    }

    /* check circular list */
    if ( first )
    {
      FT_UFast  count = 0;


      node = first;
      do
      {
        count++;
        node = node->mru_next;

      } while ( node != first );

      if ( count != manager->num_nodes )
        FT_ERROR((
          "FTC_Manager_Check: invalid cache node count %d instead of %d\n",
          manager->num_nodes, count ));
    }
  }

#endif /* FT_DEBUG_ERROR */


  /* `Compress' the manager's data, i.e., get rid of old cache nodes */
  /* that are not referenced anymore in order to limit the total     */
  /* memory used by the cache.                                       */

  /* documentation is in ftcmanag.h */

  FT_EXPORT_DEF( void )
  FTC_Manager_Compress( FTC_Manager  manager )
  {
    FTC_Node   node, first;


    if ( !manager )
      return;

    first = manager->nodes_list;

#ifdef FT_DEBUG_ERROR
    FTC_Manager_Check( manager );

    FT_ERROR(( "compressing, weight = %ld, max = %ld, nodes = %d\n",
               manager->cur_weight, manager->max_weight,
               manager->num_nodes ));
#endif

    if ( manager->cur_weight < manager->max_weight || first == NULL )
      return;

    /* go to last node - it's a circular list */
    node = first->mru_prev;
    do
    {
      FTC_Node  prev = node->mru_prev;


      prev = ( node == first ) ? NULL : node->mru_prev;

      if ( node->ref_count <= 0 )
        ftc_node_destroy( node, manager );

      node = prev;

    } while ( node && manager->cur_weight > manager->max_weight );
  }


  /* documentation is in ftcmanag.h */

  FT_EXPORT_DEF( FT_Error )
  FTC_Manager_Register_Cache( FTC_Manager      manager,
                              FTC_Cache_Class  clazz,
                              FTC_Cache       *acache )
  {
    FT_Error   error = FTC_Err_Invalid_Argument;
    FTC_Cache  cache = NULL;


    if ( manager && clazz && acache )
    {
      FT_Memory  memory = manager->library->memory;
      FT_UInt    index  = 0;


      /* check for an empty cache slot in the manager's table */
      for ( index = 0; index < FTC_MAX_CACHES; index++ )
      {
        if ( manager->caches[index] == 0 )
          break;
      }

      /* return an error if there are too many registered caches */
      if ( index >= FTC_MAX_CACHES )
      {
        error = FTC_Err_Too_Many_Caches;
        FT_ERROR(( "FTC_Manager_Register_Cache:" ));
        FT_ERROR(( " too many registered caches\n" ));
        goto Exit;
      }

      if ( !ALLOC( cache, clazz->cache_size ) )
      {
        cache->manager = manager;
        cache->memory  = memory;
        cache->clazz   = clazz;

        /* THIS IS VERY IMPORTANT!  IT WILL WRETCH THE MANAGER */
        /* IF IT IS NOT SET CORRECTLY                          */
        cache->cache_index = index;

        if ( clazz->cache_init )
        {
          error = clazz->cache_init( cache );
          if ( error )
          {
            if ( clazz->cache_done )
              clazz->cache_done( cache );

            FREE( cache );
            goto Exit;
          }
        }

        manager->caches[index] = cache;
      }
    }

  Exit:
    *acache = cache;
    return error;
  }


  /* documentation is in ftcmanag.h */

  FT_EXPORT_DEF( void )
  FTC_Node_Unref( FTC_Node     node,
                  FTC_Manager  manager )
  {
    if ( node && (FT_UInt)node->fam_index < manager->families.count &&
         manager->families.entries[node->fam_index].cache )
    {
      node->ref_count--;
    }
  }


/* END */
