/***************************************************************************/
/*                                                                         */
/*  ftmoderr.h                                                             */
/*                                                                         */
/*    FreeType module error offsets (specification).                       */
/*                                                                         */
/*  Copyright 2001 by                                                      */
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
  /* This file is used to define the FreeType module error offsets.        */
  /*                                                                       */
  /* The lower byte gives the error code, the higher byte gives the        */
  /* module.  The base module has error offset 0.  For example, the error  */
  /* `FT_Err_Invalid_File_Format' has value 0x003, the error               */
  /* `TT_Err_Invalid_File_Format' has value 0xB03, the error               */
  /* `T1_Err_Invalid_File_Format' has value 0xC03, etc.                    */
  /*                                                                       */
  /* Undefine the macro FT_CONFIG_OPTION_USE_MODULE_ERRORS in ftoption.h   */
  /* to make the higher byte always zero (disabling the module error       */
  /* mechanism).                                                           */
  /*                                                                       */
  /* It can also be used to create a module error message table easily     */
  /* with something like                                                   */
  /*                                                                       */
  /*   {                                                                   */
  /*     #undef __FTMODERR_H__                                             */
  /*     #define FT_MODERRDEF( e, v, s )  { FT_Mod_Err_ ## e, s },         */
  /*     #define FT_MODERR_START_LIST     {                                */
  /*     #define FT_MODERR_END_LIST       { 0, 0 } };                      */
  /*                                                                       */
  /*     const struct                                                      */
  /*     {                                                                 */
  /*       int          mod_err_offset;                                    */
  /*       const char*  mod_err_msg                                        */
  /*     } ft_mod_errors[] =                                               */
  /*                                                                       */
  /*     #include FT_MODULE_ERRORS_H                                       */
  /*   }                                                                   */
  /*                                                                       */
  /* To use such a table, all errors must be ANDed with 0xFF00 to remove   */
  /* the error code.                                                       */
  /*                                                                       */
  /*************************************************************************/


#ifndef __FTMODERR_H__
#define __FTMODERR_H__


  /*******************************************************************/
  /*******************************************************************/
  /*****                                                         *****/
  /*****                       SETUP MACROS                      *****/
  /*****                                                         *****/
  /*******************************************************************/
  /*******************************************************************/


#undef  FT_NEED_EXTERN_C

#ifndef FT_MODERRDEF

#ifdef FT_CONFIG_OPTION_USE_MODULE_ERRORS
#define FT_MODERRDEF( e, v, s )  FT_Mod_Err_ ## e = v,
#else
#define FT_MODERRDEF( e, v, s )  FT_Mod_Err_ ## e = 0,
#endif

#define FT_MODERR_START_LIST  enum {
#define FT_MODERR_END_LIST    FT_Mod_Err_Max };

#ifdef __cplusplus
#define FT_NEED_EXTERN_C
  extern "C" {
#endif

#endif /* !FT_MODERRDEF */


  /*******************************************************************/
  /*******************************************************************/
  /*****                                                         *****/
  /*****               LIST MODULE ERROR BASES                   *****/
  /*****                                                         *****/
  /*******************************************************************/
  /*******************************************************************/


#ifdef FT_MODERR_START_LIST
  FT_MODERR_START_LIST
#endif


  FT_MODERRDEF( Base,     0x000, "base module" )
  FT_MODERRDEF( Autohint, 0x100, "autohinter module" )
  FT_MODERRDEF( Cache,    0x200, "cache module" )
  FT_MODERRDEF( CFF,      0x300, "CFF module" )
  FT_MODERRDEF( CID,      0x400, "CID module" )
  FT_MODERRDEF( PCF,      0x500, "PCF module" )
  FT_MODERRDEF( PSaux,    0x600, "PS auxiliary module" )
  FT_MODERRDEF( PSnames,  0x700, "PS names module" )
  FT_MODERRDEF( Raster,   0x800, "raster module" )
  FT_MODERRDEF( SFNT,     0x900, "SFNT module" )
  FT_MODERRDEF( Smooth,   0xA00, "smooth raster module" )
  FT_MODERRDEF( TrueType, 0xB00, "TrueType module" )
  FT_MODERRDEF( Type1,    0xC00, "Type 1 module" )
  FT_MODERRDEF( Winfonts, 0xD00, "Windows FON/FNT module" )


#ifdef FT_MODERR_END_LIST
  FT_MODERR_END_LIST
#endif


  /*******************************************************************/
  /*******************************************************************/
  /*****                                                         *****/
  /*****                      CLEANUP                            *****/
  /*****                                                         *****/
  /*******************************************************************/
  /*******************************************************************/


#ifdef FT_NEED_EXTERN_C
  }
#endif

#undef FT_MODERR_START_LIST
#undef FT_MODERR_END_LIST
#undef FT_MODERRDEF
#undef FT_NEED_EXTERN_C


#endif /* __FTMODERR_H__ */


/* END */
