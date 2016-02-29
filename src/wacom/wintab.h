/* -------------------------------- wintab.h -------------------------------- */
/* Combined 16 & 32-bit version. */

/*------------------------------------------------------------------------------
The text and information contained in this file may be freely used,
copied, or distributed without compensation or licensing restrictions.

This file is Copyright (c) Wacom Company, Ltd. 2010 All Rights Reserved
with portions copyright 1991-1998 by LCS/Telegraphics.
------------------------------------------------------------------------------*/

#ifndef _INC_WINTAB	/* prevent multiple includes */
#define _INC_WINTAB

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* -------------------------------------------------------------------------- */
/* Messages */
#ifndef NOWTMESSAGES

	#define WT_DEFBASE			0x7FF0
	#define WT_MAXOFFSET			0xF

	#define _WT_PACKET(b)		((b)+0)
	#define _WT_CTXOPEN(b)		((b)+1)
	#define _WT_CTXCLOSE(b)		((b)+2)
	#define _WT_CTXUPDATE(b)	((b)+3)
	#define _WT_CTXOVERLAP(b)	((b)+4)
	#define _WT_PROXIMITY(b)	((b)+5)
	#define _WT_INFOCHANGE(b)	((b)+6)
	#define _WT_CSRCHANGE(b)	((b)+7) /* 1.1 */
	#define _WT_PACKETEXT(b)	((b)+8) /* 1.4 */
	#define _WT_MAX(b)			((b)+WT_MAXOFFSET)

	#define WT_PACKET				_WT_PACKET(WT_DEFBASE)
	#define WT_CTXOPEN			_WT_CTXOPEN(WT_DEFBASE)
	#define WT_CTXCLOSE			_WT_CTXCLOSE(WT_DEFBASE)
	#define WT_CTXUPDATE			_WT_CTXUPDATE(WT_DEFBASE)
	#define WT_CTXOVERLAP		_WT_CTXOVERLAP(WT_DEFBASE)
	#define WT_PROXIMITY			_WT_PROXIMITY(WT_DEFBASE)
	#define WT_INFOCHANGE		_WT_INFOCHANGE(WT_DEFBASE)
	#define WT_CSRCHANGE			_WT_CSRCHANGE(WT_DEFBASE) /* 1.1 */
	#define WT_PACKETEXT			_WT_PACKETEXT(WT_DEFBASE) /* 1.4 */
	#define WT_MAX					_WT_MAX(WT_DEFBASE)

#endif

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* Data Types */

/* -------------------------------------------------------------------------- */
/* COMMON DATA DEFS */

DECLARE_HANDLE(HMGR);		/* manager handle */
DECLARE_HANDLE(HCTX);		/* context handle */
DECLARE_HANDLE(HWTHOOK);	/* hook handle */

typedef DWORD WTPKT;			/* packet mask */


#ifndef NOWTPKT

	/* WTPKT bits */
	#define PK_CONTEXT				0x0001	/* reporting context */
	#define PK_STATUS					0x0002	/* status bits */
	#define PK_TIME					0x0004	/* time stamp */
	#define PK_CHANGED				0x0008	/* change bit vector */
	#define PK_SERIAL_NUMBER		0x0010	/* packet serial number */
	#define PK_CURSOR					0x0020	/* reporting cursor */
	#define PK_BUTTONS				0x0040	/* button information */
	#define PK_X						0x0080	/* x axis */
	#define PK_Y						0x0100	/* y axis */
	#define PK_Z						0x0200	/* z axis */
	#define PK_NORMAL_PRESSURE		0x0400	/* normal or tip pressure */
	#define PK_TANGENT_PRESSURE	0x0800	/* tangential or barrel pressure */
	#define PK_ORIENTATION			0x1000	/* orientation info: tilts */
	#define PK_ROTATION				0x2000	/* rotation info; 1.1 */

#endif

typedef DWORD FIX32;				/* fixed-point arithmetic type */

