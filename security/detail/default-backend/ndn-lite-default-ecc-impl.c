/*
 * Copyright (C) 2018-2019 Zhiyi Zhang, Tianyuan Yu, Edward Lu
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v3.0. See the file LICENSE in the top level
 * directory for more details.
 */

#include "ndn-lite-default-ecc-impl.h"
#include "ndn-lite-default-sha-impl.h"
#include "sec-lib/tinycrypt/tc_hmac.h"
#include "sec-lib/tinycrypt/tc_ecc_dh.h"
#include "sec-lib/tinycrypt/tc_constants.h"
#include "../../ndn-lite-ecc.h"
#include "../../ndn-lite-sha.h"
#include "../../ndn-lite-sec-utils.h"
#include "../../../ndn-constants.h"
#include "../../../ndn-error-code.h"
#include "../../../ndn-enums.h"
#include <string.h>

#ifndef FEATURE_PERIPH_HWRNG
typedef struct uECC_SHA256_HashContext {
  uECC_HashContext uECC;
  struct tc_sha256_state_struct ctx;
} uECC_SHA256_HashContext;

static void
_init_sha256(const uECC_HashContext *base)
{
  uECC_SHA256_HashContext *context = (uECC_SHA256_HashContext*)base;
  tc_sha256_init(&context->ctx);
}

static void
_update_sha256(const uECC_HashContext *base,
               const uint8_t *message,
               unsigned message_size)
{
  uECC_SHA256_HashContext *context = (uECC_SHA256_HashContext*)base;
  tc_sha256_update(&context->ctx, message, message_size);
}

static void
_finish_sha256(const uECC_HashContext *base, uint8_t *hash_result)
{
  uECC_SHA256_HashContext *context = (uECC_SHA256_HashContext*)base;
  tc_sha256_final(hash_result, &context->ctx);
}
#endif

uint32_t
ndn_lite_default_ecc_get_pub_key_size(const struct abstract_ecc_pub_key* pub_key)
{
  return pub_key->key_size;
}

uint32_t
ndn_lite_default_ecc_get_prv_key_size(const struct abstract_ecc_prv_key* prv_key)
{
  return prv_key->key_size;
}

const uint8_t*
ndn_lite_default_ecc_get_pub_key_value(const struct abstract_ecc_pub_key* pub_key)
{
  return pub_key->key_value;
}

int
ndn_lite_default_ecc_load_pub_key(struct abstract_ecc_pub_key* pub_key,
                                  const uint8_t* key_value, uint32_t key_size)
{
  memset(pub_key->key_value, 0, 64);
  memcpy(pub_key->key_value, key_value, key_size);
  pub_key->key_size = key_size;
  return 0;
}

int
ndn_lite_default_ecc_load_prv_key(struct abstract_ecc_prv_key* prv_key,
                                  const uint8_t* key_value, uint32_t key_size)
{
  memset(prv_key->key_value, 0, 32);
  memcpy(prv_key->key_value, key_value, key_size);
  prv_key->key_size = key_size;
  return 0;
}

int
ndn_lite_default_ecc_set_rng(ndn_rng_impl rng)
{
  tc_uECC_set_rng(rng);
  uECC_set_rng(rng);
  return NDN_SUCCESS;
}

/**
 * @note Current default backend implementation (i.e., tinycrypt) only supports curve type secp256r1.
 */
int
ndn_lite_default_ecc_dh_shared_secret(const struct abstract_ecc_pub_key* pub_abs_key,
                                      const struct abstract_ecc_prv_key* prv_abs_key,
                                      uint8_t curve_type, uint8_t* output, uint32_t output_size)
{
  if (output_size < 24)
    return NDN_SEC_DISABLED_FEATURE;
  uECC_Curve curve;
  switch (curve_type) {
  case NDN_ECDSA_CURVE_SECP160R1:
    curve = uECC_secp160r1();
    break;
  case NDN_ECDSA_CURVE_SECP192R1:
    curve = uECC_secp192r1();
    break;
  case NDN_ECDSA_CURVE_SECP224R1:
    curve = uECC_secp224r1();
    break;
  case NDN_ECDSA_CURVE_SECP256R1:
    curve = uECC_secp256r1();
    break;
  case NDN_ECDSA_CURVE_SECP256K1:
    curve = uECC_secp256k1();
    break;
  default:
    return NDN_SEC_UNSUPPORT_CRYPTO_ALGO;
  }
  int r = uECC_shared_secret(pub_abs_key->key_value, prv_abs_key->key_value, output, curve);
  if (r == 0)
    return NDN_SEC_CRYPTO_ALGO_FAILURE;
  return NDN_SUCCESS;
}

