OBJS = $(wildcard src/*.c)

CC = clang 

CFLAGS = -Wall

LDFLAGS = -lSDL2

TARGET = chip8

all : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)
