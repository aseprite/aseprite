#ifndef LIBART_FEATURES_H
#define LIBART_FEATURES_H 1

#define LIBART_MAJOR_VERSION (2)
#define LIBART_MINOR_VERSION (3)
#define LIBART_MICRO_VERSION (3)
#define LIBART_VERSION "2.3.3"

extern const unsigned int libart_major_version, libart_minor_version, libart_micro_version;
extern const char *libart_version;

void libart_preinit(void *app, void *modinfo);
void libart_postinit(void *app, void *modinfo);
#endif
