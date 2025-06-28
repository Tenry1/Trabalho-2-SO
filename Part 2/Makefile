CC = gcc
CFLAGS = -Wall -Wextra -O2

OBJS = main.o p2_simulator.o queue.o inputs_part2.o

all: p2_sim.exe

p2_sim.exe: $(OBJS)
    $(CC) $(CFLAGS) -o p2_sim.exe $(OBJS)

main.o: main.c p2_simulator.h inputs_part2.h
    $(CC) $(CFLAGS) -c main.c

p2_simulator.o: p2_simulator.c p2_simulator.h queue.h
    $(CC) $(CFLAGS) -c p2_simulator.c

queue.o: queue.c queue.h
    $(CC) $(CFLAGS) -c queue.c

inputs_part2.o: inputs_part2.c inputs_part2.h
    $(CC) $(CFLAGS) -c inputs_part2.c

clean:
    rm -f *.o p2_sim.exe output2T*.out

.PHONY: all clean