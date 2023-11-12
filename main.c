#include "main.h"
#include "inline_utility.h"
#include "inline_twi.h"

int main(void) {
    /*
    ** Initializations
    */

    /* Cascaded shift registers for FNDs
    ** PF0 --- SER
    ** PF4 --- SRCLK
    ** PF5 --- /SRCLR
    ** PF6 --- RCLK
    ** PF7 --- /RCLR
    */
    /* IO register configurations */
    DDRF |= _BV(PORTF0) | _BV(PORTF4) | _BV(PORTF5) | _BV(PORTF6) | _BV(PORTF7);
    PORTF &= ~(_BV(PORTF0) | _BV(PORTF4) | _BV(PORTF5) | _BV(PORTF6) | _BV(PORTF7));
    /* Reset shift registers */
    PORTF |= _BV(PORTF5) | _BV(PORTF7);

    /* Buttons
    ** PB6 --- Button.Next
    ** PB7 --- Button.Up
    ** PCINT based inputs with software debouncing
    */
    /* IO register configurations */
    DDRB &= ~(_BV(PORTB6) | _BV(PORTB7));
    PORTB &= ~(_BV(PORTB6) | _BV(PORTB7));
    /* Timer1 stop clock */
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
    /* Timer1 clear */
    TCNT1 = 0x0000;
    /* Timer1 enable overflow interrupt */
    TIMSK1 |= _BV(TOIE1);
    /* set mask for enabling pin change interrupt for PCINT7 and PCINT6 */
    PCMSK0 |= _BV(PCINT7) | _BV(PCINT6);
    /* enable PCINT source 0 */
    PCICR = _BV(PCIE0);

    /* RTC
    ** communicate using I2C
    */
    rtc_init();

    /*
    ** End of initializations
    */

    sei();

    rtc_read_buffer();

    enum state_t mode = Counter;

    void (*buffer)(void) = buffer_counter;
    void (*up)(void) = nullptr;
    uint8_t cnt = 0;

    for (;;) {
        _delay_ms(100);

        /*
        ** FND displaying phase
        */
        /* buffering matches to state */
        buffer();
        /* check blinking timing */
        if (cnt > 2) {
            for (uint8_t i = 0; i < blink.streak; ++i) {
                buf[blink.start + i] = FND_OFF;
            }
        }
        ++cnt;
        if (cnt > 5) {
            cnt = 0;
        }
        /* buffer out to FNDs */
        fnd_display();

        /*
        ** Button input handling phase
        */

        if (pressed.up) {
            if (up != nullptr) {
                up();
                cnt = 0;
                buffer();
                fnd_display();
            }
            pressed.up = false;
        }

        if (pressed.next) {
            switch (mode) {
                case Counter:
                    buffer = buffer_ymd;
                    up = up_year;
                    mode = YMD_Y;
                    blink.start = 1;
                    blink.streak = 4;
                    rtc_clock_disable();
                    break;
                case YMD_Y:
                    up = up_month;
                    mode = YMD_M;
                    blink.start = 5;
                    blink.streak = 2;
                    break;
                case YMD_M:
                    up = up_day;
                    mode = YMD_D;
                    blink.start = 7;
                    blink.streak = 2;
                    break;
                case YMD_D:
                    buffer = buffer_hms;
                    up = up_hour;
                    mode = HMS_H;
                    blink.start = 1;
                    blink.streak = 2;
                    break;
                case HMS_H:
                    up = up_minute;
                    mode = HMS_M;
                    blink.start = 4;
                    blink.streak = 2;
                    break;
                case HMS_M:
                    up = up_second;
                    mode = HMS_S;
                    blink.start = 7;
                    blink.streak = 2;
                    break;
                case HMS_S:
                    buffer = buffer_counter;
                    up = nullptr;
                    mode = Counter;
                    blink.start = 0;
                    blink.streak = 0;
                    rtc_clock_enable();
                    break;
            }
            pressed.next = false;
        }
    }
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

void rtc_init(void) {
    /* Set Bit Rate Generator Unit
    ** SCL frequency to about 76.923kHz */
    twi_bitrate(0b0110'0000);
    twi_prescaler(0x00);
    /* Enable TWI interface */
    twi_enable();
}

void rtc_clock_enable(void) {
    twi_write_data(DS1337_SLA, 0x0E, 0b0000'0000);
}

void rtc_clock_disable(void) {
    twi_write_data(DS1337_SLA, 0x0E, 0b1000'0000);
}

void rtc_write_buffer(void) {
    twi_write_data(DS1337_SLA, 0x00, rtc.seconds);
    twi_write_data(DS1337_SLA, 0x01, rtc.minutes);
    twi_write_data(DS1337_SLA, 0x02, rtc.hours);
    twi_write_data(DS1337_SLA, 0x03, rtc.day);
    twi_write_data(DS1337_SLA, 0x04, rtc.date);
    twi_write_data(DS1337_SLA, 0x05, rtc.month_century);
    twi_write_data(DS1337_SLA, 0x06, rtc.year);
}

void rtc_read_buffer(void) {
    twi_read_data(DS1337_SLA, 0x00, &rtc.seconds);
    twi_read_data(DS1337_SLA, 0x01, &rtc.minutes);
    twi_read_data(DS1337_SLA, 0x02, &rtc.hours);
    twi_read_data(DS1337_SLA, 0x03, &rtc.day);
    twi_read_data(DS1337_SLA, 0x04, &rtc.date);
    twi_read_data(DS1337_SLA, 0x05, &rtc.month_century);
    twi_read_data(DS1337_SLA, 0x06, &rtc.year);
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

void buffer_counter(void) {
    rtc_read_buffer();
    time_t remain_s = rtc_difftime(DDAY_EPOCH, rtc_mktime(&rtc));

    for (int i = 8; i >= 0; --i) {
        buf[i] = remain_s % 10;
        remain_s /= 10;
    }
}

void buffer_ymd(void) {
    rtc_read_buffer();

    uint8_t date = rtc.date;
    uint8_t month = rtc.month_century & 0b0001'1111;
    uint8_t year = rtc.year;

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
    rtc_read_buffer();

    uint8_t seconds = rtc.seconds;
    uint8_t minutes = rtc.minutes;
    uint8_t hours = rtc.seconds;

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
    rtc_read_buffer();

    uint8_t date = rtc.date;
    uint8_t month = rtc.month_century & 0b0001'1111;
    uint8_t year = rtc.year;
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

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

ISR(PCINT0_vect) {
    if (bit_is_set(PINB, PINB6)) {
        pressed.next = true;
    }
    if (bit_is_set(PINB, PINB7)) {
        pressed.up = true;
    }
    /* Timer1 clear */
    TCNT1 = 0x0000;
    /* Timer1 prescale 8 */
    TCCR1B |= _BV(CS10);
    /* disable PCINT source 0 */
    PCICR &= ~_BV(PCIE0);
    PCIFR |= _BV(PCIF0);
}

ISR(TIMER1_OVF_vect) {
    /* Timer1 stop clock */
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
    /* Timer1 clear */
    TCNT1 = 0x0000;
    /* enable PCINT source 0 */
    PCIFR |= _BV(PCIF0);
    PCICR |= _BV(PCIE0);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
