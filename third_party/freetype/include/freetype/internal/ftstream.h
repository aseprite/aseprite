/***************************************************************************/
/*                                                                         */
/*  ftstream.h                                                             */
/*                                                                         */
/*    Stream handling(specification).                                      */
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


#ifndef __FTSTREAM_H__
#define __FTSTREAM_H__


#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


  /* format of an 8-bit frame_op value = [ xxxxx | e | s ] */
  /* s is set to 1 if the value is signed,                 */
  /* e is set to 1 if the value is little-endian           */
  /* xxxxx is a command                                    */

#define FT_FRAME_OP_SHIFT         2
#define FT_FRAME_OP_SIGNED        1
#define FT_FRAME_OP_LITTLE        2
#define FT_FRAME_OP_COMMAND( x )  ( x >> FT_FRAME_OP_SHIFT )

#define FT_MAKE_FRAME_OP( command, little, sign ) \
          ( ( command << FT_FRAME_OP_SHIFT ) | ( little << 1 ) | sign )

#define FT_FRAME_OP_END   0
#define FT_FRAME_OP_START 1  /* start a new frame     */
#define FT_FRAME_OP_BYTE  2  /* read 1-byte value     */
#define FT_FRAME_OP_SHORT 3  /* read 2-byte value     */
#define FT_FRAME_OP_LONG  4  /* read 4-byte value     */
#define FT_FRAME_OP_OFF3  5  /* read 3-byte value     */
#define FT_FRAME_OP_BYTES 6  /* read a bytes sequence */


  typedef enum  FT_Frame_Op_
  {
    ft_frame_end       = 0,
    ft_frame_start     = FT_MAKE_FRAME_OP( FT_FRAME_OP_START, 0, 0 ),

    ft_frame_byte      = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTE,  0, 0 ),
    ft_frame_schar     = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTE,  0, 1 ),

    ft_frame_ushort_be = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 0, 0 ),
    ft_frame_short_be  = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 0, 1 ),
    ft_frame_ushort_le = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 1, 0 ),
    ft_frame_short_le  = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 1, 1 ),

    ft_frame_ulong_be  = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 0, 0 ),
    ft_frame_long_be   = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 0, 1 ),
    ft_frame_ulong_le  = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 1, 0 ),
    ft_frame_long_le   = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 1, 1 ),

    ft_frame_uoff3_be  = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 0, 0 ),
    ft_frame_off3_be   = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 0, 1 ),
    ft_frame_uoff3_le  = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 1, 0 ),
    ft_frame_off3_le   = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 1, 1 ),

    ft_frame_bytes     = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTES, 0, 0 ),
    ft_frame_skip      = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTES, 0, 1 )

  } FT_Frame_Op;


  typedef struct  FT_Frame_Field_
  {
    FT_Byte      value;
    FT_Byte      size;
    FT_UShort    offset;

  } FT_Frame_Field;


  /* Construct an FT_Frame_Field out of a structure type and a field name. */
  /* The structure type must be set in the FT_STRUCTURE macro before       */
  /* calling the FT_FRAME_START() macro.                                   */
#define FT_FIELD_SIZE( f ) \
          (FT_Byte)sizeof ( ((FT_STRUCTURE*)0)->f )

#define FT_FIELD_SIZE_DELTA( f ) \
          (FT_Byte)sizeof ( ((FT_STRUCTURE*)0)->f[0] )

#define FT_FIELD_OFFSET( f ) \
          (FT_UShort)( offsetof( FT_STRUCTURE, f ) )

#define FT_FRAME_FIELD( frame_op, field ) \
          {                               \
            frame_op,                     \
            FT_FIELD_SIZE( field ),       \
            FT_FIELD_OFFSET( field )      \
          }

#define FT_MAKE_EMPTY_FIELD( frame_op )  { frame_op, 0, 0 }

#define FT_FRAME_START( size )   { ft_frame_start, 0, size }
#define FT_FRAME_END             { ft_frame_end, 0, 0 }

