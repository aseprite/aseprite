# Copyright (c) 2012, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name of Intel Corporation nor the names of its contributors may
#   be used to endorse or promote products derived from this software without
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Try to find Harfbuzz include and library directories.
#
# After successful discovery, this will set for inclusion where needed:
# HARFBUZZ_INCLUDE_DIRS - containg the HarfBuzz headers
# HARFBUZZ_LIBRARIES - containg the HarfBuzz library

find_package(PkgConfig QUIET)

pkg_check_modules(PC_HARFBUZZ harfbuzz>=0.9.7)

find_path(HARFBUZZ_INCLUDE_DIRS NAMES hb.h
    HINTS ${PC_HARFBUZZ_INCLUDE_DIRS} ${PC_HARFBUZZ_INCLUDEDIR})

find_library(HARFBUZZ_LIBRARIES NAMES harfbuzz
    HINTS ${PC_HARFBUZZ_LIBRARY_DIRS} ${PC_HARFBUZZ_LIBDIR})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HarfBuzz DEFAULT_MSG HARFBUZZ_INCLUDE_DIRS
    HARFBUZZ_LIBRARIES)

mark_as_advanced(
    HARFBUZZ_INCLUDE_DIRS
    HARFBUZZ_LIBRARIES)
