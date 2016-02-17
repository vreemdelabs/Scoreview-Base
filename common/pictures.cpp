/*
 Scoreview (R)
 Copyright (C) 2015 Patrick Areny
 All Rights Reserved.

 Scoreview is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>

#include <iterator>
#include <list>
#include <vector>

#ifdef __ANDROID__
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#endif

#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "pictures.h"

using namespace std;

CpictureList::CpictureList(CGL2Dprimitives *primitives):
  m_gfxprimitives(primitives)
{
}

CpictureList::~CpictureList()
{
}

void CpictureList::add_picture_from_area(t_coord dim, SDL_Rect srcrect, SDL_Surface *img, string name)
{
  const int    cdepth = 32;
  SDL_Rect     dstrect;
  SDL_Surface  *pdestsurface;
  Cpicture     *pp;
  int          err;
  int          i;
  Uint32       *pixels;
  Uint32       rmask, gmask, bmask, amask;
  GLuint       texture_id;

  dstrect.x = dstrect.y = 0;
  dstrect.w = dim.x;
  dstrect.h = dim.y;
  //printf("creating a surface of %d, %d\n", dim.x, dim.y);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  rmask = 0xff000000;
  gmask = 0x00ff0000;
  bmask = 0x0000ff00;
  amask = 0x000000ff;
#else
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
  amask = 0xff000000;
#endif
  pdestsurface = SDL_CreateRGBSurface(0, dim.x, dim.y, cdepth, rmask, gmask, bmask, amask);
  if (pdestsurface == NULL)
    {
      printf("SDL surface creation error\n");
      exit(EXIT_FAILURE);
    }
  err = SDL_BlitSurface(img, &srcrect, pdestsurface, &dstrect);
  if (err != 0)
    {
      printf("SDL blit surface error:%s\n", SDL_GetError());
    }
  // Set the alpha channel of the transparent pixels to 0
  pixels = (Uint32*)pdestsurface->pixels;
  for (i = 0; i < pdestsurface->w * pdestsurface->h; i++)
    if (pixels[i] == 0xFF00FF30)
      pixels[i] = pixels[i] & 0x00FFFFFF;
  texture_id = m_gfxprimitives->create_OpenGL_texture(pdestsurface, name);
  pp = new Cpicture(pdestsurface, dim, name, texture_id);
  m_gfxprimitives->add_picture(pp);
}

void CpictureList::add_picture(SDL_Surface *img, string name)
{
  Cpicture     *pp;
  t_coord       dim;
  GLuint       texture_id;

  dim.x = dim.y = dim.z = 0;
  dim.x = img->w;
  dim.y = img->h;
  texture_id = m_gfxprimitives->create_OpenGL_texture(img, name);
  pp = new Cpicture(img, dim, name, texture_id);
  m_gfxprimitives->add_picture(pp);
}

void CpictureList::add_whole_image(string imagefile, string name)
{
  SDL_Surface *img;

  try
    {
      img = IMG_Load(imagefile.c_str());
      if (img == NULL)
	throw 1;
      //
      add_picture(img, name);
      //
      //SDL_FreeSurface(img);
    }
  catch (int err)
    {
      if (err)
	{
	  printf("User interface Error: could not load the images from \"%s\".\n", imagefile.c_str());
	  exit(EXIT_FAILURE);
	}
    }
}

SDL_Surface *CpictureList::img_load_ARGB(string imagefile)
{
  SDL_Surface  *loadedimg;
  SDL_Surface  *img;
  Uint32        rmask, gmask, bmask, amask;

  loadedimg = IMG_Load(imagefile.c_str());
  if (loadedimg == NULL)
    throw 1;
  //  if (SDL_SetColorKey(loadedimg, SDL_FALSE, 0xFF30FF00))
  //  throw 1;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
  if (loadedimg->format->Rmask == rmask &&
      loadedimg->format->Gmask == gmask &&
      loadedimg->format->Bmask == bmask &&
      loadedimg->format->Amask == amask )
    {
      return loadedimg;
    }
  else
    {
      //printf("In  Rmask=%X Gmask=%X Bmask=%X Amask=%X\n", loadedimg->format->Rmask, loadedimg->format->Gmask, loadedimg->format->Bmask, loadedimg->format->Amask);
      // Conversion to a known surface type
      img = SDL_ConvertSurfaceFormat(loadedimg, SDL_PIXELFORMAT_ARGB8888, 0);
      SDL_FreeSurface(loadedimg);
      //printf("Cnv Rmask=%X Gmask=%X Bmask=%X Amask=%X\n", img->format->Rmask, img->format->Gmask, img->format->Bmask, img->format->Amask);
      if (img == NULL)
	throw 1;
    }
  return img;
}

void CpictureList::open_interface_imgs(string imagefile)
{
  t_coord      dim;
  t_coord      rdim;
  SDL_Surface  *img;
  SDL_Rect     srcrect;

  try
    {
      img = img_load_ARGB(imagefile);
      //
      dim.x = 32;
      dim.y = 64;
      srcrect.x = srcrect.y = 1;
      srcrect.w = dim.x;
      srcrect.h = dim.y;
      //
      add_picture_from_area(dim, srcrect, img, string("clefsol"));
      srcrect.x += dim.x + 1;
      add_picture_from_area(dim, srcrect, img, string("cleffa"));
      srcrect.x += dim.x + 1;
      rdim = dim;
      rdim.y = 38;
      add_picture_from_area(rdim, srcrect, img, string("cursorhand"));
      srcrect.x += dim.x + 1;
      rdim.y = 8;
      add_picture_from_area(rdim, srcrect, img, string("beamlink"));
      srcrect.y = 19;
      add_picture_from_area(rdim, srcrect, img, string("brokenbeamlink"));
      srcrect.x += dim.x + 1;
      rdim.y = 18;
      srcrect.y = 1;
      add_picture_from_area(rdim, srcrect, img, string("cursorlr"));
      srcrect.x += dim.x + 1;
      rdim.y = dim.y / 2;
      add_picture_from_area(dim, srcrect, img, string("mousewheel"));
      srcrect.x += dim.x + 1;
      add_picture_from_area(dim, srcrect, img, string("trashcan"));
      srcrect.x = 7 * 32 + 8;
      srcrect.y = 1;
      srcrect.w = 16;
      srcrect.h = 16;
      add_picture_from_area(dim, srcrect, img, string("back_carbon"));
      //
      SDL_FreeSurface(img);
    }
  catch (int err)
    {
      if (err)
	{
	  printf("User interface Error: could not load the images from \"%s\".\n", imagefile.c_str());
	  exit(EXIT_FAILURE);
	}
    }
}

void CpictureList::open_practice_drawings(string imagefile)
{
  t_coord      dim;
  SDL_Surface  *img;
  SDL_Rect     srcrect;

  try
    {
      img = img_load_ARGB(imagefile);
      //
      dim.x = 16;
      dim.y = 64;
      srcrect.x = 1;
      srcrect.y = 66;
      srcrect.w = dim.x;
      srcrect.h = dim.y;
      //
      add_picture_from_area(dim, srcrect, img, string("whitekey"));
      srcrect.x += srcrect.w;
      add_picture_from_area(dim, srcrect, img, string("whitekeypushed"));
      srcrect.x += srcrect.w + 1;
      dim.x = 12;
      dim.y = 44;
      srcrect.w = dim.x;
      srcrect.h = dim.y;
      add_picture_from_area(dim, srcrect, img, string("blackkey"));
      srcrect.x += srcrect.w;
      add_picture_from_area(dim, srcrect, img, string("blackkeypushed"));
      srcrect.x = 2 * 32 + 3;
      add_picture_from_area(dim, srcrect, img, string("blackkeytobepressed"));
      //
      srcrect.x = 59;
      srcrect.y = 66;
      dim.x =  7;
      dim.y = 12;
      srcrect.w = dim.x;
      srcrect.h = dim.y;
      add_picture_from_area(dim, srcrect, img, string("string"));
      srcrect.x = 59;
      srcrect.y = 78;
      add_picture_from_area(dim, srcrect, img, string("string_played"));
      //
      SDL_FreeSurface(img);
    }
  catch (int err)
    {
      if (err)
	{
	  printf("User interface Error: could not load the images from \"%s\".\n", imagefile.c_str());
	  exit(EXIT_FAILURE);
	}
    }
}

void CpictureList::open_instrument_tabs_drawings(string imagefile)
{
  t_coord      dim;
  SDL_Surface *img;
  SDL_Rect     srcrect;

  try
    {
      img = img_load_ARGB(imagefile);
      dim.x = dim.y = 128;
      srcrect.x = 1;
      srcrect.y = 1;
      srcrect.w = dim.x;
      srcrect.h = dim.y;
      add_picture_from_area(dim, srcrect, img, string("nail_violin"));
      srcrect.x += 129;
      add_picture_from_area(dim, srcrect, img, string("nail_piano"));
      srcrect.x += 129;
      add_picture_from_area(dim, srcrect, img, string("nail_guitar"));
      add_picture_from_area(dim, srcrect, img, string("nail_guitar_dropD"));
      //
      SDL_FreeSurface(img);
    }
  catch (int err)
    {
      if (err)
	{
	  printf("User interface Error: could not load the images from \"%s\".\n", imagefile.c_str());
	  exit(EXIT_FAILURE);
	}
    }
}

void CpictureList::open_skin(string imagefile, string skin_name)
{
  SDL_Surface  *img;

  try
    {
      img = img_load_ARGB(imagefile);
      add_picture(img, skin_name);
    }
  catch (int err)
    {
      if (err)
	{
	  printf("User interface Error: could not load the images from \"%s\".\n", imagefile.c_str());
	  exit(EXIT_FAILURE);
	}
    }
}

