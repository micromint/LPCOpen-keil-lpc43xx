/*
 * @brief LPCSPIFILIB FLASH library device specific functions
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licenser disclaim any and
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

#ifndef __SPIFILIB_DEV_H_
#define __SPIFILIB_DEV_H_
#include "chip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup LPCSPIFILIB_DEV LPCSPIFILIB device driver API functions
 * @ingroup LPCSPIFILIB
 * @{
 */	
/**
 * @brief Possible error codes that can be returned from functions
 */
typedef enum {
	SPIFI_ERR_NONE = 0,							/**< No error */
	SPIFI_ERR_BUSY,									/**< Device is busy */
	SPIFI_ERR_GEN,									/**< General error */
	SPIFI_ERR_NOTSUPPORTED,					/**< Capability not supported */
	SPIFI_ERR_ALIGNERR,							/**< Attempted to do an operation on an unaligned section of the device */
	SPIFI_ERR_LOCKED,								/**< Device was locked and a program/erase operation was attempted */
	SPIFI_ERR_PROGERR,							/**< Error programming device (blocking mode only) */
	SPIFI_ERR_ERASEERR,							/**< Erase error (blocking mode only) */
	SPIFI_ERR_NOTBLANK,							/**< Program operation on block that is not blank */
	SPIFI_ERR_PAGESIZE,							/**< PageProgram write size exceeds page size */
	SPIFI_ERR_VAL,									/**< Program operation failed validation or readback compare */
	SPIFI_ERR_RANGE,								/**< Range error, bad block number, address out of range, etc. */
	SPIFI_ERR_MEMMODE,							/**< Library calls not allowed while in memory mode. */
	SPIFI_ERR_LASTINDEX
} SPIFI_ERR_T;

/**
 * @brief Possible device capabilities returned from getInfo()
 */
#define SPIFI_CAP_QUAD   			(1 << 0)			/**< Supports QUAD device mode */
#define SPIFI_CAP_FULLLOCK  	(1 << 1)			/**< Full device lock supported */
#define SPIFI_CAP_BLOCKLOCK 	(1 << 2)			/**< Individual block device lock supported */
#define SPIFI_CAP_SUBBLKERASE (1 << 3)			/**< Sub-block erase supported */
#define SPIFI_CAP_NOBLOCK   	(1 << 16)			/**< Non-blocking mode supported */

/**
 * @brief Possible device statuses returned from getInfo()
 */
#define SPIFI_STAT_BUSY     (1 << 0)			/**< Device is busy erasing or programming */
#define SPIFI_STAT_ISWP     (1 << 1)			/**< Device is write protected (software or hardware) */
#define SPIFI_STAT_FULLLOCK (1 << 2)			/**< Device is fully locked */
#define SPIFI_STAT_PARTLOCK (1 << 3)			/**< Device is partially locked (device specific) */
#define SPIFI_STAT_PROGERR  (1 << 4)			/**< Device status shows a program error (non-blocking mode only) */
#define SPIFI_STAT_ERASEERR (1 << 5)			/**< Device status shows a erase error (non-blocking mode only) */

/**
 * @brief Possible driver options, may not be supported by all drivers
 */
#define SPIFI_OPT_USE_QUAD  	(1 << 0)			/**< Enables QUAD operations, only supported if the SPIFI_CAP_QUAD capability exists */
#define SPIFI_OPT_NOBLOCK   	(1 << 16)			/**< Will not block on program and erase operations, poll device status manually */


/**
 * @brief Common data applicable to all devices
 */
typedef struct {
	uint32_t				spifiCtrlAddr;			/**< SPIFI controller base address */
	uint32_t        baseAddr;					/**< Physical base address for the device */
	uint32_t        numBlocks;					/**< Number of blocks on the device */
	uint32_t        blockSize;					/**< Size of blocks on the device */
	uint32_t        numSubBlocks;				/**< Number of sub-blocks on the device */
	uint32_t        subBlockSize;				/**< Size of sub-blocks on the device */
	uint32_t        pageSize;					/**< Size of a page, usually denotes maximum write size in bytes for a single write operation */
	uint32_t        maxReadSize;				/**< Maximum read size in bytes for a single read operation */
	uint32_t        maxClkRate;					/**< Maximum clock rate of device in Hz */
	uint32_t        caps;						/**< Device capabilities of values SPIFI_CAP_* */
	uint32_t        opts;						/**< Device options of values SPIFI_OPT_* */
	char *pDevName;						/**< Pointer to generic device name */
	SPIFI_ERR_T     lastErr;					/**< Last error for the driver */
	uint8_t         id[4];						/**< JEDEC ID and size data, varies per device */
} SPIFI_INFODATA_T;

