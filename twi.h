#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <util/delay.h>

volatile uint8_t twi_slarw = 0x00;
#define TWI_BUFFER_SIZE 2
volatile uint8_t twi_buffer[TWI_BUFFER_SIZE] = { 0, };
volatile uint8_t twi_index = 0;

enum {
    TWI_READY,
    TWI_ADDRESSED_WRITE,
    TWI_ADDRESSED_READ,
};
typedef uint8_t twi_state_t;

static volatile twi_state_t twi_state = TWI_READY;

/* T_SU_STO_MIN: Minimum setup time for STOP condition in microsecond (f_SCL <= 100kHz) */
#define T_SU_STO_MIN 4.0
#define T_SU_STO_TOLERANCE 6.0
#define STOP_DELAY do { _delay_us(T_SU_STO_MIN + T_SU_STO_TOLERANCE); } while (false)

#define TX_START do { TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE); } while (false)
#define TX_STOP do { TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN) | _BV(TWIE); STOP_DELAY; twi_state = TWI_READY; } while (false)
#define TX_WITH_ACK do { TWCR = TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE); } while (false)
#define TX_WITH_NACK do { TWCR = TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE); } while (false)
#define TX_WITH_XACK TX_WITH_NACK
#define WAIT_TWI_READY do { while (twi_state != TWI_READY) {} } while (false)

static inline void twi_write_data(uint8_t sla, uint8_t loc, uint8_t data) {
    WAIT_TWI_READY;
    twi_state = TWI_ADDRESSED_WRITE;
    twi_slarw = (sla << 1) | TW_WRITE;
    twi_index = 0;
    twi_buffer[0] = loc;
    twi_buffer[1] = data;
    TX_START;
}

static inline void twi_read_data(uint8_t sla, uint8_t loc, uint8_t *receive) {
    WAIT_TWI_READY;
    twi_state = TWI_ADDRESSED_READ;
    twi_slarw = (sla << 1) | TW_WRITE;
    twi_index = 0;
    twi_buffer[0] = loc;
    TX_START;
    WAIT_TWI_READY;
    *receive = twi_buffer[1];
}

ISR(TWI_vect) {
    switch (TW_STATUS) {
        case TW_START:
        case TW_REP_START:
            TWDR = twi_slarw;
            TX_WITH_XACK;
            break;
        case TW_MT_SLA_ACK:
            TWDR = twi_buffer[twi_index];
            ++twi_index;
            TX_WITH_XACK;
            break;
        case TW_MT_DATA_ACK:
            if (twi_state == TWI_ADDRESSED_READ) {
                twi_slarw = twi_slarw | TW_READ;
                TX_START;
            } else {
                if (twi_index < TWI_BUFFER_SIZE) {
                    TWDR = twi_buffer[twi_index];
                    ++twi_index;
                    TX_WITH_XACK;
                } else {
                    TX_STOP;
                }
            }
            break;
        case TW_MR_SLA_ACK:
        case TW_MR_DATA_ACK:
            TX_WITH_NACK;
            break;
        case TW_MR_DATA_NACK:
            twi_buffer[1] = TWDR;
            TX_STOP;
            break;
        case TW_MT_SLA_NACK:
        case TW_MT_DATA_NACK:
        case TW_MR_SLA_NACK:
        default:
            TX_STOP;
            break;
    }
}

