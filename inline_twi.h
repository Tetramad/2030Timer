#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

enum {
    TWIC_STO,
    TWIC_WRITE_SLA,
    TWIC_WRITE_LOC,
    TWIC_WRITE_DATA,
    TWIC_READ_SLA,
    TWIC_READ_SELECT,
    TWIC_READ_REP,
    TWIC_READ_REP_SLA,
    TWIC_READ_READY,
    TWIC_READ_DATA,
} volatile twic = TWIC_STO;

volatile uint8_t twic_sla = 0x00;
volatile uint8_t twic_loc = 0x00;
volatile uint8_t twic_data = 0x00;
uint8_t * volatile twic_receive = nullptr;

static inline void twi_enable(void) {
    TWCR = _BV(TWEN) | _BV(TWIE);
}

static inline void twi_bitrate(uint8_t bitrate) {
    TWBR = bitrate;
}

static inline void twi_prescaler(uint8_t prescaler) {
    TWSR = prescaler & 0b10 ? TWSR | _BV(TWPS1) : TWSR & ~_BV(TWPS1);
    TWSR = prescaler & 0b01 ? TWSR | _BV(TWPS1) : TWSR & ~_BV(TWPS1);
}

static inline void twi_write_data(uint8_t sla, uint8_t loc, uint8_t data) {
    while (twic != TWIC_STO) {}
    twic_sla = sla;
    twic_loc = loc;
    twic_data = data;
    twic = TWIC_WRITE_SLA;
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
}

static inline void twi_read_data(uint8_t sla, uint8_t loc, uint8_t *receive) {
    while (twic != TWIC_STO) {}
    twic_sla = sla;
    twic_loc = loc;
    twic_receive = receive;
    twic = TWIC_READ_SLA;
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

ISR(TWI_vect) {
    switch (twic) {
        case TWIC_WRITE_SLA:
            if (TW_STATUS == TW_START) {
                TWDR = (twic_sla << 1) | TW_WRITE;
                twic = TWIC_WRITE_LOC;
                TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_WRITE_LOC:
            if (TW_STATUS == TW_MT_SLA_ACK) {
                TWDR = twic_loc;
                twic = TWIC_WRITE_DATA;
                TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_WRITE_DATA:
            if (TW_STATUS == TW_MT_DATA_ACK) {
                TWDR = twic_data;
                twic = TWIC_STO;
                TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_READ_SLA:
            if (TW_STATUS == TW_START) {
                TWDR = (twic_sla << 1) | TW_WRITE;
                twic = TWIC_READ_SELECT;
                TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_READ_SELECT:
            if (TW_STATUS == TW_MT_SLA_ACK) {
                TWDR = twic_loc;
                twic = TWIC_READ_REP;
                TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
                return;
            } {
                break;
            }
        case TWIC_READ_REP:
            if (TW_STATUS == TW_MT_DATA_ACK) {
                twic = TWIC_READ_REP_SLA;
                TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_READ_REP_SLA:
            if (TW_STATUS == TW_REP_START) {
                TWDR = (twic_sla << 1) | TW_READ;
                twic = TWIC_READ_READY;
                TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_READ_READY:
            if (TW_STATUS == TW_MR_SLA_ACK) {
                twic = TWIC_READ_DATA;
                TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_READ_DATA:
            if (TW_STATUS == TW_MR_DATA_NACK) {
                *twic_receive = TWDR;
                twic = TWIC_STO;
                TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN) | _BV(TWIE);
                return;
            } else {
                break;
            }
        case TWIC_STO:
            break;
    }
    twic = TWIC_STO;
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN) | _BV(TWIE);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
