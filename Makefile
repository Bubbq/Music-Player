all: 
ifeq (,$(wildcard raylib/src/libraylib.a))
	make -C raylib/src/
endif
	g++ main.cpp -o run -I raylib/src/ raylib/src/libraylib.a -lm -pedantic

clean:
	rm -f run
	rm -f savedMusic.txt
	rm -rf covers
