/***************************************************************************/
/*                                                                         */
/*  ftmemory.h                                                             */
/*                                                                         */
/*    The FreeType memory management macros (specification).               */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg                       */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __FTMEMORY_H__
#define __FTMEMORY_H__


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_TYPES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    FT_SET_ERROR                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro is used to set an implicit `error' variable to a given  */
  /*    expression's value (usually a function call), and convert it to a  */
  /*    boolean which is set whenever the value is != 0.                   */
  /*                                                                       */
#undef  FT_SET_ERROR
#define FT_SET_ERROR( expression ) \
          ( ( error = (expression) ) != 0 )


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                           M E M O R Y                           ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

#ifdef FT_DEBUG_MEMORY

  FT_BASE( FT_Error )
  FT_Alloc_Debug( FT_Memory    memory,
                  FT_Long      size,
                  void*       *P,
                  const char*  file_name,
                  FT_Long      line_no );

  FT_BASE( FT_Error )
  FT_Realloc_Debug( FT_Memory    memory,
                    FT_Long      current,
                    FT_Long      size,
                    void*       *P,
                    const char*  file_name,
                    FT_Long      line_no );

  FT_BASE( void )
  FT_Free_Debug( FT_Memory    memory,
                 FT_Pointer   block,
                 const char*  file_name,
                 FT_Long      line_no );

#endif


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Allocates a new block of memory.  The returned area is always      */
  /*    zero-filled; this is a strong convention in many FreeType parts.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' which handles        */
  /*              allocation.                                              */
  /*                                                                       */
  /*    size   :: The size in bytes of the block to allocate.              */
  /*                                                                       */
  /* <Output>                                                              */
  /*    P      :: A pointer to the fresh new block.  It should be set to   */
  /*              NULL if `size' is 0, or in case of error.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_BASE( FT_Error )
  FT_Alloc( FT_Memory  memory,
            FT_Long    size,
            void*     *P );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Realloc                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Reallocates a block of memory pointed to by `*P' to `Size' bytes   */
  /*    from the heap, possibly changing `*P'.                             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory  :: A handle to a given `memory object' which handles       */
  /*               reallocation.                                           */
  /*                                                                       */
  /*    current :: The current block size in bytes.                        */
  /*                                                                       */
  /*    size    :: The new block size in bytes.                            */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    P       :: A pointer to the fresh new block.  It should be set to  */
  /*               NULL if `size' is 0, or in case of error.               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    All callers of FT_Realloc() _must_ provide the current block size  */
  /*    as well as the new one.                                            */
  /*                                                                       */
  FT_BASE( FT_Error )
  FT_Realloc( FT_Memory  memory,
              FT_Long    current,
              FT_Long    size,
              void**     P );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Free                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Releases a given block of memory allocated through FT_Alloc().     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A handle to a given `memory object' which handles        */
  /*              memory deallocation                                      */
  /*                                                                       */
  /*    P      :: This is the _address_ of a _pointer_ which points to the */
  /*              allocated block.  It is always set to NULL on exit.      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If P or *P are NULL, this function should return successfully.     */
  /*    This is a strong convention within all of FreeType and its         */
  /*    drivers.                                                           */
  /*                                                                       */
  FT_BASE( void )
  FT_Free( FT_Memory  memory,
           void**     P );


  /* This `#include' is needed by the MEM_xxx() macros; it should be */
  /* available on all platforms we know of.                          */
#include <string.h>

#define MEM_Set( dest, byte, count )     memset( dest, byte, count )

#define MEM_Copy( dest, source, count )  memcpy( dest, source, count )

#define MEM_Move( dest, source, count )  memmove( dest, source, count )


  /*************************************************************************/
  /*                                                                       */
  /* We now support closures to produce completely reentrant code.  This   */
  /* means the allocation functions now takes an additional argument       */
  /* (`memory').  It is a handle to a given memory object, responsible for */
  /* all low-level operations, including memory management and             */
  /* synchronisation.                                                      */
  /*                                                                       */
  /* In order to keep our code readable and use the same macros in the     */
  /* font drivers and the rest of the library, MEM_Alloc(), ALLOC(), and   */
  /* ALLOC_ARRAY() now use an implicit variable, `memory'.  It must be     */
  /* defined at all locations where a memory operation is queried.         */
  /*                                                                       */

#ifdef FT_DEBUG_MEMORY

#define MEM_Alloc( _pointer_, _size_ )                               \
          FT_Alloc_Debug( memory, _size_,                            \
                          (void**)&(_pointer_), __FILE__, __LINE__ )

#define MEM_Alloc_Array( _pointer_, _count_, _type_ )    \
          FT_Alloc_Debug( memory, (_count_)*sizeof ( _type_ ), \
                          (void**)&(_pointer_), __FILE__, __LINE__ )

#define MEM_Realloc( _pointer_, _current_, _size_ )                    \
          FT_Realloc_Debug( memory, _current_, _size_,                 \
                            (void**)&(_pointer_), __FILE__, __LINE__ )

#define MEM_Realloc_Array( _pointer_, _current_, _new_, _type_ )       \
          FT_Realloc_Debug( memory, (_current_)*sizeof ( _type_ ),     \
                            (_new_)*sizeof ( _type_ ),                 \
                            (void**)&(_pointer_), __FILE__, __LINE__ )

#define MEM_Free( _pointer_ )                                               \
          FT_Free_Debug( memory, (void**)&(_pointer_), __FILE__, __LINE__ )
  
#else  /* !FT_DEBUG_MEMORY */

#define MEM_Alloc( _pointer_, _size_ )                     \
          FT_Alloc( memory, _size_, (void**)&(_pointer_) )

#define MEM_Alloc_Array( _pointer_, _count_, _type_ )    \
          FT_Alloc( memory, (_count_)*sizeof ( _type_ ), \
                    (void**)&(_pointer_) )

#define MEM_Realloc( _pointer_, _current_, _size_ )                     \
          FT_Realloc( memory, _current_, _size_, (void**)&(_pointer_) )

#define MEM_Realloc_Array( _pointer_, _current_, _new_, _type_ )        \
          FT_Realloc( memory, (_current_)*sizeof ( _type_ ),            \
                      (_new_)*sizeof ( _type_ ), (void**)&(_pointer_) )

#define MEM_Free( _pointer_ )                     \
          FT_Free( memory, (void**)&(_pointer_) )

#endif /* !FT_DEBUG_MEMORY */


#define ALLOC( _pointer_, _size_ )                       \
          FT_SET_ERROR( MEM_Alloc( _pointer_, _size_ ) )

#define REALLOC( _pointer_, _current_, _size_ )                       \
          FT_SET_ERROR( MEM_Realloc( _pointer_, _current_, _size_ ) )

#define ALLOC_ARRAY( _pointer_, _count_, _type_ )                  \
          FT_SET_ERROR( MEM_Alloc( _pointer_,                      \
                                   (_count_)*sizeof ( _type_ ) ) )

#define REALLOC_ARRAY( _pointer_, _current_, _count_, _type_ )       \
          FT_SET_ERROR( MEM_Realloc( _pointer_,                      \
                                     (_current_)*sizeof ( _type_ ),  \
                                     (_count_)*sizeof ( _type_ ) ) )

#define FREE( _pointer_ )       \
          MEM_Free( _pointer_ )


FT_END_HEADER

#endif /* __FTMEMORY_H__ */


/* END */
