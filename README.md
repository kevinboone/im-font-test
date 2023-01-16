# im-font-test

Version 0.1a, January 2023.

## What is this

This is an example program that demonstrates how to use ImageMagick to generate
a set of anti-aliased font glyphs for the ASCII character set, how to pack the
glyph data into C arrays to be linked into an application program, and how to
decompress them at runtime. The example displays text on a Linux framebuffer,
because it has to go somewhere, just for demonstration purposes. The technique
demonstrated is really intended for use in a microcontroller application.  In
such an application, there probably isn't storage to accommodate uncompressed
font data, nor any way to run a full-scale font rendering library. Storing the
fonts as JPEG reduces the storage requirement by about 80% but at the expense,
of course, of increased processing time.

I've tried to keep the framebuffer code separate from the part of the example
that handles the character rendering.

## Why?

Many low-cost displays for microcontrollers have only on-off pixels. 
In these cases, there's really no need (or way) to anti-alias fonts -- simple
bitmap fonts will be as fine. Well, as fine as can be achieved with
on-off displays.

Many displays, however, support colour or, at last, grayscale operation.  On
such displays, text quality can be improved enormously using anti-aliased font
glyphs. These use grayscale pixels to give the impression of smoother font
shapes than really exist. Unfortunately, suitable font data is not widely
available.  The example demonstrates how to generate it, from any font that can
be rendered by ImageMagick.

## Building the font data

The utility `makefont.pl` in the `fonts/` directory generates the
font data in the form of a C source and header file. This utility is
designed to be run under Linux, but the files `courier_bold_72.c` and
`courier_bold_72.h` could be used as supplied on any system with a
C compiler. 

To run `makefont.pl` you will need Perl, ImageMagick, and `xsd`.

`makefont.pl` does the following.

1. Uses ImageMagick to write out a monochrome JPEG file for each character in
   the range 32-126. Other characters could be included, of course, at the
expense of additional storage.

2. It then uses `xsd` to transform each character's JPEG file into a block of C
   code that defines an array.

3. The arrays are concatenated into a single C source file; `makefont.pl` then
   generates a single array that lists each individual character's array. This
overall array will have 95 character pointers, one for each character in the
range 32-126. 

4. The utility also generates a 95-element array containing the size of the
   JPEG data for each character. This ought to be an array of integers, but
actually it's an array of pointers to integers, because of the way that `xsd`
generates C code.

5. It then generates a C header (`.h`) file containing declarations of the
   elements that are defined in the C file. Thus applications can include the
`.h` file, and keep the `.c` file as a separate unit.

## Changing the font face and size

At present, the example only supports monospaced fonts. It should go without
saying that ImageMagick will only render fonts that are actually installed.
You can use the ImageMagick `identify` utility to list known fonts: 

    $identify -list font

As supplied, the example uses Courier Bold because it seems to be available on
most Linux systems. The font can readily be changed, but note that changes will
need to be made in several places.

0. Before changing anything, ensure that you have ImageMagick and `xsd`.

1. Edit fonts/makefont.pl, and change the `FONT` and `SIZE` values at the top
   of the file.

2. Run `makefont.pl` in the `fonts/` directory. This will generate the `.c` and
   `.h` files that match the font.

3. Change the `FONT_NAME` and `FONT_SIZE` variables at the top of `Makefile`.
   This will ensure that the C source file containing the new font data gets
compiled and linked.

4. Change `src/main.c`, so that the correct `.h` file is included, and the
   appropriate auto-generated length and data arrays are used.  These settings
are all in the first few lines of `main.c`.
  
Remember that this example only supports monospaced fonts. 

If the font name contains a dash (minus sign) then `makefont.pl` will write C
source and header files with underscores in their names instead. More
importantly, however, the C variables that are named after the font will also
contain underscores, because the minus sign is an operator in C.

## Building on Linux

If the font files have been generated, just run `make`.

## Testing on Linux

The example writes to a framebuffer. As supplied, only linear RGB framebuffers
are supported (this isn't a demonstration of how to use a Linux framebuffer,
and I didn't want to clutter it with code for this purpose).

You almost certainly won't be able to run the example under a graphical
desktop: you'll first need to switch to a plain, text console.  The way to do
this varies considerably between Linux versions, and even between different
releases of the same Linux.

On my Fedora system running the Gnome desktop, I use ctrl+alt+F6 to switch to a
text console, and ctrl+alt+F2 to switch back to the graphical desktop. 

Please note that you'll probably need to run as `root` to write directly to the
framebuffer.

The included `screenshot.png` shows what output is expected on the
framebuffer.

## Storage issues

The font data for the example font (Courier Bold 72pt), when compressed as
JPEG, uses about 100kB of storage for the ASCII printable characters.  If this
same data were stored uncompressed, it would require about 500kB.  At runtime,
the JPEG decompresser requires about 256 bytes of RAM, because it works on 8x8
pixel blocks. As written, the application decompresses each glyph into a block
of memory large enough to store the whole glyph -- that's about 4kB for the
chosen font. However, in a very constrained application, we could decompress
directly onto the display device. In practice, most microcontroller displays
are much faster at handling blocks of pixels than writing the same data
pixel-by-pixel, so there's a trade-off between speed and RAM.

## Improvements

An obvious way to improve the example would be to extend it to support
variable-pitch fonts. To do this, it will be necessary to have the `makefont`
utility write out the width of each glyph in a C array, rather than the size of
the bounding box that will accommodate all glyphs. The application would then
read the glyph width, in exactly the same way it currently reads the glyph data
length, and adjust the character spacing accordingly.

In a system with sufficient RAM, another improvement would be to maintain a
cache of rendered glyphs, rather than rending each one.  In practice, however,
the increased use of memory is unlikely to offset the increased speed, unless
the same character, or small number of characters, is used repeatedly.

The example only demonstrates rendering white text on a black background.
Anti-aliasing is background-dependent. To use the technique demonstrated
in this example with a different text colour is trivial -- just write
scale the RGB value of the desired colour by the value in the 
glyph data. To use a different background colours, however, you'll need
to find a way to combine the grayscale glyph data with the background.

## Legal

In general, this example program is copyright (c)Kevin Boone, released under
the terms of the GNU Public Licence, v3.0. However, the PicoJPEG decompresser
is by Rich Geldreic and Chris Phoenix, and is in the public domain.  

## More information

For a more detailed discussion of how this program works, please see;

https://kevinboone.me/im-font-test.html


