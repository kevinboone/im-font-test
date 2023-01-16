/*============================================================================

  framebuffer.h

  General functions for manipulating a linear RGB framebuffer

  Copyright (c)2020 Kevin Boone, GPL v3.0

============================================================================*/

#pragma once

#ifndef BOOL
typedef int BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef BYTE 
typedef unsigned char BYTE;
#endif

struct _FrameBuffer;
typedef struct _FrameBuffer FrameBuffer;


#ifdef __cplusplus
extern "C" {
#endif

FrameBuffer     *framebuffer_create (const char *fbdev);
BOOL             framebuffer_init (FrameBuffer *self, char **error);
void             framebuffer_deinit (FrameBuffer *self);
void             framebuffer_destroy (FrameBuffer *self);
void             framebuffer_set_pixel (FrameBuffer *self, int x,
                      int y, BYTE r, BYTE g, BYTE b);
int              framebuffer_get_width (const FrameBuffer *self);
int              framebuffer_get_height (const FrameBuffer *self);
void             framebuffer_get_pixel (const FrameBuffer *self, 
                      int x, int y, BYTE *r, BYTE *g, BYTE *b);
BYTE            *framebuffer_get_data (FrameBuffer *self);

#ifdef __cplusplus
}
#endif
