/*
 * Copyright (C) Edward Lu
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * See AUTHORS.md for complete list of NDN IOT PKG authors and contributors.
 */

#include "sign-on-basic-client-nrf-sdk-ble.h"

#include "../secure-sign-on/variants/basic/sign-on-basic-client.h"
#include "../../../../adaptation/ndn-nrf-ble-adaptation/nrf-sdk-ble-consts.h"

#include "../../../../adaptation/ndn-nrf-ble-adaptation/logger.h"

#define TEMP_BUF_LENGTH 500

static struct sign_on_basic_client_t m_sign_on_basic_client;
static sign_on_basic_client_nrf_sdk_ble_t sign_on_basic_client_nrf_sdk_ble;
void (*m_on_sign_on_completed)(enum sign_on_basic_client_nrf_sdk_ble_completed_result result); //**< Callback for sign on completion set by sign_on_basic_client_ble_init_result. */

sign_on_basic_client_nrf_sdk_ble_t *
get_sign_on_basic_client_nrf_sdk_ble_instance() {
  return &sign_on_basic_client_nrf_sdk_ble;
}

void m_on_transport_connected(uint16_t conn_handle) {

}

void m_on_transport_disconnected() {

}

void m_on_transport_hvn_tx_complete(uint16_t conn_handle) {

}

void m_on_transport_adv_stopped() {
  APP_LOG("in sign-on-basic-client-ble.c, m_on_transport_adv_stopped got called\n");
}

void m_on_transport_mtu_rqst() {
  // after the central is done negotiating its MTU with us, we send our bootstrapping request

  APP_LOG("in sign-on-basic-client-ble.c, m_on_transport_mtu_rqst got called\n");

  ret_code_t err_code;

  uint16_t bootstrappingRequestBufLength = TEMP_BUF_LENGTH;
  uint8_t bootstrappingRequestBuf[TEMP_BUF_LENGTH];
  uint16_t bootstrappingRequestLength = 0;

  enum cnstrct_btstrp_rqst_result cnstrct_result;
  cnstrct_result = cnstrct_btstrp_rqst(bootstrappingRequestBuf, bootstrappingRequestBufLength, &bootstrappingRequestLength,
      &m_sign_on_basic_client);
  if (cnstrct_result != CNSTRCT_BTSTRP_RQST_SUCCESS) {
    APP_LOG("Construction of bootstrapping request failed, error code %d\n", cnstrct_result);
    m_on_sign_on_completed(SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_COMPLETED_FAILED_TO_GENERATE_BOOTSTRAPPING_REQUEST);
    return;
  }

  APP_LOG("Value of bootstrappingRequestLength after constructing bootstrapping request: %u\n",
      bootstrappingRequestLength);

  if (nrf_sdk_ble_ndn_lite_ble_unicast_transport_send(bootstrappingRequestBuf, 
      bootstrappingRequestLength) != NRF_BLE_OP_SUCCESS) {
    APP_LOG("in sign-on-basic-client-ble.c, m_on_transport_mtu_rqst, ndn_lite_ble_unicast_transport_send failed.\n");
    m_on_sign_on_completed(SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_COMPLETED_FAILED_TO_SEND_BOOTSTRAPPING_REQUEST);
  }
}

