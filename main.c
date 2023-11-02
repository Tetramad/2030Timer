#include "main.h"
#include "inline_utility.h"
#include "inline_twi.h"

struct rtc_t rtc = {
    0b0000'0000,
    0b0000'0000,
    0b0000'0000,
    0b0000'0001,
    0b0000'0001,
    0b1000'0001,
    0b0000'0000
};

#define ceilof(big, small) \
    (sizeof(big) + sizeof(small) - 1) / sizeof(small)

int main(void) {
    fnd_init();
    ctl_init();
    rtc_init();
    sei();

    if (bit_is_set(PINB, PINB6) && bit_is_set(PINB, PINB7)) {
        rtc_write_buffer();
    }

    rtc_read_buffer();

    enum {
        Counter,
        YMD_Y,
        YMD_M,
        YMD_D,
        HMS_H,
        HMS_M,
        HMS_S,
    } mode = Counter;

    void (*display)(void) = buffer_counter;
    void (*up)(void) = nullptr;
    uint8_t cnt = 0;

    for (;;) {
        _delay_ms(100);
        display();
        if (display == buffer_ymd || display == buffer_hms) {
            if (cnt > 2) {
                switch (mode) {
                    case Counter:
                        break;
                    case YMD_Y:
                        buf[1] = FND_OFF;
                        buf[2] = FND_OFF;
                        buf[3] = FND_OFF;
                        buf[4] = FND_OFF;
                        break;
                    case YMD_M:
                        buf[5] = FND_OFF;
                        buf[6] = FND_OFF;
                        break;
                    case YMD_D:
                        buf[7] = FND_OFF;
                        buf[8] = FND_OFF;
                        break;
                    case HMS_H:
                        buf[1] = FND_OFF;
                        buf[2] = FND_OFF;
                        break;
                    case HMS_M:
                        buf[4] = FND_OFF;
                        buf[5] = FND_OFF;
                        break;
                    case HMS_S:
                        buf[7] = FND_OFF;
                        buf[8] = FND_OFF;
                        break;
                }
            }
            ++cnt;
            if (cnt > 5) {
                cnt = 0;
            }
        }
        fnd_display();

        if (is_U_pressed) {
            if (up != nullptr) {
                up();
                cnt = 0;
                display();
                fnd_display();
            }
            is_U_pressed = false;
        }

        if (is_N_pressed) {
            switch (mode) {
                case Counter:
                    display = buffer_ymd;
                    up = up_year;
                    mode = YMD_Y;
                    rtc_clock_disable();
                    break;
                case YMD_Y:
                    up = up_month;
                    mode = YMD_M;
                    break;
                case YMD_M:
                    up = up_day;
                    mode = YMD_D;
                    break;
                case YMD_D:
                    display = buffer_hms;
                    up = up_hour;
                    mode = HMS_H;
                    break;
                case HMS_H:
                    up = up_minute;
                    mode = HMS_M;
                    break;
                case HMS_M:
                    up = up_second;
                    mode = HMS_S;
                    break;
                case HMS_S:
                    display = buffer_counter;
                    up = nullptr;
                    mode = Counter;
                    rtc_clock_enable();
                    break;
            }
            is_N_pressed = false;
        }
    }
}

void fnd_init(void) {
    /* GPIO settings */
    DDRF |= _BV(PORTF0) | _BV(PORTF4) | _BV(PORTF5) | _BV(PORTF6) | _BV(PORTF7);
    PORTF &= ~(_BV(PORTF0) | _BV(PORTF4) | _BV(PORTF5) | _BV(PORTF6) | _BV(PORTF7));

    /* Reset shift registers */
    PORTF |= _BV(PORTF5) | _BV(PORTF7);
}

void fnd_display(void) {
    for (int i = 8; i >= 0; --i) {
        uint8_t n = FND_LUT[buf[i]];
        for (int j = 0; j < 8; ++j) {
            if (n & 1) {
                PORTF &= ~_BV(PORTF0);
            } else {
                PORTF |= _BV(PORTF0);
            }
            PORTF |= _BV(PORTF4);
            PORTF &= ~_BV(PORTF4);
            n >>= 1;
        }
    }
    PORTF |= _BV(PORTF6);
    PORTF &= ~_BV(PORTF6);
}

