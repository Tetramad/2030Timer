#include <stdint.h>

static inline uint8_t bcdtouint8(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0b0000'1111);
}

static inline uint8_t uint8tobcd(uint8_t uint8) {
    return (uint8 / 10) << 4 | (uint8 % 10);
}

static inline uint8_t bcd_incwa(uint8_t val, uint8_t max, uint8_t reset) {
    val += 1;
    if ((val & 0x0F) >= 0x0A) {
        val -= 0x0A;
        val += 0x10;
    }
    if ((val & 0xF0) >= 0xA0) {
        val -= 0xA0;
    }
    if (val > max) {
        val = reset;
    }
    return val;
}

static inline bool bcd_is_leap_year(uint8_t year) {
    /* assume year [00,99] as [2000, 2099] */
    /* bit magic warning */
    /* hi = x >> 4
    ** lo = x & 0x0F
    ** k = 10*hi + lo
    ** if k is divisible by 4, then ((hi<<1) + low)&0b11 would be 0.
    ** Because we need only last 2 bit, replacing hi and lo to expression of x
    ** is maybe ok */

    uint8_t hi = year >> 4;
    uint8_t lo = year & 0x0F;

    /* temporary calculation for optimization
    ** both operands of shift operator are applied integer promotion.
    ** because of that, compiler generates 16 bits operations.
    ** so, by cut down to 8 bits again compiler knows my intention. */
    hi = hi << 1;
    if (((hi + lo) & 0b11) != 0x00) {
        return false;
    } else {
        return true;
    }
}

static inline uint8_t bcd_month_length(uint8_t year, uint8_t month) {
    if (month == 0x02) {
        return 0x28 + bcd_is_leap_year(year);
    }

    if (month > 0x07) {
        month++;
    }

    return 0x30 + (month & 0x01);
}
