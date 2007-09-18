#include "libart-features.h"

/* General initialization hooks */
const unsigned int libart_major_version=LIBART_MAJOR_VERSION,
  libart_minor_version=LIBART_MINOR_VERSION,
  libart_micro_version=LIBART_MICRO_VERSION;

const char *libart_version = LIBART_VERSION;

void
libart_preinit(void *app, void *modinfo)
{
}

void
libart_postinit(void *app, void *modinfo)
{
}
