# KIT_PSE84_EVAL_EPC2 BSP Release Notes
The PSOC™ Edge E84 Evaluation Kit (KIT\_PSE84\_EVAL) is based on the PSOC™ Edge family of devices. It enables the evaluation and development of applications for the PSOC™ Edge E84 EPC2 MCU.

NOTE: BSPs are versioned by family. This means that version 1.2.0 of any BSP in a family (eg: XMC™ ) will have the same software maturity level. However, not all updates are necessarily applicable for each BSP in the family so not all version numbers will exist for each board. Additionally, new BSPs may not start at version 1.0.0. In the event of adding a common feature across all BSPs, the libraries are assigned the same version number. For example if BSP_A is at v1.3.0 and BSP_B is at v1.2.0, the event will trigger a version update to v1.4.0 for both BSP_A and BSP_B. This allows the common feature to be tracked in a consistent way.

### What's Included?
The KIT_PSE84_EVAL_EPC2 library includes the following:
* BSP specific makefile to configure the build process for the board
* cybsp.c/h files to initialize the board and any system peripherals
* cybsp_types.h file describing basic board setup
* CM33 Linker script & startup code for GCC and ARM toolchains
* CM55 Linker script & startup code for GCC and ARM toolchains
* Configurator design files (and generated code) to setup board specific peripherals
* .lib file references for all dependent libraries
* API documentation

