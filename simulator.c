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

typedef struct entryStruct {
	int* data;
	int  tag;
	int  isValid;
	int  isDirty;
	int  lru;
} entrytype;

typedef struct cacheStruct {
	int          blkSize; // words per block
	int          numSets;
	int          numWays; // associativity
	int          numBlks; // (words/block) * ways * sets
	entrytype*** entries;
} cachetype;

enum action_type {cache_to_processor, processor_to_cache, memory_to_cache, cache_to_memory, cache_to_nowhere};

void printState( statetype* state );
int  runInstrs( statetype* state, cachetype* cache );
int  getCache( statetype* state, cachetype* cache, int addr );
int  evictBlock( statetype* state, cachetype* cache, int addr, int lruIndex );
void putCache( statetype* state, cachetype* cache, int addr, int newData );
void writeBack( statetype* state, cachetype* cache );
void print_action(int address, int size, enum action_type type);


int main( int argc, char** argv ) {
// GETOPT
	int        opt;
	char*      userFile = NULL;
	cachetype* cache = calloc(1, sizeof(cachetype)); // <frd>

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
	cache-> entries = malloc( cache-> numSets * sizeof(entrytype**) ); // <frd>

	for (int i = 0; i < cache-> numSets; ++i) {
		cache-> entries[i] = malloc( cache-> numWays * sizeof(entrytype*) ); // <frd>

		for (int j = 0; j < cache-> numWays; ++j) {
			cache-> entries[i][j] = malloc( sizeof(entrytype) ); // <frd>
			cache-> entries[i][j]-> data = malloc( cache-> blkSize * sizeof(int) );  // <frd>
			cache-> entries[i][j]-> isValid = 0;
			cache-> entries[i][j]-> isDirty = 0;
			cache-> entries[i][j]-> lru = 0;
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
	int numInstrs = runInstrs( state, cache );

// FREE CACHE ARRAYS
	for (int i = 0; i < cache-> numSets; ++i) {
		for (int j = 0; j < cache-> numWays; ++j) {
			free( cache-> entries[i][j]-> data );
			free( cache-> entries[i][j] );
		}
		free( cache-> entries[i] );
	}
	free( cache-> entries );
	free( cache );

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

	writeBack( state, cache );
	return numInstrs;
}

int getCache( statetype* state, cachetype* cache, int addr ) {
	// parse addr format [ tag | set | blk ]
	int blk = addr & (cache-> blkSize - 1);
	int set = (addr >> (int)log2( cache-> blkSize )) & (cache-> numSets - 1);
	int tag = addr >> (int)(log2( cache-> blkSize ) + log2( cache-> numSets ));
	int lruMax = 0;
	int lruIndex = 0;
	int cacheHit = 0;
	int result;

	for (int i = 0; i < cache-> numWays; ++i) {
		if (cache-> entries[set][i]-> isValid && cache-> entries[set][i]-> tag == tag) {
			cache-> entries[set][i]-> lru = 0;
			result = cache-> entries[set][i]-> data[blk];
			cacheHit = 1;
		}
		else {
			cache-> entries[set][i]-> lru += 1;
		}

		if (lruMax < cache-> entries[set][i]-> lru) {
			lruMax = cache-> entries[set][i]-> lru;
			lruIndex = i;
		}
	}

	result = (!cacheHit) ? evictBlock( state, cache, addr, lruIndex ) : result;

	int bgnAddr = (int)(floor( addr / cache-> blkSize ) * cache-> blkSize);
	print_action( bgnAddr, cache-> blkSize, cache_to_processor );

	return result;
}

void putCache( statetype* state, cachetype* cache, int addr, int newData ) {
	int blk = addr & (cache-> blkSize - 1);
	int set = (addr >> (int)log2( cache-> blkSize )) & (cache-> numSets - 1);
	int tag = addr >> (int)(log2( cache-> blkSize ) + log2( cache-> numSets ));
	int lruMax = 0;
	int lruIndex = 0;
	int cacheHit = 0;

	for (int i = 0; i < cache-> numWays; ++i) {
		if (cache-> entries[set][i]-> isValid && cache-> entries[set][i]-> tag == tag) {
			cache-> entries[set][i]-> lru = 0;
			cache-> entries[set][i]-> isDirty = 1;
			cache-> entries[set][i]-> data[blk] = newData;
			cacheHit = 1;
		}
		else {
			cache-> entries[set][i]-> lru += 1;
		}

		if (lruMax < cache-> entries[set][i]-> lru) {
			lruMax = cache-> entries[set][i]-> lru;
			lruIndex = i;
		}
	}

	if (!cacheHit) {
		evictBlock( state, cache, addr, lruIndex );
	}

	int bgnAddr = (int)(floor( addr / cache-> blkSize ) * cache-> blkSize);
	print_action( bgnAddr, cache-> blkSize, processor_to_cache );
}

int evictBlock( statetype* state, cachetype* cache, int addr, int lruIndex ) {
	int blk = addr & (cache-> blkSize - 1);
	int set = (addr >> (int)log2( cache-> blkSize )) & (cache-> numSets - 1);
	int tag = addr >> (int)(log2( cache-> blkSize ) + log2( cache-> numSets ));
	// combine into one variable?
	int bgnAddr = (int)(floor( addr / cache-> blkSize ) * cache-> blkSize);

	if (cache-> entries[set][lruIndex]-> isDirty) {
		int stAddr = cache-> entries[set][lruIndex]-> tag;
		    stAddr = stAddr << (int)log2( cache-> numSets );
		    stAddr = stAddr |	set;
		    stAddr = stAddr << (int)log2( cache-> blkSize );
		    //stAddr = stAddr |	blk;
		    //stAddr = (int)(floor( stAddr / cache-> blkSize ) * cache-> blkSize);

		for (int i = 0; i < cache-> blkSize; ++i) {
			state-> mem[stAddr + i] = cache-> entries[set][lruIndex]-> data[i];
		}

		print_action( bgnAddr, cache-> blkSize, cache_to_memory );
	}
	else {
		print_action( bgnAddr, cache-> blkSize, cache_to_nowhere );
	}

	cache-> entries[set][lruIndex]-> isValid = 1;
	cache-> entries[set][lruIndex]-> isDirty = 0;
	cache-> entries[set][lruIndex]-> tag = tag;
	cache-> entries[set][lruIndex]-> lru = 0;
	for (int i = 0; i < cache-> blkSize; ++i) {
		cache-> entries[set][lruIndex]-> data[i] = state-> mem[bgnAddr + i];
	}

	print_action( bgnAddr, cache-> blkSize, memory_to_cache );

	return cache-> entries[set][lruIndex]-> data[blk];
}

//do we print for transfers on writeback???
void writeBack( statetype* state, cachetype* cache ) {
	for (int i = 0; i < cache-> numSets; ++i) {
		for (int j = 0; j < cache-> numWays; ++j) {
			if (cache-> entries[i][j]-> isValid) {
				if (cache-> entries[i][j]-> isDirty) {
					int stAddr = cache-> entries[i][j]-> tag;
							stAddr = stAddr << (int)log2( cache-> numSets );
							stAddr = stAddr |	i;
							stAddr = stAddr << (int)log2( cache-> blkSize );
							//stAddr = stAddr |	blk;
							//stAddr = (int)(floor( stAddr / cache-> blkSize ) * cache-> blkSize);

					for (int k = 0; k < cache-> blkSize; ++k) {
						state-> mem[stAddr + k] = cache-> entries[i][j]-> data[k];
					}
				}
				cache-> entries[i][j]-> isValid = 0;
			}
		}
	}
}

/*
	* Log the specifics of each cache action.
	*
	* address is the starting word address of the range of data being transferred.
	* size is the size of the range of data being transferred.
	* type specifies the source and destination of the data being transferred.
	*
	* cache_to_processor: reading data from the cache to the processor
	* processor_to_cache: writing data from the processor to the cache
	* memory_to_cache: reading data from the memory to the cache
	* cache_to_memory: evicting cache data by writing it to the memory
	* cache_to_nowhere: evicting cache data by throwing it away
*/
void print_action(int address, int size, enum action_type type) {
	printf("transferring word [%i-%i] ", address, address + size - 1);
	if (type == cache_to_processor) {
		printf("from the cache to the processor\n");
	} else if (type == processor_to_cache) {
		printf("from the processor to the cache\n");
	} else if (type == memory_to_cache) {
		printf("from the memory to the cache\n");
	} else if (type == cache_to_memory) {
		printf("from the cache to the memory\n");
	} else if (type == cache_to_nowhere) {
		printf("from the cache to nowhere\n");
	}
}
