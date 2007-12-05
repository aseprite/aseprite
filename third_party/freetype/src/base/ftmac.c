/***************************************************************************/
/*                                                                         */
/*  ftmac.c                                                                */
/*                                                                         */
/*    Mac FOND support.  Written by just@letterror.com.                    */
/*                                                                         */
/*  Copyright 1996-2001 by                                                 */
/*  Just van Rossum, David Turner, Robert Wilhelm, and Werner Lemberg.     */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*
    Notes

    Mac suitcase files can (and often do!) contain multiple fonts. To
    support this I use the face_index argument of FT_(Open|New)_Face()
    functions, and pretend the suitcase file is a collection.
    Warning: although the FOND driver sets face->num_faces field to the
    number of available fonts, but the Type 1 driver sets it to 1 anyway.
    So this field is currently not reliable, and I don't see a clean way
    to  resolve that. The face_index argument translates to
      Get1IndResource( 'FOND', face_index + 1 );
    so clients should figure out the resource index of the FOND.
    (I'll try to provide some example code for this at some point.)


    The Mac FOND support works roughly like this:

    - Check whether the offered stream points to a Mac suitcase file.
      This is done by checking the file type: it has to be 'FFIL' or 'tfil'.
      The stream that gets passed to our init_face() routine is a stdio
      stream, which isn't usable for us, since the FOND resources live
      in the resource fork. So we just grab the stream->pathname field.

    - Read the FOND resource into memory, then check whether there is
      a TrueType font and/or (!) a Type 1 font available.

    - If there is a Type 1 font available (as a separate 'LWFN' file),
      read its data into memory, massage it slightly so it becomes
      PFB data, wrap it into a memory stream, load the Type 1 driver
      and delegate the rest of the work to it by calling FT_Open_Face().
      (XXX TODO: after this has been done, the kerning data from the FOND
      resource should be appended to the face: on the Mac there are usually
      no AFM files available. However, this is tricky since we need to map
      Mac char codes to ps glyph names to glyph ID's...)

    - If there is a TrueType font (an 'sfnt' resource), read it into
      memory, wrap it into a memory stream, load the TrueType driver
      and delegate the rest of the work to it, by calling FT_Open_Face().
  */


#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_STREAM_H
#include "truetype/ttobjs.h"
#include "type1/t1objs.h"

#include <Resources.h>
#include <Fonts.h>
#include <Errors.h>
#include <Files.h>
#include <TextUtils.h>

#include <ctype.h>  /* for isupper() and isalnum() */

#include FT_MAC_H


  /* Set PREFER_LWFN to 1 if LWFN (Type 1) is preferred over
     TrueType in case *both* are available (this is not common,
     but it *is* possible). */
#ifndef PREFER_LWFN
#define PREFER_LWFN 1
#endif


  /* Quick'n'dirty Pascal string to C string converter.
     Warning: this call is not thread safe! Use with caution. */
  static char*
  p2c_str( unsigned char*  pstr )
  {
    static char  cstr[256];


    strncpy( cstr, (char*)pstr + 1, pstr[0] );
    cstr[pstr[0]] = '\0';
    return cstr;
  }


  /* Given a pathname, fill in a file spec. */
  static int
  file_spec_from_path( const char*  pathname,
                       FSSpec*      spec )
  {
    Str255    p_path;
    FT_ULong  path_len;


    /* convert path to a pascal string */
    path_len = strlen( pathname );
    if ( path_len > 255 )
      return -1;
    p_path[0] = (unsigned char)path_len;
    strncpy( (char*)p_path + 1, pathname, path_len );

    if ( FSMakeFSSpec( 0, 0, p_path, spec ) != noErr )
      return -1;
    else
      return 0;
  }


  /* Return the file type of the file specified by spec. */
  static OSType
  get_file_type( FSSpec*  spec )
  {
    FInfo  finfo;


    if ( FSpGetFInfo( spec, &finfo ) != noErr )
      return 0;  /* file might not exist */

    return finfo.fdType;
  }


  /* is this a Mac OS X .dfont file */
  static Boolean
  is_dfont( FSSpec*  spec )
  {
    int nameLen = spec->name[0];


    if ( spec->name[nameLen - 5] == '.' &&
         spec->name[nameLen - 4] == 'd' &&
         spec->name[nameLen - 3] == 'f' &&
         spec->name[nameLen - 2] == 'o' &&
         spec->name[nameLen - 1] == 'n' &&
         spec->name[nameLen    ] == 't'  )
      return true;
    else
      return false;
  }


  /* Given a PostScript font name, create the Macintosh LWFN file name. */
  static void
  create_lwfn_name( char*   ps_name,
                    Str255  lwfn_file_name )
  {
    int       max = 5, count = 0;
    FT_Byte*  p = lwfn_file_name;
    FT_Byte*  q = (FT_Byte*)ps_name;


    lwfn_file_name[0] = 0;

    while ( *q )
    {
      if ( isupper( *q ) )
      {
        if ( count )
          max = 3;
        count = 0;
      }
      if ( count < max && ( isalnum( *q ) || *q == '_' ) )
      {
        *++p = *q;
        lwfn_file_name[0]++;
        count++;
      }
      q++;
    }
  }


  /* Given a file reference, answer its location as a vRefNum
     and a dirID. */
  static FT_Error
  get_file_location( short           ref_num,
                     short*          v_ref_num,
                     long*           dir_id,
                     unsigned char*  file_name )
  {
    FCBPBRec  pb;
    OSErr     error;

    pb.ioNamePtr = file_name;
    pb.ioVRefNum = 0;
    pb.ioRefNum  = ref_num;
    pb.ioFCBIndx = 0;


    error = PBGetFCBInfoSync( &pb );
    if ( error == noErr )
    {
      *v_ref_num = pb.ioFCBVRefNum;
      *dir_id    = pb.ioFCBParID;
    }
    return error;
  }


  /* Make a file spec for an LWFN file from a FOND resource and
     a file name. */
  static FT_Error
  make_lwfn_spec( Handle          fond,
                  unsigned char*  file_name,
                  FSSpec*         spec )
  {
    FT_Error  error;
    short     ref_num, v_ref_num;
    long      dir_id;
    Str255    fond_file_name;


    ref_num = HomeResFile( fond );

    error = ResError();
    if ( !error )
      error = get_file_location( ref_num, &v_ref_num,
                                 &dir_id, fond_file_name );
    if ( !error )
      error = FSMakeFSSpec( v_ref_num, dir_id, file_name, spec );

    return error;
  }


  /* Look inside the FOND data, answer whether there should be an SFNT
     resource, and answer the name of a possible LWFN Type 1 file.

     Thanks to Paul Miller (paulm@profoundeffects.com) for the fix
     to load a face OTHER than the first one in the FOND!
  */
  static void
  parse_fond( char*   fond_data,
              short*  have_sfnt,
              short*  sfnt_id,
              Str255  lwfn_file_name,
              short   face_index )
  {
    AsscEntry*  assoc;
    AsscEntry*  base_assoc;
    FamRec*     fond;


    *sfnt_id          = 0;
    *have_sfnt        = 0;
    lwfn_file_name[0] = 0;

    fond       = (FamRec*)fond_data;
    assoc      = (AsscEntry*)( fond_data + sizeof ( FamRec ) + 2 );
    base_assoc = assoc;
    assoc     += face_index;        /* add on the face_index! */

    /* if the face at this index is not scalable,
       fall back to the first one (old behavior) */
    if ( assoc->fontSize == 0 )
    {
      *have_sfnt = 1;
      *sfnt_id   = assoc->fontID;
    }
    else if ( base_assoc->fontSize == 0 )
    {
      *have_sfnt = 1;
      *sfnt_id   = base_assoc->fontID;
    }

    if ( fond->ffStylOff )
    {
      unsigned char*  p = (unsigned char*)fond_data;
      StyleTable*     style;
      unsigned short  string_count;
      unsigned char*  name_table = 0;
      char            ps_name[256];
      unsigned char*  names[64];
      int             i;


      p += fond->ffStylOff;
      style = (StyleTable*)p;
      p += sizeof ( StyleTable );
      string_count = *(unsigned short*)(p);
      p += sizeof ( short );

      for ( i = 0 ; i < string_count && i < 64; i++ )
      {
        names[i] = p;
        p += names[i][0];
        p++;
      }
      strcpy( ps_name, p2c_str( names[0] ) );  /* Family name */

      if ( style->indexes[0] > 1 )
      {
        unsigned char*  suffixes = names[style->indexes[0] - 1];


        for ( i = 1; i <= suffixes[0]; i++ )
          strcat( ps_name, p2c_str( names[suffixes[i] - 1] ) );
      }
      create_lwfn_name( ps_name, lwfn_file_name );
    }
  }


  /* Read Type 1 data from the POST resources inside the LWFN file,
     return a PFB buffer. This is somewhat convoluted because the FT2
     PFB parser wants the ASCII header as one chunk, and the LWFN
     chunks are often not organized that way, so we'll glue chunks
     of the same type together. */
  static FT_Error
  read_lwfn( FT_Memory  memory,
             FSSpec*    lwfn_spec,
             FT_Byte**  pfb_data,
             FT_ULong*  size )
  {
    FT_Error       error = FT_Err_Ok;
    short          res_ref, res_id;
    unsigned char  *buffer, *p, *size_p;
    FT_ULong       total_size = 0;
    FT_ULong       post_size, pfb_chunk_size;
    Handle         post_data;
    char           code, last_code;


    res_ref = FSpOpenResFile( lwfn_spec, fsRdPerm );
    if ( ResError() )
      return FT_Err_Out_Of_Memory;
    UseResFile( res_ref );

    /* First pass: load all POST resources, and determine the size of
       the output buffer. */
    res_id = 501;
    last_code = -1;

    for (;;)
    {
      post_data = Get1Resource( 'POST', res_id++ );
      if ( post_data == NULL )
        break;  /* we're done */

      code = (*post_data)[0];

      if ( code != last_code )
      {
        if ( code == 5 )
          total_size += 2; /* just the end code */
        else
          total_size += 6; /* code + 4 bytes chunk length */
      }

      total_size += GetHandleSize( post_data ) - 2;
      last_code = code;
    }

    if ( ALLOC( buffer, (FT_Long)total_size ) )
      goto Error;

    /* Second pass: append all POST data to the buffer, add PFB fields.
       Glue all consecutive chunks of the same type together. */
    p = buffer;
    res_id = 501;
    last_code = -1;
    pfb_chunk_size = 0;

    for (;;)
    {
      post_data = Get1Resource( 'POST', res_id++ );
      if ( post_data == NULL )
        break;  /* we're done */

      post_size = (FT_ULong)GetHandleSize( post_data ) - 2;
      code = (*post_data)[0];

      if ( code != last_code )
      {
        if ( last_code != -1 )
        {
          /* we're done adding a chunk, fill in the size field */
          *size_p++ = (FT_Byte)(   pfb_chunk_size         & 0xFF );
          *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 8  ) & 0xFF );
          *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 16 ) & 0xFF );
          *size_p++ = (FT_Byte)( ( pfb_chunk_size >> 24 ) & 0xFF );
          pfb_chunk_size = 0;
        }

        *p++ = 0x80;
        if ( code == 5 )
          *p++ = 0x03;  /* the end */
        else if ( code == 2 )
          *p++ = 0x02;  /* binary segment */
        else
          *p++ = 0x01;  /* ASCII segment */

        if ( code != 5 )
        {
          size_p = p;   /* save for later */
          p += 4;       /* make space for size field */
        }
      }

      memcpy( p, *post_data + 2, post_size );
      pfb_chunk_size += post_size;
      p += post_size;
      last_code = code;
    }

    *pfb_data = buffer;
    *size = total_size;

  Error:
    CloseResFile( res_ref );
    return error;
  }


  /* Finalizer for a memory stream; gets called by FT_Done_Face().
     It frees the memory it uses. */
  static void
  memory_stream_close( FT_Stream  stream )
  {
    FT_Memory  memory = stream->memory;


    FREE( stream->base );

    stream->size  = 0;
    stream->base  = 0;
    stream->close = 0;
  }


  /* Create a new memory stream from a buffer and a size. */
  static FT_Error
  new_memory_stream( FT_Library       library,
                     FT_Byte*         base,
                     FT_ULong         size,
                     FT_Stream_Close  close,
                     FT_Stream*       astream )
  {
      FT_Error   error;
      FT_Memory  memory;
      FT_Stream  stream;


      if ( !library )
        return FT_Err_Invalid_Library_Handle;

      if ( !base )
        return FT_Err_Invalid_Argument;

      *astream = 0;
      memory = library->memory;
      if ( ALLOC( stream, sizeof ( *stream ) ) )
        goto Exit;

      FT_New_Memory_Stream( library,
                            base,
                            size,
                            stream );

      stream->close = close;

      *astream = stream;

    Exit:
      return error;
  }


  /* Create a new FT_Face given a buffer and a driver name. */
  static FT_Error
  open_face_from_buffer( FT_Library  library,
                         FT_Byte*    base,
                         FT_ULong    size,
                         FT_Long     face_index,
                         char*       driver_name,
                         FT_Face*    aface )
  {
    FT_Open_Args  args;
    FT_Error      error;
    FT_Stream     stream;
    FT_Memory     memory = library->memory;


    error = new_memory_stream( library,
                               base,
                               size,
                               memory_stream_close,
                               &stream );
    if ( error )
    {
      FREE( base );
      return error;
    }

    args.flags = ft_open_stream;
    args.stream = stream;
    if ( driver_name )
    {
      args.flags = args.flags | ft_open_driver;
      args.driver = FT_Get_Module( library, driver_name );
    }

    error = FT_Open_Face( library, &args, face_index, aface );
    if ( error == FT_Err_Ok )
      (*aface)->face_flags &= ~FT_FACE_FLAG_EXTERNAL_STREAM;
    else
    {
      FT_Done_Stream( stream );
      FREE( stream );
    }
    return error;
  }


  /* Create a new FT_Face from a file spec to an LWFN file. */
  static FT_Error
  FT_New_Face_From_LWFN( FT_Library  library,
                         FSSpec*     spec,
                         FT_Long     face_index,
                         FT_Face*    aface )
  {
    FT_Byte*   pfb_data;
    FT_ULong   pfb_size;
    FT_Error   error;
    FT_Memory  memory = library->memory;


    error = read_lwfn( library->memory, spec, &pfb_data, &pfb_size );
    if ( error )
      return error;

#if 0
    {
      FILE*  f;
      char*  path;


      path = p2c_str( spec->name );
      strcat( path, ".PFB" );
      f = fopen( path, "wb" );
      if ( f )
      {
        fwrite( pfb_data, 1, pfb_size, f );
        fclose( f );
      }
    }
#endif

    return open_face_from_buffer( library,
                                  pfb_data,
                                  pfb_size,
                                  face_index,
                                  "type1",
                                  aface );
  }


  /* Create a new FT_Face from an SFNT resource, specified by res ID. */
  static FT_Error
  FT_New_Face_From_SFNT( FT_Library  library,
                         short       sfnt_id,
                         FT_Long     face_index,
                         FT_Face*    aface )
  {
    Handle     sfnt = NULL;
    FT_Byte*   sfnt_data;
    size_t     sfnt_size;
    FT_Stream  stream = NULL;
    FT_Error   error = 0;
    FT_Memory  memory = library->memory;


    sfnt = GetResource( 'sfnt', sfnt_id );
    if ( ResError() )
      return FT_Err_Invalid_Handle;

    sfnt_size = (FT_ULong)GetHandleSize( sfnt );
    if ( ALLOC( sfnt_data, (FT_Long)sfnt_size ) )
    {
      ReleaseResource( sfnt );
      return error;
    }

    HLock( sfnt );
    memcpy( sfnt_data, *sfnt, sfnt_size );
    HUnlock( sfnt );
    ReleaseResource( sfnt );

    return open_face_from_buffer( library,
                                  sfnt_data,
                                  sfnt_size,
                                  face_index,
                                  "truetype",
                                  aface );
  }


  /* Create a new FT_Face from a file spec to a suitcase file. */
  static FT_Error
  FT_New_Face_From_Suitcase( FT_Library  library,
                             FSSpec*     spec,
                             FT_Long     face_index,
                             FT_Face*    aface )
  {
    FT_Error  error = FT_Err_Ok;
    short     res_ref, res_index;
    Handle    fond;


    res_ref = FSpOpenResFile( spec, fsRdPerm );
    if ( ResError() )
      return FT_Err_Cannot_Open_Resource;
    UseResFile( res_ref );

    /* face_index may be -1, in which case we
       just need to do a sanity check */
    if ( face_index < 0 )
      res_index = 1;
    else
    {
      res_index = (short)( face_index + 1 );
      face_index = 0;
    }
    fond = Get1IndResource( 'FOND', res_index );
    if ( ResError() )
    {
      error = FT_Err_Cannot_Open_Resource;
      goto Error;
    }

    error = FT_New_Face_From_FOND( library, fond, face_index, aface );

  Error:
    CloseResFile( res_ref );
    return error;
  }


  /* Create a new FT_Face from a file spec to a suitcase file. */
  static FT_Error
  FT_New_Face_From_dfont( FT_Library  library,
                          FSSpec*     spec,
                          FT_Long     face_index,
                          FT_Face*    aface )
  {
    FT_Error  error = FT_Err_Ok;
    short     res_ref, res_index;
    Handle    fond;
    FSRef     hostContainerRef;


    error = FSpMakeFSRef( spec, &hostContainerRef );
    if ( error == noErr )
      error = FSOpenResourceFile( &hostContainerRef,
                                  0, NULL, fsRdPerm, &res_ref );

    if ( error != noErr )
      return FT_Err_Cannot_Open_Resource;

    UseResFile( res_ref );

    /* face_index may be -1, in which case we
       just need to do a sanity check */
    if ( face_index < 0 )
      res_index = 1;
    else
    {
      res_index = (short)( face_index + 1 );
      face_index = 0;
    }
    fond = Get1IndResource( 'FOND', res_index );
    if ( ResError() )
    {
      error = FT_Err_Cannot_Open_Resource;
      goto Error;
    }

    error = FT_New_Face_From_FOND( library, fond, face_index, aface );

  Error:
    CloseResFile( res_ref );
    return error;
  }


  /* documentation in ftmac.h */

  FT_EXPORT_DEF( FT_Error )
  FT_New_Face_From_FOND( FT_Library  library,
                         Handle      fond,
                         FT_Long     face_index,
                         FT_Face    *aface )
  {
    short     sfnt_id, have_sfnt, have_lwfn = 0;
    Str255    lwfn_file_name;
    short     fond_id;
    OSType    fond_type;
    Str255    fond_name;
    FSSpec    lwfn_spec;
    FT_Error  error = FT_Err_Unknown_File_Format;


    GetResInfo( fond, &fond_id, &fond_type, fond_name );
    if ( ResError() != noErr || fond_type != 'FOND' )
      return FT_Err_Invalid_File_Format;

    HLock( fond );
    parse_fond( *fond, &have_sfnt, &sfnt_id, lwfn_file_name, face_index );
    HUnlock( fond );

    if ( lwfn_file_name[0] )
    {
      if ( make_lwfn_spec( fond, lwfn_file_name, &lwfn_spec ) == FT_Err_Ok )
        have_lwfn = 1;  /* yeah, we got one! */
      else
        have_lwfn = 0;  /* no LWFN file found */
    }

    if ( have_lwfn && ( !have_sfnt || PREFER_LWFN ) )
      return FT_New_Face_From_LWFN( library,
                                    &lwfn_spec,
                                    face_index,
                                    aface );
    else if ( have_sfnt )
      return FT_New_Face_From_SFNT( library,
                                    sfnt_id,
                                    face_index,
                                    aface );

    return FT_Err_Unknown_File_Format;
  }


  /* documentation in ftmac.h */

  FT_EXPORT_DEF( FT_Error )
  FT_GetFile_From_Mac_Name( char*     fontName,
                            FSSpec*   pathSpec,
                            FT_Long*  face_index )
  {
    OptionBits            options = kFMUseGlobalScopeOption;

    FMFontFamilyIterator  famIter;
    OSStatus              status = FMCreateFontFamilyIterator( NULL, NULL,
                                                               options,
                                                               &famIter );
    FMFont                the_font = NULL;
    FMFontFamily          family   = NULL;


    *face_index = 0;
    while ( status == 0 && !the_font )
    {
      status = FMGetNextFontFamily( &famIter, &family );
      if ( status == 0 )
      {
        int                           stat2;
        FMFontFamilyInstanceIterator  instIter;
        Str255                        famNameStr;
        char                          famName[256];


        /* get the family name */
        FMGetFontFamilyName( family, famNameStr );
        CopyPascalStringToC( famNameStr, famName );

        /* iterate through the styles */
        FMCreateFontFamilyInstanceIterator( family, &instIter );

        *face_index = 0;
        stat2 = 0;
        while ( stat2 == 0 && !the_font )
        {
          FMFontStyle  style;
          FMFontSize   size;
          FMFont       font;


          stat2 = FMGetNextFontFamilyInstance( &instIter, &font,
                                               &style, &size );
          if ( stat2 == 0 && size == 0 )
          {
            char fullName[256];


            /* build up a complete face name */
            strcpy( fullName, famName );
            if ( style & bold )
              strcat( fullName, " Bold" );
            if ( style & italic )
              strcat( fullName, " Italic" );

            /* compare with the name we are looking for */
            if ( strcmp( fullName, fontName ) == 0 )
            {
              /* found it! */
              the_font = font;
            }
            else
               ++(*face_index);
          }
        }

        FMDisposeFontFamilyInstanceIterator( &instIter );
      }
    }

    FMDisposeFontFamilyIterator( &famIter );

    if ( the_font )
    {
      FMGetFontContainer( the_font, pathSpec );
      return FT_Err_Ok;
    }
    else
      return FT_Err_Unknown_File_Format;
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Face                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This is the Mac-specific implementation of FT_New_Face.  In        */
  /*    addition to the standard FT_New_Face() functionality, it also      */
  /*    accepts pathnames to Mac suitcase files.  For further              */
  /*    documentation see the original FT_New_Face() in freetype.h.        */
  /*                                                                       */
  FT_EXPORT_DEF( FT_Error )
  FT_New_Face( FT_Library   library,
               const char*  pathname,
               FT_Long      face_index,
               FT_Face     *aface )
  {
    FT_Open_Args  args;
    FSSpec        spec;
    OSType        file_type;


    /* test for valid `library' and `aface' delayed to FT_Open_Face() */
    if ( !pathname )
      return FT_Err_Invalid_Argument;

    if ( file_spec_from_path( pathname, &spec ) )
      return FT_Err_Invalid_Argument;

    file_type = get_file_type( &spec );
    if ( file_type == 'FFIL' || file_type == 'tfil' )
      return FT_New_Face_From_Suitcase( library, &spec, face_index, aface );
    else if ( file_type == 'LWFN' )
      return FT_New_Face_From_LWFN( library, &spec, face_index, aface );
    else if ( is_dfont( &spec ) )
      return FT_New_Face_From_dfont( library, &spec, face_index, aface );
    else  /* let it fall through to normal loader (.ttf, .otf, etc.) */
    {
      args.flags    = ft_open_pathname;
      args.pathname = (char*)pathname;

      return FT_Open_Face( library, &args, face_index, aface );
    }
  }


/* END */
