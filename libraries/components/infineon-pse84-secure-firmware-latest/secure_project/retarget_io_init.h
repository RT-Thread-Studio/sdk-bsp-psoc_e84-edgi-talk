/*******************************************************************************
 * File Name:   retarget_io_init.h
 *
 * Description: Public interface for secure-project debug UART retarget-io init.
 *******************************************************************************/

#ifndef _RETARGET_IO_INIT_H_
#define _RETARGET_IO_INIT_H_

#include "cybsp.h"
#include "mtb_hal.h"
#include "mtb_syspm_callbacks.h"

#if defined(CYBSP_DEBUG_UART_M33_ENABLED) && defined(CYBSP_DEBUG_UART_M33_HW)
#include <stdio.h>
#include "cy_retarget_io.h"
#define SECURE_RETARGET_IO_ENABLED (1U)
#define SECURE_PRINTF(...) printf(__VA_ARGS__)
#define SECURE_WAIT_TX_DONE() while (cy_retarget_io_is_tx_active())
#else
#define SECURE_RETARGET_IO_ENABLED (0U)
#define SECURE_PRINTF(...) ((void)0)
#define SECURE_WAIT_TX_DONE() ((void)0)
#endif

#define DEBUG_UART_RTS_PORT     (NULL)
#define DEBUG_UART_RTS_PIN      (0U)

#define SYSPM_SKIP_MODE         (0U)
#define SYSPM_CALLBACK_ORDER    (1U)

void init_retarget_io(void);

__STATIC_INLINE void handle_error(void)
{
    __disable_irq();
    CY_ASSERT(0);
    while (true)
    {
    }
}

#endif /* _RETARGET_IO_INIT_H_ */
