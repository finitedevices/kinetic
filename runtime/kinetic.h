#ifndef KINETIC_H_
#define KINETIC_H_

#include <stdint.h>

typedef uint8_t KByte;
typedef uint16_t KWord;
typedef uint32_t KDWord;

typedef enum {
    KOP_RET     = 0b00000000,
    KOP_POP     = 0b00001000,
    KOP_PUSH    = 0b00010000,
    KOP_IDXA    = 0b00011000,
    KOP_LDA     = 0b00100000,
    KOP_LDB     = 0b00101000,
    KOP_STA     = 0b00110000,
    KOP_STB     = 0b00111000,
    KOP_SUB     = 0b01000000,
    KOP_ADD     = 0b01001000,
    KOP_DIV     = 0b01010000,
    KOP_MUL     = 0b01011000,
    KOP_OR      = 0b01100000,
    KOP_XOR     = 0b01101000,
    KOP_AND     = 0b01110000,
    KOP_NOT     = 0b01111000,
    KOP_EQ      = 0b10000000,
    KOP_MOD     = 0b10001000,
    KOP_GTN     = 0b10010000,
    KOP_BSR     = 0b10011000,
    KOP_DEC     = 0b10100000,
    KOP_LTN     = 0b10101000,
    KOP_BSL     = 0b10110000,
    KOP_INC     = 0b10111000,
    KOP_JUMP    = 0b11000000,
    KOP_CALL    = 0b11001000,
    KOP_STCB    = 0b11010000,
    KOP_INT     = 0b11011000,
    KOP_CLRA    = 0b11100000,
    KOP_CLRB    = 0b11101000,
    KOP_CIF     = 0b11110000,
    KOP_IF      = 0b11111000
} KOp;

typedef enum {
    KMODE_LIT   = 0b000,
    KMODE_MEM   = 0b001,
    KMODE_A     = 0b010,
    KMODE_B     = 0b011,
    KMODE_BYTE  = 0b000,
    KMODE_WORD  = 0b100
} KMode;

typedef enum {
    KINT_EXIT       = 0x00,
    KINT_SETUP      = 0x01,
    KINT_PROTECT    = 0x02,
    KINT_SAVE       = 0x03,
    KINT_LOAD       = 0x04
} KIntCode;

struct Kinetic;

typedef KByte (*KReadHandler)(struct Kinetic* vm, KWord addr);
typedef void (*KWriteHandler)(struct Kinetic* vm, KWord addr, KByte data);
typedef KByte (*KInterruptHandler)(struct Kinetic* vm, KWord code);

typedef struct KineticContext {
    KWord ip;       // Instruction pointer
    KWord csp;      // Call stack pointer
    KWord dsp;      // Data stack pointer
    KWord a;        // A register
    KWord b;        // B register
    KByte c;        // Carry register
    KWord offset;   // Protected context code start
    KWord limit;    // Protected context code end
    KWord fuel;     // Protected context maximum run length
} KineticContext;

typedef struct Kinetic {
    KineticContext main;
    KineticContext protect;
    KineticContext* currentContext;
    KReadHandler readHandler;
    KWriteHandler writeHandler;
    KInterruptHandler interruptHandler;
    void* metadata;
} Kinetic;

void kinetic_init(Kinetic* vm, KReadHandler readHandler, KWriteHandler writeHandler);
void kinetic_push(Kinetic* vm, KMode mode, KWord value);
KWord kinetic_pop(Kinetic* vm, KMode mode);
void kinetic_step(Kinetic* vm);

#endif