void fnd_reset(void) {
    PORTF |= _BV(PORTF0);
    for (int i = 0; i < 9 * 8; ++i) {
        PORTF |= _BV(PORTF4);
        PORTF &= ~_BV(PORTF4);
    }
    PORTF |= _BV(PORTF6);
    PORTF &= ~_BV(PORTF6);
}

void buffer_counter(void) {
    rtc_read_buffer();
    time_t remain_s = rtc_difftime(dday_s, rtc_mktime(&rtc));

    for (int i = 8; i >= 0; --i) {
        buf[i] = remain_s % 10;
        remain_s /= 10;
    }
}

void buffer_ymd(void) {
    uint8_t date = 0;
    uint8_t month = 0;
    uint8_t year = 0;

    rtc_read_ymd(&year, &month, &date);

    buf[0] = FND_OFF;
    buf[1] = 2;
    buf[2] = 0;
    buf[3] = year >> 4;
    buf[4] = year & 0x0F;
    buf[5] = month >> 4;
    buf[6] = month & 0x0F;
    buf[7] = date >> 4;
    buf[8] = date & 0x0F;
}

void buffer_hms(void) {
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours = 0;

    rtc_read_hms(&hours, &minutes, &seconds);

    buf[0] = FND_OFF;
    buf[1] = hours >> 4;
    buf[2] = hours & 0x0F;
    buf[3] = FND_OFF;
    buf[4] = minutes >> 4;
    buf[5] = minutes & 0x0F;
    buf[6] = FND_OFF;
    buf[7] = seconds >> 4;
    buf[8] = seconds & 0x0F;
}

void up_year(void) {
    rtc_read_buffer();
    rtc.year = bcd_incwa(rtc.year, 0x29, 0x00);
    rtc_write_buffer();
}

