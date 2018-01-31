#include <smack-contracts.h>
#include "sha1.h"
#include "sidewinder.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <smack.h>
#include "ct-verif.h"

void sha1_final_stub(WORD datalen)
{
  datalen++;
  __VERIFIER_ASSUME_LEAKAGE(0);
}
void sha1_final(WORD datalen, SHA1_CTX *ctx)
{
        WORD i;

// Pad whatever data is left in the buffer.
        if (datalen < 56) {
                ctx->data[datalen] = 0x80;
		for (i = 0; i < 56 - datalen; i++)
		  {
		    invariant(i<=56-datalen);
		    ctx->data[i+datalen+1] = 0x00;
		  }
        }
        else {
                ctx->data[datalen] = 0x80;
		for (i = 0; i < 64 - datalen; i++)
		  {
		    invariant(i<=64-datalen);
		    ctx->data[i+datalen+1] = 0x00;
		  }
        }

// Append to the padding the total message's length in bits and transform.
        ctx->bitlen += datalen * 8;
        ctx->data[63] = ctx->bitlen;
        ctx->data[62] = ctx->bitlen >> 8;
        ctx->data[61] = ctx->bitlen >> 16;
        ctx->data[60] = ctx->bitlen >> 24;
        ctx->data[59] = ctx->bitlen >> 32;
        ctx->data[58] = ctx->bitlen >> 40;
        ctx->data[57] = ctx->bitlen >> 48;
        ctx->data[56] = ctx->bitlen >> 56;


}

void sha1_final_wrapper(WORD datalen, SHA1_CTX *ctx)

{

  // Purpose: Compute the timing behavior of sha1_final ignoring calls to transform
  // Any value smaller than 367 fails to verify.
        __VERIFIER_ASSERT_MAX_LEAKAGE(367);
	__VERIFIER_assume(datalen>=0);
	__VERIFIER_assume(datalen<64);
	sha1_final(datalen, ctx);
	sha1_final_stub(datalen);
}