#define FT_FRAME_LONG( f )       FT_FRAME_FIELD( ft_frame_long_be, f )
#define FT_FRAME_ULONG( f )      FT_FRAME_FIELD( ft_frame_ulong_be, f )
#define FT_FRAME_SHORT( f )      FT_FRAME_FIELD( ft_frame_short_be, f )
#define FT_FRAME_USHORT( f )     FT_FRAME_FIELD( ft_frame_ushort_be, f )
#define FT_FRAME_BYTE( f )       FT_FRAME_FIELD( ft_frame_byte, f )
#define FT_FRAME_CHAR( f )       FT_FRAME_FIELD( ft_frame_schar, f )

#define FT_FRAME_LONG_LE( f )    FT_FRAME_FIELD( ft_frame_long_le, f )
#define FT_FRAME_ULONG_LE( f )   FT_FRAME_FIELD( ft_frame_ulong_le, f )
#define FT_FRAME_SHORT_LE( f )   FT_FRAME_FIELD( ft_frame_short_le, f )
#define FT_FRAME_USHORT_LE( f )  FT_FRAME_FIELD( ft_frame_ushort_le, f )

#define FT_FRAME_SKIP_LONG       { ft_frame_long_be, 0, 0 }
#define FT_FRAME_SKIP_SHORT      { ft_frame_short_be, 0, 0 }
#define FT_FRAME_SKIP_BYTE       { ft_frame_byte, 0, 0 }

#define FT_FRAME_BYTES( field, count ) \
          {                            \
            ft_frame_bytes,            \
            count,                     \
            FT_FIELD_OFFSET( field )   \
          }

#define FT_FRAME_SKIP_BYTES( count )  { ft_frame_skip, count, 0 }



  /*************************************************************************/
  /*                                                                       */
  /* integer extraction macros -- the `buffer' parameter must ALWAYS be of */
  /* type `char*' or equivalent (1-byte elements).                         */
  /*                                                                       */

#define FT_GET_SHORT_BE( p )                                   \
          ((FT_Int16)( ( (FT_Int16)(FT_Char)(p)[0] <<  8 ) |   \
                         (FT_Int16)(FT_Byte)(p)[1]         ) )

#define FT_GET_USHORT_BE( p )                                   \
          ((FT_Int16)( ( (FT_UInt16)(FT_Byte)(p)[0] <<  8 ) |   \
                         (FT_UInt16)(FT_Byte)(p)[1]         ) )

#define FT_GET_OFF3_BE( p )                                      \
          ( (FT_Int32) ( ( (FT_Int32)(FT_Char)(p)[0] << 16 ) |   \
                         ( (FT_Int32)(FT_Byte)(p)[1] <<  8 ) |   \
                           (FT_Int32)(FT_Byte)(p)[2]         ) )

#define FT_GET_UOFF3_BE( p )                                      \
          ( (FT_Int32) ( ( (FT_UInt32)(FT_Byte)(p)[0] << 16 ) |   \
                         ( (FT_UInt32)(FT_Byte)(p)[1] <<  8 ) |   \
                           (FT_UInt32)(FT_Byte)(p)[2]         ) )

#define FT_GET_LONG_BE( p )                                      \
          ( (FT_Int32) ( ( (FT_Int32)(FT_Char)(p)[0] << 24 ) |   \
                         ( (FT_Int32)(FT_Byte)(p)[1] << 16 ) |   \
                         ( (FT_Int32)(FT_Byte)(p)[2] <<  8 ) |   \
                           (FT_Int32)(FT_Byte)(p)[3]         ) )

#define FT_GET_ULONG_BE( p )                                      \
          ( (FT_Int32) ( ( (FT_UInt32)(FT_Byte)(p)[0] << 24 ) |   \
                         ( (FT_UInt32)(FT_Byte)(p)[1] << 16 ) |   \
                         ( (FT_UInt32)(FT_Byte)(p)[2] <<  8 ) |   \
                           (FT_UInt32)(FT_Byte)(p)[3]         ) )