#ifndef NOFIX32
	#define INT(x)			HIWORD(x)
	#define FRAC(x)		LOWORD(x)

	#define CASTFIX32(x)	((FIX32)((x)*65536L))

	#define ROUND(x)		(INT(x) + (FRAC(x) > (WORD)0x8000))

	#define FIX_MUL(c, a, b)							\
		(c = (((DWORD)FRAC(a) * FRAC(b)) >> 16) +	\
			(DWORD)INT(a) * FRAC(b) +					\
			(DWORD)INT(b) * FRAC(a) +					\
			((DWORD)INT(a) * INT(b) << 16))

	#ifdef _WINDLL
		#define FIX_DIV_SC static
	#else
		#define FIX_DIV_SC
	#endif

	#define FIX_DIV(c, a, b)						\
		{													\
			FIX_DIV_SC DWORD temp, rem, btemp;	\
															\
			/* fraction done bytewise */			\
			temp = ((a / b) << 16);					\
			rem = a % b;								\
			btemp = b;									\
			if (INT(btemp) < 256) {					\
				rem <<= 8;								\
			}												\
			else {										\
				btemp >>= 8;							\
			}												\
			temp += ((rem / btemp) << 8);			\
			rem %= btemp;								\
			rem <<= 8;									\
			temp += rem / btemp;						\
			c = temp;									\
		}
#endif

/* -------------------------------------------------------------------------- */
/* INFO DATA DEFS */

#ifndef NOWTINFO

#ifndef NOWTAXIS

typedef struct tagAXIS {
	LONG	axMin;
	LONG	axMax;
	UINT	axUnits;
	FIX32	axResolution;
} AXIS, *PAXIS, NEAR *NPAXIS, FAR *LPAXIS;

	/* unit specifiers */
	#define TU_NONE			0
	#define TU_INCHES		1
	#define TU_CENTIMETERS	2
	#define TU_CIRCLE		3

#endif

#ifndef NOWTSYSBUTTONS

/* system button assignment values */
#define SBN_NONE			0x00
#define SBN_LCLICK		0x01
#define SBN_LDBLCLICK	0x02
#define SBN_LDRAG			0x03
#define SBN_RCLICK		0x04
#define SBN_RDBLCLICK	0x05
#define SBN_RDRAG			0x06
#define SBN_MCLICK		0x07
#define SBN_MDBLCLICK	0x08
#define SBN_MDRAG			0x09
/* for Pen Windows */
#define SBN_PTCLICK		0x10
#define SBN_PTDBLCLICK	0x20
#define SBN_PTDRAG		0x30
#define SBN_PNCLICK		0x40
#define SBN_PNDBLCLICK	0x50
#define SBN_PNDRAG		0x60
#define SBN_P1CLICK		0x70
#define SBN_P1DBLCLICK	0x80
#define SBN_P1DRAG		0x90
#define SBN_P2CLICK		0xA0
#define SBN_P2DBLCLICK	0xB0
#define SBN_P2DRAG		0xC0
#define SBN_P3CLICK		0xD0
#define SBN_P3DBLCLICK	0xE0
#define SBN_P3DRAG		0xF0

#endif

#ifndef NOWTCAPABILITIES

/* hardware capabilities */
#define HWC_INTEGRATED		0x0001
#define HWC_TOUCH				0x0002
#define HWC_HARDPROX			0x0004
#define HWC_PHYSID_CURSORS	0x0008 /* 1.1 */
#endif

#ifndef NOWTIFC

#ifndef NOWTCURSORS

/* cursor capabilities */
#define CRC_MULTIMODE	0x0001 /* 1.1 */
#define CRC_AGGREGATE	0x0002 /* 1.1 */
#define CRC_INVERT		0x0004 /* 1.1 */

#endif

/* info categories */
#define WTI_INTERFACE		1
	#define IFC_WINTABID			1
	#define IFC_SPECVERSION		2
	#define IFC_IMPLVERSION		3
	#define IFC_NDEVICES			4
	#define IFC_NCURSORS			5
	#define IFC_NCONTEXTS		6
	#define IFC_CTXOPTIONS		7
	#define IFC_CTXSAVESIZE		8
	#define IFC_NEXTENSIONS		9
	#define IFC_NMANAGERS		10
	#define IFC_MAX				10


#endif

#ifndef NOWTSTATUS

#define WTI_STATUS			2
	#define STA_CONTEXTS			1
	#define STA_SYSCTXS			2
	#define STA_PKTRATE			3
	#define STA_PKTDATA			4
	#define STA_MANAGERS			5
	#define STA_SYSTEM			6
	#define STA_BUTTONUSE		7
	#define STA_SYSBTNUSE		8
	#define STA_MAX				8

#endif

#ifndef NOWTDEFCONTEXT

