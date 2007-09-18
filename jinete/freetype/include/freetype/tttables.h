/***************************************************************************/
/*                                                                         */
/*  tttables.h                                                             */
/*                                                                         */
/*    Basic SFNT/TrueType tables definitions and interface                 */
/*    (specification only).                                                */
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


#ifndef __TTTABLES_H__
#define __TTTABLES_H__


#include <ft2build.h>
#include FT_FREETYPE_H


FT_BEGIN_HEADER

  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    truetype_tables                                                    */
  /*                                                                       */
  /* <Title>                                                               */
  /*    TrueType Tables                                                    */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    TrueType-specific table types and functions.                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains the definition of TrueType-specific tables   */
  /*    as well as some routines used to access and process them.          */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_Header                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a TrueType font header table.  All       */
  /*    fields follow the TrueType specification.                          */
  /*                                                                       */
  typedef struct  TT_Header_
  {
    FT_Fixed   Table_Version;
    FT_Fixed   Font_Revision;

    FT_Long    CheckSum_Adjust;
    FT_Long    Magic_Number;

    FT_UShort  Flags;
    FT_UShort  Units_Per_EM;

    FT_Long    Created [2];
    FT_Long    Modified[2];

    FT_Short   xMin;
    FT_Short   yMin;
    FT_Short   xMax;
    FT_Short   yMax;

    FT_UShort  Mac_Style;
    FT_UShort  Lowest_Rec_PPEM;

    FT_Short   Font_Direction;
    FT_Short   Index_To_Loc_Format;
    FT_Short   Glyph_Data_Format;

  } TT_Header;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_HoriHeader                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a TrueType horizontal header, the `hhea' */
  /*    table, as well as the corresponding horizontal metrics table,      */
  /*    i.e., the `hmtx' table.                                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    Version                :: The table version.                       */
  /*                                                                       */
  /*    Ascender               :: The font's ascender, i.e., the distance  */
  /*                              from the baseline to the top-most of all */
  /*                              glyph points found in the font.          */
  /*                                                                       */
  /*                              This value is invalid in many fonts, as  */
  /*                              it is usually set by the font designer,  */
  /*                              and often reflects only a portion of the */
  /*                              glyphs found in the font (maybe ASCII).  */
  /*                                                                       */
  /*                              You should use the `sTypoAscender' field */
  /*                              of the OS/2 table instead if you want    */
  /*                              the correct one.                         */
  /*                                                                       */
  /*    Descender              :: The font's descender, i.e., the distance */
  /*                              from the baseline to the bottom-most of  */
  /*                              all glyph points found in the font.  It  */
  /*                              is negative.                             */
  /*                                                                       */
  /*                              This value is invalid in many fonts, as  */
  /*                              it is usually set by the font designer,  */
  /*                              and often reflects only a portion of the */
  /*                              glyphs found in the font (maybe ASCII).  */
  /*                                                                       */
  /*                              You should use the `sTypoDescender'      */
  /*                              field of the OS/2 table instead if you   */
  /*                              want the correct one.                    */
  /*                                                                       */
  /*    Line_Gap               :: The font's line gap, i.e., the distance  */
  /*                              to add to the ascender and descender to  */
  /*                              get the BTB, i.e., the                   */
  /*                              baseline-to-baseline distance for the    */
  /*                              font.                                    */
  /*                                                                       */
  /*    advance_Width_Max      :: This field is the maximum of all advance */
  /*                              widths found in the font.  It can be     */
  /*                              used to compute the maximum width of an  */
  /*                              arbitrary string of text.                */
  /*                                                                       */
  /*    min_Left_Side_Bearing  :: The minimum left side bearing of all     */
  /*                              glyphs within the font.                  */
  /*                                                                       */
  /*    min_Right_Side_Bearing :: The minimum right side bearing of all    */
  /*                              glyphs within the font.                  */
  /*                                                                       */
  /*    xMax_Extent            :: The maximum horizontal extent (i.e., the */
  /*                              `width' of a glyph's bounding box) for   */
  /*                              all glyphs in the font.                  */
  /*                                                                       */
  /*    caret_Slope_Rise       :: The rise coefficient of the cursor's     */
  /*                              slope of the cursor (slope=rise/run).    */
  /*                                                                       */
  /*    caret_Slope_Run        :: The run coefficient of the cursor's      */
  /*                              slope.                                   */
  /*                                                                       */
  /*    Reserved               :: 10 reserved bytes.                       */
  /*                                                                       */
  /*    metric_Data_Format     :: Always 0.                                */
  /*                                                                       */
  /*    number_Of_HMetrics     :: Number of HMetrics entries in the `hmtx' */
  /*                              table -- this value can be smaller than  */
  /*                              the total number of glyphs in the font.  */
  /*                                                                       */
  /*    long_metrics           :: A pointer into the `hmtx' table.         */
  /*                                                                       */
  /*    short_metrics          :: A pointer into the `hmtx' table.         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    IMPORTANT: The TT_HoriHeader and TT_VertHeader structures should   */
  /*               be identical except for the names of their fields which */
  /*               are different.                                          */
  /*                                                                       */
  /*               This ensures that a single function in the `ttload'     */
  /*               module is able to read both the horizontal and vertical */
  /*               headers.                                                */
  /*                                                                       */
  typedef struct  TT_HoriHeader_
  {
    FT_Fixed   Version;
    FT_Short   Ascender;
    FT_Short   Descender;
    FT_Short   Line_Gap;

    FT_UShort  advance_Width_Max;      /* advance width maximum */

    FT_Short   min_Left_Side_Bearing;  /* minimum left-sb       */
    FT_Short   min_Right_Side_Bearing; /* minimum right-sb      */
    FT_Short   xMax_Extent;            /* xmax extents          */
    FT_Short   caret_Slope_Rise;
    FT_Short   caret_Slope_Run;
    FT_Short   caret_Offset;

    FT_Short   Reserved[4];

    FT_Short   metric_Data_Format;
    FT_UShort  number_Of_HMetrics;

    /* The following fields are not defined by the TrueType specification */
    /* but they are used to connect the metrics header to the relevant    */
    /* `HMTX' table.                                                      */

    void*      long_metrics;
    void*      short_metrics;

  } TT_HoriHeader;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_VertHeader                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a TrueType vertical header, the `vhea'   */
  /*    table, as well as the corresponding vertical metrics table, i.e.,  */
  /*    the `vmtx' table.                                                  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    Version                 :: The table version.                      */
  /*                                                                       */
  /*    Ascender                :: The font's ascender, i.e., the distance */
  /*                               from the baseline to the top-most of    */
  /*                               all glyph points found in the font.     */
  /*                                                                       */
  /*                               This value is invalid in many fonts, as */
  /*                               it is usually set by the font designer, */
  /*                               and often reflects only a portion of    */
  /*                               the glyphs found in the font (maybe     */
  /*                               ASCII).                                 */
  /*                                                                       */
  /*                               You should use the `sTypoAscender'      */
  /*                               field of the OS/2 table instead if you  */
  /*                               want the correct one.                   */
  /*                                                                       */
  /*    Descender               :: The font's descender, i.e., the         */
  /*                               distance from the baseline to the       */
  /*                               bottom-most of all glyph points found   */
  /*                               in the font.  It is negative.           */
  /*                                                                       */
  /*                               This value is invalid in many fonts, as */
  /*                               it is usually set by the font designer, */
  /*                               and often reflects only a portion of    */
  /*                               the glyphs found in the font (maybe     */
  /*                               ASCII).                                 */
  /*                                                                       */
  /*                               You should use the `sTypoDescender'     */
  /*                               field of the OS/2 table instead if you  */
  /*                               want the correct one.                   */
  /*                                                                       */
  /*    Line_Gap                :: The font's line gap, i.e., the distance */
  /*                               to add to the ascender and descender to */
  /*                               get the BTB, i.e., the                  */
  /*                               baseline-to-baseline distance for the   */
  /*                               font.                                   */
  /*                                                                       */
  /*    advance_Height_Max      :: This field is the maximum of all        */
  /*                               advance heights found in the font.  It  */
  /*                               can be used to compute the maximum      */
  /*                               height of an arbitrary string of text.  */
  /*                                                                       */
  /*    min_Top_Side_Bearing    :: The minimum top side bearing of all     */
  /*                               glyphs within the font.                 */
  /*                                                                       */
  /*    min_Bottom_Side_Bearing :: The minimum bottom side bearing of all  */
  /*                               glyphs within the font.                 */
  /*                                                                       */
  /*    yMax_Extent             :: The maximum vertical extent (i.e., the  */
  /*                               `height' of a glyph's bounding box) for */
  /*                               all glyphs in the font.                 */
  /*                                                                       */
  /*    caret_Slope_Rise        :: The rise coefficient of the cursor's    */
  /*                               slope of the cursor (slope=rise/run).   */
  /*                                                                       */
  /*    caret_Slope_Run         :: The run coefficient of the cursor's     */
  /*                               slope.                                  */
  /*                                                                       */
  /*    caret_Offset            :: The cursor's offset for slanted fonts.  */
  /*                               This value is `reserved' in vmtx        */
  /*                               version 1.0.                            */
  /*                                                                       */
  /*    Reserved                :: 8 reserved bytes.                       */
  /*                                                                       */
  /*    metric_Data_Format      :: Always 0.                               */
  /*                                                                       */
  /*    number_Of_HMetrics      :: Number of VMetrics entries in the       */
  /*                               `vmtx' table -- this value can be       */
  /*                               smaller than the total number of glyphs */
  /*                               in the font.                            */
  /*                                                                       */
  /*    long_metrics           :: A pointer into the `vmtx' table.         */
  /*                                                                       */
  /*    short_metrics          :: A pointer into the `vmtx' table.         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    IMPORTANT: The TT_HoriHeader and TT_VertHeader structures should   */
  /*               be identical except for the names of their fields which */
  /*               are different.                                          */
  /*                                                                       */
  /*               This ensures that a single function in the `ttload'     */
  /*               module is able to read both the horizontal and vertical */
  /*               headers.                                                */
  /*                                                                       */
  typedef struct  TT_VertHeader_
  {
    FT_Fixed   Version;
    FT_Short   Ascender;
    FT_Short   Descender;
    FT_Short   Line_Gap;

    FT_UShort  advance_Height_Max;      /* advance height maximum */

    FT_Short   min_Top_Side_Bearing;    /* minimum left-sb or top-sb       */
    FT_Short   min_Bottom_Side_Bearing; /* minimum right-sb or bottom-sb   */
    FT_Short   yMax_Extent;             /* xmax or ymax extents            */
    FT_Short   caret_Slope_Rise;
    FT_Short   caret_Slope_Run;
    FT_Short   caret_Offset;

    FT_Short   Reserved[4];

    FT_Short   metric_Data_Format;
    FT_UShort  number_Of_VMetrics;

    /* The following fields are not defined by the TrueType specification */
    /* but they're used to connect the metrics header to the relevant     */
    /* `HMTX' or `VMTX' table.                                            */

    void*      long_metrics;
    void*      short_metrics;

  } TT_VertHeader;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_OS2                                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a TrueType OS/2 table. This is the long  */
  /*    table version.  All fields comply to the TrueType specification.   */
  /*                                                                       */
  /*    Note that we now support old Mac fonts which do not include an     */
  /*    OS/2 table.  In this case, the `version' field is always set to    */
  /*    0xFFFF.                                                            */
  /*                                                                       */
  typedef struct  TT_OS2_
  {
    FT_UShort  version;                /* 0x0001 - more or 0xFFFF */
    FT_Short   xAvgCharWidth;
    FT_UShort  usWeightClass;
    FT_UShort  usWidthClass;
    FT_Short   fsType;
    FT_Short   ySubscriptXSize;
    FT_Short   ySubscriptYSize;
    FT_Short   ySubscriptXOffset;
    FT_Short   ySubscriptYOffset;
    FT_Short   ySuperscriptXSize;
    FT_Short   ySuperscriptYSize;
    FT_Short   ySuperscriptXOffset;
    FT_Short   ySuperscriptYOffset;
    FT_Short   yStrikeoutSize;
    FT_Short   yStrikeoutPosition;
    FT_Short   sFamilyClass;

    FT_Byte    panose[10];

    FT_ULong   ulUnicodeRange1;        /* Bits 0-31   */
    FT_ULong   ulUnicodeRange2;        /* Bits 32-63  */
    FT_ULong   ulUnicodeRange3;        /* Bits 64-95  */
    FT_ULong   ulUnicodeRange4;        /* Bits 96-127 */

    FT_Char    achVendID[4];

    FT_UShort  fsSelection;
    FT_UShort  usFirstCharIndex;
    FT_UShort  usLastCharIndex;
    FT_Short   sTypoAscender;
    FT_Short   sTypoDescender;
    FT_Short   sTypoLineGap;
    FT_UShort  usWinAscent;
    FT_UShort  usWinDescent;

    /* only version 1 tables: */

    FT_ULong   ulCodePageRange1;       /* Bits 0-31   */
    FT_ULong   ulCodePageRange2;       /* Bits 32-63  */

    /* only version 2 tables: */

    FT_Short   sxHeight;
    FT_Short   sCapHeight;
    FT_UShort  usDefaultChar;
    FT_UShort  usBreakChar;
    FT_UShort  usMaxContext;

  } TT_OS2;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_Postscript                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a TrueType Postscript table.  All fields */
  /*    comply to the TrueType table.  This structure does not reference   */
  /*    the Postscript glyph names, which can be nevertheless accessed     */
  /*    with the `ttpost' module.                                          */
  /*                                                                       */
  typedef struct  TT_Postscript_
  {
    FT_Fixed  FormatType;
    FT_Fixed  italicAngle;
    FT_Short  underlinePosition;
    FT_Short  underlineThickness;
    FT_ULong  isFixedPitch;
    FT_ULong  minMemType42;
    FT_ULong  maxMemType42;
    FT_ULong  minMemType1;
    FT_ULong  maxMemType1;

    /* Glyph names follow in the file, but we don't   */
    /* load them by default.  See the ttpost.c file.  */

  } TT_Postscript;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_PCLT                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a TrueType PCLT table.  All fields       */
  /*    comply to the TrueType table.                                      */
  /*                                                                       */
  typedef struct  TT_PCLT_
  {
    FT_Fixed   Version;
    FT_ULong   FontNumber;
    FT_UShort  Pitch;
    FT_UShort  xHeight;
    FT_UShort  Style;
    FT_UShort  TypeFamily;
    FT_UShort  CapHeight;
    FT_UShort  SymbolSet;
    FT_Char    TypeFace[16];
    FT_Char    CharacterComplement[8];
    FT_Char    FileName[6];
    FT_Char    StrokeWeight;
    FT_Char    WidthType;
    FT_Byte    SerifStyle;
    FT_Byte    Reserved;

  } TT_PCLT;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_MaxProfile                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The maximum profile is a table containing many max values which    */
  /*    can be used to pre-allocate arrays.  This ensures that no memory   */
  /*    allocation occurs during a glyph load.                             */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    version               :: The version number.                       */
  /*                                                                       */
  /*    numGlyphs             :: The number of glyphs in this TrueType     */
  /*                             font.                                     */
  /*                                                                       */
  /*    maxPoints             :: The maximum number of points in a         */
  /*                             non-composite TrueType glyph.  See also   */
  /*                             the structure element                     */
  /*                             `maxCompositePoints'.                     */
  /*                                                                       */
  /*    maxContours           :: The maximum number of contours in a       */
  /*                             non-composite TrueType glyph.  See also   */
  /*                             the structure element                     */
  /*                             `maxCompositeContours'.                   */
  /*                                                                       */
  /*    maxCompositePoints    :: The maximum number of points in a         */
  /*                             composite TrueType glyph.  See also the   */
  /*                             structure element `maxPoints'.            */
  /*                                                                       */
  /*    maxCompositeContours  :: The maximum number of contours in a       */
  /*                             composite TrueType glyph.  See also the   */
  /*                             structure element `maxContours'.          */
  /*                                                                       */
  /*    maxZones              :: The maximum number of zones used for      */
  /*                             glyph hinting.                            */
  /*                                                                       */
  /*    maxTwilightPoints     :: The maximum number of points in the       */
  /*                             twilight zone used for glyph hinting.     */
  /*                                                                       */
  /*    maxStorage            :: The maximum number of elements in the     */
  /*                             storage area used for glyph hinting.      */
  /*                                                                       */
  /*    maxFunctionDefs       :: The maximum number of function            */
  /*                             definitions in the TrueType bytecode for  */
  /*                             this font.                                */
  /*                                                                       */
  /*    maxInstructionDefs    :: The maximum number of instruction         */
  /*                             definitions in the TrueType bytecode for  */
  /*                             this font.                                */
  /*                                                                       */
  /*    maxStackElements      :: The maximum number of stack elements used */
  /*                             during bytecode interpretation.           */
  /*                                                                       */
  /*    maxSizeOfInstructions :: The maximum number of TrueType opcodes    */
  /*                             used for glyph hinting.                   */
  /*                                                                       */
  /*    maxComponentElements  :: An obscure value related to composite     */
  /*                             glyphs definitions.                       */
  /*                                                                       */
  /*    maxComponentDepth     :: An obscure value related to composite     */
  /*                             glyphs definitions.  Probably the maximum */
  /*                             number of simple glyphs in a composite.   */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This structure is only used during font loading.                   */
  /*                                                                       */
  typedef struct  TT_MaxProfile_
  {
    FT_Fixed   version;
    FT_UShort  numGlyphs;
    FT_UShort  maxPoints;
    FT_UShort  maxContours;
    FT_UShort  maxCompositePoints;
    FT_UShort  maxCompositeContours;
    FT_UShort  maxZones;
    FT_UShort  maxTwilightPoints;
    FT_UShort  maxStorage;
    FT_UShort  maxFunctionDefs;
    FT_UShort  maxInstructionDefs;
    FT_UShort  maxStackElements;
    FT_UShort  maxSizeOfInstructions;
    FT_UShort  maxComponentElements;
    FT_UShort  maxComponentDepth;

  } TT_MaxProfile;


  /* */

  typedef enum
  {
    ft_sfnt_head = 0,
    ft_sfnt_maxp = 1,
    ft_sfnt_os2  = 2,
    ft_sfnt_hhea = 3,
    ft_sfnt_vhea = 4,
    ft_sfnt_post = 5,
    ft_sfnt_pclt = 6,

    sfnt_max   /* don't remove */

  } FT_Sfnt_Tag;


  /* internal use only */
  typedef void*
  (*FT_Get_Sfnt_Table_Func)( FT_Face      face,
                             FT_Sfnt_Tag  tag );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Sfnt_Table                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns a pointer to a given SFNT table within a face.             */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source.                                    */
  /*                                                                       */
  /*    tag  :: The index of the SFNT table.                               */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A type-less pointer to the table.  This will be 0 in case of       */
  /*    error, or if the corresponding table was not found *OR* loaded     */
  /*    from the file.                                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The table is owned by the face object and disappears with it.      */
  /*                                                                       */
  /*    This function is only useful to access SFNT tables that are loaded */
  /*    by the sfnt/truetype/opentype drivers.  See FT_Sfnt_Tag for a      */
  /*    list.                                                              */
  /*                                                                       */
  FT_EXPORT( void* )
  FT_Get_Sfnt_Table( FT_Face      face,
                     FT_Sfnt_Tag  tag );


  /* */


FT_END_HEADER

#endif /* __TTTABLES_H__ */


/* END */
