CC = clang
CFLAGS = -g -O3 -ffast-math -Wall -Werror $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS = $(shell pkg-config --libs sdl2 SDL2_image) -lm

all: uvgut uvtit

uvgut: uvgut.c uvmap.h
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $<

uvtit: uvtit.c uvmap.h
	$(CC) $(CFLAGS) $(LDLIBS) -o $@ $<

clean:
	rm -rf uv{gut,tit} uv{gut,tit}.dSYM gut-*.* tit-*.* *~
