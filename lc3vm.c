#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

/**
 * 65536 memory locations
 * Making 128 kb memory available to the VM
 */
uint16_t memory[UINT16_MAX];

/**
 * Initialization of registers
 * 8 general purpose (R0-R7)
 * 1 Program counter
 * 1 Condition flag
 */
enum
{
	R_R0 = 0,
	R_R1,
	R_R2,
	R_R3,
	R_R4,
	R_R5,
	R_R6,
	R_R7,
	R_PC, // Program counter
	R_COND, // Condition flag
	R_COUNT // To store the count of registers
};

/**
 * Array to access the registers
 */
uint16_t REGISTER[R_COUNT];

/**
 * Set of opcodes that can be used in the VM
 */
enum
{
	OP_BR = 0,	/* BRANCH */
	OP_ADD,			/* ADD */
	OP_LD,			/* LOAD */
	OP_ST,			/* STORE */
	OP_JSR,			/* JUMP REGISTER */
	OP_AND,			/* BITWISE AND */
	OP_LDR,			/* LOAD REGISTER */
	OP_STR,			/* STORE REGISTER */
	OP_RTI,			/* UNUSED */
	OP_NOT,			/* BITWISE NOT */
	OP_LDI,			/* LOAD INDIRECT */
	OP_STI,			/* STORE INDIRECT */
	OP_JMP,			/* JUMP */
	OP_RES,			/* RESERVED(UNUSED) */
	OP_LEA,			/* LOAD EFFECTIVE ADDRESS */
	OP_TRAP			/* EXECUTE TRAP */
};

/**
 * Set of available condition flags
 * LC3 supports only 3 types of condition flags (positive, negative, and zero)
 */
enum
{
	FL_POS = 1 << 0,	/* P */
	FL_ZRO = 1 << 1,	/* Z */
	FL_NEG = 1 << 2		/* N */
};

