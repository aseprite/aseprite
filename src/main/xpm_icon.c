#include <allegro.h>
/* XPM */
static const char *ase16png_xpm[] = {
/* columns rows colors chars-per-pixel */
"16 16 8 1",
"  c #010000",
". c #325C70",
"X c #AD947D",
"o c #529BC1",
"O c #65D7DE",
"+ c #EDD6C0",
"@ c #FFFFFF",
"# c None",
/* pixels */
"################",
"################",
"########XXXX@ ##",
"#######oX+@@  ##",
"######oOX@++Xo##",
"#####oOoo+++Xo##",
"####oOo@Oo+XXo##",
"###oOo@OO@oXX.##",
"##.Oo@OOOOoo.###",
"##.o@OOOOoo.####",
"##.OOOOOoo.#####",
"##.oOOOoo.######",
"###.oOoo.#######",
"####....########",
"################",
"################"
};
#if defined ALLEGRO_WITH_XWINDOWS && defined ALLEGRO_USE_CONSTRUCTOR
extern void *allegro_icon;
CONSTRUCTOR_FUNCTION(static void _set_allegro_icon(void));
static void _set_allegro_icon(void)
{
    allegro_icon = ase16png_xpm;
}
#endif
