#
# FreeType 2 build system -- top-level Makefile for OpenVMS
#


# Copyright 2001 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.


all :
        define freetype [--.include.freetype] 
        define psaux [-.psaux] 
        define autohint [-.autohint] 
        define base [-.base] 
        define cache [-.cache] 
        define cff [-.cff] 
        define cid [-.cid] 
        define pcf [-.pcf] 
        define psnames [-.psnames] 
        define raster [-.raster] 
        define sfnt [-.sfnt] 
        define smooth [-.smooth] 
        define truetype [-.truetype] 
        define type1 [-.type1] 
        define winfonts [-.winfonts] 
        if f$search("lib.dir") .eqs. "" then create/directory [.lib]
        set default [.builds.vms]
        $(MMS)$(MMSQUALIFIERS)
        set default [--.src.autohint]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.base]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.cache]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.cff]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.cid]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.pcf]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.psaux]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.pshinter]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.psnames]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.raster]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.sfnt]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.smooth]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.truetype]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.type1]
        $(MMS)$(MMSQUALIFIERS)
        set default [-.winfonts]
        $(MMS)$(MMSQUALIFIERS)
        set default [--]

# EOF
