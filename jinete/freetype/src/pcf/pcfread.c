/*  pcfread.c

    FreeType font driver for pcf fonts

  Copyright 2000-2001 by
  Francesco Zappa Nardelli

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <ft2build.h>

#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_OBJECTS_H

#include "pcf.h"
#include "pcfdriver.h"

#include "pcferror.h"

#include <string.h>     /* strlen(), strcpy() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_pcfread


#if defined( FT_DEBUG_LEVEL_TRACE )
  static const char*  tableNames[] =
  {
    "prop", "accl", "mtrcs", "bmps", "imtrcs",
    "enc", "swidth", "names", "accel"
  };
#endif


  static
  const FT_Frame_Field  pcf_toc_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_TocRec

    FT_FRAME_START( 8 ),
      FT_FRAME_ULONG_LE( version ),
      FT_FRAME_ULONG_LE( count ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_table_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_TableRec

    FT_FRAME_START( 16  ),
      FT_FRAME_ULONG_LE( type ),
      FT_FRAME_ULONG_LE( format ),
      FT_FRAME_ULONG_LE( size ),
      FT_FRAME_ULONG_LE( offset ),
    FT_FRAME_END
  };


  static FT_Error
  pcf_read_TOC( FT_Stream  stream,
                PCF_Face   face )
  {
    FT_Error   error;
    PCF_Toc    toc = &face->toc;
    PCF_Table  tables;

    FT_Memory     memory = FT_FACE(face)->memory;
    unsigned int  n;


    if ( FILE_Seek ( 0 )                   ||
         READ_Fields ( pcf_toc_header, toc ) )
      return PCF_Err_Cannot_Open_Resource;

    if ( toc->version != PCF_FILE_VERSION )
      return PCF_Err_Invalid_File_Format;

    if ( ALLOC( face->toc.tables, toc->count * sizeof ( PCF_TableRec ) ) )
      return PCF_Err_Out_Of_Memory;

    tables = face->toc.tables;
    for ( n = 0; n < toc->count; n++ )
    {
      if ( READ_Fields( pcf_table_header, tables ) )
        goto Exit;
      tables++;
    }

#if defined( FT_DEBUG_LEVEL_TRACE )

    {
      unsigned int  i, j;
      const char    *name = "?";


      FT_TRACE4(( "Tables count: %ld\n", face->toc.count ));
      tables = face->toc.tables;
      for ( i = 0; i < toc->count; i++ )
      {
        for( j = 0; j < sizeof ( tableNames ) / sizeof ( tableNames[0] ); j++ )
          if ( tables[i].type == (unsigned int)( 1 << j ) )
            name = tableNames[j];
        FT_TRACE4(( "Table %d: type=%-6s format=0x%04lX "
                    "size=0x%06lX (%8ld) offset=0x%04lX\n",
                    i, name,
                    tables[i].format,
                    tables[i].size, tables[i].size,
                    tables[i].offset ));
      }
    }

#endif

    return PCF_Err_Ok;

  Exit:
    FREE( face->toc.tables );
    return error;
  }


  static
  const FT_Frame_Field  pcf_metric_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_MetricRec

    FT_FRAME_START( 12 ),
      FT_FRAME_SHORT_LE( leftSideBearing ),
      FT_FRAME_SHORT_LE( rightSideBearing ),
      FT_FRAME_SHORT_LE( characterWidth ),
      FT_FRAME_SHORT_LE( ascent ),
      FT_FRAME_SHORT_LE( descent ),
      FT_FRAME_SHORT_LE( attributes ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_metric_msb_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_MetricRec

    FT_FRAME_START( 12 ),
      FT_FRAME_SHORT( leftSideBearing ),
      FT_FRAME_SHORT( rightSideBearing ),
      FT_FRAME_SHORT( characterWidth ),
      FT_FRAME_SHORT( ascent ),
      FT_FRAME_SHORT( descent ),
      FT_FRAME_SHORT( attributes ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_compressed_metric_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_Compressed_MetricRec

    FT_FRAME_START( 5 ),
      FT_FRAME_BYTE( leftSideBearing ),
      FT_FRAME_BYTE( rightSideBearing ),
      FT_FRAME_BYTE( characterWidth ),
      FT_FRAME_BYTE( ascent ),
      FT_FRAME_BYTE( descent ),
    FT_FRAME_END
  };


  static FT_Error
  pcf_parse_metric( FT_Stream              stream,
                    const FT_Frame_Field*  header,
                    PCF_Metric             metric )
  {
    FT_Error  error = PCF_Err_Ok;


    if ( READ_Fields( header, metric ) )
      return error;

    return PCF_Err_Ok;
  }


  static FT_Error
  pcf_parse_compressed_metric( FT_Stream   stream,
                               PCF_Metric  metric )
  {
    PCF_Compressed_MetricRec  compr_metric;
    FT_Error                  error = PCF_Err_Ok;


    if ( READ_Fields( pcf_compressed_metric_header, &compr_metric ) )
      return error;

    metric->leftSideBearing =
      (FT_Short)( compr_metric.leftSideBearing - 0x80 );
    metric->rightSideBearing =
      (FT_Short)( compr_metric.rightSideBearing - 0x80 );
    metric->characterWidth =
      (FT_Short)( compr_metric.characterWidth - 0x80 );
    metric->ascent =
      (FT_Short)( compr_metric.ascent - 0x80 );
    metric->descent =
      (FT_Short)( compr_metric.descent - 0x80 );
    metric->attributes = 0;

    return PCF_Err_Ok;
  }


  static FT_Error
  pcf_get_metric( FT_Stream   stream,
                  FT_ULong    format,
                  PCF_Metric  metric )
  {
    FT_Error error = PCF_Err_Ok;


    if ( PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        error = pcf_parse_metric( stream, pcf_metric_msb_header, metric );
      else
        error = pcf_parse_metric( stream, pcf_metric_header, metric );
    }
    else
      error = pcf_parse_compressed_metric( stream, metric );

    return error;
  }


  static FT_Error
  pcfSeekToType( FT_Stream  stream,
                 PCF_Table  tables,
                 int        ntables,
                 FT_ULong   type,
                 FT_ULong*  formatp,
                 FT_ULong*  sizep )
  {
    FT_Error error;
    int      i;


    for ( i = 0; i < ntables; i++ )
      if ( tables[i].type == type )
      {
        if ( stream->pos > tables[i].offset )
          return PCF_Err_Invalid_Stream_Skip;
        if ( FILE_Skip( tables[i].offset - stream->pos ) )
          return PCF_Err_Invalid_Stream_Skip;
        *sizep   = tables[i].size;  /* unused - to be removed */
        *formatp = tables[i].format;
        return PCF_Err_Ok;
      }

    return PCF_Err_Invalid_File_Format;
  }


  static FT_Bool
  pcfHasType( PCF_Table  tables,
              int        ntables,
              FT_ULong   type )
  {
    int i;


    for ( i = 0; i < ntables; i++ )
      if ( tables[i].type == type )
        return TRUE;

    return FALSE;
  }


  static
  const FT_Frame_Field  pcf_property_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_ParsePropertyRec

    FT_FRAME_START( 9 ),
      FT_FRAME_LONG_LE( name ),
      FT_FRAME_BYTE   ( isString ),
      FT_FRAME_LONG_LE( value ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_property_msb_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_ParsePropertyRec

    FT_FRAME_START( 9 ),
      FT_FRAME_LONG( name ),
      FT_FRAME_BYTE( isString ),
      FT_FRAME_LONG( value ),
    FT_FRAME_END
  };


  static PCF_Property
  find_property( PCF_Face          face,
                 const FT_String*  prop )
  {
    PCF_Property  properties = face->properties;
    FT_Bool       found      = 0;
    int           i;


    for ( i = 0 ; i < face->nprops && !found; i++ )
    {
      if ( !strcmp( properties[i].name, prop ) )
        found = 1;
    }

    if ( found )
      return properties + i - 1;
    else
      return NULL;
  }


  static FT_Error
  pcf_get_properties( FT_Stream  stream,
                      PCF_Face   face )
  {
    PCF_ParseProperty  props      = 0;
    PCF_Property       properties = 0;
    int                nprops, i;
    FT_ULong           format, size;
    FT_Error           error;
    FT_Memory          memory     = FT_FACE(face)->memory;
    FT_ULong           string_size;
    FT_String*         strings    = 0;


    error = pcfSeekToType( stream,
                           face->toc.tables,
                           face->toc.count,
                           PCF_PROPERTIES,
                           &format,
                           &size );
    if ( error )
      goto Bail;

    if ( READ_ULongLE( format ) )
      goto Bail;

    FT_TRACE4(( "get_prop: format = %ld\n", format ));

    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
      goto Bail;

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      (void)READ_ULong( nprops );
    else
      (void)READ_ULongLE( nprops );
    if ( error )
      goto Bail;

    FT_TRACE4(( "get_prop: nprop = %d\n", nprops ));

    if ( ALLOC( props, nprops * sizeof ( PCF_ParsePropertyRec ) ) )
      goto Bail;

    for ( i = 0; i < nprops; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      {
        if ( READ_Fields( pcf_property_msb_header, props + i ) )
          goto Bail;
      }
      else
      {
        if ( READ_Fields( pcf_property_header, props + i ) )
          goto Bail;
      }
    }

    /* pad the property array                                            */
    /*                                                                   */
    /* clever here - nprops is the same as the number of odd-units read, */
    /* as only isStringProp are odd length   (Keith Packard)             */
    /*                                                                   */
    if ( nprops & 3 )
    {
      i = 4 - ( nprops & 3 );
      FT_Skip_Stream( stream, i );
    }

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      (void)READ_ULong( string_size );
    else
      (void)READ_ULongLE( string_size );
    if ( error )
      goto Bail;

    FT_TRACE4(( "get_prop: string_size = %ld\n", string_size ));

    if ( ALLOC( strings, string_size * sizeof ( char ) ) )
      goto Bail;

    error = FT_Read_Stream( stream, (FT_Byte*)strings, string_size );
    if ( error )
      goto Bail;

    if ( ALLOC( properties, nprops * sizeof ( PCF_PropertyRec ) ) )
      goto Bail;

    for ( i = 0; i < nprops; i++ )
    {
      /* XXX: make atom */
      if ( ALLOC( properties[i].name,
                     ( strlen( strings + props[i].name ) + 1 ) *
                       sizeof ( char ) ) )
        goto Bail;
      strcpy( properties[i].name,strings + props[i].name );

      properties[i].isString = props[i].isString;

      if ( props[i].isString )
      {
        if ( ALLOC( properties[i].value.atom,
                       ( strlen( strings + props[i].value ) + 1 ) *
                         sizeof ( char ) ) )
          goto Bail;
        strcpy( properties[i].value.atom, strings + props[i].value );
      }
      else
        properties[i].value.integer = props[i].value;
    }

    face->properties = properties;
    face->nprops = nprops;

    FREE( props );
    FREE( strings );

    return PCF_Err_Ok;

  Bail:
    FREE( props );
    FREE( strings );

    return error;
  }


  static FT_Error
  pcf_get_metrics( FT_Stream  stream,
                   PCF_Face   face )
  {
    FT_Error    error    = PCF_Err_Ok;
    FT_Memory   memory   = FT_FACE(face)->memory;
    FT_ULong    format   = 0;
    FT_ULong    size     = 0;
    PCF_Metric  metrics  = 0;
    int         i;
    int         nmetrics = -1;


    error = pcfSeekToType( stream,
                           face->toc.tables,
                           face->toc.count,
                           PCF_METRICS,
                           &format,
                           &size );
    if ( error )
      return error;

    error = READ_ULongLE( format );

    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT )   &&
         !PCF_FORMAT_MATCH( format, PCF_COMPRESSED_METRICS ) )
      return PCF_Err_Invalid_File_Format;

    if ( PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)READ_ULong( nmetrics );
      else
        (void)READ_ULongLE( nmetrics );
    }
    else
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)READ_UShort( nmetrics );
      else
        (void)READ_UShortLE( nmetrics );
    }
    if ( error || nmetrics == -1 )
      return PCF_Err_Invalid_File_Format;

    face->nmetrics = nmetrics;

    if ( ALLOC( face->metrics, nmetrics * sizeof ( PCF_MetricRec ) ) )
      return PCF_Err_Out_Of_Memory;

    metrics = face->metrics;
    for ( i = 0; i < nmetrics; i++ )
    {
      pcf_get_metric( stream, format, metrics + i );

      metrics[i].bits = 0;

      FT_TRACE4(( "%d : width=%d, "
                  "lsb=%d, rsb=%d, ascent=%d, descent=%d, swidth=%d\n",
                  i,
                  ( metrics + i )->characterWidth,
                  ( metrics + i )->leftSideBearing,
                  ( metrics + i )->rightSideBearing,
                  ( metrics + i )->ascent,
                  ( metrics + i )->descent,
                  ( metrics + i )->attributes ));

      if ( error )
        break;
    }

    if ( error )
      FREE( face->metrics );
    return error;
  }


  static FT_Error
  pcf_get_bitmaps( FT_Stream  stream,
                   PCF_Face   face )
  {
    FT_Error   error  = PCF_Err_Ok;
    FT_Memory  memory = FT_FACE(face)->memory;
    FT_Long*   offsets;
    FT_Long    bitmapSizes[GLYPHPADOPTIONS];
    FT_ULong   format, size;
    int        nbitmaps, i, sizebitmaps = 0;
    char*      bitmaps;


    error = pcfSeekToType( stream,
                           face->toc.tables,
                           face->toc.count,
                           PCF_BITMAPS,
                           &format,
                           &size );
    if ( error )
      return error;

    error = FT_Access_Frame( stream, 8 );
    if ( error )
      return error;
    format = GET_ULongLE();
    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
      return PCF_Err_Invalid_File_Format;

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
      nbitmaps  = GET_ULong();
    else
      nbitmaps  = GET_ULongLE();
    FT_Forget_Frame( stream );
    if ( nbitmaps != face->nmetrics )
      return PCF_Err_Invalid_File_Format;

    if ( ALLOC( offsets, nbitmaps * sizeof ( FT_ULong ) ) )
      return error;

    if ( error )
      goto Bail;
    for ( i = 0; i < nbitmaps; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)READ_Long( offsets[i] );
      else
        (void)READ_LongLE( offsets[i] );

      FT_TRACE4(( "bitmap %d is at offset %ld\n", i, offsets[i] ));
    }
    if ( error )
      goto Bail;

    if ( error )
      goto Bail;
    for ( i = 0; i < GLYPHPADOPTIONS; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        (void)READ_Long( bitmapSizes[i] );
      else
        (void)READ_LongLE( bitmapSizes[i] );
      if ( error )
        goto Bail;

      sizebitmaps = bitmapSizes[PCF_GLYPH_PAD_INDEX( format )];

      FT_TRACE4(( "padding %d implies a size of %ld\n", i, bitmapSizes[i] ));
    }

    FT_TRACE4(( "  %d bitmaps, padding index %ld\n",
                nbitmaps,
                PCF_GLYPH_PAD_INDEX( format ) ));
    FT_TRACE4(( "bitmap size = %d\n", sizebitmaps ));

    for ( i = 0; i < nbitmaps; i++ )
      face->metrics[i].bits = stream->pos + offsets[i];

    face->bitmapsFormat = format;

    FREE ( offsets );
    return error;

  Bail:
    FREE ( offsets );
    FREE ( bitmaps );
    return error;
  }


  static FT_Error
  pcf_get_encodings( FT_Stream  stream,
                     PCF_Face   face )
  {
    FT_Error      error   = PCF_Err_Ok;
    FT_Memory     memory  = FT_FACE(face)->memory;
    FT_ULong      format, size;
    int           firstCol, lastCol;
    int           firstRow, lastRow;
    int           nencoding, encodingOffset;
    int           i, j;
    PCF_Encoding  tmpEncoding, encoding = 0;


    error = pcfSeekToType( stream,
                           face->toc.tables,
                           face->toc.count,
                           PCF_BDF_ENCODINGS,
                           &format,
                           &size );
    if ( error )
      return error;

    error = FT_Access_Frame( stream, 14 );
    if ( error )
      return error;
    format = GET_ULongLE();
    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT ) )
      return PCF_Err_Invalid_File_Format;

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
    {
      firstCol          = GET_Short();
      lastCol           = GET_Short();
      firstRow          = GET_Short();
      lastRow           = GET_Short();
      face->defaultChar = GET_Short();
    }
    else
    {
      firstCol          = GET_ShortLE();
      lastCol           = GET_ShortLE();
      firstRow          = GET_ShortLE();
      lastRow           = GET_ShortLE();
      face->defaultChar = GET_ShortLE();
    }

    FT_Forget_Frame( stream );

    FT_TRACE4(( "enc: firstCol %d, lastCol %d, firstRow %d, lastRow %d\n",
                firstCol, lastCol, firstRow, lastRow ));

    nencoding = ( lastCol - firstCol + 1 ) * ( lastRow - firstRow + 1 );

    if ( ALLOC( tmpEncoding, nencoding * sizeof ( PCF_EncodingRec ) ) )
      return PCF_Err_Out_Of_Memory;

    error = FT_Access_Frame( stream, 2 * nencoding );
    if ( error )
      goto Bail;

    for ( i = 0, j = 0 ; i < nencoding; i++ )
    {
      if ( PCF_BYTE_ORDER( format ) == MSBFirst )
        encodingOffset = GET_Short();
      else
        encodingOffset = GET_ShortLE();

      if ( encodingOffset != -1 )
      {
        tmpEncoding[j].enc = ( ( ( i / ( lastCol - firstCol + 1 ) ) +
                                 firstRow ) * 256 ) +
                               ( ( i % ( lastCol - firstCol + 1 ) ) +
                                 firstCol );

        tmpEncoding[j].glyph = (FT_Short)encodingOffset;
        j++;
      }

      FT_TRACE4(( "enc n. %d ; Uni %ld ; Glyph %d\n",
                  i, tmpEncoding[j - 1].enc, encodingOffset ));
    }
    FT_Forget_Frame( stream );

    if ( ALLOC( encoding, (--j) * sizeof ( PCF_EncodingRec ) ) )
      goto Bail;

    for ( i = 0; i < j; i++ )
    {
      encoding[i].enc   = tmpEncoding[i].enc;
      encoding[i].glyph = tmpEncoding[i].glyph;
    }

    face->nencodings = j;
    face->encodings  = encoding;
    FREE( tmpEncoding );

    return error;

  Bail:
    FREE( encoding );
    FREE( tmpEncoding );
    return error;
  }


  static
  const FT_Frame_Field  pcf_accel_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_AccelRec

    FT_FRAME_START( 20 ),
      FT_FRAME_BYTE      ( noOverlap ),
      FT_FRAME_BYTE      ( constantMetrics ),
      FT_FRAME_BYTE      ( terminalFont ),
      FT_FRAME_BYTE      ( constantWidth ),
      FT_FRAME_BYTE      ( inkInside ),
      FT_FRAME_BYTE      ( inkMetrics ),
      FT_FRAME_BYTE      ( drawDirection ),
      FT_FRAME_SKIP_BYTES( 1 ),
      FT_FRAME_LONG_LE   ( fontAscent ),
      FT_FRAME_LONG_LE   ( fontDescent ),
      FT_FRAME_LONG_LE   ( maxOverlap ),
    FT_FRAME_END
  };


  static
  const FT_Frame_Field  pcf_accel_msb_header[] =
  {
#undef  FT_STRUCTURE
#define FT_STRUCTURE  PCF_AccelRec

    FT_FRAME_START( 20 ),
      FT_FRAME_BYTE      ( noOverlap ),
      FT_FRAME_BYTE      ( constantMetrics ),
      FT_FRAME_BYTE      ( terminalFont ),
      FT_FRAME_BYTE      ( constantWidth ),
      FT_FRAME_BYTE      ( inkInside ),
      FT_FRAME_BYTE      ( inkMetrics ),
      FT_FRAME_BYTE      ( drawDirection ),
      FT_FRAME_SKIP_BYTES( 1 ),
      FT_FRAME_LONG      ( fontAscent ),
      FT_FRAME_LONG      ( fontDescent ),
      FT_FRAME_LONG      ( maxOverlap ),
    FT_FRAME_END
  };


  static FT_Error
  pcf_get_accel( FT_Stream  stream,
                 PCF_Face   face,
                 FT_ULong   type )
  {
    FT_ULong   format, size;
    FT_Error   error = PCF_Err_Ok;
    PCF_Accel  accel = &face->accel;


    error = pcfSeekToType( stream,
                           face->toc.tables,
                           face->toc.count,
                           type,
                           &format,
                           &size );
    if ( error )
      goto Bail;

    error = READ_ULongLE( format );
    if ( !PCF_FORMAT_MATCH( format, PCF_DEFAULT_FORMAT )  &&
         !PCF_FORMAT_MATCH( format, PCF_ACCEL_W_INKBOUNDS ) )
      goto Bail;

    if ( PCF_BYTE_ORDER( format ) == MSBFirst )
    {
      if ( READ_Fields( pcf_accel_msb_header, accel ) )
        goto Bail;
    }
    else
    {
      if ( READ_Fields( pcf_accel_header, accel ) )
        goto Bail;
    }

    error = pcf_get_metric( stream, format, &(accel->minbounds) );
    if ( error )
      goto Bail;
    error = pcf_get_metric( stream, format, &(accel->maxbounds) );
    if ( error )
      goto Bail;

    if ( PCF_FORMAT_MATCH( format, PCF_ACCEL_W_INKBOUNDS ) )
    {
      error = pcf_get_metric( stream, format, &(accel->ink_minbounds) );
      if ( error )
        goto Bail;
      error = pcf_get_metric( stream, format, &(accel->ink_maxbounds) );
      if ( error )
        goto Bail;
    }
    else
    {
      accel->ink_minbounds = accel->minbounds; /* I'm not sure about this */
      accel->ink_maxbounds = accel->maxbounds;
    }
    return error;

  Bail:
    return error;
  }


  FT_LOCAL_DEF FT_Error
  pcf_load_font( FT_Stream  stream,
                 PCF_Face   face )
  {
    FT_Error   error  = PCF_Err_Ok;
    FT_Memory  memory = FT_FACE(face)->memory;
    FT_Bool    hasBDFAccelerators;


    error = pcf_read_TOC( stream, face );
    if ( error )
      return error;

    error = pcf_get_properties( stream, face );
    if ( error )
      return error;;

    /* Use the old accelerators if no BDF accelerators are in the file. */
    hasBDFAccelerators = pcfHasType( face->toc.tables,
                                     face->toc.count,
                                     PCF_BDF_ACCELERATORS );
    if ( !hasBDFAccelerators )
    {
      error = pcf_get_accel( stream, face, PCF_ACCELERATORS );
      if ( error )
        goto Bail;
    }

    /* metrics */
    error = pcf_get_metrics( stream, face );
    if ( error )
      goto Bail;

    /* bitmaps */
    error = pcf_get_bitmaps( stream, face );
    if ( error )
      goto Bail;

    /* encodings */
    error = pcf_get_encodings( stream, face );
    if ( error )
      goto Bail;

    /* BDF style accelerators (i.e. bounds based on encoded glyphs) */
    if ( hasBDFAccelerators )
    {
      error = pcf_get_accel( stream, face, PCF_BDF_ACCELERATORS );
      if ( error )
        goto Bail;
    }

    /* XXX: TO DO: inkmetrics and glyph_names are missing */

    /* now construct the face object */
    {
      FT_Face       root = FT_FACE( face );
      PCF_Property  prop;
      int           size_set = 0;


      root->num_faces = 1;
      root->face_index = 0;
      root->face_flags = FT_FACE_FLAG_FIXED_SIZES |
                         FT_FACE_FLAG_HORIZONTAL  |
                         FT_FACE_FLAG_FAST_GLYPHS;

      if ( face->accel.constantWidth )
        root->face_flags |= FT_FACE_FLAG_FIXED_WIDTH;

      root->style_flags = 0;
      prop = find_property( face, "SLANT" );
      if ( prop != NULL )
        if ( prop->isString )
          if ( ( *(prop->value.atom) == 'O' ) ||
               ( *(prop->value.atom) == 'I' ) )
            root->style_flags |= FT_STYLE_FLAG_ITALIC;

      prop = find_property( face, "WEIGHT_NAME" );
      if ( prop != NULL )
        if ( prop->isString )
          if ( *(prop->value.atom) == 'B' )
            root->style_flags |= FT_STYLE_FLAG_BOLD;

      root->style_name = (char *)"Regular";

      if ( root->style_flags & FT_STYLE_FLAG_BOLD ) {
        if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
          root->style_name = (char *)"Bold Italic";
        else
          root->style_name = (char *)"Bold";
      }
      else if ( root->style_flags & FT_STYLE_FLAG_ITALIC )
        root->style_name = (char *)"Italic";

      prop = find_property( face, "FAMILY_NAME" );
      if ( prop != NULL )
      {
        if ( prop->isString )
        {
          int  l = strlen( prop->value.atom ) + 1;


          if ( ALLOC( root->family_name, l * sizeof ( char ) ) )
            goto Bail;
          strcpy( root->family_name, prop->value.atom );
        }
      }
      else
        root->family_name = 0;

      root->num_glyphs = face->nmetrics;

      root->num_fixed_sizes = 1;
      if ( ALLOC_ARRAY( root->available_sizes, 1, FT_Bitmap_Size ) )
        goto Bail;

      prop = find_property( face, "PIXEL_SIZE" );
      if ( prop != NULL )
      {
        root->available_sizes->width  = (FT_Short)( prop->value.integer );
        root->available_sizes->height = (FT_Short)( prop->value.integer );
        
        size_set = 1;
      }
      else 
      {
        prop = find_property( face, "POINT_SIZE" );
        if ( prop != NULL )
        {
          PCF_Property  xres = 0, yres = 0;


          xres = find_property( face, "RESOLUTION_X" );
          yres = find_property( face, "RESOLUTION_Y" );
              
          if ( ( xres != NULL ) && ( yres != NULL ) )
          {
            root->available_sizes->width =
              (FT_Short)( prop->value.integer *  
                          xres->value.integer / 720 );
            root->available_sizes->height =
              (FT_Short)( prop->value.integer *  
                          yres->value.integer / 720 ); 
                  
            size_set = 1;
          }
        }
      }

      if (size_set == 0 )
      {
#if 0
        printf( "PCF Warning: Pixel Size undefined, assuming 12\n");
#endif
        root->available_sizes->width  = 12;
        root->available_sizes->height = 12;
      }

      /* XXX: charmaps */
      root->charmaps     = &face->charmap_handle;
      root->num_charmaps = 1;

      {
        PCF_Property  charset_registry = 0, charset_encoding = 0;


        charset_registry = find_property( face, "CHARSET_REGISTRY" );
        charset_encoding = find_property( face, "CHARSET_ENCODING" );

        if ( ( charset_registry != NULL ) &&
             ( charset_encoding != NULL ) )
        {
          if ( ( charset_registry->isString ) &&
               ( charset_encoding->isString ) )
          {
            if ( ALLOC( face->charset_encoding,
                        ( strlen( charset_encoding->value.atom ) + 1 ) *
                          sizeof ( char ) ) )
              goto Bail;
            if ( ALLOC( face->charset_registry,
                        ( strlen( charset_registry->value.atom ) + 1 ) *
                          sizeof ( char ) ) )
              goto Bail;
            strcpy( face->charset_registry, charset_registry->value.atom );
            strcpy( face->charset_encoding, charset_encoding->value.atom );

#if 0
            if ( !strcmp( charset_registry, "ISO10646" ) )
            {
              face->charmap.encoding    = ft_encoding_unicode;
              face->charmap.platform_id = 3;
              face->charmap.encoding_id = 1;
              face->charmap.face        = root;
              face->charmap_handle

              return PCF_Err_Ok;
            }
#endif
          }
        }
      }

      face->charmap.encoding    = ft_encoding_none;
      face->charmap.platform_id = 0;
      face->charmap.encoding_id = 0;
      face->charmap.face        = root;
      face->charmap_handle      = &face->charmap;
      root->charmap             = face->charmap_handle;
    }
    return PCF_Err_Ok;

  Bail:
    PCF_Done_Face( face );
    return PCF_Err_Invalid_File_Format;
  }


/* END */
