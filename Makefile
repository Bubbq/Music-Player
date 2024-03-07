all: main.c
	gcc main.c -o run -lraylib -lm -Werror -Wall -std=c11 -pedantic
clean:
	rm run
