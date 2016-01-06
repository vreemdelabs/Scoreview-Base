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
#include <SDL2/SDL2_gfxPrimitives.h>

#include <GL/gl.h>

#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "score.h"
#include "f2n.h"
#include "pictures.h"
#include "keypress.h"
#include "stringcolor.h"
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreedit.h"
#include "scorerenderer.h"

using namespace std;

float CScorePlacement::ypixel_2_float(int pixel_thickness)
{
  return (m_vzoom * (float)pixel_thickness * m_fdim.y / (float)m_wdim.y);
}

float CScorePlacement::xpixel_2_float(int pixel_thickness)
{
  return (m_vzoom * (float)pixel_thickness * m_fdim.x / (float)m_wdim.x);
}

bool CScorePlacement::compare(const t_note_sketch *pfirst, const t_note_sketch *psecond)
{
  return (pfirst->pnote->m_time < psecond->pnote->m_time);
}

void CScorePlacement::place_mesh(t_fcoord pos, t_fcoord dim, int color, char *meshname)
{
  t_mesh    mesh;

  mesh.name = string(meshname);
  mesh.pos = pos;
  mesh.dim = dim;
  mesh.color = color;
  m_rendered_meshes.push_front(mesh);
}

void CScorePlacement::place_flag(t_note_sketch *pnotesk, int color)
{
  t_lsegment  flag_segment;
  float       xo, yo;
  t_fcoord    pos, dim;

  if (pnotesk->nf.note_type > noire)
    {
      xo = pnotesk->pos.x + pnotesk->dim.x;
      yo = pnotesk->pos.y;
      pos.x = xo;
      pos.y = yo;
      dim.x = pnotesk->tailboxdim.x;
      dim.y = pnotesk->tailboxdim.y;
      if (pnotesk->nf.note_type == croche)
	place_mesh(pos, dim, color, (char*)"flag");
      else
	{
	  // Double croche
	  place_mesh(pos, dim, color, (char*)"flag");
	  pos.y += pnotesk->tailboxdim.y / 3;
	  place_mesh(pos, dim, color, (char*)"flag");
	}
    }
}

void CScorePlacement::change_tail(t_note_sketch *pns, float y)
{
  float diff;

  diff = pns->pos.y - y;
  pns->pos.y -= diff;
  pns->dim.y += diff;
  pns->tailboxdim.y += diff;
}

void CScorePlacement::add_rectangle_polygon(float x, float y, float dimx, float dimy, int color)
{
  t_polygon pol;

  pol.point[0].x = x;
  pol.point[0].y = y;
  pol.point[1].x = x + dimx;
  pol.point[1].y = y;
  pol.point[2].x = x + dimx;
  pol.point[2].y = y + dimy;
  pol.point[3].x = x;
  pol.point[3].y = y + dimy;
  pol.nbpoints = 4;
  pol.color = color;
  m_polygon_list.push_front(pol);  
}

void CScorePlacement::create_beam_polygons(t_beam *pbeam, float hi)
{
  enum {case_simple = 0, case_double_right = 1, case_double_left = 2, case_double = 3};
  std::list<t_note_sketch*>::iterator noteiter;
  std::list<t_note_sketch*>::iterator next_noteiter;
  float     thickness = 2.;
  float     x0, x1, y0, y1;
  int       beam_case;
  int       color;

  color = BLACK;
  noteiter = pbeam->beam_notes_list.begin();
  while (noteiter != pbeam->beam_notes_list.end())
    {
      next_noteiter = noteiter;
      next_noteiter++;
      if (next_noteiter == pbeam->beam_notes_list.end())
	break;
      x0 = (*noteiter)->pos.x + (*noteiter)->dim.x;
      x1 = (*next_noteiter)->pos.x + (*next_noteiter)->dim.x;
      y0 = y1 = hi;
      // Main beam
      add_rectangle_polygon(x0, y0, x1 - x0, xpixel_2_float(thickness), color);
      // Second beam
      beam_case  = (*noteiter)->nf.note_type == doublecroche? 1 : 0;
      beam_case |= (*next_noteiter)->nf.note_type == doublecroche? 2 : 0;
      switch (beam_case)
	{
	case case_double:
	  //printf("double\n");
	  break;
	case case_double_right:
	  {
	    //printf("double right\n");
	    x1 = x0 + (x1 - x0) / 3.;
	  }
	  break;
	case case_double_left:
	  {
	    //printf("double left\n");
	    x0 = x1 - ((x1 - x0) / 3.);
	  }
	  break;
	case case_simple:
	default:
	  break;
	}
      switch (beam_case)
	{
	case case_double:
	case case_double_right:
	case case_double_left:
	  {
	    add_rectangle_polygon(x0, y0 + xpixel_2_float(2. * thickness), x1 - x0, xpixel_2_float(thickness), color);
	  }
	  break;
	case case_simple:
	default:
	  break;
	}
      noteiter++;
    }
}

