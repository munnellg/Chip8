#ifndef _CHIP8_H_
#define _CHIP8_H_

#include <stdint.h>

/* uncomment to enable disassembly dump to terminal when emulator is running */
/* #define CHIP8_DEBUG_ENABLE */

/* useful macro for printing code that is being executed by emulator */
#ifdef CHIP8_DEBUG_ENABLE
#include <stdio.h>
#define DEBUG(pc, opcode, ...) do { \
    fprintf( stdout, "0x%04X : 0x%04X : ", pc, opcode ); \
    fprintf( stdout, __VA_ARGS__ );                      \
    fprintf( stdout, "\n" );                             \
} while(0)
#else
#define DEBUG(pc, opcode, ...) do { } while(0)
#endif

/* chip-8 screen properties */
/* display buf is smaller than display because 8 pixels per buf element */
#define CHIP8_DISPLAY_BUF_WIDTH  8
#define CHIP8_DISPLAY_BUF_HEIGHT 32
#define CHIP8_DISPLAY_BUF_SIZE (CHIP8_DISPLAY_BUF_WIDTH*CHIP8_DISPLAY_BUF_HEIGHT)
#define CHIP8_DISPLAY_WIDTH      (CHIP8_DISPLAY_BUF_WIDTH * 8)
#define CHIP8_DISPLAY_HEIGHT     CHIP8_DISPLAY_BUF_HEIGHT

#define CHIP8_MEMORY_CAPACITY 0x1000

/* chip-8 keyboard key codes */
#define CHIP8_KEY_0 0x00
#define CHIP8_KEY_1 0x01
#define CHIP8_KEY_2 0x02
#define CHIP8_KEY_3 0x03
#define CHIP8_KEY_4 0x04
#define CHIP8_KEY_5 0x05
#define CHIP8_KEY_6 0x06
#define CHIP8_KEY_7 0x07
#define CHIP8_KEY_8 0x08
#define CHIP8_KEY_9 0x09
#define CHIP8_KEY_A 0x0A
#define CHIP8_KEY_B 0x0B
#define CHIP8_KEY_C 0x0C
#define CHIP8_KEY_D 0x0D
#define CHIP8_KEY_E 0x0E
#define CHIP8_KEY_F 0x0F

/* chip-8 keyboard key states */
#define CHIP8_KEY_DOWN 1
#define CHIP8_KEY_UP   0

typedef uint8_t  u8;  /* 8-bit unsigned data type */
typedef uint16_t u16; /* 16-bit unsigned data type */

struct chip8 {
	u16 i;     /* 16 bit address register */
	u16 pc;    /* program counter */
	u8  v[16]; /* Chip 8 has 16 8 bit registers. VF is status register */
	u8  sp;    /* stack pointer */
	u8  dt;    /* delay timer */
	u8  st;    /* sound timer */

	/* stack used to store pc when calling subroutines */
	u16 stack[16];
	/* chip-8 memory for storing ROMs etc */
	u8 mem[CHIP8_MEMORY_CAPACITY];
	/* chip-8 screen buffer */
	u8 display[CHIP8_DISPLAY_BUF_WIDTH * CHIP8_DISPLAY_BUF_HEIGHT];
	/* chip-8 keyboard key states */
	u8 keyboard[16];

	/* boolean will be set to 1 when chip-8 emits beep */
	int beep;
};

void chip8_init ( struct chip8 *chip8 );
int  chip8_load ( struct chip8 *chip8, const char *romfile );
void chip8_step ( struct chip8 *chip8 );
void chip8_key_set_state ( struct chip8 *chip8, int key, int state );

#endif
