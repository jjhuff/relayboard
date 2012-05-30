#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <avr/interrupt.h>
#include <avr/io.h>

#define cbi(p, q) ((p) &= ~_BV(q))
#define sbi(p, q) ((p) |= _BV(q))

#define nop() asm volatile("nop\n\t"::);

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#endif /* __COMPILER_H__ */

