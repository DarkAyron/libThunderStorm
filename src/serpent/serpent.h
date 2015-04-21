/*
 * Common values for serpent algorithms
 */

#ifndef _SERPENT_H
#define _SERPENT_H

#include <stdint.h>

#define SERPENT_MIN_KEY_SIZE		  0
#define SERPENT_MAX_KEY_SIZE		 32
#define SERPENT_EXPKEY_WORDS		132
#define SERPENT_BLOCK_SIZE		 16

typedef struct serpentSchedule {
	uint32_t expkey[SERPENT_EXPKEY_WORDS];
} serpentSchedule_t;

int serpentKeySetup(const uint8_t *key, struct serpentSchedule *ctx,
		     unsigned int keylen);

void serpentEncrypt(struct serpentSchedule *ctx, uint8_t *src, const uint8_t *dst);
void serpentDecrypt(struct serpentSchedule *ctx, uint8_t *src, const uint8_t *dst);

#endif
