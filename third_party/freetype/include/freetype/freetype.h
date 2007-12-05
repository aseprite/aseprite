/***************************************************************************/
/*                                                                         */
/*  freetype.h                                                             */
/*                                                                         */
/*    FreeType high-level API and common types (specification only).       */
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


#ifndef __FREETYPE_H__
#define __FREETYPE_H__


  /*************************************************************************/
  /*                                                                       */
  /* The `raster' component duplicates some of the declarations in         */
  /* freetype.h for stand-alone use if _FREETYPE_ isn't defined.           */
  /*                                                                       */


  /*************************************************************************/
  /*                                                                       */
  /* The FREETYPE_MAJOR and FREETYPE_MINOR macros are used to version the  */
  /* new FreeType design, which is able to host several kinds of font      */
  /* drivers.  It starts at 2.0.                                           */
  /*                                                                       */
#define FREETYPE_MAJOR 2
#define FREETYPE_MINOR 0
#define FREETYPE_PATCH 5


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_ERRORS_H
#include FT_TYPES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                        B A S I C   T Y P E S                          */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    base_interface                                                     */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Base Interface                                                     */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    The FreeType 2 base font interface.                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section describes the public high-level API of FreeType 2.    */
  /*                                                                       */
  /* <Order>                                                               */
  /*    FT_Library                                                         */
  /*    FT_Face                                                            */
  /*    FT_Size                                                            */
  /*    FT_GlyphSlot                                                       */
  /*    FT_CharMap                                                         */
  /*    FT_Encoding                                                        */
  /*                                                                       */
  /*    FT_FaceRec                                                         */
  /*                                                                       */
  /*    FT_FACE_FLAG_SCALABLE                                              */
  /*    FT_FACE_FLAG_FIXED_SIZES                                           */
  /*    FT_FACE_FLAG_FIXED_WIDTH                                           */
  /*    FT_FACE_FLAG_HORIZONTAL                                            */
  /*    FT_FACE_FLAG_VERTICAL                                              */
  /*    FT_FACE_FLAG_SFNT                                                  */
  /*    FT_FACE_FLAG_KERNING                                               */
  /*    FT_FACE_FLAG_MULTIPLE_MASTERS                                      */
  /*    FT_FACE_FLAG_GLYPH_NAMES                                           */
  /*    FT_FACE_FLAG_EXTERNAL_STREAM                                       */
  /*    FT_FACE_FLAG_FAST_GLYPHS                                           */
  /*                                                                       */
  /*    FT_STYLE_FLAG_BOLD                                                 */
  /*    FT_STYLE_FLAG_ITALIC                                               */
  /*                                                                       */
  /*    FT_SizeRec                                                         */
  /*    FT_Size_Metrics                                                    */
  /*                                                                       */
  /*    FT_GlyphSlotRec                                                    */
  /*    FT_Glyph_Metrics                                                   */
  /*    FT_SubGlyph                                                        */
  /*                                                                       */
  /*    FT_Bitmap_Size                                                     */
  /*                                                                       */
  /*    FT_Init_FreeType                                                   */
  /*    FT_Done_FreeType                                                   */
  /*                                                                       */
  /*    FT_New_Face                                                        */
  /*    FT_Done_Face                                                       */
  /*    FT_New_Memory_Face                                                 */
  /*    FT_Open_Face                                                       */
  /*    FT_Open_Args                                                       */
  /*    FT_Open_Flags                                                      */
  /*    FT_Parameter                                                       */
  /*    FT_Attach_File                                                     */
  /*    FT_Attach_Stream                                                   */
  /*                                                                       */
  /*    FT_Set_Char_Size                                                   */
  /*    FT_Set_Pixel_Sizes                                                 */
  /*    FT_Set_Transform                                                   */
  /*    FT_Load_Glyph                                                      */
  /*    FT_Get_Char_Index                                                  */
  /*    FT_Get_Name_Index                                                  */
  /*    FT_Load_Char                                                       */
  /*                                                                       */
  /*    FT_LOAD_DEFAULT                                                    */
  /*    FT_LOAD_RENDER                                                     */
  /*    FT_LOAD_MONOCHROME                                                 */
  /*    FT_LOAD_LINEAR_DESIGN                                              */
  /*    FT_LOAD_NO_SCALE                                                   */
  /*    FT_LOAD_NO_HINTING                                                 */
  /*    FT_LOAD_NO_BITMAP                                                  */
  /*    FT_LOAD_CROP_BITMAP                                                */
  /*                                                                       */
  /*    FT_LOAD_VERTICAL_LAYOUT                                            */
  /*    FT_LOAD_IGNORE_TRANSFORM                                           */
  /*    FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH                                */
  /*    FT_LOAD_FORCE_AUTOHINT                                             */
  /*    FT_LOAD_NO_RECURSE                                                 */
  /*    FT_LOAD_PEDANTIC                                                   */
  /*                                                                       */
  /*    FT_Render_Glyph                                                    */
  /*    FT_Render_Mode                                                     */
  /*    FT_Get_Kerning                                                     */
  /*    FT_Kerning_Mode                                                    */
  /*    FT_Get_Glyph_Name                                                  */
  /*    FT_Get_Postscript_Name                                             */
  /*                                                                       */
  /*    FT_CharMapRec                                                      */
  /*    FT_Select_Charmap                                                  */
  /*    FT_Set_Charmap                                                     */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Glyph_Metrics                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model the metrics of a single glyph.  Note     */
  /*    that values are expressed in 26.6 fractional pixel format or in    */
  /*    font units, depending on context.                                  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    width        :: The glyph's width.                                 */
  /*                                                                       */
  /*    height       :: The glyph's height.                                */
  /*                                                                       */
  /*    horiBearingX :: Horizontal left side bearing.                      */
  /*                                                                       */
  /*    horiBearingY :: Horizontal top side bearing.                       */
  /*                                                                       */
  /*    horiAdvance  :: Horizontal advance width.                          */
  /*                                                                       */
  /*    vertBearingX :: Vertical left side bearing.                        */
  /*                                                                       */
  /*    vertBearingY :: Vertical top side bearing.                         */
  /*                                                                       */
  /*    vertAdvance  :: Vertical advance height.                           */
  /*                                                                       */
  typedef struct  FT_Glyph_Metrics_
  {
    FT_Pos  width;         /* glyph width  */
    FT_Pos  height;        /* glyph height */

    FT_Pos  horiBearingX;  /* left side bearing in horizontal layouts */
    FT_Pos  horiBearingY;  /* top side bearing in horizontal layouts  */
    FT_Pos  horiAdvance;   /* advance width for horizontal layout     */

    FT_Pos  vertBearingX;  /* left side bearing in vertical layouts */
    FT_Pos  vertBearingY;  /* top side bearing in vertical layouts  */
    FT_Pos  vertAdvance;   /* advance height for vertical layout    */

  } FT_Glyph_Metrics;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Bitmap_Size                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An extremely simple structure used to model the size of a bitmap   */
  /*    strike (i.e., a bitmap instance of the font for a given            */
  /*    resolution) in a fixed-size font face.  This is used for the       */
  /*    `available_sizes' field of the FT_Face_Properties structure.       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    height :: The character height in pixels.                          */
  /*                                                                       */
  /*    width  :: The character width in pixels.                           */
  /*                                                                       */
  typedef struct  FT_Bitmap_Size_
  {
    FT_Short  height;
    FT_Short  width;

  } FT_Bitmap_Size;


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                     O B J E C T   C L A S S E S                       */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Library                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a FreeType library instance.  Each `library' is        */
  /*    completely independent from the others; it is the `root' of a set  */
  /*    of objects like fonts, faces, sizes, etc.                          */
  /*                                                                       */
  /*    It also embeds a system object (see FT_System), as well as a       */
  /*    scan-line converter object (see FT_Raster).                        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Library objects are created through FT_Init_FreeType().            */
  /*                                                                       */
  typedef struct FT_LibraryRec_  *FT_Library;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Module                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given FreeType module object.  Each module can be a  */
  /*    font driver, a renderer, or anything else that provides services   */
  /*    to the formers.                                                    */
  /*                                                                       */
  typedef struct FT_ModuleRec_*  FT_Module;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Driver                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given FreeType font driver object.  Each font driver */
  /*    is able to create faces, sizes, glyph slots, and charmaps from the */
  /*    resources whose format it supports.                                */
  /*                                                                       */
  /*    A driver can support either bitmap, graymap, or scalable font      */
  /*    formats.                                                           */
  /*                                                                       */
  typedef struct FT_DriverRec_*  FT_Driver;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Renderer                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given FreeType renderer.  A renderer is in charge of */
  /*    converting a glyph image to a bitmap, when necessary.  Each        */
  /*    supports a given glyph image format, and one or more target        */
  /*    surface depths.                                                    */
  /*                                                                       */
  typedef struct FT_RendererRec_*  FT_Renderer;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Face                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given driver face object.  A face object contains    */
  /*    all the instance and glyph independent data of a font file         */
  /*    typeface.                                                          */
  /*                                                                       */
  /*    A face object is created from a resource object through the        */
  /*    new_face() method of a given driver.                               */
  /*                                                                       */
  typedef struct FT_FaceRec_*  FT_Face;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Size                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given driver size object.  Such an object models the */
  /*    _resolution_ AND _size_ dependent state of a given driver face     */
  /*    size.                                                              */
  /*                                                                       */
  /*    A size object is always created from a given face object.  It is   */
  /*    discarded automatically by its parent face.                        */
  /*                                                                       */
  typedef struct FT_SizeRec_*  FT_Size;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_GlyphSlot                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given `glyph slot'.  A slot is a container where it  */
  /*    is possible to load any of the glyphs contained within its parent  */
  /*    face.                                                              */
  /*                                                                       */
  /*    A glyph slot is created from a given face object.  It is discarded */
  /*    automatically by its parent face.                                  */
  /*                                                                       */
  typedef struct FT_GlyphSlotRec_*  FT_GlyphSlot;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_CharMap                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A handle to a given character map.  A charmap is used to translate */
  /*    character codes in a given encoding into glyph indexes for its     */
  /*    parent's face.  Some font formats may provide several charmaps per */
  /*    font.                                                              */
  /*                                                                       */
  /*    A charmap is created from a given face object.  It is discarded    */
  /*    automatically by its parent face.                                  */
  /*                                                                       */
  typedef struct FT_CharMapRec_*  FT_CharMap;


  /*************************************************************************/
  /*                                                                       */
  /* <Macro>                                                               */
  /*    FT_ENC_TAG                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This macro converts four letter tags into an unsigned long.        */
  /*                                                                       */
#ifndef FT_ENC_TAG
#define FT_ENC_TAG( value, _x1, _x2, _x3, _x4 ) \
        value = ( ( (unsigned long)_x1 << 24 ) | \
                  ( (unsigned long)_x2 << 16 ) | \
                  ( (unsigned long)_x3 << 8  ) | \
                    (unsigned long)_x4         )
#endif /* FT_ENC_TAG */


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_Encoding                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration used to specify encodings supported by charmaps.    */
  /*    Used in the FT_Select_Charmap() API function.                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Because of 32-bit charcodes defined in Unicode (i.e., surrogates), */
  /*    all character codes must be expressed as FT_Longs.                 */
  /*                                                                       */
  /*    Other encodings might be defined in the future.                    */
  /*                                                                       */
  typedef enum  FT_Encoding_
  {
    FT_ENC_TAG( ft_encoding_none, 0, 0, 0, 0 ),

    FT_ENC_TAG( ft_encoding_symbol,  's', 'y', 'm', 'b' ),
    FT_ENC_TAG( ft_encoding_unicode, 'u', 'n', 'i', 'c' ),
    FT_ENC_TAG( ft_encoding_latin_2, 'l', 'a', 't', '2' ),
    FT_ENC_TAG( ft_encoding_sjis,    's', 'j', 'i', 's' ),
    FT_ENC_TAG( ft_encoding_gb2312,  'g', 'b', ' ', ' ' ),
    FT_ENC_TAG( ft_encoding_big5,    'b', 'i', 'g', '5' ),
    FT_ENC_TAG( ft_encoding_wansung, 'w', 'a', 'n', 's' ),
    FT_ENC_TAG( ft_encoding_johab,   'j', 'o', 'h', 'a' ),

    FT_ENC_TAG( ft_encoding_adobe_standard, 'A', 'D', 'O', 'B' ),
    FT_ENC_TAG( ft_encoding_adobe_expert,   'A', 'D', 'B', 'E' ),
    FT_ENC_TAG( ft_encoding_adobe_custom,   'A', 'D', 'B', 'C' ),

    FT_ENC_TAG( ft_encoding_apple_roman, 'a', 'r', 'm', 'n' )

  } FT_Encoding;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_CharMapRec                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The base charmap class.                                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face        :: A handle to the parent face object.                 */
  /*                                                                       */
  /*    encoding    :: A tag which identifies the charmap.  Use this with  */
  /*                   FT_Select_Charmap().                                */
  /*                                                                       */
  /*    platform_id :: An ID number describing the platform for the        */
  /*                   following encoding ID.  This comes directly from    */
  /*                   the TrueType specification and should be emulated   */
  /*                   for other formats.                                  */
  /*                                                                       */
  /*    encoding_id :: A platform specific encoding number.  This also     */
  /*                   comes from the TrueType specification and should be */
  /*                   emulated similarly.                                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    We STRONGLY recommmend emulating a Unicode charmap for drivers     */
  /*    that do not support TrueType or OpenType.                          */
  /*                                                                       */
  typedef struct  FT_CharMapRec_
  {
    FT_Face      face;
    FT_Encoding  encoding;
    FT_UShort    platform_id;
    FT_UShort    encoding_id;

  } FT_CharMapRec;


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                 B A S E   O B J E C T   C L A S S E S                 */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Face_Internal                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to an FT_Face_InternalRec structure, used to      */
  /*    model private data of a given FT_Face object.                      */
  /*                                                                       */
  /*    This field might change between releases of FreeType 2 and are     */
  /*    not generally available to client applications.                    */
  /*                                                                       */
  typedef struct FT_Face_InternalRec_*  FT_Face_Internal;


  /*************************************************************************/
  /*                                                                       */
  /*                       FreeType base face class                        */
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_FaceRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    FreeType root face class structure.  A face object models the      */
  /*    resolution and point-size independent data found in a font file.   */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    num_faces           :: In the case where the face is located in a  */
  /*                           collection (i.e., a resource which embeds   */
  /*                           several faces), this is the total number of */
  /*                           faces found in the resource.  1 by default. */
  /*                                                                       */
  /*    face_index          :: The index of the face in its resource.      */
  /*                           Usually, this is 0 for all normal font      */
  /*                           formats.  It can be more in the case of     */
  /*                           collections (which embed several fonts in a */
  /*                           single resource/file).                      */
  /*                                                                       */
  /*    face_flags          :: A set of bit flags that give important      */
  /*                           information about the face; see the         */
  /*                           FT_FACE_FLAG_XXX macros for details.        */
  /*                                                                       */
  /*    style_flags         :: A set of bit flags indicating the style of  */
  /*                           the face (i.e., italic, bold, underline,    */
  /*                           etc).                                       */
  /*                                                                       */
  /*    num_glyphs          :: The total number of glyphs in the face.     */
  /*                                                                       */
  /*    family_name         :: The face's family name.  This is an ASCII   */
  /*                           string, usually in English, which describes */
  /*                           the typeface's family (like `Times New      */
  /*                           Roman', `Bodoni', `Garamond', etc).  This   */
  /*                           is a least common denominator used to list  */
  /*                           fonts.  Some formats (TrueType & OpenType)  */
  /*                           provide localized and Unicode versions of   */
  /*                           this string.  Applications should use the   */
  /*                           format specific interface to access them.   */
  /*                                                                       */
  /*    style_name          :: The face's style name.  This is an ASCII    */
  /*                           string, usually in English, which describes */
  /*                           the typeface's style (like `Italic',        */
  /*                           `Bold', `Condensed', etc).  Not all font    */
  /*                           formats provide a style name, so this field */
  /*                           is optional, and can be set to NULL.  As    */
  /*                           for `family_name', some formats provide     */
  /*                           localized/Unicode versions of this string.  */
  /*                           Applications should use the format specific */
  /*                           interface to access them.                   */
  /*                                                                       */
  /*    num_fixed_sizes     :: The number of fixed sizes available in this */
  /*                           face.  This should be set to 0 for scalable */
  /*                           fonts, unless its resource includes a       */
  /*                           complete set of glyphs (called a `strike')  */
  /*                           for the specified size.                     */
  /*                                                                       */
  /*    available_sizes     :: An array of sizes specifying the available  */
  /*                           bitmap/graymap sizes that are contained in  */
  /*                           in the font resource.  Should be set to     */
  /*                           NULL if the field `num_fixed_sizes' is set  */
  /*                           to 0.                                       */
  /*                                                                       */
  /*    num_charmaps        :: The total number of character maps in the   */
  /*                           face.                                       */
  /*                                                                       */
  /*    charmaps            :: A table of pointers to the face's charmaps. */
  /*                           Used to scan the list of available charmaps */
  /*                           -- this table might change after a call to  */
  /*                           FT_Attach_File/Stream (e.g. when used to    */
  /*                           hook an additional encoding/CMap to the     */
  /*                           face object).                               */
  /*                                                                       */
  /*    generic             :: A field reserved for client uses.  See the  */
  /*                           FT_Generic type description.                */
  /*                                                                       */
  /*    bbox                :: The font bounding box.  Coordinates are     */
  /*                           expressed in font units (see units_per_EM). */
  /*                           The box is large enough to contain any      */
  /*                           glyph from the font.  Thus, bbox.yMax can   */
  /*                           be seen as the `maximal ascender',          */
  /*                           bbox.yMin as the `minimal descender', and   */
  /*                           the maximal glyph width is given by         */
  /*                           `bbox.xMax-bbox.xMin' (not to be confused   */
  /*                           with the maximal _advance_width_).  Only    */
  /*                           relevant for scalable formats.              */
  /*                                                                       */
  /*    units_per_EM        :: The number of font units per EM square for  */
  /*                           this face.  This is typically 2048 for      */
  /*                           TrueType fonts, 1000 for Type1 fonts, and   */
  /*                           should be set to the (unrealistic) value 1  */
  /*                           for fixed-sizes fonts.  Only relevant for   */
  /*                           scalable formats.                           */
  /*                                                                       */
  /*    ascender            :: The face's ascender is the vertical         */
  /*                           distance from the baseline to the topmost   */
  /*                           point of any glyph in the face.  This       */
  /*                           field's value is positive, expressed in     */
  /*                           font units.  Some font designs use a value  */
  /*                           different from `bbox.yMax'.  Only relevant  */
  /*                           for scalable formats.                       */
  /*                                                                       */
  /*    descender           :: The face's descender is the vertical        */
  /*                           distance from the baseline to the           */
  /*                           bottommost point of any glyph in the face.  */
  /*                           This field's value is positive, expressed   */
  /*                           in font units.  Some font designs use a     */
  /*                           value different from `-bbox.yMin'.  Only    */
  /*                           relevant for scalable formats.              */
  /*                                                                       */
  /*    height              :: The face's height is the vertical distance  */
  /*                           from one baseline to the next when writing  */
  /*                           several lines of text.  Its value is always */
  /*                           positive, expressed in font units.  The     */
  /*                           value can be computed as                    */
  /*                           `ascender+descender+line_gap' where the     */
  /*                           value of `line_gap' is also called          */
  /*                           `external leading'.  Only relevant for      */
  /*                           scalable formats.                           */
  /*                                                                       */
  /*    max_advance_width   :: The maximal advance width, in font units,   */
  /*                           for all glyphs in this face.  This can be   */
  /*                           used to make word wrapping computations     */
  /*                           faster.  Only relevant for scalable         */
  /*                           formats.                                    */
  /*                                                                       */
  /*    max_advance_height  :: The maximal advance height, in font units,  */
  /*                           for all glyphs in this face.  This is only  */
  /*                           relevant for vertical layouts, and should   */
  /*                           be set to the `height' for fonts that do    */
  /*                           not provide vertical metrics.  Only         */
  /*                           relevant for scalable formats.              */
  /*                                                                       */
  /*    underline_position  :: The position, in font units, of the         */
  /*                           underline line for this face.  It's the     */
  /*                           center of the underlining stem.  Only       */
  /*                           relevant for scalable formats.              */
  /*                                                                       */
  /*    underline_thickness :: The thickness, in font units, of the        */
  /*                           underline for this face.  Only relevant for */
  /*                           scalable formats.                           */
  /*                                                                       */
  /*    driver              :: A handle to the face's parent driver        */
  /*                           object.                                     */
  /*                                                                       */
  /*    memory              :: A handle to the face's parent memory        */
  /*                           object.  Used for the allocation of         */
  /*                           subsequent objects.                         */
  /*                                                                       */
  /*    stream              :: A handle to the face's stream.              */
  /*                                                                       */
  /*    glyph               :: The face's associated glyph slot(s).  This  */
  /*                           object is created automatically with a new  */
  /*                           face object.  However, certain kinds of     */
  /*                           applications (mainly tools like converters) */
  /*                           can need more than one slot to ease their   */
  /*                           task.                                       */
  /*                                                                       */
  /*    sizes_list          :: The list of child sizes for this face.      */
  /*                                                                       */
  /*    internal            :: A pointer to internal fields of the face    */
  /*                           object.  These fields can change freely     */
  /*                           between releases of FreeType and are not    */
  /*                           publicly available.                         */
  /*                                                                       */
  typedef struct  FT_FaceRec_
  {
    FT_Long           num_faces;
    FT_Long           face_index;

    FT_Long           face_flags;
    FT_Long           style_flags;

    FT_Long           num_glyphs;

    FT_String*        family_name;
    FT_String*        style_name;

    FT_Int            num_fixed_sizes;
    FT_Bitmap_Size*   available_sizes;

    FT_Int            num_charmaps;
    FT_CharMap*       charmaps;

    FT_Generic        generic;

    /*# the following are only relevant to scalable outlines */
    FT_BBox           bbox;

    FT_UShort         units_per_EM;
    FT_Short          ascender;
    FT_Short          descender;
    FT_Short          height;

    FT_Short          max_advance_width;
    FT_Short          max_advance_height;

    FT_Short          underline_position;
    FT_Short          underline_thickness;

    FT_GlyphSlot      glyph;
    FT_Size           size;
    FT_CharMap        charmap;

    /*@private begin */

    FT_Driver         driver;
    FT_Memory         memory;
    FT_Stream         stream;

    FT_ListRec        sizes_list;

    FT_Generic        autohint;
    void*             extensions;

    FT_Face_Internal  internal;

    /*@private end */

  } FT_FaceRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_SCALABLE                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face provides  */
  /*    vectorial outlines (i.e., TrueType or Type1).  This doesn't        */
  /*    prevent embedding of bitmap strikes though, i.e., a given face can */
  /*    have both this bit set, and a `num_fixed_sizes' property > 0.      */
  /*                                                                       */
