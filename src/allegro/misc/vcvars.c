/*
 *  VCVARS - sets up a command prompt with suitable options for using MSVC.
 *
 *  This program tries to locate the vcvars32.bat file, first by looking in
 *  the registry, and then if that fails, by asking the user. It then sets
 *  the environment variable VCVARS to the full path of this file.
 *
 *  If it was given a commandline argument, it then runs that program,
 *  or otherwise it opens up a new command prompt. In either case it will
 *  adjust the environment size to make sure there is enough space for
 *  setting the compiler variables.
 */


#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>



/* try to read a value from the registry */
int read_registry(char *path, char *var, char *val, int size)
{
   HKEY key;

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &key) != ERROR_SUCCESS)
      return 0;

   if (RegQueryValueEx(key, var, NULL, NULL, val, &size) != ERROR_SUCCESS) {
      RegCloseKey(key);
      return 0;
   }

   RegCloseKey(key);
   return 1;
}



/* locate vcvars32.bat */
void find_vcvars()
{
   char data[256], name[256];
   int i;

   /* look in the registry (try VC versions 4 through 9, just in case :-) */
   for (i=9; i>=4; i--) {
      sprintf(name, "Software\\Microsoft\\DevStudio\\%d.0\\Products\\Microsoft Visual C++", i);

      if (read_registry(name, "ProductDir", data, sizeof(data))) {
	 strcat(data, "\\bin\\vcvars32.bat");

	 if (access(data, 4) == 0) {
	    printf("Found %s\n", data);
	    break;
	 }
      }

      data[0] = 0;
   }

   /* oh dear, have to ask the user where they put it */
   if (!data[0]) {
      printf("\n  Unable to find MSVC ProductDir information in your registry!\n\n");
      printf("  To install Allegro, I need to know the path where your compiler is\n");
      printf("  installed. Somewhere in your MSVC installation directory there will\n");
      printf("  be a file called vcvars32.bat, which contains this information.\n");
      printf("  Please enter the full path to where I can find that file, for example\n");
      printf("  c:\\Program Files\\Microsoft Visual Studio\\VC98\\bin\\vcvars32.bat\n");

      for (;;) {
	 printf("\n> ");
	 fflush(stdout);

	 if (gets(data)) {
	    i = strlen(data) - 12;
	    if (i < 0)
	       i = 0;

	    if (stricmp(data+i, "vcvars32.bat") != 0)
	       printf("\nError: that path doesn't end in vcvars32.bat!\n");
	    else if (access(data, 4) != 0)
	       printf("\nError: can't find a vcvars32.bat file there!\n");
	    else {
	       printf("\nUsing %s\n", data);
	       break;
	    }
	 }

	 data[0] = 0;
      }
   }

   /* put it in the environment */
   strcpy(name, "VCVARS=");
   strcat(name, data);

   putenv(name);
}



/* the main program */
int main(int argc, char *argv[])
{
   char cmd[256];
   int i;

   find_vcvars();

   if ((getenv("OS")) && (stricmp(getenv("OS"), "Windows_NT") == 0)) {
      if (argc > 1) {
	 /* run program using cmd.exe */
	 strcpy(cmd, "cmd.exe /e:8192 /c");
      }
      else {
	 /* TSR using cmd.exe */
	 sprintf(cmd, "cmd.exe /e:8192 /k \"%s\"", getenv("VCVARS"));
      }
   }
   else {
      if (argc > 1) {
	 /* run program using command.com */
	 strcpy(cmd, "command.com /e:8192 /c");
      }
      else {
	 /* TSR using command.com */
	 sprintf(cmd, "command.com /e:8192 /k \"%s\"", getenv("VCVARS"));
      }
   }

   /* what program do we want to invoke? */
   for (i=1; i<argc; i++) {
      strcat(cmd, " ");
      strcat(cmd, argv[i]);
   }

   /* do it... */
   printf("Invoking nested command interpreter\n");

   system(cmd);

   return 0;
}
