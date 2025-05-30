all:
	gcc music_player.c  file_management.c stream_management.c file_watch.c -o music_player -I raylib/src/ raylib/src/libraylib.a -lavformat -lavutil -lm -Wall
clean:
	rm -rf $(HOME)/.cache/song_covers
	rm music_player