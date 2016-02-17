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
#include "vrender.h"

// This is just a loosely related remake of synthesia visuals
using namespace std;

#define VIOLIN_STRINGS 4

CvRenderer::CvRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list):CsiRenderer(pgfxprimitives, pfont, viewtime, pmesh_list, VIOLIN_STRINGS)
{
  int octave;
  int note;

  //
  // E5
  octave = 5;
  note = 4;
  m_baseAbsNote[0] = m_f2n.notenum(octave, note);
  // A4
  octave = 4;
  note = 9;
  m_baseAbsNote[1] = m_f2n.notenum(octave, note);
  // D4
  octave = 4;
  note = 2;
  m_baseAbsNote[2] = m_f2n.notenum(octave, note);
  // G3
  octave = 3;
  note = 7;
  m_baseAbsNote[3] = m_f2n.notenum(octave, note);
  //
  //m_gfxprimitives->set_clear_color(VBACKGROUND);
  m_bgcolor = VBACKGROUND;
  //
  m_string_type[0] = esinglewire;
  m_string_type[1] = esinglewire;
  m_string_type[2] = ewound;
  m_string_type[3] = ewound;
}

CvRenderer::~CvRenderer()
{
  
}

void CvRenderer::draw_finger_board(Cgfxarea *pw, erlvl level)
{
  bool     bantialiased;
  t_fcoord offset;
  t_fcoord pos, dim;
  CMesh   *pmesh;

  bantialiased = false;
  pw->get_dimf(&dim);
/* For fingerboard only, when removing pegbox and body from the obj file
  pos.x = dim.x / 2.92;
  pos.y = dim.y / 20;
  dim.x = dim.x / 5;
  dim.y = dim.y - 3 * pos.y;
*/
  pos.x = dim.x / 2.92;
  pos.y = dim.y / 20;
  dim.x = dim.x / 2;
  dim.y = dim.y * 1.3 - 3 * pos.y;
  if (level == lvl_background && false)
    {
      pmesh = m_mesh_list->get_mesh(string("body"));
      m_gfxprimitives->triangle_mesh2D(pos, dim, pmesh, TABLE_COLOR, bantialiased);
    }
  if (level == lvl_fingerboard)
    {
      pmesh = m_mesh_list->get_mesh(string("pegbox"));
      m_gfxprimitives->triangle_mesh2D(pos, dim, pmesh, TABLE_COLOR, bantialiased);
      pmesh = m_mesh_list->get_mesh(string("pegbox2"));
      m_gfxprimitives->triangle_mesh2D(pos, dim, pmesh, PEGBOX_COLOR, bantialiased);
      pmesh = m_mesh_list->get_mesh(string("vfingerboard"));
      m_gfxprimitives->triangle_mesh2D(pos, dim, pmesh, EBONY_COLOR, bantialiased);
      //pos.y += 10;
      pmesh = m_mesh_list->get_mesh(string("vfingerboardside"));
      m_gfxprimitives->triangle_mesh2D(pos, dim, pmesh, EBONY_SIDE_COLOR, bantialiased);
    }
}

