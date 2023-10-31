#include <stdint.h>

static inline void u8rprinti16(uint8_t buf[], int begin, int end, int16_t val) {
    bool const inc = begin <= end;

    for (int i = begin; (inc ? i <= end : i >= end); (inc ? ++i : --i)) {
        buf[i] = val % 10;
        val /= 10;
    }
}

static inline void u8rprinti8(uint8_t buf[], int begin, int end, int8_t val) {
    bool const inc = begin <= end;

    for (int i = begin; (inc ? i <= end : i >= end); (inc ? ++i : --i)) {
        buf[i] = val % 10;
        val /= 10;
    }
}

static inline void u8rprintu8(uint8_t buf[], int begin, int end, uint8_t val) {
    bool const inc = begin <= end;

    for (int i = begin; (inc ? i <= end : i >= end); (inc ? ++i : --i)) {
        buf[i] = val % 10;
        val /= 10;
    }
}

#define u8rprint(buf, begin, end, val) \
    _Generic((val), int16_t: u8rprinti16, int8_t: u8rprinti8, uint8_t: u8rprintu8)(buf, begin, end, val)

static inline void incwai16(int16_t *pval, int16_t max, int16_t reset) {
    int16_t val = *pval;

    ++val;
    if (val > max) {
        val = reset;
    }
    *pval = val;
}

static inline void incwai8(int8_t *pval, int8_t max, int8_t reset) {
    int8_t val = *pval;

    ++val;
    if (val > max) {
        val = reset;
    }
    *pval = val;
}

#define incwa(pval, max, reset) \
    _Generic((*pval), int16_t: incwai16, int8_t: incwai8)(pval, max, reset)

