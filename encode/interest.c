/*
 * Copyright (C) 2018 Zhiyi Zhang, Edward Lu
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v3.0. See the file LICENSE in the top level
 * directory for more details.
 */

#include "interest.h"

/************************************************************/
/*  Definition of helper functions                          */
/************************************************************/

// get the length of tlv's v of the interest
static uint32_t
ndn_interest_probe_block_value_size(const ndn_interest_t* interest)
{
  uint32_t interest_buffer_size = ndn_name_probe_block_size(&interest->name);
  // can be prefix
  if (interest->enable_CanBePrefix)
    interest_buffer_size += 2;
  // must be fresh
  if (interest->enable_MustBeFresh)
    interest_buffer_size += 2;
  // nonce
  interest_buffer_size += 6;
  // life time
  interest_buffer_size += 2 + encoder_probe_uint_length(interest->lifetime); // lifetime
  // hop limit
  if (interest->enable_HopLimit)
    interest_buffer_size += 3;
  // parameters
  if (interest->enable_Parameters)
    interest_buffer_size += encoder_probe_block_size(TLV_Parameters, interest->parameters.size);
  if (interest->is_SignedInterest > 0) {
    // signature info
    interest_buffer_size += ndn_signature_info_probe_block_size(&interest->signature);
    // signature value
    interest_buffer_size += ndn_signature_value_probe_block_size(&interest->signature);
  }
  return interest_buffer_size;
}

/************************************************************/
/*  Definition of Interest APIs                             */
/************************************************************/

