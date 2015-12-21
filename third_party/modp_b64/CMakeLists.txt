# cmake support for modp_b64 by David Capello

check_c_source_runs("
int main() {
  unsigned long value = 0x40302010ul;
  return (((unsigned char*)&value)[0] != 0x10) ? 0: 1;
}
" WORDS_BIGENDIAN)

if(WORDS_BIGENDIAN)
  add_definitions(-DWORDS_BIGENDIAN)
endif()

add_library(modp_b64 modp_b64.c)
