#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <util/twi.h>

#define DDAY_EPOCH 946771200
#define YEAR_OFFSET 1900
#define MONTH_OFFSET 1

void twi_init(void);
bool twi_start(void);
bool twi_send_address(uint8_t address, bool tw_rw);
bool twi_write_data(uint8_t data);
bool twi_repeated_start(void);
bool twi_read_data(uint8_t *data, bool ack);
bool twi_stop(void);

void fnd_init(void);
void fnd_display(void);
void fnd_reset(void);

enum {
    FND_OFF = 10,
    FND_ON = 11
};

time_t dday_s = DDAY_EPOCH;
time_t rtc_s = 0;
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

void time_init(void);
void buffer_counter(void);
void buffer_ymd(void);
void buffer_hms(void);

void up_year(void);
void up_month(void);
void up_day(void);
void up_hour(void);
void up_minute(void);
void up_second(void);

bool is_U_pressed = false;
bool is_N_pressed = false;

void ctl_init(void);
void stop_counter(void);
void resume_counter(void);

#endif