#define FT_FACE_FLAG_SCALABLE  1


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_FIXED_SIZES                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face contains  */
  /*    `fixed sizes', i.e., bitmap strikes for some given pixel sizes.    */
  /*    See the `num_fixed_sizes' and `available_sizes' face properties    */
  /*    for more information.                                              */
  /*                                                                       */
#define FT_FACE_FLAG_FIXED_SIZES  2


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_FIXED_WIDTH                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face contains  */
  /*    fixed-width characters (like Courier, Lucida, MonoType, etc.).     */
  /*                                                                       */
#define FT_FACE_FLAG_FIXED_WIDTH  4


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_SFNT                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face uses the  */
  /*    `sfnt' storage fomat.  For now, this means TrueType or OpenType.   */
  /*                                                                       */
#define FT_FACE_FLAG_SFNT  8


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_HORIZONTAL                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face contains  */
  /*    horizontal glyph metrics.  This should be set for all common       */
  /*    formats, but who knows.                                            */
  /*                                                                       */
#define FT_FACE_FLAG_HORIZONTAL  0x10


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_VERTICAL                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face contains  */
  /*    vertical glyph metrics.  If not set, the glyph loader will         */
  /*    synthetize vertical metrics itself to help display vertical text   */
  /*    correctly.                                                         */
  /*                                                                       */
