CC = clang
CFLAGS = -g -O3 -ffast-math -Wall -Werror $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS = $(shell pkg-config --libs sdl2 SDL2_image) -lm

uvgut: uvgut.c

clean:
	rm -rf uvgut uvgut.dSYM *~