#define FT_GET_SHORT_LE( p )                                   \
          ((FT_Int16)( ( (FT_Int16)(FT_Char)(p)[1] <<  8 ) |   \
                         (FT_Int16)(FT_Byte)(p)[0]         ) )

#define FT_GET_USHORT_LE( p )                                   \
          ((FT_Int16)( ( (FT_UInt16)(FT_Byte)(p)[1] <<  8 ) |   \
                         (FT_UInt16)(FT_Byte)(p)[0]         ) )

#define FT_GET_OFF3_LE( p )                                      \
          ( (FT_Int32) ( ( (FT_Int32)(FT_Char)(p)[2] << 16 ) |   \
                         ( (FT_Int32)(FT_Byte)(p)[1] <<  8 ) |   \
                           (FT_Int32)(FT_Byte)(p)[0]         ) )

#define FT_GET_UOFF3_LE( p )                                      \
          ( (FT_Int32) ( ( (FT_UInt32)(FT_Byte)(p)[2] << 16 ) |   \
                         ( (FT_UInt32)(FT_Byte)(p)[1] <<  8 ) |   \
                           (FT_UInt32)(FT_Byte)(p)[0]         ) )

#define FT_GET_LONG_LE( p )                                      \
          ( (FT_Int32) ( ( (FT_Int32)(FT_Char)(p)[3] << 24 ) |   \
                         ( (FT_Int32)(FT_Byte)(p)[2] << 16 ) |   \
                         ( (FT_Int32)(FT_Byte)(p)[1] <<  8 ) |   \
                           (FT_Int32)(FT_Byte)(p)[0]         ) )

#define FT_GET_ULONG_LE( p )                                      \
          ( (FT_Int32) ( ( (FT_UInt32)(FT_Byte)(p)[3] << 24 ) |   \
                         ( (FT_UInt32)(FT_Byte)(p)[2] << 16 ) |   \
                         ( (FT_UInt32)(FT_Byte)(p)[1] <<  8 ) |   \
                           (FT_UInt32)(FT_Byte)(p)[0]         ) )


#define NEXT_Char( buffer )          \
          ( (signed char)*buffer++ )

#define NEXT_Byte( buffer )            \
          ( (unsigned char)*buffer++ )

#define NEXT_Short( buffer )                                        \
          ( (short)( buffer += 2, FT_GET_SHORT_BE( buffer - 2 ) ) )

#define NEXT_UShort( buffer )                                                 \
          ( (unsigned short)( buffer += 2, FT_GET_USHORT_BE( buffer - 2 ) ) )

#define NEXT_Offset( buffer )                                     \
          ( (long)( buffer += 3, FT_GET_OFF3_BE( buffer - 3 ) ) )

#define NEXT_UOffset( buffer )                                              \
          ( (unsigned long)( buffer += 3, FT_GET_UOFF3_BE( buffer - 3 ) ) )

#define NEXT_Long( buffer )                                       \
          ( (long)( buffer += 4, FT_GET_LONG_BE( buffer - 4 ) ) )

#define NEXT_ULong( buffer )                                                \
          ( (unsigned long)( buffer += 4, FT_GET_ULONG_BE( buffer - 4 ) ) )


#define NEXT_ShortLE( buffer )                                      \
          ( (short)( buffer += 2, FT_GET_SHORT_LE( buffer - 2 ) ) )

#define NEXT_UShortLE( buffer )                                               \
          ( (unsigned short)( buffer += 2, FT_GET_USHORT_LE( buffer - 2 ) ) )

#define NEXT_OffsetLE( buffer )                                   \
          ( (long)( buffer += 3, FT_GET_OFF3_LE( buffer - 3 ) ) )

#define NEXT_UOffsetLE( buffer )                                            \
          ( (unsigned long)( buffer += 3, FT_GET_UOFF3_LE( buffer - 3 ) ) )


