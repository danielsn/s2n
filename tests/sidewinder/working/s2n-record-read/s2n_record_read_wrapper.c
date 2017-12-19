/*
 * Copyright 2014 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <sys/param.h>
#include <stdint.h>

#include "crypto/s2n_hmac.h"

#include "tls/s2n_record.h"
#include "tls/s2n_prf.h"
#include "tls/s2n_connection.h"

#include <smack.h>
#include <smack-contracts.h>
#include "ct-verif.h"
#include "sidewinder.h"
#include "utils/s2n_safety.h"
#include "tls/s2n_cipher_suites.h"
#include "crypto/s2n_cipher.h"

int s2n_record_parse_test(struct s2n_connection *conn,
			  const struct s2n_cipher_suite *cipher_suite,
			  uint8_t* header,
			  //struct s2n_blob en,
			  struct s2n_hmac_state *mac,
			  int payload_length,
			  uint8_t* sequence_number
			  )
{
  struct s2n_blob en;


  //L271 of original
 /* Subtract the padding length */
    if (cipher_suite->record_alg->cipher->type == S2N_CBC || cipher_suite->record_alg->cipher->type == S2N_COMPOSITE) {
        gt_check(en.size, 0);
        payload_length -= (en.data[en.size - 1] + 1);
    }

    uint8_t mac_digest_size = mac->digest_size;


        /* Update the MAC */
    header[3] = (payload_length >> 8);
    header[4] = payload_length & 0xff;
    GUARD(s2n_hmac_reset(mac));
    GUARD(s2n_hmac_update(mac, sequence_number, S2N_TLS_SEQUENCE_NUM_LEN));

    if (conn->actual_protocol_version == S2N_SSLv3) {
        GUARD(s2n_hmac_update(mac, header, 1));
        GUARD(s2n_hmac_update(mac, header + 3, 2));
    } else {
        GUARD(s2n_hmac_update(mac, header, S2N_TLS_RECORD_HEADER_LENGTH));
    }
    
    struct s2n_blob seq = {.data = sequence_number,.size = S2N_TLS_SEQUENCE_NUM_LEN };
    //GUARD(s2n_increment_sequence_number(&seq));

    /* Padding */
    if (cipher_suite->record_alg->cipher->type == S2N_CBC) {
        if (s2n_verify_cbc(conn, mac, &en) < 0) {
            GUARD(s2n_stuffer_wipe(&conn->in));
            S2N_ERROR(S2N_ERR_BAD_MESSAGE);
        }
    } else {
        /* MAC check for streaming ciphers - no padding */
        GUARD(s2n_hmac_update(mac, en.data, payload_length));

        uint8_t check_digest[S2N_MAX_DIGEST_LEN];
        lte_check(mac_digest_size, sizeof(check_digest));
        GUARD(s2n_hmac_digest(mac, check_digest, mac_digest_size));

        if (s2n_hmac_digest_verify(en.data + payload_length, check_digest, mac_digest_size) < 0) {
            GUARD(s2n_stuffer_wipe(&conn->in));
            S2N_ERROR(S2N_ERR_BAD_MESSAGE);
        }
    }

    /* O.k., we've successfully read and decrypted the record, now we need to align the stuffer
     * for reading the plaintext data.
     */
    GUARD(s2n_stuffer_reread(&conn->in));
    GUARD(s2n_stuffer_reread(&conn->header_in));

    return 0;

}

int s2n_record_parse_wrapper(int payload_length, int *xor_pad, int * digest_pad)
{
  uint8_t header[S2N_TLS_RECORD_HEADER_LENGTH];
  //increment sequence number does work based on the value here
  //needs to be public_in
  uint8_t sequence_number[S2N_TLS_SEQUENCE_NUM_LEN];
  
  struct s2n_hmac_state hmac = {
    .alg = S2N_HMAC_SHA1,
    .hash_block_size = BLOCK_SIZE,
    .currently_in_hash_block = 0,
    .digest_size = SHA_DIGEST_LENGTH,
    .xor_pad_size = BLOCK_SIZE,
    .inner.alg = S2N_HASH_SHA1,
    .inner.currently_in_hash_block = 0,
    .inner_just_key.alg = S2N_HASH_SHA1,
    .inner_just_key.currently_in_hash_block = 0,
    .outer.alg = S2N_HASH_SHA1,
    .outer.currently_in_hash_block = 0,
    .outer_just_key.alg = S2N_HASH_SHA1,
    .outer_just_key.currently_in_hash_block = 0,
     .xor_pad = *xor_pad,
    //xor_pad is an array
    .digest_pad = *digest_pad
  };


  struct s2n_cipher cipher = {
    .type = S2N_CBC,
  };
  
  struct s2n_record_algorithm record_algorithm = {
    .cipher = &cipher,
  };
  
  struct s2n_cipher_suite cipher_suite = {
    .record_alg = &record_algorithm,
  };

  struct s2n_connection conn = {
    .actual_protocol_version = S2N_TLS12,
  };
  
  return s2n_record_parse_test(&conn, &cipher_suite, header, &hmac, payload_length, sequence_number);
}
