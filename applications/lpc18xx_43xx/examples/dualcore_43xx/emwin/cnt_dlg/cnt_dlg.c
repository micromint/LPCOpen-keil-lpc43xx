/*
 * @brief	emWin graphics dual core example using emWin library
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


#include "app_dualcore_cfg.h"
#include "ipc_example.h"
#include "ipc_msg.h"

#include "GUI.h"
#include "DIALOG.h"

#ifdef OS_FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#define FREERTOS_LCD_STACK_SZ            512
#define FREERTOS_TSC_STACK_SZ            128
#endif

#ifdef OS_UCOS_III
#include "os.h"

#define UCOS_LCD_STACK_SZ            1024
#define UCOS_TSC_STACK_SZ            512

#define UCOS_LCD_TASK_PRIORITY     (APP_CFG_TASK_START_PRIO + 1)
#define UCOS_TSC_TASK_PRIORITY     (APP_CFG_TASK_START_PRIO - 1)
#endif

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
/**
 * Global variables used by emWin application
 */
static volatile int start = 0;				/* Start flag for counter */
static volatile short counter = 0;			/* Count value */
static WM_HWIN hWin;						/* emWin Widget Frame Window structure */

#if (defined(OS_FREE_RTOS)) || (defined(OS_UCOS_III))
/**
 * X, Y coordinates used by TSC
 */
static int16_t old_tmp_x = -1, old_tmp_y = -1;
#endif

/**
 * Widget IDs used in Frame window
 */
#define ID_FRAMEWIN_0 (GUI_ID_USER + 0x0A)
#define ID_TEXT_0     (GUI_ID_USER + 0x0B)
#define ID_BUTTON_0   (GUI_ID_USER + 0x0C)
#define ID_BUTTON_1   (GUI_ID_USER + 0x0D)
#define ID_BUTTON_2   (GUI_ID_USER + 0x0E)
#define ID_TEXT_1     (GUI_ID_USER + 0x10)
#define ID_TEXT_2     (GUI_ID_USER + 0x11)
#define ID_TEXT_3     (GUI_ID_USER + 0x12)
#define ID_TEXT_4     (GUI_ID_USER + 0x13)
#define ID_TEXT_5     (GUI_ID_USER + 0x14)
#define ID_EDIT_0     (GUI_ID_USER + 0x15)
#define ID_EDIT_1     (GUI_ID_USER + 0x16)

/**
 * TSC status check delay
 */
#define TSC_CHECK_DELAY   (20)

/**
 * Widget Frame window array
 */
static const GUI_WIDGET_CREATE_INFO _aDialogCreate[] = {
	{FRAMEWIN_CreateIndirect, "emWin Demo",          ID_FRAMEWIN_0, 8,   8,   310, 230, 0, 0, 0},
	{TEXT_CreateIndirect,     "NXP Semiconductors",  ID_TEXT_0,     30,  10,  260, 100, TEXT_CF_HCENTER},
	{TEXT_CreateIndirect,     "0000",                ID_TEXT_1,     100, 80,  100, 60,  TEXT_CF_HCENTER},
	{BUTTON_CreateIndirect,   "START",               ID_BUTTON_0,   36,  160, 70,  30,  0, 0, 0},
	{BUTTON_CreateIndirect,   "STOP",                ID_BUTTON_1,   126, 160, 60,  30},
	{BUTTON_CreateIndirect,   "RESET",               ID_BUTTON_2,   203, 160, 70,  30},
};

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/**
 * @brief Touch screen initialisation done flag
 */
volatile int tsc_init_done = 0;				/*!< TSC Initiliation done flag */

/**
 * @brief GUI buffers required for emwin library
 */
#define GUI_BUF_ADDR  0x28500000				/*!< Start address of GUI buffer in SDRAM */
#define GUI_NUMBYTES  ((1024 * 1024) * 2)	/*!< Size of GUI buffer in No. of bytes */
#define GUI_BLOCKSIZE (128)				/*!< GUI block size */

