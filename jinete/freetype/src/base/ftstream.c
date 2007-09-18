/***************************************************************************/
/*                                                                         */
/*  ftstream.c                                                             */
/*                                                                         */
/*    I/O stream support (body).                                           */
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
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_DEBUG_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_stream


  FT_BASE_DEF( void )
  FT_New_Memory_Stream( FT_Library  library,
                        FT_Byte*    base,
                        FT_ULong    size,
                        FT_Stream   stream )
  {
    stream->memory = library->memory;
    stream->base   = base;
    stream->size   = size;
    stream->pos    = 0;
    stream->cursor = 0;
    stream->read   = 0;
    stream->close  = 0;
  }


  FT_BASE_DEF( FT_Error )
  FT_Seek_Stream( FT_Stream  stream,
                  FT_ULong   pos )
  {
    FT_Error  error;


    stream->pos = pos;

    if ( stream->read )
    {
      if ( stream->read( stream, pos, 0, 0 ) )
      {
        FT_ERROR(( "FT_Seek_Stream:" ));
        FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
                   pos, stream->size ));

        error = FT_Err_Invalid_Stream_Operation;
      }
      else
        error = FT_Err_Ok;
    }
    /* note that seeking to the first position after the file is valid */
    else if ( pos > stream->size )
    {
      FT_ERROR(( "FT_Seek_Stream:" ));
      FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
                 pos, stream->size ));

      error = FT_Err_Invalid_Stream_Operation;
    }

    else
      error = FT_Err_Ok;

    return error;
  }


  FT_BASE_DEF( FT_Error )
  FT_Skip_Stream( FT_Stream  stream,
                  FT_Long    distance )
  {
    return FT_Seek_Stream( stream, (FT_ULong)( stream->pos + distance ) );
  }


  FT_BASE_DEF( FT_Long )
  FT_Stream_Pos( FT_Stream  stream )
  {
    return stream->pos;
  }


  FT_BASE_DEF( FT_Error )
  FT_Read_Stream( FT_Stream  stream,
                  FT_Byte*   buffer,
                  FT_ULong   count )
  {
    return FT_Read_Stream_At( stream, stream->pos, buffer, count );
  }


  FT_BASE_DEF( FT_Error )
  FT_Read_Stream_At( FT_Stream  stream,
                     FT_ULong   pos,
                     FT_Byte*   buffer,
                     FT_ULong   count )
  {
    FT_Error  error = FT_Err_Ok;
    FT_ULong  read_bytes;


    if ( pos >= stream->size )
    {
      FT_ERROR(( "FT_Read_Stream_At:" ));
      FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
                 pos, stream->size ));

      return FT_Err_Invalid_Stream_Operation;
    }

    if ( stream->read )
      read_bytes = stream->read( stream, pos, buffer, count );
    else
    {
      read_bytes = stream->size - pos;
      if ( read_bytes > count )
        read_bytes = count;

      MEM_Copy( buffer, stream->base + pos, read_bytes );
    }

    stream->pos = pos + read_bytes;

    if ( read_bytes < count )
    {
      FT_ERROR(( "FT_Read_Stream_At:" ));
      FT_ERROR(( " invalid read; expected %lu bytes, got %lu\n",
                 count, read_bytes ));

      error = FT_Err_Invalid_Stream_Operation;
    }

    return error;
  }


  FT_BASE_DEF( FT_Error )
  FT_Extract_Frame( FT_Stream  stream,
                    FT_ULong   count,
                    FT_Byte**  pbytes )
  {
    FT_Error  error;


    error = FT_Access_Frame( stream, count );
    if ( !error )
    {
      *pbytes = (FT_Byte*)stream->cursor;

      /* equivalent to FT_Forget_Frame(), with no memory block release */
      stream->cursor = 0;
      stream->limit  = 0;
    }

    return error;
  }


  FT_BASE_DEF( void )
  FT_Release_Frame( FT_Stream  stream,
                    FT_Byte**  pbytes )
  {
    if ( stream->read )
    {
      FT_Memory  memory = stream->memory;


      FREE( *pbytes );
    }
    *pbytes = 0;
  }


  FT_BASE_DEF( FT_Error )
  FT_Access_Frame( FT_Stream  stream,
                   FT_ULong   count )
  {
    FT_Error  error = FT_Err_Ok;
    FT_ULong  read_bytes;


    /* check for nested frame access */
    FT_Assert( stream && stream->cursor == 0 );

    if ( stream->read )
    {
      /* allocate the frame in memory */
      FT_Memory  memory = stream->memory;


      if ( ALLOC( stream->base, count ) )
        goto Exit;

      /* read it */
      read_bytes = stream->read( stream, stream->pos,
                                 stream->base, count );
      if ( read_bytes < count )
      {
        FT_ERROR(( "FT_Access_Frame:" ));
        FT_ERROR(( " invalid read; expected %lu bytes, got %lu\n",
                   count, read_bytes ));

        FREE( stream->base );
        error = FT_Err_Invalid_Stream_Operation;
      }
      stream->cursor = stream->base;
      stream->limit  = stream->cursor + count;
      stream->pos   += read_bytes;
    }
    else
    {
      /* check current and new position */
      if ( stream->pos >= stream->size        ||
           stream->pos + count > stream->size )
      {
        FT_ERROR(( "FT_Access_Frame:" ));
        FT_ERROR(( " invalid i/o; pos = 0x%lx, count = %lu, size = 0x%lx\n",
                   stream->pos, count, stream->size ));

        error = FT_Err_Invalid_Stream_Operation;
        goto Exit;
      }

      /* set cursor */
      stream->cursor = stream->base + stream->pos;
      stream->limit  = stream->cursor + count;
      stream->pos   += count;
    }

  Exit:
    return error;
  }


  FT_BASE_DEF( void )
  FT_Forget_Frame( FT_Stream  stream )
  {
    /* IMPORTANT: The assertion stream->cursor != 0 was removed, given    */
    /*            that it is possible to access a frame of length 0 in    */
    /*            some weird fonts (usually, when accessing an array of   */
    /*            0 records, like in some strange kern tables).           */
    /*                                                                    */
    /*  In this case, the loader code handles the 0-length table          */
    /*  gracefully; however, stream.cursor is really set to 0 by the      */
    /*  FT_Access_Frame() call, and this is not an error.                 */
    /*                                                                    */
    FT_Assert( stream );

    if ( stream->read )
    {
      FT_Memory  memory = stream->memory;


      FREE( stream->base );
    }
    stream->cursor = 0;
    stream->limit  = 0;
  }


  FT_BASE_DEF( FT_Char )
  FT_Get_Char( FT_Stream  stream )
  {
    FT_Char  result;


    FT_Assert( stream && stream->cursor );

    result = 0;
    if ( stream->cursor < stream->limit )
      result = *stream->cursor++;

    return result;
  }


  FT_BASE_DEF( FT_Short )
  FT_Get_Short( FT_Stream  stream )
  {
    FT_Byte*  p;
    FT_Short  result;


    FT_Assert( stream && stream->cursor );

    result         = 0;
    p              = stream->cursor;
    if ( p + 1 < stream->limit )
      result       = NEXT_Short( p );
    stream->cursor = p;

    return result;
  }


  FT_BASE_DEF( FT_Short )
  FT_Get_ShortLE( FT_Stream  stream )
  {
    FT_Byte*  p;
    FT_Short  result;


    FT_Assert( stream && stream->cursor );

    result         = 0;
    p              = stream->cursor;
    if ( p + 1 < stream->limit )
      result       = NEXT_ShortLE( p );
    stream->cursor = p;

    return result;
  }


  FT_BASE_DEF( FT_Long )
  FT_Get_Offset( FT_Stream  stream )
  {
    FT_Byte*  p;
    FT_Long   result;


    FT_Assert( stream && stream->cursor );

    result         = 0;
    p              = stream->cursor;
    if ( p + 2 < stream->limit )
      result       = NEXT_Offset( p );
    stream->cursor = p;
    return result;
  }


  FT_BASE_DEF( FT_Long )
  FT_Get_Long( FT_Stream  stream )
  {
    FT_Byte*  p;
    FT_Long   result;


    FT_Assert( stream && stream->cursor );

    result         = 0;
    p              = stream->cursor;
    if ( p + 3 < stream->limit )
      result       = NEXT_Long( p );
    stream->cursor = p;
    return result;
  }


  FT_BASE_DEF( FT_Long )
  FT_Get_LongLE( FT_Stream  stream )
  {
    FT_Byte*  p;
    FT_Long   result;


    FT_Assert( stream && stream->cursor );

    result         = 0;
    p              = stream->cursor;
    if ( p + 3 < stream->limit )
      result       = NEXT_LongLE( p );
    stream->cursor = p;
    return result;
  }


  FT_BASE_DEF( FT_Char )
  FT_Read_Char( FT_Stream  stream,
                FT_Error*  error )
  {
    FT_Byte  result = 0;


    FT_Assert( stream );

    *error = FT_Err_Ok;

    if ( stream->read )
    {
      if ( stream->read( stream, stream->pos, &result, 1L ) != 1L )
        goto Fail;
    }
    else
    {
      if ( stream->pos < stream->size )
        result = stream->base[stream->pos];
      else
        goto Fail;
    }
    stream->pos++;

    return result;

  Fail:
    *error = FT_Err_Invalid_Stream_Operation;
    FT_ERROR(( "FT_Read_Char:" ));
    FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
               stream->pos, stream->size ));

    return 0;
  }


  FT_BASE_DEF( FT_Short )
  FT_Read_Short( FT_Stream  stream,
                 FT_Error*  error )
  {
    FT_Byte   reads[2];
    FT_Byte*  p = 0;
    FT_Short  result = 0;


    FT_Assert( stream );

    *error = FT_Err_Ok;

    if ( stream->pos + 1 < stream->size )
    {
      if ( stream->read )
      {
        if ( stream->read( stream, stream->pos, reads, 2L ) != 2L )
          goto Fail;

        p = reads;
      }
      else
      {
        p = stream->base + stream->pos;
      }

      if ( p )
        result = NEXT_Short( p );
    }
    else
      goto Fail;

    stream->pos += 2;

    return result;

  Fail:
    *error = FT_Err_Invalid_Stream_Operation;
    FT_ERROR(( "FT_Read_Short:" ));
    FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
               stream->pos, stream->size ));

    return 0;
  }


  FT_BASE_DEF( FT_Short )
  FT_Read_ShortLE( FT_Stream  stream,
                   FT_Error*  error )
  {
    FT_Byte   reads[2];
    FT_Byte*  p = 0;
    FT_Short  result = 0;


    FT_Assert( stream );

    *error = FT_Err_Ok;

    if ( stream->pos + 1 < stream->size )
    {
      if ( stream->read )
      {
        if ( stream->read( stream, stream->pos, reads, 2L ) != 2L )
          goto Fail;

        p = reads;
      }
      else
      {
        p = stream->base + stream->pos;
      }

      if ( p )
        result = NEXT_ShortLE( p );
    }
    else
      goto Fail;

    stream->pos += 2;

    return result;

  Fail:
    *error = FT_Err_Invalid_Stream_Operation;
    FT_ERROR(( "FT_Read_Short:" ));
    FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
               stream->pos, stream->size ));

    return 0;
  }


  FT_BASE_DEF( FT_Long )
  FT_Read_Offset( FT_Stream  stream,
                  FT_Error*  error )
  {
    FT_Byte   reads[3];
    FT_Byte*  p = 0;
    FT_Long   result = 0;


    FT_Assert( stream );

    *error = FT_Err_Ok;

    if ( stream->pos + 2 < stream->size )
    {
      if ( stream->read )
      {
        if (stream->read( stream, stream->pos, reads, 3L ) != 3L )
          goto Fail;

        p = reads;
      }
      else
      {
        p = stream->base + stream->pos;
      }

      if ( p )
        result = NEXT_Offset( p );
    }
    else
      goto Fail;

    stream->pos += 3;

    return result;

  Fail:
    *error = FT_Err_Invalid_Stream_Operation;
    FT_ERROR(( "FT_Read_Offset:" ));
    FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
               stream->pos, stream->size ));

    return 0;
  }


  FT_BASE_DEF( FT_Long )
  FT_Read_Long( FT_Stream  stream,
                FT_Error*  error )
  {
    FT_Byte   reads[4];
    FT_Byte*  p = 0;
    FT_Long   result = 0;


    FT_Assert( stream );

    *error = FT_Err_Ok;

    if ( stream->pos + 3 < stream->size )
    {
      if ( stream->read )
      {
        if ( stream->read( stream, stream->pos, reads, 4L ) != 4L )
          goto Fail;

        p = reads;
      }
      else
      {
        p = stream->base + stream->pos;
      }

      if ( p )
        result = NEXT_Long( p );
    }
    else
      goto Fail;

    stream->pos += 4;

    return result;

  Fail:
    FT_ERROR(( "FT_Read_Long:" ));
    FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
               stream->pos, stream->size ));
    *error = FT_Err_Invalid_Stream_Operation;

    return 0;
  }


  FT_BASE_DEF( FT_Long )
  FT_Read_LongLE( FT_Stream  stream,
                  FT_Error*  error )
  {
    FT_Byte   reads[4];
    FT_Byte*  p = 0;
    FT_Long   result = 0;


    FT_Assert( stream );

    *error = FT_Err_Ok;

    if ( stream->pos + 3 < stream->size )
    {
      if ( stream->read )
      {
        if ( stream->read( stream, stream->pos, reads, 4L ) != 4L )
          goto Fail;

        p = reads;
      }
      else
      {
        p = stream->base + stream->pos;
      }

      if ( p )
        result = NEXT_LongLE( p );
    }
    else
      goto Fail;

    stream->pos += 4;

    return result;

  Fail:
    FT_ERROR(( "FT_Read_Long:" ));
    FT_ERROR(( " invalid i/o; pos = 0x%lx, size = 0x%lx\n",
               stream->pos, stream->size ));
    *error = FT_Err_Invalid_Stream_Operation;

    return 0;
  }


  FT_BASE_DEF( FT_Error )
  FT_Read_Fields( FT_Stream              stream,
                  const FT_Frame_Field*  fields,
                  void*                  structure )
  {
    FT_Error  error;
    FT_Bool   frame_accessed = 0;
    FT_Byte*  cursor = stream->cursor;


    if ( !fields || !stream )
      return FT_Err_Invalid_Argument;

    error = FT_Err_Ok;
    do
    {
      FT_ULong  value;
      FT_Int    sign_shift;
      FT_Byte*  p;


      switch ( fields->value )
      {
      case ft_frame_start:  /* access a new frame */
        error = FT_Access_Frame( stream, fields->offset );
        if ( error )
          goto Exit;

        frame_accessed = 1;
        cursor         = stream->cursor;
        fields++;
        continue;  /* loop! */

      case ft_frame_bytes:  /* read a byte sequence */
      case ft_frame_skip:   /* skip some bytes      */
        {
          FT_UInt  len = fields->size;


          if ( cursor + len > stream->limit )
          {
            error = FT_Err_Invalid_Stream_Operation;
            goto Exit;
          }

          if ( fields->value == ft_frame_bytes )
          {
            p = (FT_Byte*)structure + fields->offset;
            MEM_Copy( p, cursor, len );
          }
          cursor += len;
          fields++;
          continue;
        }

      case ft_frame_byte:
      case ft_frame_schar:  /* read a single byte */
        value = NEXT_Byte(cursor);
        sign_shift = 24;
        break;

      case ft_frame_short_be:
      case ft_frame_ushort_be:  /* read a 2-byte big-endian short */
        value = NEXT_UShort(cursor);
        sign_shift = 16;
        break;

      case ft_frame_short_le:
      case ft_frame_ushort_le:  /* read a 2-byte little-endian short */
        value = NEXT_UShortLE(cursor);
        sign_shift = 16;
        break;

      case ft_frame_long_be:
      case ft_frame_ulong_be:  /* read a 4-byte big-endian long */
        value = NEXT_ULong(cursor);
        sign_shift = 0;
        break;

      case ft_frame_long_le:
      case ft_frame_ulong_le:  /* read a 4-byte little-endian long */
        value = NEXT_ULongLE(cursor);
        sign_shift = 0;
        break;

      case ft_frame_off3_be:
      case ft_frame_uoff3_be:  /* read a 3-byte big-endian long */
        value = NEXT_UOffset(cursor);
        sign_shift = 8;
        break;

      case ft_frame_off3_le:
      case ft_frame_uoff3_le:  /* read a 3-byte little-endian long */
        value = NEXT_UOffsetLE(cursor);
        sign_shift = 8;
        break;

      default:
        /* otherwise, exit the loop */
        stream->cursor = cursor;
        goto Exit;
      }

      /* now, compute the signed value is necessary */
      if ( fields->value & FT_FRAME_OP_SIGNED )
        value = (FT_ULong)( (FT_Int32)( value << sign_shift ) >> sign_shift );

      /* finally, store the value in the object */

      p = (FT_Byte*)structure + fields->offset;
      switch ( fields->size )
      {
      case 1:
        *(FT_Byte*)p = (FT_Byte)value;
        break;

      case 2:
        *(FT_UShort*)p = (FT_UShort)value;
        break;

      case 4:
        *(FT_UInt32*)p = (FT_UInt32)value;
        break;

      default:  /* for 64-bit systems */
        *(FT_ULong*)p = (FT_ULong)value;
      }

      /* go to next field */
      fields++;
    }
    while ( 1 );

  Exit:
    /* close the frame if it was opened by this read */
    if ( frame_accessed )
      FT_Forget_Frame( stream );

    return error;
  }


/* END */
