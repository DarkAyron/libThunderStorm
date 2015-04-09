#ifndef _LOOKUP3_H
#define _LOOKUP3_H

#ifdef WIN32
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#else
#include <stdint.h>     /* defines uint32_t etc */
#endif

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
void hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);

#ifdef __cplusplus
}
#endif

#endif  /* _LOOKUP3_H */
