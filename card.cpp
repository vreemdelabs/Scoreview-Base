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

#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h> 
#include <GL/gl.h>

#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "card.h"

CCardList::CCardList()
{
  m_card_list.clear();
}

CCardList::~CCardList()
{
  Ccard *pc;

  pc = get_first();
  while (pc != NULL)
    {
      delete pc;
      pc = get_next();
    }
  m_card_list.clear();
}

void CCardList::add(Ccard *pc)
{
  m_card_list.push_back(pc);
}

Ccard* CCardList::get_first()
{
  m_iter = m_card_list.begin();
  if (m_iter == m_card_list.end())
    return NULL;
  return (*m_iter);
}

Ccard* CCardList::get_next()
{
  m_iter++;
  if (m_iter == m_card_list.end())
    return NULL;
  return *m_iter;
}

Ccard *CCardList::is_in(int x, int y)
{
  std::list<Ccard*>::iterator citer;
  Ccard   *pc;
  t_coord  p;

  p.x = x;
  p.y = y;
  citer = m_card_list.begin();
  while (citer != m_card_list.end())
    {
      pc = *citer;
      if (pc->is_in(p))
	{
	  return pc;
	}
      citer++;
    }
  return NULL;
}

Ccard *CCardList::get_card(char *gfxareaname)
{
  std::list<Ccard*>::iterator citer;
  Ccard   *pc;

  citer = m_card_list.begin();
  while (citer != m_card_list.end())
    {
      pc = *citer;
      if (pc->same_name(gfxareaname))
	{
	  return pc;
	}
      citer++;
    }
  return NULL;  
}

void CCardList::enable_card(char *name, float en)
{
  Ccard *pc;

  pc = get_card(name);
  if (pc)
    {
      pc->activate_card(en);
    }
}

void CCardList::resize(t_coord new_window_size)
{
  std::list<Ccard*>::iterator citer;

  citer = m_card_list.begin();
  while (citer != m_card_list.end())
    {
      (*citer)->resize_keep_ratio(new_window_size);
      citer++;
    }
}

// ------------------------------------------------------------------

Ccard::Ccard(Cgfxarea area, string title, string image_file_name, CGL2Dprimitives *pr, bool bhelpercard):
  m_bactive(false),
  m_binspect(false),
  m_mouseovertimecode(0),
  m_mouseofftimecode(0),
  m_start_display_timecode(0),
  m_bhelper(bhelpercard),
  m_area(area),
  m_gfxprimitives(pr),
  m_cardimg_filename(image_file_name),
  m_state(ewait)
{
  add_info_line(title);
}

Ccard::~Ccard()
{
}

t_coord Ccard::renderOutlinedText(SDL_Surface *dest, string text, TTF_Font *font, t_coord pos, t_coord dim, int color, int outcolor)
{
  SDL_Rect    dstrect;
  SDL_Color   c;
  SDL_Color   cbg;
  SDL_Surface *surface;
//  int         w, h;
  //Uint32      format;
  int         outline;
  t_coord     size;
  t_fcoord    fpos, fdim;

  c.r = (color >> 16) & 0xFF;
  c.g = (color >> 8)  & 0xFF;
  c.b = color & 0xFF;
  cbg.r = (outcolor >> 16) & 0xFF;
  cbg.g = (outcolor >> 8)  & 0xFF;
  cbg.b = outcolor & 0xFF;
  // Bg
  outline = 2;
  TTF_SetFontOutline(font, outline);
  surface = TTF_RenderText_Blended(font, text.c_str(), cbg);
  dstrect.x = pos.x + (dim.x - surface->w) / 2;
  dstrect.y = pos.y;
  dstrect.w = surface->w;
  dstrect.h = surface->h;
  m_gfxprimitives->blend_surfaces(surface, NULL, dest, &dstrect);
  SDL_FreeSurface(surface);
  // Fg
  outline = 0;
  TTF_SetFontOutline(font, outline);
  surface = TTF_RenderText_Blended(font, text.c_str(), c);
  dstrect.x = pos.x + 2 + (dim.x - surface->w) / 2;
  dstrect.y = pos.y + 2;
  dstrect.w = surface->w;
  dstrect.h = surface->h;
  m_gfxprimitives->blend_surfaces(surface, NULL, dest, &dstrect);
  SDL_FreeSurface(surface);
  return size;
}

