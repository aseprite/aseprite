/* 
 *    Silly little bodge for getting the MinGW compiler
 *    to handle "response files" a la Watcom/Microsoft.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
   char var[4096], *p;
   FILE *file;
   int i,c;

   p = var;
   *p = '\0';

   for (i=1; i < argc; i++) {
      if (argv[i][0] == '@') {
         if ((file=fopen(argv[i]+1, "r")) == NULL) {
            fprintf(stderr, "runnergw: Unable to open '%s'.\n", argv[i]);
            exit(EXIT_FAILURE);
         }

         while ((c=fgetc(file)) != EOF) {
            if (c != '\n')
               *p++ = c;
         }

         fclose(file);
      }
      else {
         strncat(var, argv[i], sizeof(var)-1);
         p = var + strlen(var);
      }

      *p++ = ' ';
      *p = '\0';
   }

   return system(var);
}
