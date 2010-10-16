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
 *      Bezier spline plotter.
 *
 *      By Seymour Shlien.
 *
 *      Optimised version by Sven Sandberg.
 *
 *      I'm not sure wether or not we still use the Castelau Algorithm
 *      described in the book :o)
 *
 *      Interactive Computer Graphics
 *      by Peter Burger and Duncan Gillies
 *      Addison-Wesley Publishing Co 1989
 *      ISBN 0-201-17439-1
 *
 *      The 4 th order Bezier curve is a cubic curve passing
 *      through the first and fourth point. The curve does
 *      not pass through the middle two points. They are merely
 *      guide points which control the shape of the curve. The
 *      curve is tangent to the lines joining points 1 and 2
 *      and points 3 and 4.
 *
 *      See readme.txt for copyright information.
 */


#include <math.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* calc_spline:
 *  Calculates a set of pixels for the bezier spline defined by the four
 *  points specified in the points array. The required resolution
 *  is specified by the npts parameter, which controls how many output
 *  pixels will be stored in the x and y arrays.
 */
void calc_spline(AL_CONST int points[8], int npts, int *out_x, int *out_y)
{
   /* Derivatives of x(t) and y(t). */
   double x, dx, ddx, dddx;
   double y, dy, ddy, dddy;
   int i;

   /* Temp variables used in the setup. */
   double dt, dt2, dt3;
   double xdt2_term, xdt3_term;
   double ydt2_term, ydt3_term;

   dt = 1.0 / (npts-1);
   dt2 = (dt * dt);
   dt3 = (dt2 * dt);

   /* x coordinates. */
   xdt2_term = 3 * (points[4] - 2*points[2] + points[0]);
   xdt3_term = points[6] + 3 * (-points[4] + points[2]) - points[0];

   xdt2_term = dt2 * xdt2_term;
   xdt3_term = dt3 * xdt3_term;

   dddx = 6*xdt3_term;
   ddx = -6*xdt3_term + 2*xdt2_term;
   dx = xdt3_term - xdt2_term + 3 * dt * (points[2] - points[0]);
   x = points[0];

   out_x[0] = points[0];

   x += .5;
   for (i=1; i<npts; i++) {
      ddx += dddx;
      dx += ddx;
      x += dx;

      out_x[i] = (int)x;
   }

   /* y coordinates. */
   ydt2_term = 3 * (points[5] - 2*points[3] + points[1]);
   ydt3_term = points[7] + 3 * (-points[5] + points[3]) - points[1];

   ydt2_term = dt2 * ydt2_term;
   ydt3_term = dt3 * ydt3_term;

   dddy = 6*ydt3_term;
   ddy = -6*ydt3_term + 2*ydt2_term;
   dy = ydt3_term - ydt2_term + dt * 3 * (points[3] - points[1]);
   y = points[1];

   out_y[0] = points[1];

   y += .5;

   for (i=1; i<npts; i++) {
      ddy += dddy;
      dy += ddy;
      y += dy;

      out_y[i] = (int)y;
   }
}



/* spline:
 *  Draws a bezier spline onto the specified bitmap in the specified color.
 */
void _soft_spline(BITMAP *bmp, AL_CONST int points[8], int color)
{   
   #define MAX_POINTS   64

   int xpts[MAX_POINTS], ypts[MAX_POINTS];
   int i;
   int num_points;
   int c;
   int old_drawing_mode, old_drawing_x_anchor, old_drawing_y_anchor;
   BITMAP *old_drawing_pattern;
   ASSERT(bmp);

   /* Calculate the number of points to draw. We want to draw as few as
      possible without loosing image quality. This algorithm is rather
      random; I have no motivation for it at all except that it seems to work
      quite well. The length of the spline is approximated by the sum of
      distances from first to second to third to last point. The number of
      points to draw is then the square root of this distance. I first tried
      to make the number of points proportional to this distance without
      taking the square root of it, but then short splines kind of had too
      few points and long splines had too many. Since sqrt() increases more
      for small input than for large, it seems in a way logical to use it,
      but I don't precisely have any mathematical proof for it. So if someone
      has a better idea of how this could be done, don't hesitate to let us
      know...
   */

   #undef DIST
   #define DIST(x, y) (sqrt((x) * (x) + (y) * (y)))
   num_points = (int)(sqrt(DIST(points[2]-points[0], points[3]-points[1]) +
			   DIST(points[4]-points[2], points[5]-points[3]) +
			   DIST(points[6]-points[4], points[7]-points[5])) *
		      1.2);

   if (num_points > MAX_POINTS)
      num_points = MAX_POINTS;

   calc_spline(points, num_points, xpts, ypts);

   acquire_bitmap(bmp);

   if ((_drawing_mode == DRAW_MODE_XOR) ||
       (_drawing_mode == DRAW_MODE_TRANS)) {
      /* Must compensate for the end pixel being drawn twice,
	 hence the mess. */
      old_drawing_mode = _drawing_mode;
      old_drawing_pattern = _drawing_pattern;
      old_drawing_x_anchor = _drawing_x_anchor;
      old_drawing_y_anchor = _drawing_y_anchor;
      for (i=1; i<num_points-1; i++) {
	 c = getpixel(bmp, xpts[i], ypts[i]);
	 line(bmp, xpts[i-1], ypts[i-1], xpts[i], ypts[i], color);
	 solid_mode();
	 putpixel(bmp, xpts[i], ypts[i], c);
	 drawing_mode(old_drawing_mode, old_drawing_pattern,
		      old_drawing_x_anchor, old_drawing_y_anchor);
      }
      line(bmp, xpts[i-1], ypts[i-1], xpts[i], ypts[i], color);
   }
   else {
      for (i=1; i<num_points; i++)
	 line(bmp, xpts[i-1], ypts[i-1], xpts[i], ypts[i], color);
   }

   release_bitmap(bmp);
}

