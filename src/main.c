/**
  ******************************************************************************
  * @file    Templates/Src/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    22-April-2016
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32F7xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define MAX_ELEMENTS 100

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern uint8_t _isr_vector_ram_start asm("_isr_vector_ram_start");     /* Defined by the linker. */
extern uint8_t _isr_vector_flash_start asm("_isr_vector_flash_start"); /* Defined by the linker. */
extern uint8_t _isr_vector_flash_end asm("_isr_vector_flash_end");     /* Defined by the linker. */

USBH_HandleTypeDef hUSBHost;
uint8_t host_state;

char rom_path[MAX_ELEMENTS][256];
uint8_t file_selected;

osThreadId ui_thread_id;
osThreadId nes_thread_id;

FATFS FatFs;  /* File system object for SD card logical drive */
char Path[4]; /* SD card logical drive path */

extern SD_HandleTypeDef uSdHandle;

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void Error_Handler(void);
static void MPU_Config(void);
static void CPU_CACHE_Enable(void);

static void USBThread(void const *argument);
static void NESThread(void const *argument);
static void UIThread(void const *argument);

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

static void GetRomPath(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Copy ISRs to RAM */
  memcpy(	(uint8_t *)&_isr_vector_ram_start,
		(uint8_t *)&_isr_vector_flash_start,
		&_isr_vector_flash_end - &_isr_vector_flash_start );

  /* Relocate the vector table */
  SCB->VTOR = (uint32_t) &_isr_vector_ram_start;

  /* This project template calls firstly two functions in order to configure MPU feature 
     and to enable the CPU Cache, respectively MPU_Config() and CPU_CACHE_Enable().
     These functions are provided as template implementation that User may integrate 
     in his application, to enhance the performance in case of use of AXI interface 
     with several masters. */ 
  
  /* Configure the MPU attributes as Write Through */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 216 MHz */
  SystemClock_Config();

  /* LCD DSI initialization in mode Video Burst */
  /* Initialize DSI LCD */
  while(BSP_LCD_InitEx(LCD_ORIENTATION_LANDSCAPE) != LCD_OK);

  BSP_LCD_SetBrightness(100);

  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);

  BSP_LCD_SelectLayer(0);

  /* Link the SD Card disk I/O driver */
  FATFS_LinkDriver(&SD_Driver, Path);
  BSP_SD_Detect_MspInit(&uSdHandle, NULL);

  while( BSP_SD_IsDetected() != 1 );
  while( f_mount(&FatFs, (TCHAR const*)Path, 1) != FR_OK );

  /* Start Host Library */
  USBH_Init(&hUSBHost, USBH_UserProcess, 0);

  /* Add Supported Class */
  USBH_RegisterClass(&hUSBHost, USBH_HID_CLASS);

  /* Start Host Process */
  USBH_Start(&hUSBHost);

  /* USB task */
  osThreadDef(USB_Thread, USBThread, osPriorityNormal, 0, 8 * configMINIMAL_STACK_SIZE);
  osThreadCreate(osThread(USB_Thread), NULL);

  /* NES task */
  osThreadDef(NES_Thread, NESThread, osPriorityNormal, 0, 8 * configMINIMAL_STACK_SIZE);
  nes_thread_id = osThreadCreate(osThread(NES_Thread), NULL);

  /* UI task */
  osThreadDef(UI_Thread, UIThread, osPriorityNormal, 0, 8 * configMINIMAL_STACK_SIZE);
  ui_thread_id = osThreadCreate(osThread(UI_Thread), NULL);

  osThreadSuspend (nes_thread_id);
  osThreadSuspend (ui_thread_id);

  /* Start scheduler */
  osKernelStart();

  /* Infinite loop */
  while (1)
  {
  }
}

static void USBThread(void const * argument)
{
	for( ;; )
	{
	    /* USB Host Background task */
	    USBH_Process(&hUSBHost);
	}
}

static void NESThread(void const * argument)
{
nes_start:
#ifdef SCREEN_DOBLE
    BSP_LCD_SetLayerWindow(0,
						   (BSP_LCD_GetXSize() - 256 * 2) / 2,
						   (BSP_LCD_GetYSize() - 220 * 2) / 2,
						   256 * 2, 220 * 2 );
#else
    BSP_LCD_SetLayerWindow(0,
						   (BSP_LCD_GetXSize() - 256) / 2,
						   (BSP_LCD_GetYSize() - 220) / 2,
						   256, 220 );
#endif

    pNesX_Load( &rom_path[file_selected][0] );

    ApuInit();

    pNesX_Main();

	osThreadResume (ui_thread_id);
	osThreadSuspend (nes_thread_id);

	goto nes_start;
}

