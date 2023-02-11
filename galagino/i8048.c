/*
 * i8048.c - 8048 emulator for galagino/donkey kong arcade audio
 *           controller
 *
 * This emulation does not implement all instructions. Especially
 * those dealing with port 0 are missing as the Donkey Kong audio
 * controller uses external ROM and port 0 is not available for
 * general use.
 *
 * Derived from:
 * https://github.com/asig/kosmos-cp1/blob/master/src/main/java/com/asigner/cp1/emulation/Intel8049.java
 */

#include "i8048.h"

//
// Bit-fiddling
//

#define GET_BIT(val, bit) ((val >> bit)&1)
#define SET_BIT(val, bit) (val | (1<<bit))
#define CLR_BIT(val, bit) (val & ~(1<<bit))
#define SET_BIT_TO(val, bit, state)  ((val & ~(1<<bit)) | ((state?1:0)<<bit))

//
// Read/Write
//

static inline void ram_write(struct i8048_state_S *state, unsigned char addr, unsigned char data) {
  //  printf("@%04x RAM WR %04x = %02x\n", state->PC-1, addr, data);
  state->ram[addr & 0x7f] = data;
}

static inline unsigned char ram_read(struct i8048_state_S *state, unsigned char addr) {
  //  printf("@%04x RAM RD %04x = %02x\n", state->PC-1, addr, ram[addr]);
  return state->ram[addr & 0x7f];
}

static inline unsigned char readReg(struct i8048_state_S *state, unsigned char reg) {
  unsigned short base = !GET_BIT(state->PSW, BS_BIT) ? REGISTER_BANK_0_BASE : REGISTER_BANK_1_BASE;
  // printf("RR %02x %02x = %02x\n", base, reg, ram_read(state, base + reg));
  return ram_read(state, base + reg);
}

static inline void writeReg(struct i8048_state_S *state, unsigned char reg, unsigned char val) {
  unsigned short base = !GET_BIT(state->PSW, BS_BIT) ? REGISTER_BANK_0_BASE : REGISTER_BANK_1_BASE;
  ram_write(state, base + reg, val);
}

static inline void setCarry(struct i8048_state_S *state, boolean carry) {
  state->PSW = SET_BIT_TO(state->PSW, CY_BIT, carry);
}

static void addToAcc(struct i8048_state_S *state, int value) {
  value &= 0xff;
  int oldA = state->A;
  int oldLoNibble = state->A & 0xf;
  state->A = (state->A + value) & 0xff;
  int loNibble = state->A & 0xf;
  setCarry(state, oldA > state->A);
  state->PSW = SET_BIT_TO(state->PSW, AC_BIT, oldLoNibble > loNibble);
}

void i8048_reset(struct i8048_state_S *state) {
  // clear mem without external dependency
  for(int i=0;i<sizeof(struct i8048_state_S);i++)
    ((unsigned char*)state)[i] = 0;

  state->p2_state  = 0xff;   // port 2 state 
    
  state->notINT = true;
  state->T0 = true;         // low when "score" sfx
  state->T1 = true;         // low when "falling" sfx
  
  state->PSW = 0x08;  // bit 3 is always rom_read as "1"
}

static inline void tick(struct i8048_state_S *state) {
  // "executes" another cycle, and performs some periodic
  // stuff (e.g. counting after a STRT T)
  if (state->timerRunning) {
    state->cyclesUntilCount--;
    if(!(state->cyclesUntilCount & 31)) {
      // increment counter
      state->TF |= (state->T == 0xff);
      state->timerInterruptRequested |= (state->T == 0xff);
      
      state->T++;
    }
  }
}

static inline unsigned char fetch(struct i8048_state_S *state) {
  tick(state);
  return i8048_rom_read(state, state->PC++);
}

static void push(struct i8048_state_S *state) {
  unsigned char sp = state->PSW & 0x7;
  ram_write(state, 8+2*sp, state->PC & 0xff);
  ram_write(state, 9+2*sp, (state->PSW & 0xf0) | ((state->PC >> 8) & 0xf) );
  state->PSW = (state->PSW & 0xf8) | ((sp + 1) & 0x7);
}

