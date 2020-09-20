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

/**
 * Applying the necessary padding to the number
 * For negative sign the padding of 1 should be applied for maintaining the 2's complement
 */
uint16_t sign_extend(uint16_t x, int bit_count)
{
	if ((x >> (bit_count - 1)) & 1)
	{
		x |= (0xFFFF << bit_count);
	}
	return x;
}

void update_flags(uint16_t r) {
	if (REGISTER[r] == 0)
	{
		REGISTER[R_COND] = FL_ZRO;
	}
	else if (REGISTER[r] >> 15) /* If the value has the leftmost bit set, indicating it is negative */
	{
		REGISTER[R_COND] = FL_NEG;
	}
	else
	{
		REGISTER[R_COND] = FL_POS;
	}

}

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
				{
					/* Destination Register */
					uint16_t r0 = (INSTR >> 9) & 0x7;
					/* First operand */
					uint16_t r1 = (INSTR >> 6) & 0x7;
					/* Flag to check if current operation is in immediate mode */
					uint16_t IMM_FLAG = (INSTR >> 5) & 0x1;

					if (IMM_FLAG)
					{
						uint16_t imm_5 = sign_extend(INSTR & 0x1F, 5);
						REGISTER[r0] = REGISTER[r1] + imm_5;
					}
					else
					{
						uint16_t R2 = INSTR & 0x7;
						REGISTER[r0] = REGISTER[r1] + REGISTER[R2];
					}

					update_flags(r0);
				}

				break;
			case OP_AND:
				{
					/* Destination Register */
					uint16_t r0 = (INSTR >> 9) & 0x7;
					/* First operand */
					uint16_t r1 = (INSTR >> 6) & 0x7;
					/* Flag to check if current operation is in immediate mode */
					uint16_t IMM_FLAG = (INSTR >> 5) & 0x1;

					if (IMM_FLAG)
					{
						uint16_t imm_5 = sign_extend(INSTR & 0x1F, 5);
						REGISTER[r0] = REGISTER[r1] & imm_5;
					}
					else
					{
						uint16_t r2 = INSTR & 0x7;
						REGISTER[r0] = REGISTER[r1] & REGISTER[r2];
					}
					update_flags(r0);
				}

				break;
			case OP_NOT:
				{
					/* Destination Register */
					uint16_t r0 = (INSTR >> 9) & 0x7;
					/* Operand */
					uint16_t r1 = (INSTR >> 6) & 0x7;

					REGISTER[r0] = ~REGISTER[r1];
					update_flags(r0);
				}

				break;
			case OP_BR:
				{
					uint16_t pc_offset = sign_extend(INSTR & 0x1FF, 9);
					uint16_t cond_flag = (INSTR >> 9) & 0x7;

					if (cond_flag & REGISTER[R_COND])
					{
						REGISTER[R_PC] += pc_offset;
					}
				}

				break;
			case OP_JMP:
				{
					uint16_t r1 = (INSTR >> 6) & 0x7;
					REGISTER[R_PC] = REGISTER[r1];
				}

				break;
			case OP_JSR:
				{
					uint16_t long_flag = (INSTR >> 11) & 1;
					REGISTER[R_R7] = REGISTER[R_PC];
					if (long_flag)
					{
						uint16_t long_pc_offset = sign_extend(INSTR & 0x7FF, 11);
						REGISTER[R_PC] += long_pc_offset;
					}
					else
					{
						uint16_t r1 = (INSTR >> 6) & 0x7;
						REGISTER[R_PC] = REGISTER[r1];
					}
					break;
				}

				break;
			case OP_LD:
				{
					uint16_t r0 = (INSTR >> 9) & 0x7;
					uint16_t pc_offset = sign_extend(INSTR & 0x1FF, 9);
					REGISTER[r0] = mem_read(REGISTER[R_PC] + pc_offset);
					update_flags(r0);
				}
				break;
			case OP_LDI:
				{
					/* Destination register */
					uint16_t r0 = (INSTR >> 9) & 0x7;
					/* PC offset */
					uint16_t pc_offset = sign_extend(INSTR & 0x1FF, 9);
					/* Add offset to the current PC, and look at the final memory address obtained */
					REGISTER[r0] = mem_read(mem_read(REGISTER[R_PC] + pc_offset));
					update_flags(r0);
				}

				break;
			case OP_LDR:
				{
					uint16_t r0 = (INSTR >> 9) & 0x7;
					uint16_t r1 = (INSTR >> 6) & 0x7;
					uint16_t offset = sign_extend(INSTR & 0x3F, 6);
					REGISTER[r0] = mem_read(REGISTER[r1] + offset);
					update_flags(r0);
				}

				break;
			case OP_LEA:
				{
					uint16_t r0 = (INSTR >> 9) & 0x7;
					uint16_t pc_offset = sign_extend(INSTR & 0x1FF, 9);
					REGISTER[r0] = REGISTER[R_PC] + pc_offset;
					update_flags(r0);
				}

				break;
			case OP_ST:
				{
					uint16_t r0 = (INSTR >> 9) & 0x7;
					uint16_t pc_offset = sign_extend(INSTR & 0x1FF, 9);
					mem_write(REGISTER[R_PC] + pc_offset, REGISTER[r0]);
				}

				break;
			case OP_STI:
				{
					uint16_t r0 = (INSTR >> 9) & 0x7;
					uint16_t r1 = (INSTR >> 6) & 0x7;
					uint16_t offset = sign_extend(INSTR & 0x3F, 6);
					mem_write(REGISTER[r1] + offset, REGISTER[r0]);
				}

				break;
			case OP_STR:
				{
					uint16_t r0 = (INSTR >> 9) & 0x7;
					uint16_t r1 = (INSTR >> 6) & 0x7;
					uint16_t offset = sign_extend(INSTR & 0x3F, 6);
					mem_write(REGISTER[r1] + offset, REGISTER[r0]);
				}

				break;
			case OP_TRAP:
				/**
				 * Operation yet to implement
				 */
				break;
			case OP_RES:
			case OP_RTI:
			default:
				/* Throw error for bad opcode */
				abort();
				break;
		}
	}

	/**
	 * Perform shutdown operation
	 */
}