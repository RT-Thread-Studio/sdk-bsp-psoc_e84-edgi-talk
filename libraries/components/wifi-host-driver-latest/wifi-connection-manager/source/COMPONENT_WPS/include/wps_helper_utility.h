/*
 * (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may use
 * this Software only as provided in the license agreement accompanying the
 * software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation, or
 * compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
 * SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
 * Infineon reserves the right to make changes to the Software without notice.
 * You are responsible for properly designing, programming, and testing the
 * functionality and safety of your intended application of the Software, as
 * well as complying with any legal requirements related to its use. Infineon
 * does not guarantee that the Software will be free from intrusion, data theft
 * or loss, or other breaches ("Security Breaches"), and Infineon shall have
 * no liability arising out of any Security Breaches. Unless otherwise
 * explicitly approved by Infineon, the Software may not be used in any
 * application where a failure of the Product or any consequences of the use
 * thereof can reasonably be expected to result in personal injury.
 */
/**
* @file wps_helper_utility.h
* @brief Utility functions for WPS
*/
#pragma once

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file WPS helper functions
 *
 *  WPS Utility functions
 */

/******************************************************
 *                      Macros
 ******************************************************/

#ifdef LINT
/* Lint does not know about inline functions */
extern uint16_t htobe16(uint16_t v);
extern uint32_t htobe32(uint32_t v);

#else /* ifdef LINT */

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE_PRE
//#define ALWAYS_INLINE    __attribute__((always_inline))
/* Fixme: ALWAYS_INLINE should be set based on compiler GCC or IAR. but for
 * now it is forced inline is removed. Need to identify how to detect compiler flag here
 */
#define ALWAYS_INLINE
#endif

#ifndef htobe16   /* This is defined in POSIX platforms */
ALWAYS_INLINE_PRE static inline ALWAYS_INLINE uint16_t htobe16(uint16_t v)
{
    return (uint16_t)(((v&0x00FF) << 8) | ((v&0xFF00)>>8));
}
#endif /* ifndef htobe16 */

#ifndef htobe32   /* This is defined in POSIX platforms */
ALWAYS_INLINE_PRE static inline ALWAYS_INLINE uint32_t htobe32(uint32_t v)
{
    return (uint32_t)(((v&0x000000FF) << 24) | ((v&0x0000FF00) << 8) | ((v&0x00FF0000) >> 8) | ((v&0xFF000000) >> 24));
}
#endif /* ifndef htobe32 */

#endif /* ifdef LINT */

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif /* ifndef MIN */

#ifndef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif /* ifndef MAX */

#ifndef ROUND_UP
#define ROUND_UP(x,y)    ((x) % (y) ? (x) + (y)-((x)%(y)) : (x))
#endif /* ifndef ROUND_UP */

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(m, n)    (((m) + (n) - 1) / (n))
#endif /* ifndef DIV_ROUND_UP */

/** @} */

#ifdef __cplusplus
} /*extern "C" */
#endif