/**
 * @note Current default backend implementation (i.e., tinycrypt) only supports curve type secp256r1.
 */
int
ndn_lite_default_ecc_make_key(struct abstract_ecc_pub_key* pub_abs_key,
                              struct abstract_ecc_prv_key* prv_abs_key,
                              uint8_t curve_type)
{
  uECC_Curve curve;
  switch (curve_type) {
  case NDN_ECDSA_CURVE_SECP160R1:
    curve = uECC_secp160r1();
    break;
  case NDN_ECDSA_CURVE_SECP192R1:
    curve = uECC_secp192r1();
    break;
  case NDN_ECDSA_CURVE_SECP224R1:
    curve = uECC_secp224r1();
    break;
  case NDN_ECDSA_CURVE_SECP256R1:
    curve = uECC_secp256r1();
    break;
  case NDN_ECDSA_CURVE_SECP256K1:
    curve = uECC_secp256k1();
    break;
  default:
    return NDN_SEC_UNSUPPORT_CRYPTO_ALGO;
  }
  memset(pub_abs_key->key_value, 0, 64);
  memset(prv_abs_key->key_value, 0, 32);
  int r = uECC_make_key(pub_abs_key->key_value, prv_abs_key->key_value, curve);
  if (r == 0){
    return NDN_SEC_CRYPTO_ALGO_FAILURE;
  }
  pub_abs_key->key_size = uECC_curve_public_key_size(curve);
  prv_abs_key->key_size = uECC_curve_private_key_size(curve);
  return NDN_SUCCESS;
}

int
ndn_lite_default_ecdsa_verify(const uint8_t* input_value, uint32_t input_size,
                              const uint8_t* sig_value, uint32_t sig_size,
                              const struct abstract_ecc_pub_key* abs_key, uint8_t ecdsa_type)
{

  if (sig_size > NDN_ASN1_ECDSA_MAX_ENCODED_SIG_SIZE)
    return NDN_SEC_WRONG_SIG_SIZE;
  if (abs_key->key_size > NDN_SEC_ECC_SECP256R1_PUBLIC_KEY_SIZE)
    return NDN_SEC_WRONG_KEY_SIZE;

  uECC_Curve curve;
  uint32_t raw_signature_size = 0;
  switch(ecdsa_type){
  case NDN_ECDSA_CURVE_SECP160R1:
    curve = uECC_secp160r1();
    break;
  case NDN_ECDSA_CURVE_SECP192R1:
    curve = uECC_secp192r1();
    break;
  case NDN_ECDSA_CURVE_SECP224R1:
    curve = uECC_secp224r1();
    break;
  case NDN_ECDSA_CURVE_SECP256R1:
    curve = uECC_secp256r1();
    break;
  case NDN_ECDSA_CURVE_SECP256K1:
    curve = uECC_secp256k1();
    break;
  default:
    return NDN_SEC_UNSUPPORT_CRYPTO_ALGO;
  }
  raw_signature_size = uECC_curve_public_key_size(curve);
  uint8_t raw_sig_temp_buf[raw_signature_size];

  uint32_t decoded_raw_signature_size;
  int ret_val = ndn_asn1_decode_ecdsa_signature(sig_value, sig_size, raw_sig_temp_buf, 
                                  raw_signature_size, &decoded_raw_signature_size);
  if (ret_val != NDN_SUCCESS) {
    return ret_val;
  }

  if (uECC_verify(abs_key->key_value, input_value, input_size,
                  raw_sig_temp_buf, curve) == 0) {
    return NDN_SEC_FAIL_VERIFY_SIG;
  }
  else
    return NDN_SUCCESS;
}

