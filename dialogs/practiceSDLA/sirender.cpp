/*
 Scoreview (R)
 Copyright (C) 2015-2016 Patrick Areny
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
#include <assert.h>
#include <vector>

#ifdef __ANDROID__
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <GL/gl.h>
#endif

#include "stringcolor.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "score.h"
#include "pictures.h"
#include "f2n.h"
#include "stringcolor.h"
#include "fingerender.h"
#include "sirender.h"

// String Instrument renderer
// This is the base class for the string instruments with a board and neck.

using namespace std;

#define USE_STRING_TEXTURE

CsiRenderer::CsiRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list, int string_number):CFingerRenderer(pgfxprimitives, pfont, viewtime),
  m_string_number(string_number),
  m_mesh_list(pmesh_list)
{
  m_baseAbsNote = new int[m_string_number];
  m_playedy = new int[m_string_number];
  m_string_type = new int[m_string_number];
  m_bgcolor = BLACK;
}

CsiRenderer::~CsiRenderer()
{
  delete[] m_baseAbsNote;
  delete[] m_playedy;
  delete[] m_string_type;
}

t_fcoord CsiRenderer::resize(t_coord wsize, t_fcoord pos, t_fcoord dim)
{
  t_fcoord res;

  res.x = (pos.x + dim.x) * (float)wsize.x / 100.;
  res.y = (pos.y + dim.y) * (float)wsize.y / 100.;
  return res;
}

int CsiRenderer::blend(float xpos, int color, bool bleft)
{
  int   r, g, b, a;
  float rest, coef;

  r = (color >> 16) & 0xFF;
  g = (color >>  8) & 0xFF;
  b = (color >>  0) & 0xFF;
  //a = color >> 24;
  rest = xpos - floor(xpos);
  coef = bleft? (1 - rest) : rest;
  r = (float)r * coef;
  g = (float)g * coef;
  b = (float)b * coef;
  a = 0xFF000000 | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
  return a;
}

t_string_equation CsiRenderer::get_string(t_coord dim, int string_number)
{
  t_string_equation eq;
  float    ydiff;

  eq.start.y = 0;
  eq.start.x = string_x(dim, string_number, eq.start.y);
  eq.stop.y = dim.y;
  eq.stop.x = string_x(dim, string_number, eq.stop.y);
  //
  ydiff = (float)(eq.stop.y - eq.start.y);
  eq.xdiff = (float)(eq.stop.x - eq.start.x) / ydiff;
  return eq;
}

int CsiRenderer::get_time0_x_limit_from_y(t_coord dim, int y, bool left)
{
  int         string_number;
  int         x;
  float       fret_width;
  const float fretbordercoef = 0.07;

  fret_width = string_x(dim, 0, y) - string_x(dim, 5, y);
  if (left)
    {
      string_number = 5;
      x = string_x(dim, string_number, y) - fret_width * fretbordercoef;
    }
  else
    {
      string_number = 0;
      x = string_x(dim, string_number, y) + fret_width * fretbordercoef;
    }
  return x;
}

// Empty, do the real one int the subclasses
void CsiRenderer::draw_finger_board(Cgfxarea *pw, erlvl level)
{
}

int CsiRenderer::string_width(int string_number)
{
  if (string_number < 2)
    return string_number + 1;
  if (string_number == 2)
    return 2;
  return string_number - 1;
}

void CsiRenderer::render_string(Cgfxarea *pw, int string_number, int played_note)
{
  t_string_equation eq;
  t_coord  dim;
  int      color;
  int      ydiff, ystart;
  float    stringwidth;
  t_fcoord start, stop;
  bool     bwound;
  bool     bantialiased = true;
  int      points;
  t_fcoord pointarr[4];

  // uses a circle with a center past the strings attachments and angles to calculate string an frets coordinates.
  pw->get_dim(&dim);
  eq = get_string(dim, string_number);
  ydiff = eq.stop.y - eq.start.y;
  ystart = dim.y / 40;
  //
  // No blending, use a texture or a polygon
  stringwidth = string_width(string_number);
  start.y = ystart;
  start.x = string_x(dim, string_number, start.y) - (stringwidth / 2.);

  // Played string
  if (m_playedy[string_number] && (m_playedy[string_number] != -1))
    stop.y = m_playedy[string_number];
  else
    stop.y = ystart + ydiff;
  stop.x = string_x(dim, string_number, stop.y) - (stringwidth / 2.);
  //
  color = 0xFFCFCFCF;
  bwound = false;
  switch (m_string_type[string_number])
    {
    case elaser:
	color = 0xFF7F00FF;
      break;
    case ebowel:
    case enylon:
	color = 0xFF99FFFF;
      break;
    case esinglewire:
	color = 0xFFAFAFAF;
      break;
    case ewound:
      {
	color = 0xFFCFCFCF;
	bwound = true;
      }
      break;
    default:
	  break;
    }
  //bwound = false;
  if (bwound)
    {
      bool     filtering;
      bool     blend;
      t_fcoord subdim;

      filtering = blend = true;
      points = 4;
      //
      pointarr[0] = start;
      pointarr[1] = stop;
      pointarr[2] = stop;
      pointarr[2].x += stringwidth;
      pointarr[3] = start;
      pointarr[3].x += stringwidth;
      //
      subdim.x = 7.;
      subdim.y = (float)(stop.y - start.y);
      //
      m_gfxprimitives->draw_texture_quad(string("string"), pointarr, filtering, blend, &subdim);
      start = stop;
      stop.y = ystart + ydiff;
      stop.x = string_x(dim, string_number, stop.y) - (stringwidth / 2.);
      if (start.y != stop.y)
	{
	  pointarr[0] = start;
	  pointarr[1] = stop;
	  pointarr[2] = stop;
	  pointarr[2].x += stringwidth;
	  pointarr[3] = start;
	  pointarr[3].x += stringwidth;
	  subdim.x = 7.;
	  subdim.y = (float)(stop.y - start.y);
	  m_gfxprimitives->draw_texture_quad(string("string_played"), pointarr, filtering, blend, &subdim); //, t_fcoord *psubdim, t_fcoord *poffset)
	}
    }
  else
    {
      points = 3;

      pointarr[0] = start;
      pointarr[1] = stop;
      pointarr[2] = stop;
      pointarr[2].x += stringwidth;
      m_gfxprimitives->polygon(pointarr, points, color, bantialiased);
      pointarr[0] = start;
      pointarr[1] = stop;
      pointarr[1].x += stringwidth;
      pointarr[2] = start;
      pointarr[2].x += stringwidth;
      m_gfxprimitives->polygon(pointarr, points, color, bantialiased);
      
      start = stop;
      stop.y = ystart + ydiff;
      stop.x = string_x(dim, string_number, stop.y) - (stringwidth / 2.);
      if (start.y != stop.y)
	{
	  color = 0xFF0000FF;
	  pointarr[0] = start;
	  pointarr[1] = stop;
	  pointarr[2] = stop;
	  pointarr[2].x += stringwidth;
	  m_gfxprimitives->polygon(pointarr, points, color, bantialiased);
	  pointarr[0] = start;
	  pointarr[1] = stop;
	  pointarr[1].x += stringwidth;
	  pointarr[2] = start;
	  pointarr[2].x += stringwidth;
	  m_gfxprimitives->polygon(pointarr, points, color, bantialiased);
	}
    }
}

void CsiRenderer::draw_strings(Cgfxarea *pw)
{
  int i;

  for (i = 0; i < m_string_number; i++)
    render_string(pw, i, -1);
}

int CsiRenderer::note_2_y(t_coord dim, t_notefreq *pnotef)
{
  int   yoffset;
  float string_length;
  float constant;
  float freq;
  float y;

  yoffset = (4. * (float)dim.y / 100.);
  // The finger board makes 88% of the screen, say it is a Guarneri del Gesu then the string length
  // is (88% of the screen) * (1 + (2.2 / 10.3)). Even if Guarneri did not make electric guitars, it seems to be the same concept: 23 notes per string.
  string_length = (97. * (float)dim.y / 100.) * (1. + (2.2 / 10.3));
  // The full string is say A4, half string is A5, if we have f = Constant / L
  // (nearly constant: sqrt(T/µ) / 2L)
  constant = string_length * m_f2n.note2frequ(m_baseAbsNote[pnotef->string]);
  freq = pnotef->f;
  y = constant / freq;
  //printf("string_length=%f ", string_length);
  //printf("notelen=%f freq=%f\n", y, freq);
  y = yoffset + string_length - y;
  return (int)y;
}

int CsiRenderer::string_to_color(t_notefreq *pnf)
{
  int color;
  int string_number;
  int abs_note, note, octave;

  string_number = pnf->string;
  switch (string_number)
    {
    case 0:
      color = STRING0_COLOR;
      break;
    case 1:
      color = STRING1_COLOR;
      break;
    case 2:
      color = STRING2_COLOR;
      break;
    case 3:
      color = STRING3_COLOR;
      break;
    case 4:
      color = STRING4_COLOR;
      break;
    case 5:
    default:
      color = STRING5_COLOR;
      break;
    }
  abs_note = m_f2n.frequ2note(pnf->f);
  m_f2n.noteoctave(abs_note, &note, &octave);
  if (black_note(octave, note))
    {
      // FIXME this makes ugly colors
      color = reducecolor(color);
    }
  return color;
}

// It is like the strings a radiuses from a circle center way above the neck.
float CsiRenderer::string_x(t_coord dim, int string_number, int y)
{
  float    x;
  double   interstring_angle;
  double   interstring_spaces;

  interstring_spaces = ((double)m_string_number - 1.) / 2.;
  interstring_angle = m_s0angle / interstring_spaces;
  x = (((float)y - m_string_center.y) * atan(m_s0angle - interstring_angle * (double)string_number) + m_string_center.x);
  return x;
}

// Virtual
void CsiRenderer::add_drawings(t_coord dim, int y, CNote *pn, t_notefreq *pnf, t_limits *pl, int hnote_id)
{
  printf("Warning: CsiRenderer::add_drawings, virtual function.\n");
}

void CsiRenderer::check_visible_notes(Cgfxarea *pw, CScore *pscore, t_limits *pl, int hnote_id)
{
  std::list<t_notefreq>::iterator     notefreq_iter;
  CInstrument                        *pi;
  CNote                              *pn;
  t_coord                             pos, dim;
  double                              timecode;
  int                                 y;
  int                                 i;

  m_drawlist.clear();
  for (i = 0; i < m_string_number; i++)
    m_playedy[i] = -1;
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  timecode = pl->current;
  pi = m_instrument;
  pn = pi->get_first_note_reverse(timecode + m_viewtime);
  while (pn != NULL)
    {
      notefreq_iter = pn->m_freq_list.begin();
      while (notefreq_iter != pn->m_freq_list.end())
	{
	  if ((*notefreq_iter).string >= 0) // Exclude out of range notes
	    {
	      y = note_2_y(dim, &(*notefreq_iter));
	      add_drawings(dim, y, pn, &(*notefreq_iter), pl, hnote_id);
	    }
	  notefreq_iter++;
	}
      if (pn->m_time >= timecode + m_viewtime)
	break;
      pn = pi->get_next_note_reverse();
    }
}

int CsiRenderer::reducecolor(int color)
{
  int alpha = color & 0xFF000000;

  color = (color >> 1) & 0x007F7F7F;
  return color | alpha;
}

void CsiRenderer::draw_note_drawings(erlvl level, int hnote_id)
{
  std::list<t_note_drawing>::iterator iter;
  t_note_drawing                     *pd;
  int                                 color, masked_outline_color;
  t_fcoord                            center;
  t_fcoord                            fpos, fdim;
  bool                                bantialiased;

  bantialiased = true;
  iter = m_drawlist.begin();
  while (iter != m_drawlist.end())
    {
      pd = &(*iter);
      if (pd->bcircle && pd->lvl == level)
	{
	  color = pd->color;
	  // If pressed but masked by another note then reduce the color
	  if (pd->y < m_playedy[pd->string] && pd->lvl == lvl_top)
	    {
	      color = STRING_GRAY; 
	      //color = reducecolor(color);
	      masked_outline_color = (hnote_id == pd->note_id)? WHITE : NOTE_COLOR; //BORDEAUX;
	    }
	  else
	    {
	      masked_outline_color = (hnote_id == pd->note_id)? WHITE : NOTE_COLOR;
	    }
	  center.x = pd->x;
	  center.y = pd->y;
	  m_gfxprimitives->disc(center, pd->rad, masked_outline_color, bantialiased);
	  m_gfxprimitives->disc(center, pd->rad - pd->rad / 4.5, color, bantialiased);
	}
      else
	{
	  //printf("drawing note box rad == %f\n", pd->rad);
	  if (pd->lvl == level)
	    {
	      // Outline
	      color = pd->color;
	      fpos.x = pd->x;
	      fpos.y = pd->y;
	      fdim.x = pd->w;
	      fdim.y = pd->h;
	      m_gfxprimitives->box(fpos, fdim, color, bantialiased);
	    }
	}
      iter++;
    }
}

void CsiRenderer::Draw_the_measure_bars(CScore *pscore, t_coord pos, t_coord dim, double timecode)
{
  std::list<CMesure*>::reverse_iterator it;
  CMesure *pm;
  double   previoustcode;
  double   bartime;
  int      y, x, xt0r, xt1r;
  float    slope;
  float    xsto;

  //
  y = 0;
  xt0r = get_time0_x_limit_from_y(dim, y, false);
  xt1r = get_time0_x_limit_from_y(dim, dim.y, false);
  slope = (float)(xt1r - xt0r) / (float)dim.y;
  previoustcode = timecode + m_viewtime;
  it = pscore->m_mesure_list.rbegin();
  // Assuming the list is sorted then the last measure segment begining
  // before the note is the needed one. Hence reverse order iterator.
  while (it != pscore->m_mesure_list.rend())
    {
      pm = (*it);
      if (pm->m_start < previoustcode)
	{
	  bartime = pm->m_start;
	  while (bartime < previoustcode)
	    {
	      if (bartime > timecode &&
		  bartime < timecode + m_viewtime)
		{
		  x = (bartime - timecode) * (float)(dim.x - xt0r) / m_viewtime;
		  x += xt0r;
		  xsto = x + slope * dim.y;
		  m_gfxprimitives->line(x, 0, xsto, dim.y, VBARCOLOR);
		  // Double the first bar
		  if (bartime == pm->m_start)
		    m_gfxprimitives->line(x + 6, 0, xsto + 6, dim.y, BLACK);
		}
	      bartime += pm->duration();
	    }
	  previoustcode = pm->m_start;
	}
      it++;
    }
}

void CsiRenderer::Draw_the_practice_end(t_coord pos, t_coord dim, t_limits *pl)
{
  double timecode;
  double end;
  int    y, x, xt0r, xt1r;
  float  slope;
  float  xsto;

  y = 0;
  xt0r = get_time0_x_limit_from_y(dim, y, false);
  xt1r = get_time0_x_limit_from_y(dim, dim.y, false);
  slope = (float)(xt1r - xt0r) / (float)dim.y;
  timecode = pl->current;
  end = pl->stop;
  if (end > timecode && end < timecode + m_viewtime)
    {
      x = (end - timecode) * (float)(dim.x - xt0r) / m_viewtime;
      x += xt0r;
      xsto = x + slope * dim.y;
      m_gfxprimitives->line(x, 0, xsto, dim.y, BLACK);
    }
}

void CsiRenderer::print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime)
{
  char     text[TIMETXTSZ];
  t_coord  pos;
  t_coord  dim;
  t_coord  linepos;
  t_fcoord fpos, fdim;
  double   minutes;
  double   seconds;
  double   mseconds;
  double   tbegin, tplay;
  bool     blended;
  int      outlinecolor;
  int      w, h, texth, textw;
  float    outline;

  //--------------------------------------------
  // Render the timecode
  //--------------------------------------------
  tbegin = timecode + viewtime;
  minutes = floor(tbegin / 60.);
  seconds = tbegin - (minutes * 60.);
  mseconds = seconds;
  mseconds -= seconds;
  mseconds *= 100;
  if (minutes == 0)
    snprintf(text, TIMETXTSZ, "     %- 2.1fs", seconds);
  else
    snprintf(text, TIMETXTSZ, "%- 3.0f:%- 2.1fs", minutes, seconds);
  if (TTF_SizeText(m_font, text, &w, &h) == 0)
    {
      texth = h;
      textw = w;
    }
  else
    {
      texth = 15;
      textw = 80;
    }
  pw->get_posf(&fpos);
  pw->get_dimf(&fdim);
  outline = 0;
  //
  fpos.x = fdim.x - textw;
  fdim.x = textw;
  fdim.y = texth;
  blended = true;
  //color   = WHITE;
  outlinecolor = BLACK;
  m_gfxprimitives->print(text, m_font, fpos, fdim, color, blended, outline, outlinecolor);
  // Play time
  tplay  = timecode;
  minutes = floor(tplay / 60.);
  seconds = tplay - (minutes * 60.);
  mseconds = seconds;
  mseconds -= seconds;
  mseconds *= 100;
  if (minutes == 0)
    snprintf(text, TIMETXTSZ, "     %- 2.1fs", seconds);
  else
    snprintf(text, TIMETXTSZ, "%- 3.0f:%- 2.1fs", minutes, seconds);
  if (TTF_SizeText(m_font, text, &w, &h) == 0)
    {
      texth = h;
      textw = w;
    }
  else
    {
      texth = 15;
      textw = 80;
    }
  pw->get_posf(&fpos);
  pw->get_dimf(&fdim);
  outline = 0;
  //
  fpos.x = fdim.x * 0.32;
  fdim.x = textw;
  fdim.y = texth;
  blended = true;
  //color   = WHITE;
  outlinecolor = BLACK;
  m_gfxprimitives->print(text, m_font, fpos, fdim, color, blended, outline, outlinecolor);
}

// Useless
void CsiRenderer::render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl, int hnote_id)
{
  printf("Warning: CsiRenderer::render, virtual function.\n");
}

