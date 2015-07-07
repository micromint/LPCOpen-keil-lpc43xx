/*
 * @brief HID generic example's callabck routines
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

#include "board.h"
#include <stdint.h>
#include <string.h>
#include "usbd_rom_api.h"
#include "hid_sio.h"
#include "lpcusbsio_i2c.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define HID_I2C_STATE_DISCON        0
#define HID_I2C_STATE_CONNECTED     1

#define HID_SIO_I2C_PORTS			1
#define HID_SIO_SPI_PORTS			1
#define HID_SIO_GPIO_PORTS			8

#if (defined(BOARD_KEIL_MCB_1857) || defined(BOARD_KEIL_MCB_4357) || \
	defined(BOARD_NGX_XPLORER_1830) || defined(BOARD_NGX_XPLORER_4330) || defined(BOARD_BAMBINO) || \
	defined(BOARD_NXP_LPCLINK2_4370) || defined(BOARD_NXP_LPCXPRESSO_4337))
#define LPC_SSP				LPC_SSP1
#define SSP_IRQ				SSP1_IRQn
#define SSPIRQHANDLER		SSP1_IRQHandler
#elif (defined(BOARD_HITEX_EVA_1850) || defined(BOARD_HITEX_EVA_4350))
#define LPC_SSP				LPC_SSP0
#define SSP_IRQ				SSP0_IRQn
#define SSPIRQHANDLER		SSP0_IRQHandler
#else
#warning Unsupported Board
#endif

/**
 * Structure containing HID_I2C control data
 */
typedef struct __HID_SIO_CTRL_T {
	USBD_HANDLE_T hUsb;		/*!< Handle to USB stack */
	LPC_I2C_T *pI2C[HID_SIO_I2C_PORTS];		/*!< I2C ports this bridge is associated. */
	LPC_SSP_T *pSSP[HID_SIO_SPI_PORTS];		/*!< SPI ports this bridge is associated. */
	uint8_t reqWrIndx;		/*!< Write index for request queue. */
	uint8_t reqRdIndx;		/*!< Read index for request queue. */
	uint8_t respWrIndx;		/*!< Write index for response queue. */
	uint8_t respRdIndx;		/*!< Read index for response queue. */
	uint8_t reqPending;		/*!< Flag indicating request is pending in EP RAM */
	uint8_t respIdle;		/*!< Flag indicating EP_IN/response interrupt is idling */
	uint8_t state;			/*!< State of the controller */
	uint8_t resetReq;		/*!< Flag indicating if reset is requested by host */
	uint8_t epin_adr;		/*!< Interrupt IN endpoint associated with this HID instance. */
	uint8_t epout_adr;		/*!< Interrupt OUT endpoint associated with this HID instance. */
	uint16_t pad0;

	uint8_t reqQ[HID_I2C_MAX_PACKETS][HID_I2C_PACKET_SZ];		/*!< Requests queue */
	uint8_t respQ[HID_I2C_MAX_PACKETS][HID_I2C_PACKET_SZ];	/*!< Response queue */
} HID_SIO_CTRL_T;

/* Firmware version number defines and variables. */
#define HID_SIO_MAJOR_VER			2
#define HID_SIO_MINOR_VER			0
static const char *g_fwVersion = "(" __DATE__ " " __TIME__ ")";

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

extern const uint8_t HID_I2C_ReportDescriptor[];
extern const uint16_t HID_I2C_ReportDescSize;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static void InitI2CPorts(HID_SIO_CTRL_T *pHidI2c)
{
	Chip_SCU_I2C0PinConfig(I2C0_FAST_MODE_PLUS);
	NVIC_DisableIRQ(I2C0_IRQn);
	pHidI2c->pI2C[0] = LPC_I2C0;

#if (HID_SIO_I2C_PORTS == 2)	
	/* Configure pin function for I2C1 on PE.13 (I2C1_SDA) and PE.15 (I2C1_SCL) */
	Chip_SCU_PinMuxSet(0xE, 13, (SCU_MODE_ZIF_DIS | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC2));
	Chip_SCU_PinMuxSet(0xE, 15, (SCU_MODE_ZIF_DIS | SCU_MODE_INBUFF_EN | SCU_MODE_FUNC2));
	NVIC_DisableIRQ(I2C1_IRQn);
	/* bind to I2C port */
	pHidI2c->pI2C[1] = LPC_I2C1;
#endif
}

