#include <allegro.h>

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#ifdef ALLEGRO_BIG_ENDIAN
#  define WORDS_BIGENDIAN
#else
#  undef WORDS_BIGENDIAN
#endif

/* Name of package */
#define PACKAGE "libart_lgpl"

/* Version number of package */
#define VERSION "2.3.3"
