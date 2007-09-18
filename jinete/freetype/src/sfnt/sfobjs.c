/***************************************************************************/
/*                                                                         */
/*  sfobjs.c                                                               */
/*                                                                         */
/*    SFNT object management (base).                                       */
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


#include <ft2build.h>
#include "sfobjs.h"
#include "ttload.h"
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H

#include "sferrors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_sfobjs


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    Get_Name                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns a given ENGLISH name record in ASCII.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the source face object.                      */
  /*                                                                       */
  /*    nameid :: The name id of the name record to return.                */
  /*                                                                       */
  /* <Return>                                                              */
  /*    Character string.  NULL if no name is present.                     */
  /*                                                                       */
  static FT_String*
  Get_Name( TT_Face    face,
            FT_UShort  nameid )
  {
    FT_Memory    memory = face->root.memory;
    FT_UShort    n;
    TT_NameRec*  rec;
    FT_Bool      wide_chars = 1;


    rec = face->name_table.names;
    for ( n = 0; n < face->name_table.numNameRecords; n++, rec++ )
    {
      if ( rec->nameID == nameid )
      {
        /* found the name -- now create an ASCII string from it */
        FT_Bool  found = 0;


        /* test for Microsoft English language */
        if ( rec->platformID == TT_PLATFORM_MICROSOFT &&
             rec->encodingID <= TT_MS_ID_UNICODE_CS   &&
             ( rec->languageID & 0x3FF ) == 0x009     )
          found = 1;

        /* test for Apple Unicode encoding */
        else if ( rec->platformID == TT_PLATFORM_APPLE_UNICODE )
          found = 1;

        /* test for Apple Roman */
        else if ( rec->platformID == TT_PLATFORM_MACINTOSH &&
                  rec->languageID == TT_MAC_ID_ROMAN       )
        {
          found      = 1;
          wide_chars = 0;
        }

        /* found a Unicode name */
        if ( found )
        {
          FT_String*  string;
          FT_UInt     len;


          if ( wide_chars )
          {
            FT_UInt   m;


            len = (FT_UInt)rec->stringLength / 2;
            if ( MEM_Alloc( string, len + 1 ) )
              return NULL;

            for ( m = 0; m < len; m ++ )
              string[m] = rec->string[2 * m + 1];
          }
          else
          {
            len = rec->stringLength;
            if ( MEM_Alloc( string, len + 1 ) )
              return NULL;

            MEM_Copy( string, rec->string, len );
          }

          string[len] = '\0';
          return string;
        }
      }
    }

    return NULL;
  }


  static FT_Encoding
  find_encoding( int  platform_id,
                 int  encoding_id )
  {
    typedef struct  TEncoding
    {
      int          platform_id;
      int          encoding_id;
      FT_Encoding  encoding;

    } TEncoding;

    static
    const TEncoding  tt_encodings[] =
    {
      { TT_PLATFORM_ISO,           -1,                  ft_encoding_unicode },

      { TT_PLATFORM_APPLE_UNICODE, -1,                  ft_encoding_unicode },

      { TT_PLATFORM_MACINTOSH,     TT_MAC_ID_ROMAN,     ft_encoding_apple_roman },

      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SYMBOL_CS,  ft_encoding_symbol },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UCS_4,      ft_encoding_unicode },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_UNICODE_CS, ft_encoding_unicode },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_SJIS,       ft_encoding_sjis },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_GB2312,     ft_encoding_gb2312 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_BIG_5,      ft_encoding_big5 },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_WANSUNG,    ft_encoding_wansung },
      { TT_PLATFORM_MICROSOFT,     TT_MS_ID_JOHAB,      ft_encoding_johab }
    };

    const TEncoding  *cur, *limit;


    cur   = tt_encodings;
    limit = cur + sizeof ( tt_encodings ) / sizeof ( tt_encodings[0] );

    for ( ; cur < limit; cur++ )
    {
      if ( cur->platform_id == platform_id )
      {
        if ( cur->encoding_id == encoding_id ||
             cur->encoding_id == -1          )
          return cur->encoding;
      }
    }

    return ft_encoding_none;
  }


  FT_LOCAL_DEF FT_Error
  SFNT_Init_Face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error            error;
    FT_Library          library = face->root.driver->root.library;
    SFNT_Interface*     sfnt;
    SFNT_Header         sfnt_header;

    /* for now, parameters are unused */
    FT_UNUSED( num_params );
    FT_UNUSED( params );

    sfnt = (SFNT_Interface*)face->sfnt;
    if ( !sfnt )
    {
      sfnt = (SFNT_Interface*)FT_Get_Module_Interface( library, "sfnt" );
      if ( !sfnt )
      {
        error = SFNT_Err_Invalid_File_Format;
        goto Exit;
      }

      face->sfnt       = sfnt;
      face->goto_table = sfnt->goto_table;
    }

    if ( !face->psnames )
    {
      face->psnames = (PSNames_Interface*)
                       FT_Get_Module_Interface( library, "psnames" );
    }

    /* check that we have a valid TrueType file */
    error = sfnt->load_sfnt_header( face, stream, face_index, &sfnt_header );
    if ( error )
      goto Exit;

    face->format_tag = sfnt_header.format_tag;
    face->num_tables = sfnt_header.num_tables;

    /* Load font directory */
    error = sfnt->load_directory( face, stream, &sfnt_header );
    if ( error )
      goto Exit;

    face->root.num_faces = face->ttc_header.count;
    if ( face->root.num_faces < 1 )
      face->root.num_faces = 1;

  Exit:
    return error;
  }


