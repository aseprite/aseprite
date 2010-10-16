/* 
 *    Silly little bodge for getting GNU make to pass long commands
 *    to broken programs like the Microsoft and Watcom linkers. This
 *    tool is built with gcc, and invoked using GNU make. It echoes
 *    the arguments into a temporary file, and then passes that as a 
 *    script to the utility in question. Ugly, but it does the job.
 *    An @ symbol marks that all commands from here on should go in
 *    the argument file, and a \ character indicates to convert slashes
 *    from / to \ format.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>



char **__crt0_glob_function(char *_arg)
{
   /* don't let djgpp glob our command line arguments */
   return NULL;
}



int main(int argc, char *argv[])
{
   char buf[256] = "";
   FILE *f = NULL;
   int flip_slashes = 0;
   int ret, i, j;
   char *p;

   if (argc < 2) {
      printf("Usage: runner program args\n");
      return 1;
   }

   for (i=1; i<argc; i++) {
      if ((strcmp(argv[i], "\\") == 0) || (strcmp(argv[i], "\\\\") == 0)) {
	 flip_slashes = 1;
      }
      else if (strcmp(argv[i], "@") == 0) {
	 if (!f) {
	    f = fopen("_tmpfile.arg", "w");

	    if (!f) {
	       printf("runner: Error writing _tmpfile.arg\n");
	       return 1;
	    }
	 }
      }
      else if (f) {
	 if (flip_slashes) {
	    for (j=0; argv[i][j]; j++) {
	       if (argv[i][j] == '/')
		  fputc('\\', f);
	       else
		  fputc(argv[i][j], f);
	    }
	    fputc('\n', f);
	 }
	 else
	    fprintf(f, "%s\n", argv[i]);
      }
      else {
	 if (buf[0])
	    strncat(buf, " ", sizeof(buf)-1);

	 if (flip_slashes) {
	    j = strlen(buf);
	    strncat(buf, argv[i], sizeof(buf)-1);
	    while (buf[j]) {
	       if (buf[j] == '/')
		  buf[j] = '\\';
	       j++;
	    }
	 }
	 else
	    strncat(buf, argv[i], sizeof(buf)-1);
      }
   }

   if (f) {
      fclose(f);
      strncat(buf, " @_tmpfile.arg", sizeof(buf)-1);
   }

   p = strchr(buf, ' ');
   if (p) {
      if (strlen(p) >= 126) {
	 fprintf(stderr, "runner: oops: command line is longer than 126 characters!\n");
	 remove("_tmpfile.arg");
	 return 1; 
      }
   }

   ret = system(buf);

   remove("_tmpfile.arg");

   return ret;
}