#define FT_FACE_FLAG_VERTICAL  0x20


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_KERNING                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face contains  */
  /*    kerning information.  When set, this information can be retrieved  */
  /*    through the function FT_Get_Kerning().  Note that when unset, this */
  /*    function will always return the kerning vector (0,0).              */
  /*                                                                       */
#define FT_FACE_FLAG_KERNING  0x40


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_FAST_GLYPHS                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that the glyphs in a given  */
  /*    font can be retrieved very quickly, and that a glyph cache is thus */
  /*    not necessary for any of its child size objects.                   */
  /*                                                                       */
  /*    This flag should really be set for fixed-size formats like FNT,    */
  /*    where each glyph bitmap is available directly in binary form       */
  /*    without any kind of compression.                                   */
  /*                                                                       */
#define FT_FACE_FLAG_FAST_GLYPHS  0x80


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_MULTIPLE_MASTERS                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that the font contains      */
  /*    multiple masters and is capable of interpolating between them.     */
  /*                                                                       */
#define FT_FACE_FLAG_MULTIPLE_MASTERS  0x100


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_GLYPH_NAMES                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that the font contains      */
  /*    glyph names that can be retrieved through FT_Get_Glyph_Name().     */
  /*                                                                       */
#define FT_FACE_FLAG_GLYPH_NAMES  0x200


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_FACE_FLAG_EXTERNAL_STREAM                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This bit field is used internally by FreeType to indicate that     */
  /*    a face's stream was provided by the client application and should  */
  /*    not be destroyed by FT_Done_Face().                                */
  /*                                                                       */
#define FT_FACE_FLAG_EXTERNAL_STREAM  0x4000


  /* */


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_HAS_HORIZONTAL (face)                                      */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains           */
  /*   horizontal metrics (this is true for all font formats though).      */
  /*                                                                       */
  /* @also:                                                                */
  /*   @FT_HAS_VERTICAL can be used to check for vertical metrics.         */
  /*                                                                       */
#define FT_HAS_HORIZONTAL( face ) \
          ( face->face_flags & FT_FACE_FLAG_HORIZONTAL )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_HAS_VERTICAL (face)                                        */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains vertical  */
  /*   metrics.                                                            */
  /*                                                                       */
#define FT_HAS_VERTICAL( face ) \
          ( face->face_flags & FT_FACE_FLAG_VERTICAL )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_HAS_KERNING (face)                                         */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains kerning   */
  /*   data that can be accessed with @FT_Get_Kerning.                     */
  /*                                                                       */
