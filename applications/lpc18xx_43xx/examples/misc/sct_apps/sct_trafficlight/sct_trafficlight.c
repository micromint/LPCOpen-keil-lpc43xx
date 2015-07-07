/*
 * @brief State Configurable Timer Traffic Lights example
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

#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "sct_fsm.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
#if (defined(BOARD_HITEX_EVA_1850) || defined(BOARD_HITEX_EVA_4350))
#define MCSEL_BIT           23
#define MCSEL_PORT          6
#endif

static uint16_t delay;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initialise the SCT Pins */
void SCT_PinsConfigure(void)
{
#if (defined(BOARD_HITEX_EVA_1850) || defined(BOARD_HITEX_EVA_4350))
	/* Enable signals on MC connector X19 */
	Chip_SCU_PinMuxSet(0xD, 9, (SCU_MODE_PULLUP | SCU_MODE_FUNC4));		/* PD_9:  GPIO 6.23, MCSEL */
	Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, MCSEL_PORT, MCSEL_BIT);
	Chip_GPIO_SetPinState(LPC_GPIO_PORT, MCSEL_PORT, MCSEL_BIT, false);

	/* Pin configuration for SCT */
	/* PD_7:  SCTIN_5 used for the CROSS_REQUEST input (button) */
	Chip_SCU_PinMuxSet(0xD, 7, (SCU_MODE_PULLDOWN | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC1));

	/* PD_10:  SCTIN_1, used for SCT clock input */
	/* Connect JP13 pin 1 with connector X19  pin2 */
	Chip_SCU_PinMuxSet(0xD, 10, (SCU_MODE_PULLUP | SCU_MODE_INBUFF_EN | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC1));

	Chip_SCU_PinMuxSet(0xE, 6, (SCU_MODE_INACT | SCU_MODE_FUNC1));	/* PE_6:  SCTOUT_2 connected to RGB green */
	Chip_SCU_PinMuxSet(0xE, 5, (SCU_MODE_INACT | SCU_MODE_FUNC1));	/* PE_5:  SCTOUT_3 connected to RGB red */
	Chip_SCU_PinMuxSet(0xE, 8, (SCU_MODE_INACT | SCU_MODE_FUNC1));	/* P4_4:  SCTOUT_4 connected to RGB blue */
	Chip_SCU_PinMuxSet(0xE, 7, (SCU_MODE_INACT | SCU_MODE_FUNC1));	/* P4_3:  SCTOUT_5 */
	Chip_SCU_PinMuxSet(0xD, 3, (SCU_MODE_INACT | SCU_MODE_FUNC1));	/* PD_3:  SCTOUT_6 */

	/* Global configuration of the SCT */
	/* use external clock, rising edge, on CTIN1 */
	/* For Hitex4350, external clock is IRC clock /16 = 12 MHz / 16 = 750 KHz */
	/* For Hitex1850, external clock is IRC clock /16 = 12 MHz / 16 = 750 KHz */
	Chip_SCT_Config(LPC_SCT, (SCT_CONFIG_16BIT_COUNTER | (0x2 << 1) | (0x2 << 3)));

#elif (defined(BOARD_KEIL_MCB_1857) || defined(BOARD_KEIL_MCB_4357))

	/* PD_10:  SCTIN_2, used for SCT clock input */
	/* Connect PF_8 pin to CLK3 */
	Chip_SCU_PinMuxSet(0xF, 8, (SCU_MODE_PULLUP | SCU_MODE_INBUFF_EN | SCU_MODE_HIGHSPEEDSLEW_EN | SCU_MODE_FUNC2));

	Chip_SCU_PinMuxSet(0xD, 11, (SCU_MODE_INACT | SCU_MODE_FUNC6));	/* PD_11:  SCTOUT_14 connected to LED1 */
	Chip_SCU_PinMuxSet(0xD, 12, (SCU_MODE_INACT | SCU_MODE_FUNC6));	/* PD_12:  SCTOUT_10 connected to LED2 */
	Chip_SCU_PinMuxSet(0xD, 13, (SCU_MODE_INACT | SCU_MODE_FUNC6));	/* PD_13:  SCTOUT_13 connected to LED3 */
	Chip_SCU_PinMuxSet(0xD, 14, (SCU_MODE_INACT | SCU_MODE_FUNC6));	/* PD_14:  SCTOUT_11 connected to LED4 */
	Chip_SCU_PinMuxSet(0xD, 15, (SCU_MODE_INACT | SCU_MODE_FUNC6));	/* PD_15:  SCTOUT_8 connected to LED5 */

	/* Global configuration of the SCT */
	/* use external clock, rising edge, on CTIN2 */
	/* For Keil4357, external clock is IRC clock /16 = 12 MHz / 16 = 750 KHz */
	/* For Keil1857, external clock is IRC clock /16 = 12 MHz / 16 = 750 KHz */
	Chip_SCT_Config(LPC_SCT, (SCT_CONFIG_16BIT_COUNTER | (0x2 << 1) | (0x4 << 3)));

#else
#error Board not supported!
#endif

}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	SCT interrupt handler
 * @return	Nothing
 */
void SCT_IRQHandler(void)
{
	/* Acknowledge the interrupt source */
	if (LPC_SCT->EVFLAG & (1 << SCT_IRQ_EVENT_crossingTimeout)) {

		LPC_SCT->EVFLAG |= (1 << SCT_IRQ_EVENT_crossingTimeout);

		/* determine the captured RPM measurement */
		delay = (rand() % 22 + 1) * 2929;
		reload_cross_timeout(delay);
	}
}

/**
 * @brief	Main entry point
 * @return	Nothing
 */
int main(void)
{
	uint32_t SCT_FRQ;

	SystemCoreClockUpdate();
	Board_Init();

	/* Initialize SCT */
	Chip_SCT_Init(LPC_SCT);

	LPC_SCU->SFSCLK[3] = SCU_MODE_PULLDOWN | SCU_MODE_FUNC1;	/* function 1; CGU clk out, pull up */

	/* Attach IRC clock to divider B with a divider of 16 */
	Chip_Clock_SetDivider(CLK_IDIV_B, CLKIN_IRC, 16);

	/* Route divider B output to Clock output base clock and enable base out clock */
	Chip_Clock_SetBaseClock(CLK_BASE_OUT, CLKIN_IDIVB, true, false);

	/* Configure SCT pins */
	SCT_PinsConfigure();

	SCT_FRQ = Chip_Clock_GetRate(CLK_MX_SCT);
	DEBUGOUT("SCT_FRQ: %d\r\n", SCT_FRQ);

	Chip_SCT_SetClrControl(LPC_SCT, SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L | SCT_CTRL_PRE_L(256 - 1)
						   | SCT_CTRL_HALT_H | SCT_CTRL_CLRCTR_H | SCT_CTRL_PRE_H(256 - 1),
						   ENABLE);

	/* Now use the FSM code to configure the state machine */
	sct_fsm_init();

	/* initialize random seed: */
	srand(0x5EED);

	/*  12000000 / 256 / 16 = 2929 Hz
	 * 65535 / 2929 =~ 22 */
	/* generate a random delay from 1 to 22 seconds */
	delay = (rand() % 22 + 1) * 2929;

	NVIC_ClearPendingIRQ(SCT_IRQn);
	NVIC_EnableIRQ(SCT_IRQn);

	/* Start the SCT */
	Chip_SCT_SetClrControl(LPC_SCT, SCT_CTRL_STOP_L | SCT_CTRL_HALT_L | SCT_CTRL_STOP_H | SCT_CTRL_HALT_H, DISABLE);

	while (1) {
		__WFI();
	}
}