uint32_t *GUI_Memory = (uint32_t *) GUI_BUF_ADDR;	/*!< GUI buffer */
U32 GUI_Memory_Size = GUI_NUMBYTES;					/*!< GUI buffer size */
U32 GUI_Block_Size = GUI_BLOCKSIZE;					/*!< GUI block size */

uint16_t *Frame_Buf = (uint16_t *) FRAMEBUFFER_ADDR;	/*!< Frame buffer */

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Update the host IP address in Emwin Window */
static void lcd_create_infobox(void)
{
	WM_HWIN hItem;
	static char name[] = {"HTTP Server Address"};
	static char remote_name[] = {"Remote IP address"};

	/* Create HTTP server address Text box */
	hItem  = TEXT_CreateEx(24, 40, 150, 100, WM_GetClientWindow(hWin), WM_CF_SHOW, 0, ID_TEXT_2, name);
	TEXT_SetFont(hItem, &GUI_Font16B_ASCII);
	TEXT_SetTextAlign(hItem, GUI_TA_HCENTER);
	TEXT_SetTextColor(hItem, GUI_DARKCYAN);

	/* Create Remote IP address Text box */
	hItem  = TEXT_CreateEx(24, 60, 150, 100, WM_GetClientWindow(hWin), WM_CF_SHOW, 0, ID_TEXT_3, remote_name);
	TEXT_SetFont(hItem, &GUI_Font16B_ASCII);
	TEXT_SetTextAlign(hItem, GUI_TA_HCENTER);
	TEXT_SetTextColor(hItem, GUI_DARKGREEN);

	/* Create Host IP edit box */
	hItem  = EDIT_CreateEx(170, 40, 100, 20, WM_GetClientWindow(hWin), WM_CF_SHOW, 0, ID_EDIT_0, 16);

	/* Create Remote IP edit box */
	hItem  = EDIT_CreateEx(170, 60, 100, 20, WM_GetClientWindow(hWin), WM_CF_SHOW, 0, ID_EDIT_1, 16);
}

/**
 * Update EDIT boxes on LCD function
 */
static void lcd_update_values(WM_HWIN hWin_up)
{
	WM_HWIN hItem;
	char tbuff[16];
	static uint32_t ipaddr, hostip;

	if (ipaddr != ipcex_getGblVal(SHGBL_IPADDR)) {
		if (!ipaddr) lcd_create_infobox();
		ipaddr = ipcex_getGblVal(SHGBL_IPADDR);
		/* Set Host IP Address */
		hItem = WM_GetDialogItem(hWin_up, ID_EDIT_0);
		EDIT_SetTextAlign(hItem, GUI_TA_HCENTER);
		EDIT_SetBkColor(hItem, EDIT_CI_DISABLED, GUI_GREEN);
		EDIT_SetTextColor(hItem, EDIT_CI_DISABLED, GUI_GREEN);
		snprintf(tbuff, sizeof(tbuff) - 1, "%d.%d.%d.%d",
			ipaddr & 0xFF, (ipaddr >> 8) & 0xFF,
			(ipaddr >> 16) & 0xFF, (ipaddr >> 24) & 0xFF);
		EDIT_SetText(hItem, tbuff);
	}

	if (ipaddr && hostip != ipcex_getGblVal(SHGBL_HOSTADDR)) {
		hostip = ipcex_getGblVal(SHGBL_HOSTADDR);
		/* Set Remote IP address */
		hItem = WM_GetDialogItem(hWin_up, ID_EDIT_1);
		EDIT_SetTextAlign(hItem, GUI_TA_HCENTER);
		EDIT_SetBkColor(hItem, EDIT_CI_DISABLED, GUI_RED);
		EDIT_SetTextColor(hItem, EDIT_CI_DISABLED, GUI_RED);
		snprintf(tbuff, sizeof(tbuff)-1, "%d.%d.%d.%d",
			hostip & 0xFF, (hostip >> 8) & 0xFF,
			(hostip >> 16) & 0xFF, (hostip >> 24) & 0xFF);
		EDIT_SetText(hItem, tbuff);
	}
}

