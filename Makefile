all: main.cpp
	g++ main.cpp -o run -lraylib -lm -Werror -Wall -pedantic
clean:
	rm run && rm playlistFiles.txt && rm savedMusic.txt