/* Forward type declaration */
struct SPIFI_HANDLE;

/**
 * @brief Possible info lookup requests
 */
typedef enum {
	SPIFI_INFO_DEVSIZE = 0,						/**< Device size in Bytes */
	SPIFI_INFO_ERASE_BLOCKS,					/**< Number of erase blocks */
	SPIFI_INFO_ERASE_BLOCKSIZE,				/**< Size of erase blocks */
	SPIFI_INFO_ERASE_SUBBLOCKS,					/**< Number of erase sub-blocks */
	SPIFI_INFO_ERASE_SUBBLOCKSIZE,				/**< Size of erase sub-blocks */
	SPIFI_INFO_PAGESIZE,						/**< Size of a page, page write size limit */
	SPIFI_INFO_MAXREADSIZE,						/**< Maximum read size, read size limit in bytes */
	SPIFI_INFO_MAXCLOCK,						/**< Maximum device speed in Hz */
	SPIFI_INFO_CAPS,							/**< Device capabilities, OR'ed SPIFI_CAP_* values */
	SPIFI_INFO_STATUS,							/**< Device capabilities, Or'ed SPIFI_STAT_* values */
	SPIFI_INFO_OPTIONS							/**< Device capabilities, Or'ed SPIFI_OPT_* values */
} SPIFI_INFO_ID_T;


/**
 * @brief Device specific function pointers
 */
typedef struct {
	/* Device init and de-initialization */
	SPIFI_ERR_T (*init)(struct SPIFI_HANDLE *);																/**< Initialize SPIFI device */
	SPIFI_ERR_T (*deInit)(struct SPIFI_HANDLE *);															/**< De-initialize SPIFI device */
	SPIFI_ERR_T (*unlockDevice)(struct SPIFI_HANDLE *);												/**< Full device unlock */
	SPIFI_ERR_T (*lockDevice)(struct SPIFI_HANDLE *);													/**< Full device lock */
	SPIFI_ERR_T (*unlockBlock)(struct SPIFI_HANDLE *, uint32_t);							/**< Unlock a block */
	SPIFI_ERR_T (*lockBlock)(struct SPIFI_HANDLE *, uint32_t);								/**< Lock a block */
	SPIFI_ERR_T (*eraseAll)(struct SPIFI_HANDLE *);														/**< Full device erase */
	SPIFI_ERR_T (*eraseBlock)(struct SPIFI_HANDLE *, uint32_t);								/**< Erase a block by block number */
	SPIFI_ERR_T (*eraseSubBlock)(struct SPIFI_HANDLE *, uint32_t);						/**< Erase a sub-block by block number */
	SPIFI_ERR_T (*pageProgram)(struct SPIFI_HANDLE *, uint32_t, const uint32_t *, uint32_t);		/**< Program up to a page of data at an address */
	SPIFI_ERR_T (*read)(struct SPIFI_HANDLE *, uint32_t, uint32_t *, uint32_t);	/**< Read an address range */
	SPIFI_ERR_T (*setOpts)(struct SPIFI_HANDLE *, uint32_t, bool);						/**< Set or Unset driver options */
	SPIFI_ERR_T (*reset)(struct SPIFI_HANDLE *);															/**< Reset SPIFI device */

	/* Info query functions */
	uint32_t (*getMemModeCmd)(struct SPIFI_HANDLE *, bool);										/**< returns memoryMode cmd */
	const char *(*getDevName)(struct SPIFI_HANDLE *);													/**< Returns the device name */
	uint32_t (*getInfo)(struct SPIFI_HANDLE *, SPIFI_INFO_ID_T);							/**< Returns info on the device */
} SPIFI_DEV_T;

	
/**
 * @brief Register device data.
 */
