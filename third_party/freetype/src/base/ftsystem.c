/***************************************************************************/
/*                                                                         */
/*  ftsystem.c                                                             */
/*                                                                         */
/*    ANSI-specific FreeType low-level system interface (body).            */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
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
  /* This file contains the default interface used by FreeType to access   */
  /* low-level, i.e. memory management, i/o access as well as thread       */
  /* synchronisation.  It can be replaced by user-specific routines if     */
  /* necessary.                                                            */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_DEBUG_H
#include FT_SYSTEM_H
#include FT_ERRORS_H
#include FT_TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


  /*************************************************************************/
  /*                                                                       */
  /*                       MEMORY MANAGEMENT INTERFACE                     */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* It is not necessary to do any error checking for the                  */
  /* allocation-related functions.  This will be done by the higher level  */
  /* routines like FT_Alloc() or FT_Realloc().                             */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory allocation function.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A pointer to the memory object.                          */
  /*                                                                       */
  /*    size   :: The requested size in bytes.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of newly allocated block.                              */
  /*                                                                       */
  FT_CALLBACK_DEF( void* )
  ft_alloc( FT_Memory  memory,
            long       size )
  {
    FT_UNUSED( memory );

    return malloc( size );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_realloc                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory reallocation function.                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory   :: A pointer to the memory object.                        */
  /*                                                                       */
  /*    cur_size :: The current size of the allocated memory block.        */
  /*                                                                       */
  /*    new_size :: The newly requested size in bytes.                     */
  /*                                                                       */
  /*    block    :: The current address of the block in memory.            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of the reallocated memory block.                       */
  /*                                                                       */
  FT_CALLBACK_DEF( void* )
  ft_realloc( FT_Memory  memory,
              long       cur_size,
              long       new_size,
              void*      block )
  {
    FT_UNUSED( memory );
    FT_UNUSED( cur_size );

    return realloc( block, new_size );
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_free                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory release function.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory  :: A pointer to the memory object.                         */
  /*                                                                       */
  /*    block   :: The address of block in memory to be freed.             */
  /*                                                                       */
  FT_CALLBACK_DEF( void )
  ft_free( FT_Memory  memory,
           void*      block )
  {
    FT_UNUSED( memory );

    free( block );
  }


  /*************************************************************************/
  /*                                                                       */
  /*                     RESOURCE MANAGEMENT INTERFACE                     */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_io

  /* We use the macro STREAM_FILE for convenience to extract the       */
  /* system-specific stream handle from a given FreeType stream object */
#define STREAM_FILE( stream )  ( (FILE*)stream->descriptor.pointer )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_close_stream                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to close a stream.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  FT_CALLBACK_DEF( void )
  ft_close_stream( FT_Stream  stream )
  {
    fclose( STREAM_FILE( stream ) );

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_io_stream                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to open a stream.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  /*    offset :: The position in the data stream to start reading.        */
  /*                                                                       */
  /*    buffer :: The address of buffer to store the read data.            */
  /*                                                                       */
  /*    count  :: The number of bytes to read from the stream.             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The number of bytes actually read.                                 */
  /*                                                                       */
  FT_CALLBACK_DEF( unsigned long )
  ft_io_stream( FT_Stream       stream,
                unsigned long   offset,
                unsigned char*  buffer,
                unsigned long   count )
  {
    FILE*  file;


    file = STREAM_FILE( stream );

    fseek( file, offset, SEEK_SET );

    return (unsigned long)fread( buffer, 1, count, file );
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Stream( const char*  filepathname,
                 FT_Stream    astream )
  {
    FILE*  file;


    if ( !astream )
      return FT_Err_Invalid_Stream_Handle;

    file = fopen( filepathname, "rb" );
    if ( !file )
    {
      FT_ERROR(( "FT_New_Stream:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));

      return FT_Err_Cannot_Open_Resource;
    }

    fseek( file, 0, SEEK_END );
    astream->size = ftell( file );
    fseek( file, 0, SEEK_SET );

    astream->descriptor.pointer = file;
    astream->pathname.pointer   = (char*)filepathname;
    astream->pos                = 0;

    astream->read  = ft_io_stream;
    astream->close = ft_close_stream;

    FT_TRACE1(( "FT_New_Stream:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, astream->size ));

    return FT_Err_Ok;
  }



#ifdef FT_DEBUG_MEMORY

  extern FT_Int
  ft_mem_debug_init( FT_Memory  memory );
  
  extern void
  ft_mem_debug_done( FT_Memory  memory );
  
#endif  
      
      
  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Memory )
  FT_New_Memory( void )
  {
    FT_Memory  memory;


    memory = (FT_Memory)malloc( sizeof ( *memory ) );
    if ( memory )
    {
      memory->user    = 0;
      memory->alloc   = ft_alloc;
      memory->realloc = ft_realloc;
      memory->free    = ft_free;
#ifdef FT_DEBUG_MEMORY
      ft_mem_debug_init( memory );
#endif    
    }

    return memory;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( void )
  FT_Done_Memory( FT_Memory  memory )
  {
#ifdef FT_DEBUG_MEMORY
    ft_mem_debug_done( memory );
#endif  
    memory->free( memory, memory );
  }


/* END */
