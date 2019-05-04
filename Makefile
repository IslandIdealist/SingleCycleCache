CC = gcc
CFLAGS = -pipe
LINKER = -lm
PRNAME = simulator

all: $(PRNAME).o
	$(CC) $(CFLAGS) $(PRNAME).o -o $(PRNAME) $(LINKER)

simulator.o: $(PRNAME).c
	$(CC) $(CFLAGS) -c $(PRNAME).c

debug: CFLAGS += -g
debug: all
	@echo DEBUG: gdb $(PRNAME)
	@echo MEM: valgrind --leak-check=full ./$(PRNAME) *args*

tags:
	ctags -R -f ./.tags
	cscope -bcqR -f ./.cscope.out

entr:
	(find *.c -type f | entr -s 'ctags -R -f ./.tags && cscope -bcqR -f ./.cscope.out' &) > /dev/null

leave:
	pkill entr

clean:
	rm $(PRNAME) *.o *.*~ *~ .tags .cscope*