typedef struct SPIFI_DEV_PDATA {
	uint8_t id[4];									/**< ID match value */
	uint16_t blks;									/**< # of blocks */
	uint32_t blkSize;								/**< size of block */
	uint16_t subBlks;								/**< # of sub-blocks */
	uint16_t subBlkSize;						/**< size of sub-block */
	uint16_t pageSize;							/**< page size */
	uint32_t maxReadSize;						/**< max read allowed in one operation */
	uint32_t maxClkRate;						/**< maximum clock rate */
	uint32_t caps;									/**< capabilities supported */
} SPIFI_DEV_PDATA_T;

/**
 * @brief Register device data node
 */
typedef struct SPIFI_DEV_DATA {
	const SPIFI_DEV_PDATA_T *pDevData;	/**< Pointer to device geometry / caps */
	struct SPIFI_DEV_DATA *pNext; 			/**< Reserved */
	
}SPIFI_DEV_DATA_T;

/**
 * @brief LPCSPIFILIB device handle, used with all device and info functions
 */
typedef struct SPIFI_HANDLE {
	const SPIFI_DEV_T       *pDev;				  /**< Pointer to device specific functions */
	SPIFI_INFODATA_T        *pInfoData;			/**< Pointer to info data area */
	void                    *pDevData;			/**< Pointer to device context (used by device functions) */
} SPIFI_HANDLE_T;


/**
 * @brief LPCSPIFILIB device descriptor, used to describe devices to non-device specific functions
 */
typedef struct {
	const char              *pDevName;						/**< Pointer to generic device name */
	uint32_t                prvDataSize;					/**< Number of bytes needed for driver private data area allocation */
	uint32_t (*pPrvDevDetect)(uint32_t baseAddr);	/**< Pointer to device specific detection function */
	SPIFI_ERR_T (*pPrvDevSetup)(SPIFI_HANDLE_T *pHandle, uint32_t spifiCtrlAddr, uint32_t baseAddr);	/**< Pointer to device specific device initialization */
	SPIFI_ERR_T (*pPrvDevRegister)(void *pFamily, SPIFI_DEV_DATA_T *pDevData); /**< Pointer to device specific register device function */
} SPIFI_DEVDESC_T;

/**
 * @brief LPCSPIFILIB family data.
 */
typedef struct SPIFI_DEV_FAMILY {
	const SPIFI_DEVDESC_T *pDesc;							/**< Pointer to device descriptor */
	struct SPIFI_DEV_FAMILY *pNext;						/**< Reserved list pointer */
}SPIFI_DEV_FAMILY_T;

/**
 * @}
 */

/** @defgroup LPCSPIFILIB_REGISTERHELPER LPCSPIFILIB family registration functions
 * @ingroup LPCSPIFILIB
 * @{
 */

/**
 * @brief	Family registration function
 * @return	A pointer to a non-volatile SPIFI_DEV_FAMILY_T initialized for family. 
 * @note	This function constructs and returns a non-volitile SPIFI_DEV_FAMILY_T 
 * structure that contains family specific information needed to register family. 
 * This function MUST NOT be called directly and should only be passed to the 
 * registration function spifiRegisterFamily()
 */
SPIFI_DEV_FAMILY_T *SPIFI_REG_FAMILY_SpansionS25FLP(void);

/**
 * @brief	Family registration function
 * @return	A pointer to a non-volatile SPIFI_DEV_FAMILY_T initialized for family. 
 * @note	This function constructs and returns a non-volitile SPIFI_DEV_FAMILY_T 
 * structure that contains family specific information needed to register family. 
 * This function MUST NOT be called directly and should only be passed to the 
 * registration function spifiRegisterFamily()
 */
SPIFI_DEV_FAMILY_T *SPIFI_REG_FAMILY_SpansionS25FL1(void);

/**
 * @brief	Family registration function
 * @return	A pointer to a non-volatile SPIFI_DEV_FAMILY_T initialized for family. 
 * @note	This function constructs and returns a non-volitile SPIFI_DEV_FAMILY_T 
 * structure that contains family specific information needed to register family. 
 * This function MUST NOT be called directly and should only be passed to the 
 * registration function spifiRegisterFamily()
 */
SPIFI_DEV_FAMILY_T *SPIFI_REG_FAMILY_MacronixMX25L(void);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* __SPIFILIB_DEV_H_ */
