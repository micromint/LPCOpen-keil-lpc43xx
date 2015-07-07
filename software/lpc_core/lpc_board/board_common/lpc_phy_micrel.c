/*
 * @brief simple Micrel PHY driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2012
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

#include "chip.h"
#include "lpc_phy.h"

/** @defgroup MICREL_PHY BOARD: PHY status and control driver for the MICREL KSZ PHYs
 * @ingroup BOARD_PHY
 * Various functions for controlling and monitoring the status of the
 * MICREL KSZ PHYs.
 * @{
 */

/* MICREL PHY register offsets */
#define KSZPHY_BCR_REG        0x0	/*!< Basic Control Register */
#define KSZPHY_BSR_REG        0x1	/*!< Basic Status Reg */
#define KSZPHY_PHYID1_REG     0x2	/*!< PHY ID 1 Reg  */
#define KSZPHY_PHYID2_REG     0x3	/*!< PHY ID 2 Reg */
#define KSZPHY_PHYSPLCTL_REG  0x1F/*!< PHY special control/status Reg */

/* MICREL BCR register definitions */
#define KSZPHY_RESET          (1 << 15)	/*!< 1= S/W Reset */
#define KSZPHY_LOOPBACK       (1 << 14)	/*!< 1=loopback Enabled */
#define KSZPHY_SPEED_SELECT   (1 << 13)	/*!< 1=Select 100MBps */
#define KSZPHY_AUTONEG        (1 << 12)	/*!< 1=Enable auto-negotiation */
#define KSZPHY_POWER_DOWN     (1 << 11)	/*!< 1=Power down PHY */
#define KSZPHY_ISOLATE        (1 << 10)	/*!< 1=Isolate PHY */
#define KSZPHY_RESTART_AUTONEG (1 << 9)	/*!< 1=Restart auto-negoatiation */
#define KSZPHY_DUPLEX_MODE    (1 << 8)	/*!< 1=Full duplex mode */

/* MICREL BSR register definitions */
#define KSZPHY_100BASE_T4     (1 << 15)	/*!< T4 mode */
#define KSZPHY_100BASE_TX_FD  (1 << 14)	/*!< 100MBps full duplex */
#define KSZPHY_100BASE_TX_HD  (1 << 13)	/*!< 100MBps half duplex */
#define KSZPHY_10BASE_T_FD    (1 << 12)	/*!< 100Bps full duplex */
#define KSZPHY_10BASE_T_HD    (1 << 11)	/*!< 10MBps half duplex */
#define KSZPHY_AUTONEG_COMP   (1 << 5)	/*!< Auto-negotation complete */
#define KSZPHY_RMT_FAULT      (1 << 4)	/*!< Fault */
#define KSZPHY_AUTONEG_ABILITY (1 << 3)	/*!< Auto-negotation supported */
#define KSZPHY_LINK_STATUS    (1 << 2)	/*!< 1=Link active */
#define KSZPHY_JABBER_DETECT  (1 << 1)	/*!< Jabber detect */
#define KSZPHY_EXTEND_CAPAB   (1 << 0)	/*!< Supports extended capabilities */

/* MICREL PHYSPLCTL status definitions */
#define KSZPHY_SPEEDMASK      (7 << 2)	/*!< Speed and duplex mask */
#define KSZPHY_SPEED100F      (6 << 2)	/*!< 100BT full duplex */
#define KSZPHY_SPEED10F       (5 << 2)	/*!< 10BT full duplex */
#define KSZPHY_SPEED100H      (2 << 2)	/*!< 100BT half duplex */
#define KSZPHY_SPEED10H       (1 << 2)	/*!< 10BT half duplex */

/* MICREL PHY ID 1/2 register definitions */
#define KSZPHY_PHYID1_OUI     0x0007		/*!< Expected PHY ID1 */
#define KSZPHY_PHYID2_OUI     0xC0F0		/*!< Expected PHY ID2, except last 4 bits */

/* DP83848 PHY update flags */
static uint32_t physts, olddphysts;

/* PHY update counter for state machine */
static int32_t phyustate;

/* Pointer to delay function used for this driver */
static p_msDelay_func_t pDelayMs;

/* Write to the PHY. Will block for delays based on the pDelayMs function. Returns
   true on success, or false on failure */
static Status lpc_mii_write(uint8_t reg, uint16_t data)
{
	Status sts = ERROR;
	int32_t mst = 250;

	/* Write value for register */
	Chip_ENET_StartMIIWrite(LPC_ETHERNET, reg, data);

	/* Wait for unbusy status */
	while (mst > 0) {
		if (Chip_ENET_IsMIIBusy(LPC_ETHERNET)) {
			mst--;
			pDelayMs(1);
		}
		else {
			mst = 0;
			sts = SUCCESS;
		}
	}

	return sts;
}

