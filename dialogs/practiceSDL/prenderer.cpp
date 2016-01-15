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

#include <assert.h>
#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include "env.h"
#include "gfxareas.h"
#include "mesh.h"
#include "stringcolor.h"
#include "gl2Dprimitives.h"
#include "score.h"
#include "pictures.h"
#include "f2n.h"
#include "fingerender.h"
#include "prenderer.h"

// This is just a loosely related remake of synthesia visuals

using namespace std;

CpRenderer::CpRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime):CFingerRenderer(pgfxprimitives, pfont, viewtime)
{
  m_picturelist = new CpictureList(pgfxprimitives);
  m_picturelist->open_practice_drawings(string(IMGPATH) + string("images.png"));
  m_bgcolor = BACKGROUND;
}

CpRenderer::~CpRenderer()
{
  delete m_picturelist;
}

int CpRenderer::reduce_color(int color, float coef)
{
  float       r, g, b, a;
#ifdef SATURATION
  float       P;
  const float Pr = 0.299;
  const float Pg = 0.587;
  const float Pb = 0.114;
#endif
  
  a = ((color >> 24) & 0xFF);
  b = (color >> 16) & 0xFF;
  g = (color >>  8) & 0xFF;
  r = color & 0xFF;
#ifdef SATURATION
  P = sqrt(r * r * Pr + g * g * Pg + b * b * Pb);
  a = a * coef;
  g = P + (g - P) * coef;
  b = P + (b - P) * coef;
  r = P + (r - P) * coef;
  //a = 90;
#else
  a = a * coef;
  g = g * coef;
  b = b * coef;
  r = r * coef;
#endif
  return ((((int)a & 0xFF) << 24) | (((int)b & 0xFF) << 16) | (((int)g & 0xFF) << 8) | ((int)r & 0xFF));
}

int CpRenderer::brighten(int color)
{
  float r, g, b, a;
  float coef = 0.75;
  
  a = ((color >> 24) & 0xFF);
  b = (color >> 16) & 0xFF;
  g = (color >>  8) & 0xFF;
  r = color & 0xFF;
  //
  //a = a + (256 - a) * coef;
  g += (g) * coef;
  g = g > 255? 255 : g;
  b += (b) * coef;
  b = b > 255? 255 : b;
  r += (r) * coef;
  r = r > 255? 255 : r;
  return ((((int)a & 0xFF) << 24) | (((int)b & 0xFF) << 16) | (((int)g & 0xFF) << 8) | ((int)r & 0xFF));
}

void CpRenderer::render_texture(t_fcoord pos, t_fcoord dim, int color, string name)
{
  bool filtering;
  bool blend;

  filtering = true;
  blend = false;
  m_gfxprimitives->draw_texture(name, pos, dim, filtering, blend);
  m_gfxprimitives->box(pos, dim, color, filtering);
}

void CpRenderer::render_key(int color, t_coord pos, float sizex, float sizey, string name)
{
  t_fcoord fpos, fdim;

  fpos.x = pos.x - (sizex / 2);
  fpos.y = pos.y;
  fdim.x = sizex;
  fdim.y = sizey;
  render_texture(fpos, fdim, color, name);
}

void CpRenderer::render_white_key(int color, t_coord pos)
{
  render_key(color, pos, m_key_sizex, m_key_sizey, std::string("whitekey"));
}

void CpRenderer::render_black_key(int color, t_coord pos)
{
  pos.y -= m_bkey_sizey / 40;
  render_key(color, pos, m_bkey_sizex, m_bkey_sizey, std::string("blackkey"));
}

void CpRenderer::render_black_key_to_be_pressed(int color, t_coord pos)
{
  pos.y -= m_bkey_sizey / 40;
  render_key(color, pos, m_bkey_sizex, m_bkey_sizey, std::string("blackkeytobepressed"));
}

void CpRenderer::render_white_key_pressed(int color, t_coord pos)
{
  render_key(color, pos, m_key_sizex, m_key_sizey, std::string("whitekeypushed"));
}

void CpRenderer::render_black_key_pressed(int color, t_coord pos)
{
  pos.y -= m_bkey_sizey / 40;
  render_key(color, pos, m_bkey_sizex, m_bkey_sizey, std::string("blackkeypushed"));
}

