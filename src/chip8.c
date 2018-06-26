#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static u8 fonts[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */
    0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */
    0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */
    0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */
    0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */
    0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */
    0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */
    0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */
    0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */
    0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */
    0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */
    0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */
    0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */
    0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */
    0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */
    0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */
};

void
chip8_init ( struct chip8 *c8 ) {
    memset( c8, 0, sizeof(struct chip8) );
    memcpy( c8->mem, fonts, sizeof(fonts)/sizeof(fonts[0]) );
    c8->pc = 0x200;
}

int
chip8_load ( struct chip8 *c8, const char *romfile ) {
    FILE *f;
    size_t flen;
    f = fopen( romfile, "rb" );
    if (!f) { return 0; }
    fseek(f, 0, SEEK_END);
    flen = ftell(f);
    fseek(f, 0, SEEK_SET);
    if ( flen + 0x200 >= CHIP8_MEMORY_CAPACITY ) { fclose(f); return 0; }
    fread( c8->mem + 0x200, flen, sizeof(u8), f );    
    fclose(f);
    return 1;
}

void
chip8_step ( struct chip8 *c8 ) {
    /* fetch the next instruction and advance program counter */
    u16 opcode = c8->mem[c8->pc++];
    opcode = (opcode << 8) | c8->mem[c8->pc++];

    /* decode opcode and pull out all possible arguments */
    u8  byte = opcode & 0xFF;
    u16 word = opcode & 0xFFF;
    u8  x    = (opcode >> 8) & 0xF;
    u8  y    = (opcode >> 4) & 0xF;
    u8  n    = (opcode >> 0) & 0xF;
    
    /* tick timers along */
    if (c8->st == 1) { c8->beep = 1; }
    if (c8->st > 0)  { c8->st--; }
    if (c8->dt > 0)  { c8->dt--; }
    
    switch ( opcode & 0xF000 ) {
        case 0x0000:
            switch ( opcode & 0x0FFF ) {
                case 0x00E0: 
                    /* clear the screen */
                    DEBUG(c8->pc-2, opcode, "CLS");
                    memset( c8->display, 0, sizeof(u8)*CHIP8_DISPLAY_BUF_SIZE );
                    break;
                case 0x00EE: 
                    /* return from subroutine */
                    DEBUG(c8->pc-2, opcode, "RET");
                    c8->pc = c8->stack[--c8->sp];
                    break;
                default:
                    /* call program (typically not implemented) */
                    DEBUG(c8->pc-2, opcode, "SYS  0x%03X", word);
                    c8->pc = word;
                    break;
            }
            break;        
        case 0x1000:
            /* jump to address */
            DEBUG(c8->pc-2, opcode, "JP   0x%03X", word ); 
            c8->pc = word; 
            break;
        case 0x2000:
            /* call subroutine: backup pc on stack and then branch */
            DEBUG(c8->pc-2, opcode, "CALL 0x%03X", word ); 
            c8->stack[c8->sp++] = c8->pc;            
            c8->pc = word;
            break;
        case 0x3000:
            /* skip next instruction if Vx == byte */
            DEBUG(c8->pc-2, opcode, "SE   V%d, 0x%02X", x, byte ); 
            if (c8->v[x] == byte) { c8->pc += 2; }
            break;
        case 0x4000:
            /* skip next instruction if Vx != byte */
            DEBUG(c8->pc-2, opcode, "SNE  V%d, Ox%02X", x, byte ); 
            if (c8->v[x] != byte) { c8->pc += 2; }
            break;
        case 0x5000:
            /* skip next instruction if Vx == Vy */
            DEBUG(c8->pc-2, opcode, "SE   V%d, V%d", x, y ); 
            if (c8->v[x] == c8->v[y]) { c8->pc += 2; }
            break;
        case 0x6000:
            /* load byte into Vx */
            DEBUG(c8->pc-2, opcode, "LD   V%d, 0x%02X", x, byte );  
            c8->v[x] = byte;
            break;
        case 0x7000:
            /* add byte to Vx and store the result in Vx */
            DEBUG(c8->pc-2, opcode, "ADD  V%d, 0x%02X", x, byte );  
            c8->v[x] += byte;
            break;
        case 0x8000:
            switch ( opcode & 0x000F ) {
                case 0x0000:
                    /* load Vy into Vx */
                    DEBUG(c8->pc-2, opcode, "LD   V%d, V%d", x, y ); 
                    c8->v[x] = c8->v[y];
                    break;
                case 0x0001:
                    /* or the values in Vx and Vy. Store result in Vx */
                    DEBUG(c8->pc-2, opcode, "OR   V%d, V%d", x, y ); 
                    c8->v[x] |= c8->v[y];
                    break;
                case 0x0002:
                    /* and the values in Vx and Vy. Store result in Vx */
                    DEBUG(c8->pc-2, opcode, "AND  V%d, V%d", x, y ); 
                    c8->v[x] &= c8->v[y];
                    break;
                case 0x0003:
                    /* xor the values in Vx and Vy. Store result in Vx */
                    DEBUG(c8->pc-2, opcode, "XOR  V%d, V%d", x, y ); 
                    c8->v[x] ^= c8->v[y];
                    break;
                case 0x0004:
                    /* add the values in Vx and Vy. Store result in Vx */
                    /* VF is set if overflow occurs */
                    DEBUG(c8->pc-2, opcode, "ADD  V%d, V%d", x, y ); 
                    c8->v[0xF] = ((255 - c8->v[x]) < c8->v[y]);
                    c8->v[x] += c8->v[y];
                    break;
                case 0x0005:
                    /* subtract Vy from Vx and store result in Vx */
                    /* VF is 0 if result is negative */
                    DEBUG(c8->pc-2, opcode, "SUB  V%d, V%d", x, y ); 
                    c8->v[0xF] = c8->v[y] < c8->v[x];
                    c8->v[x] -= c8->v[y];
                    break;
                case 0x0006:
                    /* shift Vx right by one position */
                    /* VF is set to value of least significant bit */
                    DEBUG(c8->pc-2, opcode, "SHR  V%d", x ); 
                    c8->v[0xF] = c8->v[x] & 0x01;
                    c8->v[x] >>= 1;
                    break;
                case 0x0007:
                    /* subtract Vx from Vy and store result in Vx */
                    /* VF is 0 if result is negative */
                    DEBUG(c8->pc-2, opcode, "SUBN V%d, V%d", x, y ); 
                    c8->v[0xF] = c8->v[x] < c8->v[y];
                    c8->v[x] = c8->v[y] - c8->v[x];
                    break;
                case 0x000E:
                    /* shift Vx left by one position */
                    /* VF is set to value of most significant bit */
                    DEBUG(c8->pc-2, opcode, "SHL  V%d", x ); 
                    c8->v[0xF] = c8->v[x] & 0x80;
                    c8->v[x] <<= 1;
                    break;
            }
            break;
        case 0x9000:
            /* skip next instruction if Vx != Vy */
            DEBUG(c8->pc-2, opcode, "SNE  V%d, V%d", x, y ); 
            if (c8->v[x] != c8->v[y]) { c8->pc += 2; }
            break;
        case 0xA000:
            /* set value of I register to literal address */
            DEBUG(c8->pc-2, opcode, "LD   I, 0x%03X", word ); 
            c8->i = word;
            break;
        case 0xB000:
            /* jump to literal address incremented by value of Vx */
            DEBUG(c8->pc-2, opcode, "JP   V0, 0x%03X", word ); 
            c8->pc = word + c8->v[0];
            break;
        case 0xC000:
            /* get random number anded with value of byte */
            DEBUG(c8->pc-2, opcode, "RND  V%d, 0x%02X", x, byte );
            c8->v[x] = rand()%256 & byte;
            break;
        case 0xD000: {
            /* draws a sprite to the screen and performs collision detection */
            /* Vx and Vy are screen position of sprite */
            /* register I is the memory location for the start of the sprite */
            /* n is the number of bytes which must be read to retrieve sprite */
            /* VF is 1 on collision i.e. sprite overlaps another sprite */

            DEBUG(c8->pc-2, opcode, "DRW  V%d, V%d, 0x%X", x, y, n );
            int sx = c8->v[x] % CHIP8_DISPLAY_WIDTH;  /* sprite x coord */
            int sy = c8->v[y] % CHIP8_DISPLAY_HEIGHT; /* sprite y coord */
            int sbufx = sx / 8;      /* sprite x coord index in display */
            int rshift = sx % 8;     /* right shift for MSBs of sprite */
            int lshift = 8 - rshift; /* left shift for LSBs of sprite */
            
            c8->v[0xF] = 0;          /* assume no collision */

            if ( n + sy > CHIP8_DISPLAY_HEIGHT ) { 
                n = CHIP8_DISPLAY_HEIGHT - sy;
            }
            
            /* for each row of the sprite */
            for ( int i = 0; i < n; i++, sy++ ) {
                /* retrieve the row from memory */
                u8 sprite = c8->mem[c8->i + i];   
                
                /* compute index in display buffer */             
                int sbufi = sbufx + sy * CHIP8_DISPLAY_BUF_WIDTH;
                if ( sbufi > CHIP8_DISPLAY_BUF_SIZE ) { break; }

                /* test for collision with anything on screen */
                if ( (c8->display[sbufi] & (sprite >> rshift)) != 0 ) {
                    c8->v[0xF] = 1;
                }

                /* draw first part of sprite */
                c8->display[sbufi++] ^= (sprite >> rshift);
                if ( sbufi > CHIP8_DISPLAY_BUF_SIZE ) { break; }

                /* check for collision */
                if ( (c8->display[sbufi] & (sprite << lshift)) != 0 ) {
                    c8->v[0xF] = 1;
                }

                /* render second half of the sprite */
                c8->display[sbufi] ^= (sprite << lshift);                
            }
            break;
        }            
        case 0xE000:
            switch ( opcode & 0x00FF ) {                
                case 0x009E: 
                    /* skip the next instruction if key in Vx is pressed */
                    DEBUG(c8->pc-2, opcode, "SKP  V%d", x );
                    if (c8->keyboard[c8->v[x]] == CHIP8_KEY_DOWN) {
                        c8->pc += 2;
                    }
                    break;
                case 0x00A1: 
                    /* skip the next instruction if key in Vx is not pressed */
                    DEBUG(c8->pc-2, opcode, "SKNP V%d", x );
                    if (c8->keyboard[c8->v[x]] != CHIP8_KEY_DOWN) {
                        c8->pc += 2;
                    }
                    break;
            }
            break;
        case 0xF000:
            switch ( opcode & 0x00FF ) {
                case 0x0007:
                    /* load the value of the delay timer into Vx */
                    DEBUG(c8->pc-2, opcode, "LD   V%d, DT", x );
                    c8->v[x] = c8->dt;
                    break;
                case 0x000A:
                    /* pause for key press and store pressed key in Vx */
                    DEBUG(c8->pc-2, opcode, "LD   V%d, K", x );
                    c8->pc -= 2;
                    for ( int i=0; i <= 0xF; i++ ) {
                        if ( c8->keyboard[i] == CHIP8_KEY_DOWN ) {
                            c8->v[x] = i;
                            c8->pc += 2;
                            break;
                        }
                    }
                    break;
                case 0x0015:
                    /* set delay timer to value of Vx */
                    DEBUG(c8->pc-2, opcode, "LD   DT, V%d", x );
                    c8->dt = c8->v[x];
                    break;
                case 0x0018:
                    /* set sound timer to value of Vx */
                    DEBUG(c8->pc-2, opcode, "LD   ST, V%d", x );
                    c8->st = c8->v[x];
                    break;
                case 0x001E:
                    /* increment I by value in Vx */
                    DEBUG(c8->pc-2, opcode, "ADD  I, V%d", x );
                    c8->i += c8->v[x];
                    break;
                case 0x0029:
                    /* point I to address of font for value in Vx */
                    DEBUG(c8->pc-2, opcode, "LD   F, V%d", x );
                    c8->i = (c8->v[x] % 0x10) * 5;
                    break;
                case 0x0033:
                    /* store value of Vx in BCD at location pointed to by I */
                    DEBUG(c8->pc-2, opcode, "LD   B, V%d", x );
                    c8->mem[c8->i + 0] = (c8->v[x] / 100);
                    c8->mem[c8->i + 1] = (c8->v[x] / 10) % 100;
                    c8->mem[c8->i + 2] = (c8->v[x]) % 10;
                    break;
                case 0x0055:
                    /* store registers V0-Vx at location pointed to by I */
                    DEBUG(c8->pc-2, opcode, "LD   [I], V%d", x );
                    for ( int i = 0; i <= x; i++ ) {
                        c8->mem[c8->i + i] = c8->v[i];
                    }                    
                    break;
                case 0x0065:
                    /* load registers V0-Vx from location pointed to by I */
                    DEBUG(c8->pc-2, opcode, "LD   V%d, [I]", x );
                    for ( int i = 0; i <= x; i++ ) {
                        c8->v[i] = c8->mem[c8->i + i];
                    }
                    break;

            } 
            break;
        default:
            fprintf( stdout, "Opcode 0x%04X not implemented\n", opcode );
    }
}

void
chip8_key_set_state ( struct chip8 *chip8, int key, int state ) {
    chip8->keyboard[key] = state;    
}
