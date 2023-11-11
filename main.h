#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>

#define DDAY_EPOCH 946771200
#define YEAR_OFFSET 2000

void fnd_init(void);
void fnd_display(void);
void fnd_reset(void);

enum {
    FND_OFF = 10,
    FND_ON = 11
};

typedef uint32_t time_t;

time_t dday_s = DDAY_EPOCH;
uint8_t buf[9] = { 0, };
uint8_t const FND_LUT[] = {
    [0]=0b11111100,
    [1]=0b01100000,
    [2]=0b11011010,
    [3]=0b11110010,
    [4]=0b01100110,
    [5]=0b10110110,
    [6]=0b10111110,
    [7]=0b11100000,
    [8]=0b11111110,
    [9]=0b11110110,
    [FND_OFF]=0b00000000,
    [FND_ON]=0b11111111,
};

void buffer_counter(void);
void buffer_ymd(void);
void buffer_hms(void);

void up_year(void);
void up_month(void);
void up_day(void);
void up_hour(void);
void up_minute(void);
void up_second(void);

struct button_press_info_t {
    bool next;
    bool up;
};

struct blink_info_t {
    uint8_t start;
    uint8_t streak;
};

void ctl_init(void);

#define DS1337_SLA 0b1101000

struct rtc_t {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month_century;
    uint8_t year;
};

void rtc_init(void);
void rtc_clock_enable(void);
void rtc_clock_disable(void);
void rtc_write_buffer(void);
void rtc_read_buffer(void);
void rtc_read_ymd(uint8_t *y, uint8_t *m, uint8_t *d);
void rtc_read_hms(uint8_t *h, uint8_t *m, uint8_t *s);
time_t rtc_mktime(struct rtc_t const *rtc);
time_t rtc_difftime(time_t lhs, time_t rhs);

#endif
