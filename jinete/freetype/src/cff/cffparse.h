/***************************************************************************/
/*                                                                         */
/*  cffparse.h                                                             */
/*                                                                         */
/*    CFF token stream parser (specification)                              */
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


#ifndef __CFF_PARSE_H__
#define __CFF_PARSE_H__


#include <ft2build.h>
#include FT_INTERNAL_CFF_TYPES_H
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER


#define CFF_MAX_STACK_DEPTH  96

#define CFF_CODE_TOPDICT  0x1000
#define CFF_CODE_PRIVATE  0x2000


  typedef struct  CFF_Parser_
  {
    FT_Byte*   start;
    FT_Byte*   limit;
    FT_Byte*   cursor;

    FT_Byte*   stack[CFF_MAX_STACK_DEPTH + 1];
    FT_Byte**  top;

    FT_UInt    object_code;
    void*      object;

  } CFF_Parser;


  FT_LOCAL void
  CFF_Parser_Init( CFF_Parser*  parser,
                   FT_UInt      code,
                   void*        object );

  FT_LOCAL FT_Error
  CFF_Parser_Run( CFF_Parser*  parser,
                  FT_Byte*     start,
                  FT_Byte*     limit );


FT_END_HEADER


#endif /* __CFF_PARSE_H__ */


/* END */
