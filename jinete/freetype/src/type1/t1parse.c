/***************************************************************************/
/*                                                                         */
/*  t1parse.c                                                              */
/*                                                                         */
/*    Type 1 parser (body).                                                */
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
  /* The Type 1 parser is in charge of the following:                      */
  /*                                                                       */
  /*  - provide an implementation of a growing sequence of objects called  */
  /*    a `T1_Table' (used to build various tables needed by the loader).  */
  /*                                                                       */
  /*  - opening .pfb and .pfa files to extract their top-level and private */
  /*    dictionaries.                                                      */
  /*                                                                       */
  /*  - read numbers, arrays & strings from any dictionary.                */
  /*                                                                       */
  /* See `t1load.c' to see how data is loaded from the font file.          */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H

#include "t1parse.h"

#include "t1errors.h"

#include <string.h>     /* for strncmp() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1parse


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   INPUT STREAM PARSER                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


#define IS_T1_WHITESPACE( c )  ( (c) == ' '  || (c) == '\t' )
#define IS_T1_LINESPACE( c )   ( (c) == '\r' || (c) == '\n' )

#define IS_T1_SPACE( c )  ( IS_T1_WHITESPACE( c ) || IS_T1_LINESPACE( c ) )


  typedef struct  PFB_Tag_
  {
    FT_UShort  tag;
    FT_Long    size;

  } PFB_Tag;


