/*============================================================================
 *
 * main.c
 * 
 * Copyright (c)2023 Kevin Boone, GPLv3
 * =========================================================================*/
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
// Include the .h file in fonts/ that contains the declarations of the
//   font length and data arrays for the chosen font.
#include <courier_bold_72.h> 
#include "picojpeg.h" 
#include "framebuffer.h" 

#define FB_DEV "/dev/fb0"

// Use the names of the font data and length arrays that match the 
//   declarations in the auto-generated .h file in fonts/
#define FONT_LENGTH_ARRAY courier_bold_72_length 
#define FONT_DATA_ARRAY courier_bold_72_data
#define GLYPH_WIDTH courier_bold_72_width
#define GLYPH_HEIGHT courier_bold_72_height

/*============================================================================
 * DecoderContext 
 * This is used to carry the state of the JPEG decode process. PicoJPEG
 *   works by requesting new data using a callback function. This function
 *   can only take one application-specific argument, so all the data about
 *   the data buffer and current position must be carried in a single structure.
 * =========================================================================*/
typedef struct _DecoderContext
  {
  unsigned char *data;
  unsigned int pos;
  unsigned int remain;
  } DecoderContext;

/*============================================================================
 * get_jpeg_data_for_char 
 * Gets the JPEG image data for a specific character, which must be in the
 *   range set when generating the font data using makefont.pl. The data
 *   will be one of the entries in the auto-generated font array.
 * If there is no data for the character, return NULL.
 * =========================================================================*/
static unsigned char *get_jpeg_data_for_char (int c)
  {
  if (c < ' ' || c > '~') return NULL;
  return FONT_DATA_ARRAY[c - ' '];
  }

/*============================================================================
 * get_jpeg_length_for_char
 * Gets the length of the JPEG image data for a specific character, 
 *   which must be in the range set when generating the font data 
 *   using makefont.pl. The data will be one of the entries in the 
 *   auto-generated font array.
 * If there is no data for the character, return 0.
 * =========================================================================*/
unsigned int get_jpeg_length_for_char (int c)
  {
  if (c < ' ' || c > '~') return 0;
  return *FONT_LENGTH_ARRAY[c - ' '];
  }

/* =======================================================================
 * pjpeg_callback
 * This function is called by PicoJPEG each time it needs more JPEG image
 *   data. The application is expected to read up to buf_size bytes, and
 *   return the number of bytes actually read. In principle, we can signal
 *   an error by returning non-zero; but as the image data is actually
 *   in memory, no (detectable) error is possible.
 * The DecoderContext object keeps track of the amount of data that 
 *   remains to read.
 ======================================================================= */
static unsigned char pjpeg_callback (unsigned char *buf, 
        unsigned char buf_size, unsigned char *bytes_actually_read, 
        void *data)
  {
  DecoderContext *context = data;
  int avail = context->remain; 
  if (avail > buf_size) avail = buf_size;
  memcpy (buf, context->data + context->pos, avail);
  *bytes_actually_read = (unsigned char)avail;
  context->remain -= avail;
  context->pos += avail;
  return 0;
  }


/*============================================================================
 * uncompress_jpeg_glyph 
 * Uncompress the JPEG data at address 'data', with length 'len', into the
 *   buffer at 'output'. The output buffer must be large enough to store
 *   the expanded JPEG image -- no checks are made. We need to know the
 *   image width, as data is arrange in row order in the output buffer.
 * This function would benefit from more extensive error checking, 
 *   particularly if the application were expanded to handle multiple fonts.
 * The output buffer is assumed to be zeroed before calling this method.
 * =========================================================================*/
