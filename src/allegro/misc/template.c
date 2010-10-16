/*
 *	ÇPROJECTNAMEÈ
 *
 *	Created by ÇFULLUSERNAMEÈ on ÇDATEÈ.
 *	Copyright (c) ÇYEARÈ ÇORGANIZATIONNAMEÈ. All rights reserved.
 */

#include <Allegro/allegro.h>


int main(int argc, const char *argv[])
{
	allegro_init();
	install_keyboard();
	
	if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0)) {
		allegro_message("Error setting 320x200x8 gfx mode:\n%s\n", allegro_error);
		return -1;
	}
	
	clear_to_color(screen, makecol(255, 255, 255));
	
	textout_centre_ex(screen, font, "Hello, world!", 160, 96, makecol(0, 0, 0), -1);
	
	readkey();
	
	return 0;
}
END_OF_MAIN();
