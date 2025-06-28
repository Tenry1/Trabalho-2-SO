CC = gcc
CFLAGS = -Wall -Wextra -g

SRCS = main.c p1_simulator.c inputs_part1.c
OBJS = $(SRCS:.c=.o)
TARGET = sim.exe

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