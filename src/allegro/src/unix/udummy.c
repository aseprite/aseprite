/* This file contains a dummy symbol that gets unconditionally included
 * in liballeg_unsharable.a through a tweak in misc/deplib.sh, in order
 * to make sure that the static library contains at least one symbol.
 * This is needed for Solaris because the linker chokes when a library
 * contains no symbol.
 */

int _dummy_symbol;
