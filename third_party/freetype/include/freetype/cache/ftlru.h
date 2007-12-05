/***************************************************************************/
/*                                                                         */
/*  ftlru.h                                                                */
/*                                                                         */
/*    Simple LRU list-cache (specification).                               */
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
  /* An LRU is a list that cannot hold more than a certain number of       */
  /* elements (`max_elements').  All elements in the list are sorted in    */
  /* least-recently-used order, i.e., the `oldest' element is at the tail  */
  /* of the list.                                                          */
  /*                                                                       */
  /* When doing a lookup (either through `Lookup()' or `Lookup_Node()'),   */
  /* the list is searched for an element with the corresponding key.  If   */
  /* it is found, the element is moved to the head of the list and is      */
  /* returned.                                                             */
  /*                                                                       */
  /* If no corresponding element is found, the lookup routine will try to  */
  /* obtain a new element with the relevant key.  If the list is already   */
  /* full, the oldest element from the list is discarded and replaced by a */
  /* new one; a new element is added to the list otherwise.                */
  /*                                                                       */
  /* Note that it is possible to pre-allocate the element list nodes.      */
  /* This is handy if `max_elements' is sufficiently small, as it saves    */
  /* allocations/releases during the lookup process.                       */
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


#ifndef __FTLRU_H__
#define __FTLRU_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /* generic list key type */
  typedef FT_Pointer  FT_LruKey;

  /* a list list handle */
  typedef struct FT_LruListRec_*  FT_LruList;

  /* a list class handle */
  typedef const struct FT_LruList_ClassRec_*  FT_LruList_Class;

  /* a list node handle */
  typedef struct FT_LruNodeRec_*  FT_LruNode;

  /* the list node structure */
  typedef struct  FT_LruNodeRec_
  {
    FT_LruNode  next;
    FT_LruKey   key;

  } FT_LruNodeRec;


  /* the list structure */
  typedef struct  FT_LruListRec_
  {
    FT_Memory         memory;
    FT_LruList_Class  clazz;
    FT_LruNode        nodes;
    FT_UInt           max_nodes;
    FT_UInt           num_nodes;
    FT_Pointer        data;

  } FT_LruListRec;


  /* initialize a list list */
  typedef FT_Error
  (*FT_LruList_InitFunc)( FT_LruList  list );

  /* finalize a list list */
  typedef void
  (*FT_LruList_DoneFunc)( FT_LruList  list );

  /* this method is used to initialize a new list element node */
  typedef FT_Error
  (*FT_LruNode_InitFunc)( FT_LruNode  node,
                          FT_LruKey   key,
                          FT_Pointer  data );

  /* this method is used to finalize a given list element node */
  typedef void
  (*FT_LruNode_DoneFunc)( FT_LruNode  node,
                          FT_Pointer  data );

  /* If defined, this method is called when the list if full        */
  /* during the lookup process -- it is used to change the contents */
  /* of a list element node instead of calling `done_element()',    */
  /* then `init_element()'.  Set it to 0 for default behaviour.     */
  typedef FT_Error
  (*FT_LruNode_FlushFunc)( FT_LruNode  node,
                           FT_LruKey   new_key,
                           FT_Pointer  data );

  /* If defined, this method is used to compare a list element node */
  /* with a given key during a lookup.  If set to 0, the `key'      */
  /* fields will be directly compared instead.                      */
  typedef FT_Bool
  (*FT_LruNode_CompareFunc)( FT_LruNode  node,
                             FT_LruKey   key,
                             FT_Pointer  data );

  /* A selector is used to indicate whether a given list element node */
  /* is part of a selection for FT_LruList_Remove_Selection().  The   */
  /* functrion must return true (i.e., non-null) to indicate that the */
  /* node is part of it.                                              */
  typedef FT_Bool
  (*FT_LruNode_SelectFunc)( FT_LruNode  node,
                            FT_Pointer  data,
                            FT_Pointer  list_data );

  /* LRU class */
  typedef struct  FT_LruList_ClassRec_
  {
    FT_UInt                 list_size;
    FT_LruList_InitFunc     list_init;      /* optional */
    FT_LruList_DoneFunc     list_done;      /* optional */

    FT_UInt                 node_size;
    FT_LruNode_InitFunc     node_init;     /* MANDATORY */
    FT_LruNode_DoneFunc     node_done;     /* optional  */
    FT_LruNode_FlushFunc    node_flush;    /* optional  */
    FT_LruNode_CompareFunc  node_compare;  /* optional  */

  } FT_LruList_ClassRec;


  /* The following functions must be exported in the case where */
  /* applications would want to write their own cache classes.  */

  FT_EXPORT( FT_Error )
  FT_LruList_New( FT_LruList_Class  clazz,
                  FT_UInt           max_elements,
                  FT_Pointer        user_data,
                  FT_Memory         memory,
                  FT_LruList       *alist );

  FT_EXPORT( void )
  FT_LruList_Reset( FT_LruList  list );

  FT_EXPORT( void )
  FT_LruList_Destroy ( FT_LruList  list );

  FT_EXPORT( FT_Error )
  FT_LruList_Lookup( FT_LruList  list,
                     FT_LruKey   key,
                     FT_LruNode *anode );

  FT_EXPORT( void )
  FT_LruList_Remove( FT_LruList  list,
                     FT_LruNode  node );

  FT_EXPORT( void )
  FT_LruList_Remove_Selection( FT_LruList             list,
                               FT_LruNode_SelectFunc  select_func,
                               FT_Pointer             select_data );

 /* */

FT_END_HEADER


#endif /* __FTLRU_H__ */


/* END */