static void UIThread(void const * argument)
{
ui_start:
	BSP_LCD_SetLayerWindow(	0,
							0,
							0,
							BSP_LCD_GetXSize(), BSP_LCD_GetYSize() );

	GetRomPath();

	osThreadResume (nes_thread_id);

	osThreadSuspend (ui_thread_id);

	goto ui_start;

	for( ;; )
	{
	}
}

void str_cpy(char *dst, const char *src)
{
	do
	{
		*dst = *src;
		src++;
		dst++;
	} while(*src != 0);
	*dst = 0;
}

/**
  * @brief
  * @param
  * @retval
  */
static void GetRomPath(void)
{
	HID_KEYBD_Info_TypeDef *k_pinfo;
	char 		c[] = {0, 0}, str[256], pathstr[256], *strptr, *pathptr;
	char 		numstr[] = {0, 0, 0, 0}, *numptr = numstr;
	WORD        bCounter;
	FRESULT 	fr;
	DIR 		dir;
	FILINFO 	finfo;

	/* Initialize LCD Log module */
  	LCD_LOG_Init();

	f_chdir ("0:/nesroms");

	fr = f_findfirst (&dir, &finfo, "", "*.nes");
	for(bCounter = 0; bCounter < MAX_ELEMENTS; bCounter++)
	{
		if(fr == FR_OK)
		{
			if( (finfo.fattrib == AM_ARC) || (finfo.fattrib == (AM_ARC | AM_RDO)) )
			{
				if( (finfo.fname[0] == '.' || finfo.fname[0] == '\0') )
				{
					fr = f_findnext(&dir, &finfo);
					continue;
				}

				strptr = &str,
				sprintf(str, "%d: " , bCounter);
				strptr += strlen(str);
				str_cpy(strptr, finfo.fname);
				strptr += strlen(finfo.fname);
				str_cpy(strptr, "\n");
				LCD_UsrLog (str);

				pathptr = &pathstr;
				f_getcwd (&pathstr, sizeof(pathstr));
				pathptr += strlen(pathstr);
				str_cpy(pathptr, "/");
				pathptr += strlen("/");
				str_cpy(pathptr, finfo.fname);
				str_cpy(&rom_path[bCounter][0], pathstr);
			}
		}
		else
		{
			break;
		}

		fr = f_findnext(&dir, &finfo);
	}

	do
	{
		if(host_state == HOST_USER_CLASS_ACTIVE)
		{
			k_pinfo = USBH_HID_GetKeybdInfo(&hUSBHost);
			if(k_pinfo != NULL)
			{
				c[0] = USBH_HID_GetASCIICode(k_pinfo);

				if(c[0] == 'w')
					LCD_LOG_ScrollBack();
				else if(c[0] == 's')
					LCD_LOG_ScrollForward();

				if(c[0] != 0 && c[0] != '\n' && c[0] != 'w' && c[0] != 's')
				{
					LCD_UsrLog ("%c\n", c[0]);
					str_cpy(numptr, c);
					numptr += strlen(numptr);
				}
			}
		}
	} while(c[0] != '\n');

	file_selected = atoi(numstr);
}

/**
  * @brief  User Process
  * @param  phost: Host Handle
  * @param  id: Host Library user message ID
  * @retval None
  */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  	switch(id)
  	{
  		case HOST_USER_SELECT_CONFIGURATION:
    			break;

 	 	case HOST_USER_DISCONNECTION:
 	 			host_state = HOST_USER_DISCONNECTION;
    			break;

  		case HOST_USER_CLASS_ACTIVE:
  				osDelay (100);

  				host_state = HOST_USER_CLASS_ACTIVE;

  				osThreadResume (ui_thread_id);
    			break;

  		case HOST_USER_CONNECTION:
  				host_state = HOST_USER_CONNECTION;
    			break;

  		default:
    			break;
  	}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            PLL_R                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 480;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 10;
  RCC_OscInitStruct.PLL.PLLR = 8;
  
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Activate the OverDrive to reach the 216 MHz Frequency */  
  ret = HAL_PWREx_EnableOverDrive();
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; 
  
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
  if(ret != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

/**
  * @brief  Configure the MPU attributes as Write Through for Internal SRAM1/2.
  * @note   The Base Address is 0x20020000 since this memory interface is the AXI.
  *         The Configured Region Size is 512KB because the internal SRAM1/2 
  *         memory size is 384KB.
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();

  /* Configure the MPU attributes as WT for SRAM */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x20020000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