/* Read from the PHY. Will block for delays based on the pDelayMs function. Returns
   true on success, or false on failure */
static Status lpc_mii_read(uint8_t reg, uint16_t *data)
{
	Status sts = ERROR;
	int32_t mst = 250;

	/* Start register read */
	Chip_ENET_StartMIIRead(LPC_ETHERNET, reg);

	/* Wait for unbusy status */
	while (mst > 0) {
		if (!Chip_ENET_IsMIIBusy(LPC_ETHERNET)) {
			mst = 0;
			*data = Chip_ENET_ReadMIIData(LPC_ETHERNET);
			sts = SUCCESS;
		}
		else {
			mst--;
			pDelayMs(1);
		}
	}

	return sts;
}

/* Update PHY status from passed value */
static void kszphy_update_phy_sts(uint16_t linksts, uint16_t sdsts)
{
	/* Update link active status */
	if (linksts & KSZPHY_LINK_STATUS) {
		physts |= PHY_LINK_CONNECTED;
	}
	else {
		physts &= ~PHY_LINK_CONNECTED;
	}

	switch (sdsts & KSZPHY_SPEEDMASK) {
	case KSZPHY_SPEED100F:
	default:
		physts |= PHY_LINK_SPEED100;
		physts |= PHY_LINK_FULLDUPLX;
		break;

	case KSZPHY_SPEED10F:
		physts &= ~PHY_LINK_SPEED100;
		physts |= PHY_LINK_FULLDUPLX;
		break;

	case KSZPHY_SPEED100H:
		physts |= PHY_LINK_SPEED100;
		physts &= ~PHY_LINK_FULLDUPLX;
		break;

	case KSZPHY_SPEED10H:
		physts &= ~PHY_LINK_SPEED100;
		physts &= ~PHY_LINK_FULLDUPLX;
		break;
	}

	/* If the status has changed, indicate via change flag */
	if ((physts & (PHY_LINK_SPEED100 | PHY_LINK_FULLDUPLX | PHY_LINK_CONNECTED)) !=
		(olddphysts & (PHY_LINK_SPEED100 | PHY_LINK_FULLDUPLX | PHY_LINK_CONNECTED))) {
		olddphysts = physts;
		physts |= PHY_LINK_CHANGED;
	}
}

/* Initialize the MICREL KSZ PHY */
uint32_t lpc_phy_init(bool rmii, p_msDelay_func_t pDelayMsFunc)
{
	uint16_t tmp;
	int32_t i;

	pDelayMs = pDelayMsFunc;

	/* Initial states for PHY status and state machine */
	olddphysts = physts = phyustate = 0;

	/* Only first read and write are checked for failure */
	/* Put the DP83848C in reset mode and wait for completion */
	if (lpc_mii_write(KSZPHY_BCR_REG, KSZPHY_RESET) != SUCCESS) {
		return ERROR;
	}
	i = 400;
	while (i > 0) {
		pDelayMs(1);
		if (lpc_mii_read(KSZPHY_BCR_REG, &tmp) != SUCCESS) {
			return ERROR;
		}

		if (!(tmp & (KSZPHY_RESET | KSZPHY_POWER_DOWN))) {
			i = -1;
		}
		else {
			i--;
		}
	}
	/* Timeout? */
	if (i == 0) {
		return ERROR;
	}

	/* Setup link */
	lpc_mii_write(KSZPHY_BCR_REG, KSZPHY_AUTONEG);

	/* The link is not set active at this point, but will be detected
	   later */

	return SUCCESS;
}

/* Phy status update state machine */
uint32_t lpcPHYStsPoll(void)
{
	static uint16_t sts;

	switch (phyustate) {
	default:
	case 0:
		/* Read BMSR to clear faults */
		Chip_ENET_StartMIIRead(LPC_ETHERNET, KSZPHY_BSR_REG);
		physts &= ~PHY_LINK_CHANGED;
		physts = physts | PHY_LINK_BUSY;
		phyustate = 1;
		break;

	case 1:
		/* Wait for read status state */
		if (!Chip_ENET_IsMIIBusy(LPC_ETHERNET)) {
			/* Get PHY status with link state */
			sts = Chip_ENET_ReadMIIData(LPC_ETHERNET);
			Chip_ENET_StartMIIRead(LPC_ETHERNET, KSZPHY_PHYSPLCTL_REG);
			phyustate = 2;
		}
		break;

	case 2:
		/* Wait for read status state */
		if (!Chip_ENET_IsMIIBusy(LPC_ETHERNET)) {
			/* Update PHY status */
			physts &= ~PHY_LINK_BUSY;
			kszphy_update_phy_sts(sts, Chip_ENET_ReadMIIData(LPC_ETHERNET));
			phyustate = 0;
		}
		break;
	}

	return physts;
}

/**
 * @}
 */
