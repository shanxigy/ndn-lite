
/*
 * Copyright (C) Edward Lu
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 * See AUTHORS.md for complete list of NDN IOT PKG authors and contributors.
 */

#ifndef NRF_SDK_BLE_GATT_H
#define NRF_SDK_BLE_GATT_H

#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"

// ALL FUNCTIONS BELOW THAT RETURN INTEGERS RETURN NRF_BLE_OP_SUCCESS UPON SUCCESS,
// AND NRF_BLE_OP_FAILURE UPON FAILURE.

/**@brief Function for initializing the GATT module. 
*/
int nrf_sdk_ble_gatt_init(void);

#endif // NRF_SDK_BLE_GATT_H