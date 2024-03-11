all: 
	g++ main.cpp -o run -I ./raylib/src/ ./raylib/src/libraylib.a -lm -pedantic
clean:
	rm run && rm savedMusic.txt && rm covers/*
