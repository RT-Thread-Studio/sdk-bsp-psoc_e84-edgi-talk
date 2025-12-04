/*
 * $ Copyright Cypress Semiconductor $
*/

#ifndef LIBS_CONNECTIVITY_MANAGER_INTERNAL_HEADER_H_
#define LIBS_CONNECTIVITY_MANAGER_INTERNAL_HEADER_H_

#if defined(CYBSP_ETHERNET_CAPABLE)
#ifdef COMPONENT_CAT1
#include "cy_ethif.h"
#include "cy_ephy.h"
#include "cycfg.h"

/******************************************************
 *                      Macros
 ******************************************************/
/**
 * Maximum number of interface instances supported.
 */
#define CY_IFACE_MAX_HANDLE    (4U)
#ifndef ETH_RX_NO_OF_BUFFERS
    #define ETH_RX_NO_OF_BUFFERS   (30)
#endif

#if defined CYCFG_PBUF_POOL_SIZE
    #define ETH_RX_NO_OF_MEMPOOL_ELEMENTS   (CYCFG_PBUF_POOL_SIZE)
#else
    #define ETH_RX_NO_OF_MEMPOOL_ELEMENTS   (50)
#endif
#define MEM_BYTE_ALIGNMENT     (32)
/******************************************************
 *                      Type definitions
 ******************************************************/
/** Buffer created from the pool */
typedef void *cy_buffer_t;

CY_ALIGN(32)
typedef struct cy_rx_buffer_info
{
    uint8_t *rx_data_ptr;  /* Pointer to the actual buffer created */
    uint32_t eth_idx;
    uint32_t length;
    uint32_t reserved[5];
} cy_rx_buffer_info_t;

CY_ALIGN(32)
typedef struct list_node_t
{
    void *pool_handle; /* Stores the pool handle */
    struct list_node_t *next; /* Points to the next buffer in the list */
    uint8_t *buffer_ptr; /* Pointer to the actual buffer created */
    uint32_t reserved[5];
} list_node_t;

typedef struct
{
    uint16_t total_num_buf_created; /* Total number of buffers created */
    uint16_t sizeof_buffer; /* Size of each created buffer */
    uint8_t *data_buffer; /* Base address of the data buffer */
    list_node_t *head; /* Pointer to the head node in the buffer list */
} cy_internal_buffer_pool_t;

typedef struct rx_buf_info
{
    uint32_t rx_data_ptr; /* Pointer to the actual buffer created */
    uint32_t length;
} rx_buf_info_t;

typedef struct my_custom_pbuf
{
    struct pbuf_custom pbuf_custom;
    cy_rx_buffer_info_t *pbuffer;
} my_custom_pbuf_t;

#endif
#endif

#endif /* LIBS_CONNECTIVITY_MANAGER_INTERNAL_HEADER_H_ */