#undef  FT_STRUCTURE
#define FT_STRUCTURE  PFB_Tag


  static
  const FT_Frame_Field  pfb_tag_fields[] =
  {
    FT_FRAME_START( 6 ),
      FT_FRAME_USHORT ( tag ),
      FT_FRAME_LONG_LE( size ),
    FT_FRAME_END
  };


  static FT_Error
  read_pfb_tag( FT_Stream   stream,
                FT_UShort*  tag,
                FT_Long*    size )
  {
    FT_Error  error;
    PFB_Tag   head;


    *tag  = 0;
    *size = 0;
    if ( !READ_Fields( pfb_tag_fields, &head ) )
    {
      if ( head.tag == 0x8001 || head.tag == 0x8002 )
      {
        *tag  = head.tag;
        *size = head.size;
      }
    }
    return error;
  }


  FT_LOCAL_DEF FT_Error
  T1_New_Parser( T1_ParserRec*     parser,
                 FT_Stream         stream,
                 FT_Memory         memory,
                 PSAux_Interface*  psaux )
  {
    FT_Error   error;
    FT_UShort  tag;
    FT_Long    size;


    psaux->t1_parser_funcs->init( &parser->root,0, 0, memory );

    parser->stream       = stream;
    parser->base_len     = 0;
    parser->base_dict    = 0;
    parser->private_len  = 0;
    parser->private_dict = 0;
    parser->in_pfb       = 0;
    parser->in_memory    = 0;
    parser->single_block = 0;

    /******************************************************************/
    /*                                                                */
    /* Here a short summary of what is going on:                      */
    /*                                                                */
    /*   When creating a new Type 1 parser, we try to locate and load */
    /*   the base dictionary if this is possible (i.e. for PFB        */
    /*   files).  Otherwise, we load the whole font into memory.      */
    /*                                                                */
    /*   When `loading' the base dictionary, we only setup pointers   */
    /*   in the case of a memory-based stream.  Otherwise, we         */
    /*   allocate and load the base dictionary in it.                 */
    /*                                                                */
    /*   parser->in_pfb is set if we are in a binary (".pfb") font.   */
    /*   parser->in_memory is set if we have a memory stream.         */
    /*                                                                */

    /* try to compute the size of the base dictionary;   */
    /* look for a Postscript binary file tag, i.e 0x8001 */
    if ( FILE_Seek( 0L ) )
      goto Exit;

    error = read_pfb_tag( stream, &tag, &size );
    if ( error )
      goto Exit;

    if ( tag != 0x8001 )
    {
      /* assume that this is a PFA file for now; an error will */
      /* be produced later when more things are checked        */
      if ( FILE_Seek( 0L ) )
        goto Exit;
      size = stream->size;
    }
    else
      parser->in_pfb = 1;

    /* now, try to load `size' bytes of the `base' dictionary we */
    /* found previously                                          */

    /* if it is a memory-based resource, set up pointers */
    if ( !stream->read )
    {
      parser->base_dict = (FT_Byte*)stream->base + stream->pos;
      parser->base_len  = size;
      parser->in_memory = 1;

      /* check that the `size' field is valid */
      if ( FILE_Skip( size ) )
        goto Exit;
    }
    else
    {
      /* read segment in memory */
      if ( ALLOC( parser->base_dict, size )     ||
           FILE_Read( parser->base_dict, size ) )
        goto Exit;
      parser->base_len = size;
    }

    /* Now check font format; we must see `%!PS-AdobeFont-1' */
    /* or `%!FontType'                                       */
    {
      if ( size <= 16                                    ||
           ( strncmp( (const char*)parser->base_dict,
                      "%!PS-AdobeFont-1", 16 )        &&
             strncmp( (const char*)parser->base_dict,
                      "%!FontType", 10 )              )  )
      {
        FT_TRACE2(( "[not a Type1 font]\n" ));
        error = T1_Err_Unknown_File_Format;
      }
      else
      {
        parser->root.base   = parser->base_dict;
        parser->root.cursor = parser->base_dict;
        parser->root.limit  = parser->root.cursor + parser->base_len;
      }
    }

  Exit:
    if ( error && !parser->in_memory )
      FREE( parser->base_dict );

    return error;
  }


  FT_LOCAL_DEF void
  T1_Finalize_Parser( T1_ParserRec*  parser )
  {
    FT_Memory   memory = parser->root.memory;


    /* always free the private dictionary */
    FREE( parser->private_dict );

    /* free the base dictionary only when we have a disk stream */
    if ( !parser->in_memory )
      FREE( parser->base_dict );

    parser->root.funcs.done( &parser->root );
  }


  /* return the value of an hexadecimal digit */
  static int
  hexa_value( char  c )
  {
    unsigned int  d;


    d = (unsigned int)( c - '0' );
    if ( d <= 9 )
      return (int)d;

    d = (unsigned int)( c - 'a' );
    if ( d <= 5 )
      return (int)( d + 10 );

    d = (unsigned int)( c - 'A' );
    if ( d <= 5 )
      return (int)( d + 10 );

    return -1;
  }


  FT_LOCAL_DEF FT_Error
  T1_Get_Private_Dict( T1_ParserRec*     parser,
                       PSAux_Interface*  psaux )
  {
    FT_Stream  stream = parser->stream;
    FT_Memory  memory = parser->root.memory;
    FT_Error   error  = 0;
    FT_Long    size;


    if ( parser->in_pfb )
    {
      /* in the case of the PFB format, the private dictionary can be  */
      /* made of several segments.  We thus first read the number of   */
      /* segments to compute the total size of the private dictionary  */
      /* then re-read them into memory.                                */
      FT_Long    start_pos = FILE_Pos();
      FT_UShort  tag;


      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error )
          goto Fail;

        if ( tag != 0x8002 )
          break;

        parser->private_len += size;

        if ( FILE_Skip( size ) )
          goto Fail;
      }

      /* Check that we have a private dictionary there */
      /* and allocate private dictionary buffer        */
      if ( parser->private_len == 0 )
      {
        FT_ERROR(( "T1_Get_Private_Dict:" ));
        FT_ERROR(( " invalid private dictionary section\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Fail;
      }

      if ( FILE_Seek( start_pos )                             ||
           ALLOC( parser->private_dict, parser->private_len ) )
        goto Fail;

      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error || tag != 0x8002 )
        {
          error = T1_Err_Ok;
          break;
        }

        if ( FILE_Read( parser->private_dict + parser->private_len, size ) )
          goto Fail;

        parser->private_len += size;
      }
    }
    else
    {
      /* we have already `loaded' the whole PFA font file into memory; */
      /* if this is a memory resource, allocate a new block to hold    */
      /* the private dict. Otherwise, simply overwrite into the base   */
      /* dictionary block in the heap.                                 */

      /* first of all, look at the `eexec' keyword */
      FT_Byte*  cur   = parser->base_dict;
      FT_Byte*  limit = cur + parser->base_len;
      FT_Byte   c;


      for (;;)
      {
        c = cur[0];
        if ( c == 'e' && cur + 9 < limit )  /* 9 = 5 letters for `eexec' + */
                                            /* newline + 4 chars           */
        {
          if ( cur[1] == 'e' && cur[2] == 'x' &&
               cur[3] == 'e' && cur[4] == 'c' )
          {
            cur += 6; /* we skip the newling after the `eexec' */

            /* XXX: Some fonts use DOS-linefeeds, i.e. \r\n; we need to */
            /*      skip the extra \n if we find it                     */
            if ( cur[0] == '\n' )
              cur++;

            break;
          }
        }
        cur++;
        if ( cur >= limit )
        {
          FT_ERROR(( "T1_Get_Private_Dict:" ));
          FT_ERROR(( " could not find `eexec' keyword\n" ));
          error = T1_Err_Invalid_File_Format;
          goto Exit;
        }
      }

      /* now determine where to write the _encrypted_ binary private  */
      /* dictionary.  We overwrite the base dictionary for disk-based */
      /* resources and allocate a new block otherwise                 */

      size = (FT_Long)( parser->base_len - ( cur - parser->base_dict ) );

      if ( parser->in_memory )
      {
        /* note that we allocate one more byte to put a terminating `0' */
        if ( ALLOC( parser->private_dict, size + 1 ) )
          goto Fail;
        parser->private_len = size;
      }
      else
      {
        parser->single_block = 1;
        parser->private_dict = parser->base_dict;
        parser->private_len  = size;
        parser->base_dict    = 0;
        parser->base_len     = 0;
      }

      /* now determine whether the private dictionary is encoded in binary */
      /* or hexadecimal ASCII format -- decode it accordingly              */

      /* we need to access the next 4 bytes (after the final \r following */
      /* the `eexec' keyword); if they all are hexadecimal digits, then   */
      /* we have a case of ASCII storage                                  */

      if ( ( hexa_value( cur[0] ) | hexa_value( cur[1] ) |
             hexa_value( cur[2] ) | hexa_value( cur[3] ) ) < 0 )

        /* binary encoding -- `simply' copy the private dict */
        MEM_Copy( parser->private_dict, cur, size );

      else
      {
        /* ASCII hexadecimal encoding */

        FT_Byte*  write;
        FT_Int    count;


        write = parser->private_dict;
        count = 0;

        for ( ;cur < limit; cur++ )
        {
          int  hex1;


          /* check for newline */
          if ( cur[0] == '\r' || cur[0] == '\n' )
            continue;

          /* exit if we have a non-hexadecimal digit that isn't a newline */
          hex1 = hexa_value( cur[0] );
          if ( hex1 < 0 || cur + 1 >= limit )
            break;

          /* otherwise, store byte */
          *write++ = (FT_Byte)( ( hex1 << 4 ) | hexa_value( cur[1] ) );
          count++;
          cur++;
        }

        /* put a safeguard */
        parser->private_len = (FT_Int)( write - parser->private_dict );
        *write++ = 0;
      }
    }

    /* we now decrypt the encoded binary private dictionary */
    psaux->t1_decrypt( parser->private_dict, parser->private_len, 55665U );
    parser->root.base = parser->private_dict;
    parser->root.cursor = parser->private_dict;
    parser->root.limit  = parser->root.cursor + parser->private_len;

  Fail:
  Exit:
    return error;
  }


/* END */
