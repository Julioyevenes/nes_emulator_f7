/**
  ******************************************************************************
  * @file    ${file_name} 
  * @author  ${user}
  * @version 
  * @date    ${date}
  * @brief   
  ******************************************************************************
  * @attention
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "pNesX_System.h"

#include "pNesX.h"

#include "ff.h"
#include "stm32f769i_discovery_lcd.h"
#include "stm32f769i_discovery_audio.h"

#include "usbh_hid.h"

/* Private types ------------------------------------------------------------*/
typedef struct _KeyPressed_TypeDef
{
	uint8_t ascii[6];
} KeyPressed_TypeDef;

/* Private constants --------------------------------------------------------*/
#define SCREEN_WIDTH 	256
#define SAMPLE_FREQ 	24000

#define TIMx                TIM4
#define TIMx_CLK_ENABLE     __HAL_RCC_TIM4_CLK_ENABLE
#define TIMx_IRQn           TIM4_IRQn
#define TIMx_IRQHandler     TIM4_IRQHandler

/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
FIL nesfile;
WORD br;

WORD Sample[2] = {0};

BYTE restart_emulator = 0;

KeyPressed_TypeDef KeyASCII;

TIM_HandleTypeDef TimHandle;

static  const  int8_t  HID_KEYBRD_Key[] = {
  '\0',  '`',  '1',  '2',  '3',  '4',  '5',  '6',
  '7',  '8',  '9',  '0',  '-',  '=',  '\0', '\r',
  '\t',  'q',  'w',  'e',  'r',  't',  'y',  'u',
  'i',  'o',  'p',  '[',  ']',  '\\',
  '\0',  'a',  's',  'd',  'f',  'g',  'h',  'j',
  'k',  'l',  ';',  '\'', '\0', '\n',
  '\0',  '\0', 'z',  'x',  'c',  'v',  'b',  'n',
  'm',  ',',  '.',  '/',  '\0', '\0',
  '\0',  '\0', '\0', ' ',  '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '\0', '\0', '\0', '\r', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '7',  '4',  '1',
  '\0',  '/',  '8',  '5',  '2',
  '0',   '*',  '9',  '6',  '3',
  '.',   '-',  '+',  '\0', '\n', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0'
};

