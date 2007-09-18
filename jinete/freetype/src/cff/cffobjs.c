/***************************************************************************/
/*                                                                         */
/*  cffobjs.c                                                              */
/*                                                                         */
/*    OpenType objects manager (body).                                     */
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
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_ERRORS_H
#include FT_TRUETYPE_IDS_H
#include FT_TRUETYPE_TAGS_H
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_POSTSCRIPT_NAMES_H
#include FT_INTERNAL_POSTSCRIPT_HINTS_H
#include "cffobjs.h"
#include "cffload.h"

#include "cfferrs.h"

#include <string.h>         /* for strlen() */


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffobjs




  /*************************************************************************/
  /*                                                                       */
  /*                            SIZE FUNCTIONS                             */
  /*                                                                       */
  /*  Note that we store the global hints in the size's "internal" root    */
  /*  field.                                                               */
  /*                                                                       */
  /*************************************************************************/


  static PSH_Globals_Funcs
  CFF_Size_Get_Globals_Funcs( CFF_Size  size )
  {
    CFF_Face             face     = (CFF_Face)size->face;
    CFF_Font*            font     = face->extra.data;
    PSHinter_Interface*  pshinter = font->pshinter;
    FT_Module            module;


    module = FT_Get_Module( size->face->driver->root.library,
                            "pshinter" );
    return ( module && pshinter && pshinter->get_globals_funcs )
           ? pshinter->get_globals_funcs( module )
           : 0;
  }


  FT_LOCAL_DEF void
  CFF_Size_Done( CFF_Size  size )
  {
    if ( size->internal )
    {
      PSH_Globals_Funcs  funcs;


      funcs = CFF_Size_Get_Globals_Funcs( size );
      if ( funcs )
        funcs->destroy( (PSH_Globals)size->internal );

      size->internal = 0;
    }
  }


  FT_LOCAL_DEF FT_Error
  CFF_Size_Init( CFF_Size  size )
  {
    FT_Error           error = 0;
    PSH_Globals_Funcs  funcs = CFF_Size_Get_Globals_Funcs( size );


    if ( funcs )
    {
      PSH_Globals   globals;
      CFF_Face      face    = (CFF_Face)size->face;
      CFF_Font*     font    = face->extra.data;
      CFF_SubFont*  subfont = &font->top_font;

      CFF_Private*  cpriv   = &subfont->private_dict;
      T1_Private    priv;


      /* IMPORTANT: The CFF and Type1 private dictionaries have    */
      /*            slightly different structures; we need to      */
      /*            synthetize a type1 dictionary on the fly here. */

      {
        FT_UInt  n, count;


        MEM_Set( &priv, 0, sizeof ( priv ) );

        count = priv.num_blue_values = cpriv->num_blue_values;
        for ( n = 0; n < count; n++ )
          priv.blue_values[n] = (FT_Short) cpriv->blue_values[n];

        count = priv.num_other_blues = cpriv->num_other_blues;
        for ( n = 0; n < count; n++ )
          priv.other_blues[n] = (FT_Short) cpriv->other_blues[n];

        count = priv.num_family_blues = cpriv->num_family_blues;
        for ( n = 0; n < count; n++ )
          priv.family_blues[n] = (FT_Short) cpriv->family_blues[n];

        count = priv.num_family_other_blues = cpriv->num_family_other_blues;
        for ( n = 0; n < count; n++ )
          priv.family_other_blues[n] = (FT_Short) cpriv->family_other_blues[n];

        priv.blue_scale = cpriv->blue_scale;
        priv.blue_shift = cpriv->blue_shift;
        priv.blue_fuzz  = cpriv->blue_fuzz;

        priv.standard_width[0]  = (FT_UShort) cpriv->standard_width;
        priv.standard_height[0] = (FT_UShort) cpriv->standard_height;

        count = priv.num_snap_widths = cpriv->num_snap_widths;
        for ( n = 0; n < count; n++ )
          priv.snap_widths[n] = (FT_Short) cpriv->snap_widths[n];

        count = priv.num_snap_heights = cpriv->num_snap_heights;
        for ( n = 0; n < count; n++ )
          priv.snap_heights[n] = (FT_Short) cpriv->snap_heights[n];

        priv.force_bold     = cpriv->force_bold;
        priv.language_group = cpriv->language_group;
        priv.lenIV          = cpriv->lenIV;
      }

      error = funcs->create( size->face->memory, &priv, &globals );
      if ( !error )
        size->internal = (FT_Size_Internal)(void*)globals;
    }

    return error;
  }


  FT_LOCAL_DEF FT_Error
  CFF_Size_Reset( CFF_Size  size )
  {
    PSH_Globals_Funcs  funcs = CFF_Size_Get_Globals_Funcs( size );
    FT_Error           error = 0;


    if ( funcs )
      error = funcs->set_scale( (PSH_Globals)size->internal,
                                 size->metrics.x_scale,
                                 size->metrics.y_scale,
                                 0, 0 );
    return error;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                            SLOT  FUNCTIONS                            */
  /*                                                                       */
  /*************************************************************************/

  FT_LOCAL_DEF void
  CFF_GlyphSlot_Done( CFF_GlyphSlot  slot )
  {
    slot->root.internal->glyph_hints = 0;
  }


  FT_LOCAL_DEF FT_Error
  CFF_GlyphSlot_Init( CFF_GlyphSlot  slot )
  {
    CFF_Face             face     = (CFF_Face)slot->root.face;
    CFF_Font*            font     = face->extra.data;
    PSHinter_Interface*  pshinter = font->pshinter;


    if ( pshinter )
    {
      FT_Module  module;


      module = FT_Get_Module( slot->root.face->driver->root.library,
                              "pshinter" );
      if ( module )
      {
        T2_Hints_Funcs  funcs;


        funcs = pshinter->get_t2_funcs( module );
        slot->root.internal->glyph_hints = (void*)funcs;
      }
    }

    return 0;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                           FACE  FUNCTIONS                             */
  /*                                                                       */
  /*************************************************************************/

  static FT_String*
  CFF_StrCopy( FT_Memory         memory,
               const FT_String*  source )
  {
    FT_Error    error;
    FT_String*  result = 0;
    FT_Int      len = (FT_Int)strlen( source );


    if ( !ALLOC( result, len + 1 ) )
    {
      MEM_Copy( result, source, len );
      result[len] = 0;
    }
    return result;
  }


#if 0

  /* this function is used to build a Unicode charmap from the glyph names */
  /* in a file                                                             */
  static FT_Error
  CFF_Build_Unicode_Charmap( CFF_Face            face,
                             FT_ULong            base_offset,
                             PSNames_Interface*  psnames )
  {
    CFF_Font*       font = (CFF_Font*)face->extra.data;
    FT_Memory       memory = FT_FACE_MEMORY(face);
    FT_UInt         n, num_glyphs = face->root.num_glyphs;
    const char**    glyph_names;
    FT_Error        error;
    CFF_Font_Dict*  dict = &font->top_font.font_dict;
    FT_ULong        charset_offset;
    FT_Byte         format;
    FT_Stream       stream = face->root.stream;


    charset_offset = dict->charset_offset;
    if ( !charset_offset )
    {
      FT_ERROR(( "CFF_Build_Unicode_Charmap: charset table is missing\n" ));
      error = CFF_Err_Invalid_File_Format;
      goto Exit;
    }

    /* allocate the charmap */
    if ( ALLOC( face->charmap, ...

    /* seek to charset table and allocate glyph names table */
    if ( FILE_Seek( base_offset + charset_offset )           ||
         ALLOC_ARRAY( glyph_names, num_glyphs, const char* ) )
      goto Exit;

    /* now, read each glyph name and store it in the glyph name table */
    if ( READ_Byte( format ) )
      goto Fail;

    switch ( format )
    {
    case 0:  /* format 0 - one SID per glyph */
      {
        const char**  gname = glyph_names;
        const char**  limit = gname + num_glyphs;


        if ( ACCESS_Frame( num_glyphs * 2 ) )
          goto Fail;

        for ( ; gname < limit; gname++ )
          gname[0] = CFF_Get_String( &font->string_index,
                                     GET_UShort(),
                                     psnames );
        FORGET_Frame();
        break;
      }

    case 1:  /* format 1 - sequential ranges                    */
    case 2:  /* format 2 - sequential ranges with 16-bit counts */
      {
        const char**  gname = glyph_names;
        const char**  limit = gname + num_glyphs;
        FT_UInt       len = 3;


        if ( format == 2 )
          len++;

        while ( gname < limit )
        {
          FT_UInt  first;
          FT_UInt  count;


          if ( ACCESS_Frame( len ) )
            goto Fail;

          first = GET_UShort();
          if ( format == 3 )
            count = GET_UShort();
          else
            count = GET_Byte();

          FORGET_Frame();

          for ( ; count > 0; count-- )
          {
            gname[0] = CFF_Get_String( &font->string_index,
                                       first,
                                       psnames );
            gname++;
            first++;
          }
        }
        break;
      }

    default:   /* unknown charset format! */
      FT_ERROR(( "CFF_Build_Unicode_Charmap: unknown charset format!\n" ));
      error = CFF_Err_Invalid_File_Format;
      goto Fail;
    }

    /* all right, the glyph names were loaded; we now need to create */
    /* the corresponding unicode charmap                             */

  Fail:
    for ( n = 0; n < num_glyphs; n++ )
      FREE( glyph_names[n] );

    FREE( glyph_names );

  Exit:
    return error;
  }

#endif /* 0 */


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


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Face_Init                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given OpenType face object.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The source font stream.                              */
  /*                                                                       */
  /*    face_index :: The index of the font face in the resource.          */
  /*                                                                       */
  /*    num_params :: Number of additional generic parameters.  Ignored.   */
  /*                                                                       */
  /*    params     :: Additional generic parameters.  Ignored.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The newly built face object.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  CFF_Face_Init( FT_Stream      stream,
                 CFF_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params )
  {
    FT_Error             error;
    SFNT_Interface*      sfnt;
    PSNames_Interface*   psnames;
    PSHinter_Interface*  pshinter;
    FT_Bool              pure_cff    = 1;
    FT_Bool              sfnt_format = 0;


    sfnt = (SFNT_Interface*)FT_Get_Module_Interface(
             face->root.driver->root.library, "sfnt" );
    if ( !sfnt )
      goto Bad_Format;

    psnames = (PSNames_Interface*)FT_Get_Module_Interface(
                face->root.driver->root.library, "psnames" );

    pshinter = (PSHinter_Interface*)FT_Get_Module_Interface(
                 face->root.driver->root.library, "pshinter" );

    /* create input stream from resource */
    if ( FILE_Seek( 0 ) )
      goto Exit;

    /* check that we have a valid OpenType file */
    error = sfnt->init_face( stream, face, face_index, num_params, params );
    if ( !error )
    {
      if ( face->format_tag != 0x4F54544FL )  /* `OTTO'; OpenType/CFF font */
      {
        FT_TRACE2(( "[not a valid OpenType/CFF font]\n" ));
        goto Bad_Format;
      }

      /* if we are performing a simple font format check, exit immediately */
      if ( face_index < 0 )
        return CFF_Err_Ok;

      sfnt_format = 1;

      /* now, the font can be either an OpenType/CFF font, or an SVG CEF */
      /* font in the later case; it doesn't have a `head' table          */
      error = face->goto_table( face, TTAG_head, stream, 0 );
      if ( !error )
      {
        pure_cff = 0;

        /* load font directory */
        error = sfnt->load_face( stream, face,
                                 face_index, num_params, params );
        if ( error )
          goto Exit;
      }
      else
      {
        /* load the `cmap' table by hand */
        error = sfnt->load_charmaps( face, stream );
        if ( error )
          goto Exit;

        /* XXX: we don't load the GPOS table, as OpenType Layout     */
        /* support will be added later to a layout library on top of */
        /* FreeType 2                                                */
      }

      /* now, load the CFF part of the file */
      error = face->goto_table( face, TTAG_CFF, stream, 0 );
      if ( error )
        goto Exit;
    }
    else
    {
      /* rewind to start of file; we are going to load a pure-CFF font */
      if ( FILE_Seek( 0 ) )
        goto Exit;
      error = CFF_Err_Ok;
    }

    /* now load and parse the CFF table in the file */
    {
      CFF_Font*  cff;
      FT_Memory  memory = face->root.memory;
      FT_Face    root;
      FT_UInt    flags;


      if ( ALLOC( cff, sizeof ( *cff ) ) )
        goto Exit;

      face->extra.data = cff;
      error = CFF_Load_Font( stream, face_index, cff );
      if ( error )
        goto Exit;

      cff->pshinter = pshinter;

      /* Complement the root flags with some interesting information. */
      /* Note that this is only necessary for pure CFF and CEF fonts. */

      root             = &face->root;
      root->num_glyphs = cff->num_glyphs;

      if ( pure_cff )
      {
        CFF_Font_Dict*  dict = &cff->top_font.font_dict;


        /* we need the `PSNames' module for pure-CFF and CEF formats */
        if ( !psnames )
        {
          FT_ERROR(( "CFF_Face_Init:" ));
          FT_ERROR(( " cannot open CFF & CEF fonts\n" ));
          FT_ERROR(( "             " ));
          FT_ERROR(( " without the `PSNames' module\n" ));
          goto Bad_Format;
        }

        /* Set up num_faces. */
        root->num_faces = cff->num_faces;

        /* compute number of glyphs */
        if ( dict->cid_registry )
          root->num_glyphs = dict->cid_count;
        else
          root->num_glyphs = cff->charstrings_index.count;

        /* set global bbox, as well as EM size */
        root->bbox      = dict->font_bbox;
        root->ascender  = (FT_Short)( root->bbox.yMax >> 16 );
        root->descender = (FT_Short)( root->bbox.yMin >> 16 );
        root->height    = (FT_Short)(
          ( ( root->ascender - root->descender ) * 12 ) / 10 );

        if ( dict->units_per_em )
          root->units_per_EM = dict->units_per_em;
        else
          root->units_per_EM = 1000;

        /* retrieve font family & style name */
        root->family_name  = CFF_Get_Name( &cff->name_index, face_index );
        if ( dict->cid_registry )
          root->style_name = CFF_StrCopy( memory, "Regular" );  /* XXXX */
        else
          root->style_name = CFF_Get_String( &cff->string_index,
                                             dict->weight,
                                             psnames );

        /*******************************************************************/
        /*                                                                 */
        /* Compute face flags.                                             */
        /*                                                                 */
        flags = FT_FACE_FLAG_SCALABLE  |    /* scalable outlines */
                FT_FACE_FLAG_HORIZONTAL;    /* horizontal data   */

        if ( sfnt_format )
          flags |= FT_FACE_FLAG_SFNT;

        /* fixed width font? */
        if ( dict->is_fixed_pitch )
          flags |= FT_FACE_FLAG_FIXED_WIDTH;

  /* XXX: WE DO NOT SUPPORT KERNING METRICS IN THE GPOS TABLE FOR NOW */
#if 0
        /* kerning available? */
        if ( face->kern_pairs )
          flags |= FT_FACE_FLAG_KERNING;
#endif

#ifndef FT_CONFIG_OPTION_NO_GLYPH_NAMES
        flags |= FT_FACE_FLAG_GLYPH_NAMES;
#endif

        root->face_flags = flags;

        /*******************************************************************/
        /*                                                                 */
        /* Compute style flags.                                            */
        /*                                                                 */
        flags = 0;

        if ( dict->italic_angle )
          flags |= FT_STYLE_FLAG_ITALIC;

        /* XXX: may not be correct */
        if ( cff->top_font.private_dict.force_bold )
          flags |= FT_STYLE_FLAG_BOLD;

        root->style_flags = flags;

        /* set the charmaps if any */
        if ( sfnt_format )
        {
          /*****************************************************************/
          /*                                                               */
          /* Polish the charmaps.                                          */
          /*                                                               */
          /*   Try to set the charmap encoding according to the platform & */
          /*   encoding ID of each charmap.                                */
          /*                                                               */
          TT_CharMap  charmap;
          FT_Int      n;


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
        }
      }
    }

  Exit:
    return error;

  Bad_Format:
    error = CFF_Err_Unknown_File_Format;
    goto Exit;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Face_Done                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given face object.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A pointer to the face object to destroy.                   */
  /*                                                                       */
  FT_LOCAL_DEF void
  CFF_Face_Done( CFF_Face  face )
  {
    FT_Memory        memory = face->root.memory;
    SFNT_Interface*  sfnt   = (SFNT_Interface*)face->sfnt;


    if ( sfnt )
      sfnt->done_face( face );

    {
      CFF_Font*  cff = (CFF_Font*)face->extra.data;


      if ( cff )
      {
        CFF_Done_Font( cff );
        FREE( face->extra.data );
      }
    }
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Driver_Init                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a given OpenType driver object.                        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target driver object.                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_LOCAL_DEF FT_Error
  CFF_Driver_Init( CFF_Driver  driver )
  {
    /* init extension registry if needed */

#ifdef TT_CONFIG_OPTION_EXTEND_ENGINE

    return TT_Init_Extensions( driver );

#else

    FT_UNUSED( driver );

    return CFF_Err_Ok;

#endif
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    CFF_Driver_Done                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Finalizes a given OpenType driver.                                 */
  /*                                                                       */
  /* <Input>                                                               */
  /*    driver :: A handle to the target OpenType driver.                  */
  /*                                                                       */
  FT_LOCAL_DEF void
  CFF_Driver_Done( CFF_Driver  driver )
  {
    /* destroy extensions registry if needed */

#ifdef TT_CONFIG_OPTION_EXTEND_ENGINE

    TT_Done_Extensions( driver );

#else

    FT_UNUSED( driver );

#endif
  }


/* END */
