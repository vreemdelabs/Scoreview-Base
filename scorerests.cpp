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
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreedit.h"
#include "scorerenderer.h"

using namespace std;

void CScorePlacement::place_rest_mesh(double start, float y, double duration, int color, char *meshname)
{
  float     x0;
  t_mesh    mesh;

  mesh.name = string(meshname);
  x0 = ((start + (duration / 2.) - m_starttime) * m_fdim.x / m_viewtime);
  mesh.pos.x = x0;
  mesh.pos.y = y;
  mesh.dim.y = note2y(4, 4) - note2y(3, 5);
  mesh.dim.x = mesh.dim.y / 1.618;
  mesh.pos.x -= mesh.dim.x / 2.;
  mesh.color = color;
  m_rendered_meshes.push_front(mesh);
}

void CScorePlacement::place_whole_or_half_rest(double start, double duration, int color, int timedivs)
{
  float     x0, y, h;

  h = m_zradius;
  if (timedivs <= 48)
    {
      y = note2y(0, 5); // Rectangle on the middle bottom line      
    }
  else
    {
      y = note2y(2, 5); // Rectangle on the middle top line
    }
  x0 = ((start + duration / 2. - m_starttime) * m_fdim.x / m_viewtime);
  add_rectangle_polygon(x0 - h, y, 2 * h, h, color);
}

t_measure_occupancy* CScorePlacement::find_measure_occupancy_element(std::list<t_measure_occupancy> *pmol, CMesure *pm, int index)
{
  std::list<t_measure_occupancy>::iterator iter;

  iter = pmol->begin();
  while (iter != pmol->end())
    {
      if ((*iter).pm == pm && (*iter).index == index)
	return &(*iter);
      iter++;
    }
  return NULL;
}

void CScorePlacement::rest_placement(t_measure_occupancy *po)
{
  enum {stateemptyspace, statefilledspace};
  CMesure      *pm;
  int           state, times;
  double        start, timediv, t, d;
  bool         *pmask;
  int           empty_counter;
  int           i;
  float         y;
  int           black = 0xFF000000;

  pm = po->pm;
  times = pm->m_times * 16;
  start = pm->start_time(po->index);
  timediv = pm->duration() / (double)times;
  pmask = po->spaces;
  state = pmask[0]? statefilledspace : stateemptyspace;
  for (i = 0, empty_counter = 0; i < times; i++)
    {
//#define SHOW_MASK_TIMEDIVS
#ifdef SHOW_MASK_TIMEDIVS
      {
	float                x, y, dx, dy;
	
	y = note2y(4, 4);
	dy = m_fdim.y / 30.;
	x  = (((start + (double)i * timediv) - m_starttime) * m_fdim.x / m_viewtime);
	dx = timediv * m_fdim.x / m_viewtime;
	add_rectangle_polygon(x, y, dx, dy, pmask[i]? BLACK : 0XDF43DFEF);
      }
#endif
      switch (state)
	{
	case stateemptyspace:
	  {
	    if (!pmask[i])
	      empty_counter++;
	    if (pmask[i] || i == times - 1)
	      {
		t = start + (double)(i - empty_counter) * timediv;
		d = (double)empty_counter * timediv;
		if (empty_counter > 24)
		  {
		    if (empty_counter > 48)
		      {
			if (empty_counter > 64)
			  {
			    // No rest bigger than whole rest
			    empty_counter -= 64;
			  }
		      }
		    place_whole_or_half_rest(t, d, black, empty_counter);
		  }
		else
		  {
		    if (empty_counter > 12)
		      {
			y = note2y(5, 5);
			place_rest_mesh(t, y, d, black, (char*)"qnrest");
		      }
		    else
		      {
			if (empty_counter > 6)
			  {
			    y = note2y(2, 5);
			    place_rest_mesh(t, y, d, black, (char*)"enrest");
			  }
			else
			  {
			    y = note2y(2, 5);
			    place_rest_mesh(t, y, d, black, (char*)"snrest");
			  }
		      }
		  }
		state = statefilledspace;
	      }
	  }
	  break;
	case statefilledspace:
	  {
	    if (!pmask[i])
	      {
		empty_counter = 1;
		state = stateemptyspace;
	      }
	  }
	  break;
	default:
	  break;
	};
    }
}

