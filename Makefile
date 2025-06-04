all:
	gcc dotify.c song_information.c file_management.c stream_management.c music_management.c file_watch.c timer.c -o dotify -I raylib/src/ raylib/src/libraylib.a -lavformat -lavutil -lm -Wall
clean:
	rm -rf $(HOME)/.cache/extracted_dotify_covers
	rm dotify