int uncompress_jpeg_glyph (unsigned char *data, unsigned int len, 
     unsigned int output_width, unsigned char *output)
  {
  int ret = 0;
  DecoderContext context;
  context.data = data; // Tell the decompressor callback where to start
  context.pos = 0; // Store where in the input buffer we currently are
  context.remain = len; // Store how much is left to read

  pjpeg_image_info_t image_info;
  unsigned char r = pjpeg_decode_init (&image_info,
                        pjpeg_callback, &context, 0); 
  if (r == 0)
    {
    // PicoJPEG decoder read the JPEG header OK.

    // TODO -- we should really check that the decompressed JPEG really
    //   does have the size of our converted output buffer

    int decoded_width = image_info.m_width;
    int decoded_height = image_info.m_height;
    int block_width = image_info.m_MCUWidth;
    int block_height = image_info.m_MCUHeight;
    int x_offset = (output_width - decoded_width) / 2;

    int mcu_x = 0, mcu_y = 0;
    for (;;)
      {
      // Get the next block of data from the decoder
      unsigned char r = pjpeg_decode_mcu();
      if (r)
        {
        // TODO -- show error, if we haven't run out of data
        // It isn't necessarily an error if we get here -- most likely
        //   we've just run out of data to decode
        break;
        }

      int target_x = mcu_x * block_width;
      int target_y = mcu_y * block_height;

      for (int y = 0; y < block_height; y += 8)
        {
        int by_limit = decoded_height - (mcu_y * block_height + y);
        if (by_limit > 8) by_limit = 8;

        for (int x = 0; x < block_width; x += 8)
          {
          int bx_limit = decoded_width - (mcu_x * block_width + x);
          if (bx_limit > 8) bx_limit = 8;

          int src_ofs = (x * 8U) + (y * 16U);
          // Although the image is grayscale, we'll pick up the 
          //   decompressed data from the green channel -- I guess that's
          //   just how PicoJPEG works.
          unsigned char *p = image_info.m_pMCUBufG + src_ofs;

          for (int by = 0; by < by_limit; by++)
            {
            for (int bx = 0; bx < bx_limit; bx++)
               {
               unsigned char g = *p++;
               output [(target_y + by) * output_width 
                 + (x_offset + target_x + bx)] = g; 
               }

            p += (8 - bx_limit);
            }
          }
        }

      mcu_x++;
      if (mcu_x == image_info.m_MCUSPerRow)
        {
        mcu_x = 0;
        mcu_y++;
        }
      }
    }
  else
    ret = (int)r;

  return ret;
  }

/*============================================================================
 * display_glyph 
 * Displays a glyph on the framebuffer at location (top left) (x,y). We
 *   need to know the width and height of the glyph, because the glyph
 *   data is stored in row order in a flat array 'buffer'.
 * =========================================================================*/
void display_glyph (FrameBuffer *fb, unsigned char *buffer, 
      unsigned int x, unsigned int y, unsigned int glyph_width, 
      unsigned int glyph_height)
  {
  for (unsigned int i = 0; i < glyph_height; i++)
    {
    for (unsigned int j = 0; j < glyph_width; j++)
      {
      unsigned char b = buffer[i * glyph_width + j];
      //if (b) printf ("."); else printf (" "); 
      framebuffer_set_pixel (fb, x + j, y + i, b, b, b);
      }
    //printf ("\n");
    }
  }

/*============================================================================
 * main
 * =========================================================================*/
int main (int argc, char **argv)
  {
  (void)argc; (void)argv; // Not used

  // Define the text to display 
  const char *text = "Hello, World!\nThis is a test !@#$%^&*({}[\\|";

  // Where to start drawing the text
  unsigned int x_start = 50; 
  unsigned int y_start = 200;

  // Create and initalize the framebuffer driver
  FrameBuffer *fb = framebuffer_create (FB_DEV);
  
  char *error;
  if (framebuffer_init (fb, &error))
    {
    // Create a buffer large enough to hold a single character glyph. This
    //   size was determined when the font data was generated using
    //   makefonts.pl. Because this application only supports monospaced 
    //   fonts at present, the same buffer can be used for all glyphs.
    // Howewver, if the application has more memory available, it would make
    //   good sense to store a cache of decompressed glyphs, to avoid
    //   having to decmopress each glyph for each character. THere's a
    //   trade-off here between speed and memory and, in this case, we're
    //   opting for minimal memory usage.
    unsigned char *buffer = malloc (GLYPH_WIDTH * GLYPH_HEIGHT);

    unsigned int x = x_start;
    unsigned int y = y_start;
    while (*text)
      {
      int c = *text;
      if (c == 10) // Newline
        {
        x = x_start; 
        y += GLYPH_HEIGHT; // TODO -- adjust line spacing
        }
      else
        {
        memset (buffer, 0,(GLYPH_WIDTH * GLYPH_HEIGHT)); 
	// Find the JPEG data and its length
	int len = get_jpeg_length_for_char (c);
	unsigned char *data = get_jpeg_data_for_char (c);

	// Uncompress the JPEG data into buffer, which we know (hope) will 
	//   be large enough.
	if (data)
	  {
	  uncompress_jpeg_glyph (data, len, GLYPH_WIDTH, 
	    buffer);
	  // Show the glyph on the framebuffer.
	  display_glyph (fb, buffer, x, y, GLYPH_WIDTH, GLYPH_HEIGHT);
	  }

	x += GLYPH_WIDTH;
        }
      text++;
      }

    free (buffer);
    }
  else
    {
    fprintf (stderr, "Can't open framebuffer: %s\n", error);
    free (error);
    }
  framebuffer_destroy (fb);
  }

