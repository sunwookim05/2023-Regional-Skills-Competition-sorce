/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "DS3231.h"
#include "SK6812.h"
#include "ssd1306.h"
#include "stm32l0xx_EEPROM.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum bool{false, true};
typedef uint8_t boolean;
typedef char *String;
typedef uint8_t MODE;
typedef enum __MODE__{
	MAIN, PSAVE, PUSE, USE, REFILL, PFIND, FINDR, PARTITION, PLOG, LOGD
}__MODE__;

typedef struct TIME {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} TIME;

typedef struct DATE {
	uint8_t day;
	uint8_t month;
	uint16_t year;
} DATE;

typedef struct PART{
	volatile String name;
	uint8_t cate;
	uint8_t partF;
	uint8_t pos;

	uint8_t ptionID;
	uint8_t ptionC;

	uint16_t store;
	uint16_t max;
}PART;

typedef struct PLOG{
	uint8_t workCate;
	char content[2][22];
	TIME time;
	DATE date;
}PTLOG;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUZ(x) (HAL_GPIO_WritePin(BUZ_GPIO_Port, BUZ_Pin, x))
#define ADC_JOY_X (adc[0])
#define ADC_JOY_Y (adc[1])
#define JOY_R (ADC_JOY_X > 4000)
#define JOY_L (ADC_JOY_X < 300)
#define JOY_U (ADC_JOY_Y > 4000)
#define JOY_D (ADC_JOY_Y < 300)
#define JOY_P (!HAL_GPIO_ReadPin(JOY_SW_GPIO_Port, JOY_SW_Pin))
#define RE1 (reC>=100)

#define UPDATE SSD1306_UpdateScreen(); led_update()
#define PTR pt[i].cate == 1 ? 1 : pt[i].cate == 2 ? 4 : pt[i].cate == 3 ? 0 : 4
#define PTG pt[i].cate == 1 ? 0 : pt[i].cate == 2 ? 4 : pt[i].cate == 3 ? 4 : 1
#define PTB pt[i].cate == 1 ? 4 : pt[i].cate == 2 ? 0 : pt[i].cate == 3 ? 4 : 0
#define PTMAX pt[ptC].cate == 0 ? 200 : pt[ptC].cate == 1 ? 100 : pt[ptC].cate == 2 ? 50 : 10
#define PTRF pt[findArr[sel]].cate == 1 ? 1 : pt[findArr[sel]].cate == 2 ? 4 : pt[findArr[sel]].cate == 3 ? 0 : 4
#define PTGF pt[findArr[sel]].cate == 1 ? 0 : pt[findArr[sel]].cate == 2 ? 4 : pt[findArr[sel]].cate == 3 ? 4 : 1
#define PTBF pt[findArr[sel]].cate == 1 ? 4 : pt[findArr[sel]].cate == 2 ? 0 : pt[findArr[sel]].cate == 3 ? 4 : 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
DATE date;
TIME time;
PART pt[36];
PTLOG pLog[6];
MODE modeFlag = MAIN;