void CpRenderer::Draw_the_keys_notes_limit(t_coord pos, t_coord dim)
{
  t_fcoord fpos, fdim;
  int      color = 0xFF303030;
  bool     bantialiased = true;

  m_keylimity = dim.y *  3. / 4.;
  m_keylimitsizey = dim.y / 40.;
  m_keystarty = m_keylimity + m_keylimitsizey;
  fpos.x = 0;
  fpos.y = m_keylimity;
  fdim.x = dim.x;
  fdim.y = m_keylimitsizey;
  m_gfxprimitives->box(fpos, fdim, color, bantialiased);
}

void CpRenderer::Draw_the_measure_bars(CScore *pscore, t_coord pos, t_coord dim, double timecode)
{
  std::list<CMesure*>::reverse_iterator it;
  CMesure *pm;
  double   previoustcode;
  double   bartime;
  int      y;

  previoustcode = timecode;
  it = pscore->m_mesure_list.rbegin();
  // Assuming the list is sorted then the last measure segment begining
  // before the note is the needed one. Hence reverse order iterator.
  while (it != pscore->m_mesure_list.rend())
    {
      pm = (*it);
      if (pm->m_start < timecode + m_viewtime)
	{
	  bartime = pm->m_start;
	  while (bartime < previoustcode + m_viewtime)
	    {
	      if (bartime > timecode &&
		  bartime < timecode + m_viewtime)
		{
		  y = ((timecode + m_viewtime)- bartime) * m_keylimity / m_viewtime;
		  m_gfxprimitives->hline(0, y, dim.x, BARCOLOR);
		}
	      bartime += pm->duration();
	    }
	  previoustcode = pm->m_start;
	}
      it++;
    }
}

void CpRenderer::Draw_the_practice_end(t_coord pos, t_coord dim, t_limits *pl)
{
  double timecode;
  double end;
  int    y;

  timecode = pl->current;
  end = pl->stop;
  if (end > timecode && end < timecode + m_viewtime)
    {
      y = ((timecode + m_viewtime) - end) * m_keylimity / m_viewtime;
      m_gfxprimitives->hline(0, y, dim.x, BLACK);
    }
}

void CpRenderer::Draw_the_octave_limits(t_coord pos, t_coord dim)
{
  int   i;
  float offset;

  offset = 2. * m_key_sizex;
  for (i = 0; i < 8; i++)
    {
      m_gfxprimitives->vline(offset, 0, m_keylimity, LINECOLOR);
      offset += 7. * m_key_sizex;
    }
}

int CpRenderer::time_to_y(double note_time, double timecode)
{
  if (note_time >= timecode + m_viewtime)
    {
      return 0;
    }
  if (note_time < timecode)
    {
      return m_keylimity;
    }
  return (m_keylimity * ((timecode + m_viewtime) - note_time) / m_viewtime);
}

void CpRenderer::fill_note_segments_list(CScore *pscore, t_coord pos, t_coord dim, t_limits *pl, int hnote_id)
{
  std::list<t_notefreq>::iterator iter;
  CInstrument                    *pi;
  CNote                          *pn;
  int                             i;
  int                             abs_note;
  int                             octave, note;
  int                             piano_note;
  t_tobeplayed                    tp;
  float                           freq;
  double                          timecode;

  for (i = 0; i < PIANO_NOTE_NUMBER; i++)
    {
      m_keyboard[i].btobeplayed = false;
      m_keyboard[i].brighthand = false;
    }
  m_playlist.clear();
  timecode = pl->current;
  pi = m_instrument;
  pn = pi->get_first_note(0.);
  while (pn != NULL)
    {
      if (pn->m_time + pn->m_duration >= timecode && pn->m_time < timecode + m_viewtime)
	{
	  //printf("addnote notetime=%f ends=%f to begin=%f end=%f\n", pn->m_time, pn->m_time + pn->m_duration, timecode, timecode + m_viewtime);
	  iter = pn->m_freq_list.begin();
	  while (iter != pn->m_freq_list.end())
	    {
	      freq = (*iter).f;
	      if ((pl->lof < freq && freq < pl->hif) && (*iter).string != -1)
		{
		  abs_note = m_f2n.frequ2note(freq);
		  m_f2n.noteoctave(abs_note, &note, &octave);
		  piano_note = m_f2n.pianonotenum(octave, note);
		  if (piano_note < 0 || piano_note > PIANO_NOTE_NUMBER)
		    {
		      printf("note error note is=%d freq is=%f\n", piano_note, freq);
		      exit(1);
		    }
		  tp.abs_note = piano_note;
		  tp.bhighlighted = (hnote_id == pn->identifier());
		  tp.brighthand = (((*iter).string & 1) == 1);
		  tp.ystart = time_to_y(pn->m_time + pn->m_duration, timecode);
		  tp.ystop =  time_to_y(pn->m_time, timecode);
		  //
		  tp.x  = m_key_sizex * 7 * octave;
		  tp.x += m_key_sizex * (float)(note + 1) / 2.;
		  tp.x += (note > 4)? m_key_sizex / 2. : 0;
		  tp.x -= 5. * m_key_sizex;
		  if (black_note(octave, note))
		    {
		      tp.x -= floor(m_bkey_sizex / 2.);
		      tp.w = m_bkey_sizex * 0.8; // ???
		      tp.h = m_bkey_sizey;
		    }
		  else
		    {
		      tp.x -= floor(m_key_sizex / 2.);
		      tp.w = m_key_sizex;
		      tp.h = m_key_sizey;
		    }
		  m_playlist.push_front(tp);
		  m_keyboard[piano_note].btobeplayed = true;
		  m_keyboard[piano_note].brighthand = tp.brighthand;
		}
	      iter++;
	    }
	}
      if (pn->m_time >= timecode + m_viewtime)
	break;
      pn = pi->get_next_note();
    }
}