/**
 * GUI Frame window callback function
 */
static void _cbCallback(WM_MESSAGE *pMsg)
{
	int NCode, Id;
	WM_HWIN hDlg;
	WM_HWIN hItem;
	int fdig, sdig, tdig, frdig;
	BUTTON_Handle hButton;
	char acText[5] = {0};

	hDlg = pMsg->hWin;
	switch (pMsg->MsgId) {
	case WM_PAINT:
		WM_DefaultProc(pMsg);	/* Handle dialog items */
		break;

	case WM_INIT_DIALOG:
		FRAMEWIN_SetTextAlign(hDlg, GUI_TA_HCENTER | GUI_TA_VCENTER);
		FRAMEWIN_SetFont(hDlg, GUI_FONT_24_ASCII);

		/* set the Text properties */
		hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_0);
		TEXT_SetText(hItem, "NXP SEMICONDUCTORS");
		TEXT_SetFont(hItem, &GUI_Font24B_ASCII);
		TEXT_SetTextAlign(hItem, (GUI_TA_HCENTER));
		TEXT_SetTextColor(hItem, GUI_YELLOW);

		hItem = WM_GetDialogItem(pMsg->hWin, ID_TEXT_1);
		counter = counter % 10000;
		fdig = counter / 1000;
		sdig = (counter - (fdig * 1000)) / 100;
		tdig = (counter - ((fdig * 1000) + (sdig * 100))) / 10;
		frdig = counter % 10;
		acText[0] = '0' + fdig;
		acText[1] = '0' + sdig;
		acText[2] = '0' + tdig;
		acText[3] = '0' + frdig;
		TEXT_SetFont(hItem, &GUI_Font8x16x1x2);
		TEXT_SetTextAlign(hItem, (GUI_TA_HCENTER));
		TEXT_SetTextColor(hItem, GUI_RED);
		TEXT_SetText(hItem, acText);
		break;

	case WM_KEY:
		break;

	case WM_NOTIFY_PARENT:

		Id    = WM_GetId(pMsg->hWinSrc);	/* Id of widget */
		NCode = pMsg->Data.v;					/* Notification code */
		switch (NCode) {
		case WM_NOTIFICATION_RELEASED:	/* React only if released */
			if (Id == ID_BUTTON_0) {	/* Start button */
				hButton = WM_GetDialogItem(hDlg, ID_BUTTON_0);
				start = 1;
				WM_InvalidateWindow(hButton);
			}

			if (Id == ID_BUTTON_1) {	/* Stop button */
				hButton = WM_GetDialogItem(hDlg, ID_BUTTON_1);
				start = 0;
				WM_InvalidateWindow(hButton);
			}

			if (Id == ID_BUTTON_2) {	/* Reset button */
				hButton = WM_GetDialogItem(hDlg, ID_BUTTON_2);
				counter = 0;
				start = 0;
				WM_InvalidateWindow(hButton);
			}
			break;
		}
		break;

	default:
		WM_DefaultProc(pMsg);
	}
}

/**
 * LCD appplication task function for FreeRTOS & uCOS-III
 */
#if (defined(OS_FREE_RTOS)) || (defined(OS_UCOS_III))

