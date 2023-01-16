#
# Makefile for im-font-test. Some of the .c and .h files used in this 
#   test are auto-generated by the makefont.pl utility, in the fonts/
#   directory.
# Copyright (c)2023 Kevin Boone, GPLv3
#

TARGET=im-font-test

# Font name and size must match the name and size of a font generated by
#   fonts/makefont.pl. In the Makefile, we're justing specifying the fonts
#   to compile and build. In the C source we have to specify which font
#   to use.

FONT_NAME=courier_bold
FONT_SIZE=72
FONT=$(FONT_NAME)_$(FONT_SIZE)

all: $(TARGET) 

CC=gcc
CFLAGS=-Wall -Wextra -O0 -g

$(TARGET): build/main.o build/$(FONT).o build/picojpeg.o build/framebuffer.o
	$(CC) -o $(TARGET) build/main.o build/$(FONT).o build/picojpeg.o build/framebuffer.o

build/$(FONT).o: fonts/$(FONT).c fonts/$(FONT).h
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/$(FONT).o -c fonts/$(FONT).c

build/main.o: src/main.c fonts/$(FONT).h src/picojpeg.h src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -DFONT=\"$(FONT)\" -I fonts -o build/main.o -c src/main.c

build/framebuffer.o: src/framebuffer.c src/framebuffer.h
	@mkdir -p build
	$(CC) $(CFLAGS) -I fonts -o build/framebuffer.o -c src/framebuffer.c

build/picojpeg.o: src/picojpeg.c src/picojpeg.h
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/picojpeg.o -c src/picojpeg.c

clean:
	rm -rf build
