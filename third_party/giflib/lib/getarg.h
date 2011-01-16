/***************************************************************************
 * Error numbers as returned by GAGetArg routine:
 *
 * Gershon Elber Mar 88
 ***************************************************************************
 * History:
 * 11 Mar 88 - Version 1.0 by Gershon Elber.
 **************************************************************************/

#ifndef _GETARG_H
#define _GETARG_H

#define CMD_ERR_NotAnOpt  1    /* None Option found. */
#define CMD_ERR_NoSuchOpt 2    /* Undefined Option Found. */
#define CMD_ERR_WildEmpty 3    /* Empty input for !*? seq. */
#define CMD_ERR_NumRead   4    /* Failed on reading number. */
#define CMD_ERR_AllSatis  5    /* Fail to satisfy (must-'!') option. */

#ifdef HAVE_STDARG_H
int GAGetArgs(int argc, char **argv, char *CtrlStr, ...);
#elif defined (HAVE_VARARGS_H)
int GAGetArgs(int va_alist, ...);
#endif /* HAVE_STDARG_H */

void GAPrintErrMsg(int Error);
void GAPrintHowTo(char *CtrlStr);

#endif /* _GETARG_H */
