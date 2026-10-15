#include <string.h>

#include "sha256.h"

#define ROTATE_RIGHT(value, count) (((value) >> (count)) | ((value) << (32U - (count))))
#define CHOOSE(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJORITY(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SIGMA0(x) (ROTATE_RIGHT((x), 2) ^ ROTATE_RIGHT((x), 13) ^ ROTATE_RIGHT((x), 22))
#define SIGMA1(x) (ROTATE_RIGHT((x), 6) ^ ROTATE_RIGHT((x), 11) ^ ROTATE_RIGHT((x), 25))
#define GAMMA0(x) (ROTATE_RIGHT((x), 7) ^ ROTATE_RIGHT((x), 18) ^ ((x) >> 3))
#define GAMMA1(x) (ROTATE_RIGHT((x), 17) ^ ROTATE_RIGHT((x), 19) ^ ((x) >> 10))

static const uint32_t constants[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

static uint32_t read_big_endian(const unsigned char *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
        ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static void write_big_endian(unsigned char *output, uint32_t value)
{
    output[0] = (unsigned char)(value >> 24);
    output[1] = (unsigned char)(value >> 16);
    output[2] = (unsigned char)(value >> 8);
    output[3] = (unsigned char)value;
}

static void transform(sha256_context_t *context, const unsigned char block[64])
{
    uint32_t words[64];
    uint32_t a, b, c, d, e, f, g, h;

    for (unsigned int index = 0; index < 16; ++index)
        words[index] = read_big_endian(block + index * 4);
    for (unsigned int index = 16; index < 64; ++index)
        words[index] = GAMMA1(words[index - 2]) + words[index - 7] +
            GAMMA0(words[index - 15]) + words[index - 16];

    a = context->state[0]; b = context->state[1]; c = context->state[2];
    d = context->state[3]; e = context->state[4]; f = context->state[5];
    g = context->state[6]; h = context->state[7];
    for (unsigned int index = 0; index < 64; ++index) {
        uint32_t first = h + SIGMA1(e) + CHOOSE(e, f, g) + constants[index] + words[index];
        uint32_t second = SIGMA0(a) + MAJORITY(a, b, c);
        h = g; g = f; f = e; e = d + first;
        d = c; c = b; b = a; a = first + second;
    }
    context->state[0] += a; context->state[1] += b;
    context->state[2] += c; context->state[3] += d;
    context->state[4] += e; context->state[5] += f;
    context->state[6] += g; context->state[7] += h;
}

void sha256_init(sha256_context_t *context)
{
    static const uint32_t initial[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
    };
    memcpy(context->state, initial, sizeof(initial));
    context->bit_count = 0;
    context->buffer_size = 0;
}

void sha256_update(sha256_context_t *context, const void *input, size_t size)
{
    const unsigned char *data = input;
    context->bit_count += (uint64_t)size * 8U;
    while (size > 0) {
        size_t available = sizeof(context->buffer) - context->buffer_size;
        size_t chunk = size < available ? size : available;
        memcpy(context->buffer + context->buffer_size, data, chunk);
        context->buffer_size += chunk;
        data += chunk;
        size -= chunk;
        if (context->buffer_size == sizeof(context->buffer)) {
            transform(context, context->buffer);
            context->buffer_size = 0;
        }
    }
}

void sha256_final(sha256_context_t *context, unsigned char digest[SHA256_DIGEST_SIZE])
{
    uint64_t bits = context->bit_count;
    context->buffer[context->buffer_size++] = 0x80U;
    if (context->buffer_size > 56) {
        memset(context->buffer + context->buffer_size, 0,
               sizeof(context->buffer) - context->buffer_size);
        transform(context, context->buffer);
        context->buffer_size = 0;
    }
    memset(context->buffer + context->buffer_size, 0, 56 - context->buffer_size);
    for (unsigned int index = 0; index < 8; ++index)
        context->buffer[63 - index] = (unsigned char)(bits >> (index * 8));
    transform(context, context->buffer);
    for (unsigned int index = 0; index < 8; ++index)
        write_big_endian(digest + index * 4, context->state[index]);
    memset(context, 0, sizeof(*context));
}

void sha256(const void *data, size_t size, unsigned char digest[SHA256_DIGEST_SIZE])
{
    sha256_context_t context;
    sha256_init(&context);
    sha256_update(&context, data, size);
    sha256_final(&context, digest);
}