#define WTI_DEFCONTEXT	3
#define WTI_DEFSYSCTX	4
#define WTI_DDCTXS		400 /* 1.1 */
#define WTI_DSCTXS		500 /* 1.1 */
	#define CTX_NAME			1
	#define CTX_OPTIONS		2
	#define CTX_STATUS		3
	#define CTX_LOCKS			4
	#define CTX_MSGBASE		5
	#define CTX_DEVICE		6
	#define CTX_PKTRATE		7
	#define CTX_PKTDATA		8
	#define CTX_PKTMODE		9
	#define CTX_MOVEMASK		10
	#define CTX_BTNDNMASK	11
	#define CTX_BTNUPMASK	12
	#define CTX_INORGX		13
	#define CTX_INORGY		14
	#define CTX_INORGZ		15
	#define CTX_INEXTX		16
	#define CTX_INEXTY		17
	#define CTX_INEXTZ		18
	#define CTX_OUTORGX		19
	#define CTX_OUTORGY		20
	#define CTX_OUTORGZ		21
	#define CTX_OUTEXTX		22
	#define CTX_OUTEXTY		23
	#define CTX_OUTEXTZ		24
	#define CTX_SENSX			25
	#define CTX_SENSY			26
	#define CTX_SENSZ			27
	#define CTX_SYSMODE		28
	#define CTX_SYSORGX		29
	#define CTX_SYSORGY		30
	#define CTX_SYSEXTX		31
	#define CTX_SYSEXTY		32
	#define CTX_SYSSENSX		33
	#define CTX_SYSSENSY		34
	#define CTX_MAX			34

#endif

#ifndef NOWTDEVICES

#define WTI_DEVICES		100
	#define DVC_NAME			1
	#define DVC_HARDWARE		2
	#define DVC_NCSRTYPES	3
	#define DVC_FIRSTCSR		4
	#define DVC_PKTRATE		5
	#define DVC_PKTDATA		6
	#define DVC_PKTMODE		7
	#define DVC_CSRDATA		8
	#define DVC_XMARGIN		9
	#define DVC_YMARGIN		10
	#define DVC_ZMARGIN		11
	#define DVC_X				12
	#define DVC_Y				13
	#define DVC_Z				14
	#define DVC_NPRESSURE	15
	#define DVC_TPRESSURE	16
	#define DVC_ORIENTATION	17
	#define DVC_ROTATION		18 /* 1.1 */
	#define DVC_PNPID			19 /* 1.1 */
	#define DVC_MAX			19

#endif

#ifndef NOWTCURSORS

#define WTI_CURSORS			200
	#define CSR_NAME				1
	#define CSR_ACTIVE			2
	#define CSR_PKTDATA			3
	#define CSR_BUTTONS			4
	#define CSR_BUTTONBITS		5
	#define CSR_BTNNAMES			6
	#define CSR_BUTTONMAP		7
	#define CSR_SYSBTNMAP		8
	#define CSR_NPBUTTON			9
	#define CSR_NPBTNMARKS		10
	#define CSR_NPRESPONSE		11
	#define CSR_TPBUTTON			12
	#define CSR_TPBTNMARKS		13
	#define CSR_TPRESPONSE		14
	#define CSR_PHYSID			15 /* 1.1 */
	#define CSR_MODE				16 /* 1.1 */
	#define CSR_MINPKTDATA		17 /* 1.1 */
	#define CSR_MINBUTTONS		18 /* 1.1 */
	#define CSR_CAPABILITIES	19 /* 1.1 */
	#define CSR_TYPE				20 /* 1.2 */
	#define CSR_MAX				20

#endif

#ifndef NOWTEXTENSIONS

#define WTI_EXTENSIONS	300
	#define EXT_NAME			1
	#define EXT_TAG			2
	#define EXT_MASK			3
	#define EXT_SIZE			4
	#define EXT_AXES			5
	#define EXT_DEFAULT		6
	#define EXT_DEFCONTEXT	7
	#define EXT_DEFSYSCTX	8
	#define EXT_CURSORS		9
	#define EXT_DEVICES     110 /* Allow 100 cursors */
	#define EXT_MAX			210 /* Allow 100 devices */

#endif

#endif

/* -------------------------------------------------------------------------- */
/* CONTEXT DATA DEFS */

