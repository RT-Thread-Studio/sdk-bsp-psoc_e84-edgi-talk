/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-10-25     tyx          first version
 * 2026-07-20     RTT          adapt for Cortex-M55
 */

.syntax unified
.thumb
.text

.global tzcall
.type tzcall, %function
tzcall:
    SVC     1                       /* call SVC 1 */
    BX      LR

tzcall_entry:
    PUSH    {LR}
    BL      rt_secure_svc_handle
    POP     {LR}
    BX      LR

/*
 * void SVC_Handler(void);
 */
.global SVC_Handler
.type SVC_Handler, %function
SVC_Handler:
    TST     LR, #0x04
    IT      EQ
    MRSEQ   R1, MSP                 /* get fault context from handler. */
    IT      NE
    MRSNE   R1, PSP                 /* get fault context from thread. */

    LDR     R0, [R1, #24]           /* get pc */
    LDRB    R0, [R0, #-2]           /* get svc instruction */

    PUSH    {LR}

    /* if svc == 1, do TrustZone call */
    CMP     R0, #1
    BNE     svc_exit

    LDR     R0, [R1, #0]            /* svc id */
    LDR     R2, [R1, #8]            /* arg1 */
    LDR     R3, [R1, #12]           /* arg2 */
    LDR     R1, [R1, #4]            /* arg0 */
    BL      tzcall_entry

svc_exit:
    POP     {LR}
    BX      LR
