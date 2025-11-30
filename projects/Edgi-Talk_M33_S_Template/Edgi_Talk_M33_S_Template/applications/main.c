#include "cy_pdl.h"
#include "cybsp.h"
#include "retarget_io_init.h"
#include "cycfg_qspi_memslot.h"
#include "mtb_serial_memory.h"


/*****************************************************************************
* Macros
******************************************************************************/
#define CM33_NS_APP_BOOT_ADDR      (CYMEM_CM33_0_m33_nvm_START + \
                                       CYBSP_MCUBOOT_HEADER_SIZE) 
#define CACHE_ENABLE                    (1U)
#define ADDRESS_SIZE_IN_BYTES           (4U)
#define SMIF_MMIO_ADDRESS_OFFSET        (0U)
#define SMIF_1_PSRAM_SECURE_ADDRESS     (0x74000000U)
#define SMIF_INIT_TIMEOUT_USEC          (10000U)

static mtb_serial_memory_t serial_memory_obj;
static cy_stc_smif_mem_context_t smif_mem_context;
static cy_stc_smif_mem_info_t smif_mem_info;
static void check_status(char *message, uint32_t status)
{
    if (status)
    {
        printf("\n\r====================================================\n\r");
        printf("\n\rFAIL: %s\n\r", message);
        printf("Error Code: 0x%x\n\r", (int) status);
        printf("\n\r====================================================\n\r");
        while (true);
    }
}

static void smif_ospi_psram_init(void)
{
    cy_rslt_t result;

    /* Disable SMIF Block for reconfiguration. */
    Cy_SMIF_Disable(CYBSP_SMIF_CORE_1_PSRAM_HW);

    /* Initialize SMIF-1 Peripheral. */
    result = Cy_SMIF_Init((CYBSP_SMIF_CORE_1_PSRAM_hal_config.base),
                           (CYBSP_SMIF_CORE_1_PSRAM_hal_config.config),
                           SMIF_INIT_TIMEOUT_USEC, &smif_mem_context.smif_context);

    check_status("Cy_SMIF_Init failed", result);

    /* Configure Data Select Option for SMIF-1 */
    Cy_SMIF_SetDataSelect(CYBSP_SMIF_CORE_1_PSRAM_hal_config.base,
                          smif1BlockConfig.memConfig[0]->slaveSelect,
                          smif1BlockConfig.memConfig[0]->dataSelect);

    /* Enable the SMIF_CORE_1 block. */
    Cy_SMIF_Enable(CYBSP_SMIF_CORE_1_PSRAM_hal_config.base, &smif_mem_context.smif_context);

    /* Set-up serial memory. */
    result = mtb_serial_memory_setup(&serial_memory_obj,
                                MTB_SERIAL_MEMORY_CHIP_SELECT_2,
                                CYBSP_SMIF_CORE_1_PSRAM_hal_config.base,
                                CYBSP_SMIF_CORE_1_PSRAM_hal_config.clock,
                                &smif_mem_context,
                                &smif_mem_info,
                                &smif1BlockConfig);

    check_status("serial memory setup failed", result);
}

int main(void)
{
    uint32_t ns_stack;
    cy_cmse_funcptr NonSecure_ResetHandler;
    cy_rslt_t result;

    /* Set up internal routing, pins, and clock-to-peripheral connections */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* 
    * Initialize the clock for the APP_MMIO_TCM (512K) peripheral group.
    * This sets up the necessary clock and peripheral routing to ensure 
    * the APP_MMIO_TCM can be correctly accessed and utilized.
    */
    Cy_SysClk_PeriGroupSlaveInit(
        CY_MMIO_CM55_TCM_512K_PERI_NR, 
        CY_MMIO_CM55_TCM_512K_GROUP_NR, 
        CY_MMIO_CM55_TCM_512K_SLAVE_NR, 
        CY_MMIO_CM55_TCM_512K_CLK_HF_NR
    );

    /* 
    * Initialize the clock for the SMIF0 peripheral group.
    * This sets up the necessary clock and peripheral routing to ensure 
    * the SMIF0 can be correctly accessed and utilized.
    */
    Cy_SysClk_PeriGroupSlaveInit(
        CY_MMIO_SMIF0_PERI_NR,
        CY_MMIO_SMIF0_GROUP_NR,
        CY_MMIO_SMIF0_SLAVE_NR,
        CY_MMIO_SMIF0_CLK_HF_NR
    );

    /* Initialize SMIF in QSPI mode */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Memory protection initialization */
    result = Cy_MPC_Init();
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    init_retarget_io();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "PSOC Edge MCU: CM33 Secure Mode"
           "****************** \r\n");
#if (1U == CACHE_ENABLE)
    /* Cache attributes set to Write Back, Read & Write Allocate, to demonstrate
     * cache clean and invalidation operations. */
    cy_stc_smif_cache_region_t cache_region_0 =
    {
        .enabled = true,
        .start_address = SMIF_1_PSRAM_SECURE_ADDRESS,
        .end_address = SMIF_1_PSRAM_SECURE_ADDRESS + CY_XIP_PORT1_SIZE,
        .cache_attributes = CY_SMIF_CACHEABLE_WB_RWA
    };

    cy_stc_smif_cache_config_t cache_config =
    {
        .enabled = true,
        .cache_retention_on = true,
    };

    memcpy(&cache_config .cache_region_0, &cache_region_0, sizeof(cache_region_0));

    Cy_SMIF_InitCache(SMIF1_CACHE_BLOCK, &cache_config);

    printf("PSRAM Cache is Enabled\r\n");
#else
    printf("PSRAM Cache is Disabled\r\n");
#endif

    /* Initialize PSRAM and set-up serial memory */
    smif_ospi_psram_init();

    check_status("smif_ospi_psram_init error", (uint32_t)result);

    /* Enable XIP mode for the SMIF memory slot associated with the PSRAM. */
    result = mtb_serial_memory_enable_xip(&serial_memory_obj, true);
    check_status("mtb_serial_memory_enable_xip: failed", result);

    /* Enable write for the SMIF memory slot associated with the PSRAM. */
    result = mtb_serial_memory_set_write_enable(&serial_memory_obj, true);
    check_status("mtb_serial_memory_set_write_enable: failed", result);
    printf("PSRAM init successful \r\n");

    printf("****************** "
           "PSOC Edge MCU: CM33 Secure Mode Exit"
           "****************** \r\n\n");

    while (cy_retarget_io_is_tx_active());

    /* Peripheral protection initialization (PPC0) */
    result = Cy_PPC0_Init();
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }

    /* Peripheral protection initialization (PPC1) */
    result = Cy_PPC1_Init();
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }

    ns_stack = (uint32_t)(*((uint32_t*)CM33_NS_APP_BOOT_ADDR));
    __TZ_set_MSP_NS(ns_stack);
    
    NonSecure_ResetHandler = (cy_cmse_funcptr)(*((uint32_t*)(CM33_NS_APP_BOOT_ADDR + 4)));    /* Start non-secure application */
    NonSecure_ResetHandler();

    for (;;)
    {
    }
}