#define LCNAMELEN		40
#define LC_NAMELEN	40
#ifdef WIN32
typedef struct tagLOGCONTEXTA {
	char	lcName[LCNAMELEN];
	UINT	lcOptions;
	UINT	lcStatus;
	UINT	lcLocks;
	UINT	lcMsgBase;
	UINT	lcDevice;
	UINT	lcPktRate;
	WTPKT	lcPktData;
	WTPKT	lcPktMode;
	WTPKT	lcMoveMask;
	DWORD	lcBtnDnMask;
	DWORD	lcBtnUpMask;
	LONG	lcInOrgX;
	LONG	lcInOrgY;
	LONG	lcInOrgZ;
	LONG	lcInExtX;
	LONG	lcInExtY;
	LONG	lcInExtZ;
	LONG	lcOutOrgX;
	LONG	lcOutOrgY;
	LONG	lcOutOrgZ;
	LONG	lcOutExtX;
	LONG	lcOutExtY;
	LONG	lcOutExtZ;
	FIX32	lcSensX;
	FIX32	lcSensY;
	FIX32	lcSensZ;
	BOOL	lcSysMode;
	int	lcSysOrgX;
	int	lcSysOrgY;
	int	lcSysExtX;
	int	lcSysExtY;
	FIX32	lcSysSensX;
	FIX32	lcSysSensY;
} LOGCONTEXTA, *PLOGCONTEXTA, NEAR *NPLOGCONTEXTA, FAR *LPLOGCONTEXTA;
typedef struct tagLOGCONTEXTW {
	WCHAR	lcName[LCNAMELEN];
	UINT	lcOptions;
	UINT	lcStatus;
	UINT	lcLocks;
	UINT	lcMsgBase;
	UINT	lcDevice;
	UINT	lcPktRate;
	WTPKT	lcPktData;
	WTPKT	lcPktMode;
	WTPKT	lcMoveMask;
	DWORD	lcBtnDnMask;
	DWORD	lcBtnUpMask;
	LONG	lcInOrgX;
	LONG	lcInOrgY;
	LONG	lcInOrgZ;
	LONG	lcInExtX;
	LONG	lcInExtY;
	LONG	lcInExtZ;
	LONG	lcOutOrgX;
	LONG	lcOutOrgY;
	LONG	lcOutOrgZ;
	LONG	lcOutExtX;
	LONG	lcOutExtY;
	LONG	lcOutExtZ;
	FIX32	lcSensX;
	FIX32	lcSensY;
	FIX32	lcSensZ;
	BOOL	lcSysMode;
	int	lcSysOrgX;
	int	lcSysOrgY;
	int	lcSysExtX;
	int	lcSysExtY;
	FIX32	lcSysSensX;
	FIX32	lcSysSensY;
} LOGCONTEXTW, *PLOGCONTEXTW, NEAR *NPLOGCONTEXTW, FAR *LPLOGCONTEXTW;
#ifdef UNICODE
typedef LOGCONTEXTW LOGCONTEXT;
typedef PLOGCONTEXTW PLOGCONTEXT;
typedef NPLOGCONTEXTW NPLOGCONTEXT;
typedef LPLOGCONTEXTW LPLOGCONTEXT;
#else
typedef LOGCONTEXTA LOGCONTEXT;
typedef PLOGCONTEXTA PLOGCONTEXT;
typedef NPLOGCONTEXTA NPLOGCONTEXT;
typedef LPLOGCONTEXTA LPLOGCONTEXT;
#endif /* UNICODE */
#else /* WIN32 */
typedef struct tagLOGCONTEXT {
	char	lcName[LCNAMELEN];
	UINT	lcOptions;
	UINT	lcStatus;
	UINT	lcLocks;
	UINT	lcMsgBase;
	UINT	lcDevice;
	UINT	lcPktRate;
	WTPKT	lcPktData;
	WTPKT	lcPktMode;
	WTPKT	lcMoveMask;
	DWORD	lcBtnDnMask;
	DWORD	lcBtnUpMask;
	LONG	lcInOrgX;
	LONG	lcInOrgY;
	LONG	lcInOrgZ;
	LONG	lcInExtX;
	LONG	lcInExtY;
	LONG	lcInExtZ;
	LONG	lcOutOrgX;
	LONG	lcOutOrgY;
	LONG	lcOutOrgZ;
	LONG	lcOutExtX;
	LONG	lcOutExtY;
	LONG	lcOutExtZ;
	FIX32	lcSensX;
	FIX32	lcSensY;
	FIX32	lcSensZ;
	BOOL	lcSysMode;
	int	lcSysOrgX;
	int	lcSysOrgY;
	int	lcSysExtX;
	int	lcSysExtY;
	FIX32	lcSysSensX;
	FIX32	lcSysSensY;
} LOGCONTEXT, *PLOGCONTEXT, NEAR *NPLOGCONTEXT, FAR *LPLOGCONTEXT;
#endif /* WIN32 */

	/* context option values */
	#define CXO_SYSTEM		0x0001
	#define CXO_PEN			0x0002
	#define CXO_MESSAGES		0x0004
	#define CXO_MARGIN		0x8000
	#define CXO_MGNINSIDE	0x4000
	#define CXO_CSRMESSAGES	0x0008 /* 1.1 */

	/* context status values */
	#define CXS_DISABLED		0x0001
	#define CXS_OBSCURED		0x0002
	#define CXS_ONTOP			0x0004

	/* context lock values */
	#define CXL_INSIZE		0x0001
	#define CXL_INASPECT		0x0002
	#define CXL_SENSITIVITY	0x0004
	#define CXL_MARGIN		0x0008
	#define CXL_SYSOUT		0x0010