t_coord Ccard::renderText(SDL_Surface *dest, string text, TTF_Font *font, t_coord pos, int color)
{
  SDL_Rect    dstrect;
  SDL_Color   c;
  SDL_Surface *surface;
//  int         w, h, access;
//  Uint32      format;
  t_coord     size;
  t_fcoord     fpos, fdim;

  c.r = (color >> 16) & 0xFF;
  c.g = (color >> 8)  & 0xFF;
  c.b = color & 0xFF;
  surface = TTF_RenderText_Blended(font, text.c_str(), c);
  dstrect.x = pos.x;
  dstrect.y = pos.y;
  dstrect.h = surface->h;
  dstrect.w = surface->w;
  m_gfxprimitives->blend_surfaces(surface, NULL, dest, &dstrect);
  size.x = surface->w;
  size.y = surface->w;
  SDL_FreeSurface(surface);
  return size;
}

void Ccard::create_texture(TTF_Font *title_font, TTF_Font *font, SDL_Surface* contour_texture)
{
  SDL_Surface* card_img;
  SDL_Surface* pdestsurface;
  SDL_Surface* card_image;
  SDL_Rect     dstrect;
  t_coord      dim, titledim;
  float        txth = 32;
  float        horizborder = 4;
  float        hsize = 88;
  std::list<string>::iterator iter;
  Uint32       *pixels;
  int          w, h;
  int          i;
  //Uint32       format;
  Uint32       rmask, gmask, bmask, amask;
  //int          access;
  t_coord      pos;
  GLuint       ident;
  Cpicture    *pp;

  //-----------------------------------------------------
  // Card image
  //-----------------------------------------------------
  try
    {
      card_img = IMG_Load(m_cardimg_filename.c_str());
      if (card_img == NULL)
	throw 1;
    }
  catch (int err)
    {
      if (err)
	{
	  printf("User interface Error: could not load the images.\n");
	  exit(EXIT_FAILURE);
	}
    }
  // Conversion to a known surface type
  pdestsurface = SDL_ConvertSurfaceFormat(card_img, SDL_PIXELFORMAT_ARGB8888, 0);
  if (pdestsurface == NULL)
    {
      printf("User interface Error: could not convert the image color space.\n");
      exit(EXIT_FAILURE);
    }
  SDL_FreeSurface(card_img);
  dim.x = contour_texture->w;
  dim.y = contour_texture->h;
  // Fixme use SDL color key
  // Set the alpha channel of the transparent pixels to 0
  pixels = (Uint32*)pdestsurface->pixels;
  for (i = 0; i < pdestsurface->w * pdestsurface->h; i++)
    if (pixels[i] == 0xFF30FF00)
      pixels[i] = pixels[i] & 0x00FFFFFF;
  
  //-----------------------------------------------------
  // Rendering Surface
  //-----------------------------------------------------
  horizborder = (dim.x * horizborder / 100);
  hsize = dim.y * hsize / 100;
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
  card_image = SDL_CreateRGBSurface(0, dim.x, dim.y, 32, rmask, gmask, bmask, amask);

  //-----------------------------------------------------
  // Card contour
  //-----------------------------------------------------
  dstrect.x = 0;
  dstrect.y = 0;
  dstrect.w = dim.x;
  dstrect.h = dim.y;
  m_gfxprimitives->blend_surfaces(contour_texture, NULL, card_image, &dstrect);

  //-----------------------------------------------------
  // Title text
  //-----------------------------------------------------
  iter = m_textlist.begin();
  if (TTF_SizeText(title_font, (*iter).c_str(), &w, &h) == 0)
    {
      pos.x = 76;
      pos.y = 4;
      titledim.x = 348;
      titledim.y =  40;
      renderOutlinedText(card_image, *iter, title_font, pos, titledim, 0xFFE4A221, 0xFF000000);
      txth = h;
    }
  iter++;

  //-----------------------------------------------------
  // Card image
  //-----------------------------------------------------
  dstrect.x = horizborder;
  dstrect.y = 8 + txth;
  dstrect.w = dim.x - 2. * horizborder;
  dstrect.h = hsize;
  //printf("dimx=%d dimy=%d resw=%d resh=%d\n", w, h, dstrect.w, dstrect.h);
  m_gfxprimitives->blend_surfaces(pdestsurface, NULL, card_image, &dstrect);
  SDL_FreeSurface(pdestsurface);

  //-----------------------------------------------------
  // Information
  //-----------------------------------------------------
  if (iter != m_textlist.end())
    {
      pos.x =  25;
      pos.y = 408;
      titledim.x = 180 - 25;
      titledim.y = 447 - 408;
      renderOutlinedText(card_image, *iter, title_font, pos, titledim, 0xFFE4A221, 0xFF000000);
      iter++;
    }
  txth = TTF_FontLineSkip(font);
  pos.x += 8;
  pos.y += 12;
  while (iter != m_textlist.end())
    {
      pos.y += txth;
      renderText(card_image, *iter, font, pos, 0xFF000000);
      iter++;
    }
  m_card_texture_name = string(m_area.m_name);
  ident = m_gfxprimitives->create_OpenGL_texture(card_image, m_card_texture_name);
  pp = new Cpicture(card_image, dim, m_card_texture_name, ident);
  m_gfxprimitives->add_picture(pp);
}

