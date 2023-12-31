#include "main.h"
#include "bcd.h"
#include "twi.h"

static uint8_t buf[9] = { 0, };
static uint8_t const FND_LUT[] = {
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
static rtc_t rtc = {
    0b0000'0000,
    0b0000'0000,
    0b0000'0000,
    0b0000'0001,
    0b0000'0001,
    0b1000'0001,
    0b0000'0000
};
static button_press_info_t pressed = { false, false };
static const state_info_t state_info[7] = {
    [Counter] = {
        .buffer = buffer_counter,
        .up = nullptr,
        .blink = { .start = 0, .streak = 0 },
    },
    [YMD_Y] = {
        .buffer = buffer_ymd,
        .up = up_year,
        .blink = { .start = 1, .streak = 4 },
    },
    [YMD_M] = {
        .buffer = buffer_ymd,
        .up = up_month,
        .blink = { .start = 5, .streak = 2 },
    },
    [YMD_D] = {
        .buffer = buffer_ymd,
        .up = up_day,
        .blink = { .start = 7, .streak = 2 },
    },
    [HMS_H] = {
        .buffer = buffer_hms,
        .up = up_hour,
        .blink = { .start = 1, .streak = 2 },
    },
    [HMS_M] = {
        .buffer = buffer_hms,
        .up = up_minute,
        .blink = { .start = 4, .streak = 2 },
    },
    [HMS_S] = {
        .buffer = buffer_hms,
        .up = up_second,
        .blink = { .start = 7, .streak = 2 },
    },
};

int main(void) {
    /*
    ** Initializations
    */

    /* Cascaded shift registers for FNDs
    ** PF0 --- SER
    ** PF4 --- /SRCLR
    ** PF5 --- SRCLK
    ** PF6 --- RCLK
    ** PF7 --- /RCLR
    */
    /* IO register configurations */
    DDRF |= _BV(PORTF0) | _BV(PORTF4) | _BV(PORTF5) | _BV(PORTF6) | _BV(PORTF7);
    PORTF &= ~(_BV(PORTF0) | _BV(PORTF4) | _BV(PORTF5) | _BV(PORTF6) | _BV(PORTF7));
    /* Reset shift registers */
    PORTF |= _BV(PORTF4) | _BV(PORTF7);

    /* Buttons
    ** PB6 --- Button.Up
    ** PB7 --- Button.Next
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

    /* TWI configurations
    ** SCL frequency = CPU Clock frequency / (16 + 2 * TWBR * pow(4, TWPS))
    ** TWBR should be 10 or higher if the TWI operates in Master mode.
    **
    ** TWBR = 96 and TWPS = 0
    ** SCL frequency to 50kHz
    */
    TWBR = 152;
    /* TWPS default to 0b00 */
    /* Enable TWI interface */
    TWCR = _BV(TWEN) | _BV(TWIE);

    /*
    ** End of initializations
    */

    sei();

    rtc_read_buffer();

    state_t mode = Counter;
    uint8_t cnt = 0;

    for (;;) {
        _delay_ms(100);

        void (*const buffer)(void) = state_info[mode].buffer;
        void (*const up)(void) = state_info[mode].up;
        const blink_info_t blink = state_info[mode].blink;
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
            mode = mode + 1 > HMS_S ? Counter : mode + 1;
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
            PORTF |= _BV(PORTF5);
            PORTF &= ~_BV(PORTF5);
            n >>= 1;
        }
    }
    PORTF |= _BV(PORTF6);
    PORTF &= ~_BV(PORTF6);
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

time_t rtc_mktime(rtc_t const *rtc) {
    time_t t = 0;

    uint16_t year = bcdtouint8(rtc->year);
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

    uint8_t date = bcdtouint8(rtc->date);
    t += date - 1;

    uint8_t hours = bcdtouint8(rtc->hours);
    t *= 24;
    t += hours;

    uint8_t minutes = bcdtouint8(rtc->minutes);
    t *= 60;
    t += minutes;

    uint8_t seconds = bcdtouint8(rtc->seconds);
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
    uint8_t hours = rtc.hours;

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

ISR(PCINT0_vect) {
    if (bit_is_set(PINB, PINB7)) {
        pressed.next = true;
    }
    if (bit_is_set(PINB, PINB6)) {
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

