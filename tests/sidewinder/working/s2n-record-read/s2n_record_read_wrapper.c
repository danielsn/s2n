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
#include <stdlib.h>

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
#include "utils/s2n_blob.h"
#include "crypto/s2n_cipher.h"

#define DECRYPT_COST 10
#define IV_SIZE 16
#define MAX_SIZE 1024
int decrypt_cbc(struct s2n_session_key *session_key,
		struct s2n_blob* iv,
		struct s2n_blob* in,
		struct s2n_blob* out)

{
  int size = in->size;
  __VERIFIER_ASSUME_LEAKAGE(size * DECRYPT_COST);
  out->data = malloc(size);
  return 0;
}

int decrypt_stream(struct s2n_session_key *session_key,
		   struct s2n_blob* in,
		   struct s2n_blob* out)

{
  int size = in->size;
  __VERIFIER_ASSUME_LEAKAGE(size * DECRYPT_COST);
  out->data = malloc(size);
  return 0;
}

int decrypt_aead(struct s2n_session_key *session_key,
		struct s2n_blob* iv,
		struct s2n_blob* aad,
		struct s2n_blob* in,
		struct s2n_blob* out)

{
  int size = in->size;
  __VERIFIER_ASSUME_LEAKAGE(size * DECRYPT_COST);
  out->data = malloc(size);
  return 0;
}


int s2n_record_parse_test(struct s2n_connection *conn,
			  uint8_t* header,
			  int payload_length,
			  int packet_size,
			  int padding_length
			  )
{

    struct s2n_blob iv;
    iv.size = IV_SIZE;//DSN trying to fix bad memcopy
    __VERIFIER_assume(packet_size > IV_SIZE);
    struct s2n_blob en;
    en.data = malloc(packet_size);
    struct s2n_blob aad;
    uint8_t content_type;
    uint16_t fragment_length;
    uint8_t ivpad[S2N_TLS_MAX_IV_LEN];
    uint8_t aad_gen[S2N_TLS_MAX_AAD_LEN];// = { 0 };
    uint8_t aad_iv[S2N_TLS_MAX_IV_LEN];// = { 0 };

    uint8_t *sequence_number = conn->client->client_sequence_number;
    struct s2n_hmac_state *mac = &conn->client->client_record_mac;
    struct s2n_session_key *session_key = &conn->client->client_key;
    const struct s2n_cipher_suite *cipher_suite = conn->client->cipher_suite;
    uint8_t *implicit_iv = conn->client->client_implicit_iv;

    en.size = packet_size;

    //Line 222
    /* Decrypt stuff! */
    switch (cipher_suite->record_alg->cipher->type) {
        case S2N_STREAM:
            GUARD(cipher_suite->record_alg->cipher->io.stream.decrypt(session_key, &en, &en));
            break;
        case S2N_CBC:
            /* Check that we have some data to decrypt */
            ne_check(en.size, 0);

            /* ... and that we have a multiple of the block size */
            eq_check(en.size % iv.size, 0);

            /* Copy the last encrypted block to be the next IV */
            if (conn->actual_protocol_version < S2N_TLS11) {
	       memcpy_check(ivpad, en.data + en.size - iv.size, iv.size);
            }

            GUARD(cipher_suite->record_alg->cipher->io.cbc.decrypt(session_key, &iv, &en, &en));

            if (conn->actual_protocol_version < S2N_TLS11) {
	      memcpy_check(implicit_iv, ivpad, iv.size);
            }
            break;
        case S2N_AEAD:

            /* Skip explicit IV for decryption */
            en.size -= cipher_suite->record_alg->cipher->io.aead.record_iv_size;
            en.data += cipher_suite->record_alg->cipher->io.aead.record_iv_size;
            /* Check that we have some data to decrypt */
            ne_check(en.size, 0);

            GUARD(cipher_suite->record_alg->cipher->io.aead.decrypt(session_key, &iv, &aad, &en, &en));
            break;
        case S2N_COMPOSITE:
            ne_check(en.size, 0);
            eq_check(en.size % iv.size,  0);

            /* Copy the last encrypted block to be the next IV */
            memcpy_check(ivpad, en.data + en.size - iv.size, iv.size);

            /* This will: Skip the explicit IV(if applicable), decrypt the payload, verify the MAC and padding. */
            GUARD((cipher_suite->record_alg->cipher->io.comp.decrypt(session_key, &iv, &en, &en)));

            memcpy_check(implicit_iv, ivpad, iv.size);
            break;
        default:
            S2N_ERROR(S2N_ERR_CIPHER_TYPE);
            break;
    }
  

  //L271 of original
 /* Subtract the padding length */
    if (cipher_suite->record_alg->cipher->type == S2N_CBC || cipher_suite->record_alg->cipher->type == S2N_COMPOSITE) {
        gt_check(en.size, 0);
	en.data[en.size - 1] = padding_length;
        payload_length -= (en.data[en.size - 1] + 1);
	__VERIFIER_assume(payload_length >= 0);
	__VERIFIER_assume(payload_length < MAX_SIZE);
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
	  __VERIFIER_assume(conn->in->
	  __VERIFIER_assume(0);
            GUARD(s2n_stuffer_wipe(&conn->in));
            S2N_ERROR(S2N_ERR_BAD_MESSAGE);
        }
    }

    /* O.k., we've successfully read and decrypted the record, now we need to align the stuffer
     * for reading the plaintext data.
     */
    GUARD(s2n_stuffer_reread(&conn->in));
    GUARD(s2n_stuffer_reread(&conn->header_in));

    /* Skip the IV, if any */
    if (cipher_suite->record_alg->cipher->type == S2N_CBC && conn->actual_protocol_version > S2N_TLS10) {
        GUARD(s2n_stuffer_skip_read(&conn->in, cipher_suite->record_alg->cipher->io.cbc.record_iv_size));
    } else if (cipher_suite->record_alg->cipher->type == S2N_AEAD && conn->actual_protocol_version >= S2N_TLS12) {
        GUARD(s2n_stuffer_skip_read(&conn->in, cipher_suite->record_alg->cipher->io.aead.record_iv_size));
    } else if (cipher_suite->record_alg->cipher->type == S2N_COMPOSITE && conn->actual_protocol_version > S2N_TLS10) {
        GUARD(s2n_stuffer_skip_read(&conn->in, cipher_suite->record_alg->cipher->io.comp.record_iv_size));
    }

    /* Truncate and wipe the MAC and any padding */
    GUARD(s2n_stuffer_wipe_n(&conn->in, s2n_stuffer_data_available(&conn->in) - payload_length));
    conn->in_status = PLAINTEXT;

    
    return 0;

}