//#define USEROUNDEDBOXES
void CpRenderer::Draw_the_notes(t_coord pos, t_coord dim, bool bblack)
{
  std::list<t_tobeplayed>::iterator iter;
  t_tobeplayed                     *tbp;
  //int                               rad;
  int                               color;
  bool                              bantialiased = true;
  t_fcoord                          fpos, fdim;
#ifdef USEROUNDEDBOXES
  float                             radius;
#endif

  iter = m_playlist.begin();
  //rad = 2;
  while (iter != m_playlist.end())
    {
      tbp = &(*iter);

      if (tbp->w == (int)m_key_sizex) // white note
	{
	  if (!bblack)
	    {
	      color = tbp->brighthand? RIGHT_HAND_COLOR_W : LEFT_HAND_COLOR_W;
	      if (tbp->bhighlighted)
		{
		  color = brighten(color);
		}
#ifdef USEROUNDEDBOXES
	      fpos.x = tbp->x;
	      fpos.y = tbp->ystart;
	      fdim.x = tbp->w;
	      fdim.y = tbp->ystop - tbp->ystart;
	      radius = fdim.x / 4;
	      m_gfxprimitives->rounded_box(fpos, fdim, color, bantialiased, radius);
#else
	      m_gfxprimitives->box(tbp->x, tbp->ystart, tbp->x + tbp->w, tbp->ystop, color, bantialiased);
#endif
	    }
	}
      else
	{
	  if (bblack)
	    {
	      color = tbp->brighthand? RIGHT_HAND_COLOR_B : LEFT_HAND_COLOR_B;
	      if (tbp->bhighlighted)
		{
		  color = brighten(color);
		}
#ifdef USEROUNDEDBOXES
	      fpos.x = tbp->x;
	      fpos.y = tbp->ystart;
	      fdim.x = tbp->w;
	      fdim.y = tbp->ystop - tbp->ystart;
	      radius = fdim.x / 4;
	      m_gfxprimitives->rounded_box(fpos, fdim, color, bantialiased, radius);
#else
	      m_gfxprimitives->box(tbp->x, tbp->ystart, tbp->x + tbp->w, tbp->ystop, color, bantialiased);
#endif
	    }
	}
      iter++;
    }
}

void CpRenderer::check_played(CScore *pscore, t_limits *pl)
{
  std::list<t_notefreq>::iterator iter;
  CInstrument                    *pi;
  CNote                          *pn;
  int                             i;
  int                             abs_note;
  int                             octave, note;
  int                             piano_note;
  double                          timecode;
  float                           freq;
  
  for (i = 0; i < PIANO_NOTE_NUMBER; i++)
    {
      m_keyboard[i].bplayed = false;
    }
  timecode = pl->current;
  pi = m_instrument;
  pn = pi->get_first_note(0.);
  while (pn != NULL)
    {
      if (pn->m_time <= timecode &&
	  pn->m_time + pn->m_duration >= timecode)
	{
	  iter = pn->m_freq_list.begin();
	  while (iter != pn->m_freq_list.end())
	    {
	      freq = (*iter).f;
	      if (pl->lof < freq && freq < pl->hif)
		{
		  abs_note = m_f2n.frequ2note(freq);
		  m_f2n.noteoctave(abs_note, &note, &octave);
		  piano_note = m_f2n.pianonotenum(octave, note);
		  assert(piano_note >= 0 && piano_note < PIANO_NOTE_NUMBER);
		  m_keyboard[piano_note].bplayed = true;
		  m_keyboard[piano_note].brighthand = ((*iter).string == 1);
		}
	      iter++;
	    }
	}
      pn = pi->get_next_note();
    }
}

