#include "utils/sha1.h"
#include <stddef.h>
#include <stdint.h>
#include <malloc.h>

static inline uint32_t __circular_shift(int bits, uint32_t word)
{
    return (word << bits) | (word >> (32 - bits));
}

static inline void __fix(uint32_t * const A,
                         uint32_t * const B,
                         uint32_t * const C,
                         uint32_t * const D,
                         uint32_t * const E,
                         uint32_t tmp)
{
    *E = *D;
    *D = *C;
    *C = __circular_shift(30, *B);
    *B = *A;
    *A = tmp;
}

static void __process_block(uint32_t * const intermediate_hash,
                            const uint8_t * const block)
{
    static const uint32_t K[] = {
        0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
    };
    uint32_t A, B, C, D, E;
    uint32_t word[80];
    int i;

    for (i = 0; i < 16; i++) {
        word[i] = block[i << 2] << 24;
        word[i] |= block[(i << 2) + 1] << 16;
        word[i] |= block[(i << 2) + 2] << 8;
        word[i] |= block[(i << 2) + 3];
    }

    for (i = 16; i < 80; i++) {
        word[i] = __circular_shift(1, word[i - 3]
                                   ^ word[i - 8]
                                   ^ word[i - 14]
                                   ^ word[i - 16]);
    }

    A = intermediate_hash[0];
    B = intermediate_hash[1];
    C = intermediate_hash[2];
    D = intermediate_hash[3];
    E = intermediate_hash[4];

    for (i = 0; i < 20; i++) {
        __fix(&A, &B, &C, &D, &E,
              __circular_shift(5, A)
              + ((B & C) | ((~B) & D))
              + E
              + word[i]
              + K[0]);
    }

    for (i = 20; i < 40; i++) {
        __fix(&A, &B, &C, &D, &E,
              __circular_shift(5, A)
              + (B ^ C ^ D)
              + E
              + word[i]
              + K[1]);
    }

    for (i = 40; i < 60; i++) {
        __fix(&A, &B, &C, &D, &E,
              __circular_shift(5, A)
              + ((B & C) | (B & D) | (C & D))
              + E
              + word[i]
              + K[2]);
    }

    for (i = 60; i < 80; i++) {
        __fix(&A, &B, &C, &D, &E,
              __circular_shift(5, A)
              + (B ^ C ^ D)
              + E
              + word[i]
              + K[3]);
    }

    intermediate_hash[0] += A;
    intermediate_hash[1] += B;
    intermediate_hash[2] += C;
    intermediate_hash[3] += D;
    intermediate_hash[4] += E;
}

char * zl_sha1(const char * const plain, int plain_size)
{
    uint32_t intermediate_hash[] = {
        0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
    };
    char block[64] = { 0 };
    int i;
    int block_idx;
    size_t len;
    char * digest;
    digest = calloc(20, 1);
    if (digest == NULL) {
        return NULL;
    }

    len = 0;
    block_idx = 0;
    for (i = 0; i < plain_size; i++) {
        block[block_idx++] = plain[i];
        len += 8;

        if (block_idx == 64) {
            __process_block(intermediate_hash, (uint8_t *) block);
            block_idx = 0;
        }
    }

    if (block_idx > 55) {
        block[block_idx++] = 0x80;
        while (block_idx < 64) {
            block[block_idx++] = 0x00;
        }
        __process_block(intermediate_hash, (uint8_t *) block);

        block_idx = 0;
        while (block_idx < 56) {
            block[block_idx++] = 0;
        }
    }
    else {
        block[block_idx++] = 0x80;
        while (block_idx < 56) {
            block[block_idx++] = 0x00;
        }
    }

    block[56] = len >> 56;
    block[57] = len >> 48;
    block[58] = len >> 40;
    block[59] = len >> 32;
    block[60] = len >> 24;
    block[61] = len >> 16;
    block[62] = len >> 8;
    block[63] = len;

    __process_block(intermediate_hash, (uint8_t *) block);

    for (i = 0; i < 20; i++) {
        digest[i] = intermediate_hash[i >> 2] >> 8 * (3 - (i & 0x03));
    }

    return digest;
}
