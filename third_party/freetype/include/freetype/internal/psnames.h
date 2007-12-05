/***************************************************************************/
/*                                                                         */
/*  psnames.h                                                              */
/*                                                                         */
/*    High-level interface for the `PSNames' module (in charge of          */
/*    various functions related to Postscript glyph names conversion).     */
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


#ifndef __PSNAMES_H__
#define __PSNAMES_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    PS_Unicode_Value_Func                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to return the Unicode index corresponding to a     */
  /*    given glyph name.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph_name :: The glyph name.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The Unicode character index resp. the non-Unicode value 0xFFFF if  */
  /*    the glyph name has no known Unicode meaning.                       */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function is able to map several different glyph names to the  */
  /*    same Unicode value, according to the rules defined in the Adobe    */
  /*    Glyph List table.                                                  */
  /*                                                                       */
  /*    This function will not be compiled if the configuration macro      */
  /*    FT_CONFIG_OPTION_ADOBE_GLYPH_LIST is undefined.                    */
  /*                                                                       */
  typedef FT_ULong
  (*PS_Unicode_Value_Func)( const char*  glyph_name );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    PS_Unicode_Index_Func                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to return the glyph index corresponding to a given */
  /*    Unicode value.                                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    num_glyphs  :: The number of glyphs in the face.                   */
  /*                                                                       */
  /*    glyph_names :: An array of glyph name pointers.                    */
  /*                                                                       */
  /*    unicode     :: The Unicode value.                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The glyph index resp. 0xFFFF if no glyph corresponds to this       */
  /*    Unicode value.                                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function is able to recognize several glyph names per Unicode */
  /*    value, according to the Adobe Glyph List.                          */
  /*                                                                       */
  /*    This function will not be compiled if the configuration macro      */
  /*    FT_CONFIG_OPTION_ADOBE_GLYPH_LIST is undefined.                    */
  /*                                                                       */
  typedef FT_UInt
  (*PS_Unicode_Index_Func)( FT_UInt       num_glyphs,
                            const char**  glyph_names,
                            FT_ULong      unicode );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    PS_Macintosh_Name_Func                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to return the glyph name corresponding to an Apple */
  /*    glyph name index.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    name_index :: The index of the Mac name.                           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The glyph name, or 0 if the index is invalid.                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will not be compiled if the configuration macro      */
  /*    FT_CONFIG_OPTION_POSTSCRIPT_NAMES is undefined.                    */
  /*                                                                       */
  typedef const char*
  (*PS_Macintosh_Name_Func)( FT_UInt  name_index );


  typedef const char*
  (*PS_Adobe_Std_Strings_Func)( FT_UInt  string_index );


  typedef struct  PS_UniMap_
  {
    FT_UInt  unicode;
    FT_UInt  glyph_index;

  } PS_UniMap;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    PS_Unicodes                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple table used to map Unicode values to glyph indices.  It is */
  /*    built by the PS_Build_Unicodes table according to the glyphs       */
  /*    present in a font file.                                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_codes :: The number of glyphs in the font that match a given   */
  /*                 Unicode value.                                        */
  /*                                                                       */
  /*    unicodes  :: An array of unicode values, sorted in increasing      */
  /*                 order.                                                */
  /*                                                                       */
  /*    gindex    :: An array of glyph indices, corresponding to each      */
  /*                 Unicode value.                                        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Use the function PS_Lookup_Unicode() to retrieve the glyph index   */
  /*    corresponding to a given Unicode character code.                   */
  /*                                                                       */
  typedef struct  PS_Unicodes_
  {
    FT_UInt     num_maps;
    PS_UniMap*  maps;

  } PS_Unicodes;


  typedef FT_Error
  (*PS_Build_Unicodes_Func)( FT_Memory     memory,
                             FT_UInt       num_glyphs,
                             const char**  glyph_names,
                             PS_Unicodes*  unicodes );

  typedef FT_UInt
  (*PS_Lookup_Unicode_Func)( PS_Unicodes*  unicodes,
                             FT_UInt       unicode );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    PSNames_Interface                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This structure defines the PSNames interface.                      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    unicode_value         :: A function used to convert a glyph name   */
  /*                             into a Unicode character code.            */
  /*                                                                       */
  /*    build_unicodes        :: A function which builds up the Unicode    */
  /*                             mapping table.                            */
  /*                                                                       */
  /*    lookup_unicode        :: A function used to return the glyph index */
  /*                             corresponding to a given Unicode          */
  /*                             character.                                */
  /*                                                                       */
  /*    macintosh_name        :: A function used to return the standard    */
  /*                             Apple glyph Postscript name corresponding */
  /*                             to a given string index (used by the      */
  /*                             TrueType `post' table).                   */
  /*                                                                       */
  /*    adobe_std_strings     :: A function that returns a pointer to a    */
  /*                             Adobe Standard String for a given SID.    */
  /*                                                                       */
  /*    adobe_std_encoding    :: A table of 256 unsigned shorts that maps  */
  /*                             character codes in the Adobe Standard     */
  /*                             Encoding to SIDs.                         */
  /*                                                                       */
  /*    adobe_expert_encoding :: A table of 256 unsigned shorts that maps  */
  /*                             character codes in the Adobe Expert       */
  /*                             Encoding to SIDs.                         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    `unicode_value' and `unicode_index' will be set to 0 if the        */
  /*    configuration macro FT_CONFIG_OPTION_ADOBE_GLYPH_LIST is           */
  /*    undefined.                                                         */
  /*                                                                       */
  /*    `macintosh_name' will be set to 0 if the configuration macro       */
  /*    FT_CONFIG_OPTION_POSTSCRIPT_NAMES is undefined.                    */
  /*                                                                       */
  typedef struct  PSNames_Interface_
  {
    PS_Unicode_Value_Func      unicode_value;
    PS_Build_Unicodes_Func     build_unicodes;
    PS_Lookup_Unicode_Func     lookup_unicode;
    PS_Macintosh_Name_Func     macintosh_name;

    PS_Adobe_Std_Strings_Func  adobe_std_strings;
    const unsigned short*      adobe_std_encoding;
    const unsigned short*      adobe_expert_encoding;

  } PSNames_Interface;


FT_END_HEADER

#endif /* __PSNAMES_H__ */


/* END */
