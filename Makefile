SRC = $(wildcard src/*.c)

run: build
	./i8080_assembler

build:
	gcc -Wall $(SDL_CFLAGS) $(SRC) -o i8080_assembler $(SDL_LIBS)