static void InitSPIPorts(HID_SIO_CTRL_T *pHidI2c)
{
	/* SSP initialization */
	Board_SSP_Init(LPC_SSP);
	Chip_SSP_Init(LPC_SSP);
	pHidI2c->pSSP[0] = LPC_SSP;
}

static INLINE void HID_I2C_IncIndex(uint8_t *index)
{
	*index = (*index + 1) & (HID_I2C_MAX_PACKETS - 1);
}

/*  HID get report callback function. */
static ErrorCode_t HID_I2C_GetReport(USBD_HANDLE_T hHid, USB_SETUP_PACKET *pSetup,
									 uint8_t * *pBuffer, uint16_t *plength)
{
	return LPC_OK;
}

/* HID set report callback function. */
static ErrorCode_t HID_I2C_SetReport(USBD_HANDLE_T hHid, USB_SETUP_PACKET *pSetup,
									 uint8_t * *pBuffer, uint16_t length)
{
	return LPC_OK;
}

/* HID_I2C Interrupt endpoint event handler. */
static ErrorCode_t HID_I2C_EpIn_Hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	HID_SIO_CTRL_T *pHidI2c = (HID_SIO_CTRL_T *) data;
	uint16_t len;

	/* last report is successfully sent. Send next response if in queue. */
	if (pHidI2c->respRdIndx != pHidI2c->respWrIndx) {
		pHidI2c->respIdle = 0;
		len = pHidI2c->respQ[pHidI2c->respRdIndx][0];
		USBD_API->hw->WriteEP(pHidI2c->hUsb, pHidI2c->epin_adr, &pHidI2c->respQ[pHidI2c->respRdIndx][0], len);
		HID_I2C_IncIndex(&pHidI2c->respRdIndx);
	}
	else {
		pHidI2c->respIdle = 1;
	}
	return LPC_OK;
}

/* HID_I2C Interrupt endpoint event handler. */
static ErrorCode_t HID_I2C_EpOut_Hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	HID_SIO_CTRL_T *pHidI2c = (HID_SIO_CTRL_T *) data;
	HID_I2C_OUT_REPORT_T *pOut;

	if (event == USB_EVT_OUT) {
		/* Read the new request received. */
		USBD_API->hw->ReadEP(hUsb, pHidI2c->epout_adr, &pHidI2c->reqQ[pHidI2c->reqWrIndx][0]);
		pOut = (HID_I2C_OUT_REPORT_T *) &pHidI2c->reqQ[pHidI2c->reqWrIndx][0];

		/* handle HID_I2C_REQ_FLUSH request in IRQ context to abort current
		   transaction and reset the queue states. */
		if (pOut->req == HID_I2C_REQ_RESET) {
			pHidI2c->resetReq = 1;
		}
		/* normal request queue the buffer */
		HID_I2C_IncIndex(&pHidI2c->reqWrIndx);
		/* Queue the next buffer for Rx */
		USBD_API->hw->ReadReqEP(hUsb, pHidI2c->epout_adr, &pHidI2c->reqQ[pHidI2c->reqWrIndx][0], HID_I2C_PACKET_SZ);
	}
	return LPC_OK;
}

static uint32_t HID_I2C_StatusCheckLoop(HID_SIO_CTRL_T *pHidI2c, uint8_t port)
{
	/* wait for status change interrupt */
	while ( (Chip_I2CM_StateChanged(pHidI2c->pI2C[port]) == 0) &&
			(pHidI2c->resetReq == 0)) {
		/* loop */
	}
	return pHidI2c->resetReq;
}

