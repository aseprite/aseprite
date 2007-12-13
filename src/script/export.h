/*===================================================================*/
/* Base routines                                                     */
/*===================================================================*/

double MAX(double x, double y);
double MIN(double x, double y);
double MID(double x, double y, double z);

void include(const char *filename);
void dofile(const char *filename);
void print(const char *buf);

double rand(double min, double max); /* CODE */

/* string routines */

const char *_(const char *msgid);

int strcmp(const char *s1, const char *s2);

/* math routines */

#define PI

double fabs(double x);
double ceil(double x);
double floor(double x);

double exp(double x);
double log(double x);
double log10(double x);
double pow(double x, double y);
double sqrt(double x);
double hypot(double x, double y);

double cos(double x);
double sin(double x);
double tan(double x);
double acos(double x);
double asin(double x);
double atan(double x);
double atan2(double y, double x);

double cosh(double x);
double sinh(double x);
double tanh(double x);
/* TODO these routines aren't in MinGW math library */
/* double acosh (double x); */
/* double asinh (double x); */
/* double atanh (double x); */

/* file routines */

bool file_exists(const char *filename);
char *get_filename(const char *filename);

/*===================================================================*/
/* Sprite                                                            */
/*===================================================================*/

#define IMAGE_RGB
#define IMAGE_GRAYSCALE
#define IMAGE_INDEXED

Sprite *NewSprite(int imgtype, int w, int h);
Sprite *LoadSprite(const char *filename);
void SaveSprite(const char *filename);

void SetSprite(Sprite *sprite); 

/*===================================================================*/
/* Layer                                                             */
/*===================================================================*/

Layer *NewLayer(void);
Layer *NewLayerSet(void);
void RemoveLayer(void);

/* #define BLEND_MODE_NORMAL */
/* #define BLEND_MODE_DISSOLVE */
/* #define BLEND_MODE_MULTIPLY */
/* #define BLEND_MODE_SCREEN */
/* #define BLEND_MODE_OVERLAY */
/* #define BLEND_MODE_HARD_LIGHT */
/* #define BLEND_MODE_DODGE */
/* #define BLEND_MODE_BURN */
/* #define BLEND_MODE_DARKEN */
/* #define BLEND_MODE_LIGHTEN */
/* #define BLEND_MODE_ADDITION */
/* #define BLEND_MODE_SUBTRACT */
/* #define BLEND_MODE_DIFFERENCE */
/* #define BLEND_MODE_HUE */
/* #define BLEND_MODE_SATURATION */
/* #define BLEND_MODE_COLOR */
/* #define BLEND_MODE_LUMINOSITY */
 
/* #define DITHERING_NONE */
/* #define DITHERING_ORDERED */

/*===================================================================*/
/* End                                                               */
/*===================================================================*/