static void pop(struct i8048_state_S *state, boolean restoreState) {
  unsigned char sp = (state->PSW - 1) & 0x7;
  unsigned char highbyte = ram_read(state, 9+2*sp);
  state->PC = ((highbyte & 0xf) << 8) | ram_read(state, 8+2*sp);
  if (restoreState) {
    state->PSW = (highbyte & 0xf0) | 0x8 | (sp & 0x7);
    state->inInterrupt = false;
  } else {
    state->PSW = state->PSW & 0xf0 | 0x8 | (sp & 0x7);
  }
}

static void handleInterrupts(struct i8048_state_S *state) {
  if (!state->inInterrupt) {
    // not handling an interrupt, so let's check if we need to.
    if (!state->notINT && state->externalInterruptsEnabled) {
      // handle external interrupt
      push(state);
      state->PC = 3;
      state->inInterrupt = true;
    } else if (state->timerInterruptRequested && state->tcntInterruptsEnabled) {
      // handle timer interrupt
      state->timerInterruptRequested = false;
      push(state);
      state->PC = 7;
      state->inInterrupt = true;
    }
  }
}

// conditional jump
static inline void cjump(struct i8048_state_S *state, boolean condition) {    
  unsigned short addr = fetch(state) & 0xff;
  addr |= (state->PC-1) & 0xf00;
  if(condition) state->PC = addr; 
}

