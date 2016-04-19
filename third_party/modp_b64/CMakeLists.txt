# cmake support for modp_b64 by David Capello

include(TestBigEndian)

test_big_endian(WORDS_BIGENDIAN)

if(WORDS_BIGENDIAN)
  add_definitions(-DWORDS_BIGENDIAN)
endif()

add_library(modp_b64 modp_b64.c)
