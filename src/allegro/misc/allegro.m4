# Configure paths for Allegro
# Shamelessly stolen from libxml.a4
# Jon Rafkind 2004-06-06
# Adapted from:
# Configure paths for libXML
# Toshio Kuratomi 2001-04-21
# Adapted from:
# Configure paths for GLIB
# Owen Taylor     97-11-3

dnl AM_PATH_allegro([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for allegro, and define allegro_CFLAGS and allegro_LIBS
dnl
AC_DEFUN([AM_PATH_ALLEGRO],[ 
AC_ARG_WITH(allegro-prefix,
            [  --with-allegro-prefix=PFX   Prefix where liballegro is installed (optional)],
            ALLEGRO_CONFIG_prefix="$withval", ALLEGRO_CONFIG_prefix="")
AC_ARG_WITH(allegro-exec-prefix,
            [  --with-allegro-exec-prefix=PFX Exec prefix where liballegro is installed (optional)],
            ALLEGRO_CONFIG_exec_prefix="$withval", ALLEGRO_CONFIG_exec_prefix="")
AC_ARG_ENABLE(allegrotest,
              [  --disable-allegrotest       Do not try to compile and run a test LIBallegro program],,
              enable_allegrotest=yes)

  if test x$ALLEGRO_CONFIG_exec_prefix != x ; then
     ALLEGRO_CONFIG_args="$ALLEGRO_CONFIG_args --exec-prefix=$ALLEGRO_CONFIG_exec_prefix"
     if test x${ALLEGRO_CONFIG+set} != xset ; then
        ALLEGRO_CONFIG=$ALLEGRO_CONFIG_exec_prefix/bin/allegro-config
     fi
  fi
  if test x$ALLEGRO_CONFIG_prefix != x ; then
     ALLEGRO_CONFIG_args="$ALLEGRO_CONFIG_args --prefix=$ALLEGRO_CONFIG_prefix"
     if test x${ALLEGRO_CONFIG+set} != xset ; then
        ALLEGRO_CONFIG=$ALLEGRO_CONFIG_prefix/bin/allegro-config
     fi
  fi

  AC_PATH_PROG(ALLEGRO_CONFIG, allegro-config, no)
  min_allegro_version=ifelse([$1], ,4.0.0,[$1])
  AC_MSG_CHECKING(for Allegro - version >= $min_allegro_version)
  no_allegro=""
  if test "$ALLEGRO_CONFIG" = "no" ; then
    no_allegro=yes
  else
    allegro_CFLAGS=`$ALLEGRO_CONFIG $ALLEGRO_CONFIG_args --cflags`
    allegro_LIBS=`$ALLEGRO_CONFIG $ALLEGRO_CONFIG_args --libs`
    ALLEGRO_CONFIG_major_version=`$ALLEGRO_CONFIG $ALLEGRO_CONFIG_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    ALLEGRO_CONFIG_minor_version=`$ALLEGRO_CONFIG $ALLEGRO_CONFIG_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    ALLEGRO_CONFIG_micro_version=`$ALLEGRO_CONFIG $ALLEGRO_CONFIG_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_allegrotest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $allegro_CFLAGS"
      LIBS="$allegro_LIBS $LIBS"
dnl
dnl Now check if the installed liballegro is sufficiently new.
dnl (Also sanity checks the results of allegro-config to some extent)
dnl
      rm -f conf.allegrotest
      AC_TRY_RUN([
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <allegro.h>

int 
main()
{
  int allegro_major_version, allegro_minor_version, allegro_micro_version;
  int major, minor, micro;
  char *tmp_version;
  int tmp_int_version;

  system("touch conf.allegrotest");

  /* Capture allegro-config output via autoconf/configure variables */
  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = (char *)strdup("$min_allegro_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string from allegro-config\n", "$min_allegro_version");
     free(tmp_version);
     exit(1);
   }
   free(tmp_version);

   /* Capture the version information from the header files */
   allegro_major_version = ALLEGRO_VERSION;
   allegro_minor_version = ALLEGRO_SUB_VERSION;
   allegro_micro_version = ALLEGRO_WIP_VERSION;

 /* Compare allegro-config output to the Allegro headers */
  if ((allegro_major_version != $ALLEGRO_CONFIG_major_version) ||
      (allegro_minor_version != $ALLEGRO_CONFIG_minor_version))
	  
    {
      printf("*** Allegro header files (version %d.%d.%d) do not match\n",
         allegro_major_version, allegro_minor_version, allegro_micro_version);
      printf("*** allegro-config (version %d.%d.%d)\n",
         $ALLEGRO_CONFIG_major_version, $ALLEGRO_CONFIG_minor_version, $ALLEGRO_CONFIG_micro_version);
      return 1;
    } 
/* Compare the headers to the library to make sure we match */
  /* Less than ideal -- doesn't provide us with return value feedback, 
   * only exits if there's a serious mismatch between header and library.
   */
   	/* TODO:
	 * This doesnt work!
	 */
    /* ALLEGRO_TEST_VERSION; */

    /* Test that the library is greater than our minimum version */
    if (($ALLEGRO_CONFIG_major_version > major) ||
        (($ALLEGRO_CONFIG_major_version == major) && ($ALLEGRO_CONFIG_minor_version > minor)) ||
        (($ALLEGRO_CONFIG_major_version == major) && ($ALLEGRO_CONFIG_minor_version == minor) &&
        ($ALLEGRO_CONFIG_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of Allegro (%d.%d.%d) was found.\n",
               allegro_major_version, allegro_minor_version, allegro_micro_version);
        printf("*** You need a version of Allegro newer than %d.%d.%d. The latest version of\n",
           major, minor, micro);
        printf("*** Allegro is always available from http://alleg.sf.net.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the allegro-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of Allegro, but you can also set the ALLEGRO_CONFIG environment to point to the\n");
        printf("*** correct copy of allegro-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
    }
  return 1;
} END_OF_MAIN()
],, no_allegro=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi

  if test "x$no_allegro" = x ; then
     AC_MSG_RESULT(yes (version $ALLEGRO_CONFIG_major_version.$ALLEGRO_CONFIG_minor_version.$ALLEGRO_CONFIG_micro_version))
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$ALLEGRO_CONFIG" = "no" ; then
       echo "*** The allegro-config script installed by Allegro could not be found"
       echo "*** If Allegro was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the ALLEGRO_CONFIG environment variable to the"
       echo "*** full path to allegro-config."
     else
       if test -f conf.allegrotest ; then
        :
       else
          echo "*** Could not run Allegro test program, checking why..."
          CFLAGS="$CFLAGS $allegro_CFLAGS"
          LIBS="$LIBS $allegro_LIBS"
          AC_TRY_LINK([
#include <allegro.h>
#include <stdio.h>
],      [ allegro_error; return 0;],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding Allegro or finding the wrong"
          echo "*** version of Allegro. If it is not finding Allegro, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means Allegro was incorrectly installed"
          echo "*** or that you have moved Allegro since it was installed. In the latter case, you"
          echo "*** may want to edit the allegro-config script: $ALLEGRO_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi

     allegro_CFLAGS=""
     allegro_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(allegro_CFLAGS)
  AC_SUBST(allegro_LIBS)
  rm -f conf.allegrotest
])
