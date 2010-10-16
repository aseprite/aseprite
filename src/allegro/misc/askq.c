/*
 *  ASKQ - asks a yes/no question, returning an exit status to the caller.
 *  This is used by the msvcmake installation batch file.
 */


#include <stdio.h>
#include <ctype.h>



int main(int argc, char *argv[])
{
   int i;

   puts("\n");

   for (i=1; i<argc; i++) {
      if (i > 1)
	 putc(' ', stdout);

      puts(argv[i]);
   }

   puts("? [y/n] ");

   for (;;) {
      i = getc(stdin);

      if ((tolower(i) == 'y') || (tolower(i) == 'n')) {
	 putc(i, stdout);
	 puts("\n\n");
	 return (tolower(i) == 'y') ? 0 : 1;
      }
      else
	 putc(7, stdout);
   }
}