static  const  int8_t  HID_KEYBRD_ShiftKey[] = {
  '\0', '~',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
  '_',  '+',  '\0', '\0', '\0', 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',
  'I',  'O',  'P',  '{',  '}',  '|',  '\0', 'A',  'S',  'D',  'F',  'G',
  'H',  'J',  'K',  'L',  ':',  '"',  '\0', '\n', '\0', '\0', 'Z',  'X',
  'C',  'V',  'B',  'N',  'M',  '<',  '>',  '?',  '\0', '\0',  '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0',    '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

static  const  uint8_t  HID_KEYBRD_Codes[] = {
  0,     0,    0,    0,   31,   50,   48,   33,
  19,   34,   35,   36,   24,   37,   38,   39,       /* 0x00 - 0x0F */
  52,    51,   25,   26,   17,   20,   32,   21,
  23,   49,   18,   47,   22,   46,    2,    3,       /* 0x10 - 0x1F */
  4,    5,    6,    7,    8,    9,   10,   11,
  43,  110,   15,   16,   61,   12,   13,   27,       /* 0x20 - 0x2F */
  28,   29,   42,   40,   41,    1,   53,   54,
  55,   30,  112,  113,  114,  115,  116,  117,       /* 0x30 - 0x3F */
  118,  119,  120,  121,  122,  123,  124,  125,
  126,   75,   80,   85,   76,   81,   86,   89,       /* 0x40 - 0x4F */
  79,   84,   83,   90,   95,  100,  105,  106,
  108,   93,   98,  103,   92,   97,  102,   91,       /* 0x50 - 0x5F */
  96,  101,   99,  104,   45,  129,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x60 - 0x6F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x70 - 0x7F */
  0,    0,    0,    0,    0,  107,    0,   56,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x80 - 0x8F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x90 - 0x9F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xA0 - 0xAF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xB0 - 0xBF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xC0 - 0xCF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xD0 - 0xDF */
  58,   44,   60,  127,   64,   57,   62,  128        /* 0xE0 - 0xE7 */
};

const WORD NesPalette[ 64 ] =
{
	0x6b5b, 0x0123, 0x0029, 0x3823, 0x681b, 0x700f, 0x6801, 0x5881,
	0x4101, 0x3181, 0x0241, 0x0211, 0x01d7, 0x0000, 0x0000, 0x0000,
	0xadb7, 0x1af5, 0x31bf, 0x78fb, 0xb817, 0xe00b, 0xc8c7, 0xca41,
	0x7a81, 0x5301, 0x0401, 0x0399, 0x0411, 0x0000, 0x0000, 0x0000,
	0xffdf, 0x3ddf, 0x5c9f, 0x445f, 0xf3df, 0xfb96, 0xfb8c, 0xfc8d,
	0xf5c7, 0x8682, 0x4ec9, 0x5fd3, 0x075b, 0x0000, 0x0000, 0x0000,
	0xffdf, 0xaf1f, 0xc69f, 0xd65f, 0xfe1f, 0xfe1b, 0xfdd6, 0xfed5,
	0xff14, 0xef67, 0xbfef, 0xa77b, 0x9f3f, 0xbdef, 0x0000, 0x0000
};

extern WORD *pDrawData;
extern USBH_HandleTypeDef hUSBHost;
extern uint8_t host_state;
extern SAI_HandleTypeDef haudio_out_sai;
extern AUDIO_DrvTypeDef *audio_drv;

/* Private function prototypes ----------------------------------------------*/
void USBH_HID_GetASCIICode_Multi(HID_KEYBD_Info_TypeDef *info);

int pNesX_Menu()
{
	if(restart_emulator)
	{
		restart_emulator = 0;
		return -1;
	}

	return 0;
}

int pNesX_ReadRom( const char *pszFileName )
{
	/* Open ROM file */
	if(f_open(&nesfile, pszFileName, FA_READ) != FR_OK)
		return -1;

	/* Read ROM Header */
	if(f_read(&nesfile, &NesHeader, sizeof(NesHeader), &br) != FR_OK)
		return -1;

	if ( memcmp( NesHeader.byID, "NES\x1a", 4 ) != 0 )
	{
		/* not .nes file */
		f_close(&nesfile);
		return -1;
	}

	/* Clear SRAM */
	memset( SRAM, 0, SRAM_SIZE );

	/* If trainer presents Read Triner at 0x7000-0x71ff */
	if ( NesHeader.byInfo1 & 4 )
	{
		f_read(&nesfile, &SRAM[ 0x1000 ], 512, &br);
	}

	/* Allocate Memory for ROM Image */
	ROM = (BYTE *) malloc( NesHeader.byRomSize * 0x4000 );

	/* Read ROM Image */
	f_read(&nesfile, ROM, 0x4000 * NesHeader.byRomSize, &br);

	if ( NesHeader.byVRomSize > 0 )
	{
		/* Allocate Memory for VROM Image */
		VROM = (BYTE *) malloc( NesHeader.byVRomSize * 0x2000 );

		/* Read VROM Image */
		f_read(&nesfile, VROM, 0x2000 * NesHeader.byVRomSize, &br);
	}

	/* File close */
	f_close(&nesfile);

	/* Successful */
	return 0;
}

void pNesX_ReleaseRom()
{
	if ( ROM )
	{
		free( ROM );
		ROM = NULL;
	}

	if ( VROM )
	{
		free( VROM );
		VROM = NULL;
	}
}

void pNesX_LoadFrame()
{
}

void pNesX_TransmitLinedata(WORD CurLine)
{
#ifdef SCREEN_DOBLE
	DWORD x, pixel;

	for(x = 0; x < SCREEN_WIDTH; x++)
	{
		pixel = ((CurLine * 2) * (SCREEN_WIDTH * 2)) + (x * 2);

		((uint16_t*)LCD_FB_START_ADDRESS)[pixel    ] = pDrawData[x];
		((uint16_t*)LCD_FB_START_ADDRESS)[pixel + 1] = pDrawData[x];

		((uint16_t*)LCD_FB_START_ADDRESS)[(SCREEN_WIDTH * 2) + pixel    ] = pDrawData[x];
		((uint16_t*)LCD_FB_START_ADDRESS)[(SCREEN_WIDTH * 2) + pixel + 1] = pDrawData[x];
	}
#else
	DWORD x, pixel;

	for(x = 0; x < SCREEN_WIDTH; x++)
	{
		pixel = (CurLine * SCREEN_WIDTH) + x;
		((uint16_t*)LCD_FB_START_ADDRESS)[pixel    ] = pDrawData[x];
	}
#endif
}

void pNesX_PadState( DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem )
{
	HID_KEYBD_Info_TypeDef *k_pinfo;
	uint8_t i;

	if(host_state == HOST_USER_CLASS_ACTIVE)
	{
		k_pinfo = USBH_HID_GetKeybdInfo(&hUSBHost);
		if(k_pinfo != NULL)
		{
			USBH_HID_GetASCIICode_Multi(k_pinfo);

			for(i = 0; i < 6 ; i++)
			{
				switch(KeyASCII.ascii[i])
				{
					case '\r':
						restart_emulator = 1;
						break;

					case 'w':
						*pdwPad1 |= 0b00010000;
						break;

					case 's':
						*pdwPad1 |= 0b00100000;
						break;

					case 'a':
						*pdwPad1 |= 0b01000000;
						break;

					case 'd':
						*pdwPad1 |= 0b10000000;
						break;

					case 'o':
						*pdwPad1 |= 0b00000001;
						break;

					case 'p':
						*pdwPad1 |= 0b00000010;
						break;

					case 'u':
						*pdwPad1 |= 0b00000100;
						break;

					case 'i':
						*pdwPad1 |= 0b00001000;
						break;

					default:
						if(KeyASCII.ascii[0] == 0)
							*pdwPad1 = 0;
						break;
				}
			}
		}
	}
}

void *pNesX_MemoryCopy( void *dest, const void *src, int count )
{
	memcpy(dest, src, count );
	return dest;
}

void *pNesX_MemorySet( void *dest, int c, int count )
{
	memset(dest, c, count);
	return dest;
}

void pNesX_DebugPrint( char *pszMsg )
{
}

void pNesX_InitSound(void)
{
	if( BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, 50, SAMPLE_FREQ) == 0 )
	{
		BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);

		audio_drv->Play(AUDIO_I2C_ADDRESS, 0, 0);
	}

    TimHandle.Instance = TIMx;
    TimHandle.Init.Prescaler = (uint32_t) ((SystemCoreClock / 2) / (SAMPLE_FREQ * 10)) - 1;
    TimHandle.Init.Period = 10 - 1;
    TimHandle.Init.ClockDivision = 0;
    TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&TimHandle);

    HAL_TIM_Base_Start_IT(&TimHandle);
}

