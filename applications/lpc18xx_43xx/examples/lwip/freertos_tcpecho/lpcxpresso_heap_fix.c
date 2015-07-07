/*
 * @brief NXP LPCXpresso heap validation function
 *
 * Startup file (having reset and main routines)
 * This file provides functions necessary to start all the example tasks
 * based on the configuration.
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

/**
 * Define STACK_SIZE or HEAP_SIZE in the command line
 * FIXME: Need to be standardized
 */
#define DEFAULT_STACK_SZ    0x800

#if (!defined(HEAP_SIZE) && !defined(STACK_SIZE))
#define HEAP_END    ((unsigned long) &_vStackTop - DEFAULT_STACK_SZ)
#elif (defined(HEAP_SIZE))
#define HEAP_END    ((unsigned long) &_pvHeapStart + HEAP_SIZE)
#else
#define HEAP_END    ((unsigned long) &_vStackTop - STACK_SIZE)
#endif

// *****************************************************************************
//
// External declaration for the pointer to the stack top and Heap start
// from the Linker Script
//
// *****************************************************************************
extern void _vStackTop(void);
extern void _pvHeapStart(void);

// *****************************************************************************
//
// Heap overflow check function required by REDLib_V2 library
//
// *****************************************************************************
unsigned int __check_heap_overflow (void * new_end_of_heap)
{
	return (new_end_of_heap >= (void *)HEAP_END);
}