### What Changed?
#### v1.1.0
* Removing out of date defines and improving error handling
#### v1.0.0
* BSP version update to 1.0.0
* Removed Postbuild command from bsp.mk
* ICWAEPROD-26044: Added epc2/epc4 capabilities to respective BSPs
* Changed min_tool version for EPC2 and EPC4 BSPs to 3.6.0
* Added bt-fw-mur-cyw55513 as dependency for KIT_PSE84_AI BSP
#### v0.9.5
* Updated bsp dependencies: Reverted bt-fw-ifx-cyw55500a1 version updated to release-2.2.0
* Re-enabled Wifi capability for for all suported kit BSPs
* Enable CYBSP_SDHC_DETECT (P17_7) pin for all supported kit BSPs
* Added guard macros for sec_api_link lib in bsp.mk for all supported kit BSPs
* Updated BSPs to support ModusToolbox&trade; v3.5
* Updated bsp dependencies: bt-fw-ifx-cyw55500a1 version updated to v2.X, core-make updated to v3.7.0
* Updated bsps components: CYW55513IUBG to CYW55513_MOD_PSE84_SOM
* Enabled BT FW download baud rate to 3M
* Updated PDM-PCM related configurations in design.modus for 16KHz sampling rate
* Updated system clock configurations: Turned-off DPLL_HP, DPLL_LP0 = 400MHz, DPLL_LP1 = 49.152MHz
* Renamed the following BSPs: KIT_PSE84_EVAL -> KIT_PSE84_EVAL_EPC2, KIT_PSE84S4_EVAL -> KIT_PSE84_EVAL_EPC4, KIT_PSE84_EVAL_CYW955513SDM2WLIPA -> KIT_PSE84_EVAL_EPC2_CYW955513SDM2WLIPA
#### v0.9.0
* Updated CLK_HF2 divider to 2 resulting System SRAM operates @ 200 MHz by default
* Updated BSPs to support ModusToolbox&trade; v3.4
* Updated BSPs dependencies: recipe-make-cat1d v2.1.0, bt-fw-ifx-cyw55500a1 v2.1.0, mtb-pdl-cat1 v3.14.103
* Migrated KIT BSPs to support only silicon revision 2. Added support for security configurations. Added peripheral configurations in default design.modus
* Updated the Start address for RRAM due to increase in size of CM33 L1-boot (64 KB)
* Updated bsp dependencies: recipe-make-cat1d v2.0.0, mtb-pdl-cat1 v3.100.0. Updated flash loader version to 1.0.1
* System Idle Power Mode = CPU Sleep
* Configured Bluetooth UART, DEBUG UART, USER LED1, USER LED2, USER LED3, USER BUTTON and USER BUTTON2 in design.modus for PSE84 KIT BSPs
* Added KIT_PSE84S4_EVAL BSP to support EPC4 part
* Renamed BSP for EPC2 part from KIT_PSOCE84_EVK to KIT_PSE84_EVAL
* Added syspm-callbacks-pse84 as dependency for PSE84 KIT BSPs
* Changed dependency from mtb-hal-cat1 to mtb-hal-pse84
* Removed define CY_USING_HAL from makefile to support HAL Next
* Updated minimum tools version to 3.3
#### v0.8.0
* Added bt-fw-ifx-cyw55500a1 as dependency for KIT_PSOCE84_EVK BSP and updated the Bluetooth firmware download baud rate to 3 Mbps
* Support for ModusToolbox&trade; 3.3, memory configuration personality
* Updated bsp dependencies: recipe-make-cat1d v1.0.3, core-lib v1.4.3, mtb-pdl-cat1 v3.11.102
* Renamed DEVICE_MODE to VCORE_ATTRS in bsp.mk file
#### v0.7.0
* Removed RAM linkers
* KIT_PSOCE84_EVK: CYW55513IUBG A1 silicon support in the BSP
* KIT_PSOCE84_EVK: BSP dependencies update
#### v0.6.0
* ECO (17.2032 MHz) as source for DPLL LP (DPLL_LP0, DPLL_LP1) and IHO (50 MHz) for DPLL_HP
* Reserved DPLL_LP0 as source for SMIF0 (CLK_HF3) with default 200 MHz frequency
* Reserved DPLL_LP1 (65.536 MHz) as source for CLK_HF7 and divider set to 4 to generate CLK_HF7 = 16.384 MHz
* Used DPLL_HP as source for CLK_HF0 (CM33 = 200 MHz), CLK_HF1 (CM55 = 400 MHz)
* Enabled EXT_CLK (24 MHz) on P7_4 as input to CLK_HF12 (GFXSS)
* ECO -> DPLL_LP0 (200 MHz) -> CLK_HF8 (50 MHz for USB)
* Added aliases for TRACE, USB_HOST_EN, USB_DEVICE_DET, SD_CARD_DET, EXT_CLK, BL_PWM_DISP, DISP_RST, DISP_TP_INT, DISP_TP_RST pins
* Updated bsp.mk file to support ARM compiler
* Replaced CYW955513WLIPA with CYW55513IUBG as component for connectivity chip
* Updated Deep Sleep Latency = 20 ms
* Updated VDDD, VDDA, VDDIO0 and VDDIO1 values from 3.3 mV to 1.8 mV
* Updated bsp dependencies: recipe-make-cat1d, mtb-pdl-cat1 and mtb-hal-cat1
#### v0.5.2
* System Idle Power Mode = System Deep Sleep
* Removed prebuilt binaries for CM33 secure application
* Arm compiler support
#### v0.5.1
* CLK_HF3 = 200 MHz
* System Idle Power Mode = CPU Sleep
#### v0.5.0
* Initial pre-production release

### Supported Software and Tools
This version of the KIT_PSE84_EVAL_EPC2 BSP was validated for compatibility with the following Software and Tools:

| Software and Tools                        | Version |
| :---                                      | :----:  |
| ModusToolbox™ Software Environment        | 3.6.0   |
| GCC Compiler                              | 14.2.1  |
| ARM Compiler&reg;                         | 6.16    |
| IAR Compiler                              | 9.50.1  |
| LLVM ARM Compiler                         | 19.1.1  |

Minimum required ModusToolbox™ Software Environment: v3.6.0

### More information
* [KIT_PSE84_EVAL_EPC2 BSP API Reference Manual][api]
* [KIT_PSE84_EVAL_EPC2 Documentation](https://www.infineon.com/cms/en/product/evaluation-boards/placeholder/)
* [Infineon Technologies AG](https://www.infineon.com)
* [Infineon GitHub](https://github.com/infineon)
* [ModusToolbox™](https://www.infineon.com/modustoolbox)

[api]: https://infineon.github.io/TARGET_KIT_PSE84_EVAL_EPC2/html/modules.html

---
© Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation, 2019-2023.