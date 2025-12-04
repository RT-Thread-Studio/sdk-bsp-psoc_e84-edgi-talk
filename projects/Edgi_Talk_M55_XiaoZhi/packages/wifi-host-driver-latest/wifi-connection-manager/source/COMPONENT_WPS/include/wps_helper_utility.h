/*
 * $ Copyright Cypress Semiconductor $
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
    return (uint16_t)(((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8));
}
#endif /* ifndef htobe16 */

#ifndef htobe32   /* This is defined in POSIX platforms */
ALWAYS_INLINE_PRE static inline ALWAYS_INLINE uint32_t htobe32(uint32_t v)
{
    return (uint32_t)(((v & 0x000000FF) << 24) | ((v & 0x0000FF00) << 8) | ((v & 0x00FF0000) >> 8) | ((v & 0xFF000000) >> 24));
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
