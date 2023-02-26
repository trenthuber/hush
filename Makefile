CC = cc
CFLAGS = -I./include -Wall -Wextra -pedantic
LDFLAGS = 

SRC = $(wildcard ./src/*.c)
OBJ = $(SRC:.c=.o)
BIN = ./bin

all: clean hush test

clean:
	rm -rf $(BIN) $(OBJ)

hush: $(OBJ)
	mkdir -p $(BIN)
	cc -o $(BIN)/hush $^ $(LDFLAGS)

test:
	$(BIN)/hush

%.o: %.c
	cc -o $@ -c $< $(CFLAGS)
