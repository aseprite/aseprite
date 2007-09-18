/***************************************************************************/
/*                                                                         */
/*  ftlru.c                                                                */
/*                                                                         */
/*    Simple LRU list-cache (body).                                        */
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
#include FT_CACHE_INTERNAL_LRU_H
#include FT_LIST_H
#include FT_INTERNAL_OBJECTS_H

#include "ftcerror.h"


  FT_EXPORT_DEF( FT_Error )
  FT_LruList_New( FT_LruList_Class  clazz,
                  FT_UInt           max_nodes,
                  FT_Pointer        user_data,
                  FT_Memory         memory,
                  FT_LruList       *alist )
  {
    FT_Error    error;
    FT_LruList  list;


    if ( !alist || !clazz )
      return FTC_Err_Invalid_Argument;

    *alist = NULL;
    if ( !ALLOC( list, clazz->list_size ) )
    {
      /* initialize common fields */
      list->clazz      = clazz;
      list->memory     = memory;
      list->max_nodes  = max_nodes;
      list->data       = user_data;

      if ( clazz->list_init )
      {
        error = clazz->list_init( list );
        if ( error )
        {
          if ( clazz->list_done )
            clazz->list_done( list );

          FREE( list );
        }
      }

      *alist = list;
    }

    return error;
  }


  FT_EXPORT_DEF( void )
  FT_LruList_Destroy( FT_LruList  list )
  {
    FT_Memory         memory;
    FT_LruList_Class  clazz;


    if ( !list )
      return;

    memory = list->memory;
    clazz  = list->clazz;

    FT_LruList_Reset( list );

    if ( clazz->list_done )
      clazz->list_done( list );

    FREE( list );
  }


  FT_EXPORT_DEF( void )
  FT_LruList_Reset( FT_LruList  list )
  {
    FT_LruNode        node;
    FT_LruList_Class  clazz;
    FT_Memory         memory;


    if ( !list )
      return;

    node   = list->nodes;
    clazz  = list->clazz;
    memory = list->memory;

    while ( node )
    {
      FT_LruNode  next = node->next;


      if ( clazz->node_done )
        clazz->node_done( node, list->data );

      FREE( node );
      node = next;
    }

    list->nodes     = NULL;
    list->num_nodes = 0;
  }


  FT_EXPORT_DEF( FT_Error )
  FT_LruList_Lookup( FT_LruList   list,
                     FT_LruKey    key,
                     FT_LruNode  *anode )
  {
    FT_Error          error = 0;
    FT_LruNode        node, *pnode;
    FT_LruList_Class  clazz;
    FT_LruNode*       plast;
    FT_LruNode        result = NULL;
    FT_Memory         memory;


    if ( !list || !key || !anode )
      return FTC_Err_Invalid_Argument;

    pnode  = &list->nodes;
    plast  = pnode;
    node   = NULL;
    clazz  = list->clazz;
    memory = list->memory;

    if ( clazz->node_compare )
    {
      for (;;)
      {
        node = *pnode;
        if ( node == NULL )
          break;

        if ( clazz->node_compare( node, key, list->data ) )
          break;

        plast = pnode;
        pnode = &(*pnode)->next;
      }
    }
    else
    {
      for (;;)
      {
        node = *pnode;
        if ( node == NULL )
          break;

        if ( node->key == key )
          break;

        plast = pnode;
        pnode = &(*pnode)->next;
      }
    }

    if ( node )
    {
      /* move element to top of list */
      if ( list->nodes != node )
      {
        *pnode      = node->next;
        node->next  = list->nodes;
        list->nodes = node;
      }
      result = node;
      goto Exit;
    }

    /* we haven't found the relevant element.  We will now try */
    /* to create a new one.                                    */
    /*                                                         */

    /* first, check if our list if full, when appropriate */
    if ( list->max_nodes > 0 && list->num_nodes >= list->max_nodes )
    {
      /* this list list is full; we will now flush */
      /* the oldest node, if there's one!          */
      FT_LruNode  last = *plast;


      if ( last )
      {
        if ( clazz->node_flush )
        {
          error = clazz->node_flush( last, key, list->data );
        }
        else
        {
          if ( clazz->node_done )
            clazz->node_done( last, list->data );

          last->key  = key;
          error = clazz->node_init( last, key, list->data );
        }

        if ( !error )
        {
          /* move it to the top of the list */
          *plast      = NULL;
          last->next  = list->nodes;
          list->nodes = last;

          result = last;
          goto Exit;
        }

        /* in case of error during the flush or done/init cycle, */
        /* we need to discard the node                           */
        if ( clazz->node_done )
          clazz->node_done( last, list->data );

        *plast = NULL;
        list->num_nodes--;

        FREE( last );
        goto Exit;
      }
    }

    /* otherwise, simply allocate a new node */
    if ( ALLOC( node, clazz->node_size ) )
      goto Exit;

    node->key = key;
    error = clazz->node_init( node, key, list->data );
    if ( error )
    {
      FREE( node );
      goto Exit;
    }

    result      = node;
    node->next  = list->nodes;
    list->nodes = node;
    list->num_nodes++;

  Exit:
    *anode = result;
    return error;
  }


  FT_EXPORT_DEF( void )
  FT_LruList_Remove( FT_LruList  list,
                     FT_LruNode  node )
  {
    FT_LruNode  *pnode;


    if ( !list || !node )
      return;

    pnode = &list->nodes;
    for (;;)
    {
      if ( *pnode == node )
      {
        FT_Memory         memory = list->memory;
        FT_LruList_Class  clazz  = list->clazz;


        *pnode     = node->next;
        node->next = NULL;

        if ( clazz->node_done )
          clazz->node_done( node, list->data );

        FREE( node );
        list->num_nodes--;
        break;
      }

      pnode = &(*pnode)->next;
    }
  }


  FT_EXPORT_DEF( void )
  FT_LruList_Remove_Selection( FT_LruList             list,
                               FT_LruNode_SelectFunc  select_func,
                               FT_Pointer             select_data )
  {
    FT_LruNode       *pnode, node;
    FT_LruList_Class  clazz;
    FT_Memory         memory;


    if ( !list || !select_func )
      return;

    memory = list->memory;
    clazz  = list->clazz;
    pnode  = &list->nodes;

    for (;;)
    {
      node = *pnode;
      if ( node == NULL )
        break;

      if ( select_func( node, select_data, list->data ) )
      {
        *pnode     = node->next;
        node->next = NULL;

        if ( clazz->node_done )
          clazz->node_done( node, list );

        FREE( node );
      }
      else
        pnode = &(*pnode)->next;
    }
  }


/* END */
