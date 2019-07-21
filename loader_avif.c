/* File: loader_avif.c
   Time-stamp: <2012-12-09 21:19:30 gawen>

   Copyright (c) 2011 David Hauweele <david@hauweele.net>
   All rights reserved.
   
   Modified by Vitaly "_Vi" Shukela to use avif instead of webp. Contains snippets from apps/avifdec.c

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the University nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE. */

#define _BSD_SOURCE 1

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <Imlib2.h>

#include "imlib2_common.h"
#include "loader.h"

#include <inttypes.h>
#include "avif/avif.h"

char load(ImlibImage * im, ImlibProgressFunction progress,
          char progress_granularity, char immediate_load)
{
  
  int w,h;
  int y=0;

  char retcode = 0;
  
  if(im->data)
    return 0;

  FILE * inputFile = fopen(im->real_file, "rb");
  if (!inputFile) {
      fprintf(stderr, "Cannot open file for read: %s\n", im->real_file);
      return 0;
  }
  fseek(inputFile, 0, SEEK_END);
  size_t inputFileSize = ftell(inputFile);
  fseek(inputFile, 0, SEEK_SET);

  if (inputFileSize < 1) {
      fprintf(stderr, "File too small: %s\n", im->real_file);
      fclose(inputFile);
      return 0;
  }

  avifRawData raw = AVIF_RAW_DATA_EMPTY;
  avifRawDataRealloc(&raw, inputFileSize);
  if (fread(raw.data, 1, inputFileSize, inputFile) != inputFileSize) {
      fprintf(stderr, "Failed to read %zu bytes: %s\n", inputFileSize, im->real_file);
      fclose(inputFile);
      avifRawDataFree(&raw);
      return 0;
  }
  fclose(inputFile);

  avifImage * avif = avifImageCreateEmpty();
  avifDecoder * decoder = avifDecoderCreate();
  avifResult decodeResult = avifDecoderRead(decoder, avif, &raw);
  if (decodeResult == AVIF_RESULT_OK) {
    // OK
  } else {
    fprintf(stderr, "avif result: %s\n", avifResultToString(decodeResult));
    goto EXIT;
  }

  w = avif->width;
  h = avif->height;
  im->w = w;
  im->h = h;

  if(!IMAGE_DIMENSIONS_OK(w, h))
      goto EXIT;
  
  if(progress) {
      progress(im, 0, 0, 0, w, h);
  }

  if (!immediate_load) {
      retcode = 1;
      goto EXIT;
  }

  uint8_t *bgra;
  bgra = (uint8_t*)malloc(4 * w * h);
  if (!bgra) goto EXIT;

  for (y = 0; y < h; ++y) {
     int x;
     for (x=0; x < w; ++x) {
		  bgra[4*(y*w + x) + 0] = 0;//plane[y*stride + 4*x + 2];
		  bgra[4*(y*w + x) + 1] = 0;//plane[y*stride + 4*x + 1];
		  bgra[4*(y*w + x) + 2] = 0;//plane[y*stride + 4*x + 0];
		  bgra[4*(y*w + x) + 3] = 0;//plane[y*stride + 4*x + 3];
     }
  }

  im->data = (DATA32*)bgra;
  if(progress)
      progress(im, 100, 0, 0, w, h);

  SET_FLAGS(im->flags, F_HAS_ALPHA);
    
  im->format = strdup("avif");
  retcode = 1;

EXIT:
  avifRawDataFree(&raw);
  if(decoder) avifDecoderDestroy(decoder);
  if(avif) avifImageDestroy(avif);

  return retcode;
}

char save(ImlibImage *im, ImlibProgressFunction progress,
          char progress_granularity)
{
  return 0;
}

void formats(ImlibLoader *l)
{
  int i;
  char *list_formats[] = { "avif" };

  l->num_formats = (sizeof(list_formats) / sizeof(char *));
  l->formats     = malloc(sizeof(char *) * l->num_formats);
  for(i = 0 ; i < l->num_formats ; i++)
    l->formats[i] = strdup(list_formats[i]);
}
