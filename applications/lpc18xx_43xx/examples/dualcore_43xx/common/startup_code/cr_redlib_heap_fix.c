#include "app_dualcore_cfg.h"

#ifndef STACK_SIZE
#define STACK_SIZE  (0x400)
#endif

/* Top of stack that comes from linker script */
extern void _vStackTop(void);

// *****************************************************************************
//
// Heap overflow check function required by REDLib_V2 library
// Without the fix Redlib's malloc calls from inside freeRtos tasks
// will always return NULL.
//
// *****************************************************************************
extern unsigned int *_pvHeapStart;
unsigned int __check_heap_overflow (void * new_end_of_heap)
{
	uint32_t stackend = ((uint32_t) &_vStackTop) - STACK_SIZE;
	return ((uint32_t)new_end_of_heap >= stackend);
}