void CvRenderer::add_drawings(t_coord dim, int y, CNote *pn, t_notefreq *pnf, t_limits *pl, int hnote_id)
{
  int            noteheigth;
  int            xt0l, xt0r;
  int            newx;
  const int      cmarg = 25;
  double         timecode, proxi;
  t_note_drawing nd;
  float          pix_s;
  float          passed_viewtime;

  if ((pl->lof >= pnf->f || pnf->f >= pl->hif))
    {
      return;
    }
  timecode = pl->current;
  noteheigth = (dim.y * 88. / 100) / 30.; // average of 30 notes on the fingerboard
  xt0r = get_time0_x_limit_from_y(dim, y, false);
  xt0l = get_time0_x_limit_from_y(dim, y, true);
  pix_s = (dim.x - xt0r) / m_viewtime;
  passed_viewtime = xt0l * pix_s;
  if (pn->m_time + pn->m_duration >= timecode && pn->m_time < timecode + m_viewtime)
    {
      // Coming note, right segment
      nd.lvl = lvl_background;
      nd.note_id = pn->identifier();
      nd.bcircle = false;
      nd.string = pnf->string;
      nd.color = string_to_color(pnf);
      nd.h = noteheigth;
      nd.y = y - nd.h / 2;
      nd.w = pix_s * pn->m_duration;
      pix_s = (dim.x - xt0r) / m_viewtime;
      nd.x = xt0r + pix_s * (pn->m_time - timecode);
      if (nd.x < xt0r - cmarg)
	{
	  newx = xt0r - cmarg;
	  nd.w = nd.w - abs(nd.x - newx);
	  nd.x = newx;
	}
      //nd.rad = nd.h < nd.w? nd.h : nd.w;
      if (nd.w < nd.h) // Add a circle instead
	{
	  nd.bcircle = true;
	  nd.rad = nd.h / 2.;
	  nd.y += nd.rad;
	}
      else
	nd.rad = nd.h / 2.5;
      m_drawlist.push_back(nd);
      //printf("adding %s note x=%d y=%d w=%d h=%d dimw=%d dimh=%d\n", "coming", nd.x, nd.y, nd.w, nd.h, dim.x, dim.y);
      // Add a gray little circle on the board
      if (fabs(m_f2n.note2frequ(m_baseAbsNote[pnf->string]) - pnf->f) > 0.5)
	{
	  nd.lvl = lvl_fingerboard;
	  nd.bcircle = true;
	  proxi = pn->m_time < timecode? timecode : pn->m_time;
	  proxi = timecode + m_viewtime - proxi;
	  proxi = proxi < 0? 0 : proxi / 2.;
	  nd.rad = (float)noteheigth * (proxi / m_viewtime);
	  nd.string = pnf->string;
	  nd.color = STRING_GRAY;
	  nd.y = y;
	  nd.x = string_x(dim, nd.string, nd.y);
	  m_drawlist.push_back(nd);
	}
    }
  if (pn->m_time <= timecode && pn->m_time + pn->m_duration >= timecode)
    {
      // Note being played
      if (fabs(m_f2n.note2frequ(m_baseAbsNote[pnf->string]) - pnf->f) >= 0.)
	{
	  nd.note_id = pn->identifier();
	  nd.lvl = lvl_top;
	  nd.bcircle = true;
	  nd.rad = noteheigth / 2;
	  nd.string = pnf->string;
	  nd.color = string_to_color(pnf);
	  nd.y = y;
	  // x pos of the string
	  nd.x = string_x(dim, nd.string, nd.y);
	  m_drawlist.push_back(nd);
	  //printf("adding %s note x=%d y=%d w=%d h=%d dimw=%d dimh=%d\n", "in", nd.x, nd.y, nd.w, nd.h, dim.x, dim.y);
	}
      m_playedy[pnf->string] = m_playedy[pnf->string] > y? m_playedy[pnf->string] : y;
    }
  if (false && pn->m_time < timecode && pn->m_time + pn->m_duration > timecode - passed_viewtime)
    {
      // Passed note segment
      nd.note_id = pn->identifier();
      nd.lvl = lvl_background;
      nd.bcircle = false;
      nd.string = pnf->string;
      nd.color = string_to_color(pnf);
      nd.y = y;
      pix_s = xt0l / m_viewtime;
      nd.x = xt0l + pix_s * (timecode - pn->m_time);
      nd.w = pix_s * (pn->m_time + pn->m_duration);
      nd.w = nd.w < xt0l? xt0l : nd.w;
      nd.h = noteheigth;
      nd.rad = nd.h < nd.w? nd.h : nd.w;
      nd.rad /= 3.;
      m_drawlist.push_back(nd);
      //printf("adding %s note x=%d y=%d w=%d h=%d dimw=%d dimh=%d\n", "passed", nd.x, nd.y, nd.w, nd.h, dim.x, dim.y);
    }
}

