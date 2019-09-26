#include "riscv.h"

#define __bswap_constant_32(x)					\
  ((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >> 8)	\
   | (((x) & 0x0000ff00u) << 8) | (((x) & 0x000000ffu) << 24))

inline unsigned int MemoryRead(volatile unsigned int address)
{
	unsigned int value = *(volatile unsigned int*)address;
	value = __bswap_constant_32(value);
	return value;
}

inline void MemoryWrite(volatile unsigned int address, unsigned int value)
{
	value = __bswap_constant_32(value);
	*(volatile unsigned int*)address = value;
}
