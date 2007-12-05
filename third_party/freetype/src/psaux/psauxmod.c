/***************************************************************************/
/*                                                                         */
/*  psauxmod.c                                                             */
/*                                                                         */
/*    FreeType auxiliary PostScript module implementation (body).          */
/*                                                                         */
/*  Copyright 2000-2001 by                                                 */
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
#include "psauxmod.h"
#include "psobjs.h"
#include "t1decode.h"


  FT_CALLBACK_TABLE_DEF
  const PS_Table_Funcs  ps_table_funcs =
  {
    PS_Table_New,
    PS_Table_Done,
    PS_Table_Add,
    PS_Table_Release
  };


  FT_CALLBACK_TABLE_DEF
  const T1_Parser_Funcs  t1_parser_funcs =
  {
    T1_Init_Parser,
    T1_Done_Parser,
    T1_Skip_Spaces,
    T1_Skip_Alpha,
    T1_ToInt,
    T1_ToFixed,
    T1_ToCoordArray,
    T1_ToFixedArray,
    T1_ToToken,
    T1_ToTokenArray,
    T1_Load_Field,
    T1_Load_Field_Table
  };


  FT_CALLBACK_TABLE_DEF
  const T1_Builder_Funcs  t1_builder_funcs =
  {
    T1_Builder_Init,
    T1_Builder_Done,
    T1_Builder_Check_Points,
    T1_Builder_Add_Point,
    T1_Builder_Add_Point1,
    T1_Builder_Add_Contour,
    T1_Builder_Start_Point,
    T1_Builder_Close_Contour
  };


  FT_CALLBACK_TABLE_DEF
  const T1_Decoder_Funcs  t1_decoder_funcs =
  {
    T1_Decoder_Init,
    T1_Decoder_Done,
    T1_Decoder_Parse_Charstrings
  };


  static
  const PSAux_Interface  psaux_interface =
  {
    &ps_table_funcs,
    &t1_parser_funcs,
    &t1_builder_funcs,
    &t1_decoder_funcs,

    T1_Decrypt
  };


  FT_CALLBACK_TABLE_DEF
  const FT_Module_Class  psaux_module_class =
  {
    0,
    sizeof( FT_ModuleRec ),
    "psaux",
    0x10000L,
    0x20000L,

    &psaux_interface,  /* module-specific interface */

    (FT_Module_Constructor)0,
    (FT_Module_Destructor) 0,
    (FT_Module_Requester)  0
  };


/* END */