void i8048_step(struct i8048_state_S *state) {
  unsigned char op = fetch(state);

#if 0
  // 14%: 0x77
  // 7%: 0xfc, 0xfd, 0x7f, 0xac, 0xad, 0x6e, 0x43
  
  // generate some statistics to help optimizing the emulation
  static int op_cnt[256];
  static int op_cnt_init = 0;

  if(!op_cnt_init)
    memset(op_cnt,0, sizeof(op_cnt));

  op_cnt[op]++;
  op_cnt_init++;

  if(op_cnt_init == 1000000)
    for(int i=0;i<256;i++)
      if(op_cnt[i])
	printf("%02x: %.2f %%\n", i, 100.0 * op_cnt[i] / 1000000);
#endif
  
  switch(op) {
  case 0x00: // NOP
    break;
    
    /* OUTL BUS, A, 0x02. not implemented */

  case 0x03:  // ADD A, #data
    addToAcc(state, fetch(state));
    break;
      
  case 0x04: case 0x24: case 0x44: case 0x64: case 0x84: case 0xa4: case 0xc4: case 0xe4: // JMP addr
    state->PC = (state->DBF?(1 << 11):0) | ((op & 0xe0) << 3) | fetch(state);
    break;
      
  case 0x05: // EN I
    state->externalInterruptsEnabled = true;
    break;
    
  case 0x07:  // DEC A
    state->A = (state->A - 1) & 0xff;
    break;
    
    /* INS A, BUS, 0x08. not implemented */

  case 0x09: case 0x0a: // IN A, Pp
    tick(state);
    state->A = i8048_port_read(state, op & 0x3);   // FIX
    break;

    /* MOVD A, Pp, 0x0c, 0x0d, 0x0e, 0x0f not implemented */
    
  case 0x10: case 0x11:  // INC @Rr
    ram_write(state, readReg(state, op & 0x1), ram_read(state, readReg(state, op & 0x1)) + 1);
    break;
    
  case 0x12: case 0x32: case 0x52: case 0x72: case 0x92: case 0xb2: case 0xd2: case 0xf2: // JBb addr
    cjump(state, (state->A & 1<<((op >> 5) & 0x7)) > 0); 
    break;
    
  case 0x13: // ADDC A, #data
    addToAcc(state, GET_BIT(state->PSW, CY_BIT) + fetch(state));
    break;

  case 0x14: case 0x34: case 0x54: case 0x74: case 0x94: case 0xb4: case 0xd4: case 0xf4: {
    // CALL addr
    unsigned short addr = (state->DBF?(1<<11):0) | ((op & 0xe0) << 3) | fetch(state);
    push(state);
    state->PC = addr;
  } break;
    
  case 0x15:  // DIS I
    state->externalInterruptsEnabled = false;
    break;
    
  case 0x16: { // JTF addr
    // cannot use cjump as it includes a call to tick()
    // which in turn affects the state->TF condition
    unsigned short addr = fetch(state);
    addr |= ( (state->PC-1) & 0xf00);
    if (state->TF) {
      state->TF = false;
      state->PC = addr;
    }
  } break;
    
  case 0x17:  // INC A
    state->A++;
    break;
    
  case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // INC Rr
    writeReg(state, op & 0x7, readReg(state, op & 0x7) + 1);
    break;
    
  case 0x20: case 0x21: { // XCH A, @Rr
    unsigned char pos = readReg(state, op & 0x1);
    unsigned char tmp = state->A;
    state->A = ram_read(state, pos);
    ram_write(state, pos, tmp);
  } break;
    
  case 0x23: // MOV A, #data
    state->A = fetch(state);
    break;
    
  case 0x25:  // EN TCNTI
    state->tcntInterruptsEnabled = true;
    break;
    
  case 0x26:  // JNT0 addr
    cjump(state, !state->T0);
    break;
    
  case 0x27: // CLR A
    state->A = 0;
    break;
    
  case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: {
    // XCH A, Rr
    unsigned char tmp = readReg(state, op & 0x7);
    writeReg(state, op & 0x7, state->A);
    state->A = tmp;
  } break;
    
  case 0x30: case 0x31: { // XCHD A, @R
    unsigned char pos = readReg(state, op & 0x1);
    unsigned char tmp = ram_read(state, pos) & 0xf;
    ram_write(state, pos, ram_read(state, pos) & 0xf0 | state->A & 0x0f);
    state->A = state->A & 0xf0 | tmp;
  } break;

  case 0x35:  // DIS TCNTI
    state->tcntInterruptsEnabled = false;
    state->timerInterruptRequested = false;
    break;

  case 0x36:  // JT0 addr
    cjump(state, state->T0);
    break;

  case 0x37:  // CPL A
    state->A = ~state->A;
    break;
      
  case 0x39: case 0x3a: // OUTL Pp, A
    tick(state);
    i8048_port_write(state, op & 0x3, state->A); // FIX: & 0x01
    break;

    /* MOVD 0x3c, 0x3d, 0x3e, 0x3f not implemented */
    
  case 0x40: case 0x41: // ORL A, @Rr
    state->A |= ram_read(state, readReg(state, op & 0x1));
    break;
    
  case 0x42:  // MOV A, T
    state->A = state->T;
    break;
    
  case 0x43:  // ORL A, #data
    state->A |= fetch(state);
    break;
    
  case 0x45:  // STRT CNT
    state->counterRunning = true;
    state->timerRunning = false;
    break;
    
  case 0x46:  // JNT1 addr
    cjump(state, !state->T1);
    break;
    
  case 0x47:  // SWAP A
    state->A = ((state->A & 0x0f) << 4) | ((state->A & 0xf0) >> 4);
    break;
    
  case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // ORL A, Rr
    state->A |= readReg(state, op & 0x7);
    break;
    
  case 0x50: case 0x51: // ANL A, @Rr
    state->A &= ram_read(state, readReg(state, op & 0x1));
    break;
    
  case 0x53:  // ANL A, #data
    state->A &= fetch(state);
    break;
    
  case 0x55:  // STRT T
    state->counterRunning = false;
    state->timerRunning = true;
    state->cyclesUntilCount = 32;
    break;
    
  case 0x56:   // JT1 addr
    cjump(state, state->T1);
    break;
    
  case 0x57: { // DA A
    if ( (state->A & 0x0f) > 9 || GET_BIT(state->PSW, AC_BIT)) 
      state->A += 9;
    
    int hiNibble = (state->A & 0xf0) >> 4;
    if (hiNibble > 9 || GET_BIT(state->PSW, CY_BIT)) 
      hiNibble += 6;
    
    state->A = (hiNibble << 4 | (state->A & 0xf)) &0xff;
    setCarry(state, hiNibble > 15);
  } break;
    
  case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // ANL A, Rr
    state->A &= readReg(state, op & 0x7);
    break;  // FIX: This was missing!!
    
  case 0x60: case 0x61:  // ADD A, @Rr
    addToAcc(state, ram_read(state, readReg(state, op & 0x1)));
    break;
    
  case 0x62:  // MOV T, A
    state->T = state->A;
    break;
    
  case 0x65: { // STOP TCNT
    state->timerRunning = false;
    state->counterRunning = false;
  } break;
    
  case 0x67: { // RRC A
    boolean newCarry = state->A & 1;
    state->A = SET_BIT_TO(state->A >> 1, 7, GET_BIT(state->PSW, CY_BIT));
    setCarry(state, newCarry);
  } break;
    
  case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // ADD A, Rr
    addToAcc(state, readReg(state, op & 0x7));
    break;
    
  case 0x70: case 0x71:  // ADDC A, @Rr
    addToAcc(state, GET_BIT(state->PSW, CY_BIT) + ram_read(state, readReg(state, op & 0x1)));
    break;

    /* ENT0 CLK, 0x75 not implemented */
    
  case 0x76: // JF1 addr
    cjump(state, state->F1);
    break;
    
  case 0x77:  // RR A
    // this is the most executed instruction in Donkey Kong 

    // state->A = SET_BIT_TO(state->A>>1, 7, state->A&1);

    // optimzed to:
    if(state->A & 1) state->A = (state->A >> 1) | 0x80;
    else             state->A >>= 1;
    
    break;
    
  case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
    // ADDC A, Rr
    addToAcc(state, GET_BIT(state->PSW, CY_BIT) + readReg(state, op & 0x7));
    break;
      
  case 0x80: case 0x81:  // MOVX A, @Rr
    state->A = i8048_xdm_read(state, readReg(state, op & 0x1));
    tick(state);
    break;
    
  case 0x83: // RET
    tick(state);
    pop(state, false);
    break;
    
  case 0x85: // CLR F0
    state->PSW = CLR_BIT(state->PSW, F0_BIT);
    break;
      
  case 0x86: // JNI addr
    cjump(state, state->notINT);
    break;
    
    /* ORL BUS, #data, 0x88. not implemented */
    
  case 0x89: case 0x8a:  // ORL Pp, #data
    i8048_port_write(state, op & 0x3, (i8048_port_read(state, op & 0x3) | fetch(state)) & 0xff);
    break;
    
    /* ORLD Pp, A, 0x8c, 0x8d, 0x8e, 0x8f not implemented */

  case 0x90: case 0x91: // MOVX @R, A
    i8048_xdm_write(state, readReg(state, op & 0x1), state->A);
    tick(state);
    break;
    
  case 0x93:  // RETR
    tick(state);
    pop(state, true);
    break;
    
  case 0x95:  // CPL F0
    state->PSW = SET_BIT_TO(state->PSW, F0_BIT, !GET_BIT(state->PSW, F0_BIT));
    break;
    
  case 0x96: // JNZ addr
    cjump(state, state->A);
    break;
    
  case 0x97:  // CLR C
    state->PSW = CLR_BIT(state->PSW, CY_BIT);
    break;

    /* ANL BUS, #data, 0x98. not implemented */
    
  case 0x99: case 0x9a: // ANL Pp, #data
    i8048_port_write(state, op & 0x3, i8048_port_read(state, op & 0x3) & fetch(state));
    break;

    /* ANLD Pp, A, 0x9c, 0x9d, 0x9e, 0x9f not implemented */
    
  case 0xa0: case 0xa1:  // MOV @Rr, A
    ram_write(state, readReg(state, op & 0x1), state->A);
    break;
    
  case 0xa3: // MOVP A, @A
    tick(state);
    state->A = i8048_rom_read(state, (state->PC & 0xf00) | (state->A & 0xff));
    break;
    
  case 0xa5:   // CLR F1
    state->F1 = 0;      
    break;
    
  case 0xa7:  // CPL C
    state->PSW = SET_BIT_TO(state->PSW, CY_BIT, !GET_BIT(state->PSW, CY_BIT));
    break;
    
  case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf: // MOV Rr, A
    writeReg(state, op & 0x7, state->A);
    break;
    
  case 0xb0: case 0xb1: // MOV @Rr, #data
    ram_write(state, readReg(state, op & 0x1), fetch(state));
    break;
    
  case 0xb3:  // JMPP @A
    tick(state);
    state->PC = (state->PC & 0xff00 ) | i8048_rom_read(state, (state->PC & 0xff00) | (state->A & 0xff));
    break;
    
  case 0xb5:  // CPL F1
    state->F1 = !state->F1;
    break;
    
  case 0xb6:  // JF0 addr
    cjump(state, GET_BIT(state->PSW, F0_BIT));
    break;
    
  case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf: 
    // MOV Rr, #data
    writeReg(state, op & 0x7, fetch(state));
    break;
    
  case 0xc5:  // SEL RB0
    state->PSW = CLR_BIT(state->PSW, BS_BIT);
    break;
    
  case 0xc6: // JZ addr
    cjump(state, !state->A);
    break;
    
  case 0xc7:  // MOV A, PSW
    state->A = state->PSW;
    break;
    
  case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf: // DEC Rr
    writeReg(state, op & 0x7, readReg(state, op & 0x7) - 1);
    break;
    
  case 0xd0: case 0xd1:  // XRL A, @Rr
    state->A = state->A ^ ram_read(state, readReg(state, op & 0x1));
    break;
    
  case 0xd3: // XRL A, #data
    state->A ^= fetch(state);
    break;
    
  case 0xd5: // SEL RB1
    state->PSW = SET_BIT(state->PSW, BS_BIT);
    break;
    
  case 0xd7:  // MOV PSW, A
    state->PSW = state->A;
    break;
    
  case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf: // XRL A, Rr
    state->A ^= readReg(state, op & 0x7);
    break;
    
  case 0xe3: { // MOVP3 A, @A
    tick(state);
    int pos = 0x300 | (state->A & 0xff);
    state->A = i8048_rom_read(state, pos);
  } break;
    
  case 0xe5:  // SEL MB0
    state->DBF = false;
    break;
    
  case 0xe6: // JNC addr
    cjump(state, !GET_BIT(state->PSW, CY_BIT));
    break;
    
  case 0xe7:  // RL A
    state->A = (state->A << 1) & 0xff | ((state->A & 0x80) >> 7);
    break;
    
  case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef: {
    // DJNZ Rr, addr
    unsigned char val = readReg(state, op & 0x7) - 1;
    writeReg(state, op & 0x7, val);
    cjump(state, val);
  } break;
    
  case 0xf0: case 0xf1: // MOV A, @Rr
    state->A = ram_read(state, readReg(state, op & 0x1));
    break;
    
  case 0xf5:  // SEL MB1
    state->DBF = true;
    break;
    
  case 0xf6: // JC addr
    cjump(state, GET_BIT(state->PSW, CY_BIT));
    break;
    
  case 0xf7: { // RLC A
    unsigned char newCarry = state->A & 0x80;
    state->A = (state->A << 1) & 0xff;
    if(GET_BIT(state->PSW, CY_BIT)) state->A |= 1;
    setCarry(state, newCarry > 0);
  } break;
    
  case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff: // MOV A, Rr
    state->A = readReg(state, op & 0x7);
    break;
    
  default:
    // printf("OP not implemented: %02x\n", op);
    // exit(-1);
    break;
  }
  handleInterrupts(state);
}