void CpRenderer::Draw_the_keys_being_played(t_coord dim, bool bblack)
{
  int     abs_note;
  int     octave, note;
  t_coord pos;
  int     color;
  float   x;

  pos.y = m_keystarty;
  x = m_key_sizex / 2.;
  for (abs_note = 0; abs_note < PIANO_NOTE_NUMBER; abs_note++)
    {
      pos.x = x;
      m_f2n.noteoctave(abs_note + 9, &note, &octave);
      color = DEFAULT_COLOR_W;
      if (black_note(octave, note))
	{	
	  if (bblack)
	    {
	      if (m_keyboard[abs_note].bplayed)
		{
		  color = m_keyboard[abs_note].brighthand? RIGHT_HAND_COLOR_B : LEFT_HAND_COLOR_B;
		  color = reduce_color(color, 0.6);
		  render_black_key_pressed(color, pos);
		}
	      else
		{
		  color = DEFAULT_COLOR_B;
		  if (m_keyboard[abs_note].btobeplayed)
		    {
		      color = m_keyboard[abs_note].brighthand? RIGHT_HAND_COLOR_W : LEFT_HAND_COLOR_W;
		      color = reduce_color(color, 0.5);
		      //render_black_key_to_be_pressed(color, pos);
		    }
		  //else
		    render_black_key(color, pos);
		}
	    }
	}
      else
	{
	  if (!bblack)
	    {
	      if (m_keyboard[abs_note].bplayed)
		{
		  color = m_keyboard[abs_note].brighthand? RIGHT_HAND_COLOR_W : LEFT_HAND_COLOR_W;
		  color = reduce_color(color, 1);
		  render_white_key_pressed(color, pos);
		}
	      else
		{
		  color = DEFAULT_COLOR_W;
		  if (m_keyboard[abs_note].btobeplayed)
		    {
		      color = m_keyboard[abs_note].brighthand? RIGHT_HAND_COLOR_W : LEFT_HAND_COLOR_W;
		      color = reduce_color(color, 0.4);
		    }
		  render_white_key(color, pos);
		}
	    }
	}
      if (note == 4 || note == 11)
	x += m_key_sizex;
      else
	x += m_key_sizex / 2.;
    }
}

void CpRenderer::print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime)
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
    sprintf(text, "     %- 2.1fs", seconds);
  else
    sprintf(text, "%- 3.0f:%- 2.1fs", minutes, seconds);
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
    sprintf(text, "     %- 2.1fs", seconds);
  else
    sprintf(text, "%- 3.0f:%- 2.1fs", minutes, seconds);
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
  fpos.y = m_keylimity - texth;
  fdim.x = textw;
  fdim.y = texth;
  blended = true;
  //color   = WHITE;
  outlinecolor = BLACK;
  m_gfxprimitives->print(text, m_font, fpos, fdim, color, blended, outline, outlinecolor);
}

void CpRenderer::render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl, int hnote_id)
{
  t_coord pos;
  t_coord dim;
  float   ratio;

  //printf("time=%f\n", pl->current);
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  clear_all();
  m_instrument = pscore->get_instrument(instrument_name, instru_identifier);
  if (m_instrument == NULL)
    return ;
  Draw_the_keys_notes_limit(pos, dim);
  m_key_sizex = (float)dim.x / (float)PIANO_NOTE_WIDTHS;
  m_key_sizey = dim.y - m_keystarty;
  ratio = 0.8;
  m_bkey_sizex = m_key_sizex * ratio;
  m_bkey_sizey = m_key_sizey * ratio * 0.85;
  Draw_the_measure_bars(pscore, pos, dim, pl->current);
  Draw_the_practice_end(pos, dim, pl);
  fill_note_segments_list(pscore, pos, dim, pl, hnote_id);
  Draw_the_notes(pos, dim, false);
  Draw_the_notes(pos, dim, true);
  Draw_the_octave_limits(pos, dim);
  check_played(pscore, pl);
  Draw_the_keys_being_played(dim, false);
  Draw_the_keys_being_played(dim, true);
  print_current_timecodes(pw, WHITE, pl->current, m_viewtime);
}

