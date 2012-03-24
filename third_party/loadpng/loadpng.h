/* loadpng.h */
#ifndef _included_loadpng_h_
#define _included_loadpng_h_

#ifdef __cplusplus
extern "C" {
#endif


#if (defined LOADPNG_DYNAMIC) && (defined ALLEGRO_WINDOWS)
    #ifdef LOADPNG_SRC_BUILD
        #define _APNG_DLL __declspec(dllexport)
    #else
        #define _APNG_DLL __declspec(dllimport)
    #endif /* ALLEGRO_GL_SRC_BUILD */
#else
    #define _APNG_DLL
#endif /* (defined LOADPNG_DYNAMIC) && (defined ALLEGRO_WINDOWS) */

#define APNG_VAR(type, name) extern _APNG_DLL type name

#if (defined LOADPNG_DYNAMIC) && (defined ALLEGRO_WINDOWS)
    #define APNG_FUNC(type, name, args) extern _APNG_DLL type __cdecl name args
#else
    #define APNG_FUNC(type, name, args) extern type name args
#endif /* (defined LOADPNG_DYNAMIC) && (defined ALLEGRO_WINDOWS) */



/* Overkill :-) */
#define LOADPNG_VERSION         1
#define LOADPNG_SUBVERSION      5
#define LOADPNG_VERSIONSTR      "1.5"


/* _png_screen_gamma is slightly overloaded (sorry):
 *
 * A value of 0.0 means: Don't do any gamma correction in load_png()
 * and load_memory_png().  This meaning was introduced in v1.4.
 *
 * A value of -1.0 means: Use the value from the environment variable
 * SCREEN_GAMMA (if available), otherwise fallback to a value of 2.2
 * (a good guess for PC monitors, and the value for sRGB colourspace).
 * This is the default.
 *
 * Otherwise, the value of _png_screen_gamma is taken as-is.
 */
APNG_VAR(double, _png_screen_gamma);


/* Choose zlib compression level for saving file.
 * Default is Z_BEST_COMPRESSION.
 */
APNG_VAR(int, _png_compression_level);


/* Load a PNG from disk. */
APNG_FUNC(BITMAP *, load_png, (AL_CONST char *filename, RGB *pal));

/* Load a PNG from some place. */
APNG_FUNC(BITMAP *, load_png_pf, (PACKFILE *fp, RGB *pal));

/* Load a PNG from memory. */
APNG_FUNC(BITMAP *, load_memory_png, (AL_CONST void *buffer, int buffer_size, RGB *pal));

/* Save a bitmap to disk in PNG format. */
APNG_FUNC(int, save_png, (AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal));

/* Adds `PNG' to Allegro's internal file type table.
 * You can then just use load_bitmap and save_bitmap as usual.
 */
APNG_FUNC(void, register_png_file_type, (void));

/* Register an datafile type ID with Allegro, so that when an object
 * with that type ID is encountered while loading a datafile, that
 * object will be loaded as a PNG file.
 */
APNG_FUNC(void, register_png_datafile_object, (int id));

/* This is supposed to resemble jpgalleg_init in JPGalleg 2.0, just in
 * case you are lazier than lazy.  It contains these 3 lines of code:
 *  register_png_datafile_object(DAT_ID('P','N','G',' '));
 *  register_png_file_type();
 *  return 0;
 */
APNG_FUNC(int, loadpng_init, (void));


#ifdef __cplusplus
}
#endif

#endif /* _included_loadpng_h */