void Ccard::activate_card(bool active)
{
  m_bactive = active;
}

bool Ccard::is_activated()
{
  return m_bactive;
}

bool Ccard::is_in(t_coord pos)
{
  t_coord cpos;
  t_coord cdim;
  t_coord rpos;
  int     h;
  bool    res;

  m_area.get_pos(&cpos);
  m_area.get_dim(&cdim);
  // Is in the rotated card
  if (m_bactive)
    {
      rpos.x = cpos.x - (cdim.y - cdim.x) / 2;
      rpos.y = cpos.y + (cdim.y - cdim.x) / 2;
      h = cdim.y;
      cdim.y = cdim.x;
      cdim.x = h;
    }
  else
    {
      rpos.x = cpos.x;
      rpos.y = cpos.y;
    }
  res = (rpos.x <= pos.x && pos.x < rpos.x + cdim.x &&
	 rpos.y <= pos.y && pos.y < rpos.y + cdim.y);
  return (res);
}

bool Ccard::is_helper()
{
  return m_bhelper;
}

bool Ccard::same_name(char *name)
{
  return (strcmp(name, m_area.m_name) == 0);
}

void Ccard::add_info_line(string i)
{
  m_textlist.push_back(i);
}

void Ccard::resize_keep_ratio(t_coord new_window_size)
{
  m_area.resize_keep_ratio(new_window_size);
}

string Ccard::get_name()
{
  return m_card_texture_name;
}

void Ccard::mouse_over(bool bover, double timecode, erclick rclick, bool bmove)
{
  switch (m_state)
    {
    case ewait:
      {
	if (rclick == erclickstart && bover)
	  {
	    m_start_display_timecode = timecode;
	    m_state = eforceview;
	    return ;
	  }
	if (bover)
	  {
	    m_mouseovertimecode = timecode;
	    m_state = ewaitshow;
	  }
	else
	  m_mouseovertimecode = 0;
      }
      break;
    case eforceview:
      {
	if (rclick == erclickstop)
	  {
	    m_mouseofftimecode = timecode;
	    m_state = ewait;
	  }
      }
      break;
    case ewaitshow:
      {
	//printf("ewaitshwostate mover=%f\n", m_mouseovertimecode);
	if (rclick == erclickstart && bover)
	  {
	    m_start_display_timecode = timecode;
	    m_state = eforceview;
	    return ;
	  }
	if (/*bmove ||*/ !bover)
	  {
	    m_mouseovertimecode = 0; // Sort of additionnal state
	    m_state = ewait;
	    return ;
	  }
      }
      break;
    case eshow:
      {
	if (rclick == erclickstart && bover)
	  {
	    m_start_display_timecode = timecode;
	    m_state = eforceview;
	    return ;
	  }
	if (!bover)
	  {
	    m_mouseofftimecode = timecode;
	    m_state = ewait;
	  }
      }
      break;
    };
}

//------------------------------------------------------------------------------
//
// Reduced cards
//
//------------------------------------------------------------------------------

