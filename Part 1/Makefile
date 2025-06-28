CC = gcc
CFLAGS = -Wall -Wextra -O2

OBJS = main.o p1_simulator.o inputs_part1.o

all: p1_sim.exe

p1_sim.exe: $(OBJS)
    $(CC) $(CFLAGS) -o p1_sim.exe $(OBJS)

main.o: main.c p1_simulator.h inputs_part1.h
    $(CC) $(CFLAGS) -c main.c

p1_simulator.o: p1_simulator.c p1_simulator.h inputs_part1.h
    $(CC) $(CFLAGS) -c p1_simulator.c

inputs_part1.o: inputs_part1.c inputs_part1.h
    $(CC) $(CFLAGS) -c inputs_part1.c

clean:
    rm -f *.o p1_sim.exe fifo*.out lru*.out

.PHONY: all clean