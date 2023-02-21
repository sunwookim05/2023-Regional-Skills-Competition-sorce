/**
 ******************************************************************************
 * File Name: stm32l0xx_EEPROM.c
 * Author   : Ju-seng Kim
 ******************************************************************************
 * stm32l0xx EEPROM Interface
 *
 ******************************************************************************
 */
#include "stm32l0xx_EEPROM.h"
#include "stm32l052xx.h"

void eepWriteData(uint16_t addr, uint8_t inData)
{
	HAL_FLASHEx_DATAEEPROM_Unlock();
	HAL_FLASHEx_DATAEEPROM_Program(TYPEPROGRAMDATA_BYTE,(DATA_EEPROM_BASE+addr),inData);
	HAL_FLASHEx_DATAEEPROM_Lock();
}
uint8_t eepReadData(uint16_t addr)
{
	//return (*(__IO uint32_t *)(EEP_START_ADRESS+addr));
	return (*(__IO uint8_t*)(DATA_EEPROM_BASE+addr));
}