#undef  LOAD_
#define LOAD_( x )  ( ( error = sfnt->load_##x( face, stream ) ) \
                      != SFNT_Err_Ok )


  FT_LOCAL_DEF FT_Error
  SFNT_Load_Face( FT_Stream      stream,
                  TT_Face        face,
                  FT_Int         face_index,
                  FT_Int         num_params,
                  FT_Parameter*  params )
  {
    FT_Error         error;
    FT_Bool          has_outline;
    FT_Bool          is_apple_sbit;

    SFNT_Interface*  sfnt = (SFNT_Interface*)face->sfnt;

    FT_UNUSED( face_index );
    FT_UNUSED( num_params );
    FT_UNUSED( params );


    /* Load tables */

    /* We now support two SFNT-based bitmapped font formats.  They */
    /* are recognized easily as they do not include a `glyf'       */
    /* table.                                                      */
    /*                                                             */
    /* The first format comes from Apple, and uses a table named   */
    /* `bhed' instead of `head' to store the font header (using    */
    /* the same format).  It also doesn't include horizontal and   */
    /* vertical metrics tables (i.e. `hhea' and `vhea' tables are  */
    /* missing).                                                   */
    /*                                                             */
    /* The other format comes from Microsoft, and is used with     */
    /* WinCE/PocketPC.  It looks like a standard TTF, except that  */
    /* it doesn't contain outlines.                                */
    /*                                                             */

    /* do we have outlines in there? */
    has_outline   = FT_BOOL( ( TT_LookUp_Table( face, TTAG_glyf ) != 0 ) ||
                             ( TT_LookUp_Table( face, TTAG_CFF  ) != 0 ) );
    is_apple_sbit = 0;

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* if this font doesn't contain outlines, we try to load */
    /* a `bhed' table                                        */
    if ( !has_outline )
      is_apple_sbit = FT_BOOL( !LOAD_( bitmap_header ) );

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    /* load the font header (`head' table) if this isn't an Apple */
    /* sbit font file                                             */
    if ( !is_apple_sbit && LOAD_( header ) )
      goto Exit;

    /* load other tables */
    if ( LOAD_( max_profile ) ||
         LOAD_( charmaps )    )
      goto Exit;
      
    /* the following tables are optional in PCL fonts -- */
    /* don't check for errors                            */
    (void)LOAD_( names );
    (void)LOAD_( psnames );

    /* do not load the metrics headers and tables if this is an Apple */
    /* sbit font file                                                 */
    if ( !is_apple_sbit )
    {
      /* load the `hhea' and `hmtx' tables at once */
      error = sfnt->load_metrics( face, stream, 0 );
      if ( error )
        goto Exit;

      /* try to load the `vhea' and `vmtx' tables at once */
      error = sfnt->load_metrics( face, stream, 1 );
      if ( error )
        goto Exit;

      if ( LOAD_( os2 ) )
        goto Exit;
    }

    /* the optional tables */

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

    /* embedded bitmap support. */
    if ( sfnt->load_sbits && LOAD_( sbits ) )
    {
      /* return an error if this font file has no outlines */
      if ( error == SFNT_Err_Table_Missing && has_outline )
        error = SFNT_Err_Ok;
      else
        goto Exit;
    }
#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

    if ( LOAD_( hdmx )    ||
         LOAD_( gasp )    ||
         LOAD_( kerning ) ||
         LOAD_( pclt )    )
      goto Exit;

#ifdef TT_CONFIG_OPTION_EXTEND_ENGINE
    if ( ( error = TT_Extension_Create( face ) ) != SFNT_Err_Ok )
      goto Exit;
#endif

    face->root.family_name = Get_Name( face, TT_NAME_ID_FONT_FAMILY );
    face->root.style_name  = Get_Name( face, TT_NAME_ID_FONT_SUBFAMILY );

    /* now set up root fields */
    {
      FT_Face     root = &face->root;
      FT_Int      flags = 0;
      TT_CharMap  charmap;
      FT_Int      n;
      FT_Memory   memory;


      memory = root->memory;

      /*********************************************************************/
      /*                                                                   */
      /* Compute face flags.                                               */
      /*                                                                   */
      if ( has_outline == TRUE )
        flags = FT_FACE_FLAG_SCALABLE;    /* scalable outlines */

      flags |= FT_FACE_FLAG_SFNT      |   /* SFNT file format  */
               FT_FACE_FLAG_HORIZONTAL;   /* horizontal data   */

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
      /* might need more polish to detect the presence of a Postscript */
      /* name table in the font                                        */
      flags |= FT_FACE_FLAG_GLYPH_NAMES;
#endif

      /* fixed width font? */
      if ( face->postscript.isFixedPitch )
        flags |= FT_FACE_FLAG_FIXED_WIDTH;

      /* vertical information? */
      if ( face->vertical_info )
        flags |= FT_FACE_FLAG_VERTICAL;

      /* kerning available ? */
      if ( face->kern_pairs )
        flags |= FT_FACE_FLAG_KERNING;

      root->face_flags = flags;

      /*********************************************************************/
      /*                                                                   */
      /* Compute style flags.                                              */
      /*                                                                   */
      flags = 0;
      if ( has_outline == TRUE && face->os2.version != 0xFFFF )
      {
        /* we have an OS/2 table; use the `fsSelection' field */
        if ( face->os2.fsSelection & 1 )
          flags |= FT_STYLE_FLAG_ITALIC;

        if ( face->os2.fsSelection & 32 )
          flags |= FT_STYLE_FLAG_BOLD;
      }
      else
      {
        /* this is an old Mac font, use the header field */
        if ( face->header.Mac_Style & 1 )
          flags |= FT_STYLE_FLAG_BOLD;

        if ( face->header.Mac_Style & 2 )
          flags |= FT_STYLE_FLAG_ITALIC;
      }

      root->style_flags = flags;

      /*********************************************************************/
      /*                                                                   */
      /* Polish the charmaps.                                              */
      /*                                                                   */
      /*   Try to set the charmap encoding according to the platform &     */
      /*   encoding ID of each charmap.                                    */
      /*                                                                   */
      charmap            = face->charmaps;
      root->num_charmaps = face->num_charmaps;

      /* allocate table of pointers */
      if ( ALLOC_ARRAY( root->charmaps, root->num_charmaps, FT_CharMap ) )
        goto Exit;

      for ( n = 0; n < root->num_charmaps; n++, charmap++ )
      {
        FT_Int  platform = charmap->cmap.platformID;
        FT_Int  encoding = charmap->cmap.platformEncodingID;


        charmap->root.face        = (FT_Face)face;
        charmap->root.platform_id = (FT_UShort)platform;
        charmap->root.encoding_id = (FT_UShort)encoding;
        charmap->root.encoding    = find_encoding( platform, encoding );

        /* now, set root->charmap with a unicode charmap */
        /* wherever available                            */
        if ( !root->charmap                                &&
             charmap->root.encoding == ft_encoding_unicode )
          root->charmap = (FT_CharMap)charmap;

        root->charmaps[n] = (FT_CharMap)charmap;
      }

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS

      if ( face->num_sbit_strikes )
      {
        root->face_flags |= FT_FACE_FLAG_FIXED_SIZES;

#if 0
        /* I don't know criteria whether layout is horizontal or vertical */
        if ( has_outline.... )
        {
          ...
          root->face_flags |= FT_FACE_FLAG_VERTICAL;
        }
#endif
        root->num_fixed_sizes = face->num_sbit_strikes;

        if ( ALLOC_ARRAY( root->available_sizes,
                          face->num_sbit_strikes,
                          FT_Bitmap_Size ) )
          goto Exit;

        for ( n = 0 ; n < face->num_sbit_strikes ; n++ )
        {
          root->available_sizes[n].width =
            face->sbit_strikes[n].x_ppem;

          root->available_sizes[n].height =
            face->sbit_strikes[n].y_ppem;
        }
      }
      else

#endif /* TT_CONFIG_OPTION_EMBEDDED_BITMAPS */

      {
        root->num_fixed_sizes = 0;
        root->available_sizes = 0;
      }

      /*********************************************************************/
      /*                                                                   */
      /*  Set up metrics.                                                  */
      /*                                                                   */
      if ( has_outline == TRUE )
      {
        /* XXX What about if outline header is missing */
        /*     (e.g. sfnt wrapped outline)?            */
        root->bbox.xMin    = face->header.xMin;
        root->bbox.yMin    = face->header.yMin;
        root->bbox.xMax    = face->header.xMax;
        root->bbox.yMax    = face->header.yMax;
        root->units_per_EM = face->header.Units_Per_EM;


        /* XXX: Computing the ascender/descender/height is very different */
        /*      from what the specification tells you.  Apparently, we    */
        /*      must be careful because                                   */
        /*                                                                */
        /*      - not all fonts have an OS/2 table; in this case, we take */
        /*        the values in the horizontal header.  However, these    */
        /*        values very often are not reliable.                     */
        /*                                                                */
        /*      - otherwise, the correct typographic values are in the    */
        /*        sTypoAscender, sTypoDescender & sTypoLineGap fields.    */
        /*                                                                */
        /*        However, certains fonts have these fields set to 0.     */
        /*        Rather, they have usWinAscent & usWinDescent correctly  */
        /*        set (but with different values).                        */
        /*                                                                */
        /*      As an example, Arial Narrow is implemented through four   */
        /*      files ARIALN.TTF, ARIALNI.TTF, ARIALNB.TTF & ARIALNBI.TTF */
        /*                                                                */
        /*      Strangely, all fonts have the same values in their        */
        /*      sTypoXXX fields, except ARIALNB which sets them to 0.     */
        /*                                                                */
        /*      On the other hand, they all have different                */
        /*      usWinAscent/Descent values -- as a conclusion, the OS/2   */
        /*      table cannot be used to compute the text height reliably! */
        /*                                                                */

        /* The ascender/descender/height are computed from the OS/2 table */
        /* when found.  Otherwise, they're taken from the horizontal      */
        /* header.                                                        */
        /*                                                                */

        root->ascender  = face->horizontal.Ascender;
        root->descender = face->horizontal.Descender;

        root->height    = (FT_Short)( root->ascender - root->descender +
                                      face->horizontal.Line_Gap );

        /* if the line_gap is 0, we add an extra 15% to the text height --  */
        /* this computation is based on various versions of Times New Roman */
        if ( face->horizontal.Line_Gap == 0 )
          root->height = (FT_Short)( ( root->height * 115 + 50 ) / 100 );

#if 0

        /* some fonts have the OS/2 "sTypoAscender", "sTypoDescender" & */
        /* "sTypoLineGap" fields set to 0, like ARIALNB.TTF             */
        if ( face->os2.version != 0xFFFF && root->ascender )
        {
          FT_Int  height;


          root->ascender  =  face->os2.sTypoAscender;
          root->descender = -face->os2.sTypoDescender;

          height = root->ascender + root->descender + face->os2.sTypoLineGap;
          if ( height > root->height )
            root->height = height;
        }

#endif /* 0 */

        root->max_advance_width   = face->horizontal.advance_Width_Max;

        root->max_advance_height  = (FT_Short)( face->vertical_info
                                      ? face->vertical.advance_Height_Max
                                      : root->height );

        root->underline_position  = face->postscript.underlinePosition;
        root->underline_thickness = face->postscript.underlineThickness;

        /* root->max_points   -- already set up */
        /* root->max_contours -- already set up */
      }
    }

  Exit:
    return error;
  }


