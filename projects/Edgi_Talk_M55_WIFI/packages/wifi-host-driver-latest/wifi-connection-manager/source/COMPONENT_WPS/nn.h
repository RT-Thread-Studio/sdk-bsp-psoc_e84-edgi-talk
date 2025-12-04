/*
 * $ Copyright Cypress Semiconductor $
 */

/*******************************************************************************
 * @file
 * Header for the natural numbers library
 *******************************************************************************
 */

#ifndef INCLUDED_NN_H
#define INCLUDED_NN_H

#include <stdint.h>

#include "cy_wps_common.h"
#include "cy_wps_structures.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t len;
    uint32_t num[1];
} NN_t;

void     NN_Clr(NN_t* number);
uint32_t NN_Add(NN_t* result, const NN_t*x, const NN_t*y);
uint32_t NN_Sub(NN_t* result, const NN_t*x, const NN_t*y);
void     NN_Mul(NN_t* result, const NN_t*x, const NN_t*y);
void     NN_AddMod(NN_t* result, const NN_t*x, const NN_t*y, const NN_t*modulus);
void     NN_SubMod(NN_t* result, const NN_t*x, const NN_t*y, const NN_t*modulus);
void     NN_MulMod(NN_t* result, const NN_t*x, const NN_t*y, const NN_t*modulus);
void     NN_ExpMod(NN_t* result, NN_t*x, NN_t*modulus, NN_t*e, NN_t*w);
void     NN_MulModMont(NN_t* result, const NN_t*x, const NN_t*y, const NN_t*m, uint32_t t);
void     NN_ExpModMont(NN_t* result, NN_t*x, NN_t*m, NN_t*e, NN_t*w);
uint32_t NN_EmTick(const NN_t* mod);
void     NN_ErModEm(NN_t* result, const NN_t*m);
uint64_t NN_Mul32x32u64(uint32_t a, uint32_t b);
void     wps_NN_set(cy_wps_NN_t* m, const uint8_t* buffer);
void     wps_NN_get(const cy_wps_NN_t* m, uint8_t* buffer);

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_NN_H */

