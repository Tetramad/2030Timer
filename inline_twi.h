#include <avr/io.h>
#include <util/twi.h>

static inline void twi_bitrate(uint8_t bitrate) {
    TWBR = bitrate;
}

static inline void twi_prescaler(uint8_t prescaler) {
    TWSR = prescaler & 0b10 ? TWSR | _BV(TWPS1) : TWSR & ~_BV(TWPS1);
    TWSR = prescaler & 0b01 ? TWSR | _BV(TWPS1) : TWSR & ~_BV(TWPS1);
}

static inline void twi_enable(void) {
    TWCR |= _BV(TWEN);
}

static inline bool twi_start(void) {
    /* Send start bit */
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    loop_until_bit_is_set(TWCR, TWINT);
    return TW_STATUS == TW_START;
}

static inline bool twi_send_address(uint8_t address, bool tw_rw) {
    TWDR = (address << 1) | tw_rw;
    TWCR = _BV(TWINT) | _BV(TWEN);
    loop_until_bit_is_set(TWCR, TWINT);
    return TW_STATUS == (tw_rw ? TW_MR_SLA_ACK : TW_MT_SLA_ACK);
}

static inline bool twi_write_data(uint8_t data) {
    TWDR = data;
    TWCR = _BV(TWINT) | _BV(TWEN);
    loop_until_bit_is_set(TWCR, TWINT);
    return TW_STATUS == TW_MT_DATA_ACK;
}

static inline bool twi_write_data_burst(int n, uint8_t data[static n]) {
    bool result = true;
    for (int i = 0; result && i < n; ++i) {
        TWDR = data[i];
        TWCR = _BV(TWINT) | _BV(TWEN);
        loop_until_bit_is_set(TWCR, TWINT);
        result = result && (TW_STATUS == TW_MT_DATA_ACK);
    }
    return result;
}

static inline bool twi_repeated_start(void) {
    /* Send repeated start bit */
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    loop_until_bit_is_set(TWCR, TWINT);
    return TW_STATUS == TW_REP_START;
}

static inline bool twi_read_data(uint8_t *data, bool ack) {
    TWCR = _BV(TWINT) | (ack << TWEA) | _BV(TWEN);
    loop_until_bit_is_set(TWCR, TWINT);
    if (TW_STATUS == (ack ? TW_MR_DATA_ACK : TW_MR_DATA_NACK)) {
        *data = TWDR;
    }
    return TW_STATUS == (ack ? TW_MR_DATA_ACK : TW_MR_DATA_NACK);
}

static inline bool twi_read_data_burst(int n, uint8_t data[static n]) {
    bool result = true;
    for (int i = 0; result && i < n - 1; ++i) {
        TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN);
        loop_until_bit_is_set(TWCR, TWINT);
        result = result && (TW_STATUS == TW_MR_DATA_ACK);
        if (result) {
            data[i] = TWDR;
        }
    }
    TWCR = _BV(TWINT) | _BV(TWEN);
    loop_until_bit_is_set(TWCR, TWINT);
    result = result && (TW_STATUS == TW_MR_DATA_NACK);
    if (result) {
        data[n - 1] = TWDR;
    }
    return result;
}

static inline bool twi_stop(void) {
    /* Send stop bit */
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
    return true;
}