int
ndn_lite_default_ecdsa_hash(uint8_t ecdsa_type, uECC_SHA256_HashContext* ctx, uECC_Curve &curve)
{
    
    switch (ecdsa_type) {
        case NDN_ECDSA_CURVE_SECP160R1:
            curve = uECC_secp160r1();
            break;
        case NDN_ECDSA_CURVE_SECP192R1:
            curve = uECC_secp192r1();
            break;
        case NDN_ECDSA_CURVE_SECP224R1:
            curve = uECC_secp224r1();
            break;
        case NDN_ECDSA_CURVE_SECP256R1:
            curve = uECC_secp256r1();
            break;
        case NDN_ECDSA_CURVE_SECP256K1:
            curve = uECC_secp256k1();
            break;
        default:
            return NDN_SEC_UNSUPPORT_CRYPTO_ALGO;
    }
    
    uint8_t tmp[NDN_SEC_ECC_SECP256R1_PRIVATE_KEY_SIZE +
                NDN_SEC_ECC_SECP256R1_PRIVATE_KEY_SIZE +
                NDN_SEC_ECC_SECP256R1_PUBLIC_KEY_SIZE];
    uECC_SHA256_HashContext HashContext;
    ctx = &HashContext;
    ctx->uECC.init_hash = &_init_sha256;
    ctx->uECC.update_hash = &_update_sha256;
    ctx->uECC.finish_hash = &_finish_sha256;
    ctx->uECC.block_size = NDN_SEC_ECC_SECP256R1_PUBLIC_KEY_SIZE;
    ctx->uECC.result_size = NDN_SEC_ECC_SECP256R1_PRIVATE_KEY_SIZE;
    ctx->uECC.tmp = tmp;
    
    return NDN_SUCCESS;
}




int
ndn_lite_default_ecdsa_sign(const uint8_t* input_value, uint32_t input_size,
                            uint8_t* output_value, uint32_t output_max_size,
                            const struct abstract_ecc_prv_key* abs_key,
                            uint8_t ecdsa_type, uint32_t* output_used_size)
{
    if (output_max_size < NDN_SEC_ECC_SECP256R1_PUBLIC_KEY_SIZE)
        return NDN_OVERSIZE;
    if (abs_key->key_size > NDN_SEC_ECC_SECP256R1_PRIVATE_KEY_SIZE)
        return NDN_SEC_WRONG_KEY_SIZE;
    
    uECC_Curve curve;
    uECC_SHA256_HashContext* ctx;
    hash_result = ndn_lite_default_ecdsa_hash(ecdsa_type, ctx, curve);
    if (hash_result != NDN_SUCCESS)
        return hash_result;
    
    uint32_t signature_size = 0;
    signature_size = uECC_curve_public_key_size(curve);
    
    int ecc_sign_result = 0;
    
#ifndef FEATURE_PERIPH_HWRNG
    ecc_sign_result = uECC_sign_deterministic(abs_key->key_value, input_value, input_size,
                                              &ctx->uECC, output_value, curve);
#else
    ecc_sign_result = uECC_sign(abs_key->key_value, input_value, input_size,
                                output_value, curve);
#endif
    if (ecc_sign_result == 0) {
        return NDN_SEC_CRYPTO_ALGO_FAILURE;
    }
    
    uint32_t encoded_sig_length;
    int ret_val = ndn_asn1_probe_ecdsa_signature_encoding_size(output_value, signature_size,
                                                               &encoded_sig_length);
    if (ret_val != NDN_SUCCESS) {
        return ret_val;
    }
    
    ret_val = ndn_asn1_encode_ecdsa_signature(output_value, signature_size, output_max_size);
    if (ret_val != NDN_SUCCESS) {
        return ret_val;
    }
    
    *output_used_size = encoded_sig_length;
    
    return NDN_SUCCESS;
}


void
ndn_lite_default_ecc_load_backend(void)
{
  ndn_ecc_backend_t* ecc_back = ndn_ecc_get_backend();
  ecc_back->get_pub_key_size = ndn_lite_default_ecc_get_pub_key_size;
  ecc_back->get_prv_key_size = ndn_lite_default_ecc_get_prv_key_size;
  ecc_back->get_pub_key_value = ndn_lite_default_ecc_get_pub_key_value;
  ecc_back->load_pub_key = ndn_lite_default_ecc_load_pub_key;
  ecc_back->load_prv_key = ndn_lite_default_ecc_load_prv_key;
  ecc_back->set_rng = ndn_lite_default_ecc_set_rng;
  ecc_back->make_key = ndn_lite_default_ecc_make_key;
  ecc_back->dh_shared_secret = ndn_lite_default_ecc_dh_shared_secret;
  ecc_back->ecdsa_sign = ndn_lite_default_ecdsa_sign;
  ecc_back->ecdsa_verify = ndn_lite_default_ecdsa_verify;
}