static int32_t HID_I2C_HandleReadStates(HID_SIO_CTRL_T *pHidI2c,
										HID_I2C_OUT_REPORT_T *pOut,
										HID_I2C_IN_REPORT_T *pIn,
										uint32_t status)
{
	HID_I2C_RW_PARAMS_T *pRWParam = (HID_I2C_RW_PARAMS_T *) &pOut->data[0];
	int32_t ret = 0;
	uint8_t *buff = &pIn->length;
	uint32_t pos;

	switch (status) {
	case 0x58:			/* Data Received and NACK sent */
		buff[pIn->length++] = Chip_I2CM_ReadByte(pHidI2c->pI2C[pOut->sesId]);
		pIn->resp = HID_I2C_RES_OK;
		ret = 1;
		break;

	case 0x40:			/* SLA+R sent and ACK received */
		pHidI2c->pI2C[pOut->sesId]->CONSET = I2C_CON_AA;

	case 0x50:			/* Data Received and ACK sent */
		if (status == 0x50) {
			buff[pIn->length++] = Chip_I2CM_ReadByte(pHidI2c->pI2C[pOut->sesId]);
		}
		pos = pIn->length - HID_I2C_HEADER_SZ;
		if (pRWParam->options & HID_I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE) {
			if ((pRWParam->length - pos) == 1) {
				Chip_I2CM_NackNextByte(pHidI2c->pI2C[pOut->sesId]);
			}
		}
		else if (pRWParam->length == pos) {
			pIn->resp = HID_I2C_RES_OK;
		}
		ret = 1;
		break;
	}

	return ret;
}

static int32_t HID_I2C_HandleWriteStates(HID_SIO_CTRL_T *pHidI2c,
										 HID_I2C_OUT_REPORT_T *pOut,
										 HID_I2C_IN_REPORT_T *pIn,
										 uint32_t status)
{
	HID_I2C_RW_PARAMS_T *pRWParam = (HID_I2C_RW_PARAMS_T *) &pOut->data[0];
	int32_t ret = 0;
	uint8_t *buff = &pRWParam->data[0];
	uint32_t pos;

	switch (status) {
	case 0x30:			/* DATA sent NAK received */
		if (pRWParam->options & HID_I2C_TRANSFER_OPTIONS_BREAK_ON_NACK) {
			pIn->resp = HID_I2C_RES_NAK;
			ret = 1;
			break;
		}		/* else fall through */

	case 0x08:			/* Start condition on bus */
	case 0x10:			/* Repeated start condition */
		if ((pRWParam->options & HID_I2C_TRANSFER_OPTIONS_NO_ADDRESS) == 0) {
			break;
		}		/* else fall-through */

	case 0x18:			/* SLA+W sent and ACK received */
	case 0x28:			/* DATA sent and ACK received */
		pos = pIn->length - HID_I2C_HEADER_SZ;
		Chip_I2CM_WriteByte(pHidI2c->pI2C[pOut->sesId], buff[pos]);
		pIn->length++;
		if ((pos) == pRWParam->length) {
			pIn->resp = HID_I2C_RES_OK;
		}
		ret = 1;
		break;
	}

	return ret;
}