int
ndn_interest_from_block(ndn_interest_t* interest, const uint8_t* block_value, uint32_t block_size)
{

  int ret_val = -1;
  
  ndn_interest_init(interest);
  ndn_decoder_t decoder;
  decoder_init(&decoder, block_value, block_size);
  uint32_t type = 0;
  ret_val = decoder_get_type(&decoder, &type);
  if (ret_val != NDN_SUCCESS) return ret_val;
  if (type != TLV_Interest) {
    return NDN_WRONG_TLV_TYPE;
  }
  uint32_t interest_buffer_length = 0;
  ret_val = decoder_get_length(&decoder, &interest_buffer_length);
  if (ret_val != NDN_SUCCESS) return ret_val;

  // name
  int result = ndn_name_tlv_decode(&decoder, &interest->name);
  if (result < 0) {
    return result;
  }
  while (decoder.offset < block_size) {
    ret_val = decoder_get_type(&decoder, &type);
    uint32_t length = 0;
    if (type == TLV_CanBePrefix) {
      interest->enable_CanBePrefix = 1;
      ret_val = decoder_get_length(&decoder, &length);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else if (type == TLV_MustBeFresh) {
      interest->enable_MustBeFresh = 1;
      ret_val = decoder_get_length(&decoder, &length);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else if (type == TLV_Nonce) {
      ret_val = decoder_get_length(&decoder, &length);
      if (ret_val != NDN_SUCCESS) return ret_val;
      ret_val = decoder_get_uint32_value(&decoder, &interest->nonce);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else if (type == TLV_InterestLifetime) {
      ret_val = decoder_get_length(&decoder, &length);
      if (ret_val != NDN_SUCCESS) return ret_val;
      ret_val = decoder_get_uint_value(&decoder, length, &interest->lifetime);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else if (type == TLV_HopLimit) {
      interest->enable_HopLimit = 1;
      ret_val = decoder_get_length(&decoder, &length);
      if (ret_val != NDN_SUCCESS) return ret_val;
      ret_val = decoder_get_byte_value(&decoder, &interest->hop_limit);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else if (type == TLV_Parameters) {
      interest->enable_Parameters = 1;
      ret_val = decoder_get_length(&decoder, &interest->parameters.size);
      if (ret_val != NDN_SUCCESS) return ret_val;
      ret_val = decoder_get_raw_buffer_value(&decoder, interest->parameters.value,
				   interest->parameters.size);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else if (type == TLV_SignatureInfo) {
      interest->is_SignedInterest = 1;
      ret_val = decoder_move_backward(&decoder, encoder_get_var_size(TLV_SignatureInfo));
      if (ret_val != NDN_SUCCESS) return ret_val;
      ret_val = ndn_signature_info_tlv_decode(&decoder, &interest->signature);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else if (type == TLV_SignatureValue) {
      interest->is_SignedInterest = 1;
      ret_val = decoder_move_backward(&decoder, encoder_get_var_size(TLV_SignatureValue));
      if (ret_val != NDN_SUCCESS) return ret_val;
      ret_val = ndn_signature_value_tlv_decode(&decoder, &interest->signature);
      if (ret_val != NDN_SUCCESS) return ret_val;
    }
    else {
      return NDN_WRONG_TLV_TYPE;
    }
  }
  return 0;
}

int
ndn_interest_tlv_encode(ndn_encoder_t* encoder, const ndn_interest_t* interest)
{

  int ret_val = -1;
  
  if (interest->is_SignedInterest <= 0 && interest->enable_Parameters) {
    // TODO added by Zhiyi: add a InterestParameters Digest Name component to Interest.name
  }

  uint32_t interest_block_value_size = ndn_interest_probe_block_value_size(interest);
  int required_size = encoder_probe_block_size(TLV_Interest, interest_block_value_size);
  int rest_size = encoder->output_max_size - encoder->offset;
  if (required_size > rest_size)
    return NDN_OVERSIZE;

  ret_val = encoder_append_type(encoder, TLV_Interest);
  if (ret_val != NDN_SUCCESS) return ret_val;
  ret_val = encoder_append_length(encoder, interest_block_value_size);
  if (ret_val != NDN_SUCCESS) return ret_val;
  // name
  ret_val = ndn_name_tlv_encode(encoder, &interest->name);
  if (ret_val != NDN_SUCCESS) return ret_val;
  // can be prefix
  if (interest->enable_CanBePrefix > 0) {
    ret_val = encoder_append_type(encoder, TLV_CanBePrefix);
    if (ret_val != NDN_SUCCESS) return ret_val;
    ret_val = encoder_append_length(encoder, 0);
    if (ret_val != NDN_SUCCESS) return ret_val;
  }
  // must be fresh
  if (interest->enable_MustBeFresh > 0) {
    ret_val = encoder_append_type(encoder, TLV_MustBeFresh);
    if (ret_val != NDN_SUCCESS) return ret_val;
    ret_val = encoder_append_length(encoder, 0);
    if (ret_val != NDN_SUCCESS) return ret_val;
  }
  // nonce
  ret_val = encoder_append_type(encoder, TLV_Nonce);
  if (ret_val != NDN_SUCCESS) return ret_val;
  ret_val = encoder_append_length(encoder, 4);
  if (ret_val != NDN_SUCCESS) return ret_val;
  ret_val = encoder_append_uint32_value(encoder, interest->nonce);
  if (ret_val != NDN_SUCCESS) return ret_val;
  // lifetime
  ret_val = encoder_append_type(encoder, TLV_InterestLifetime);
  if (ret_val != NDN_SUCCESS) return ret_val;
  ret_val = encoder_append_length(encoder, encoder_probe_uint_length(interest->lifetime));
  if (ret_val != NDN_SUCCESS) return ret_val;
  ret_val = encoder_append_uint_value(encoder, interest->lifetime);
  if (ret_val != NDN_SUCCESS) return ret_val;
  // hop limit
  if (interest->enable_HopLimit > 0) {
    ret_val = encoder_append_type(encoder, TLV_HopLimit);
    if (ret_val != NDN_SUCCESS) return ret_val;
    ret_val = encoder_append_length(encoder, 1);
    if (ret_val != NDN_SUCCESS) return ret_val;
    ret_val = encoder_append_byte_value(encoder, interest->hop_limit);
    if (ret_val != NDN_SUCCESS) return ret_val;
  }
  // parameters
  if (interest->enable_Parameters > 0) {
    ret_val = encoder_append_type(encoder, TLV_Parameters);
    if (ret_val != NDN_SUCCESS) return ret_val;
    ret_val = encoder_append_length(encoder, interest->parameters.size);
    if (ret_val != NDN_SUCCESS) return ret_val;
    ret_val = encoder_append_raw_buffer_value(encoder, interest->parameters.value, interest->parameters.size);
    if (ret_val != NDN_SUCCESS) return ret_val;
  }
  if (interest->is_SignedInterest > 0) {
    // signature info
    ret_val = ndn_signature_info_tlv_encode(encoder, &interest->signature);
    if (ret_val != NDN_SUCCESS) return ret_val;
    // signature value
    ret_val = ndn_signature_value_tlv_encode(encoder, &interest->signature);
    if (ret_val != NDN_SUCCESS) return ret_val;
  }
  return 0;
}