#define SLOPE_LIMIT 4.

void CScorePlacement::create_beam(t_beam *pbeam)
{
  std::list<t_note_sketch*>::iterator noteiter;
  float   a; // From y = ax + b
  //float   b;
  float   x, y;
  float   hi, hix;
  float   lo, lox;
  float   startx, stopx;
  t_note_sketch *pns;
  t_lsegment     beam_segment;

  // Sort the notes
  pbeam->beam_notes_list.sort(CScorePlacement::compare);
  // Calculate proper beam coordinates
  hi = lo = -1;
  hix = lox = -1;
  startx = stopx = -1;
  noteiter = pbeam->beam_notes_list.begin();
  while (noteiter != pbeam->beam_notes_list.end())
    {
      pns = *noteiter;
      x = pns->pos.x + pns->dim.x;
      if (hi == -1 || pns->pos.y < hi)
	{
	  hi  = pns->pos.y;
	  hix = x;
	}
      if (lo == -1 || pns->pos.y < lo)
	{
	  lo = pns->pos.y;
	  lox = x;
	}
      startx = startx == -1? x : (x < startx? x : startx);
      stopx  = stopx  == -1? x : (x > stopx? x : stopx);
      noteiter++;
    }
  // Calculate the slope
  a = (lo - hi) / fabs(lox - hix);
  a = a >  SLOPE_LIMIT? SLOPE_LIMIT : a;
  a = a > -SLOPE_LIMIT? -SLOPE_LIMIT : a;
  // Starting height on first note
  //b = hix - a * fabs(hix - startx);
  //b = hi;
  // Change the original tails dimensions
  noteiter = pbeam->beam_notes_list.begin();
  while (noteiter != pbeam->beam_notes_list.end())
    {
      pns = *noteiter;
      y = hi;
      //y = b + a * (pns->pos.x + pns->dim.x - startx);
      change_tail(pns, y);
      noteiter++;
    }
  // Add the beam polygons
  create_beam_polygons(pbeam, hi);
}

bool CScorePlacement::same_measure_close_time(t_measure_note_info *pref_note, t_measure_note_info *pnext_note)
{
  bool     ret;

  // Same measure
  ret = (pref_note->pmesure == pnext_note->pmesure &&
	 pref_note->measure_index == pnext_note->measure_index);
  // Exact expected time between the 2 notes
  ret = ret && (pref_note->mnstart + pref_note->mnduration == pnext_note->mnstart);
  //printf("first start=%d duration=%d sum=%d next==%d\n", pref_note->mnstart, pref_note->mnduration, pref_note->mnstart + pref_note->mnduration, pnext_note->mnstart);
  ret = ret && pref_note->note_type >= croche && pnext_note->note_type >= croche;
  return ret;
}

void CScorePlacement::place_beams(std::list<t_note_sketch> *pnsk_list, int color)
{
  enum{statenobeam, stateonbeam};
  std::list<t_note_sketch>::iterator noteiter;
  std::list<t_note_sketch>::iterator next_noteiter;
  t_beam                             beam;
  bool                               bnext;
  int                                state;

  state = statenobeam;
  noteiter = pnsk_list->begin();
  while (noteiter != pnsk_list->end())
    {
      switch (state)
	{
	case statenobeam:
	  {
	    if ((*noteiter).pnote->m_lconnected || m_bautobeam)
	      {
		next_noteiter = noteiter;
		next_noteiter++;
		if (next_noteiter != pnsk_list->end() &&
		    same_measure_close_time(&(*noteiter).nf, &(*next_noteiter).nf))
		  {
		    // new beam
		    beam.beam_notes_list.clear();
		    beam.beam_notes_list.push_back(&(*noteiter));
		    state = stateonbeam;
		  }
	      }
	    if (state != stateonbeam)
	      place_flag(&(*noteiter), color);
	  }
	  break;
	case stateonbeam:
	  {
	    beam.beam_notes_list.push_back(&(*noteiter));
	    next_noteiter = noteiter;
	    next_noteiter++;
	    bnext = false;
	    if (next_noteiter != pnsk_list->end())
	      {
		bnext = same_measure_close_time(&(*noteiter).nf, &(*next_noteiter).nf);
	      }
	    if (!bnext)
	      {
		create_beam(&beam);
		state = statenobeam;
	      }
	  }
	  break;
	default:
	  break;
	};
      noteiter++;
    }
}

void CScorePlacement::set_autobeam(bool bactive)
{
  m_bautobeam = bactive; 
}