void up_month(void) {
    rtc_read_buffer();
    rtc.month_century = bcd_incwa(rtc.month_century & 0b0001'1111, 0x12, 0x01) | 0b1000'0000;
    rtc_write_buffer();
}

void up_day(void) {
    uint8_t date = 0;
    uint8_t month = 0;
    uint8_t year = 0;
    rtc_read_ymd(&year, &month, &date);
    rtc.date = bcd_incwa(date, bcd_month_length(year, month), 0x01);
    rtc_write_buffer();
}

void up_hour(void) {
    rtc_read_buffer();
    rtc.hours = bcd_incwa(rtc.hours, 0x23, 0x00);
    rtc_write_buffer();
}

void up_minute(void) {
    rtc_read_buffer();
    rtc.minutes = bcd_incwa(rtc.minutes, 0x59, 0x00);
    rtc_write_buffer();
}

void up_second(void) {
    rtc_read_buffer();
    rtc.seconds = bcd_incwa(rtc.seconds, 0x59, 0x00);
    rtc_write_buffer();
}

void ctl_init(void) {
    /* GPIO settings */
    DDRB &= ~(_BV(PORTB6) | _BV(PORTB7));
    PORTB &= ~(_BV(PORTB6) | _BV(PORTB7));

    {
        uint8_t sreg = SREG;
        cli();

        /* set mask for enabling pin change interrupt for PCINT7 and PCINT6 */
        PCMSK0 |= _BV(PCINT7) | _BV(PCINT6);
        /* enable PCINT source 0 */
        PCICR = _BV(PCIE0);

        SREG = sreg;
    }
}

void rtc_init(void) {
    /* Set Bit Rate Generator Unit
    ** SCL frequency to about 76.923kHz */
    twi_bitrate(0b0110'0000);
    twi_prescaler(0x00);
    /* Enable TWI interface */
    twi_enable();
}

void rtc_clock_enable(void) {
    uint8_t sreg = SREG;
    cli();

    twi_start();
    twi_send_address(DS1337_SLA, TW_WRITE);
    twi_write_data(0x0E);
    twi_write_data(0b0000'0000);
    twi_stop();

    SREG = sreg;
}

void rtc_clock_disable(void) {
    uint8_t sreg = SREG;
    cli();

    twi_start();
    twi_send_address(DS1337_SLA, TW_WRITE);
    twi_write_data(0x0E);
    twi_write_data(0b1000'0000);
    twi_stop();

    SREG = sreg;
}

void rtc_write_buffer(void) {
    uint8_t sreg = SREG;
    cli();

    twi_start();
    twi_send_address(DS1337_SLA, TW_WRITE);
    twi_write_data(0x00);
    twi_write_data_burst(ceilof(struct rtc_t, uint8_t), (uint8_t *)&rtc);
    twi_stop();

    SREG = sreg;
}

void rtc_read_buffer(void) {
    uint8_t sreg = SREG;
    cli();

    twi_start();
    twi_send_address(DS1337_SLA, TW_WRITE);
    twi_write_data(0x00);
    twi_repeated_start();
    twi_send_address(DS1337_SLA, TW_READ);
    twi_read_data_burst(ceilof(struct rtc_t, uint8_t), (uint8_t *)&rtc);
    twi_stop();

    SREG = sreg;
}

void rtc_read_ymd(uint8_t *y, uint8_t *m, uint8_t *d) {
    uint8_t sreg = SREG;
    cli();
    twi_start();
    twi_send_address(DS1337_SLA, TW_WRITE);
    twi_write_data(0x04);
    twi_repeated_start();
    twi_send_address(DS1337_SLA, TW_READ);
    twi_read_data(d, true);
    twi_read_data(m, true);
    twi_read_data(y, false);
    twi_stop();
    SREG = sreg;

    *m &= 0b0001'1111;
}

void rtc_read_hms(uint8_t *h, uint8_t *m, uint8_t *s) {
    uint8_t sreg = SREG;
    cli();
    twi_start();
    twi_send_address(DS1337_SLA, TW_WRITE);
    twi_write_data(0x00);
    twi_repeated_start();
    twi_send_address(DS1337_SLA, TW_READ);
    twi_read_data(s, true);
    twi_read_data(m, true);
    twi_read_data(h, false);
    twi_stop();
    SREG = sreg;
}

time_t rtc_mktime(struct rtc_t const *rtc) {
    time_t t = 0;

    uint16_t year = (rtc->year >> 4) * 10 + (rtc->year & 0x0F);
    t += (365 * 4 + 1) * (year >> 2);
    switch (year & 0b11) {
        case 0b11:
            t += 365;
            [[fallthrough]];
        case 0b10:
            t += 365;
            [[fallthrough]];
        case 0b01:
            t += 365 + 1;
            [[fallthrough]];
        case 0b00:
            break;
    }

    uint8_t month = ((rtc->month_century & 0x10) ? 10 : 0) + (rtc->month_century & 0x0F);

    switch (month) {
        case 12:
            t += 30;
            [[fallthrough]];
        case 11:
            t += 31;
            [[fallthrough]];
        case 10:
            t += 30;
            [[fallthrough]];
        case 9:
            t += 31;
            [[fallthrough]];
        case 8:
            t += 31;
            [[fallthrough]];
        case 7:
            t += 30;
            [[fallthrough]];
        case 6:
            t += 31;
            [[fallthrough]];
        case 5:
            t += 30;
            [[fallthrough]];
        case 4:
            t += 31;
            [[fallthrough]];
        case 3:
            t += 28 + (bcd_is_leap_year(rtc->year) ? 1 : 0);
            [[fallthrough]];
        case 2:
            t += 31;
            [[fallthrough]];
        case 1:
            break;
    }

    uint8_t date = (rtc->date >> 4) * 10 + (rtc->date & 0x0F);
    t += date - 1;

    uint8_t hours = (rtc->hours >> 4) * 10 + (rtc->hours & 0x0F);
    t *= 24;
    t += hours;

    uint8_t minutes = (rtc->minutes >> 4) * 10 + (rtc->minutes & 0x0F);
    t *= 60;
    t += minutes;

    uint8_t seconds = (rtc->seconds >> 4) * 10 + (rtc->seconds & 0x0F);
    t *= 60;
    t += seconds;

    return t;
}

time_t rtc_difftime(time_t lhs, time_t rhs) {
    return lhs - rhs;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

ISR(PCINT0_vect) {
    if (bit_is_set(PINB, PINB6)) {
        is_N_pressed = true;
    }
    if (bit_is_set(PINB, PINB7)) {
        is_U_pressed = true;
    }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
