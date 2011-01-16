/***************************************************************************
 * Routines to grab the parameters from the command line:
 * All the routines except the main one, starts with GA (Get Arguments) to
 * prevent from names conflicts.
 * It is assumed in these routine that any pointer, for any type has the
 * same length (i.e. length of int pointer is equal to char pointer etc.)
 *
 * The following routines are available in this module:
 * 1. int GAGetArgs(argc, argv, CtrlStr, Variables...)
 * where argc, argv as received on entry.
 * CtrlStr is the contrl string (see below)
 * Variables are all the variables to be set according to CtrlStr.
 * Note that all the variables MUST be transfered by address.
 * return 0 on correct parsing, otherwise error number (see GetArg.h).
 * 2. GAPrintHowTo(CtrlStr)
 * Print the control string to stderr, in the correct format needed.
 * This feature is very useful in case of error during GetArgs parsing.
 * Chars equal to SPACE_CHAR are not printed (regular spaces are NOT
 * allowed, and so using SPACE_CHAR you can create space in PrintHowTo).
 * 3. GAPrintErrMsg(Error)
 * Print the error to stderr, according to Error (usually returned by
 * GAGetArgs).
 *
 * CtrlStr format:
 * The control string passed to GetArgs controls the way argv (argc) are
 * parsed. Each entry in this string must not have any spaces in it. The
 * First Entry is the name of the program which is usually ignored except
 * when GAPrintHowTo is called. All the other entries (except the last one
 * which we will come back to it later) must have the following format:
 * 1. One letter which sets the option letter.
 * 2. '!' or '%' to determines if this option is really optional ('%') or
 * it must exists ('!')...
 * 3. '-' allways.
 * 4. Alpha numeric string, usually ignored, but used by GAPrintHowTo to
 * print the meaning of this input.
 * 5. Sequences starts with '!' or '%'. Again if '!' then this sequence
 * must exists (only if its option flag is given of course), and if '%'
 * it is optional. Each sequence will continue with one or two
 * characters which defines the kind of the input:
 * a. d, x, o, u - integer is expected (decimal, hex, octal base or
 * unsigned).
 * b. D, X, O, U - long integer is expected (same as above).
 * c. f - float number is expected.
 * d. F - double number is expected.
 * e. s - string is expected.
 * f. *? - any number of '?' kind (d, x, o, u, D, X, O, U, f, F, s)
 * will match this one. If '?' is numeric, it scans until
 * none numeric input is given. If '?' is 's' then it scans
 * up to the next option or end of argv.
 *
 * If the last parameter given in the CtrlStr, is not an option (i.e. the
 * second char is not in ['!', '%'] and the third one is not '-'), all what
 * remained from argv is linked to it.
 *
 * The variables passed to GAGetArgs (starting from 4th parameter) MUST
 * match the order of the CtrlStr:
 * For each option, one integer address must be passed. This integer must
 * initialized by 0. If that option is given in the command line, it will
 * be set to one.
 * In addition, the sequences that might follow an option require the
 * following parameters to pass:
 * 1. d, x, o, u - pointer to integer (int *).
 * 2. D, X, O, U - pointer to long (long *).
 * 3. f - pointer to float (float *).
 * 4. F - pointer to double (double *).
 * 5. s - pointer to char (char *). NO allocation is needed!
 * 6. *? - TWO variables are passed for each wild request. the first
 * one is (address of) integer, and it will return number of
 * parameters actually matched this sequence, and the second
 * one is a pointer to pointer to ? (? **), and will return an
 * address to a block of pointers to ? kind, terminated with
 * NULL pointer. NO pre-allocation is needed.
 * note that these two variables are pretty like the argv/argc
 * pair...
 *
 * Examples:
 *
 * "Example1 i%-OneInteger!d s%-Strings!*s j%- k!-Float!f Files"
 * Will match: Example1 -i 77 -s String1 String2 String3 -k 88.2 File1 File2
 * or match: Example1 -s String1 -k 88.3 -i 999 -j
 * but not: Example1 -i 77 78 (option i expects one integer, k must be).
 * Note the option k must exists, and that the order of the options is not
 * not important. In the first examples File1 & File2 will match the Files
 * in the command line.
 * A call to GAPrintHowTo with this CtrlStr will print to stderr:
 * Example1 [-i OneIngeter] [-s Strings...] [-j] -k Float Files...
 *
 * Notes:
 *
 * 1. This module assumes that all the pointers to all kind of data types
 * have the same length and format, i.e. sizeof(int *) == sizeof(char *).
 *
 * Gershon Elber Ver 0.2 Mar 88
 ***************************************************************************
 * History:
 * 11 Mar 88 - Version 1.0 by Gershon Elber.
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
 
#ifdef __MSDOS__
#include <alloc.h>
#endif /* __MSDOS__ */

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#elif defined(HAVE_VARARGS_H)
#include <varargs.h>
#endif