uint8_t reC = 0;
uint8_t lastDay[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
uint8_t sel;
uint8_t buzFlag = 0;
uint8_t ptC = 0;
uint8_t tempX = 0, tempY = 0;
uint8_t ledPos = 0;
uint8_t ptSetPosF = 0;
uint8_t ptInNum;
uint8_t logC=0;
uint8_t usePos = 255;
uint8_t findArr[36];
uint8_t findC = 0;
uint8_t selLog = 0;
uint16_t adc[2];
uint16_t ledC = 0;
boolean oldsw = true;
boolean udf = true, firstF = true;
volatile String ptFindName;
const String ptCate[4] = {"Res", "Cap", "IC", "Etc"};
const String ptWorkCate[4] = {"SAVE", "USE", "FIND", "MKPT"};
const String keyboard[4] = {
		"1234567890",
		"QWERTYUIOP",
		"ASDFGHJKL;",
		"ZXCVBNM,. "
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SSD1306_PutsXY(uint8_t x, uint8_t y, String str, uint8_t color) {
	SSD1306_GotoXY(x * 6, y * 8);
	SSD1306_Puts(str, &Font_6x8, color);
}

void basicScreen(){
	SSD1306_Fill(0);
	SSD1306_DrawFilledRectangle(0, 0, 127, 7, 1);
}

void swS(){
	oldsw = JOY_P ? true : false;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	static uint16_t buzC;
	if (htim->Instance == TIM2) {
		reC++;
		if(modeFlag == FINDR || modeFlag == PARTITION) ledC++;
		else ledC = 0;
		HAL_ADC_Start(&hadc);
		HAL_ADC_PollForConversion(&hadc, 10);
		adc[0] = HAL_ADC_GetValue(&hadc);
		HAL_ADC_PollForConversion(&hadc, 10);
		adc[1] = HAL_ADC_GetValue(&hadc);
		swS();
		if(buzFlag)buzC++;
		else {
			buzC = 0;
			BUZ(0);
		}
		if(buzFlag==1){
			if(buzC >= 500) buzFlag = 0;
			BUZ(1);
		}
		if(buzFlag == 2){
			if(buzC >= 100) buzFlag = 0;
			if(buzC % 50 > 25) BUZ(1);
			else BUZ(0);
		}
	}
}

void logShift(){
	logC++;
	for(uint8_t i = 0; i < 5; i++)
		pLog[5 - i] = pLog[4 - i];
}

void textIn(boolean n, volatile String resultArr ,uint8_t lim){
	String bf = (String)calloc(0, sizeof(char) * 11);
	uint8_t keyX = 0, keyY = 0, limX = 0, tempColor;
	uint8_t cur = 0;
	udf = true;
	memset(resultArr, 0, (size_t)lim + 1);

	SSD1306_Fill(0);

	while(1){
		if(RE1){
			reC = 0;
			if(JOY_R)
				if(keyX < limX) keyX++;
				else keyX = 0;
			if(JOY_L)
				if(keyX) keyX--;
				else keyX = limX;
			if(JOY_U)
				if(keyY) keyY--;
				else keyY = 3;
			if(JOY_D)
				if(keyY < 3) keyY++;
				else keyY = 0;
			if(JOY_R || JOY_L || JOY_U || JOY_D) udf = true;
		}
		if(JOY_P){
			if(!oldsw){
				swS();
				udf = true;
				if(keyX < 10) resultArr[cur++] = keyboard[keyY][keyX];
				else {
					if(keyY == 0) cur--;
					else if(keyY == 2){
						udf = true;
						resultArr[cur] = '\0';
						SSD1306_Fill(1);
						free(bf);
						return;
					}
				}
			}
		}
		if(lim - 1 == cur){
			firstF = true;
			udf = true;

			resultArr[cur]='\0';
			SSD1306_Fill(1);

			free(bf);
			return;
		}
		if(keyY == 0 || keyY == 2) limX = 10;
		else limX = 9;
		if(keyX > limX) keyX = limX;
		if(udf){
			udf = false;

			basicScreen();
			if(!n) SSD1306_PutsXY(0, 0, "#input Name", 0);
			else  SSD1306_PutsXY(0, 0, "#input part name", 0);

			for(uint8_t i = 0; i < 10; i++){
				for(uint8_t j = 0; j < 4; j++){
					tempColor = !(keyX == i && keyY == j);
					SSD1306_GotoXY(14 + (i * 10), 31 + (j * 8));
					SSD1306_Putc(keyboard[j][i], &Font_6x8, tempColor);
				}
			}
			SSD1306_GotoXY(115, 31);
			SSD1306_Puts("&", &Font_6x8, !(keyX == 10 && keyY == 0));
			SSD1306_GotoXY(115, 49);
			SSD1306_Puts("*", &Font_6x8, !(keyX == 10 && keyY == 2));
			SSD1306_PutsXY(cur, 2, " ^ ", 1);
			SSD1306_PutsXY(0, 1, " > ", 1);
			for(uint8_t i = 0; i < cur; i++){
				sprintf(bf, "%c ", resultArr[i]);
				SSD1306_PutsXY(1 + i, 1, bf, 1);
			}

			SSD1306_UpdateScreen();
		}
	}
}

void timeSet(boolean firstOn){
	DATE setDate = {.year = 2022, .month = 3, .day = 18};
	TIME setTime = {.hour = 0, .min = 0, .sec = 1};

	basicScreen();
	while(!firstOn){
		if(RE1){
			reC = 0;
			if(JOY_R && sel < 5) sel++;
			if(JOY_L && sel) sel--;
			if(JOY_U){
				switch(sel){
				case 0: if(setDate.year < 2099) setDate.year++;
				case 1: if(setDate.month < 12) setDate.month++;
				case 2: if(setDate.day < lastDay[setDate.month - 1]) setDate.day++;
				case 3: if(setTime.hour < 23) setTime.hour++;
				case 4: if(setTime.min < 59) setTime.min++;
				case 5: if(setTime.sec < 59) setTime.sec++;
				}
				if(setDate.day > lastDay[setDate.month - 1]) setDate.day = lastDay[setDate.month - 1];
			}
			if(JOY_D){
				switch(sel){
				case 0: if(setDate.year > 2000) setDate.year--;
				case 1: if(setDate.month > 1) setDate.month--;
				case 2: if(setDate.day > 1) setDate.day--;
				case 3: if(setTime.hour) setTime.hour--;
				case 4: if(setTime.min) setTime.min--;
				case 5: if(setTime.sec) setTime.sec--;
				}
			}
			if(JOY_R || JOY_L || JOY_U || JOY_D) udf = true;
		}
		if(JOY_P){
			if(!oldsw){
				swS();
				DS3231_set_date(setDate.day, setDate.month, setDate.year);
				DS3231_set_time(setTime.sec, setTime.min, setTime.hour);
				udf = true;
				eepWriteData(0, true);
				SSD1306_Clear();
				return;
			}
		}
		if(udf){
			String bf = (String)calloc(0, sizeof(char) * 12);
			udf = false;
			basicScreen();
			SSD1306_PutsXY(0, 0, "#Time Set", 0);
			SSD1306_PutsXY(0, 2, "RTC Time setting.", 1);
			switch(sel){
			case 0: sprintf(bf, "Year=%04ld", setDate.year); break;
			case 1: sprintf(bf, "Month=%02ld", setDate.month); break;
			case 2: sprintf(bf, "Day=%02ld", setDate.day); break;
			case 3: sprintf(bf, "Hour=%02ld", setTime.hour); break;
			case 4: sprintf(bf, "Min=%02ld", setTime.min); break;
			case 5: sprintf(bf, "Sec=%02ld", setTime.sec); break;
			}
			SSD1306_PutsXY(0, 4, bf, 1);
			SSD1306_UpdateScreen();
			free(bf);
		}
	}
}

MODE mainM(){
	if(firstF){
		firstF = false;
		basicScreen();
		for(uint8_t i = 0; i < 36; i++)
			if(pt[i].store) led_color(pt[i].pos, PTR, PTG, PTB);
		led_update();
		sel = 0;
	}
	if(RE1){
		if(JOY_U && sel)sel--;
		if(JOY_D && sel < 4) sel++;
		if(JOY_U || JOY_D || JOY_R || JOY_L) udf = true;

		DS3231_get_date(&date.day, &date.month, (uint8_t *)&date.year);
		DS3231_get_time(&time.sec, &time.min, &time.hour);

		String bf = (String)calloc(0, sizeof(char) * 16);
		sprintf(bf, "%04d.%02d.%02d", date.year + 1992, date.month, date.day);
		SSD1306_PutsXY(11, 3, bf, 1);
		sprintf(bf, "%02d:%02d:%02d", time.hour, time.min, time.sec);
		SSD1306_PutsXY(13, 4, bf, 1);
		SSD1306_UpdateScreen();
		free(bf);
	}
	if(JOY_P){
		if(!oldsw){
			swS();
			if(ptC || !sel){
				udf = true;
				firstF = true;
				return sel == 0 ? PSAVE : sel == 1 ? PUSE : sel == 2 ? PFIND : sel == 3 ? PARTITION : PLOG;
			}else buzFlag = 1;
		}
	}
	if(udf){
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Menu", 0);
		SSD1306_PutsXY(1, 2, "Part save", 1);
		SSD1306_PutsXY(1, 3, "Part use", 1);
		SSD1306_PutsXY(1, 4, "Part find", 1);
		SSD1306_PutsXY(1, 5, "Partition", 1);
		SSD1306_PutsXY(1, 6, "Part log", 1);

		SSD1306_PutsXY(0, sel + 2, ">", 1);
	}
	return MAIN;
}

MODE partS(){
	if(firstF){
		firstF = false;

		sel = 0;
		tempX = 1;
		tempY = 1;
		ledPos = 30;

		ptInNum = 1;
		pt[ptC].cate = 0;
		pt[ptC].max = 200;

		led_clear();
		led_update();
	}
	if(RE1){
		uint8_t i;
		reC = 0;
		if(ptSetPosF){
			if(JOY_R)
				if(tempX < 6)
					for(i = 1; i <= (6 - tempX); i++)
						if(led_cmp((ledPos + i), 0, 4, 0) == 3){
							tempX += i;
							break;
						}
			if(JOY_L)
				if(tempX > 1)
					for(i = 1; i < tempX; i++)
						if(led_cmp((ledPos - i), 0, 4, 0) == 3){
							tempX -= i;
							break;
						}
			if(JOY_U)
				if(tempY < 6)
					for(i = 1; i <= (6 - tempY); i++)
						if(led_cmp(ledPos - (6 * i), 0, 4, 0) == 3){
							tempY += i;
							break;
						}

			if(JOY_D)
				if(tempY > 1)
					for(i = 1; i <=  tempY; i++)
						if(led_cmp(ledPos +(6 * i), 0, 4, 0) == 3){
							tempY -= i;
							break;
						}
		}else{
			if(JOY_R){
				if(sel == 0){
					if(pt[ptC].cate < 3) pt[ptC].cate++;
					pt[ptC].max = PTMAX;
					if(ptInNum > pt[ptC].max) ptInNum = pt[ptC].max;
				}
				if(sel == 2 && ptInNum < pt[ptC].max - pt[ptC].store) ptInNum++;
				if(sel == 3 && pt[ptC].pos < 35) pt[ptC].pos++;
			}
			if(JOY_L){
				if(sel == 0){
					if(pt[ptC].cate) pt[ptC].cate--;
					pt[ptC].max = PTMAX;
					if(ptInNum > pt[ptC].max) ptInNum = pt[ptC].max;
				}
				if(sel == 2 && ptInNum > 1) ptInNum--;
				if(sel == 3 && pt[ptC].pos) pt[ptC].pos--;
			}
			if(JOY_U && sel) sel--;
			if(JOY_D && sel < 4) sel++;
		}
		if(JOY_R || JOY_L || JOY_U || JOY_D) udf = true;
	}
	if(JOY_P){
		if(!oldsw){
			swS();
			if(sel == 1){
				udf = true;
				pt[ptC].name = (String)calloc(0, sizeof(char) * 11);
				textIn(false, pt[ptC].name, 10);
			}else if(sel == 3){
				udf = true;
				ptSetPosF ^= 1;
				if(!ptSetPosF) led_clear();
			}else {
				if(!pt[ptC].name) buzFlag = 2;
				else if(sel == 4){
					udf = true;
					firstF = 1;

					pt[ptC].pos = ledPos;
					pt[ptC].store = ptInNum;

					logShift();
					DS3231_get_date(&date.day, &date.month, (uint8_t *)&date.year);
					DS3231_get_time(&time.sec, &time.min, &time.hour);
					pLog[0].workCate = 1;
					pLog[0].date = date;
					pLog[0].time = time;
					sprintf(pLog[0].content[0], "%s/%s", pt[ptC].name, ptCate[pt[ptC].cate]);
					sprintf(pLog[0].content[1], "%dpcs (%d,%d)", pt[ptC].store, tempX, tempY);
					ptC++;

					sel = 0;
					return MAIN;
				}
			}
		}
	}
	if(udf){
		String bf = (String)calloc(0, sizeof(char) * 11);
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Save", 0);
		SSD1306_PutsXY(0, 1, "Pls input inFormation", 1);

		SSD1306_PutsXY(1, 3, "Cate:", 1);
		SSD1306_Puts(ptCate[pt[ptC].cate], &Font_6x8, 1);

		SSD1306_PutsXY(1, 4, "Name:", 1);
		if(!pt[ptC].name[0])SSD1306_Puts("(NONE)", &Font_6x8, 1);
		else SSD1306_Puts(pt[ptC].name, &Font_6x8, 1);

		if(pt[ptC].ptionC < 1) pt[ptC].ptionC = 1;
		SSD1306_PutsXY(1, 5, "Store:", 1);
		sprintf(bf, "%d/%d", ptInNum, pt[ptC].max * pt[ptC].ptionC);
		SSD1306_Puts(bf, &Font_6x8, 1);

		if(ptSetPosF){
			led_clear();
			for(uint8_t i = 0; i < 36; i++){
				if(pt[i].store) led_color(pt[i].pos, 4, 0, 0);
				if(led_cmp(i, 0, 0, 0) == 3) led_color(i, 0, 4, 0);
			}
			ledPos = (6 - tempY) * 6 + tempX - 1;
			led_color(ledPos, 4, 4, 4);

			led_update();
		}

		SSD1306_PutsXY(1, 6, "Position ", !ptSetPosF);
		sprintf(bf, "(%d, %d)", tempX, tempY);
		SSD1306_Puts(bf, &Font_6x8, !ptSetPosF);

		SSD1306_PutsXY(1, 7, "Enter", 1);

		SSD1306_PutsXY(0, sel + 3, ">", 1);

		free(bf);
		SSD1306_UpdateScreen();
	}
	return PSAVE;
}

MODE pUseM(){
	if(firstF){
		firstF = false;
		tempX = 1;
		tempY = 1;
	}
	if(RE1){
		reC = 0;
		if(JOY_R && tempX < 6) tempX++;
		if(JOY_L && tempX > 1) tempX--;
		if(JOY_U && tempY < 6) tempY++;
		if(JOY_D && tempY > 1) tempY--;
		if(JOY_R || JOY_L || JOY_U || JOY_D) udf = true;
	}
	if(JOY_P){
		if(!oldsw){
			swS();
			if(usePos != 255){
				firstF = true;
				udf = true;
				return USE;
			}else buzFlag = 2;
		}
	}
	if(udf){
		udf = false;
		basicScreen();
		ledPos = (6 - tempY) * 6 + tempX - 1;

		SSD1306_PutsXY(0, 0, "#Use", 0);
		SSD1306_PutsXY(0, 1, "Select part", 1);

		for(uint8_t i = 0; i < 36; i++){
			if(pt[i].pos == ledPos && pt[i].store){
				String bf = (String)calloc(0, sizeof(char) * 4);
				usePos = i;
				SSD1306_PutsXY(1, 3, "Cate:", 1);
				SSD1306_Puts(ptCate[pt[usePos].cate], &Font_6x8, 1);
				SSD1306_PutsXY(1, 4, "Name:", 1);
				SSD1306_Puts(pt[usePos].name, &Font_6x8, 1);
				SSD1306_PutsXY(1, 5, "Store:", 1);
				sprintf(bf, "%d ", pt[usePos].store);
				SSD1306_Puts(bf, &Font_6x8, 1);
				free(bf);
				break;
			}else{
				usePos = 255;
				SSD1306_PutsXY(1, 3, "(Empty)", 1);
			}
		}
		led_clear();
		for(uint8_t i = 0; i < 36; i++){
			if(pt[i].store) led_color(pt[i].pos, PTR, PTG, PTB);
			if(!led_cmp(i, 0, 0, 0)) led_color(i, 0, 4, 0);
		}
		led_color(ledPos, 4, 4, 4);

		led_update();
		SSD1306_UpdateScreen();
	}

	return PUSE;
}

MODE use(){
	if(firstF){
		firstF = false;
		ptInNum = 0;
	}
	if(RE1){
		reC = 0;
		if(JOY_U && ptInNum < pt[usePos].store) ptInNum++;
		if(JOY_D && ptInNum > 1) ptInNum--;
		if(JOY_U || JOY_D) udf = 1;
	}
	if(JOY_P){
		if(!oldsw){
			swS();
			firstF = 1;
			udf = 1;

			logShift();
			DS3231_get_date(&date.day, &date.month, (uint8_t*)&date.year);
			DS3231_get_time(&time.sec, &time.min, &time.hour);
			pLog[0].workCate = 2;
			pLog[0].date = date;
			pLog[0].time = time ;
			sprintf(pLog[0].content[0], "%s/%s", pt[usePos].name, ptCate[pt[usePos].cate]);
			sprintf(pLog[0].content[1], "%dpcs (%d,%d)", ptInNum, tempX, tempY);
			if(ptInNum == pt[usePos].store) return REFILL;
			else {
				pt[usePos].store -= ptInNum;
				return MAIN;
			}
		}
	}
	if(udf){
		String bf = (String)calloc(0, sizeof(char) * 12);
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Use", 0);
		SSD1306_PutsXY(0, 1, "How Many use?", 1);
		sprintf(bf, "(%d / %d)", ptInNum, pt[usePos].store);
		SSD1306_PutsXY(0, 3, bf, 1);
		SSD1306_UpdateScreen();
		free(bf);
	}
	return USE;
}

MODE refill(){
	if(firstF){
		firstF = false;
		ptInNum = 0;
	}
	if(RE1){
		reC = 0;
		if(JOY_U && ptInNum < pt[usePos].max){
			ptInNum++;
			udf = true;
		}
		if(JOY_D && ptInNum){
			ptInNum--;
			udf = true;
		}
	}
	if(JOY_P){
		if(!oldsw){
			swS();
			firstF = true;
			udf = true;
			if(ptInNum){
				logShift();
				DS3231_get_date(&date.day, &date.month, (uint8_t*)&date.year);
				DS3231_get_time(&time.sec, &time.min, &time.hour);
				pLog[0].workCate = 1;
				pLog[0].date = date;
				pLog[0].time = time;
				sprintf(pLog[0].content[0], "%s/%s", pt[usePos].name, ptCate[pt[usePos].cate]);
				sprintf(pLog[0].content[1], "%dpcs (%d,%d)", ptInNum, tempX, tempY);
			}
			pt[usePos].store = ptInNum;
			for(uint8_t i = 0; i < 36; i++){
				if(pt[usePos].ptionID == pt[i].ptionID && pt[usePos].ptionID) pt[i] = pt[usePos];
			}
			led_clear();

			return MAIN;
		}
	}
	if(udf){
		String bf = (String)calloc(0, sizeof(char) * 11);
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#refill", 0);
		SSD1306_PutsXY(0, 1, "Part has all used.", 1);
		if(pt[usePos].ptionC < 1)pt[usePos].ptionC = 1;
		sprintf(bf, "IN:%u/%u", ptInNum, pt[usePos].max * pt[usePos].ptionC);
		SSD1306_PutsXY(0, 3, bf, 1);
		SSD1306_UpdateScreen();
		free(bf);
	}
	return REFILL;
}

MODE pFind(){
	if(firstF){
		firstF = false;
		findC = 0;
		memset(ptFindName, 0, sizeof(char) * 11);
	}
	if(RE1 && (JOY_R || JOY_L || JOY_U || JOY_D)) udf = true;
	if(JOY_P){
		if(!oldsw){
			swS();
 			if(!ptFindName[0])textIn(true, ptFindName, 10);
			else{
				for(uint8_t i = 0; i < 36; i++)
					if(!strcmp(ptFindName, pt[i].name)){
						findArr[findC] = i;
						findC++;
					}
				firstF = true;
				udf = true;
				return FINDR;
			}
		}
	}
	if(udf){
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Find", 0);
		SSD1306_DrawFilledRectangle(18, 34, 92, 10, 1);
		SSD1306_GotoXY(19, 36);
		if(!ptFindName[0]) SSD1306_Puts("input find name", &Font_6x8, 1);
		else SSD1306_Puts(ptFindName, &Font_6x8, 1);
		SSD1306_UpdateScreen();
	}
	return PFIND;
}

MODE findR(){
	if(firstF){
		firstF = false;
		sel = 0;
	}
	if(ledC >= 1000) ledC = 0;
	if(ledC < 500)
		for(uint8_t i = 0; i < 36; i++)
			led_color(pt[findArr[i]].pos, PTRF, PTGF, PTBF);
	else led_color(pt[findArr[sel]].pos, 0, 0, 0);
	if(reC >= 250){
		reC = 0;
		led_update();
		if(JOY_U && sel)sel--;
		if(JOY_D && sel < (findC - 1)) sel++;
		if(JOY_U || JOY_D) udf = true;
	}
	if(JOY_P){
		if(!oldsw){
			udf = true;
			swS();
			logShift();
			DS3231_get_date(&date.day, &date.month, (uint8_t *)&date.year);
			DS3231_get_time(&time.sec, &time.min, &time.hour);
			pLog[0].workCate = 3;
			pLog[0].date = date;
			pLog[0].time = time;
			sprintf(pLog[0].content[0], "Sear:%s", ptFindName);
			sprintf(pLog[0].content[0], "Num of Find:%u", findC);
			if(findC){
				usePos = findArr[sel];
				return USE;
			}else {
				udf = true;
				return MAIN;
			}
		}
		findC = 0;
		udf = true;
		firstF = true;
	}
	if(udf){
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Find result", 0);
		if(findC){
			String bf = (String)calloc(0, sizeof(char) * 31);
			SSD1306_PutsXY(0, sel + 2, ">", 1);
			SSD1306_PutsXY(0, 1, "Find some Parts!", 1);
			for(uint8_t i = 0; i < findC; i++){
				tempX = (pt[findArr[i]].pos) % 6 + 1;
				tempY = 6 - (pt[findArr[i]].pos) / 6;
				sprintf(bf, "%s(%s/%d,%d)", pt[findArr[i]].name, ptCate[pt[findArr[i]].cate], tempX, tempY);
				SSD1306_PutsXY(1, i + 2, bf, 1);
			}
			free(bf);
		}else SSD1306_PutsXY(0, 1, "Not Found..", 1);
		SSD1306_UpdateScreen();
	}
	return FINDR;
}

MODE partition(){
	static uint8_t minX = 0, minY = 0;
	static uint8_t maxX = 0, maxY = 0;
	static uint8_t ptiSX = 1, ptiEX = 1;
	static uint8_t ptiSY = 1, ptiEY = 1;
	static uint8_t selPtpos = 0;
	static uint8_t tempPtiC= 0;
	static uint8_t ptiC = 0;
	if(firstF){
		firstF = false;
		sel = 0;
		ptiSX = 1;
		ptiSY = 1;
		ptiEX = 1;
		ptiEY = 1;
		selPtpos = 0;
		tempPtiC = 0;
		led_clear();
		led_update();
	}
	if(RE1){
		reC = 0;
		led_clear();
		if(JOY_R){
			if(ptiSX < 6 && sel == 0) ptiSX++;
			if(ptiEX < 6 && sel == 1) ptiEX++;
		}
		if(JOY_L){
			if(ptiSX > 1 && sel == 0) ptiSX--;
			if(ptiEX > 1 && sel == 1) ptiEX--;
		}
		if(JOY_U){
			if(ptiSY < 6 && sel == 0) ptiSY++;
			if(ptiEY < 6 && sel == 1) ptiEY++;
		}
		if(JOY_D){
			if(ptiSY > 1 && sel == 0) ptiSY--;
			if(ptiEY > 1 && sel == 1) ptiEY--;
		}
		if(JOY_U || JOY_D || JOY_R || JOY_L) udf = true;
		if(sel == 0) ledPos = (6 - ptiSY) * 6 + ptiSX - 1;
		if(sel == 1) ledPos = (6 - ptiEY) * 6 + ptiEX - 1;
		if(sel <= 1){
			for(uint8_t i = 0; i < 36; i++) if(!pt[i].cate) led_color(pt[i].pos, PTR, PTG, PTB);
			led_color(ledPos, 4, 4, 4);
			led_update();
		}
		if(sel == 2){
			if(ledC >= 1000) ledC = 0;
			if(ledC < 500){
				for(uint8_t i = 0; i < 36; i++){
					tempX = i % 6 + 1;
					tempY = 6 - i / 6;
					if((tempX >= minX && tempX <= maxX) && (tempY >= minY && tempY <= maxY))
						led_color(i, 0, 4, 0);
				}
			}else led_clear();
			led_update();
		}
	}
	if(JOY_P){
		if(!oldsw){
			swS();
			udf = true;
			if(sel < 3) sel++;
			if(sel == 1){
				ptiEX = ptiSX;
				ptiEY = ptiSY;
			}
			if(sel == 2){
				maxX = ptiSX >= ptiEX ? ptiSX : ptiEX;
				maxY = ptiSY >= ptiEY ? ptiSY : ptiEY;
				minX = ptiSX >= ptiEX ? ptiEX : ptiSX;
				minY = ptiSY >= ptiEY ? ptiEY : ptiSY;
				for(uint8_t i = 0; i < 36; i++){
					tempX = i % 6 + 1;
					tempY = 6 - i / 6;
					if(tempX >= minX && tempX <= maxX)
						if(tempY >= minY && tempY <= maxY)
							tempPtiC++;
				}
			}
			if(sel == 3){
				ptiC++;
				pt[selPtpos].ptionID = ptiC;
				pt[selPtpos].ptionC = tempPtiC;
				for(uint8_t i = 0; i < 36; i++){
					tempX = i % 6 + 1;
					tempY = 6 - i / 6;
					if(tempX >= minX && tempX <= maxX){
						if(tempY >= minY && tempY <= maxY){
							if(!pt[ptC].store){
								pt[ptC] = pt[selPtpos];
								pt[ptC++].pos = i;
							}
						}
					}
				}
				logShift();
				DS3231_get_date(&date.day, &date.month, (uint8_t*)&date.year);
				DS3231_get_time(&time.sec, &time.min, &time.hour);
				sprintf(pLog[0].content[0], "S(%d,%d) E(%d,%d)", ptiSX, ptiSY, ptiEX, ptiEY);
				sprintf(pLog[0].content[1], "Size of ption:%d", tempPtiC);
				pLog[0].workCate = 4;
				pLog[0].time = time;
				pLog[0].date = date;
				sel = 0;
				firstF = true;
				udf = true;
				return MAIN;
			}
		}
	}
	if(udf){
		String bf = (String)calloc(0, sizeof(char) * 30);
		udf = false;
		led_update();
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Partition", 0);
		sprintf(bf, "Start (%d,%d)", ptiSX, ptiSY);
		SSD1306_PutsXY(0, 3, bf, 1);
		for(uint8_t i = 0; i < 36; i++){
			if(pt[i].store && pt[i].pos == ledPos){
				if(pt[i].ptionC < 1) pt[i].ptionC = 1;
				sprintf(bf, "%s(%s/%d/%d)", pt[i].name, ptCate[pt[i].cate], pt[i].store, pt[i].max * pt[i].ptionC);
				SSD1306_PutsXY(0, 4, bf, 1);
			}else {
				SSD1306_PutsXY(0, 4, "(NONE)", 1);
			}
		}
		if(sel == 1){
			SSD1306_PutsXY(0, 1, "Select end point", 1);
			sprintf(bf, "End (%d,%d)", ptiEX, ptiEY);
			SSD1306_PutsXY(0, 3, bf, 1);
			for(uint8_t i = 0; i < 36; i++){
				if(pt[i].store && pt[i].pos == ledPos){
					sprintf(bf, "%s(%s/%d/%d)", pt[i].name, ptCate[pt[i].cate], ptiEX, ptiEY);
					SSD1306_PutsXY(0, 4, bf, 1);
				}else {
					SSD1306_PutsXY(0, 4, "(NONE)", 1);
				}
			}
		}
		if(sel == 2){
			sprintf(bf, "Total number:%d", tempPtiC);
			SSD1306_PutsXY(0, 3, bf, 1);
		}
		SSD1306_UpdateScreen();
		free(bf);
	}
	return PARTITION;
}

MODE logM(){
	if(firstF){
		firstF = false;
		sel = 0;
	}
	if(RE1){
		reC = 0;
		if(JOY_L){
			firstF = true;
			udf = true;
			return MAIN;
		}
		if(JOY_U && sel) sel--;
		if(JOY_D && (sel < ((logC > 6) ? 5 : logC - 1))) sel++;
		if(JOY_U || JOY_D) udf = true;
	}
	if(JOY_P){
		if(!oldsw){
			swS();
			firstF = true;
			udf = true;
			selLog = sel;
			return LOGD;
		}
	}
	if(udf){
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Log", 0);
		for(uint8_t i = 0; i < ((logC > 6) ? 6 : logC); i++){
			if(pLog[i].date.year){
				String bf = (String)calloc(0, sizeof(char) * 30);
				sprintf(bf, "%02d.%02d.%02d/%02d:%02d/%s", pLog[i].date.year - 1800, pLog[i].date.month, pLog[i].date.day, pLog[i].time.hour, pLog[i].time.min, ptWorkCate[pLog[i].workCate - 1]);
				SSD1306_PutsXY(1, i + 2, bf, 1);
				free(bf);
			}else break;
		}
		SSD1306_PutsXY(0, sel + 2, ">", 1);
		SSD1306_UpdateScreen();
	}
	return PLOG;
}

MODE logD(){
	if(firstF) firstF = false;
	if(JOY_P){
		if(!oldsw){
			swS();
			udf = true;
			return PLOG;
		}
	}
	if(udf){
		String bf = (String)calloc(0, sizeof(char) * 30);
		udf = false;
		basicScreen();
		SSD1306_PutsXY(0, 0, "#Log detail", 0);
		sprintf(bf, "%02d.%02d.%02d/%02d:%02d/%s", pLog[selLog].date.year  - 1800, pLog[selLog].date.month, pLog[selLog].date.day, pLog[selLog].time.hour, pLog[selLog].time.min, ptWorkCate[pLog[selLog].workCate - 1]);
		SSD1306_PutsXY(0, 2, bf, 1);
		SSD1306_PutsXY(0, 4, pLog[selLog].content[0], 1);
		SSD1306_PutsXY(0, 5, pLog[selLog].content[1], 1);
		free(bf);
		SSD1306_UpdateScreen();
	}
	return LOGD;
}

typedef struct func{
	MODE (**mode)(void);
}FUNC, *PFUNC;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void){
  /* USER CODE BEGIN 1 */
	PFUNC func = (PFUNC)malloc(sizeof(FUNC));
	func->mode = (MODE**)malloc(sizeof(MODE*) * 10);

	*(func->mode + 0) = mainM;
	*(func->mode + 1) = partS;
	*(func->mode + 2) = pUseM;
	*(func->mode + 3) = use;
	*(func->mode + 4) = refill;
	*(func->mode + 5) = pFind;
	*(func->mode + 6) = findR;
	*(func->mode + 7) = partition;
	*(func->mode + 8) = logM;
	*(func->mode + 9) = logD;

	static boolean firstOn  = false;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
	SSD1306_Init();
	HAL_TIM_Base_Start_IT(&htim2);

	ptFindName = (String)calloc(0, sizeof(char) * 11);
	memset(pLog, 0, (size_t)sizeof(pLog));
	for (uint8_t i = 0; i < 36; i++)
		led_color(i, i / 6 == 0 || i / 6 == 3 ? 4 : 0, i / 6 == 1 || i / 6 == 4 ? 4 : 0, i / 6 == 2 || i / 6 == 5 ? 4 : 0);
	led_update();
	for (uint8_t i = 0; i < 8; i++) {
		SSD1306_Fill(0);
		SSD1306_GotoXY(46, 28 + i);
		SSD1306_Puts("Drawer", &Font_6x8, 0);
		SSD1306_GotoXY(43, 28);
		SSD1306_Puts(" Parts ", &Font_6x8, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(200);
	}
	HAL_Delay(1000);

	memset(pt, 0, (size_t)sizeof(pt));
	SSD1306_Clear();
	led_clear();
	UPDATE;
//	eepWriteData(0, false);
	firstOn = eepReadData(0);
	timeSet(firstOn);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		modeFlag = (*(func->mode + modeFlag))();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
	free(func->mode);
	free(func);
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_3;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* TIM2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00200C28;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 32-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000-1;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZ_GPIO_Port, BUZ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RGB_DATA_GPIO_Port, RGB_DATA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : JOY_SW_Pin */
  GPIO_InitStruct.Pin = JOY_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(JOY_SW_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZ_Pin */
  GPIO_InitStruct.Pin = BUZ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RGB_DATA_Pin */
  GPIO_InitStruct.Pin = RGB_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RGB_DATA_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
