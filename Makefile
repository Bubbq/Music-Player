all: 
ifeq (,$(wildcard raylib/src/libraylib.a))
	make -C raylib/src/
endif
	g++ main.cpp -o run -I raylib/src/ raylib/src/libraylib.a -lm -pedantic

clean:
	rm -rf $(HOME)/.cache/Music-Player-SRC
	rm run
