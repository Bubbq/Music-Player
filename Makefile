all:
	gcc music_player.c file_management.c stream_management.c -o music_player -I raylib/src/ raylib/src/libraylib.a -lavformat -lavutil -lm -Wall

clean:
	rm -rf $(HOME)/.cache/mp3_covers
	rm music_player
	rm last_song.txt