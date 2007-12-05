/***************************************************************************/
/*                                                                         */
/*  cidriver.h                                                             */
/*                                                                         */
/*    High-level CID driver interface (specification).                     */
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


#ifndef __CIDRIVER_H__
#define __CIDRIVER_H__


#include <ft2build.h>
#include FT_INTERNAL_DRIVER_H


FT_BEGIN_HEADER


  FT_CALLBACK_TABLE
  const  FT_Driver_Class  t1cid_driver_class;


FT_END_HEADER

#endif /* __CIDRIVER_H__ */


/* END */
