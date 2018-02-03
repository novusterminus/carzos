/*
 * types.h
 *
 *  Created on: Jan 27, 2018
 *      Author: ci
 */

#ifndef KTYPE_H_
#define KTYPE_H_

#include <stdint.h>

typedef  int8_t s8;
typedef uint8_t u8;

typedef  int16_t s16;
typedef uint16_t u16;

typedef  int32_t s32;
typedef uint32_t u32;

typedef  int64_t s64;
typedef uint64_t u64;


#define ALIGN(x,a)              __ALIGN_MASK(x,(typeof(x))(a)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))

#endif /* KTYPE_H_ */
