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
 * Memory mapped registers
 */
enum
{
	MR_KBSR = 0xFE00,	/* Keyboard status */
	MR_KBDR = 0xFE02	/* keyboard data */
};

/**
 * Set of available trap codes
 */
enum
{
	TRAP_GETC = 0x20,		/* Get character from keyboard, not echoed onto the terminal */
	TRAP_OUT = 0x21,		/* Output a character */
	TRAP_PUTS = 0x22,		/* Output a word string */
	TRAP_IN = 0x23,			/* Get character from keyboard, echoed onto the terminal */
	TRAP_PUTSP = 0x24,	/* Output a byte string */
	TRAP_HALT = 0x25		/* Halt the program */
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

/* Swap */
uint16_t swap16(uint16_t x)
{
	return (x << 8) | (x >> 8);
}

void read_image_file(FILE* file)
{
	/* Origin tells the location to store the image in memory */
	uint16_t origin;
	fread(&origin, sizeof(origin), 1, file);
	origin = swap16(origin);

	uint16_t max_read = UINT16_MAX - origin;
	uint16_t* p = memory + origin;
	size_t read = fread(p, sizeof(uint16_t), max_read, file);

	/* Swap to little endian */
	while (read-- > 0)
	{
		*p = swap16(*p);
		++p;
	}
}

int read_image(const char* image_path)
{
	FILE* file = fopen(image_path, "rb");
	if (!file) return 0;
	read_image_file(file);
	fclose(file);
	return 1;
}

uint16_t check_key()
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	return select(1, &readfds, NULL, NULL, &timeout) != 0;
}

/**
 * Procedure to write value into the memory
 */
void mem_write(uint16_t address, uint16_t val)
{
	memory[address] = val;
}

/**
 * Procedure to read value from memory and return it
 */
uint16_t mem_read(uint16_t address)
{
	if (address == MR_KBSR)
	{
		if (check_key())
		{
			memory[MR_KBSR] = (1 << 15);
			memory[MR_KBDR] = getchar();
		}
		else
		{
			memory[MR_KBSR] = 0;
		}
	}

	return memory[address];
}

/* Input buffering */
struct termios original_tio;

void disable_input_buffering()
{
	tcgetattr(STDIN_FILENO, &original_tio);
	struct termios new_tio = original_tio;
	new_tio.c_cflag &= ~ICANON & ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

void handle_interrupt(int signal)
{
	restore_input_buffering();
	printf("\n");
	exit(-2);
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
		if (!read_image(argv[j]))
		{
			printf("Failed to load image: %s\n", argv[j]);
			exit(1);
		}
	}


	/**
	 * Initial setup code
	 */
	signal(SIGINT, handle_interrupt);
	disable_input_buffering();


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
				switch (INSTR & 0xFF)
				{
					case TRAP_GETC:
						/* Read a single ASCII character */
						REGISTER[R_R0] = (uint16_t)getchar();
						break;
					case TRAP_OUT:
						/* Put a single ASCII character to standard output */
						putc((char)REGISTER[R_R0], stdout);
						fflush(stdout);
						break;
					case TRAP_PUTS:
						{
							uint16_t* c = memory + REGISTER[R_R0];
							while (*c)
							{
								putc((char)*c, stdout);
								++c;
							}
							fflush(stdout);
						}

						break;
					case TRAP_IN:
						{
							printf("Enter a character: ");
							char c = getchar();
							putc(c, stdout);
							REGISTER[R_R0] = (uint16_t)c;
						}

						break;
					case TRAP_PUTSP:
						{
							/**
							 * One character per byte(two bytes per word)
							 * Here we need to swap back to big endian format
							 */
							uint16_t* c = memory + REGISTER[R_R0];
							while (*c)
							{
								char char1 = (*c) & 0xFF;
								putc(char1, stdout);
								char char2 = (*c) >> 8;
								if (char2) putc(char2, stdout);
								++c;
							}
							fflush(stdout);
						}

						break;
					case TRAP_HALT:
						puts("HALT");
						fflush(stdout);
						running = 0;
						break;
				}

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
	restore_input_buffering();
}