#define NEXT_LongLE( buffer )                                     \
          ( (long)( buffer += 4, FT_GET_LONG_LE( buffer - 4 ) ) )

#define NEXT_ULongLE( buffer )                                              \
          ( (unsigned long)( buffer += 4, FT_GET_ULONG_LE( buffer - 4 ) ) )


  /*************************************************************************/
  /*                                                                       */
  /* Each GET_xxxx() macro uses an implicit `stream' variable.             */
  /*                                                                       */
#define FT_GET_MACRO( func, type )        ( (type)func( stream ) )

#define GET_Char()      FT_GET_MACRO( FT_Get_Char, FT_Char )
#define GET_Byte()      FT_GET_MACRO( FT_Get_Char, FT_Byte )
#define GET_Short()     FT_GET_MACRO( FT_Get_Short, FT_Short )
#define GET_UShort()    FT_GET_MACRO( FT_Get_Short, FT_UShort )
#define GET_Offset()    FT_GET_MACRO( FT_Get_Offset, FT_Long )
#define GET_UOffset()   FT_GET_MACRO( FT_Get_Offset, FT_ULong )
#define GET_Long()      FT_GET_MACRO( FT_Get_Long, FT_Long )
#define GET_ULong()     FT_GET_MACRO( FT_Get_Long, FT_ULong )
#define GET_Tag4()      FT_GET_MACRO( FT_Get_Long, FT_ULong )

#define GET_ShortLE()   FT_GET_MACRO( FT_Get_ShortLE, FT_Short )
#define GET_UShortLE()  FT_GET_MACRO( FT_Get_ShortLE, FT_UShort )
#define GET_LongLE()    FT_GET_MACRO( FT_Get_LongLE, FT_Long )
#define GET_ULongLE()   FT_GET_MACRO( FT_Get_LongLE, FT_ULong )

#define FT_READ_MACRO( func, type, var )        \
          ( var = (type)func( stream, &error ), \
            error != FT_Err_Ok )

#define READ_Byte( var )      FT_READ_MACRO( FT_Read_Char, FT_Byte, var )
#define READ_Char( var )      FT_READ_MACRO( FT_Read_Char, FT_Char, var )
#define READ_Short( var )     FT_READ_MACRO( FT_Read_Short, FT_Short, var )
#define READ_UShort( var )    FT_READ_MACRO( FT_Read_Short, FT_UShort, var )
#define READ_Offset( var )    FT_READ_MACRO( FT_Read_Offset, FT_Long, var )
#define READ_UOffset( var )   FT_READ_MACRO( FT_Read_Offset, FT_ULong, var )
#define READ_Long( var )      FT_READ_MACRO( FT_Read_Long, FT_Long, var )
#define READ_ULong( var )     FT_READ_MACRO( FT_Read_Long, FT_ULong, var )