void CScorePlacement::place_rests(std::list<CMesure*> *pml, std::list<t_note_sketch> *prendered_notes_list)
{
  std::list<CMesure*>::reverse_iterator     it;
  std::list<t_measure_occupancy>::iterator  moiter;
  std::list<t_measure_occupancy>            mol;
  t_measure_occupancy                       mo;
  double                                    next_measure_start;
  double                                    timecode;
  int                                       i;
  std::list<t_note_sketch>::iterator        fiter;
  t_measure_occupancy                      *pmo;
  t_note_sketch                            *pns;
  t_measure_number                          mn;

  if (!m_bautobeam)
    return;
//#define SHOW_NOTE_TIMEDIVS
#ifdef SHOW_NOTE_TIMEDIVS
  t_measure_note_info *pninf;
  double               timediv, start;
  float                x, y, dx, dy;

  y = note2y(4, 4);
  dy = m_fdim.y / 30.;
  fiter = prendered_notes_list->begin();
  while (fiter != prendered_notes_list->end())
    {
      pninf = &((*fiter).nf);
      if (pninf->pmesure != NULL)
	{
	  timediv = pninf->pmesure->duration() / ((double)pninf->pmesure->m_times * 16.);
	  start = pninf->pmesure->m_start + pninf->measure_index * pninf->pmesure->duration() + (double)pninf->mnstart * timediv;
	  x  = ((start - m_starttime) * m_fdim.x / m_viewtime);
	  dx = (double)pninf->mnduration * timediv * m_fdim.x / m_viewtime;
	  add_rectangle_polygon(x, y, dx, dy, BLACK);
	  printf("mnstart=%d mnduration=%d note_index=%d measure_index=%d absolute_index=%d\n", pninf->mnstart, pninf->mnduration, pninf->note_index, pninf->measure_index, pninf->absolute_index);
	}
      fiter++;
    }
#endif
  // Create the viewed measures objects
  for (i = 0; i < MAXMTIMESPACES; i++)
    mo.spaces[i] = false;
  it = pml->rbegin();
  next_measure_start = m_starttime + m_viewtime;
  while (it != pml->rend())
    {
      if ((*it)->m_start < next_measure_start)
	{
	  timecode = (*it)->m_start;
	  //printf("start %f stop %f\n", (*it)->m_start, (*it)->m_stop);
	  while (timecode < m_starttime)
	    timecode += (*it)->duration();
	  while (timecode < next_measure_start)
	    {
	      mo.pm = *it;
	      mo.index = (timecode - (*it)->m_start) / (*it)->duration();
	      mol.push_front(mo);
	      timecode += (*it)->duration();
	    }
	}
      next_measure_start = (*it)->m_start < next_measure_start? (*it)->m_start : next_measure_start;
      it++;
    }
  // Fill the occupancy data with the note locations
  fiter = prendered_notes_list->begin();
  while (fiter != prendered_notes_list->end())
    {
      pns = &(*fiter);
      pmo = find_measure_occupancy_element(&mol, pns->nf.pmesure, pns->nf.measure_index);
      if (pmo != NULL)
	{
	  for (i = pns->nf.mnstart; i <  16 * pmo->pm->m_times && i < pns->nf.mnstart + pns->nf.mnduration; i++)
	    {
	      pmo->spaces[i] = true;
	    }
	}
      fiter++;
    }
  // Place rests in the voids
  moiter = mol.begin();
  while (moiter != mol.end())
    {
      rest_placement(&(*moiter));
      moiter++;
    }
}