#define ENABLEDCARDC  0xAFDE5050
#define DISABLEDCARDC 0xAF5050DE

void Ccard::display_card_reduced(int w, int h, bool helpers_enabled, double curenttimecode)
{
#ifdef ICONISE
  t_fcoord  fpos, fdim, center;
  t_fcoord  fbpos, fbdim;
  float     rad;
  t_fcoord  foffset, fsubdim;
  bool      filtering = true;
  bool      blend = true;
  int       color;

  m_area.get_posf(&fpos);
  m_area.get_dimf(&fdim);
  fdim.x = fdim.x * m_reduced_sizefactor;
  fdim.y = fdim.y * m_reduced_sizefactor;
  color = m_bactive? ENABLEDCARDC : DISABLEDCARDC;
  rad = fdim.x / 40;
  fbpos = fpos;
  fbpos.x -= rad;
  fbpos.y -= rad;
  fbdim = fdim;
  fbdim.x += 2 * rad; 
  fbdim.y += 2 * rad;
  // Subset of the card
  foffset.x = 10;
  foffset.y = 63;
  fsubdim.x = 440;
  fsubdim.y = 405;
  //m_gfxprimitives->rounded_box(fbpos, fbdim, color, filtering, rad);
  m_gfxprimitives->draw_texture(m_card_texture_name, fpos, fdim, filtering, blend, &fsubdim, &foffset);
#else
  t_fcoord  fpos, fdim, dim, pos, center;
  bool      filtering = true;
  bool      blend = true;
  double    angle = 0;
  float     radius;
  int       i, color;

  m_area.get_posf(&fpos);
  m_area.get_dimf(&fdim);
  if (!is_helper() /*|| helpers_enabled*/)
    {
      if (!is_activated())
	{
	  m_gfxprimitives->draw_texture(m_card_texture_name, fpos, fdim, filtering, blend);
	}
      else
	{
	  center.x = fpos.x + fdim.x / 2;
	  center.y = fpos.y + fdim.y / 2;
	  color = CARDLIGHTC;
	  for (i = 1; i < 7; i++)
	    {
	      radius = fdim.x * (0.5 + (float)i * 0.09);
	      color = (color & 0x00FFFFFF) | ((0x89 / i) << 24);
	      m_gfxprimitives->disc(center, radius, color, true);
	    }
#ifdef HALOBOX
	  angle = CARDANGLE;
	  color = CARDLIGHTC;
	  for (i = 1; i < 2; i++)
	    {
	      float up = 1. + (0.10 * (float)i);
	      dim = fdim;
	      dim.x *= up;
	      dim.y *= up;
	      pos = fpos;
	      pos.x -= dim.x * (up - 1.) / 2.;
	      pos.y -= dim.y * (up - 1.) / 2.;
	      radius = dim.y / 16;
	      color = (color & 0x00FFFFFF) | ((0xAF / i) << 24);
	      m_gfxprimitives->moulagaufre_box(m_card_texture_name, pos, dim, angle, 0xFAFFE5D2, radius);
	    }
#endif
	  m_gfxprimitives->moulagaufre(m_card_texture_name, fpos, fdim, angle);
	}
    }
#endif
}

//------------------------------------------------------------------------------
//
// Rescaled cards
//
//------------------------------------------------------------------------------

void Ccard::get_dest(int scrw, int scrh, t_fcoord *start, t_fcoord *vect, float *pszfactor)
{
  t_fcoord apos, adim, middle, center;
  t_fcoord rdim, dest;
  float    sizef;

  m_area.get_posf(&apos);
  m_area.get_dimf(&adim);
  *start = apos;
  // Origin center
  center.x = apos.x + adim.x / 2.;
  center.y = apos.y + adim.y / 2.;
  // Destination size
  sizef = (*pszfactor + 1);
  rdim.x = adim.x * sizef;
  if (rdim.x > MAX_CARD_WIDTH)
    {
      rdim.x = MAX_CARD_WIDTH;
      sizef = rdim.x / adim.x;
      *pszfactor = sizef - 1.;
    }
  rdim.y = adim.y * sizef;
  // Destination center
  dest.x = center.x - rdim.x / 2.;
  dest.y = apos.y;
  if (apos.y > adim.y)
    dest.y -= (rdim.y - adim.y);
  else
    dest.y += 0;//(adim.y);
  // window limits
  dest.x = dest.x < 0?  0 : dest.x;
  dest.x = dest.x + rdim.x >= scrw? scrw - rdim.x : dest.x;
  dest.y = dest.y < 0?  0 : dest.y;
  dest.y = dest.y + rdim.y  > scrh? scrh - rdim.y : dest.y;
  //
  vect->x = (dest.x - start->x);
  vect->y = (dest.y - start->y);
  vect->z = 0;
  //m_gfxprimitives->line(start->x, start->y, start->x + vect->x, start->y + vect->y, 0xFFFFFFFF);
}

