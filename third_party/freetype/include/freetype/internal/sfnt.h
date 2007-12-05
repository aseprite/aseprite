/***************************************************************************/
/*                                                                         */
/*  sfnt.h                                                                 */
/*                                                                         */
/*    High-level `sfnt' driver interface (specification).                  */
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


#ifndef __SFNT_H__
#define __SFNT_H__


#include <ft2build.h>
#include FT_INTERNAL_DRIVER_H
#include FT_INTERNAL_TRUETYPE_TYPES_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Init_Face_Func                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    First part of the SFNT face object initialization.  This will find */
  /*    the face in a SFNT file or collection, and load its format tag in  */
  /*    face->format_tag.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    face_index :: The index of the TrueType font, if we are opening a  */
  /*                  collection.                                          */
  /*                                                                       */
  /*    num_params :: The number of additional parameters.                 */
  /*                                                                       */
  /*    params     :: Optional additional parameters.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the font file's origin.               */
  /*                                                                       */
  /*    This function recognizes fonts embedded in a `TrueType             */
  /*    collection'.                                                       */
  /*                                                                       */
  /*    Once the format tag has been validated by the font driver, it      */
  /*    should then call the TT_Load_Face_Func() callback to read the rest */
  /*    of the SFNT tables in the object.                                  */
  /*                                                                       */
  typedef FT_Error
  (*TT_Init_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Face_Func                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Second part of the SFNT face object initialization.  This will     */
  /*    load the common SFNT tables (head, OS/2, maxp, metrics, etc.) in   */
  /*    the face object.                                                   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    face_index :: The index of the TrueType font, if we are opening a  */
  /*                  collection.                                          */
  /*                                                                       */
  /*    num_params :: The number of additional parameters.                 */
  /*                                                                       */
  /*    params     :: Optional additional parameters.                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function must be called after TT_Init_Face_Func().            */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Face_Func)( FT_Stream      stream,
                        TT_Face        face,
                        FT_Int         face_index,
                        FT_Int         num_params,
                        FT_Parameter*  params );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Done_Face_Func                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A callback used to delete the common SFNT data from a face.        */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This function does NOT destroy the face object.                    */
  /*                                                                       */
  typedef void
  (*TT_Done_Face_Func)( TT_Face  face );


  typedef FT_Module_Interface
  (*SFNT_Get_Interface_Func)( FT_Module    module,
                              const char*  interface );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_SFNT_Header_Func                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the header of a SFNT font file.  Supports collections.       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face       :: A handle to the target face object.                  */
  /*                                                                       */
  /*    stream     :: The input stream.                                    */
  /*                                                                       */
  /*    face_index :: The index of the TrueType font, if we are opening a  */
  /*                  collection.                                          */
  /*                                                                       */
  /* <Output>                                                              */
  /*    sfnt       :: The SFNT header.                                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be at the font file's origin.               */
  /*                                                                       */
  /*    This function recognizes fonts embedded in a `TrueType             */
  /*    collection'.                                                       */
  /*                                                                       */
  /*    This function checks that the header is valid by looking at the    */
  /*    values of `search_range', `entry_selector', and `range_shift'.     */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_SFNT_Header_Func)( TT_Face       face,
                               FT_Stream     stream,
                               FT_Long       face_index,
                               SFNT_Header*  sfnt );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Directory_Func                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the table directory into a face object.                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face      :: A handle to the target face object.                   */
  /*                                                                       */
  /*    stream    :: The input stream.                                     */
  /*                                                                       */
  /*    sfnt      :: The SFNT header.                                      */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The stream cursor must be on the first byte after the 4-byte font  */
  /*    format tag.  This is the case just after a call to                 */
  /*    TT_Load_Format_Tag().                                              */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Directory_Func)( TT_Face       face,
                             FT_Stream     stream,
                             SFNT_Header*  sfnt );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Any_Func                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads any font table into client memory.                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: The face object to look for.                             */
  /*                                                                       */
  /*    tag    :: The tag of table to load.  Use the value 0 if you want   */
  /*              to access the whole font file, else set this parameter   */
  /*              to a valid TrueType table tag that you can forge with    */
  /*              the MAKE_TT_TAG macro.                                   */
  /*                                                                       */
  /*    offset :: The starting offset in the table (or the file if         */
  /*              tag == 0).                                               */
  /*                                                                       */
  /*    length :: The address of the decision variable:                    */
  /*                                                                       */
  /*                If length == NULL:                                     */
  /*                  Loads the whole table.  Returns an error if          */
  /*                  `offset' == 0!                                       */
  /*                                                                       */
  /*                If *length == 0:                                       */
  /*                  Exits immediately; returning the length of the given */
  /*                  table or of the font file, depending on the value of */
  /*                  `tag'.                                               */
  /*                                                                       */
  /*                If *length != 0:                                       */
  /*                  Loads the next `length' bytes of table or font,      */
  /*                  starting at offset `offset' (in table or font too).  */
  /*                                                                       */
  /* <Output>                                                              */
  /*    buffer :: The address of target buffer.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    TrueType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Any_Func)( TT_Face    face,
                       FT_ULong   tag,
                       FT_Long    offset,
                       FT_Byte   *buffer,
                       FT_ULong*  length );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_SBit_Image_Func                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given glyph sbit image from the font resource.  This also  */
  /*    returns its metrics.                                               */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face        :: The target face object.                             */
  /*                                                                       */
  /*    x_ppem      :: The horizontal resolution in points per EM.         */
  /*                                                                       */
  /*    y_ppem      :: The vertical resolution in points per EM.           */
  /*                                                                       */
  /*    glyph_index :: The current glyph index.                            */
  /*                                                                       */
  /*    stream      :: The input stream.                                   */
  /*                                                                       */
  /* <Output>                                                              */
  /*    amap        :: The target pixmap.                                  */
  /*                                                                       */
  /*    ametrics    :: A big sbit metrics structure for the glyph image.   */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    glyph sbit exists for the index.                                   */
  /*                                                                       */
  /*  <Note>                                                               */
  /*    The `map.buffer' field is always freed before the glyph is loaded. */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_SBit_Image_Func)( TT_Face           face,
                              FT_ULong          strike_index,
                              FT_UInt           glyph_index,
                              FT_UInt           load_flags,
                              FT_Stream         stream,
                              FT_Bitmap        *amap,
                              TT_SBit_Metrics  *ametrics );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Set_SBit_Strike_Func                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Selects an sbit strike for given horizontal and vertical ppem      */
  /*    values.                                                            */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face          :: The target face object.                           */
  /*                                                                       */
  /*    x_ppem        :: The horizontal resolution in points per EM.       */
  /*                                                                       */
  /*    y_ppem        :: The vertical resolution in points per EM.         */
  /*                                                                       */
  /* <Output>                                                              */
  /*    astrike_index :: The index of the sbit strike.                     */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.  Returns an error if no     */
  /*    sbit strike exists for the selected ppem values.                   */
  /*                                                                       */
  typedef FT_Error
  (*TT_Set_SBit_Strike_Func)( TT_Face    face,
                              FT_Int     x_ppem,
                              FT_Int     y_ppem,
                              FT_ULong  *astrike_index );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Get_PS_Name_Func                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Gets the PostScript glyph name of a glyph.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    index  :: The glyph index.                                         */
  /*                                                                       */
  /*    PSname :: The address of a string pointer.  Will be NULL in case   */
  /*              of error, otherwise it is a pointer to the glyph name.   */
  /*                                                                       */
  /*              You must not modify the returned string!                 */
  /*                                                                       */
  /* <Output>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Get_PS_Name_Func)( TT_Face      face,
                          FT_UInt      index,
                          FT_String**  PSname );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Metrics_Func                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads the horizontal or vertical header in a face object.          */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face     :: A handle to the target face object.                    */
  /*                                                                       */
  /*    stream   :: The input stream.                                      */
  /*                                                                       */
  /*    vertical :: A boolean flag.  If set, load vertical metrics.        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Metrics_Func)( TT_Face    face,
                           FT_Stream  stream,
                           FT_Bool    vertical );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_CharMap_Load_Func                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given TrueType character map into memory.                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the parent face object.                      */
  /*                                                                       */
  /*    stream :: A handle to the current stream object.                   */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    cmap   :: A pointer to a cmap object.                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function assumes that the stream is already in use (i.e.,      */
  /*    opened).  In case of error, all partially allocated tables are     */
  /*    released.                                                          */
  /*                                                                       */
  typedef FT_Error
  (*TT_CharMap_Load_Func)( TT_Face        face,
                           TT_CMapTable*  cmap,
                           FT_Stream      input );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_CharMap_Free_Func                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Destroys a character mapping table.                                */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the parent face object.                        */
  /*                                                                       */
  /*    cmap :: A handle to a cmap object.                                 */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  typedef FT_Error
  (*TT_CharMap_Free_Func)( TT_Face        face,
                           TT_CMapTable*  cmap );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Load_Table_Func                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Loads a given TrueType table.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face   :: A handle to the target face object.                      */
  /*                                                                       */
  /*    stream :: The input stream.                                        */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The function will use `face->goto_table' to seek the stream to     */
  /*    the start of the table.                                            */
  /*                                                                       */
  typedef FT_Error
  (*TT_Load_Table_Func)( TT_Face    face,
                         FT_Stream  stream );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    TT_Free_Table_Func                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Frees a given TrueType table.                                      */
  /*                                                                       */
  /* <Input>                                                               */
  /*    face :: A handle to the target face object.                        */
  /*                                                                       */
  typedef void
  (*TT_Free_Table_Func)( TT_Face  face );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    SFNT_Interface                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This structure holds pointers to the functions used to load and    */
  /*    free the basic tables that are required in a `sfnt' font file.     */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    Check the various xxx_Func() descriptions for details.             */
  /*                                                                       */
  typedef struct  SFNT_Interface_
  {
    TT_Goto_Table_Func        goto_table;

    TT_Init_Face_Func         init_face;
    TT_Load_Face_Func         load_face;
    TT_Done_Face_Func         done_face;
    SFNT_Get_Interface_Func   get_interface;

    TT_Load_Any_Func          load_any;
    TT_Load_SFNT_Header_Func  load_sfnt_header;
    TT_Load_Directory_Func    load_directory;

    /* these functions are called by `load_face' but they can also  */
    /* be called from external modules, if there is a need to do so */
    TT_Load_Table_Func        load_header;
    TT_Load_Metrics_Func      load_metrics;
    TT_Load_Table_Func        load_charmaps;
    TT_Load_Table_Func        load_max_profile;
    TT_Load_Table_Func        load_os2;
    TT_Load_Table_Func        load_psnames;

    TT_Load_Table_Func        load_names;
    TT_Free_Table_Func        free_names;

    /* optional tables */
    TT_Load_Table_Func        load_hdmx;
    TT_Free_Table_Func        free_hdmx;

    TT_Load_Table_Func        load_kerning;
    TT_Load_Table_Func        load_gasp;
    TT_Load_Table_Func        load_pclt;

    /* see `ttload.h' */
    TT_Load_Table_Func        load_bitmap_header;

    /* see `ttsbit.h' */
    TT_Set_SBit_Strike_Func   set_sbit_strike;
    TT_Load_Table_Func        load_sbits;
    TT_Load_SBit_Image_Func   load_sbit_image;
    TT_Free_Table_Func        free_sbits;

    /* see `ttpost.h' */
    TT_Get_PS_Name_Func       get_psname;
    TT_Free_Table_Func        free_psnames;

    /* see `ttcmap.h' */
    TT_CharMap_Load_Func      load_charmap;
    TT_CharMap_Free_Func      free_charmap;

  } SFNT_Interface;


FT_END_HEADER

#endif /* __SFNT_H__ */


/* END */