#undef LOAD_


  FT_LOCAL_DEF void
  SFNT_Done_Face( TT_Face  face )
  {
    FT_Memory        memory = face->root.memory;
    SFNT_Interface*  sfnt   = (SFNT_Interface*)face->sfnt;


    if ( sfnt )
    {
      /* destroy the postscript names table if it is loaded */
      if ( sfnt->free_psnames )
        sfnt->free_psnames( face );

      /* destroy the embedded bitmaps table if it is loaded */
      if ( sfnt->free_sbits )
        sfnt->free_sbits( face );
    }

    /* freeing the kerning table */
    FREE( face->kern_pairs );
    face->num_kern_pairs = 0;

    /* freeing the collection table */
    FREE( face->ttc_header.offsets );
    face->ttc_header.count = 0;

    /* freeing table directory */
    FREE( face->dir_tables );
    face->num_tables = 0;

    /* freeing the character mapping tables */
    if ( sfnt && sfnt->load_charmaps )
    {
      FT_UShort  n;


      for ( n = 0; n < face->num_charmaps; n++ )
        sfnt->free_charmap( face, &face->charmaps[n].cmap );
    }

    FREE( face->charmaps );
    face->num_charmaps = 0;

    FREE( face->root.charmaps );
    face->root.num_charmaps = 0;
    face->root.charmap      = 0;

    /* freeing the horizontal metrics */
    FREE( face->horizontal.long_metrics );
    FREE( face->horizontal.short_metrics );

    /* freeing the vertical ones, if any */
    if ( face->vertical_info )
    {
      FREE( face->vertical.long_metrics  );
      FREE( face->vertical.short_metrics );
      face->vertical_info = 0;
    }

    /* freeing the gasp table */
    FREE( face->gasp.gaspRanges );
    face->gasp.numRanges = 0;

    /* freeing the name table */
    sfnt->free_names( face );

    /* freeing the hdmx table */
    sfnt->free_hdmx( face );

    /* freeing family and style name */
    FREE( face->root.family_name );
    FREE( face->root.style_name );

    /* freeing sbit size table */
    face->root.num_fixed_sizes = 0;
    if ( face->root.available_sizes )
      FREE( face->root.available_sizes );

    face->sfnt = 0;
  }


/* END */