bool Ccard::translate_n_scale(double time, int scrw, int scrh, t_fcoord *ptranslate, t_fcoord *pscale)
{
  double   tlimit = DISPTIME; // seconds
  t_fcoord start, vect, translate;
  float    prog;
  t_fcoord startdim, scale;
  float    sizefactor = 8.;

  //printf("inspec %f\n", time);
  get_dest(scrw, scrh, &start, &vect, &sizefactor);
  time = time > tlimit? tlimit : time;
  prog = 1. - ((tlimit - time) / tlimit);
  // Translation
  m_area.get_posf(&translate);
  translate.x += prog * vect.x;// - (scrw / 50) * sin(M_PI * prog);
  translate.y += prog * vect.y;
  // Scaling
  m_area.get_dimf(&scale);
  scale.x *= (1. + sizefactor * prog);
  scale.y *= (1. + sizefactor * prog);
  //
  *ptranslate = translate;
  *pscale = scale;
  return true;
}

void Ccard::display_inspect(double timeover, int w, int h)
{
  t_fcoord  translate, scale, dim, pos;
  bool      filtering = true;
  bool      blend = true;
  float     radius, up, angle;
  int       i, color;

  translate_n_scale(timeover, w, h, &translate, &scale);
  //printf("translatex=%f y=%f size=%f %f.\n", translate.x, translate.y, scale.x, scale.y);
  if (is_activated())
    {
      angle = CARDANGLE;
      color = CARDLIGHTC;
      for (i = 1; i < 8; i++)
	{
	  up = 1. + (0.025 * (float)i);
	  dim = scale;
	  dim.x *= up;
	  dim.y *= up;
	  pos = translate;
	  pos.x -= dim.x * (up - 1.) / 2.;
	  pos.y -= dim.y * (up - 1.) / 2.;
	  radius = dim.y / 16;
	  color = (color & 0x00FFFFFF) | ((0xFF / i) << 24);
	  m_gfxprimitives->moulagaufre_box(pos, dim, angle, color, radius);
	}
      m_gfxprimitives->moulagaufre(m_card_texture_name, translate, scale, angle);
    }
  else
    m_gfxprimitives->draw_texture(m_card_texture_name, translate, scale, filtering, blend);
}

void Ccard::display_card_big(int w, int h, bool helpers_enabled, double curenttimecode)
{
  double delay;

  switch (m_state)
    {
    case ewait:
      {
	//return;
	// Reverse, the card is reduced again
	delay = (curenttimecode - m_mouseofftimecode);
	if (delay <= DISPTIME && !is_helper())
	  {
	    display_inspect(DISPTIME - delay, w, h);
	  }
      }
      break;
    case eforceview:
      {
	delay = (curenttimecode - m_start_display_timecode);
	display_inspect(delay, w, h);
      }
      break;
    case ewaitshow:
      {
	//printf("waiting %f to %f\n", curenttimecode - m_mouseovertimecode, INSPECT_THRSHOLD);
	if (curenttimecode - m_mouseovertimecode > INSPECT_THRSHOLD)
	  {
	    m_start_display_timecode = curenttimecode;
	    m_state = eshow;
	  }
      }
      break;
    case eshow:
      {
	if (!is_helper() || helpers_enabled)
	{
	  delay = (curenttimecode - m_start_display_timecode);
	  display_inspect(delay, w, h);
	}
/*
	if (timecode - m_mouseovertimecode > INSPECT_TIMEOUT)
	  {
	    m_mouseofftimecode = timecode;
	    m_state = ewait;
	  }
*/
      }
      break;
    };
}

