all: 
	g++ main.cpp -o run -I raylib/src/ raylib/src/libraylib.a -lm 

clean:
	rm -rf $(HOME)/.cache/Music-Player-SRC
	rm run

new:
	gcc music_player.c c_string.c list.c playlist.c -o run -I raylib/src/ raylib/src/libraylib.a -lm -Wall

newclean:
	rm -rf $(HOME)/.cache/mp3_covers
	rm run
	rm saved_music.txt