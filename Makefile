all:
	gcc dotify.c  song_information.c file_management.c stream_management.c file_watch.c timer.c -o dotify -I raylib/src/ raylib/src/libraylib.a -lavformat -lavutil -lm -Wall
clean:
	rm -rf $(HOME)/.cache/song_covers
	rm dotify