#include <stdio.h>
#include <string.h>
#include "getarg.h"

#ifndef MYMALLOC
#define MYMALLOC    /* If no "MyMalloc" routine elsewhere define this. */
#endif

#define MAX_PARAM           100    /* maximum number of parameters allowed. */
#define CTRL_STR_MAX_LEN    1024

#define SPACE_CHAR '|'  /* The character not to print using HowTo. */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif /* TRUE */

#define ARG_OK 0

#define ISSPACE(x) ((x) <= ' ') /* Not conventional - but works fine! */

/* The two characters '%' and '!' are used in the control string: */
#define ISCTRLCHAR(x) (((x) == '%') || ((x) == '!'))

static char *GAErrorToken;  /* On error, ErrorToken is set to point to it. */
static int GATestAllSatis(char *CtrlStrCopy, char *CtrlStr, int *argc,
                          char ***argv, int *Parameters[MAX_PARAM],
                          int *ParamCount);
static int GAUpdateParameters(int *Parameters[], int *ParamCount,
                              char *Option, char *CtrlStrCopy, char *CtrlStr,
                              int *argc, char ***argv);
static int GAGetParmeters(int *Parameters[], int *ParamCount,
                          char *CtrlStrCopy, char *Option, int *argc,
                          char ***argv);
static int GAGetMultiParmeters(int *Parameters[], int *ParamCount,
                               char *CtrlStrCopy, int *argc, char ***argv);
static void GASetParamCount(char *CtrlStr, int Max, int *ParamCount);
static void GAByteCopy(char *Dst, char *Src, unsigned n);
static int GAOptionExists(int argc, char **argv);
#ifdef MYMALLOC
static char *MyMalloc(unsigned size);
#endif /* MYMALLOC */

/***************************************************************************
 * Routine to access the    command    line argument and interpret them:       
 * Return ARG_OK (0) is case of succesfull parsing, error code else...       
 **************************************************************************/
