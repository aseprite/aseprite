/************************************************************************\
**                                                                      **
**                GrIP Prototype API Interface Library                  **
**                                for                                   **
**                               DJGPP                                  **
**                                                                      **
**                           Revision 1.00                              **
**                                                                      **
**  COPYRIGHT:                                                          **
**                                                                      **
**    (C) Copyright Advanced Gravis Computer Technology Ltd 1995.       **
**        All Rights Reserved.                                          **
**                                                                      **
**  DISCLAIMER OF WARRANTIES:                                           **
**                                                                      **
**    The following [enclosed] code is provided to you "AS IS",         **
**    without warranty of any kind.  You have a royalty-free right to   **
**    use, modify, reproduce and distribute the following code (and/or  **
**    any modified version) provided that you agree that Advanced       **
**    Gravis has no warranty obligations and shall not be liable for    **
**    any damages arising out of your use of this code, even if they    **
**    have been advised of the possibility of such damages.  This       **
**    Copyright statement and Disclaimer of Warranties may not be       **
**    removed.                                                          **
**                                                                      **
**  HISTORY:                                                            **
**                                                                      **
**    0.102   Jul 12 95   David Bollo     Initial public release on     **
**                                          GrIP hardware               **
**    0.200   Aug 10 95   David Bollo     Added Gravis Loadable Library **
**                                          support                     **
**    0.201   Aug 11 95   David Bollo     Removed Borland C++ support   **
**                                          for maintenance reasons     **
**    1.00    Nov  1 95   David Bollo     First official release as     **
**                                          part of GrIP SDK            **
**                                                                      **
\************************************************************************/

#ifndef ALLEGRO_DOS_GRIP_H
#define ALLEGRO_DOS_GRIP_H

/* 2. Type Definitions */
   typedef unsigned char GRIP_SLOT;
   typedef unsigned char GRIP_CLASS;
   typedef unsigned char GRIP_INDEX;
   typedef unsigned short GRIP_VALUE;
   typedef unsigned long GRIP_BITFIELD;
   typedef unsigned short GRIP_BOOL;
   typedef char *GRIP_STRING;
   typedef void *GRIP_BUF;
   typedef unsigned char *GRIP_BUF_C;
   typedef unsigned short *GRIP_BUF_S;
   typedef unsigned long *GRIP_BUF_L;

/* Standard Classes */
#define GRIP_CLASS_BUTTON                 ((GRIP_CLASS)1)
#define GRIP_CLASS_AXIS                   ((GRIP_CLASS)2)
#define GRIP_CLASS_POV_HAT                ((GRIP_CLASS)3)
#define GRIP_CLASS_VELOCITY               ((GRIP_CLASS)4)
#define GRIP_CLASS_THROTTLE               ((GRIP_CLASS)5)
#define GRIP_CLASS_ANALOG                 ((GRIP_CLASS)6)
#define GRIP_CLASS_MIN                    GRIP_CLASS_BUTTON
#define GRIP_CLASS_MAX                    GRIP_CLASS_ANALOG

/* Refresh Flags */
#define GRIP_REFRESH_COMPLETE             0     /* Default */
#define GRIP_REFRESH_PARTIAL              2
#define GRIP_REFRESH_TRANSMIT             0     /* Default */
#define GRIP_REFRESH_NOTRANSMIT           4

/* 3.1 System API Calls */
   GRIP_BOOL _GrInitialize(void);
   void _GrShutdown(void);
   GRIP_BITFIELD _GrRefresh(GRIP_BITFIELD flags);

/* 3.2 Configuration API Calls */
   GRIP_BITFIELD _GrGetSlotMap(void);
   GRIP_BITFIELD _GrGetClassMap(GRIP_SLOT s);
   GRIP_BITFIELD _GrGetOEMClassMap(GRIP_SLOT s);
   GRIP_INDEX _GrGetMaxIndex(GRIP_SLOT s, GRIP_CLASS c);
   GRIP_VALUE _GrGetMaxValue(GRIP_SLOT s, GRIP_CLASS c);

/* 3.3 Data API Calls */
   GRIP_VALUE _GrGetValue(GRIP_SLOT s, GRIP_CLASS c, GRIP_INDEX i);
   GRIP_BITFIELD _GrGetPackedValues(GRIP_SLOT s, GRIP_CLASS c, GRIP_INDEX start, GRIP_INDEX end);
   void _GrSetValue(GRIP_SLOT s, GRIP_CLASS c, GRIP_INDEX i, GRIP_VALUE v);

/* 3.4 OEM Information API Calls */
   void _GrGetVendorName(GRIP_SLOT s, GRIP_STRING name);
   GRIP_VALUE _GrGetProductName(GRIP_SLOT s, GRIP_STRING name);
   void _GrGetControlName(GRIP_SLOT s, GRIP_CLASS c, GRIP_INDEX i, GRIP_STRING name);
   GRIP_BITFIELD _GrGetCaps(GRIP_SLOT s, GRIP_CLASS c, GRIP_INDEX i);

/* 3.5 Library Management Calls */
   GRIP_BOOL _GrLink(GRIP_BUF image, GRIP_VALUE size);
   GRIP_BOOL _GrUnlink(void);

   GRIP_BOOL _Gr__Link(void);
   void _Gr__Unlink(void);

/* Diagnostic Information Calls */
   GRIP_VALUE _GrGetSWVer(void);
   GRIP_VALUE _GrGetHWVer(void);
   GRIP_VALUE _GrGetDiagCnt(void);
   unsigned long _GrGetDiagReg(GRIP_INDEX reg);

/* API Call Thunk */
   typedef unsigned char GRIP_THUNK[14];
   extern GRIP_THUNK _GRIP_Thunk;

#endif                          /* ALLEGRO_DOS_GRIP_H */
