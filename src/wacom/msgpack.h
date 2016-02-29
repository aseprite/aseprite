/* ------------------------------- msgpack.h -------------------------------- */
/*------------------------------------------------------------------------------
Selected message unpacking macros from windowsx.h
to circumvent compile-time memory headaches.

The text and information contained in this file may be freely used,
copied, or distributed without compensation or licensing restrictions.

This file is Copyright (c) Wacom Company, Ltd. 2010 All Rights Reserved
with portions copyright 1991-1998 by LCS/Telegraphics.
------------------------------------------------------------------------------*/
#ifdef WIN32
#define GET_WM_ACTIVATE_STATE(wp, lp)           LOWORD(wp)
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
#define FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, fn) \
    (void)(fn)((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))
/* -------------------------------------------------------------------------- */
#else
#define GET_WM_ACTIVATE_STATE(wp, lp)               (wp)
#define GET_WM_COMMAND_ID(wp, lp)                   (wp)
#define GET_WM_COMMAND_HWND(wp, lp)                 (HWND)LOWORD(lp)
#define GET_WM_COMMAND_CMD(wp, lp)                  HIWORD(lp)
#define FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, fn) \
    (void)(fn)((hwnd), WM_COMMAND, (WPARAM)(int)(id), MAKELPARAM((UINT)(hwndCtl), (codeNotify)))
/* -------------------------------------------------------------------------- */
#endif