static void HID_I2C_HandleRWReq(HID_SIO_CTRL_T *pHidI2c,
								HID_I2C_OUT_REPORT_T *pOut,
								HID_I2C_IN_REPORT_T *pIn,
								int32_t read)
{
	HID_I2C_RW_PARAMS_T *pRWParam = (HID_I2C_RW_PARAMS_T *) &pOut->data[0];
	int32_t handled = 0;
	uint32_t status;

	/* clear state change interrupt status */
	Chip_I2CM_ClearSI(pHidI2c->pI2C[pOut->sesId]);

	if (pRWParam->options & HID_I2C_TRANSFER_OPTIONS_START_BIT) {
		Chip_I2CM_SendStart(pHidI2c->pI2C[pOut->sesId]);
	}

	while (pIn->resp == HID_I2C_RES_INVALID_CMD) {
		/* wait for status change interrupt */
		if (HID_I2C_StatusCheckLoop(pHidI2c, pOut->sesId) == 0) {

			status = Chip_I2CM_GetCurState(pHidI2c->pI2C[pOut->sesId]);
			/* handle Read or write states. */
			if (read ) {
				handled = HID_I2C_HandleReadStates(pHidI2c, pOut, pIn, status);
			}
			else {
				handled = HID_I2C_HandleWriteStates(pHidI2c, pOut, pIn, status);
			}

			if (handled == 0) {
				/* check status and send data */
				switch (status) {
				case 0x08:		/* Start condition on bus */
				case 0x10:		/* Repeated start condition */
					if ((pRWParam->options & HID_I2C_TRANSFER_OPTIONS_NO_ADDRESS) == 0) {
						Chip_I2CM_WriteByte(pHidI2c->pI2C[pOut->sesId], (pRWParam->slaveAddr << 1) | read);
					}
					break;

				case 0x20:		/* SLA+W sent NAK received */
				case 0x48:		/* SLA+R sent NAK received */
					pIn->resp = HID_I2C_RES_SLAVE_NAK;
					break;

				case 0x38:		/* Arbitration lost */
					pIn->resp = HID_I2C_RES_ARBLOST;
					break;

				case 0x00:		/* Bus Error */
					pIn->resp = HID_I2C_RES_BUS_ERROR;
					break;

				default:		/* we shouldn't be in any other state */
					pIn->resp = HID_I2C_RES_ERROR;
					break;
				}
			}
			/* clear state change interrupt status */
			Chip_I2CM_ClearSI(pHidI2c->pI2C[pOut->sesId]);
		}
		else {
			pIn->resp = HID_I2C_RES_ERROR;
			break;
		}
	}

	if ((pIn->resp != HID_I2C_RES_ARBLOST) &&
		(pRWParam->options & HID_I2C_TRANSFER_OPTIONS_STOP_BIT) ) {
		Chip_I2CM_SendStop(pHidI2c->pI2C[pOut->sesId]);
	}
}

static void HID_I2C_HandleXferReq(HID_SIO_CTRL_T *pHidI2c,
								  HID_I2C_OUT_REPORT_T *pOut,
								  HID_I2C_IN_REPORT_T *pIn)
{
	HID_I2C_XFER_PARAMS_T *pXfrParam = (HID_I2C_XFER_PARAMS_T *) &pOut->data[0];
	I2CM_XFER_T xfer;
	uint32_t ret = 0;

	memset(&xfer, 0, sizeof(I2CM_XFER_T));
	xfer.slaveAddr = pXfrParam->slaveAddr;
	xfer.txBuff = &pXfrParam->data[0];
	xfer.rxBuff = &pIn->data[0];
	xfer.rxSz = pXfrParam->rxLength;
	xfer.txSz = pXfrParam->txLength;
	xfer.options = pXfrParam->options;

	/* start transfer */
	Chip_I2CM_Xfer(pHidI2c->pI2C[pOut->sesId], &xfer);

	while (ret == 0) {
		/* wait for status change interrupt */
		if (HID_I2C_StatusCheckLoop(pHidI2c, pOut->sesId) == 0) {
			/* call state change handler */
			ret = Chip_I2CM_XferHandler(pHidI2c->pI2C[pOut->sesId], &xfer);
		}
		else {
			xfer.status = HID_I2C_RES_ERROR;
			break;
		}
	}

	/* Update the length we have to send back */
	if ((pXfrParam->rxLength - xfer.rxSz) > 0) {
		pIn->length += pXfrParam->rxLength - xfer.rxSz;
	}
	/* update response with the I2CM status returned. No translation
	   needed as they map directly to base LPCUSBSIO status. */
	pIn->resp = xfer.status;
}

/* Handle SIO requests */
void SIO_RequestHandler(USBD_HANDLE_T hSIOHid, HID_I2C_OUT_REPORT_T *pOut, HID_I2C_IN_REPORT_T *pIn)
{
	switch (pOut->req) {
	case HID_SIO_REQ_DEV_INFO:
		/* Set I2C ports available on this part */
		pIn->data[0] = HID_SIO_I2C_PORTS;
		/* Set SPI ports available on this part */
		pIn->data[1] = HID_SIO_SPI_PORTS;
		pIn->data[2] = HID_SIO_GPIO_PORTS;
		*((uint32_t*)&pIn->data[4]) = (HID_SIO_MAJOR_VER << 16) | HID_SIO_MINOR_VER;
		/* send back firmware version string */
		memcpy(&pIn->data[8], g_fwVersion, strlen(g_fwVersion));
		pIn->length += strlen(g_fwVersion);

		/* update response */
		pIn->resp = HID_I2C_RES_OK;
		break;
	}
}

