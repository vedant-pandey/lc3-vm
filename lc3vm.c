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

int main(int argc, const char* argv[])
{
	/**
	 * Load arguments
	 */
	if (argc < 2)
	{
		/**
		 * Prompt correct usage
		 */
		printf("lc3 [image-file1] ...\n");
		exit(2);
	}

	for (int j = 0; j < argc; ++j)
	{
		if (!read_image[argv[j]])
		{
			printf("Failed to load image: %s\n", argv[j]);
			exit(1);
		}
	}


	/**
	 * Initial setup code
	 */
	// Yet to implement


	/**
	 * Initialize PC to the starting position
	 * Default is 0x3000
	 */
	enum { PC_START = 0x3000 };
	REGISTER[R_PC] = PC_START;

	/**
	 * Flag to check for the running status
	 */
	int running = 1;

	while (running)
	{
		/**
		 * Fetch the current instruction and the opcode
		 */
		uint16_t INSTR = mem_read(REGISTER[R_PC]++);
		uint16_t OP = INSTR >> 12;

		switch (OP)
		{
			case OP_ADD:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_AND:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_NOT:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_BR:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_JMP:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_JSR:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_LD:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_LDI:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_LDR:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_LEA:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_ST:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_STI:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_STR:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_TRAP:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_RES:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_RTI:
				/**
				 * Operation yet to implement
				 */
				break;
			default:
				/**
				 * Throw error for bad opcode
				 */
				break;
		}
	}

	/**
	 * Perform shutdown operation
	 */
}