#define READ_ShortLE( var )   FT_READ_MACRO( FT_Read_ShortLE, FT_Short, var )
#define READ_UShortLE( var )  FT_READ_MACRO( FT_Read_ShortLE, FT_UShort, var )
#define READ_LongLE( var )    FT_READ_MACRO( FT_Read_LongLE, FT_Long, var )
#define READ_ULongLE( var )   FT_READ_MACRO( FT_Read_LongLE, FT_ULong, var )


  FT_BASE( void )
  FT_New_Memory_Stream( FT_Library  library,
                        FT_Byte*    base,
                        FT_ULong    size,
                        FT_Stream   stream );

  FT_BASE( FT_Error )
  FT_Seek_Stream( FT_Stream  stream,
                  FT_ULong   pos );

  FT_BASE( FT_Error )
  FT_Skip_Stream( FT_Stream  stream,
                  FT_Long    distance );

  FT_BASE( FT_Long )
  FT_Stream_Pos( FT_Stream  stream );


  FT_BASE( FT_Error )
  FT_Read_Stream( FT_Stream  stream,
                  FT_Byte*   buffer,
                  FT_ULong   count );

  FT_BASE( FT_Error )
  FT_Read_Stream_At( FT_Stream  stream,
                     FT_ULong   pos,
                     FT_Byte*   buffer,
                     FT_ULong   count );

  FT_BASE( FT_Error )
  FT_Access_Frame( FT_Stream  stream,
                   FT_ULong   count );

  FT_BASE( void )
  FT_Forget_Frame( FT_Stream  stream );

  FT_BASE( FT_Error )
  FT_Extract_Frame( FT_Stream  stream,
                    FT_ULong   count,
                    FT_Byte**  pbytes );

  FT_BASE( void )
  FT_Release_Frame( FT_Stream  stream,
                    FT_Byte**  pbytes );

  FT_BASE( FT_Char )
  FT_Get_Char( FT_Stream  stream );

  FT_BASE( FT_Short )
  FT_Get_Short( FT_Stream  stream );

  FT_BASE( FT_Long )
  FT_Get_Offset( FT_Stream  stream );

  FT_BASE( FT_Long )
  FT_Get_Long( FT_Stream  stream );

  FT_BASE( FT_Short )
  FT_Get_ShortLE( FT_Stream  stream );

  FT_BASE( FT_Long )
  FT_Get_LongLE( FT_Stream  stream );


  FT_BASE( FT_Char )
  FT_Read_Char( FT_Stream  stream,
                FT_Error*  error );

  FT_BASE( FT_Short )
  FT_Read_Short( FT_Stream  stream,
                 FT_Error*  error );

  FT_BASE( FT_Long )
  FT_Read_Offset( FT_Stream  stream,
                  FT_Error*  error );

  FT_BASE( FT_Long )
  FT_Read_Long( FT_Stream  stream,
                FT_Error*  error );

  FT_BASE( FT_Short )
  FT_Read_ShortLE( FT_Stream  stream,
                   FT_Error*  error );

  FT_BASE( FT_Long )
  FT_Read_LongLE( FT_Stream  stream,
                  FT_Error*  error );

  FT_BASE( FT_Error )
  FT_Read_Fields( FT_Stream              stream,
                  const FT_Frame_Field*  fields,
                  void*                  structure );


#define USE_Stream( resource, stream )                       \
          FT_SET_ERROR( FT_Open_Stream( resource, stream ) )

#define DONE_Stream( stream )      \
          FT_Done_Stream( stream )


#define ACCESS_Frame( size )                              \
          FT_SET_ERROR( FT_Access_Frame( stream, size ) )

#define FORGET_Frame()              \
          FT_Forget_Frame( stream )

#define EXTRACT_Frame( size, bytes )                              \
          FT_SET_ERROR( FT_Extract_Frame( stream, size,           \
                                          (FT_Byte**)&(bytes) ) )

#define RELEASE_Frame( bytes )                            \
          FT_Release_Frame( stream, (FT_Byte**)&(bytes) )

#define FILE_Seek( position )                                \
          FT_SET_ERROR( FT_Seek_Stream( stream, position ) )

#define FILE_Skip( distance )                                \
          FT_SET_ERROR( FT_Skip_Stream( stream, distance ) )

#define FILE_Pos()                \
          FT_Stream_Pos( stream )

#define FILE_Read( buffer, count )                        \
          FT_SET_ERROR( FT_Read_Stream( stream,           \
                                        (FT_Byte*)buffer, \
                                        count ) )

#define FILE_Read_At( position, buffer, count )              \
          FT_SET_ERROR( FT_Read_Stream_At( stream,           \
                                           position,         \
                                           (FT_Byte*)buffer, \
                                           count ) )

#define READ_Fields( fields, object )  \
        ( ( error = FT_Read_Fields( stream, fields, object ) ) != FT_Err_Ok )


FT_END_HEADER

#endif /* __FTSTREAM_H__ */


/* END */
