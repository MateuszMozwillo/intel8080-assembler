SRC = $(wildcard src/*.c)

build:
	gcc -Wall $(SDL_CFLAGS) $(SRC) -o i8080_assembler $(SDL_LIBS)