/* -------------------------------------------------------------------------- */
/* EVENT DATA DEFS */

/* For packet structure definition, see pktdef.h */

/* packet status values */
#define TPS_PROXIMITY		0x0001
#define TPS_QUEUE_ERR		0x0002
#define TPS_MARGIN			0x0004
#define TPS_GRAB				0x0008
#define TPS_INVERT			0x0010 /* 1.1 */

typedef struct tagORIENTATION {
	int orAzimuth;
	int orAltitude;
	int orTwist;
} ORIENTATION, *PORIENTATION, NEAR *NPORIENTATION, FAR *LPORIENTATION;

typedef struct tagROTATION { /* 1.1 */
	int roPitch;
	int roRoll;
	int roYaw;
} ROTATION, *PROTATION, NEAR *NPROTATION, FAR *LPROTATION;
// grandfather in obsolete member names.
#define rotPitch	roPitch
#define rotRoll	roRoll
#define rotYaw		roYaw


/* relative buttons */
#define TBN_NONE	0
#define TBN_UP		1
#define TBN_DOWN	2

/* -------------------------------------------------------------------------- */
/* DEVICE CONFIG CONSTANTS */

#ifndef NOWTDEVCFG

#define WTDC_NONE		0
#define WTDC_CANCEL		1
#define WTDC_OK			2
#define WTDC_RESTART	3

#endif

/* -------------------------------------------------------------------------- */
/* HOOK CONSTANTS */

#ifndef NOWTHOOKS

#define WTH_PLAYBACK			1
#define WTH_RECORD			2

#define WTHC_GETLPLPFN		(-3)
#define WTHC_LPLPFNNEXT		(-2)
#define WTHC_LPFNNEXT		(-1)
#define WTHC_ACTION			0
#define WTHC_GETNEXT			1
#define WTHC_SKIP				2

#endif

/* -------------------------------------------------------------------------- */
/* PREFERENCE FUNCTION CONSTANTS */

#ifndef NOWTPREF

#define WTP_LPDEFAULT	((LPVOID)-1L)
#define WTP_DWDEFAULT	((DWORD)-1L)

#endif

/* -------------------------------------------------------------------------- */
/* EXTENSION TAGS AND CONSTANTS */

#ifndef NOWTEXTENSIONS

/* constants for use with pktdef.h */
#define PKEXT_ABSOLUTE	1
#define PKEXT_RELATIVE	2

/* Extension tags. */
#define WTX_OBT				0	/* Out of bounds tracking */
#define WTX_FKEYS				1	/* Function keys */
#define WTX_TILT				2	/* Raw Cartesian tilt; 1.1 */
#define WTX_CSRMASK			3	/* select input by cursor type; 1.1 */
#define WTX_XBTNMASK			4	/* Extended button mask; 1.1 */
#define WTX_EXPKEYS			5	/* ExpressKeys; 1.3 - DEPRECATED: see WTX_EXPKEYS2 */
#define WTX_TOUCHSTRIP		6	/* TouchStrips; 1.4 */
#define WTX_TOUCHRING		7	/* TouchRings; 1.4 */
#define WTX_EXPKEYS2			8	/* ExpressKeys; 1.4 */

typedef struct tagXBTNMASK {
	BYTE xBtnDnMask[32];
	BYTE xBtnUpMask[32];
} XBTNMASK;

typedef struct tagTILT { /* 1.1 */
	int tiltX;
	int tiltY;
} TILT;

typedef struct tagEXTENSIONBASE { /* 1.4 */
	HCTX		nContext;
	UINT		nStatus;
	DWORD		nTime;
	UINT		nSerialNumber;
} EXTENSIONBASE;

typedef struct tagEXPKEYSDATA { /* 1.4 */
	BYTE		nTablet;
	BYTE		nControl;
	BYTE		nLocation;
	BYTE		nReserved;
	DWORD		nState;
} EXPKEYSDATA;

typedef struct tagSLIDERDATA { /* 1.4 */
	BYTE		nTablet;
	BYTE		nControl;
	BYTE		nMode;
	BYTE		nReserved;
	DWORD		nPosition;
} SLIDERDATA;

