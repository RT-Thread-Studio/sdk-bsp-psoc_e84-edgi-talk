/*
 * BTstack configuration for the Edgi-Talk Bluetooth A2DP speaker example.
 *
 * Classic Bluetooth only: A2DP Sink + AVRCP + SBC decoder.  POSIX file I/O,
 * stdin and the BLE stack are intentionally not enabled: the RT-Thread port
 * lives in this directory and talks to the CYW55512 directly over uart4.
 */

#ifndef __BTSTACK_CONFIG
#define __BTSTACK_CONFIG

/* Use the C library heap for BTstack's dynamic pools. */
#define HAVE_MALLOC

/* Classic Bluetooth profiles. */
#define ENABLE_CLASSIC
#define ENABLE_LOG_ERROR

/*
 * HCI buffers.  A2DP media packets can be close to the L2CAP MTU; keep the
 * same values that the upstream BTstack A2DP example uses.
 */
#define HCI_INCOMING_PRE_BUFFER_SIZE 14
#define HCI_ACL_PAYLOAD_SIZE         (1691 + 4)

#endif /* __BTSTACK_CONFIG */