#ifdef HAVE_STDARG_H
int
GAGetArgs(int argc,
        char **argv,
        char *CtrlStr, ...) {

    int i, Error = FALSE, ParamCount = 0;
    int *Parameters[MAX_PARAM];     /* Save here parameter addresses. */
    char *Option, CtrlStrCopy[CTRL_STR_MAX_LEN];
    va_list ap;

    strcpy(CtrlStrCopy, CtrlStr);
    va_start(ap, CtrlStr);
    for (i = 1; i <= MAX_PARAM; i++)
        Parameters[i - 1] = va_arg(ap, int *);
    va_end(ap);

#elif defined(HAVE_VARARGS_H)
int GAGetArgs(va_alist)
    va_dcl
{
    va_list ap;
    int argc, i, Error = FALSE, ParamCount = 0;
    int *Parameters[MAX_PARAM];     /* Save here parameter addresses. */
    char **argv, *CtrlStr, *Option, CtrlStrCopy[CTRL_STR_MAX_LEN];

    va_start(ap);

    argc = va_arg(ap, int);
    argv = va_arg(ap, char **);
    CtrlStr = va_arg(ap, char *);

    va_end(ap);

    strcpy(CtrlStrCopy, CtrlStr);

    /* Using base address of parameters we access other parameters addr:
     * Note that me (for sure!) samples data beyond the current function
     * frame, but we accesson these set address only by demand. */
    for (i = 1; i <= MAX_PARAM; i++)
        Parameters[i - 1] = va_arg(ap, int *);
#endif /* HAVE_STDARG_H */

    --argc;
    argv++;    /* Skip the program name (first in argv/c list). */
    while (argc >= 0) {
        if (!GAOptionExists(argc, argv))
            break;    /* The loop. */
        argc--;
        Option = *argv++;
        if ((Error = GAUpdateParameters(Parameters, &ParamCount, Option,
                                        CtrlStrCopy, CtrlStr, &argc,
                                        &argv)) != FALSE)
            return Error;
    }
    /* Check for results and update trail of command line: */
    return GATestAllSatis(CtrlStrCopy, CtrlStr, &argc, &argv, Parameters,
                          &ParamCount);
}

/***************************************************************************
 * Routine to search for unsatisfied flags - simply scan the list for !- 
 * sequence. Before this scan, this routine updates the rest of the command
 * line into the last two parameters if it is requested by the CtrlStr 
 * (last item in CtrlStr is NOT an option). 
 * Return ARG_OK if all satisfied, CMD_ERR_AllSatis error else. 
 **************************************************************************/
static int
GATestAllSatis(char *CtrlStrCopy,
               char *CtrlStr,
               int *argc,
               char ***argv,
               int *Parameters[MAX_PARAM],
               int *ParamCount) {

    int i;
    static char *LocalToken = NULL;

    /* If LocalToken is not initialized - do it now. Note that this string
     * should be writable as well so we can not assign it directly.
     */
    if (LocalToken == NULL) {
        LocalToken = (char *)malloc(3);
        strcpy(LocalToken, "-?");
    }

    /* Check if last item is an option. If not then copy rest of command
     * line into it as 1. NumOfprm, 2. pointer to block of pointers.
     */
    for (i = strlen(CtrlStr) - 1; i > 0 && !ISSPACE(CtrlStr[i]); i--) ;
    if (!ISCTRLCHAR(CtrlStr[i + 2])) {
        GASetParamCount(CtrlStr, i, ParamCount); /* Point in correct param. */
        *Parameters[(*ParamCount)++] = *argc;
        GAByteCopy((char *)Parameters[(*ParamCount)++], (char *)argv,
                   sizeof(char *));
    }

    i = 0;
    while (++i < (int)strlen(CtrlStrCopy))
        if ((CtrlStrCopy[i] == '-') && (CtrlStrCopy[i - 1] == '!')) {
            GAErrorToken = LocalToken;
            LocalToken[1] = CtrlStrCopy[i - 2];    /* Set the correct flag. */
            return CMD_ERR_AllSatis;
        }

    return ARG_OK;
}

/***************************************************************************
 * Routine to update the parameters according to the given Option:
 **************************************************************************/
static int
GAUpdateParameters(int *Parameters[],
                   int *ParamCount,
                   char *Option,
                   char *CtrlStrCopy,
                   char *CtrlStr,
                   int *argc,
                   char ***argv) {

    int i, BooleanTrue = Option[2] != '-';

    if (Option[0] != '-') {
        GAErrorToken = Option;
        return CMD_ERR_NotAnOpt;
    }
    i = 0;    /* Scan the CtrlStrCopy for that option: */
    while (i + 2 < (int)strlen(CtrlStrCopy)) {
        if ((CtrlStrCopy[i] == Option[1]) && (ISCTRLCHAR(CtrlStrCopy[i + 1]))
            && (CtrlStrCopy[i + 2] == '-')) {
            /* We found that option! */
            break;
        }
        i++;
    }
    if (i + 2 >= (int)strlen(CtrlStrCopy)) {
        GAErrorToken = Option;
        return CMD_ERR_NoSuchOpt;
    }

    /* If we are here, then we found that option in CtrlStr - Strip it off: */
    CtrlStrCopy[i] = CtrlStrCopy[i + 1] = CtrlStrCopy[i + 2] = (char)' ';
    GASetParamCount(CtrlStr, i, ParamCount); /* Set it to point in
                                                correct prm. */
    i += 3;
    /* Set boolean flag for that option. */
    *Parameters[(*ParamCount)++] = BooleanTrue;
    if (ISSPACE(CtrlStrCopy[i]))
        return ARG_OK;    /* Only a boolean flag is needed. */

    /* Skip the text between the boolean option and data follows: */
    while (!ISCTRLCHAR(CtrlStrCopy[i]))
        i++;
    /* Get the parameters and return the appropriete return code: */
    return GAGetParmeters(Parameters, ParamCount, &CtrlStrCopy[i],
                          Option, argc, argv);
}

/***************************************************************************
 * Routine to get parameters according to the CtrlStr given from argv/c :
 **************************************************************************/
static int
GAGetParmeters(int *Parameters[],
               int *ParamCount,
               char *CtrlStrCopy,
               char *Option,
               int *argc,
               char ***argv) {

    int i = 0, ScanRes;

    while (!(ISSPACE(CtrlStrCopy[i]))) {
        switch (CtrlStrCopy[i + 1]) {
          case 'd':    /* Get signed integers. */
              ScanRes = sscanf(*((*argv)++), "%d",
                               (int *)Parameters[(*ParamCount)++]);
              break;
          case 'u':    /* Get unsigned integers. */
              ScanRes = sscanf(*((*argv)++), "%u",
                               (unsigned *)Parameters[(*ParamCount)++]);
              break;
          case 'x':    /* Get hex integers. */
              ScanRes = sscanf(*((*argv)++), "%x",
                               (unsigned int *)Parameters[(*ParamCount)++]);
              break;
          case 'o':    /* Get octal integers. */
              ScanRes = sscanf(*((*argv)++), "%o",
                               (unsigned int *)Parameters[(*ParamCount)++]);
              break;
          case 'D':    /* Get signed long integers. */
              ScanRes = sscanf(*((*argv)++), "%ld",
                               (long *)Parameters[(*ParamCount)++]);
              break;
          case 'U':    /* Get unsigned long integers. */
              ScanRes = sscanf(*((*argv)++), "%lu",
                               (unsigned long *)Parameters[(*ParamCount)++]);
              break;
          case 'X':    /* Get hex long integers. */
              ScanRes = sscanf(*((*argv)++), "%lx",
                               (unsigned long *)Parameters[(*ParamCount)++]);
              break;
          case 'O':    /* Get octal long integers. */
              ScanRes = sscanf(*((*argv)++), "%lo",
                               (unsigned long *)Parameters[(*ParamCount)++]);
              break;
          case 'f':    /* Get float number. */
              ScanRes = sscanf(*((*argv)++), "%f",
                               (float *)Parameters[(*ParamCount)++]);
          case 'F':    /* Get double float number. */
              ScanRes = sscanf(*((*argv)++), "%lf",
                               (double *)Parameters[(*ParamCount)++]);
              break;
          case 's':    /* It as a string. */
              ScanRes = 1;    /* Allways O.K. */
              GAByteCopy((char *)Parameters[(*ParamCount)++],
                         (char *)((*argv)++), sizeof(char *));
              break;
          case '*':    /* Get few parameters into one: */
              ScanRes = GAGetMultiParmeters(Parameters, ParamCount,
                                            &CtrlStrCopy[i], argc, argv);
              if ((ScanRes == 0) && (CtrlStrCopy[i] == '!')) {
                  GAErrorToken = Option;
                  return CMD_ERR_WildEmpty;
              }
              break;
          default:
              ScanRes = 0;    /* Make optimizer warning silent. */
        }
        /* If reading fails and this number is a must (!) then error: */
        if ((ScanRes == 0) && (CtrlStrCopy[i] == '!')) {
            GAErrorToken = Option;
            return CMD_ERR_NumRead;
        }
        if (CtrlStrCopy[i + 1] != '*') {
            (*argc)--;    /* Everything is OK - update to next parameter: */
            i += 2;    /* Skip to next parameter (if any). */
        } else
            i += 3;    /* Skip the '*' also! */
    }

    return ARG_OK;
}

/***************************************************************************
 * Routine to get few parameters into one pointer such that the returned
 * pointer actually points on a block of pointers to the parameters...
 * For example *F means a pointer to pointers on floats.
 * Returns number of parameters actually read.
 * This routine assumes that all pointers (on any kind of scalar) has the
 * same size (and the union below is totally ovelapped bteween dif. arrays)
***************************************************************************/
static int
GAGetMultiParmeters(int *Parameters[],
                    int *ParamCount,
                    char *CtrlStrCopy,
                    int *argc,
                    char ***argv) {

    int i = 0, ScanRes, NumOfPrm = 0, **Pmain, **Ptemp;
    union TmpArray {    /* Save here the temporary data before copying it to */
        int *IntArray[MAX_PARAM];    /* the returned pointer block. */
        long *LngArray[MAX_PARAM];
        float *FltArray[MAX_PARAM];
        double *DblArray[MAX_PARAM];
        char *ChrArray[MAX_PARAM];
    } TmpArray;

    do {
        switch (CtrlStrCopy[2]) { /* CtrlStr == '!*?' or '%*?' where ? is. */
          case 'd':    /* Format to read the parameters: */
              TmpArray.IntArray[NumOfPrm] = (int *)MyMalloc(sizeof(int));
              ScanRes = sscanf(*((*argv)++), "%d",
                               (int *)TmpArray.IntArray[NumOfPrm++]);
              break;
          case 'u':
              TmpArray.IntArray[NumOfPrm] = (int *)MyMalloc(sizeof(int));
              ScanRes = sscanf(*((*argv)++), "%u",
                               (unsigned int *)TmpArray.IntArray[NumOfPrm++]);
              break;
          case 'o':
              TmpArray.IntArray[NumOfPrm] = (int *)MyMalloc(sizeof(int));
              ScanRes = sscanf(*((*argv)++), "%o",
                               (unsigned int *)TmpArray.IntArray[NumOfPrm++]);
              break;
          case 'x':
              TmpArray.IntArray[NumOfPrm] = (int *)MyMalloc(sizeof(int));
              ScanRes = sscanf(*((*argv)++), "%x",
                               (unsigned int *)TmpArray.IntArray[NumOfPrm++]);
              break;
          case 'D':
              TmpArray.LngArray[NumOfPrm] = (long *)MyMalloc(sizeof(long));
              ScanRes = sscanf(*((*argv)++), "%ld",
                               (long *)TmpArray.IntArray[NumOfPrm++]);
              break;
          case 'U':
              TmpArray.LngArray[NumOfPrm] = (long *)MyMalloc(sizeof(long));
              ScanRes = sscanf(*((*argv)++), "%lu",
                               (unsigned long *)TmpArray.
                               IntArray[NumOfPrm++]);
              break;
          case 'O':
              TmpArray.LngArray[NumOfPrm] = (long *)MyMalloc(sizeof(long));
              ScanRes = sscanf(*((*argv)++), "%lo",
                               (unsigned long *)TmpArray.
                               IntArray[NumOfPrm++]);
              break;
          case 'X':
              TmpArray.LngArray[NumOfPrm] = (long *)MyMalloc(sizeof(long));
              ScanRes = sscanf(*((*argv)++), "%lx",
                               (unsigned long *)TmpArray.
                               IntArray[NumOfPrm++]);
              break;
          case 'f':
              TmpArray.FltArray[NumOfPrm] = (float *)MyMalloc(sizeof(float));
              ScanRes = sscanf(*((*argv)++), "%f",
                               (float *)TmpArray.LngArray[NumOfPrm++]);
              break;
          case 'F':
              TmpArray.DblArray[NumOfPrm] =
                 (double *)MyMalloc(sizeof(double));
              ScanRes = sscanf(*((*argv)++), "%lf",
                               (double *)TmpArray.LngArray[NumOfPrm++]);
              break;
          case 's':
              while ((*argc) && ((**argv)[0] != '-')) {
                  TmpArray.ChrArray[NumOfPrm++] = *((*argv)++);
                  (*argc)--;
              }
              ScanRes = 0;    /* Force quit from do - loop. */
              NumOfPrm++;    /* Updated again immediately after loop! */
              (*argv)++;    /* "" */
              break;
          default:
              ScanRes = 0;    /* Make optimizer warning silent. */
        }
        (*argc)--;
    }
    while (ScanRes == 1);    /* Exactly one parameter was read. */
    (*argv)--;
    NumOfPrm--;
    (*argc)++;

    /* Now allocate the block with the exact size, and set it: */
    Ptemp = Pmain =
       (int **)MyMalloc((unsigned)(NumOfPrm + 1) * sizeof(int *));
    /* And here we use the assumption that all pointers are the same: */
    for (i = 0; i < NumOfPrm; i++)
        *Ptemp++ = TmpArray.IntArray[i];
    *Ptemp = NULL;    /* Close the block with NULL pointer. */

    /* That it save the number of parameters read as first parameter to
     * return and the pointer to the block as second, and return: */
    *Parameters[(*ParamCount)++] = NumOfPrm;
    GAByteCopy((char *)Parameters[(*ParamCount)++], (char *)&Pmain,
               sizeof(char *));
    return NumOfPrm;
}

/***************************************************************************
 * Routine to scan the CtrlStr, upto Max and count the number of parameters
 * to that point:
 * 1. Each option is counted as one parameter - boolean variable (int)
 * 2. Within an option, each %? or !? is counted once - pointer to something
 * 3. Within an option, %*? or !*? is counted twice - one for item count
 * and one for pointer to block pointers.
 * Note ALL variables are passed by address and so of fixed size (address).
 **************************************************************************/
static void
GASetParamCount(char *CtrlStr,
                int Max,
                int *ParamCount) {
    int i;

    *ParamCount = 0;
    for (i = 0; i < Max; i++)
        if (ISCTRLCHAR(CtrlStr[i])) {
            if (CtrlStr[i + 1] == '*')
                *ParamCount += 2;
            else
                (*ParamCount)++;
        }
}

/***************************************************************************
 * Routine to copy exactly n bytes from Src to Dst. Note system library
 * routine strncpy should do the same, but it stops on NULL char !
 **************************************************************************/
static void
GAByteCopy(char *Dst,
           char *Src,
           unsigned n) {

    while (n--)
        *(Dst++) = *(Src++);
}

/***************************************************************************
 * Routine to check if more option (i.e. first char == '-') exists in the
 * given list argc, argv:
 **************************************************************************/
static int
GAOptionExists(int argc,
               char **argv) {

    while (argc--)
        if ((*argv++)[0] == '-')
            return TRUE;
    return FALSE;
}

/***************************************************************************
 * Routine to print some error messages, for this module:
 **************************************************************************/
void
GAPrintErrMsg(int Error) {

    fprintf(stderr, "Error in command line parsing - ");
    switch (Error) {
      case 0:;
          fprintf(stderr, "Undefined error");
          break;
      case CMD_ERR_NotAnOpt:
          fprintf(stderr, "None option Found");
          break;
      case CMD_ERR_NoSuchOpt:
          fprintf(stderr, "Undefined option Found");
          break;
      case CMD_ERR_WildEmpty:
          fprintf(stderr, "Empty input for '!*?' seq.");
          break;
      case CMD_ERR_NumRead:
          fprintf(stderr, "Failed on reading number");
          break;
      case CMD_ERR_AllSatis:
          fprintf(stderr, "Fail to satisfy");
          break;
    }
    fprintf(stderr, " - '%s'.\n", GAErrorToken);
}

/***************************************************************************
 * Routine to print correct format of command line allowed:
 **************************************************************************/
void
GAPrintHowTo(char *CtrlStr) {

    int i = 0, SpaceFlag;

    fprintf(stderr, "Usage: ");
    /* Print program name - first word in ctrl. str. (optional): */
    while (!(ISSPACE(CtrlStr[i])) && (!ISCTRLCHAR(CtrlStr[i + 1])))
        fprintf(stderr, "%c", CtrlStr[i++]);

    while (i < (int)strlen(CtrlStr)) {
        while ((ISSPACE(CtrlStr[i])) && (i < (int)strlen(CtrlStr)))
            i++;
        switch (CtrlStr[i + 1]) {
          case '%':
              fprintf(stderr, " [-%c", CtrlStr[i++]);
              i += 2;    /* Skip the '%-' or '!- after the char! */
              SpaceFlag = TRUE;
              while (!ISCTRLCHAR(CtrlStr[i]) && (i < (int)strlen(CtrlStr)) &&
                     (!ISSPACE(CtrlStr[i])))
                  if (SpaceFlag) {
                      if (CtrlStr[i++] == SPACE_CHAR)
                          fprintf(stderr, " ");
                      else
                          fprintf(stderr, " %c", CtrlStr[i - 1]);
                      SpaceFlag = FALSE;
                  } else if (CtrlStr[i++] == SPACE_CHAR)
                      fprintf(stderr, " ");
                  else
                      fprintf(stderr, "%c", CtrlStr[i - 1]);
              while (!ISSPACE(CtrlStr[i]) && (i < (int)strlen(CtrlStr))) {
                  if (CtrlStr[i] == '*')
                      fprintf(stderr, "...");
                  i++;    /* Skip the rest of it. */
              }
              fprintf(stderr, "]");
              break;
          case '!':
              fprintf(stderr, " -%c", CtrlStr[i++]);
              i += 2;    /* Skip the '%-' or '!- after the char! */
              SpaceFlag = TRUE;
              while (!ISCTRLCHAR(CtrlStr[i]) && (i < (int)strlen(CtrlStr)) &&
                     (!ISSPACE(CtrlStr[i])))
                  if (SpaceFlag) {
                      if (CtrlStr[i++] == SPACE_CHAR)
                          fprintf(stderr, " ");
                      else
                          fprintf(stderr, " %c", CtrlStr[i - 1]);
                      SpaceFlag = FALSE;
                  } else if (CtrlStr[i++] == SPACE_CHAR)
                      fprintf(stderr, " ");
                  else
                      fprintf(stderr, "%c", CtrlStr[i - 1]);
              while (!ISSPACE(CtrlStr[i]) && (i < (int)strlen(CtrlStr))) {
                  if (CtrlStr[i] == '*')
                      fprintf(stderr, "...");
                  i++;    /* Skip the rest of it. */
              }
              break;
          default:    /* Not checked, but must be last one! */
              fprintf(stderr, " ");
              while (!ISSPACE(CtrlStr[i]) && (i < (int)strlen(CtrlStr)) &&
                     !ISCTRLCHAR(CtrlStr[i]))
                  fprintf(stderr, "%c", CtrlStr[i++]);
              fprintf(stderr, "\n");
              return;
        }
    }
    fprintf(stderr, "\n");
}

#ifdef MYMALLOC

/***************************************************************************
 * My Routine to allocate dynamic memory. All program requests must call
 * this routine (no direct call to malloc). Dies if no memory.
 **************************************************************************/
static char *
MyMalloc(unsigned size) {

    char *p;

    if ((p = (char *)malloc(size)) != NULL)
        return p;

    fprintf(stderr, "Not enough memory, exit.\n");
    exit(2);

    return NULL;    /* Makes warning silent. */
}

#endif /* MYMALLOC */
