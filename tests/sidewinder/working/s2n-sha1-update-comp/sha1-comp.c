#include <smack-contracts.h>
#include "sha1.h"
#include "sidewinder.h"
#include <stdint.h>
#include <stdlib.h>
#include <smack.h>
#include "ct-verif.h"

enum {
        PER_BLOCK_COST = 3211,
        PER_BYTE_COST = 8,
        BLOCK_SIZE = 64,
};


void sha1_update_stub(unsigned int datalen, SHA1_CTX *ctx, size_t size)
{
        __VERIFIER_ASSUME_LEAKAGE(PER_BYTE_COST * size);
        datalen += size;
        int num_filled_blocks = datalen / BLOCK_SIZE;
        __VERIFIER_ASSUME_LEAKAGE(num_filled_blocks * PER_BLOCK_COST);
        datalen = datalen % BLOCK_SIZE;
}

void sha1_update(unsigned int datalen, SHA1_CTX *ctx, const BYTE data[], size_t len)
{
        size_t i;


        int old = datalen;
        __VERIFIER_LOOP_VAR(old);
        for (i = 1; i <= len; ++i) {
                invariant(i <= len + 1);
                invariant(datalen == (i + old -1)%64);
                ctx->data[datalen] = data[i];
                datalen++;
                if (datalen == 64) {
// Substitute the call to transform with its computed leakage	  
//                        sha1_transform(ctx, ctx->data);
		        __VERIFIER_ADD_LEAKAGE(3206);
                        ctx->bitlen += 512;
                        datalen = 0;
                }
        }
        ctx->datalen = datalen;
}



void sha1_update_wrapper(size_t len, SHA1_CTX *ctx, WORD datalen)

{
        public_in(__SMACK_value(len));
	public_in(__SMACK_value(datalen));
        __VERIFIER_ASSERT_MAX_LEAKAGE(0);
	__VERIFIER_assume(datalen >= 0);
	__VERIFIER_assume(datalen < 64);
	__VERIFIER_assume(len > 0);
	ctx->datalen=datalen;
        sha1_update(ctx->datalen, ctx, NULL, len);
        sha1_update_stub(ctx->datalen, ctx, len);
}

