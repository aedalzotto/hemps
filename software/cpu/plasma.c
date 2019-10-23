#include "plasma.h"

inline unsigned int MemoryRead(volatile unsigned int address)
{
	return *(volatile unsigned int*)address;
}

inline void MemoryWrite(volatile unsigned int address, unsigned int value)
{
	*(volatile unsigned int*)address = value;
}