/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Unix timer module.
 *
 *      By Peter Wang.
 *
 *      _unix_rest by Elias Pschernig.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/platform/aintunix.h"
#include <sys/time.h>


#ifndef ALLEGRO_MACOSX

/* System drivers provide their own lists, so this is just to keep the 
 * Allegro framework happy.  */
_DRIVER_INFO _timer_driver_list[] = {
   { 0, 0, 0 }
};

#endif



/* timeval_subtract:
 *  Subtract the `struct timeval' values X and Y, storing the result
 *  in RESULT.  Return 1 if the difference is negative, otherwise 0.
 *
 *  This function is from the glibc manual.  It handles weird platforms
 *  where the tv_sec is unsigned.
 */
static int timeval_subtract(struct timeval *result,
			    struct timeval *x,
			    struct timeval *y)
{
   /* Perform the carry for the later subtraction by updating Y. */
   if (x->tv_usec < y->tv_usec) {
      int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
      y->tv_usec -= 1000000 * nsec;
      y->tv_sec += nsec;
   }
   if (x->tv_usec - y->tv_usec > 1000000) {
      int nsec = (x->tv_usec - y->tv_usec) / 1000000;
      y->tv_usec += 1000000 * nsec;
      y->tv_sec -= nsec;
   }

   /* Compute the time remaining to wait.
    * `tv_usec' is certainly positive. */
   result->tv_sec = x->tv_sec - y->tv_sec;
   result->tv_usec = x->tv_usec - y->tv_usec;
   /* Return 1 if result is negative. */
   return x->tv_sec < y->tv_sec;
}



void _unix_rest(unsigned int ms, void (*callback) (void))
{
   if (callback) {
      struct timeval tv, tv_end;

      gettimeofday (&tv_end, NULL);
      tv_end.tv_usec += ms * 1000;
      tv_end.tv_sec  += (tv_end.tv_usec / 1000000L);
      tv_end.tv_usec %= 1000000L;

      while (1)
      {
         (*callback)();
         gettimeofday (&tv, NULL);
         if (tv.tv_sec > tv_end.tv_sec)
            break;
         if (tv.tv_sec == tv_end.tv_sec && tv.tv_usec >= tv_end.tv_usec)
             break;
      }
   }
   else {
      struct timeval now;
      struct timeval end;
      struct timeval delay;
      int result;

      gettimeofday(&now, NULL);

      end = now;
      end.tv_usec += ms * 1000;
      end.tv_sec  += (end.tv_usec / 1000000L);
      end.tv_usec %= 1000000L;

      while (1) {
	 if (timeval_subtract(&delay, &end, &now))
	    break;

#ifdef ALLEGRO_MACOSX
	 result = usleep((delay.tv_sec * 1000000L) + delay.tv_usec);
#else
	 result = select(0, NULL, NULL, NULL, &delay);
#endif
	 if (result == 0)	/* ok */
	    break;
	 if ((result != -1) || (errno != EINTR))
	    break;

	 /* interrupted */
	 gettimeofday(&now, NULL);
      }
   }
}