typedef struct tagEXTPROPERTY { /* 1.4 */
	BYTE		version;				// Structure version, 0 for now
	BYTE		tabletIndex;		// 0-based index for tablet
	BYTE		controlIndex;		// 0-based index for control
	BYTE		functionIndex;		// 0-based index for control's sub-function
	WORD		propertyID;			// property ID
	WORD		reserved;			// DWORD-alignment filler
	DWORD		dataSize;			// number of bytes in data[] buffer
	BYTE		data[1];				// raw data
} EXTPROPERTY;

#define TABLET_PROPERTY_CONTROLCOUNT	 0			// UINT32: number of physical controls on tablet
#define TABLET_PROPERTY_FUNCCOUNT		 1			// UINT32: number of functions of control
#define TABLET_PROPERTY_AVAILABLE		 2			// BOOL: control/mode is available for override
#define TABLET_PROPERTY_MIN				 3			// UINT32: minimum value
#define TABLET_PROPERTY_MAX				 4			// UINT32: maximum value
#define TABLET_PROPERTY_OVERRIDE			 5			// BOOL: control is overridden
#define TABLET_PROPERTY_OVERRIDE_NAME	 6			// UTF-8: Displayable name when control is overridden
#define TABLET_PROPERTY_LOCATION			 11		// UINT32: Physical location of control (see TABLET_LOC_*)

#define TABLET_LOC_LEFT				0
#define TABLET_LOC_RIGHT			1
#define TABLET_LOC_TOP				2
#define TABLET_LOC_BOTTOM			3
#define TABLET_LOC_TRANSDUCER		4

#endif

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* Functions */

	#ifndef API
		#ifndef WINAPI
			#define API			FAR PASCAL
		#else
			#define API			WINAPI
		#endif
	#endif

#ifndef NOWTCALLBACKS

	#ifndef CALLBACK
	#define CALLBACK	FAR PASCAL
	#endif

	#ifndef NOWTMANAGERFXNS
	/* callback function types */
	typedef BOOL (WINAPI * WTENUMPROC)(HCTX, LPARAM); /* changed CALLBACK->WINAPI, 1.1 */
	typedef BOOL (WINAPI * WTCONFIGPROC)(HCTX, HWND);
	typedef LRESULT (WINAPI * WTHOOKPROC)(int, WPARAM, LPARAM);
	typedef WTHOOKPROC FAR *LPWTHOOKPROC;
	#endif

#endif


#ifndef NOWTFUNCTIONS

	#ifndef NOWTBASICFXNS
	/* BASIC FUNCTIONS */
#ifdef WIN32
	UINT API WTInfoA(UINT, UINT, LPVOID);
	#define ORD_WTInfoA						20
	UINT API WTInfoW(UINT, UINT, LPVOID);
	#define ORD_WTInfoW						1020
	#ifdef UNICODE
	#define WTInfo			WTInfoW
	#define ORD_WTInfo	ORD_WTInfoW
	#else
	#define WTInfo			WTInfoA
	#define ORD_WTInfo	ORD_WTInfoA
	#endif /* !UNICODE */
#else
	UINT API WTInfo(UINT, UINT, LPVOID);
	#define ORD_WTInfo							20
#endif
#ifdef WIN32
	HCTX API WTOpenA(HWND, LPLOGCONTEXTA, BOOL);
	#define ORD_WTOpenA							21
	HCTX API WTOpenW(HWND, LPLOGCONTEXTW, BOOL);
	#define ORD_WTOpenW							1021
	#ifdef UNICODE
	#define WTOpen			WTOpenW
	#define ORD_WTOpen	ORD_WTOpenW
	#else
	#define WTOpen			WTOpenA
	#define ORD_WTOpen	ORD_WTOpenA
	#endif /* !UNICODE */
#else
	HCTX API WTOpen(HWND, LPLOGCONTEXT, BOOL);
	#define ORD_WTOpen							21
#endif
	BOOL API WTClose(HCTX);
	#define ORD_WTClose							22
	int API WTPacketsGet(HCTX, int, LPVOID);
	#define ORD_WTPacketsGet					23
	BOOL API WTPacket(HCTX, UINT, LPVOID);
	#define ORD_WTPacket							24
	#endif

	#ifndef NOWTVISIBILITYFXNS
	/* VISIBILITY FUNCTIONS */
	BOOL API WTEnable(HCTX, BOOL);
	#define ORD_WTEnable							40
	BOOL API WTOverlap(HCTX, BOOL);
	#define ORD_WTOverlap						41
	#endif

	#ifndef NOWTCTXEDITFXNS
	/* CONTEXT EDITING FUNCTIONS */
	BOOL API WTConfig(HCTX, HWND);
	#define ORD_WTConfig							60
