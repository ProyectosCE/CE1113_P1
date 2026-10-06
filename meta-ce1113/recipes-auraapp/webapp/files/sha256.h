#ifndef AURABOT_SHA256_H
#define AURABOT_SHA256_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_DIGEST_SIZE 32

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    unsigned char buffer[64];
    size_t buffer_size;
} sha256_context_t;

void sha256_init(sha256_context_t *context);
void sha256_update(sha256_context_t *context, const void *data, size_t size);
void sha256_final(sha256_context_t *context,
                  unsigned char digest[SHA256_DIGEST_SIZE]);
void sha256(const void *data, size_t size,
            unsigned char digest[SHA256_DIGEST_SIZE]);

#endif
