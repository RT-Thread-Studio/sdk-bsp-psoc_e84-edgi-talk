/*******************************************************************************
 * File Name:   retarget_io_init.c
 *
 * Description: Initialization routine for retarget-io on the secure project.
 *******************************************************************************/

#include "retarget_io_init.h"

#if SECURE_RETARGET_IO_ENABLED
static cy_stc_scb_uart_context_t DEBUG_UART_context;
static mtb_hal_uart_t DEBUG_UART_hal_obj;

#if defined(CYBSP_DEBUG_UART_M33_TX_PORT)
#define SECURE_DEBUG_UART_TX_PORT CYBSP_DEBUG_UART_M33_TX_PORT
#define SECURE_DEBUG_UART_TX_PIN  CYBSP_DEBUG_UART_M33_TX_PIN
#define SECURE_DEBUG_UART_TX_HSIOM CYBSP_DEBUG_UART_M33_TX_HSIOM
#endif

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP) && defined(SECURE_DEBUG_UART_TX_PORT)
static mtb_syspm_uart_deepsleep_context_t retarget_io_syspm_ds_context =
{
    .uart_context = &DEBUG_UART_context,
    .async_context = NULL,
    .tx_pin =
    {
        .port = SECURE_DEBUG_UART_TX_PORT,
        .pinNum = SECURE_DEBUG_UART_TX_PIN,
        .hsiom = SECURE_DEBUG_UART_TX_HSIOM
    },
    .rts_pin =
    {
        .port = DEBUG_UART_RTS_PORT,
        .pinNum = DEBUG_UART_RTS_PIN,
        .hsiom = HSIOM_SEL_GPIO
    }
};

static cy_stc_syspm_callback_params_t retarget_io_syspm_cb_params =
{
    .context = &retarget_io_syspm_ds_context,
    .base = CYBSP_DEBUG_UART_M33_HW
};

static cy_stc_syspm_callback_t retarget_io_syspm_cb =
{
    .callback = &mtb_syspm_scb_uart_deepsleep_callback,
    .skipMode = SYSPM_SKIP_MODE,
    .type = CY_SYSPM_DEEPSLEEP,
    .callbackParams = &retarget_io_syspm_cb_params,
    .prevItm = NULL,
    .nextItm = NULL,
    .order = SYSPM_CALLBACK_ORDER
};
#endif
#endif

void init_retarget_io(void)
{
#if SECURE_RETARGET_IO_ENABLED
    cy_rslt_t result = CY_RSLT_SUCCESS;

    result = (cy_rslt_t)Cy_SCB_UART_Init(CYBSP_DEBUG_UART_M33_HW,
                                         &CYBSP_DEBUG_UART_M33_config,
                                         &DEBUG_UART_context);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }

    Cy_SCB_UART_Enable(CYBSP_DEBUG_UART_M33_HW);

    result = mtb_hal_uart_setup(&DEBUG_UART_hal_obj,
                                &CYBSP_DEBUG_UART_M33_hal_config,
                                &DEBUG_UART_context,
                                NULL);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }

    result = cy_retarget_io_init(&DEBUG_UART_hal_obj);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }

#if (CY_CFG_PWR_SYS_IDLE_MODE == CY_CFG_PWR_MODE_DEEPSLEEP) && defined(SECURE_DEBUG_UART_TX_PORT)
    Cy_SysPm_RegisterCallback(&retarget_io_syspm_cb);
#endif
#else
    /* The M33 debug UART is disabled in Device Configurator. Keep secure
     * boot silent instead of referencing missing generated UART symbols. */
#endif
}