/* Handle I2C requests */
void I2C_RequestHandler(USBD_HANDLE_T hSIOHid, HID_I2C_OUT_REPORT_T *pOut, HID_I2C_IN_REPORT_T *pIn)
{
	HID_SIO_CTRL_T *pHidI2c = (HID_SIO_CTRL_T *) hSIOHid;
	HID_I2C_PORTCONFIG_T *pCfg;

	switch (pOut->req) {
	case HID_I2C_REQ_INIT_PORT:
		/* Init I2C port */
		Chip_I2CM_Init(pHidI2c->pI2C[pOut->sesId]);
		pCfg = (HID_I2C_PORTCONFIG_T *) &pOut->data[0];
		Chip_I2CM_SetBusSpeed(pHidI2c->pI2C[pOut->sesId], pCfg->busSpeed);

		/* TBD. Change I2C0 pads modes per bus speed requested. Do we need to?*/

		/* update response */
		pIn->resp = HID_I2C_RES_OK;
		break;

	case HID_I2C_REQ_DEINIT_PORT:
		Chip_I2CM_DeInit(pHidI2c->pI2C[pOut->sesId]);
		/* update response */
		pIn->resp = HID_I2C_RES_OK;
		break;

	case HID_I2C_REQ_DEVICE_WRITE:
	case HID_I2C_REQ_DEVICE_READ:
		HID_I2C_HandleRWReq(pHidI2c, pOut, pIn, (pOut->req == HID_I2C_REQ_DEVICE_READ));
		break;

	case HID_I2C_REQ_DEVICE_XFER:
		HID_I2C_HandleXferReq(pHidI2c, pOut, pIn);
		break;

	case HID_I2C_REQ_RESET:
		Chip_I2CM_ResetControl(pHidI2c->pI2C[pOut->sesId]);
		Chip_I2CM_SendStartAfterStop(pHidI2c->pI2C[pOut->sesId]);
		Chip_I2CM_WriteByte(pHidI2c->pI2C[pOut->sesId], 0xFF);
		Chip_I2CM_SendStop(pHidI2c->pI2C[pOut->sesId]);
		pHidI2c->resetReq = 0;
		break;
	
	}
	
}

/******* Handle GPIO requestes ****************************************/
void GPIO_RequestHandler(USBD_HANDLE_T hSIOHid, HID_I2C_OUT_REPORT_T *pOut, HID_I2C_IN_REPORT_T *pIn)
{
	uint32_t temp, value;
	
	switch (pOut->req) {
	
	case HID_GPIO_REQ_PORT_VALUE:
		
		/* set drive high pins */
		temp = *(uint32_t*)&pOut->data[0];
		Chip_GPIO_SetValue(LPC_GPIO_PORT, pOut->sesId, temp);
	
		/* clear drive low pins */
		temp = *(uint32_t*)&pOut->data[4];
		Chip_GPIO_ClearValue(LPC_GPIO_PORT, pOut->sesId, temp);
	
		/* update response */
		*((uint32_t*)&pIn->data[0]) = Chip_GPIO_GetPortValue(LPC_GPIO_PORT, pOut->data[0]);
		pIn->length += 4;
		pIn->resp = HID_I2C_RES_OK;
		break;

	case HID_GPIO_REQ_PORT_DIR:
		/* get current settings */
		value = LPC_GPIO_PORT->DIR[pOut->sesId];
		/* Set direction */
		temp = *(uint32_t*)&pOut->data[0];
		value |= temp;
		/* clear input pins */
		temp = *(uint32_t*)&pOut->data[4];
		value &= ~temp;
		/* update the register */
		LPC_GPIO_PORT->DIR[pOut->sesId] = value;
		/* update response */
		*((uint32_t*)&pIn->data[0]) = LPC_GPIO_PORT->DIR[pOut->sesId];
		pIn->length += 4;
		pIn->resp = HID_I2C_RES_OK;
		break;

	case HID_GPIO_REQ_TOGGLE_PIN:
		Chip_GPIO_SetPinDIROutput(LPC_GPIO_PORT, pOut->sesId, pOut->data[0]);
		Chip_GPIO_SetPinToggle(LPC_GPIO_PORT, pOut->sesId, pOut->data[0]);
		/* update response */
		pIn->resp = HID_I2C_RES_OK;
		break;
	}
	
}

