# all: 
# 	g++ main.cpp -o run -I raylib/src/ raylib/src/libraylib.a -lm 

# clean:
# 	rm -rf $(HOME)/.cache/Music-Player-SRC
# 	rm run

all:
	gcc music_player.c c_string.c list.c -o run -lraylib -lm -Wall