int s2n_record_parse_wrapper(int payload_length,
			     int *xor_pad,
			     int * digest_pad,
			     int packet_size,
			     int padding_length)
{
  __VERIFIER_ASSERT_MAX_LEAKAGE(68);
  __VERIFIER_assume(packet_size > 0);
  __VERIFIER_assume(packet_size < MAX_SIZE);
  __VERIFIER_assume(payload_length > 0);
  __VERIFIER_assume(payload_length <= packet_size);
  __VERIFIER_assume(padding_length >= 0);
  __VERIFIER_assume(padding_length < 256);
  __VERIFIER_assume(padding_length < payload_length);
  
  public_in(__SMACK_value(packet_size));
  public_in(__SMACK_value(payload_length));
  public_in(__SMACK_value(padding_length));
  
  uint8_t header[S2N_TLS_RECORD_HEADER_LENGTH];
  //increment sequence number does work based on the value here
  //needs to be public_in
  //uint8_t sequence_number[S2N_TLS_SEQUENCE_NUM_LEN];
  
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

  struct s2n_cipher aead_cipher = {
    .type = S2N_AEAD,
    .io.aead.decrypt = decrypt_aead,
    .io.aead.record_iv_size = IV_SIZE,
  };
  
  struct s2n_cipher cbc_cipher = {
    .type = S2N_CBC,
    .io.cbc.decrypt = decrypt_cbc,
  };

  struct s2n_cipher composite_cipher = {
    .type = S2N_COMPOSITE,
    .io.comp.decrypt = decrypt_cbc,
  };
  struct s2n_cipher stream_cipher = {
    .type = S2N_STREAM,
    .io.stream.decrypt = decrypt_stream,
  };
  
  struct s2n_record_algorithm record_algorithm = {
    //.cipher = &aead_cipher,
        .cipher = &cbc_cipher,
    //    .cipher = &composite_cipher,
    //    .cipher = &stream_cipher,

  };
  
  struct s2n_cipher_suite cipher_suite = {
    .record_alg = &record_algorithm,
  };

  struct s2n_crypto_parameters client = {
    //.client_sequence_number = sequence_number,
    .client_record_mac = {
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
    },
    .cipher_suite = &cipher_suite,
  };

    /* uint8_t *sequence_number = conn->client->client_sequence_number; */
    /* struct s2n_hmac_state *mac = &conn->client->client_record_mac; */
    /* struct s2n_session_key *session_key = &conn->client->client_key; */
    /* const struct s2n_cipher_suite *cipher_suite = conn->client->cipher_suite; */
    /* uint8_t *implicit_iv = conn->client->client_implicit_iv; */

  
  struct s2n_connection conn = {
    .actual_protocol_version = S2N_TLS10,
    .client = &client,
  };

   struct s2n_blob en;
  
   return s2n_record_parse_test(&conn, header, payload_length, packet_size, padding_length);
}

#define SIZE  10
int test_leakage_impl(struct s2n_blob* blob){
  __VERIFIER_ASSUME_LEAKAGE(blob->data[blob->size]);
  return 0;
}

int test_leakage(int length, int val, uint8_t* data) {
  public_in(__SMACK_value(data));
  public_in(__SMACK_value(length));
  public_in(__SMACK_value(val));
  struct s2n_blob blob =  {.size = length, .data=data};
  data[length] = val;
  decrypt_cbc(0, 0, &blob, &blob);
  blob.data[length] = val;
  test_leakage_impl(&blob);
  
}
