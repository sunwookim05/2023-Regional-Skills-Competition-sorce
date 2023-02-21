/*
 * DS3231.c
 *
 *  Created on: 2020. 5. 5.
 *      Author: bbb
 */

#include "DS3231.h"

extern I2C_HandleTypeDef hi2c1;

void DS3231_set_time(uint8_t sec, uint8_t minute, uint8_t hour)
{
  uint8_t buffer[4];

  buffer[0] = SECONDS_AD;
  buffer[1] = ((sec/10)<<4) + (sec%10);
  buffer[2] = ((minute/10)<<4) + (minute%10);
  buffer[3] = (((hour/10)&0x03)<<4) + (hour%10);

  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x00, &buffer[0], 4, 100);
}

void DS3231_get_time(uint8_t *get_second, uint8_t *get_minute, uint8_t *get_hour)
{
  uint8_t buffer;

  buffer = SECONDS_AD;
  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x01, &buffer, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, DS3231_AD | 0x01, get_second, 1, 100);

  buffer = MINUTES_AD;
  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x01, &buffer, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, DS3231_AD | 0x01, get_minute, 1, 100);

  buffer = HOURS_AD;
  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x01, &buffer, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, DS3231_AD | 0x01, get_hour, 1, 100);

  *get_hour = (((*get_hour & 0x30) >> 4) * 10) + (*get_hour & 0x0f);
  *get_minute = ((*get_minute >> 4) * 10) + (*get_minute & 0x0f);
  *get_second = ((*get_second >> 4) * 10) + (*get_second & 0x0f);
}

void DS3231_set_date(uint8_t day, uint8_t month, uint8_t year)
{
  uint8_t buffer[4];

  buffer[0] = DATE_AD;
  buffer[1] = ((day/10)<<4) + (day%10);
  buffer[2] = ((month/10)<<4) + (month%10);
  buffer[3] = (((year/10)&0x03)<<4) + (year%10);

  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x00, &buffer[0], 4, 100);
}

void DS3231_get_date(uint8_t *get_day, uint8_t *get_month, uint8_t *get_year)
{
  uint8_t buffer;

  buffer = DATE_AD;
  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x01, &buffer, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, DS3231_AD | 0x01, get_day, 1, 100);

  buffer = MONTH_AD;
  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x01, &buffer, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, DS3231_AD | 0x01, get_month, 1, 100);

  buffer = YEAR_AD;
  HAL_I2C_Master_Transmit(&hi2c1, DS3231_AD | 0x01, &buffer, 1, 100);
  HAL_I2C_Master_Receive(&hi2c1, DS3231_AD | 0x01, get_year, 1, 100);

  *get_day = ((*get_day >> 4) * 10) + (*get_day & 0x0f);
  *get_month = (((*get_month & 0x10) >> 4) * 10) + (*get_month & 0x0f);
  *get_year = ((*get_year >> 4) * 10) + (*get_year & 0x0f);
}