void SPI_RequestHandler(USBD_HANDLE_T hSIOHid, HID_I2C_OUT_REPORT_T *pOut, HID_I2C_IN_REPORT_T *pIn)
{
//	HID_SIO_CTRL_T *pHidI2c = (HID_SIO_CTRL_T *) hSIOHid;
	
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* HID init routine */
ErrorCode_t HID_SIO_init(USBD_HANDLE_T hUsb,
						 USB_INTERFACE_DESCRIPTOR *pIntfDesc,
						 USBD_API_INIT_PARAM_T *pUsbParam,
						 USBD_HANDLE_T *pSIOHid)
{
	USBD_HID_INIT_PARAM_T hid_param;
	USB_HID_REPORT_T reports_data[1];
	USB_ENDPOINT_DESCRIPTOR *pEpDesc;
	uint32_t new_addr, i, ep_indx;
	HID_SIO_CTRL_T *pHidI2c;
	ErrorCode_t ret = LPC_OK;

	if ((pIntfDesc == 0) || (pIntfDesc->bInterfaceClass != USB_DEVICE_CLASS_HUMAN_INTERFACE)) {
		return ERR_FAILED;
	}

	/* HID paramas */
	memset((void *) &hid_param, 0, sizeof(USBD_HID_INIT_PARAM_T));
	hid_param.max_reports = 1;
	/* Init reports_data */
	reports_data[0].len = HID_I2C_ReportDescSize;
	reports_data[0].idle_time = 0;
	reports_data[0].desc = (uint8_t *) &HID_I2C_ReportDescriptor[0];

	hid_param.mem_base = pUsbParam->mem_base;
	hid_param.mem_size = pUsbParam->mem_size;
	hid_param.intf_desc = (uint8_t *) pIntfDesc;
	/* user defined functions */
	hid_param.HID_GetReport = HID_I2C_GetReport;
	hid_param.HID_SetReport = HID_I2C_SetReport;
	hid_param.report_data  = reports_data;

	ret = USBD_API->hid->init(hUsb, &hid_param);
	if (ret == LPC_OK) {
		/* allocate memory for HID_SIO_CTRL_T instance */
		pHidI2c = (HID_SIO_CTRL_T *) hid_param.mem_base;
		hid_param.mem_base += sizeof(HID_SIO_CTRL_T);
		hid_param.mem_size -= sizeof(HID_SIO_CTRL_T);
		/* update memory variables */
		pUsbParam->mem_base = hid_param.mem_base;
		pUsbParam->mem_size = hid_param.mem_size;

		/* Init the control structure */
		memset(pHidI2c, 0, sizeof(HID_SIO_CTRL_T));
		/* store stack handle*/
		pHidI2c->hUsb = hUsb;
		/* set return handle */
		*pSIOHid = (USBD_HANDLE_T) pHidI2c;
		/* set response is idling. For HID_I2C_process() to kickstart transmission if response
		   data is pending. */
		pHidI2c->respIdle = 1;

		/* bind to I2C port */
		InitI2CPorts(pHidI2c);

		/* move to next descriptor */
		new_addr = (uint32_t) pIntfDesc + pIntfDesc->bLength;
		/* loop through descriptor to find EPs */
		for (i = 0; i < pIntfDesc->bNumEndpoints; ) {
			pEpDesc = (USB_ENDPOINT_DESCRIPTOR *) new_addr;
			new_addr = (uint32_t) pEpDesc + pEpDesc->bLength;

			/* parse endpoint descriptor */
			if ((pEpDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE) &&
				(pEpDesc->bmAttributes == USB_ENDPOINT_TYPE_INTERRUPT)) {

				/* find next endpoint */
				i++;

				if (pEpDesc->bEndpointAddress & USB_ENDPOINT_DIRECTION_MASK) {
					/* store Interrupt IN endpoint */
					pHidI2c->epin_adr = pEpDesc->bEndpointAddress;
					ep_indx = ((pHidI2c->epin_adr & 0x0F) << 1) + 1;
					/* register endpoint interrupt handler if provided*/
					ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, HID_I2C_EpIn_Hdlr, pHidI2c);
				}
				else {
					/* store Interrupt OUT endpoint */
					pHidI2c->epout_adr = pEpDesc->bEndpointAddress;
					ep_indx = ((pHidI2c->epout_adr & 0x0F) << 1);
					/* register endpoint interrupt handler if provided*/
					ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, HID_I2C_EpOut_Hdlr, pHidI2c);
				}
			}
		}

	}

	return ret;
}

