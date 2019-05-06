#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define NUMMEMORY 65536
#define NUMREGS 8

typedef struct stateStruct {
	int pc;
	int mem[NUMMEMORY];
	int reg[NUMREGS];
	int numMemory;
} statetype;

typedef struct cacheStruct {
	int   blkSize; // words per block
	int   numSets;
	int   numWays; // associativity
	int   numBlks; // (words/block) * ways * sets
	int** data;
	int** tag;
	int** isValid;
	int** isDirty;
} cachetype;

void printState( statetype* state );
int  runInstrs( statetype* state, cachetype* cache );
int  getCache( statetype* state, cachetype* cache, int addr );

int main( int argc, char** argv ) {
// GETOPT
	int        opt;
	char*      userFile = NULL;
	cachetype* cache = calloc(1, sizeof(cachetype));

	while ((opt = getopt( argc, argv, "f: b: s: a:" )) != -1) {
		switch (opt) {
			case 'f':
				userFile = strdup( optarg ); // <frd>
				break;
			case 'b':
				cache-> blkSize = atoi( optarg );
				break;
			case 's':
				cache-> numSets = atoi( optarg );
				break;
			case 'a':
				cache-> numWays = atoi( optarg );
				break;
			default:
				printf( "ERROR: Invalid command arguments\n" );
				exit( EXIT_FAILURE );
		}
	}

// GET CACHE INFO NOT PROVIDED
	if (userFile == NULL) {
		char* tempStr = malloc(256 * sizeof(char)); // <frd>

		printf("Enter the machine code program to simulate: ");
		fscanf(stdin, "%s", tempStr);
		userFile = strdup( tempStr ); // <frd>
		free( tempStr );
	}

	if (cache-> blkSize <= 0) {
		printf("Enter the block size of the cache (in words): ");
		fscanf(stdin, "%d", &cache-> blkSize);
	}

	if (cache-> numSets <= 0) {
		printf("Enter the number of sets in the cache: ");
		fscanf(stdin, "%d", &cache-> numSets);
	}

	if (cache-> numWays <= 0) {
		printf("Enter the associativity of the cache: ");
		fscanf(stdin, "%d", &cache-> numWays);
	}

	cache-> numBlks = cache-> numWays * cache-> numSets;

// CHECK CACHE FOR ERRORS
	int error = (cache-> blkSize <= 0) ? 1 : 0;
	    error = (cache-> numSets <= 0) ? 1 : error;
			error = (cache-> numWays <= 0) ? 1 : error;
	    error = (cache-> blkSize > 256) ? 1 : error;
			error = (cache-> numWays > cache-> numBlks) ? 1 : error;
	    error = (log2(cache-> blkSize) != floor(log2(cache-> blkSize))) ? 1 : 0;
	    error = (log2(cache-> numSets) != floor(log2(cache-> numSets))) ? 1 : 0;
	    error = (log2(cache-> numWays) != floor(log2(cache-> numWays))) ? 1 : 0;

	if (error) {
		printf("Improper cache settings given\n");
		exit(EXIT_FAILURE);
	}

// INITIALIZE CACHE DATA ARRAY
	for (int i = 0; i < cache-> numSets; ++i) {
		cache-> data[i] = malloc( cache-> numWays * sizeof(int) ); // <frd>
	}
	for (int i = 0; i < cache-> numSets; ++i) {
		cache-> tag[i] = malloc( cache-> numWays * sizeof(int) ); // <frd>
	}
	for (int i = 0; i < cache-> numSets; ++i) {
		cache-> isValid[i] = calloc( cache-> numWays, sizeof(int) ); // <frd>
	}
	for (int i = 0; i < cache-> numSets; ++i) {
		cache-> isDirty[i] = calloc( cache-> numWays, sizeof(int) ); // <frd>
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
	int numInstrs = runInstrs( state, cache );

// FREE CACHE ARRAYS
	for (int i = 0; i < cache-> numSets; ++i) {
		free( cache-> data[i] );
	}
	for (int i = 0; i < cache-> numSets; ++i) {
		free( cache-> tag[i] );
	}
	for (int i = 0; i < cache-> numSets; ++i) {
		free( cache-> isValid[i] );
	}
	for (int i = 0; i < cache-> numSets; ++i) {
		free( cache-> isDirty[i] );
	}

// PRINT FINAL STATE
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

int runInstrs( statetype* state, cachetype* cache ) {
	int curInstr;
	int immediate;
	int regDest;
	int regB;
	int regA;
	int opcode = (getCache( state, cache, state-> pc ) >> 22) & 7;
	int numInstrs = 1;

	while (opcode != 6) { // continue while instructions isn't halt
		curInstr   = getCache( state, cache, state-> pc );
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
			state-> reg[regDest] = state-> reg[regA] + state-> reg[regB];
		}
		else if (opcode == 1) { // nand
			state-> reg[regDest] = ~ (state-> reg[regA] & state-> reg[regB]);
		}
		else if (opcode == 2) { // lw
			int error = (state-> reg[regB] + immediate < 0)          ? 1 : error;
					error = (state-> reg[regB] + immediate >= NUMMEMORY) ? 1 : error;

			if (error) {
				printf( "ERROR: lw was given an improper reg or offset\n" );
				exit( EXIT_FAILURE );
			}

			state-> reg[regA] = getCache( state, cache, state-> reg[regB] + immediate );
		}
		else if (opcode == 3) { // sw
			int error = (state-> reg[regB] + immediate < 0)          ? 1 : error;
					error = (state-> reg[regB] + immediate >= NUMMEMORY) ? 1 : error;

			if (error) {
				printf( "ERROR: sw was given an improper reg or offset\n" );
				exit( EXIT_FAILURE );
			}

			//putCache( state, cache, state-> reg[regB] + immediate ) = state-> reg[regA];
			state-> mem[state-> reg[regB] + immediate] = state-> reg[regA];
		}
		else if (opcode == 4) { // beq
			if (state-> reg[regA] == state-> reg[regB] && (state-> pc + immediate < 0 || state-> pc + immediate >= NUMMEMORY)) {
				printf( "ERROR: beq was given an improper reg or offset\n" );
				exit( EXIT_FAILURE );
			}

			state-> pc += (state-> reg[regA] == state-> reg[regB]) ? immediate : 0;
		}
		else if (opcode == 5) { // jalr
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

int getCache( statetype* state, cachetype* cache, int addr ) {
	int blk = addr & (cache-> blkSize - 1);
	int set = (addr >> (int)log2( cache-> blkSize )) & (cache-> numSets - 1);
	int tag = addr >> (int)(log2( cache-> blkSize ) + log2( cache-> numSets ));

	if (cache-> isValid[set][blk] && cache-> tag[set][blk] == tag) {
		return cache-> data[set][blk];
	}
	else {
		cache-> data[set][blk] = state-> mem[addr];
		cache-> isValid[set][blk] = 1;
		cache-> tag[set][blk] = tag;
	}
}

int putCache( statetype* state, cachetype* cache, int addr, int data ) {
	int blk = addr & (cache-> blkSize - 1);
	int set = (addr >> (int)log2( cache-> blkSize )) & (cache-> numSets - 1);
	int tag = addr >> (int)(log2( cache-> blkSize ) + log2( cache-> numSets ));

	if (cache-> isValid[set][blk] && cache-> tag[set][blk] == tag) {
		cache-> data[set][blk] = data;
		cache-> isDirty[set][blk] = 1;
	}
	else {
		cache-> data[set][blk] = data;
		cache-> isValid[set][blk] = 1;
		cache-> tag[set][blk] = tag;
		cache-> isDirty[set][blk] = 1;
	}
}
