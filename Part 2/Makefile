CC = gcc
CFLAGS = -Wall -Wextra -g

SRCS = main.c p2_simulator.c queue.c inputs_part2.c
OBJS = $(SRCS:.c=.o)
TARGET = p2_sim.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o $(TARGET)

.PHONY: all clean run