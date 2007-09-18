#!/usr/bin/env python
#

#
# autohint math table builder
#


# Copyright 1996-2000 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


import math

ag_pi = 256

def print_arctan( atan_bits ):
    atan_base = 1 << atan_bits

    print "  static AH_Angle  ag_arctan[1L << AG_ATAN_BITS] ="
    print "  {"

    count = 0
    line  = "   "

    for n in range( atan_base ):
        comma = ","
        if ( n == atan_base - 1 ):
            comma = ""

        angle = math.atan( n * 1.0 / atan_base ) / math.pi * ag_pi
        line  = line + " " + repr( int( angle + 0.5 ) ) + comma
        count = count + 1;
        if ( count == 8 ):
            count = 0
            print line
            line = "   "

    if ( count > 0 ):
        print line
    print "  };"


# This routine is not used currently.
#
def print_sines():
    print "  static FT_Fixed  ah_sines[AG_HALF_PI + 1] ="
    print "  {"

    count = 0
    line  = "   "

    for n in range( ag_pi / 2 ):
        sinus = math.sin( n * math.pi / ag_pi )
        line  = line + " " + repr( int( 65536.0 * sinus ) ) + ","
        count = count + 1
        if ( count == 8 ):
            count = 0
            print line
            line = "   "

    if ( count > 0 ):
        print line
    print "   65536"
    print "  };"


print_arctan( 8 )
print


# END
