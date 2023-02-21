/**
 ******************************************************************************
 * File Name: stm32l0xx_EEPROM.h
 * Author   : Ju-seng Kim
 ******************************************************************************
 * stm32l0xx EEPROM Interface
 *
 ******************************************************************************
 */
#ifndef __stm32l0xx_EEP__
#define __stm32l0xx_EEP__

#include "stm32l0xx_hal.h"

void eepWriteData(uint16_t addr, uint8_t inData);
uint8_t eepReadData(uint16_t addr);

#endif
