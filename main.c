#include "main.h"

int main(void) {
    fnd_init();
    time_init();
    ctl_init();
    sei();

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
        if (cnt > 3) {
            switch (mode) {
                case Counter:
                    break;
                case YMD_Y:
                    buf[1] = 10;
                    buf[2] = 10;
                    buf[3] = 10;
                    buf[4] = 10;
                    break;
                case YMD_M:
                    buf[5] = 10;
                    buf[6] = 10;
                    break;
                case YMD_D:
                    buf[7] = 10;
                    buf[8] = 10;
                    break;
                case HMS_H:
                    buf[1] = 10;
                    buf[2] = 10;
                    break;
                case HMS_M:
                    buf[4] = 10;
                    buf[5] = 10;
                    break;
                case HMS_S:
                    buf[7] = 10;
                    buf[8] = 10;
                    break;
            }
        }
        ++cnt;
        if (cnt > 7) {
            cnt = 0;
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
                    stop_counter();
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
                    resume_counter();
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
            if (n & _BV(j)) {
                PORTF &= ~_BV(PORTF0);
            } else {
                PORTF |= _BV(PORTF0);
            }
            PORTF |= _BV(PORTF4);
            PORTF &= ~_BV(PORTF4);
        }
    }
    PORTF |= _BV(PORTF6);
    PORTF &= ~_BV(PORTF6);
}

void fnd_reset(void) {
	PORTF |= _BV(PORTF0);
    for (int i = 8; i >= 0; --i) {
        for (int j = 0; j < 8; ++j) {
            PORTF |= _BV(PORTF4);
            PORTF &= ~_BV(PORTF4);
        }
    }
    PORTF |= _BV(PORTF6);
    PORTF &= ~_BV(PORTF6);
}

void time_init(void) {
    /* TODO: load rtc time */
    rtc_s = 0;

    /* disable global interrupt */
    uint8_t sreg = SREG;
    cli();
    /* Timer1 Configuration */
    /* Compare Output Mode to Normal port opertaion */
    TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0));
    /* Waveform Generation Mode to
    ** CTC(Clear Timer on Compare match)
    ** TOP as OCRnA
    ** Update of OCRnX at Immediate
    ** TOVn Flag Set on MAX */
    TCCR1A &= ~(_BV(WGM11) | _BV(WGM10));
    TCCR1B &= ~_BV(WGM13);
    TCCR1B |= _BV(WGM12);
    /* Output Compare Register to 1s(16000000 / 1024) */
    OCR1A = 0b11110100001001;
    /* Output Compare A Match Interrupt Enable */
    TIMSK1 |= _BV(OCIE1A);
    /* Clear Timer/Counter1 */
    TCNT1 = 0;
    /* Clock Select to 1024 prescaler */
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
    TCCR1B |= _BV(CS12) | _BV(CS10);
    /* reset global interrupt */
    SREG = sreg;
}

void buffer_counter(void) {
    int32_t remain_s = difftime(dday_s, rtc_s);
    assert(remain_s >= 0);

    for (int i = 8; i >= 0; --i) {
        buf[i] = remain_s % 10;
        remain_s /= 10;
    }
}

static void u8rprinti16(uint8_t buf[], int begin, int end, int16_t val) {
    bool const inc = begin <= end;

    for (int i = begin; (inc ? i <= end : i >= end); (inc ? ++i : --i)) {
        buf[i] = val % 10;
        val /= 10;
    }
}

static void u8rprinti8(uint8_t buf[], int begin, int end, int8_t val) {
    bool const inc = begin <= end;

    for (int i = begin; (inc ? i <= end : i >= end); (inc ? ++i : --i)) {
        buf[i] = val % 10;
        val /= 10;
    }
}

#define u8rprint(buf, begin, end, val) \
	_Generic((val), int16_t: u8rprinti16, int8_t: u8rprinti8)(buf, begin, end, val)

