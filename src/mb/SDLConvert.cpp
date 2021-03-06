/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#include "ginga.h"

GINGA_PRAGMA_DIAG_IGNORE (-Wconversion)
#include "SDLConvert.h"

#ifdef _MSC_VER
# include "atlimage.h"
#endif

GINGA_MB_BEGIN

#if !defined _MSC_VER && defined WITH_MULTIDEVICE && WITH_MULTIDEVICE
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
// 24 bit RGB masks on big-endian
#define RMASK24 0xFF0000
#define GMASK24 0x00FF00
#define BMASK24 0x0000FF
#else
#define RMASK24 0x0000FF
#define GMASK24 0x00FF00
#define BMASK24 0xFF0000
#endif

#define OUTPUT_BUFFER_SIZE 4096

typedef struct
{
  struct jpeg_destination_mgr pub;
  SDL_RWops *rwop;
  Uint8 buffer[OUTPUT_BUFFER_SIZE];
} sdlrw_dest_mgr;
typedef sdlrw_dest_mgr *sdlrw_dest_ptr;

static void
init_destination (j_compress_ptr cinfo)
{
  sdlrw_dest_ptr dest = (sdlrw_dest_ptr)cinfo->dest;
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUFFER_SIZE;
}

static int
empty_output_buffer (j_compress_ptr cinfo)
{
  sdlrw_dest_ptr dest = (sdlrw_dest_ptr)cinfo->dest;
  if (SDL_RWwrite (dest->rwop, dest->buffer, 1, OUTPUT_BUFFER_SIZE)
      != OUTPUT_BUFFER_SIZE)
    {
      ERREXIT (cinfo, JERR_FILE_WRITE);
    }
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUFFER_SIZE;

  return TRUE;
}

static void
term_destination (j_compress_ptr cinfo)
{
  sdlrw_dest_ptr dest = (sdlrw_dest_ptr)cinfo->dest;
  size_t remain = OUTPUT_BUFFER_SIZE - dest->pub.free_in_buffer;
  if (remain > 0)
    {
      if (SDL_RWwrite (dest->rwop, dest->buffer, 1, remain) != remain)
        {
          ERREXIT (cinfo, JERR_FILE_WRITE);
        }
    }
}

static void
jpeg_SDL_RW_dest (j_compress_ptr cinfo, SDL_RWops *rwop)
{
  sdlrw_dest_ptr dest;
  if (cinfo->dest == NULL)
    {
      cinfo->dest
          = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small) (
              (j_common_ptr)cinfo, JPOOL_PERMANENT,
              sizeof (sdlrw_dest_mgr));
    }
  dest = (sdlrw_dest_ptr)cinfo->dest;
  dest->rwop = rwop;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUFFER_SIZE;
}

typedef struct
{
  struct jpeg_error_mgr errmgr;

  jmp_buf environ;
} sdlrw_error_mgr;

typedef sdlrw_error_mgr *sdlrw_error_ptr;

static void
sdlrw_error_exit (j_common_ptr cinfo)
{
  sdlrw_error_ptr err = (sdlrw_error_ptr)cinfo->err;
  longjmp (err->environ, 1);
}

static int
is_usable_format (SDL_Surface *surf)
{
  SDL_PixelFormat *fmt = surf->format;

  return (fmt->BytesPerPixel == 3 && fmt->Rmask == RMASK24
          && fmt->Gmask == GMASK24 && fmt->Bmask == BMASK24
          && fmt->Aloss
                 == 8); // Aloss is 8 when there is no alpha channel to use.
}

static SDL_Surface *
make_usable_format (SDL_Surface *surf)
{
  SDL_PixelFormat *pf = SDL_AllocFormat (SDL_PIXELFORMAT_RGB332);
  SDL_Surface *rgb_surf = SDL_ConvertSurface (surf, pf, SDL_SWSURFACE);
  SDL_FreeFormat (pf);

  return rgb_surf;
}

static int
IMG_SaveJPG_RW (SDL_RWops *dest, SDL_Surface *surf, int quality)
{
  struct jpeg_compress_struct cinfo;
  sdlrw_error_mgr jerr;
  SDL_Surface *outsurf = surf;
  Uint8 *line;
  int y, start, isRGB;

  if (quality < 0)
    quality = 0;
  if (quality > 100)
    quality = 100;

  if (!dest || !surf)
    {
      SDL_SetError ("No destination RWops specified.");
      return -1;
    }

  isRGB = is_usable_format (surf);
  if (!isRGB && !(outsurf = make_usable_format (surf)))
    {
      SDL_SetError ("Unable to create temporary surface");
      return -1;
    }
  start = SDL_RWtell (dest);
  cinfo.err = jpeg_std_error (&jerr.errmgr);
  jerr.errmgr.error_exit = sdlrw_error_exit;

  if (setjmp (jerr.environ))
    {
      jpeg_destroy_compress (&cinfo);
      if (!isRGB)
        SDL_FreeSurface (outsurf);

      SDL_RWseek (dest, start, RW_SEEK_SET);
      SDL_SetError ("JPEG saving error, giving up.");
      return -1;
    }

  jpeg_create_compress (&cinfo);
  jpeg_SDL_RW_dest (&cinfo, dest);

  cinfo.in_color_space = JCS_RGB;
  cinfo.image_width = surf->w;
  cinfo.image_height = surf->h;
  cinfo.input_components = 3;
  cinfo.dct_method = JDCT_FLOAT;

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);
  jpeg_start_compress (&cinfo, TRUE);

  if (SDL_MUSTLOCK (outsurf))
    SDL_LockSurface (outsurf);
  for (y = 0, line = (Uint8 *)outsurf->pixels; y < outsurf->h;
       y++, line += outsurf->pitch)
    {
      jpeg_write_scanlines (&cinfo, &line, 1);
    }

  if (SDL_MUSTLOCK (outsurf))
    SDL_UnlockSurface (outsurf);
  if (!isRGB)
    SDL_FreeSurface (outsurf);
  jpeg_finish_compress (&cinfo);
  jpeg_destroy_compress (&cinfo);
  return 0;
}
#endif

int
SDLConvert::convertSurfaceToJPEG (arg_unused (const char *filename), arg_unused(SDL_Surface *surf), arg_unused(int quality))
{
#ifdef _MSC_VER
  char *bmpfile;
  bmpfile = new char[strlen (filename) + 5];
  strcpy (bmpfile, filename);
  strcat (bmpfile, ".bmp");
  SDL_SaveBMP (surf, bmpfile);
  CImage myImage;
  myImage.Load (bmpfile);
  myImage.Save (filename);
  delete bmpfile;
  return 0;
#elif defined WITH_MULTIDEVICE && WITH_MULTIDEVICE
  SDL_RWops *out;
  if (!(out = SDL_RWFromFile (filename, "wb")))
    {
      return (-1);
    }
  int result = IMG_SaveJPG_RW (out, surf, quality);
  SDL_RWclose (out);
  return result;
#endif
  return -1;
}

GINGA_MB_END
