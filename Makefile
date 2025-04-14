# all: 
# 	g++ main.cpp -o run -I raylib/src/ raylib/src/libraylib.a -lm 

# clean:
# 	rm -rf $(HOME)/.cache/Music-Player-SRC
# 	rm run

all:
	gcc music_player.c -o run -I raylib/src/ raylib/src/libraylib.a -lm -Wall

clean:
	rm -rf $(HOME)/.cache/mp3_covers
	rm run
	rm saved_music.txt