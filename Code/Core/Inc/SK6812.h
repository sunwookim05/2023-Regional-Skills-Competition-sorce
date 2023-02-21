/*
 * SK6812.h
 *
 *  Created on: Mar 23, 2022
 *      Author: js372
 *
 *      [Notice]
 *      CPU Clock: 24MHz
 *      SPI Clock Prescaler: 8
 *      Please type "HAL_DMA_Init(&hdma_spi1_tx);" in User code2 Area
 */

#ifndef SRC_SK6812_H_
#define SRC_SK6812_H_

#include <stdint.h>
#include "main.h"
#include "stm32l0xx_hal_spi.h"

void led_color(uint8_t id, uint8_t r, uint8_t g, uint8_t b);
void led_update();
void led_clear();
uint8_t led_cmp(uint8_t id, uint8_t r, uint8_t g, uint8_t b);

#endif /* SRC_SK6812_H_ */
