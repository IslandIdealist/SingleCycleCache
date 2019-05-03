#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define NUMMEMORY 65536
#define NUMREGS 8

typedef struct stateStruct {
	int pc;
	int mem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
} statetype;

void printState( statetype* state );
int  runInstrs( statetype* state );

int main( int argc, char** argv ) {
// GETOPT
	int   opt;
	char* userFile;

	while ((opt = getopt( argc, argv, "i:" )) != -1) {
		switch (opt) {
			case 'i':
				userFile = strdup( optarg ); // <frd>
				break;
			default:
				printf( "ERROR: Invalid command arguments\n" );
				exit( EXIT_FAILURE );
		}
	}


// OPEN FILE AND SAVE INTO MEMORY
	FILE*      openFile = fopen( userFile, "r" );
	char*      curStr = malloc( 12 * sizeof(char) ); // <frd>
	statetype* state  = calloc( 1, sizeof(statetype) ); // <frd>

	free( userFile );

	if (openFile == NULL) {
		printf( "ERROR: Problem opening input file\n" );
		exit( EXIT_FAILURE );
	}

	while (fgets( curStr, 11, openFile ) != NULL) {
		state -> mem[state-> numMemory++] = atoi( curStr );
	}

	free( curStr );
	fclose( openFile );


// PERFORM INSTRUCTIONS AND PRINT STATE
	int numInstrs = runInstrs( state );

	printState( state );
	free( state );
	printf( "\nINSTRUCTIONS: %d\n", numInstrs );
	exit( EXIT_SUCCESS );
}


void printState( statetype* state ) {
	printf( "\n@@@\nstate:\n" );
	printf( "\tpc %d\n", state -> pc );
	printf( "\tmemory:\n" );

	for (int i = 0; i < state -> numMemory; i++) {
		printf( "\t\tmem[%d]=%d\n", i, state -> mem[i] );
	}
	printf( "\tregisters:\n" );
	for (int i = 0; i < NUMREGS; i++) {
		printf( "\t\treg[%d]=%d\n", i, state -> reg[i] );
	}
	printf( "end state\n" );
}

int runInstrs( statetype* state ) {
	int curInstr;
	int immediate;
	int regDest;
	int regB;
	int regA;
	int opcode = (state-> mem[state-> pc] >> 22) & 7;
	int numInstrs = 1;

	while (opcode != 6) { // continue while instructions isn't halt
		curInstr   = state-> mem[state-> pc];
		immediate  = curInstr & 0xffff; // 65535
		immediate -= (immediate & (1 << 15)) ? (1 << 16) : 0;
		regDest    = curInstr & 7;
		regB       = (curInstr >> 16) & 7;
		regA       = (curInstr >> 19) & 7;
		opcode     = (curInstr >> 22) & 7;

		printState( state );
		state-> pc++;
		numInstrs += (opcode == 6) ? 0 : 1;

		if (opcode == 0) { // add
			if (regDest < 1 || regDest > 7 || regA < 0 || regA > 7 || regB < 0 || regB > 7) {
				printf( "ERROR: add was given an improper register\n" );
				exit( EXIT_FAILURE );
			}

			state-> reg[regDest] = state-> reg[regA] + state-> reg[regB];
		}
		else if (opcode == 1) { // nand
			if (regDest < 1 || regDest > 7 || regA < 0 || regA > 7 || regB < 0 || regB > 7) {
				printf( "ERROR: nand was given an improper register\n" );
				exit( EXIT_FAILURE );
			}

			state-> reg[regDest] = ~ (state-> reg[regA] & state-> reg[regB]);
		}
		else if (opcode == 2) { // lw
			int error = (regA < 1 || regA > 7)                       ? 1 : 0;
		      error = (regB < 0 || regB > 7)                       ? 1 : error;
					error = (state-> reg[regB] + immediate < 0)          ? 1 : error;
					error = (state-> reg[regB] + immediate >= NUMMEMORY) ? 1 : error;

			if (error) {
				printf( "ERROR: lw was given an improper reg or offset\n" );
				exit( EXIT_FAILURE );
			}

			state-> reg[regA] = state-> mem[state-> reg[regB] + immediate];
		}
		else if (opcode == 3) { // sw
			int error = (regA < 1 || regA > 7)                       ? 1 : 0;
		      error = (regB < 0 || regB > 7)                       ? 1 : error;
					error = (state-> reg[regB] + immediate < 0)          ? 1 : error;
					error = (state-> reg[regB] + immediate >= NUMMEMORY) ? 1 : error;

			if (error) {
				printf( "ERROR: sw was given an improper reg or offset\n" );
				exit( EXIT_FAILURE );
			}

			state-> mem[state-> reg[regB] + immediate] = state-> reg[regA];
		}
		else if (opcode == 4) { // beq
			int error = (regA < 0 || regA > 7) ? 1 : 0;
		      error = (regB < 0 || regB > 7) ? 1 : error;

			if (error || (state-> reg[regA] == state-> reg[regB] && (state-> pc + immediate < 0 || state-> pc + immediate >= NUMMEMORY))) {
				printf( "ERROR: beq was given an improper reg or offset\n" );
				exit( EXIT_FAILURE );
			}

			state-> pc += (state-> reg[regA] == state-> reg[regB]) ? immediate : 0;
		}
		else if (opcode == 5) { // jalr
			if (regA < 1 || regA > 7 || regB < 0 || regB > 7) {
				printf( "ERROR: jalr was given an improper register\n" );
				exit( EXIT_FAILURE );
			}

			state-> reg[regA] = state-> pc;
			state-> pc = state-> reg[regB];
		}
		else if (opcode == 7) { // noop
			// nothing
		}
		else if (opcode > 7) {
			printf( "ERROR: improper opcode was given\n" );
			exit( EXIT_FAILURE );
		}
	}

	return numInstrs;
}
