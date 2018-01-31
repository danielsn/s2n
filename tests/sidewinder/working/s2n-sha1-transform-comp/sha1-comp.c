#include <smack-contracts.h>
#include "sha1.h"
#include "sidewinder.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <smack.h>
#include "ct-verif.h"
#define ROTLEFT(a, b) ((a << b) | (a >> (32 - b)))

void sha1_transform(SHA1_CTX *ctx, const BYTE data[])
{
        WORD a, b, c, d, e, i, j, k, x, y, idx, t, m[80];

        for (i = 0, j = 0; i < 16; ++i, j += 4) {
                invariant(i <= 16);
                m[i] = (data[j] << 24) + (data[j + 1] << 16) + (data[j + 2] << 8) + (data[j + 3]);
        }

        k = i;

        for (i = 0 ; i < 64; ++i)
        {
                invariant(i <= 64);
                idx=k+i;
                m[idx] = (m[idx - 3] ^ m[idx - 8] ^ m[idx - 14] ^ m[idx - 16]);
                m[idx] = (m[idx] << 1) | (m[idx] >> 31);
        }

        a = ctx->state[0];
        b = ctx->state[1];
        c = ctx->state[2];
        d = ctx->state[3];
        e = ctx->state[4];

        for (i = 0; i < 20; ++i) {
                invariant(i <= 20);
                t = ROTLEFT(a, 5) + ((b & c) ^ (~b & d)) + e + ctx->k[0] + m[i];
                e = d;
                d = c;
                c = ROTLEFT(b, 30);
                b = a;
                a = t;
        }

        k = i;

        for (i = 0 ; i < 20; ++i) {
                invariant(i <= 20);
                idx = k + i;
                t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[1] + m[idx];
                e = d;
                d = c;
                c = ROTLEFT(b, 30);
                b = a;
                a = t;
        }

        x = i;

        for (i=0 ; i < 20; ++i) {
                invariant(i <= 20);
                idx = k + x+ i;
                t = ROTLEFT(a, 5) + ((b & c) ^ (b & d) ^ (c & d))  + e + ctx->k[2] + m[idx];
                e = d;
                d = c;
                c = ROTLEFT(b, 30);
                b = a;
                a = t;
        }

        y = i;

        for (i=0 ; i < 20; ++i) {
                invariant(i <= 20);
                idx = k + x + y + i;
                t = ROTLEFT(a, 5) + (b ^ c ^ d) + e + ctx->k[3] + m[idx];
                e = d;
                d = c;
                c = ROTLEFT(b, 30);
                b = a;
                a = t;
        }

        ctx->state[0] += a;
        ctx->state[1] += b;
        ctx->state[2] += c;
        ctx->state[3] += d;
        ctx->state[4] += e;
}

void sha1_transform_stub(SHA1_CTX *ctx, const BYTE data[])
{
  __VERIFIER_ASSUME_LEAKAGE(0);
  WORD i;
  i=data[0];
}

void sha1_transform_wrapper(SHA1_CTX *ctx, BYTE * in_data, WORD datalen)

{
  // Compute the leakage of sha-transform. Any value smaller than 3206
  // fails to verify.
  __VERIFIER_ASSERT_MAX_LEAKAGE(3206);
  sha1_transform(ctx, in_data);
  sha1_transform_stub(ctx, in_data);
}

