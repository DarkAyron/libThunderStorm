// Some compilers (e.g. Visual Studio 2012) don't like the name conflict between
// zlib\crc32.c and pklib\crc32.c. This file is plain wrapper for compress.c
// in order to create obj file with a different name.

#include "crc32.c"