#define FT_HAS_KERNING( face ) \
          ( face->face_flags & FT_FACE_FLAG_KERNING )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_IS_SCALABLE (face)                                         */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains a         */
  /*   scalable font face (true for TrueType, Type 1, CID, and             */
  /*   OpenType/CFF font formats.                                          */
  /*                                                                       */
#define FT_IS_SCALABLE( face ) \
          ( face->face_flags & FT_FACE_FLAG_SCALABLE )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_IS_SFNT (face)                                             */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains a font    */
  /*   whose format is based on the SFNT storage scheme.  This usually     */
  /*   means: TrueType fonts, OpenType fonts, as well as SFNT-based        */
  /*   embedded bitmap fonts.                                              */
  /*                                                                       */
  /*   If this macro is true, all functions defined in @FT_SFNT_NAMES_H    */
  /*   and @FT_TRUETYPE_TABLES_H are available.                            */
  /*                                                                       */
#define FT_IS_SFNT( face ) \
          ( face->face_flags & FT_FACE_FLAG_SFNT )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_IS_FIXED_WIDTH (face)                                      */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains a font    */
  /*   face that contains fixed-width (or "monospace", "fixed-pitch",      */
  /*   etc.) glyphs.                                                       */
  /*                                                                       */
#define FT_IS_FIXED_WIDTH( face ) \
          ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_IS_FIXED_SIZES (face)                                      */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains some      */
  /*   embedded bitmaps.  See the `fixed_sizes' field of the @FT_FaceRec   */
  /*   structure.                                                          */
  /*                                                                       */
#define FT_HAS_FIXED_SIZES( face ) \
          ( face->face_flags & FT_FACE_FLAG_FIXED_SIZES )


   /* */


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_HAS_FAST_GLYPHS (face)                                     */
  /*                                                                       */
  /* @description:                                                         */
  /*   XXX                                                                 */
  /*                                                                       */
#define FT_HAS_FAST_GLYPHS( face ) \
          ( face->face_flags & FT_FACE_FLAG_FAST_GLYPHS )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_HAS_GLYPH_NAMES (face)                                     */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains some      */
  /*   glyph names that can be accessed through @FT_Get_Glyph_Names.       */
  /*                                                                       */
#define FT_HAS_GLYPH_NAMES( face ) \
          ( face->face_flags & FT_FACE_FLAG_GLYPH_NAMES )


  /*************************************************************************/
  /*                                                                       */
  /* @macro: FT_HAS_MULTIPLE_MASTERS (face)                                */
  /*                                                                       */
  /* @description:                                                         */
  /*   A macro that returns true whenever a face object contains some      */
  /*   multiple masters.  The functions provided by                        */
  /*   @FT_MULTIPLE_MASTERS_H are then available to choose the exact       */
  /*   design you want.                                                    */
  /*                                                                       */
#define FT_HAS_MULTIPLE_MASTERS( face ) \
          ( face->face_flags & FT_FACE_FLAG_MULTIPLE_MASTERS )


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_STYLE_FLAG_ITALIC                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face is        */
  /*    italicized.                                                        */
  /*                                                                       */
#define FT_STYLE_FLAG_ITALIC  1


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_STYLE_FLAG_BOLD                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used to indicate that a given face is        */
  /*    emboldened.                                                        */
  /*                                                                       */