void pNesX_MuteSound(BYTE mute)
{
    if (mute)
    {
        HAL_TIM_Base_Stop_IT(&TimHandle);
    }
    else
    {
        HAL_TIM_Base_Start_IT(&TimHandle);
    }
}

void pNesX_ApuStep(void)
{
}

void USBH_HID_GetASCIICode_Multi(HID_KEYBD_Info_TypeDef *info)
{
  uint8_t i;

  if((info->lshift == 1) || (info->rshift))
  {
	for(i = 0; i < 6 ; i++)
	{
  		KeyASCII.ascii[i] =  HID_KEYBRD_ShiftKey[HID_KEYBRD_Codes[info->keys[i]]];
	}
  }
  else
  {
	for(i = 0; i < 6 ; i++)
	{
  		KeyASCII.ascii[i] =  HID_KEYBRD_Key[HID_KEYBRD_Codes[info->keys[i]]];
	}
  }
}

void TIMx_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	TIMx_CLK_ENABLE();

	HAL_NVIC_SetPriority(TIMx_IRQn, 4, 0);
	HAL_NVIC_EnableIRQ(TIMx_IRQn);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	pNesX_FillSample(&Sample[0]);
	pNesX_FillSample(&Sample[1]);

	Sample[0] += 0x8000;
	Sample[1] += 0x8000;

	haudio_out_sai.Instance->DR = (uint16_t) Sample[0];
	haudio_out_sai.Instance->DR = (uint16_t) Sample[1];
}