void buffer_ymd(void) {
    struct tm const t = *localtime(&rtc_s);
    int16_t const year = t.tm_year + 1900;
    int8_t const mon = t.tm_mon + 1;
    int8_t const mday = t.tm_mday;

	u8rprint(buf, 8, 7, mday);
	u8rprint(buf, 6, 5, mon);
	u8rprint(buf, 4, 1, year);
	buf[0] = 10;
}

void buffer_hms(void) {
    struct tm const t = *localtime(&rtc_s);
    int8_t const hour = t.tm_hour;
    int8_t const min = t.tm_min;
    int8_t const sec = t.tm_sec;

	u8rprint(buf, 8, 7, sec);
	buf[6] = 10;
	u8rprint(buf, 5, 4, min);
	buf[3] = 10;
	u8rprint(buf, 2, 1, hour);
    buf[0] = 10;
}

static void incwai16(int16_t *pval, int16_t max, int16_t reset) {
    int16_t val = *pval;

    ++val;
    if (val > max) {
        val = reset;
    }
    *pval = val;
}

 void incwai8(int8_t *pval, int8_t max, int8_t reset) {
    int8_t val = *pval;

    ++val;
    if (val > max) {
        val = reset;
    }
    *pval = val;
}

#define incwa(pval, max, reset) \
	_Generic((*pval), int16_t: incwai16, int8_t: incwai8)(pval, max, reset)

void up_year(void) {
    struct tm t = *localtime(&rtc_s);
    incwa(&t.tm_year, 2029 - YEAR_OFFSET, 2000 - YEAR_OFFSET);
    rtc_s = mktime(&t);
}

void up_month(void) {
    struct tm t = *localtime(&rtc_s);
    incwa(&t.tm_mon, 12 - MONTH_OFFSET, 0);
    rtc_s = mktime(&t);
}

void up_day(void) {
    struct tm t = *localtime(&rtc_s);
    incwa(&t.tm_mday, month_length(t.tm_year, t.tm_mon), 1);
    rtc_s = mktime(&t);
}

void up_hour(void) {
    struct tm t = *localtime(&rtc_s);
    incwa(&t.tm_hour, 23, 0);
    rtc_s = mktime(&t);
}

void up_minute(void) {
    struct tm t = *localtime(&rtc_s);
    incwa(&t.tm_min, 59, 0);
    rtc_s = mktime(&t);
}

void up_second(void) {
    struct tm t = *localtime(&rtc_s);
    incwa(&t.tm_sec, 59, 0);
    rtc_s = mktime(&t);
}

void ctl_init(void) {
    uint8_t sreg = { 0 };
    /* GPIO settings */
    DDRD &= ~(_BV(PORTD0) | _BV(PORTD1));
    DDRE &= ~_BV(PORTE6);
    PORTD &= ~(_BV(PORTD0) | _BV(PORTD1));
    PORTE &= ~_BV(PORTE6);

    /* disable global interrupt */
    sreg = SREG;
    cli();
    /* set INT6 to rising edge detection */
    EICRB |= _BV(ISC61) | _BV(ISC60);
    /* enable INT6 */
    EIMSK |= _BV(INT6);
    /* reset global interrupt */
    SREG = sreg;
}

void stop_counter(void) {
    uint8_t sreg = SREG;
    cli();
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
    TCNT1 = 0;
    SREG = sreg;
}

void resume_counter(void) {
    uint8_t sreg = SREG;
    cli();
    TCNT1 = 0;
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10));
    TCCR1B |= _BV(CS12) | _BV(CS10);
    SREG = sreg;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

ISR(INT6_vect) {
    if (bit_is_set(PIND, PIND0)) {
        is_N_pressed = true;
    }
    if (bit_is_set(PIND, PIND1)) {
        is_U_pressed = true;
    }
}

ISR(TIMER1_COMPA_vect) {
	++rtc_s;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
