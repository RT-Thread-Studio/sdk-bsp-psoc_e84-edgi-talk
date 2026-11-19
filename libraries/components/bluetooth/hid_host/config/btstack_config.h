/* SPDX-License-Identifier: Apache-2.0 */
#ifndef EDGI_CLASSIC_BTSTACK_CONFIG_H
#define EDGI_CLASSIC_BTSTACK_CONFIG_H

#define ENABLE_CLASSIC
#define ENABLE_BLE
#define ENABLE_LE_CENTRAL
/* v1.6.2 shares the peripheral GAP privacy state with its scan scheduler. */
#define ENABLE_LE_PERIPHERAL
#define ENABLE_LE_SECURE_CONNECTIONS
#define ENABLE_MICRO_ECC_P256
#define ENABLE_SDP_CLIENT
#define HCI_ACL_PAYLOAD_SIZE                     1021
#define MAX_NR_HCI_CONNECTIONS                   1
#define MAX_NR_HID_HOST_CONNECTIONS              1
#define MAX_NR_GATT_CLIENTS                      1
#define MAX_NR_SM_LOOKUP_ENTRIES                 2
/* Eight saved peers and one spare entry for a new pairing. */
#define MAX_NR_LE_DEVICE_DB_ENTRIES              9
#define MAX_NR_L2CAP_CHANNELS                    4
#define MAX_NR_L2CAP_SERVICES                    3
#define MAX_NR_SERVICE_RECORD_ITEMS              2
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 9

#endif
