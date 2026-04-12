CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_FORTIFY_SOURCE=2
LDFLAGS = -ldl
SRC = src/main.c src/lexer.c src/parser.c src/interpreter.c
OUT = flux

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRC) | src
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

src:
	mkdir -p src

clean:
	rm -f $(OUT)
