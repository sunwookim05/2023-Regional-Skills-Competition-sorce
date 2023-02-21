/*
 * DS3231.h
 *
 *  Created on: 2020. 5. 5.
 *      Author: bbb
 */

#ifndef DS3231_H_
#define DS3231_H_

#include <main.h>

#define DS3231_AD   0xD0
#define SECONDS_AD    0x00
#define MINUTES_AD    0x01
#define HOURS_AD    0x02
#define DAY_AD      0x03
#define DATE_AD     0x04
#define MONTH_AD    0x05
#define YEAR_AD     0x06

void DS3231_set_time(uint8_t sec, uint8_t minute, uint8_t hour);
void DS3231_get_time(uint8_t *get_second, uint8_t *get_minute, uint8_t *get_hour);
void DS3231_set_date(uint8_t day, uint8_t month, uint8_t year);
void DS3231_get_date(uint8_t *get_day, uint8_t *get_month, uint8_t *get_year);
void DS3231_set_date_YEAR(uint8_t year);
void DS3231_get_date_YEAR(uint8_t *get_year);

#endif /* DS3231_H_ */