#define FT_STYLE_FLAG_BOLD  2


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Size_Internal                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to an FT_Size_InternalRec structure, used to      */
  /*    model private data of a given FT_Size object.                      */
  /*                                                                       */
  typedef struct FT_Size_InternalRec_*  FT_Size_Internal;


  /*************************************************************************/
  /*                                                                       */
  /*                    FreeType base size metrics                         */
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Size_Metrics                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The size metrics structure returned scaled important distances for */
  /*    a given size object.                                               */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    x_ppem       :: The character width, expressed in integer pixels.  */
  /*                    This is the width of the EM square expressed in    */
  /*                    pixels, hence the term `ppem' (pixels per EM).     */
  /*                                                                       */
  /*    y_ppem       :: The character height, expressed in integer pixels. */
  /*                    This is the height of the EM square expressed in   */
  /*                    pixels, hence the term `ppem' (pixels per EM).     */
  /*                                                                       */
  /*    x_scale      :: A simple 16.16 fixed point format coefficient used */
  /*                    to scale horizontal distances expressed in font    */
  /*                    units to fractional (26.6) pixel coordinates.      */
  /*                                                                       */
  /*    y_scale      :: A simple 16.16 fixed point format coefficient used */
  /*                    to scale vertical distances expressed in font      */
  /*                    units to fractional (26.6) pixel coordinates.      */
  /*                                                                       */
  /*    ascender     :: The ascender, expressed in 26.6 fixed point        */
  /*                    pixels.  Always positive.                          */
  /*                                                                       */
  /*    descender    :: The descender, expressed in 26.6 fixed point       */
  /*                    pixels.  Always positive.                          */
  /*                                                                       */
  /*    height       :: The text height, expressed in 26.6 fixed point     */
  /*                    pixels.  Always positive.                          */
  /*                                                                       */
  /*    max_advance  :: Maximum horizontal advance, expressed in 26.6      */
  /*                    fixed point pixels.  Always positive.              */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The values of `ascender', `descender', and `height' are only the   */
  /*    scaled versions of `face->ascender', `face->descender', and        */
  /*    `face->height'.                                                    */
  /*                                                                       */
  /*    Unfortunately, due to glyph hinting, these values might not be     */
  /*    exact for certain fonts, they thus must be treated as unreliable   */
  /*    with an error margin of at least one pixel!                        */
  /*                                                                       */
  /*    Indeed, the only way to get the exact pixel ascender and descender */
  /*    is to render _all_ glyphs.  As this would be a definite            */
  /*    performance hit, it is up to client applications to perform such   */
  /*    computations.                                                      */
  /*                                                                       */
  typedef struct  FT_Size_Metrics_
  {
    FT_UShort  x_ppem;      /* horizontal pixels per EM               */
    FT_UShort  y_ppem;      /* vertical pixels per EM                 */

    FT_Fixed   x_scale;     /* two scales used to convert font units  */
    FT_Fixed   y_scale;     /* to 26.6 frac. pixel coordinates..      */

    FT_Pos     ascender;    /* ascender in 26.6 frac. pixels          */
    FT_Pos     descender;   /* descender in 26.6 frac. pixels         */
    FT_Pos     height;      /* text height in 26.6 frac. pixels       */
    FT_Pos     max_advance; /* max horizontal advance, in 26.6 pixels */

  } FT_Size_Metrics;


  /*************************************************************************/
  /*                                                                       */
  /*                       FreeType base size class                        */
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_SizeRec                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    FreeType root size class structure.  A size object models the      */
  /*    resolution and pointsize dependent data of a given face.           */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    face    :: Handle to the parent face object.                       */
  /*                                                                       */
  /*    generic :: A typeless pointer, which is unused by the FreeType     */
  /*               library or any of its drivers.  It can be used by       */
  /*               client applications to link their own data to each size */
  /*               object.                                                 */
  /*                                                                       */
  /*    metrics :: Metrics for this size object.  This field is read-only. */
  /*                                                                       */
  typedef struct  FT_SizeRec_
  {
    FT_Face           face;      /* parent face object              */
    FT_Generic        generic;   /* generic pointer for client uses */
    FT_Size_Metrics   metrics;   /* size metrics                    */
    FT_Size_Internal  internal;

  } FT_SizeRec;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_SubGlyph                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The subglyph structure is an internal object used to describe      */
  /*    subglyphs (for example, in the case of composites).                */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The subglyph implementation is not part of the high-level API,     */
  /*    hence the forward structure declaration.                           */
  /*                                                                       */
  typedef struct FT_SubGlyph_  FT_SubGlyph;


  /*************************************************************************/
  /*                                                                       */
  /* <Type>                                                                */
  /*    FT_Slot_Internal                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An opaque handle to an FT_Slot_InternalRec structure, used to      */
  /*    model private data of a given FT_GlyphSlot object.                 */
  /*                                                                       */
  typedef struct FT_Slot_InternalRec_*  FT_Slot_Internal;


  /*************************************************************************/
  /*                                                                       */
  /*                  FreeType Glyph Slot base class                       */
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_GlyphSlotRec                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    FreeType root glyph slot class structure.  A glyph slot is a       */
  /*    container where individual glyphs can be loaded, be they           */
  /*    vectorial or bitmap/graymaps.                                      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    library           :: A handle to the FreeType library instance     */
  /*                         this slot belongs to.                         */
  /*                                                                       */
  /*    face              :: A handle to the parent face object.           */
  /*                                                                       */
  /*    next              :: In some cases (like some font tools), several */
  /*                         glyph slots per face object can be a good     */
  /*                         thing.  As this is rare, the glyph slots are  */
  /*                         listed through a direct, single-linked list   */
  /*                         using its `next' field.                       */
  /*                                                                       */
  /*    generic           :: A typeless pointer which is unused by the     */
  /*                         FreeType library or any of its drivers.  It   */
  /*                         can be used by client applications to link    */
  /*                         their own data to each glyph slot object.     */
  /*                                                                       */
  /*    metrics           :: The metrics of the last loaded glyph in the   */
  /*                         slot.  The returned values depend on the last */
  /*                         load flags (see the FT_Load_Glyph() API       */
  /*                         function) and can be expressed either in 26.6 */
  /*                         fractional pixels or font units.              */
  /*                                                                       */
  /*                         Note that even when the glyph image is        */
  /*                         transformed, the metrics are not.             */
  /*                                                                       */
  /*    linearHoriAdvance :: For scalable formats only, this field holds   */
  /*                         the linearly scaled horizontal advance width  */
  /*                         for the glyph (i.e. the scaled and unhinted   */
  /*                         value of the hori advance).  This can be      */
  /*                         important to perform correct WYSIWYG layout.  */
  /*                                                                       */
  /*                         Note that this value is expressed by default  */
  /*                         in 16.16 pixels. However, when the glyph is   */
  /*                         loaded with the FT_LOAD_LINEAR_DESIGN flag,   */
  /*                         this field contains simply the value of the   */
  /*                         advance in original font units.               */
  /*                                                                       */
  /*    linearVertAdvance :: For scalable formats only, this field holds   */
  /*                         the linearly scaled vertical advance height   */
  /*                         for the glyph.  See linearHoriAdvance for     */
  /*                         comments.                                     */
  /*                                                                       */
  /*    advance           :: This is the transformed advance width for the */
  /*                         glyph.                                        */
  /*                                                                       */
  /*    format            :: This field indicates the format of the image  */
  /*                         contained in the glyph slot.  Typically       */
  /*                         ft_glyph_format_bitmap,                       */
  /*                         ft_glyph_format_outline, and                  */
  /*                         ft_glyph_format_composite, but others are     */
  /*                         possible.                                     */
  /*                                                                       */
  /*    bitmap            :: This field is used as a bitmap descriptor     */
  /*                         when the slot format is                       */
  /*                         ft_glyph_format_bitmap.  Note that the        */
  /*                         address and content of the bitmap buffer can  */
  /*                         change between calls of FT_Load_Glyph() and a */
  /*                         few other functions.                          */
  /*                                                                       */
  /*    bitmap_left       :: This is the bitmap's left bearing expressed   */
  /*                         in integer pixels.  Of course, this is only   */
  /*                         valid if the format is                        */
  /*                         ft_glyph_format_bitmap.                       */
  /*                                                                       */
  /*    bitmap_top        :: This is the bitmap's top bearing expressed in */
  /*                         integer pixels.  Remember that this is the    */
  /*                         distance from the baseline to the top-most    */
  /*                         glyph scanline, upwards y-coordinates being   */
  /*                         *positive*.                                   */
  /*                                                                       */
  /*    outline           :: The outline descriptor for the current glyph  */
  /*                         image if its format is                        */
  /*                         ft_glyph_bitmap_outline.                      */
  /*                                                                       */
  /*    num_subglyphs     :: The number of subglyphs in a composite glyph. */
  /*                         This format is only valid for the composite   */
  /*                         glyph format, that should normally only be    */
  /*                         loaded with the FT_LOAD_NO_RECURSE flag.      */
  /*                                                                       */
  /*    subglyphs         :: An array of subglyph descriptors for          */
  /*                         composite glyphs.  There are `num_subglyphs'  */
  /*                         elements in there.                            */
  /*                                                                       */
  /*    control_data      :: Certain font drivers can also return the      */
  /*                         control data for a given glyph image (e.g.    */
  /*                         TrueType bytecode, Type 1 charstrings, etc.). */
  /*                         This field is a pointer to such data.         */
  /*                                                                       */
  /*    control_len       :: This is the length in bytes of the control    */
  /*                         data.                                         */
  /*                                                                       */
  /*    other             :: Really wicked formats can use this pointer to */
  /*                         present their own glyph image to client apps. */
  /*                         Note that the app will need to know about the */
  /*                         image format.                                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If @FT_Load_Glyph() is called with default flags (see              */
  /*    @FT_LOAD_DEFAULT ) the glyph image is loaded in the glyph slot in  */
  /*    its native format (e.g. a vectorial outline for TrueType and       */
  /*    Type 1 formats).                                                   */
  /*                                                                       */
  /*    This image can later be converted into a bitmap by calling         */
  /*    FT_Render_Glyph().  This function finds the current renderer for   */
  /*    the native image's format then invokes it.                         */
  /*                                                                       */
  /*    The renderer is in charge of transforming the native image through */
  /*    the slot's face transformation fields, then convert it into a      */
  /*    bitmap that is returned in `slot->bitmap'.                         */
  /*                                                                       */
  /*    Note that `slot->bitmap_left' and `slot->bitmap_top' are also used */
  /*    to specify the position of the bitmap relative to the current pen  */
  /*    position (e.g. coordinates [0,0] on the baseline).  Of course,     */
  /*    `slot->format' is also changed to `ft_glyph_format_bitmap' .       */
  /*                                                                       */
  typedef struct  FT_GlyphSlotRec_
  {
    FT_Library        library;
    FT_Face           face;
    FT_GlyphSlot      next;
    FT_UInt           flags;
    FT_Generic        generic;

    FT_Glyph_Metrics  metrics;
    FT_Fixed          linearHoriAdvance;
    FT_Fixed          linearVertAdvance;
    FT_Vector         advance;

    FT_Glyph_Format   format;

    FT_Bitmap         bitmap;
    FT_Int            bitmap_left;
    FT_Int            bitmap_top;

    FT_Outline        outline;

    FT_UInt           num_subglyphs;
    FT_SubGlyph*      subglyphs;

    void*             control_data;
    long              control_len;

    void*             other;

    FT_Slot_Internal  internal;

  } FT_GlyphSlotRec;


  /*************************************************************************/
  /*************************************************************************/
  /*                                                                       */
  /*                         F U N C T I O N S                             */
  /*                                                                       */
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Init_FreeType                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Initializes a new FreeType library object.  The set of drivers     */
  /*    that are registered by this function is determined at build time.  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    alibrary :: A handle to a new library object.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Init_FreeType( FT_Library  *alibrary );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_FreeType                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a given FreeType library object and all of its childs,    */
  /*    including resources, drivers, faces, sizes, etc.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the target library object.                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Done_FreeType( FT_Library  library );


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_Open_Flags                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration used to list the bit flags used within the          */
  /*    `flags' field of the @FT_Open_Args structure.                      */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    ft_open_memory   :: This is a memory-based stream.                 */
  /*                                                                       */
  /*    ft_open_stream   :: Copy the stream from the `stream' field.       */
  /*                                                                       */
  /*    ft_open_pathname :: Create a new input stream from a C pathname.   */
  /*                                                                       */
  /*    ft_open_driver   :: Use the `driver' field.                        */
  /*                                                                       */
  /*    ft_open_params   :: Use the `num_params' & `params' field.         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The `ft_open_memory', `ft_open_stream', and `ft_open_pathname'     */
  /*    flags are mutually exclusive.                                      */
  /*                                                                       */
  typedef enum
  {
    ft_open_memory   = 1,
    ft_open_stream   = 2,
    ft_open_pathname = 4,
    ft_open_driver   = 8,
    ft_open_params   = 16

  } FT_Open_Flags;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Parameter                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A simple structure used to pass more or less generic parameters    */
  /*    to FT_Open_Face().                                                 */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    tag  :: A 4-byte identification tag.                               */
  /*                                                                       */
  /*    data :: A pointer to the parameter data.                           */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The id and function of parameters are driver-specific.             */
  /*                                                                       */
  typedef struct  FT_Parameter_
  {
    FT_ULong    tag;
    FT_Pointer  data;

  } FT_Parameter;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Open_Args                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to indicate how to open a new font file/stream.   */
  /*    A pointer to such a structure can be used as a parameter for the   */
  /*    functions @FT_Open_Face() & @FT_Attach_Stream().                   */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    flags       :: A set of bit flags indicating how to use the        */
  /*                   structure.                                          */
  /*                                                                       */
  /*    memory_base :: The first byte of the file in memory.               */
  /*                                                                       */
  /*    memory_size :: The size in bytes of the file in memory.            */
  /*                                                                       */
  /*    pathname    :: A pointer to an 8-bit file pathname.                */
  /*                                                                       */
  /*    stream      :: A handle to a source stream object.                 */
  /*                                                                       */
  /*    driver      :: This field is exclusively used by FT_Open_Face();   */
  /*                   it simply specifies the font driver to use to open  */
  /*                   the face.  If set to 0, FreeType will try to load   */
  /*                   the face with each one of the drivers in its list.  */
  /*                                                                       */
  /*    num_params  :: The number of extra parameters.                     */
  /*                                                                       */
  /*    params      :: Extra parameters passed to the font driver when     */
  /*                   opening a new face.                                 */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream type is determined by the contents of `flags' which     */
  /*    are tested in the following order by @FT_Open_Face:                */
  /*                                                                       */
  /*    If the `ft_open_memory' bit is set, assume that this is a          */
  /*    memory file of `memory_size' bytes,located at `memory_address'.    */
  /*                                                                       */
  /*    Otherwise, if the `ft_open_stream' bit is set, assume that a       */
  /*    custom input stream `stream' is used.                              */
  /*                                                                       */
  /*    Otherwise, if the `ft_open_pathname' bit is set, assume that this  */
  /*    is a normal file and use `pathname' to open it.                    */
  /*                                                                       */
  /*    If the `ft_open_driver' bit is set, @FT_Open_Face() will only      */
  /*    try to open the file with the driver whose handler is in `driver'. */
  /*                                                                       */
  /*    If the `ft_open_params' bit is set, the parameters given by        */
  /*    `num_params' and `params' will be used.  They are ignored          */
  /*    otherwise.                                                         */
  /*                                                                       */
  typedef struct  FT_Open_Args_
  {
    FT_Open_Flags   flags;
    const FT_Byte*  memory_base;
    FT_Long         memory_size;
    FT_String*      pathname;
    FT_Stream       stream;
    FT_Module       driver;
    FT_Int          num_params;
    FT_Parameter*   params;

  } FT_Open_Args;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Face                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new face object from a given resource and typeface index */
  /*    using a pathname to the font file.                                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    pathname   :: A path to the font file.                             */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    FT_New_Face() can be used to determine and/or check the font       */
  /*    format of a given font resource.  If the `face_index' field is     */
  /*    negative, the function will _not_ return any face handle in        */
  /*    `aface'.  Its return value should be 0 if the font format is       */
  /*    recognized, or non-zero otherwise.                                 */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_New_Face( FT_Library   library,
               const char*  filepathname,
               FT_Long      face_index,
               FT_Face     *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Memory_Face                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Creates a new face object from a given resource and typeface index */
  /*    using a font file already loaded into memory.                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    file_base  :: A pointer to the beginning of the font data.         */
  /*                                                                       */
  /*    file_size  :: The size of the memory chunk used by the font data.  */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The font data bytes are used _directly_ by the @FT_Face object.    */
  /*    This means that they are not copied, and that the client is        */
  /*    responsible for releasing/destroying them _after_ the              */
  /*    corresponding call to @FT_Done_Face .                              */
  /*                                                                       */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    FT_New_Memory_Face() can be used to determine and/or check the     */
  /*    font format of a given font resource.  If the `face_index' field   */
  /*    is negative, the function will _not_ return any face handle in     */
  /*    `aface'.  Its return value should be 0 if the font format is       */
  /*    recognized, or non-zero otherwise.                                 */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_New_Memory_Face( FT_Library      library,
                      const FT_Byte*  file_base,
                      FT_Long         file_size,
                      FT_Long         face_index,
                      FT_Face        *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Open_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Opens a face object from a given resource and typeface index using */
  /*    an `FT_Open_Args' structure.  If the face object doesn't exist, it */
  /*    will be created.                                                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library    :: A handle to the library resource.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    args       :: A pointer to an `FT_Open_Args' structure which must  */
  /*                  be filled by the caller.                             */
  /*                                                                       */
  /*    face_index :: The index of the face within the resource.  The      */
  /*                  first face has index 0.                              */
  /* <Output>                                                              */
  /*    aface      :: A handle to a new face object.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Unlike FreeType 1.x, this function automatically creates a glyph   */
  /*    slot for the face object which can be accessed directly through    */
  /*    `face->glyph'.                                                     */
  /*                                                                       */
  /*    FT_Open_Face() can be used to determine and/or check the font      */
  /*    format of a given font resource.  If the `face_index' field is     */
  /*    negative, the function will _not_ return any face handle in        */
  /*    `*face'.  Its return value should be 0 if the font format is       */
  /*    recognized, or non-zero otherwise.                                 */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Open_Face( FT_Library     library,
                FT_Open_Args*  args,
                FT_Long        face_index,
                FT_Face       *aface );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Attach_File                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    `Attaches' a given font file to an existing face.  This is usually */
  /*    to read additional information for a single face object.  For      */
  /*    example, it is used to read the AFM files that come with Type 1    */
  /*    fonts in order to add kerning data and other metrics.              */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face         :: The target face object.                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    filepathname :: An 8-bit pathname naming the `metrics' file.       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If your font file is in memory, or if you want to provide your     */
  /*    own input stream object, use FT_Attach_Stream().                   */
  /*                                                                       */
  /*    The meaning of the `attach' action (i.e., what really happens when */
  /*    the new file is read) is not fixed by FreeType itself.  It really  */
  /*    depends on the font format (and thus the font driver).             */
  /*                                                                       */
  /*    Client applications are expected to know what they are doing       */
  /*    when invoking this function.  Most drivers simply do not implement */
  /*    file attachments.                                                  */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Attach_File( FT_Face      face,
                  const char*  filepathname );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Attach_Stream                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is similar to FT_Attach_File() with the exception    */
  /*    that it reads the attachment from an arbitrary stream.             */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face       :: The target face object.                              */
  /*                                                                       */
  /* <Input>                                                               */
  /*    parameters :: A pointer to an FT_Open_Args structure used to       */
  /*                  describe the input stream to FreeType.               */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The meaning of the `attach' (i.e. what really happens when the     */
  /*    new file is read) is not fixed by FreeType itself.  It really      */
  /*    depends on the font format (and thus the font driver).             */
  /*                                                                       */
  /*    Client applications are expected to know what they are doing       */
  /*    when invoking this function.  Most drivers simply do not implement */
  /*    file attachments.                                                  */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Attach_Stream( FT_Face        face,
                    FT_Open_Args*  parameters );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Face                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discards a given face object, as well as all of its child slots    */
  /*    and sizes.                                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to a target face object.                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Done_Face( FT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Char_Size                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the character dimensions of a given face object.  The         */
  /*    `char_width' and `char_height' values are used for the width and   */
  /*    height, respectively, expressed in 26.6 fractional points.         */
  /*                                                                       */
  /*    If the horizontal or vertical resolution values are zero, a        */
  /*    default value of 72dpi is used.  Similarly, if one of the          */
  /*    character dimensions is zero, its value is set equal to the other. */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    size            :: A handle to a target size object.               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    char_width      :: The character width, in 26.6 fractional points. */
  /*                                                                       */
  /*    char_height     :: The character height, in 26.6 fractional        */
  /*                       points.                                         */
  /*                                                                       */
  /*    horz_resolution :: The horizontal resolution.                      */
  /*                                                                       */
  /*    vert_resolution :: The vertical resolution.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    When dealing with fixed-size faces (i.e., non-scalable formats),   */
  /*    use the function FT_Set_Pixel_Sizes().                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_Char_Size( FT_Face     face,
                    FT_F26Dot6  char_width,
                    FT_F26Dot6  char_height,
                    FT_UInt     horz_resolution,
                    FT_UInt     vert_resolution );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Pixel_Sizes                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Sets the character dimensions of a given face object.  The width   */
  /*    and height are expressed in integer pixels.                        */
  /*                                                                       */
  /*    If one of the character dimensions is zero, its value is set equal */
  /*    to the other.                                                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face         :: A handle to the target face object.                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    pixel_width  :: The character width, in integer pixels.            */
  /*                                                                       */
  /*    pixel_height :: The character height, in integer pixels.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The values of `pixel_width' and `pixel_height' correspond to the   */
  /*    pixel values of the _typographic_ character size, which are NOT    */
  /*    necessarily the same as the dimensions of the glyph `bitmap        */
  /*    cells'.                                                            */
  /*                                                                       */
  /*    The `character size' is really the size of an abstract square      */
  /*    called the `EM', used to design the font.  However, depending      */
  /*    on the font design, glyphs will be smaller or greater than the     */
  /*    EM.                                                                */
  /*                                                                       */
  /*    This means that setting the pixel size to, say, 8x8 doesn't        */
  /*    guarantee in any way that you will get glyph bitmaps that all fit  */
  /*    within an 8x8 cell (sometimes even far from it).                   */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_Pixel_Sizes( FT_Face  face,
                      FT_UInt  pixel_width,
                      FT_UInt  pixel_height );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Load_Glyph                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size.                                                  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face        :: A handle to the target face object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_XXX constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If the glyph image is not a bitmap, and if the bit flag            */
  /*    FT_LOAD_IGNORE_TRANSFORM is unset, the glyph image will be         */
  /*    transformed with the information passed to a previous call to      */
  /*    FT_Set_Transform().                                                */
  /*                                                                       */
  /*    Note that this also transforms the `face.glyph.advance' field, but */
  /*    *not* the values in `face.glyph.metrics'.                          */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Load_Glyph( FT_Face  face,
                 FT_UInt  glyph_index,
                 FT_Int   load_flags );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Load_Char                                                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to load a single glyph within a given glyph slot,  */
  /*    for a given size, according to its character code.                 */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face        :: A handle to a target face object where the glyph    */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    char_code   :: The glyph's character code, according to the        */
  /*                   current charmap used in the face.                   */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_XXX constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    If the face has no current charmap, or if the character code       */
  /*    is not defined in the charmap, this function will return an        */
  /*    error.                                                             */
  /*                                                                       */
  /*    If the glyph image is not a bitmap, and if the bit flag            */
  /*    FT_LOAD_IGNORE_TRANSFORM is unset, the glyph image will be         */
  /*    transformed with the information passed to a previous call to      */
  /*    FT_Set_Transform().                                                */
  /*                                                                       */
  /*    Note that this also transforms the `face.glyph.advance' field, but */
  /*    *not* the values in `face.glyph.metrics'.                          */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Load_Char( FT_Face   face,
                FT_ULong  char_code,
                FT_Int    load_flags );


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_NO_SCALE                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the vector outline being loaded should not be scaled to 26.6       */
  /*    fractional pixels, but kept in notional units.                     */
  /*                                                                       */
#define FT_LOAD_NO_SCALE  1


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_NO_HINTING                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the vector outline being loaded should not be fitted to the pixel  */
  /*    grid but simply scaled to 26.6 fractional pixels.                  */
  /*                                                                       */
  /*    This flag is ignored if FT_LOAD_NO_SCALE is set.                   */
  /*                                                                       */
#define FT_LOAD_NO_HINTING  2


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_RENDER                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the function should load the glyph and immediately convert it into */
  /*    a bitmap, if necessary, by calling FT_Render_Glyph().              */
  /*                                                                       */
  /*    Note that by default, FT_Load_Glyph() loads the glyph image in its */
  /*    native format.                                                     */
  /*                                                                       */
#define FT_LOAD_RENDER  4


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_NO_BITMAP                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the function should not load the bitmap or pixmap of a given       */
  /*    glyph.  This is useful when you do not want to load the embedded   */
  /*    bitmaps of scalable formats, as the native glyph image will be     */
  /*    loaded, and can then be rendered through FT_Render_Glyph().        */
  /*                                                                       */
#define FT_LOAD_NO_BITMAP  8


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_VERTICAL_LAYOUT                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the glyph image should be prepared for vertical layout.  This      */
  /*    basically means that `face.glyph.advance' will correspond to the   */
  /*    vertical advance height (instead of the default horizontal         */
  /*    advance width), and that the glyph image will translated to match  */
  /*    the vertical bearings positions.                                   */
  /*                                                                       */
#define FT_LOAD_VERTICAL_LAYOUT  16


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_FORCE_AUTOHINT                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the function should try to auto-hint the glyphs, even if a driver  */
  /*    specific hinter is available.                                      */
  /*                                                                       */
#define FT_LOAD_FORCE_AUTOHINT  32


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_CROP_BITMAP                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the font driver should try to crop the bitmap (i.e. remove all     */
  /*    space around its black bits) when loading it.  For now, this       */
  /*    really only works with embedded bitmaps in TrueType fonts.         */
  /*                                                                       */
#define FT_LOAD_CROP_BITMAP  64


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_PEDANTIC                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the glyph loader should perform a pedantic bytecode                */
  /*    interpretation.  Many popular fonts come with broken glyph         */
  /*    programs.  When this flag is set, loading them will return an      */
  /*    error.  Otherwise, errors are ignored by the loader, sometimes     */
  /*    resulting in ugly glyphs.                                          */
  /*                                                                       */
#define FT_LOAD_PEDANTIC  128


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the glyph loader should ignore the global advance width defined    */
  /*    in the font.  As far as we know, this is only used by the          */
  /*    X-TrueType font server, in order to deal correctly with the        */
  /*    incorrect metrics contained in DynaLab's TrueType CJK fonts.       */
  /*                                                                       */
#define FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH  512


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_NO_RECURSE                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the glyph loader should not load composite glyph recursively.      */
  /*    Rather, when a composite glyph is encountered, it should set       */
  /*    the values of `num_subglyphs' and `subglyphs', as well as set      */
  /*    `face->glyph.format' to ft_glyph_format_composite.                 */
  /*                                                                       */
  /*    This is for use by the auto-hinter and possibly other tools.       */
  /*    For nearly all applications, this flags should be left unset       */
  /*    when invoking FT_Load_Glyph().                                     */
  /*                                                                       */
  /*    Note that the flag forces the load of unscaled glyphs.             */
  /*                                                                       */
#define FT_LOAD_NO_RECURSE  1024


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_IGNORE_TRANSFORM                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the glyph loader should not try to transform the loaded glyph      */
  /*    image.                                                             */
  /*                                                                       */
#define FT_LOAD_IGNORE_TRANSFORM  2048


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_MONOCHROME                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Only used with FT_LOAD_RENDER set, it indicates that the returned  */
  /*    glyph image should be 1-bit monochrome.  This really tells the     */
  /*    glyph loader to use `ft_render_mode_mono' when calling             */
  /*    FT_Render_Glyph().                                                 */
  /*                                                                       */
#define FT_LOAD_MONOCHROME  4096


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_LINEAR_DESIGN                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the function should return the linearly scaled metrics expressed   */
  /*    in original font units, instead of the default 16.16 pixel values. */
  /*                                                                       */
#define FT_LOAD_LINEAR_DESIGN  8192

  /* temporary hack! */
#define FT_LOAD_SBITS_ONLY  16384


  /*************************************************************************/
  /*                                                                       */
  /* <Constant>                                                            */
  /*    FT_LOAD_DEFAULT                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A bit-field constant, used with FT_Load_Glyph() to indicate that   */
  /*    the function should try to load the glyph normally, i.e.,          */
  /*    embedded bitmaps are favored over outlines, vectors are always     */
  /*    scaled and grid-fitted.                                            */
  /*                                                                       */
#define FT_LOAD_DEFAULT  0


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Transform                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to set the transformation that is applied to glyph */
  /*    images just before they are converted to bitmaps in a glyph slot   */
  /*    when FT_Render_Glyph() is called.                                  */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face   :: A handle to the source face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    matrix :: A pointer to the transformation's 2x2 matrix.  Use 0 for */
  /*              the identity matrix.                                     */
  /*    delta  :: A pointer to the translation vector.  Use 0 for the null */
  /*              vector.                                                  */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The transformation is only applied to scalable image formats after */
  /*    the glyph has been loaded.  It means that hinting is unaltered by  */
  /*    the transformation and is performed on the character size given in */
  /*    the last call to FT_Set_Char_Sizes() or FT_Set_Pixel_Sizes().      */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Set_Transform( FT_Face     face,
                    FT_Matrix*  matrix,
                    FT_Vector*  delta );


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_Render_Mode                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration type that lists the render modes supported by the   */
  /*    FreeType 2 renderer(s).  A renderer is in charge of converting a   */
  /*    glyph image into a bitmap.                                         */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    ft_render_mode_normal :: This is the default render mode; it       */
  /*                             corresponds to 8-bit anti-aliased         */
  /*                             bitmaps, using 256 levels of gray.        */
  /*                                                                       */
  /*    ft_render_mode_mono   :: This render mode is used to produce 1-bit */
  /*                             monochrome bitmaps.                       */
  /*                                                                       */
  /* <Note>                                                                */
  /*    There is no render mode to produce 8-bit `monochrome' bitmaps --   */
  /*    you have to make the conversion yourself if you need such things   */
  /*    (besides, FreeType is not a graphics library).                     */
  /*                                                                       */
  /*    More modes might appear later for specific display modes (e.g. TV, */
  /*    LCDs, etc.).  They will be supported through the simple addition   */
  /*    of a renderer module, with no changes to the rest of the engine.   */
  /*                                                                       */
  typedef enum  FT_Render_Mode_
  {
    ft_render_mode_normal = 0,
    ft_render_mode_mono   = 1

  } FT_Render_Mode;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Render_Glyph                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Converts a given glyph image to a bitmap.  It does so by           */
  /*    inspecting the glyph image format, find the relevant renderer, and */
  /*    invoke it.                                                         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    slot        :: A handle to the glyph slot containing the image to  */
  /*                   convert.                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    render_mode :: This is the render mode used to render the glyph    */
  /*                   image into a bitmap.  See FT_Render_Mode for a list */
  /*                   of possible values.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Render_Glyph( FT_GlyphSlot  slot,
                   FT_UInt       render_mode );


  /*************************************************************************/
  /*                                                                       */
  /* <Enum>                                                                */
  /*    FT_Kerning_Mode                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    An enumeration used to specify which kerning values to return in   */
  /*    FT_Get_Kerning().                                                  */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    ft_kerning_default  :: Return scaled and grid-fitted kerning       */
  /*                           distances (value is 0).                     */
  /*                                                                       */
  /*    ft_kerning_unfitted :: Return scaled but un-grid-fitted kerning    */
  /*                           distances.                                  */
  /*                                                                       */
  /*    ft_kerning_unscaled :: Return the kerning vector in original font  */
  /*                           units.                                      */
  /*                                                                       */
  typedef enum  FT_Kerning_Mode_
  {
    ft_kerning_default  = 0,
    ft_kerning_unfitted,
    ft_kerning_unscaled

  } FT_Kerning_Mode;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Kerning                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the kerning vector between two glyphs of a same face.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to a source face object.                   */
  /*                                                                       */
  /*    left_glyph  :: The index of the left glyph in the kern pair.       */
  /*                                                                       */
  /*    right_glyph :: The index of the right glyph in the kern pair.      */
  /*                                                                       */
  /*    kern_mode   :: See FT_Kerning_Mode() for more information.         */
  /*                   Determines the scale/dimension of the returned      */
  /*                   kerning vector.                                     */
  /*                                                                       */
  /* <Output>                                                              */
  /*    akerning    :: The kerning vector.  This is in font units for      */
  /*                   scalable formats, and in pixels for fixed-sizes     */
  /*                   formats.                                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    Only horizontal layouts (left-to-right & right-to-left) are        */
  /*    supported by this method.  Other layouts, or more sophisticated    */
  /*    kernings, are out of the scope of this API function -- they can be */
  /*    implemented through format-specific interfaces.                    */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Get_Kerning( FT_Face     face,
                  FT_UInt     left_glyph,
                  FT_UInt     right_glyph,
                  FT_UInt     kern_mode,
                  FT_Vector  *akerning );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Glyph_Name                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the ASCII name of a given glyph in a face.  This only    */
  /*    works for those faces where FT_HAS_GLYPH_NAME(face) returns true.  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: A handle to a source face object.                   */
  /*                                                                       */
  /*    glyph_index :: The glyph index.                                    */
  /*                                                                       */
  /*    buffer_max  :: The maximal number of bytes available in the        */
  /*                   buffer.                                             */
  /*                                                                       */
  /* <Output>                                                              */
  /*    buffer      :: A pointer to a target buffer where the name will be */
  /*                   copied to.                                          */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An error is returned if the face doesn't provide glyph names or if */
  /*    the glyph index is invalid.  In all cases of failure, the first    */
  /*    byte of `buffer' will be set to 0 to indicate an empty name.       */
  /*                                                                       */
  /*    The glyph name is truncated to fit within the buffer if it is too  */
  /*    long.  The returned string is always zero-terminated.              */
  /*                                                                       */
  /*    This function is not compiled within the library if the config     */
  /*    macro FT_CONFIG_OPTION_NO_GLYPH_NAMES is defined in                */
  /*    `include/freetype/config/ftoptions.h'                              */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Get_Glyph_Name( FT_Face     face,
                     FT_UInt     glyph_index,
                     FT_Pointer  buffer,
                     FT_UInt     buffer_max );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Postscript_Name                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Retrieves the ASCII Postscript name of a given face, if available. */
  /*    This should only work with Postscript and TrueType fonts.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the source face object.                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A pointer to the face's Postscript name.  NULL if un-available.    */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The returned pointer is owned by the face and will be destroyed    */
  /*    with it.                                                           */
  /*                                                                       */
  FT_EXPORT( const char* )
  FT_Get_Postscript_Name( FT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Select_Charmap                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Selects a given charmap by its encoding tag (as listed in          */
  /*    `freetype.h').                                                     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face     :: A handle to the source face object.                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    encoding :: A handle to the selected charmap.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will return an error if no charmap in the face       */
  /*    corresponds to the encoding queried here.                          */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Select_Charmap( FT_Face      face,
                     FT_Encoding  encoding );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Set_Charmap                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Selects a given charmap for character code to glyph index          */
  /*    decoding.                                                          */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    face    :: A handle to the source face object.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charmap :: A handle to the selected charmap.                       */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function will return an error if the charmap is not part of   */
  /*    the face (i.e., if it is not listed in the face->charmaps[]        */
  /*    table).                                                            */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Set_Charmap( FT_Face     face,
                  FT_CharMap  charmap );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Char_Index                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the glyph index of a given character code.  This function  */
  /*    uses a charmap object to do the translation.                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the source face object.                    */
  /*                                                                       */
  /*    charcode :: The character code.                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The glyph index.  0 means `undefined character code'.              */
  /*                                                                       */
  FT_EXPORT( FT_UInt )
  FT_Get_Char_Index( FT_Face   face,
                     FT_ULong  charcode );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Name_Index                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Returns the glyph index of a given glyph name.  This function uses */
  /*    driver specific objects to do the translation.                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the source face object.                  */
  /*                                                                       */
  /*    glyph_name :: The glyph name.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The glyph index.  0 means `undefined character code'.              */
  /*                                                                       */
  FT_EXPORT( FT_UInt )
  FT_Get_Name_Index( FT_Face     face,
                     FT_String*  glyph_name );



  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    computations                                                       */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Computations                                                       */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    Crunching fixed numbers and vectors                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains various functions used to perform            */
  /*    computations on 16.16 fixed-float numbers or 2d vectors.           */
  /*                                                                       */
  /* <Order>                                                               */
  /*    FT_MulDiv                                                          */
  /*    FT_MulFix                                                          */
  /*    FT_DivFix                                                          */
  /*    FT_RoundFix                                                        */
  /*    FT_CeilFix                                                         */
  /*    FT_FloorFix                                                        */
  /*    FT_Vector_Transform                                                */
  /*    FT_Matrix_Multiply                                                 */
  /*    FT_Matrix_Invert                                                   */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_MulDiv                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to perform the computation `(a*b)/c'   */
  /*    with maximal accuracy (it uses a 64-bit intermediate integer       */
  /*    whenever necessary).                                               */
  /*                                                                       */
  /*    This function isn't necessarily as fast as some processor specific */
  /*    operations, but is at least completely portable.                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The first multiplier.                                         */
  /*    b :: The second multiplier.                                        */
  /*    c :: The divisor.                                                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a*b)/c'.  This function never traps when trying to */
  /*    divide by zero; it simply returns `MaxInt' or `MinInt' depending   */
  /*    on the signs of `a' and `b'.                                       */
  /*                                                                       */
  FT_EXPORT( FT_Long )
  FT_MulDiv( FT_Long  a,
             FT_Long  b,
             FT_Long  c );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_MulFix                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to perform the computation             */
  /*    `(a*b)/0x10000' with maximal accuracy.  Most of the time this is   */
  /*    used to multiply a given value by a 16.16 fixed float factor.      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The first multiplier.                                         */
  /*    b :: The second multiplier.  Use a 16.16 factor here whenever      */
  /*         possible (see note below).                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a*b)/0x10000'.                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function has been optimized for the case where the absolute   */
  /*    value of `a' is less than 2048, and `b' is a 16.16 scaling factor. */
  /*    As this happens mainly when scaling from notional units to         */
  /*    fractional pixels in FreeType, it resulted in noticeable speed     */
  /*    improvements between versions 2.x and 1.x.                         */
  /*                                                                       */
  /*    As a conclusion, always try to place a 16.16 factor as the         */
  /*    _second_ argument of this function; this can make a great          */
  /*    difference.                                                        */
  /*                                                                       */
  FT_EXPORT( FT_Long )
  FT_MulFix( FT_Long  a,
             FT_Long  b );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_DivFix                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to perform the computation             */
  /*    `(a*0x10000)/b' with maximal accuracy.  Most of the time, this is  */
  /*    used to divide a given value by a 16.16 fixed float factor.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The first multiplier.                                         */
  /*    b :: The second multiplier.  Use a 16.16 factor here whenever      */
  /*         possible (see note below).                                    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a*0x10000)/b'.                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The optimization for FT_DivFix() is simple: If (a << 16) fits in   */
  /*    32 bits, then the division is computed directly.  Otherwise, we    */
  /*    use a specialized version of the old FT_MulDiv64().                */
  /*                                                                       */
  FT_EXPORT( FT_Long )
  FT_DivFix( FT_Long  a,
             FT_Long  b );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_RoundFix                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to round a 16.16 fixed number.         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The number to be rounded.                                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a + 0x8000) & -0x10000'.                           */
  /*                                                                       */
  FT_EXPORT( FT_Fixed )
  FT_RoundFix( FT_Fixed  a );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_CeilFix                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to compute the ceiling function of a   */
  /*    16.16 fixed number.                                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The number for which the ceiling function is to be computed.  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `(a + 0x10000 - 1) & -0x10000'.                      */
  /*                                                                       */
  FT_EXPORT( FT_Fixed )
  FT_CeilFix( FT_Fixed  a );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_FloorFix                                                        */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A very simple function used to compute the floor function of a     */
  /*    16.16 fixed number.                                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    a :: The number for which the floor function is to be computed.    */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The result of `a & -0x10000'.                                      */
  /*                                                                       */
  FT_EXPORT( FT_Fixed )
  FT_FloorFix( FT_Fixed  a );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Vector_Transform                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Transforms a single vector through a 2x2 matrix.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    vector :: The target vector to transform.                          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    matrix :: A pointer to the source 2x2 matrix.                      */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The result is undefined if either `vector' or `matrix' is invalid. */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Vector_Transform( FT_Vector*  vec,
                       FT_Matrix*  matrix );


  /* */


FT_END_HEADER

#endif /* __FREETYPE_H__ */


/* END */