/* LCd Display thread */
static void vLcdTask(void *pvParameters)
{
	WM_HWIN hItem;
	int fdig, sdig, tdig, frdig;
	char acText[5] = {0};

	GUI_Init();
	WM_SetDesktopColor(GUI_GREEN);
	WM_SetCreateFlags(WM_CF_MEMDEV);
	hWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), &_cbCallback, 0, 0, 0);

	while (1) {
		if (start) {
			counter++;
			if (counter > 9999) {
				counter = 0;
			}
		}

		/* Print Count value */
		hItem = WM_GetDialogItem(hWin, ID_TEXT_1);
		counter = counter % 10000;
		fdig = counter / 1000;
		sdig = (counter - (fdig * 1000)) / 100;
		tdig = (counter - ((fdig * 1000) + (sdig * 100))) / 10;
		frdig = counter % 10;
		acText[0] = '0' + fdig;
		acText[1] = '0' + sdig;
		acText[2] = '0' + tdig;
		acText[3] = '0' + frdig;
		TEXT_SetFont(hItem, &GUI_Font8x16x3x3);
		TEXT_SetTextAlign(hItem, (GUI_TA_HCENTER));
		TEXT_SetTextColor(hItem, GUI_RED);
		TEXT_SetText(hItem, acText);

		lcd_update_values(hWin);

		GUI_Delay(10);
	}
}

/**
 * TSC appplication task function for FreeRTOS & uCOS-III
 */
static void vTSCTask(void *pvParameters)
{
	int16_t tmp_x = -1, tmp_y = -1;
	int16_t tmp_x1 = -1, tmp_y1 = -1;
	static uint8_t pressed = 0;
	bool touched;
#ifdef  OS_UCOS_III
	OS_ERR ret;
#endif

	while (1) {
		/* Wait for TSC_CHECK_DELAY ms */
#ifdef  OS_UCOS_III
		OSTimeDlyHMSM(0, 0, TSC_CHECK_DELAY / 1000, TSC_CHECK_DELAY % 1000, OS_OPT_TIME_HMSM_STRICT, &ret);
#else
		vTaskDelay(TSC_CHECK_DELAY);
#endif

		/* Check any Touch screen events */
		touched = Board_GetTouchPos((int16_t *) &tmp_x, (int16_t *) &tmp_y);
		if (touched == true) {
			if (pressed == 1) {
				if ((tmp_x >= 0) && (tmp_y > 0) && ((tmp_x != old_tmp_x) || (tmp_y != old_tmp_y))) {
					tmp_x1 = tmp_y;
					tmp_y1 = tmp_x;
					GUI_TOUCH_StoreState(320 - tmp_x1, tmp_y1);
					old_tmp_x = tmp_x;
					old_tmp_y = tmp_y;
				}
			}
			else {
				GUI_TOUCH_StoreState(320 - tmp_x1, tmp_y1);
				old_tmp_x = tmp_x;
				old_tmp_y = tmp_y;
				pressed = 1;
			}
		}
		else {
			if (pressed == 1) {
				GUI_TOUCH_StoreState(-1, -1);
				pressed = 0;
			}
		}
	}
}

#endif

/*****************************************************************************
 * Public functions
 ****************************************************************************/

extern void SysTick_Enable(uint32_t period);

/* emWin example initialization function */
void EMWIN_Init(void)
{
	/* Initialise the LCD controller */
	Board_LCD_Init();
	Chip_LCD_Init(LPC_LCD, (LCD_CONFIG_T *) &BOARD_LCD);
	Board_InitTouchController();
	Chip_LCD_SetUPFrameBuffer(LPC_LCD, (void *) Frame_Buf);
	Chip_LCD_PowerOn(LPC_LCD);
	Board_SetLCDBacklight(1);

	#if !defined(OS_UCOS_III) && !defined(OS_FREE_RTOS)
	/* Setup a 1mS sysTick for the primary time base */
	SysTick_Enable(1);

	GUI_Init();

	WM_SetDesktopColor(GUI_GREEN);
	WM_SetCreateFlags(WM_CF_MEMDEV);
	hWin = GUI_CreateDialogBox(_aDialogCreate, GUI_COUNTOF(_aDialogCreate), &_cbCallback, 0, 0, 0);
	GUI_Delay(10);

	#endif

	tsc_init_done = 1;

}