#ifdef WIN32
	BOOL API WTGetA(HCTX, LPLOGCONTEXTA);
	#define ORD_WTGetA							61
	BOOL API WTGetW(HCTX, LPLOGCONTEXTW);
	#define ORD_WTGetW							1061
	#ifdef UNICODE
	#define WTGet			WTGetW
	#define ORD_WTGet		ORD_WTGetW
	#else
	#define WTGet			WTGetA
	#define ORD_WTGet		ORD_WTGetA
	#endif /* !UNICODE */
#else
	BOOL API WTGet(HCTX, LPLOGCONTEXT);
	#define ORD_WTGet								61
#endif
#ifdef WIN32
	BOOL API WTSetA(HCTX, LPLOGCONTEXTA);
	#define ORD_WTSetA							62
	BOOL API WTSetW(HCTX, LPLOGCONTEXTW);
	#define ORD_WTSetW							1062
	#ifdef UNICODE
	#define WTSet			WTSetW
	#define ORD_WTSet		ORD_WTSetW
	#else
	#define WTSet			WTSetA
	#define ORD_WTSet		ORD_WTSetA
	#endif /* !UNICODE */
#else
	BOOL API WTSet(HCTX, LPLOGCONTEXT);
	#define ORD_WTSet								62
#endif
	BOOL API WTExtGet(HCTX, UINT, LPVOID);
	#define ORD_WTExtGet							63
	BOOL API WTExtSet(HCTX, UINT, LPVOID);
	#define ORD_WTExtSet							64
	BOOL API WTSave(HCTX, LPVOID);
	#define ORD_WTSave							65
	HCTX API WTRestore(HWND, LPVOID, BOOL);
	#define ORD_WTRestore						66
	#endif

	#ifndef NOWTQUEUEFXNS
	/* ADVANCED PACKET AND QUEUE FUNCTIONS */
	int API WTPacketsPeek(HCTX, int, LPVOID);
	#define ORD_WTPacketsPeek					80
	int API WTDataGet(HCTX, UINT, UINT, int, LPVOID, LPINT);
	#define ORD_WTDataGet						81
	int API WTDataPeek(HCTX, UINT, UINT, int, LPVOID, LPINT);
	#define ORD_WTDataPeek						82
#ifndef WIN32
/* OBSOLETE IN WIN32! */
	DWORD API WTQueuePackets(HCTX);
	#define ORD_WTQueuePackets					83
#endif
	int API WTQueueSizeGet(HCTX);
	#define ORD_WTQueueSizeGet					84
	BOOL API WTQueueSizeSet(HCTX, int);
	#define ORD_WTQueueSizeSet					85
	#endif

	#ifndef NOWTHMGRFXNS
	/* MANAGER HANDLE FUNCTIONS */
	HMGR API WTMgrOpen(HWND, UINT);
	#define ORD_WTMgrOpen						100
	BOOL API WTMgrClose(HMGR);
	#define ORD_WTMgrClose						101
	#endif

	#ifndef NOWTMGRCTXFXNS
	/* MANAGER CONTEXT FUNCTIONS */
	BOOL API WTMgrContextEnum(HMGR, WTENUMPROC, LPARAM);
	#define ORD_WTMgrContextEnum				120
	HWND API WTMgrContextOwner(HMGR, HCTX);
	#define ORD_WTMgrContextOwner				121
	HCTX API WTMgrDefContext(HMGR, BOOL);
	#define ORD_WTMgrDefContext				122
	HCTX API WTMgrDefContextEx(HMGR, UINT, BOOL); /* 1.1 */
	#define ORD_WTMgrDefContextEx				206
	#endif

	#ifndef NOWTMGRCONFIGFXNS
	/* MANAGER CONFIG BOX  FUNCTIONS */
	UINT API WTMgrDeviceConfig(HMGR, UINT, HWND);
	#define ORD_WTMgrDeviceConfig				140
#ifndef WIN32
/* OBSOLETE IN WIN32! */
	BOOL API WTMgrConfigReplace(HMGR, BOOL, WTCONFIGPROC);
	#define ORD_WTMgrConfigReplace			141
#endif
	#endif

	#ifndef NOWTMGRHOOKFXNS
	/* MANAGER PACKET HOOK FUNCTIONS */
