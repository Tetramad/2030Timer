#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/twi.h>

#define DDAY_EPOCH 946771200
#define YEAR_OFFSET 2000
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

struct button_press_info_t {
    bool next;
    bool up;
};

struct blink_info_t {
    uint8_t start;
    uint8_t streak;
};

struct state_info_t {
    void (*buffer)(void);
    void (*up)(void);
    struct blink_info_t blink;
};

typedef uint32_t time_t;

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfixed-enum-extension"
#endif

enum state_t : uint8_t {
    Counter,
    YMD_Y,
    YMD_M,
    YMD_D,
    HMS_H,
    HMS_M,
    HMS_S,
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

enum {
    FND_OFF = 10,
    FND_ON = 11
};

void fnd_display(void);
void rtc_init(void);
void rtc_clock_enable(void);
void rtc_clock_disable(void);
void rtc_write_buffer(void);
void rtc_read_buffer(void);
time_t rtc_mktime(struct rtc_t const *rtc);
time_t rtc_difftime(time_t lhs, time_t rhs);
void buffer_counter(void);
void buffer_ymd(void);
void buffer_hms(void);
void up_year(void);
void up_month(void);
void up_day(void);
void up_hour(void);
void up_minute(void);
void up_second(void);
void up_second(void);

#endif
