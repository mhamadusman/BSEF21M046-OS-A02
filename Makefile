CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
SRC = src/ls.c
OUT = bin/ls

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)