void m_on_recvd_data_callback(const uint8_t *payload, uint16_t payload_len) {

  if (m_sign_on_basic_client.status == SIGN_ON_BASIC_CLIENT_GENERATED_FINISH_MESSAGE) {
    APP_LOG("In sign-on-basic-client-ble, saw that sign on client had already successfully generated finish message; ignoring"
             " this packet.\n");
    return;
  }
  else {
    APP_LOG("In sign-on-basic-client-ble, received a packet; current state of sign on basic client: %d\n",
      m_sign_on_basic_client.status);
  }

  if (payload_len < 1) {
    APP_LOG("Data we received was less than 1 in length, ignoring it.\n");
    return;
  }

  if (payload[0] == SECURE_SIGN_ON_BOOTSTRAPPING_REQUEST_RESPONSE_TLV_TYPE) {
    APP_LOG("Received bootstrapping request response.\n");

    enum prcs_btstrp_rqst_rspns_result result;

    result = prcs_btstrp_rqst_rspns(payload, payload_len, &m_sign_on_basic_client);
    if (result != PRCS_BTSTRP_RQST_RSPNS_SUCCESS) {
      APP_LOG("Error processing bootstrapping request response: %d", result);
      return;
    }

    APP_LOG_HEX("Value of N2 pub after processing bootstrapping request response:", 
        m_sign_on_basic_client.N2_pub_p,
        m_sign_on_basic_client.N2_pub_len);
    APP_LOG_HEX("Value of trust anchor certificate after processing bootstrapping request response:",
        m_sign_on_basic_client.trust_anchor_cert_p, 
        m_sign_on_basic_client.trust_anchor_cert_len);

    ret_code_t err_code;

    uint16_t certificateRequestBufLength = TEMP_BUF_LENGTH;
    uint8_t certificateRequestBuf[TEMP_BUF_LENGTH];
    uint16_t certificateRequestLength = 0;

    enum cnstrct_cert_rqst_result cnstrct_result;
    cnstrct_result = cnstrct_cert_rqst(certificateRequestBuf, certificateRequestBufLength, &certificateRequestLength,
        &m_sign_on_basic_client);
    if (cnstrct_result != CNSTRCT_CERT_RQST_SUCCESS) {
      APP_LOG("Construction of certificate request failed, error code %d\n", cnstrct_result);
      m_on_sign_on_completed(SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_COMPLETED_FAILED_TO_GENERATE_CERTIFICATE_REQUEST);
      return;
    }

    APP_LOG("Value of certificateRequestLength after constructing certificate request: %u\n",
        certificateRequestLength);

    if (nrf_sdk_ble_ndn_lite_ble_unicast_transport_send(certificateRequestBuf, certificateRequestLength) != NRF_BLE_OP_SUCCESS) {
      APP_LOG("in sign-on-basic-client-ble.c, m_on_recvd_data_callback, ndn_lite_ble_unicast_transport_send failed"
        " for certificate request.\n");
      m_on_sign_on_completed(SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_COMPLETED_FAILED_TO_SEND_CERTIFICATE_REQUEST);
    }

  } else if (payload[0] == SECURE_SIGN_ON_CERTIFICATE_REQUEST_RESPONSE_TLV_TYPE) {
    APP_LOG("Received certificate request response.\n");

    enum prcs_cert_rqst_rspns_result result;
    result = prcs_cert_rqst_rspns(payload, payload_len, &m_sign_on_basic_client);
    if (result != PRCS_CERT_RQST_RSPNS_SUCCESS) {
      APP_LOG("Error processing certificate request response: %d\n", result);
      return;
    }

    APP_LOG_HEX("Value of Kd pri after processing certificate request response:", 
        m_sign_on_basic_client.KD_pri_p,
        m_sign_on_basic_client.KD_pri_len);

    APP_LOG_HEX("Value of Kd pub certificate after processing certificate request respnose:",
        m_sign_on_basic_client.KD_pub_cert_p, 
        m_sign_on_basic_client.KD_pub_cert_len);

    APP_LOG_HEX("Contents of certificate request response:", payload, payload_len);

    uint16_t finishMessageBufLength = TEMP_BUF_LENGTH;
    uint8_t finishMessageBuf[finishMessageBufLength];
    uint16_t finishMessageLength;

    enum cnstrct_fin_msg_result cnstrct_result;
    cnstrct_result = cnstrct_fin_msg(finishMessageBuf, finishMessageBufLength, &finishMessageLength,
      &m_sign_on_basic_client);
    if (cnstrct_result != CNSTRCT_FIN_MSG_SUCCESS) {
      APP_LOG("Construction of finish message failed, error code %d\n", cnstrct_result);
      m_on_sign_on_completed(SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_COMPLETED_FAILED_TO_GENERATE_FINISH_MESSAGE);
      return;
    }

    if (nrf_sdk_ble_ndn_lite_ble_unicast_transport_send(finishMessageBuf, finishMessageBufLength) != NRF_BLE_OP_SUCCESS) {
      APP_LOG("in sign-on-basic-client-ble.c, m_on_recvd_data_callback, ndn_lite_ble_unicast_transport_send failed"
        " for finish message.\n");
      m_on_sign_on_completed(SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_COMPLETED_FAILED_TO_SEND_FINISH_MESSAGE);
    }

    sign_on_basic_client_nrf_sdk_ble.KD_pri_p = m_sign_on_basic_client.KD_pri_p;
    sign_on_basic_client_nrf_sdk_ble.KD_pri_len = m_sign_on_basic_client.KD_pri_len;
    sign_on_basic_client_nrf_sdk_ble.KD_pub_cert_p = m_sign_on_basic_client.KD_pub_cert_p;
    sign_on_basic_client_nrf_sdk_ble.KD_pub_cert_len = m_sign_on_basic_client.KD_pub_cert_len;
    sign_on_basic_client_nrf_sdk_ble.trust_anchor_cert_p = m_sign_on_basic_client.trust_anchor_cert_p;
    sign_on_basic_client_nrf_sdk_ble.trust_anchor_cert_len = m_sign_on_basic_client.trust_anchor_cert_len;

    m_on_sign_on_completed(SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_COMPLETED_SUCCESS);

  }
}

