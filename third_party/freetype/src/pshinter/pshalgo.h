/***************************************************************************/
/*                                                                         */
/*  pshalgo.h                                                              */
/*                                                                         */
/*    This header file defines the used hinting algorithm.                 */
/*                                                                         */
/*  Copyright 2001 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used        */
/*  modified and distributed under the terms of the FreeType project       */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __PSHALGO_H__
#define __PSHALGO_H__

FT_BEGIN_HEADER

/* define to choose hinting algorithm */
#define PSH_ALGORITHM_2

#ifdef PSH_ALGORITHM_1

#include "pshalgo1.h"
#define PS_HINTS_APPLY_FUNC  ps1_hints_apply

#else

#include "pshalgo2.h"
#define PS_HINTS_APPLY_FUNC  ps2_hints_apply

#endif

FT_END_HEADER

#endif /* __PSHALGO_H__ */


/* END */