int CvRenderer::string_width(int string_number)
{
  return 2 + string_number;
}

void CvRenderer::draw_note_drawings(erlvl level, int hnote_id)
{
  std::list<t_note_drawing>::iterator iter;
  t_note_drawing                     *pd;
  int                                 color, masked_outline_color;
  t_fcoord                            center;
  t_fcoord                            fpos, fdim;
  bool                                bantialiased;
  float                               offset;

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
	      color = STRING_GRAY; //reducecolor(color);
	      masked_outline_color = (hnote_id == pd->note_id)? WHITE : NOTE_COLOR; //BORDEAUX;
	    }
	  else
	    {
	      masked_outline_color = (hnote_id == pd->note_id)? WHITE : NOTE_COLOR;
	    }
	  center.x = pd->x;
	  center.y = pd->y;
	  m_gfxprimitives->disc(center, pd->rad + pd->rad / 4., masked_outline_color, bantialiased);
	  m_gfxprimitives->disc(center, pd->rad, color, bantialiased);
	}
      else
	{
	  //printf("drawing note box rad == %f\n", pd->rad);
	  if (pd->lvl == level)
	    {
	      // Outline
	      color = pd->color;
	      offset = pd->h / 8;
	      fpos.x = pd->x;
	      fpos.y = pd->y;
	      fdim.x = pd->w;
	      fdim.y = pd->h;
	      fdim.x += 2 * offset;
	      fdim.y += 2 * offset;
	      fpos.x -= offset;
	      fpos.y -= offset;
	      masked_outline_color = (hnote_id == pd->note_id)? WHITE : NOTE_COLOR;
	      if (pd->rad > 4)
		m_gfxprimitives->rounded_box(fpos, fdim, masked_outline_color, bantialiased, pd->rad);
	      else
		m_gfxprimitives->box(fpos, fdim, masked_outline_color, bantialiased);
	      fpos.x = pd->x;
	      fpos.y = pd->y;
	      fdim.x = pd->w;
	      fdim.y = pd->h;
	      if (pd->rad > 4)
		m_gfxprimitives->rounded_box(fpos, fdim, color, bantialiased, pd->rad);
	      else
		m_gfxprimitives->box(fpos, fdim, color, bantialiased);
	    }
	}
      iter++;
    }
}

void CvRenderer::render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl, int hnote_id)
{
  t_coord pos;
  t_coord dim;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  clear_all();
  m_string_center.x = dim.x / 2.92;
  m_string_center.y = -0.38 * (float)dim.y;
  m_s0angle = -atan((dim.x * 0.02) / m_string_center.y);
//#define TEST_NOTES_LENGTH
#ifdef TEST_NOTES_LENGTH
  t_coord dim;
  pw->get_dim(&dim);
  t_notefreq notef;
  for (int i = 0; i < 2 * 12; i++)
    {
      notef.string = 1;
      notef.f = m_f2n.note2frequ(12 * 4 + 9 + i);
      note_2_y(dim, &notef);
    }
  return ;
#endif
  m_instrument = pscore->get_instrument(instrument_name, instru_identifier);
  if (m_instrument == NULL)
    return ;
  Draw_the_measure_bars(pscore, pos, dim, pl->current);
  Draw_the_practice_end(pos, dim, pl);
  draw_finger_board(pw, lvl_background);
  check_visible_notes(pw, pscore, pl, hnote_id);
  draw_note_drawings(lvl_background, hnote_id);
  draw_finger_board(pw, lvl_fingerboard);
  draw_strings(pw);
  draw_note_drawings(lvl_fingerboard, hnote_id);
  draw_note_drawings(lvl_top, hnote_id);
  print_current_timecodes(pw, WHITE, pl->current, m_viewtime);
}