enum sign_on_basic_client_nrf_sdk_ble_construct_result sign_on_basic_client_nrf_sdk_ble_construct(uint8_t variant,
                                   const uint8_t *device_identifier_p, uint16_t device_identifier_len,
                                   const uint8_t *device_capabilities_p, uint16_t device_capabilities_len,
                                   const uint8_t *secure_sign_on_code_p,
                                   const uint8_t *KS_pub_p, uint16_t KS_pub_len,
                                   const uint8_t *KS_pri_p, uint16_t KS_pri_len,
                                   void (*on_sign_on_completed)(enum sign_on_basic_client_nrf_sdk_ble_completed_result result)) {

  m_on_sign_on_completed = on_sign_on_completed;
  
  nrf_sdk_ble_ndn_lite_ble_unicast_transport_init();
  nrf_sdk_ble_ndn_lite_ble_unicast_transport_observer_t observer;
  observer.on_connected = m_on_transport_connected;
  observer.on_disconnected = m_on_transport_disconnected;
  observer.on_mtu_rqst = m_on_transport_mtu_rqst;
  observer.on_hvn_tx_complete = m_on_transport_hvn_tx_complete;
  observer.on_recvd_data = m_on_recvd_data_callback;
  observer.on_adv_stopped = m_on_transport_adv_stopped;
  nrf_sdk_ble_ndn_lite_ble_unicast_transport_add_observer(observer);

  enum sign_on_basic_client_init_result sign_on_client_init_result;
  sign_on_client_init_result = sign_on_basic_client_init(variant,
                                                         &m_sign_on_basic_client,
                                                         device_identifier_p, device_identifier_len,
                                                         device_capabilities_p, device_capabilities_len,
                                                         secure_sign_on_code_p,
                                                         KS_pub_p, KS_pub_len,
                                                         KS_pri_p, KS_pri_len);

  if (sign_on_client_init_result != SIGN_ON_BASIC_CLIENT_INIT_SUCCESS) {
    APP_LOG("Initialization of sign on client failed.");
    APP_LOG("Error number: %d", sign_on_client_init_result);
    return SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_CONSTRUCT_FAILED_TO_INITIALIZE_SIGN_ON_BASIC_CLIENT;
  }
  
  return SIGN_ON_BASIC_CLIENT_NRF_SDK_BLE_CONSTRUCT_SUCCESS;

}