#ifndef WIN32
/* OBSOLETE IN WIN32! */
	WTHOOKPROC API WTMgrPacketHook(HMGR, BOOL, int, WTHOOKPROC);
	#define ORD_WTMgrPacketHook				160
	LRESULT API WTMgrPacketHookDefProc(int, WPARAM, LPARAM, LPWTHOOKPROC);
	#define ORD_WTMgrPacketHookDefProc		161
#endif
	#endif

	#ifndef NOWTMGRPREFFXNS
	/* MANAGER PREFERENCE DATA FUNCTIONS */
	BOOL API WTMgrExt(HMGR, UINT, LPVOID);
	#define ORD_WTMgrExt							180
	BOOL API WTMgrCsrEnable(HMGR, UINT, BOOL);
	#define ORD_WTMgrCsrEnable					181
	BOOL API WTMgrCsrButtonMap(HMGR, UINT, LPBYTE, LPBYTE);
	#define ORD_WTMgrCsrButtonMap				182
	BOOL API WTMgrCsrPressureBtnMarks(HMGR, UINT, DWORD, DWORD);
	#define ORD_WTMgrCsrPressureBtnMarks	183
	BOOL API WTMgrCsrPressureResponse(HMGR, UINT, UINT FAR *, UINT FAR *);
	#define ORD_WTMgrCsrPressureResponse	184
	BOOL API WTMgrCsrExt(HMGR, UINT, UINT, LPVOID);
	#define ORD_WTMgrCsrExt						185
	#endif

/* Win32 replacements for non-portable functions. */
	#ifndef NOWTQUEUEFXNS
	/* ADVANCED PACKET AND QUEUE FUNCTIONS */
	BOOL API WTQueuePacketsEx(HCTX, UINT FAR *, UINT FAR *);
	#define ORD_WTQueuePacketsEx				200
	#endif

	#ifndef NOWTMGRCONFIGFXNS
	/* MANAGER CONFIG BOX  FUNCTIONS */
#ifdef WIN32
	BOOL API WTMgrConfigReplaceExA(HMGR, BOOL, LPSTR, LPSTR);
	#define ORD_WTMgrConfigReplaceExA		202
	BOOL API WTMgrConfigReplaceExW(HMGR, BOOL, LPWSTR, LPSTR);
	#define ORD_WTMgrConfigReplaceExW		1202
	#ifdef UNICODE
	#define WTMgrConfigReplaceEx			WTMgrConfigReplaceExW
	#define ORD_WTMgrConfigReplaceEx		ORD_WTMgrConfigReplaceExW
	#else
	#define WTMgrConfigReplaceEx			WTMgrConfigReplaceExA
	#define ORD_WTMgrConfigReplaceEx		ORD_WTMgrConfigReplaceExA
	#endif /* !UNICODE */
#else
	BOOL API WTMgrConfigReplaceEx(HMGR, BOOL, LPSTR, LPSTR);
	#define ORD_WTMgrConfigReplaceEx			202
#endif
	#endif

	#ifndef NOWTMGRHOOKFXNS
	/* MANAGER PACKET HOOK FUNCTIONS */
#ifdef WIN32
	HWTHOOK API WTMgrPacketHookExA(HMGR, int, LPSTR, LPSTR);
	#define ORD_WTMgrPacketHookExA			203
	HWTHOOK API WTMgrPacketHookExW(HMGR, int, LPWSTR, LPSTR);
	#define ORD_WTMgrPacketHookExW			1203
	#ifdef UNICODE
	#define WTMgrPacketHookEx			WTMgrPacketHookExW
	#define ORD_WTMgrPacketHookEx		ORD_WTMgrPacketHookExW
	#else
	#define WTMgrPacketHookEx			WTMgrPacketHookExA
	#define ORD_WTMgrPacketHookEx		ORD_WTMgrPacketHookExA
	#endif /* !UNICODE */
#else
	HWTHOOK API WTMgrPacketHookEx(HMGR, int, LPSTR, LPSTR);
	#define ORD_WTMgrPacketHookEx				203
#endif
	BOOL API WTMgrPacketUnhook(HWTHOOK);
	#define ORD_WTMgrPacketUnhook				204
	LRESULT API WTMgrPacketHookNext(HWTHOOK, int, WPARAM, LPARAM);
	#define ORD_WTMgrPacketHookNext			205
	#endif

	#ifndef NOWTMGRPREFFXNS
	/* MANAGER PREFERENCE DATA FUNCTIONS */
	BOOL API WTMgrCsrPressureBtnMarksEx(HMGR, UINT, UINT FAR *, UINT FAR *);
	#define ORD_WTMgrCsrPressureBtnMarksEx	201
	#endif

#endif

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif /* #define _INC_WINTAB */