#if (defined(OS_FREE_RTOS)) || (defined(OS_UCOS_III))
/* OS specific dualcore emWin tasks */
void emwin_tasks(void)
{
#ifdef OS_FREE_RTOS
	xTaskCreate(vTSCTask, "vTSCTask",
			FREERTOS_TSC_STACK_SZ, NULL, TASK_PRIO_TOUCHSCREEN,
				(TaskHandle_t *) NULL);
	xTaskCreate(vLcdTask, "vLCDTask",
			FREERTOS_LCD_STACK_SZ, NULL, TASK_PRIO_LCD,
				(TaskHandle_t *) NULL);
#elif OS_UCOS_III
	OS_ERR os_err;
	static OS_TCB lcd_app_taskTCB;
	static CPU_STK lcd_app_taskSTK[UCOS_LCD_STACK_SZ];
	static OS_TCB tsc_app_taskTCB;
	static CPU_STK tsc_app_taskSTK[UCOS_TSC_STACK_SZ];

	OSTaskCreate((OS_TCB      *) &lcd_app_taskTCB,
				 (CPU_CHAR    *) "vLCDTask",
				 (OS_TASK_PTR) vLcdTask,
				 (void        *) 0,
				 (OS_PRIO) TASK_PRIO_LCD,
				 (CPU_STK     *) &lcd_app_taskSTK[0],
				 (CPU_STK_SIZE) 32,
				 (CPU_STK_SIZE) UCOS_LCD_STACK_SZ,
				 (OS_MSG_QTY) 0u,
				 (OS_TICK) 0u,
				 (void        *) 0,
				 (OS_OPT) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 (OS_ERR      *) &os_err);
	if (os_err != OS_ERR_NONE) {
		printf("Unable to create vLCDTask task!\r\n");
		while (1) ;
	}
	OSTaskCreate((OS_TCB      *) &tsc_app_taskTCB,
				 (CPU_CHAR    *) "vTSCTask",
				 (OS_TASK_PTR) vTSCTask,
				 (void        *) 0,
				 (OS_PRIO) TASK_PRIO_TOUCHSCREEN,
				 (CPU_STK     *) &tsc_app_taskSTK[0],
				 (CPU_STK_SIZE) 32,
				 (CPU_STK_SIZE) UCOS_TSC_STACK_SZ,
				 (OS_MSG_QTY) 0u,
				 (OS_TICK) 0u,
				 (void        *) 0,
				 (OS_OPT) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
				 (OS_ERR      *) &os_err);
	if (os_err != OS_ERR_NONE) {
		printf("Unable to create vTSCTask task!\r\n");
		while (1) ;
	}
#endif
}

#else
/* Standalone emWin dual core example task */
void emwin_tasks(void)
{
	WM_HWIN hItem;
	int fdig, sdig, tdig, frdig;
	char acText[5] = {0};

	do {
		if (start) {
			counter++;
			if (counter > 9999) {
				counter = 0;
			}
		}

		/* Print Count value */
		hItem = WM_GetDialogItem(hWin, ID_TEXT_1);
		counter = counter % 10000;
		fdig = counter / 1000;
		sdig = (counter - (fdig * 1000)) / 100;
		tdig = (counter - ((fdig * 1000) + (sdig * 100))) / 10;
		frdig = counter % 10;
		acText[0] = '0' + fdig;
		acText[1] = '0' + sdig;
		acText[2] = '0' + tdig;
		acText[3] = '0' + frdig;
		TEXT_SetFont(hItem, &GUI_Font8x16x3x3);
		TEXT_SetTextAlign(hItem, (GUI_TA_HCENTER));
		TEXT_SetTextColor(hItem, GUI_RED);
		TEXT_SetText(hItem, acText);

		lcd_update_values(hWin);
		GUI_Exec();
	} while (0);
}

#endif