/* Process HID_I2C request and response queue. */
void HID_SIO_process(USBD_HANDLE_T hSIOHid)
{
	HID_SIO_CTRL_T *pHidI2c = (HID_SIO_CTRL_T *) hSIOHid;
	HID_I2C_OUT_REPORT_T *pOut;
	HID_I2C_IN_REPORT_T *pIn;

	/* Check if we are connected to a host */
	if (USB_IsConfigured(pHidI2c->hUsb)) {

		/* check if we just got connected */
		if (pHidI2c->state == HID_I2C_STATE_DISCON) {
			/* queue the request */
			USBD_API->hw->ReadReqEP(pHidI2c->hUsb, pHidI2c->epout_adr, &pHidI2c->reqQ[0][0], HID_I2C_PACKET_SZ);
		}
		/* set state to connected */
		pHidI2c->state = HID_I2C_STATE_CONNECTED;
		if (pHidI2c->reqWrIndx != pHidI2c->reqRdIndx) {

			/* process the current packet */
			pOut = (HID_I2C_OUT_REPORT_T *) &pHidI2c->reqQ[pHidI2c->reqRdIndx][0];
			pIn = (HID_I2C_IN_REPORT_T *) &pHidI2c->respQ[pHidI2c->respWrIndx][0];
			/* construct response template */
			pIn->length = HID_I2C_HEADER_SZ;
			pIn->transId = pOut->transId;
			pIn->sesId = pOut->sesId;
			pIn->resp = HID_I2C_RES_INVALID_CMD;

			if (pOut->req < HID_I2C_REQ_MAX) {
				/* Handle I2C requests only if valid port is selected. */
				if (pOut->sesId < HID_SIO_I2C_PORTS) {
					I2C_RequestHandler(hSIOHid, pOut, pIn);
				}
				
			} else if (pOut->req < HID_SPI_REQ_MAX) {
				/* Handle SPI requests only if valid port is selected. */
				if (pOut->sesId < HID_SIO_SPI_PORTS) {
					SPI_RequestHandler(hSIOHid, pOut, pIn);
				}
				
			} else if (pOut->req < HID_GPIO_REQ_MAX) {
				/* Handle GPIO requests */
				GPIO_RequestHandler(hSIOHid, pOut, pIn);
				
			} else {
				/* Handle SIO requests */
				SIO_RequestHandler(hSIOHid, pOut, pIn);
			}

			HID_I2C_IncIndex(&pHidI2c->reqRdIndx);
			HID_I2C_IncIndex(&pHidI2c->respWrIndx);
		}

		/* Kick-start response tx if it is idling and we have something to send. */
		if ((pHidI2c->respIdle) && (pHidI2c->respRdIndx != pHidI2c->respWrIndx)) {

			pHidI2c->respIdle = 0;
			USBD_API->hw->WriteEP(pHidI2c->hUsb,
								  pHidI2c->epin_adr,
								  &pHidI2c->respQ[pHidI2c->respRdIndx][0],
								  HID_I2C_PACKET_SZ);
			HID_I2C_IncIndex(&pHidI2c->respRdIndx);
		}
	}
	else {
		/* check if we just got dis-connected */
		if (pHidI2c->state != HID_I2C_STATE_DISCON) {
			/* reset indexes */
			pHidI2c->reqWrIndx = pHidI2c->reqRdIndx = 0;
			pHidI2c->respRdIndx = pHidI2c->respWrIndx = 0;
			pHidI2c->respIdle = 1;
			pHidI2c->resetReq = 0;
		}
		pHidI2c->state = HID_I2C_STATE_DISCON;
	}
}
