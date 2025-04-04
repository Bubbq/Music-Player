# all: 
# ifeq (,$(wildcard raylib/src/libraylib.a))
# 	make -C raylib/src/
# endif
# 	g++ main.cpp -o run -I raylib/src/ raylib/src/libraylib.a -lm 

# clean:
# 	rm -rf $(HOME)/.cache/Music-Player-SRC
# 	rm run

all:
	gcc music_player.c -o run -lraylib -lm -Wall
