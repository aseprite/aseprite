
	loadpng: glue for Allegro and libpng
	

This wrapper is mostly a copy and paste job from example.c in the
libpng docs, stripping out the useless transformations and making it
use Allegro BITMAP and PALETTE structures.  It is placed in the public
domain.


Requirements:

	Allegro		http://alleg.sourceforge.net/
	libpng 		http://www.libpng.org/pub/png/
	zlib 		http://www.gzip.org/zlib/


Usage:

See loadpng.h for functions and their descriptions.  There is a
simple example program called example.c, a program demonstrating
alpha translucency in exalpha.c, and a program demonstrating how to
load a PNG object from a datafile in exdata.c.

To compile, just run "make" (or perhaps "mingw32-make").  To use
loadpng, you need to link with libpng, zlib in addition to loadpng
itself, e.g.

	gcc mygame.c -lldpng -lpng -lz -lalleg

I recommend you copy loadpng's files into your own project's directory
and compile loadpng as part of your project.


Notes:

- Grayscale images will be loaded in as 24 bit images, or 32 bit
  images if they contain an alpha channel.  These will then be
  converted as usual, according to Allegro's conversion semantics.  Be
  aware of this if you have disabled automatic colour depth
  conversion.

- save_png() doesn't save any gamma chunk.  I'm thinking of making it
  write an sRGB chunk by default.  If you want tight control of how
  your images are saved, I recommend hacking save_png() to suit your
  needs.  There's simply too many options.


Enjoy!

Peter Wang (tjaden@users.sf.net)
http://members.ozadsl.com.au/~tjaden/
http://tjaden.strangesoft.net/
