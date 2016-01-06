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

#include <math.h>
#include <string>
#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

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

CScoreRenderer::CScoreRenderer(CGL2Dprimitives *pgfxprimitives, CpictureList *picturelist, TTF_Font *pfont, CMeshList *pmeshList):
  m_gfxprimitives(pgfxprimitives),
  m_picturelist(picturelist),
  m_color(0xFF000000),
  m_font(pfont),
  m_meshList(pmeshList),
  m_prevblrcursor(false)
{
}

void CScoreRenderer::drawMeshes(CScorePlacement *pplacement)
{
  t_fcoord      pos, dim;
  bool          bfiltering;
  int           color;
  t_mesh       *pmd;
  CMesh        *pmesh;

  bfiltering = true;
  pmd =  pplacement->get_first_mesh();
  while (pmd != NULL)
    {
      pos = pmd->pos;
      dim = pmd->dim;
      color = pmd->color;
      pmesh = m_meshList->get_mesh(pmd->name);
      if (pmesh == NULL)
	{
	  printf("The mesh called \"%s\" is missing.\n", pmd->name.c_str());
	}
      else
	{
	  //printf("drawing %s %f %f %f %f col=%X\n", pmd->name.c_str(),   pos.x,  pos.y, dim.x, dim.y, color);
	  m_gfxprimitives->triangle_mesh2D(pos, dim, pmesh, color, bfiltering);
	}
      pmd = pplacement->get_next_mesh();
    }
}

void CScoreRenderer::drawmovecursor(t_coord mousepos)
{
  t_fcoord     pos, dim;
  bool         bfiltering;
  bool         blend = true;
  t_coord      tdim;
  string       tname("cursorlr");
  
  if (m_gfxprimitives->get_texture_size(tname, &tdim))
    {
      dim.x = tdim.x;
      dim.y = tdim.y;
      pos.x = mousepos.x - (dim.x / 3);
      pos.y = mousepos.y - (dim.y / 2);
      bfiltering = true;
      m_gfxprimitives->draw_texture(tname, pos, dim, bfiltering, blend);
    }
}

void CScoreRenderer::drawtrashcancursor(t_coord mousepos)
{
  t_fcoord     pos, dim;
  bool         bfiltering;
  bool         blend = true;
  t_coord      tdim;
  string       tname("trashcan");

  if (m_gfxprimitives->get_texture_size(tname, &tdim))
    {
      dim.x = tdim.x / 2;
      dim.y = tdim.y / 2;
      pos.x = mousepos.x + 10;
      pos.y = mousepos.y + 14;
      bfiltering = true;
      m_gfxprimitives->draw_texture(tname, pos, dim, bfiltering, blend);
    }
}

void CScoreRenderer::drawwheelcursor(t_coord mousepos)
{
  string       tname("mousewheel");
  t_coord      tdim;
  t_fcoord     pos, dim;
  bool         bfiltering;
  bool         blend = true;

  m_gfxprimitives->get_texture_size(tname, &tdim);
  dim.x = tdim.x;
  dim.y = tdim.y;
  pos.x = mousepos.x + 5;
  pos.y = mousepos.y + 14;
  bfiltering = true;
  m_gfxprimitives->draw_texture(tname, pos, dim, bfiltering, blend);
}

void CScoreRenderer::drawbeamlink(t_coord mousepos, bool bset)
{
  string       tname("beamlink");
  t_coord      tdim;
  t_fcoord     pos, dim;
  bool         bfiltering;
  bool         blend = true;

  if (bset)
    tname = string("brokenbeamlink");
  m_gfxprimitives->get_texture_size(tname, &tdim);
  dim.x = tdim.x;
  dim.y = tdim.y;
  pos.x = mousepos.x;
  pos.y = mousepos.y - 2 * dim.y;
  bfiltering = true;
  m_gfxprimitives->draw_texture(tname, pos, dim, bfiltering, blend);
}

void CScoreRenderer::draw_hand_text(CScorePlacement *pplacement, t_note_draw_info *pnoteinfo, t_fcoord pos)
{
  string   txt;
  char     str[4096];
  t_fcoord tdim, ndim;
  float    scaling = 4;

  txt = pplacement->get_hand()->get_string_text(pnoteinfo->string_num);
  strcpy(str, txt.c_str());
  if (m_gfxprimitives->get_textSize(str, m_font, &tdim))
    {
      ndim = pplacement->app_area_size2placement_size(tdim);
      ndim.x /= scaling;
      ndim.y /= scaling;
      m_gfxprimitives->print(str, m_font, pos, ndim, m_color);
    }
}

int CScoreRenderer::noteColor(CScorePlacement *pplacement, enote_active selectedfor, int string_num)
{
  int color;

  if (string_num < 0)
    return NOTE_OUT_OF_RANGE_COLOR;
  switch (selectedfor)
    {
    case enote_edited:
      color = EDITED_NOTE_COLOR; //pplacement->get_hand()->get_color(string_num);
      break;
    case enote_played:
      color = PLAYED_NOTE_COLOR;
      break;
    case enote_not_selected:
    default:
      color = NOTE_COLOR;
      break;
    }
  return color;
}

void CScoreRenderer::drawNote(t_note_sketch *pns, CScorePlacement *pplacement, enote_active selectedfor)
{
  std::list<t_note_draw_info>::iterator iter;
  int      color, boxcolor;
  float    radius;
  float    lowery;
  int      type;
  t_fcoord pos, txtpos;
  t_fcoord dim;
  t_fcoord center;
  bool     bantialiased;

  bantialiased = true;
  // Note circles
  radius = 10;
  lowery = -1;
  iter = pns->noteinfolist.begin();
  color = noteColor(pplacement, selectedfor, (*iter).string_num);
  while (iter != pns->noteinfolist.end())
    {
      pos = (*iter).circle.center;
      //printf("drawinf at %f=%d\n", (*iter).circle.center.y, y);
      lowery = pos.y > lowery? pos.y : lowery;
      radius = (*iter).circle.radius;
      // replace the disc surface color by the string color
 //     if (selectedfor == enote_edited)
	//color = pplacement->get_hand()->get_color((*iter).string_num);
      //else
	//color = noteColor(pplacement, selectedfor, (*iter).string_num);
      if (selectedfor == enote_edited)
	{
	  txtpos = pos;
	  txtpos.y += radius * 4;
	  draw_hand_text(pplacement, &(*iter), txtpos);
	  // length
	  // FIXME put that in a function
	  t_fcoord start, bdim;

	  start.x = (*iter).xstart;
	  start.y = (*iter).circle.center.y - (*iter).circle.radius;
	  bdim.x = (*iter).len;
	  bdim.y = 2. * (*iter).circle.radius;
	  //printf("startx=%f y=%f w=%f h=%f\n", start.x, start.y, bdim.x, bdim.y);
	  boxcolor = 0x5FF2E230;
	  m_gfxprimitives->box(start, bdim, boxcolor, bantialiased);
	}
      center = (*iter).circle.center;
      type = pns->nf.note_type;
      if (type > blanche)
	{
	  //printf("center =%f %f \n", center.x, center.y);
	  m_gfxprimitives->disc(center, radius, color, bantialiased);
	}
      else
	{
	  float thickness = radius / 4.;

	  m_gfxprimitives->circle(center, radius, thickness, color, bantialiased);
	}
      if (pns->nf.bpointed)
	{
	  float dotradius = radius / 3;

	  center.x = pos.x + 1.7 * radius;
	  center.y = pos.y + radius / 2;
	  m_gfxprimitives->disc(center, dotradius, color, bantialiased);
	}
      iter++;
    }
  // Tail
  if (pns->nf.note_type != ronde)
    {
      dim.x = radius / 4.;
      dim.y = lowery - pns->pos.y;
      pos.x = pns->pos.x + pns->dim.x - dim.x;
      pos.y = pns->pos.y;
      m_gfxprimitives->box(pos, dim, color, bantialiased);
    }
}

void CScoreRenderer::drawSegments(CScorePlacement *pplacement)
{
  t_lsegment *ps;
  t_fcoord    pos, dim;
  //float thickness = 1.;

  m_color = 0xFF000000;
  ps = pplacement->get_first_lsegment();
  while (ps != NULL)
    {
      pos = ps->pos;
      dim = ps->dim;
      m_gfxprimitives->line(pos.x, pos.y, pos.x + dim.x, pos.y + dim.y, m_color);
      ps = pplacement->get_next_lsegment();
    }
}

void CScoreRenderer::drawPolygons(CScorePlacement *pplacement)
{
  t_polygon *ppolygon;
  t_fcoord   pos;
  bool       bantialiased = true;

  ppolygon = pplacement->get_first_polygon();
  while (ppolygon != NULL)
    {
      m_gfxprimitives->polygon(ppolygon->point, ppolygon->nbpoints, ppolygon->color, bantialiased);
      ppolygon = pplacement->get_next_polygon();
    }
}

void CScoreRenderer::drawMeasure(t_bar_sketch *pbs, CScorePlacement *pplacement, t_edit_state *peditstate)
{
  bool     bedited = pbs->pmesure == peditstate->pmodified_measure;
  //float thickness;
  t_fcoord pos, dim;
  int      color;
  bool     bantialiased;

  pos = pbs->boxpos;
  dim = pbs->boxdim;
  m_color = bedited? 0xFF0000FF : 0xFF000000;
  //thickness = 1.;
  m_gfxprimitives->line(pos.x + dim.x / 2., pos.y, pos.x + dim.x / 2., pos.y + dim.y, m_color);
  if (pbs->doublebar)
    m_gfxprimitives->line(pos.x + dim.x, pos.y, pos.x + dim.x, pos.y + dim.y, m_color);
  // Draw the selection box
  if (bedited)
    {
      bantialiased = true;
      color = 0x3FF0E230;
      m_gfxprimitives->box(pos, dim, color, bantialiased);
    }
}

bool CScoreRenderer::note_is_in_list(CNote* pn, std::list<CNote*> *pplayed_note_list)
{
  std::list<CNote*>::iterator pniter;

  pniter = pplayed_note_list->begin();
  while (pniter != pplayed_note_list->end())
    {
      if (*pniter == pn)
	return true;
      pniter++;
    }
  return false;
}

// Everything outside the window will not be rendered
void CScoreRenderer::set_viewport(Cgfxarea *pw, t_fcoord orthosize)
{
  t_coord  pos, dim;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  m_gfxprimitives->set_viewport(pos, dim, orthosize);
}

void CScoreRenderer::reset_viewport()
{
  m_gfxprimitives->reset_viewport();
}

void CScoreRenderer::draw_note_selbox(t_coord bstart, t_coord bstop, bool bloop, int progress)
{
  const int   cnpts = 3;
  const float len = 4;
  t_fcoord    p;
  t_fcoord    points[cnpts];
  int         color;
  bool        bantialiased = true;
  t_coord     start;
  t_coord     stop;
  float       height;

  color = 0xFF0FE20F;
  start = bstart;
  stop  = bstop;
  if (start.x > stop.x)
    {
      stop.x = bstart.x;
      start.x = bstop.x;
    }
  if (start.y > stop.y)
    {
      stop.y = bstart.y;
      start.y = bstop.y;
    }
  if (progress > 0)
    {
      height = stop.y - start.y;
      m_gfxprimitives->vline(progress, start.y, height, BLACK);
    }
  m_gfxprimitives->rectangle(start.x, start.y, stop.x, stop.y, color);
  if (bloop)
    {
      // Top left triangle
      p.x = start.x;
      p.y = start.y;
      points[0] = points[1] = points[2] = p;
      points[0].y -= len;
      points[1].x += 2. * len;
      points[2].y += len;
      m_gfxprimitives->polygon(points, cnpts, color, bantialiased);
      // Bottom right triangle
      p.x = stop.x + 1;
      p.y = stop.y;
      points[0] = points[1] = points[2] = p;
      points[0].x -= 2. * len;
      points[1].y -= len;
      points[2].y += len;
      m_gfxprimitives->polygon(points, cnpts, color, bantialiased);      
    }
}

void CScoreRenderer::render_sel_box(Cgfxarea *pw, CScorePlacement *pplacement, t_note_selection_box *pnsb, double elapsed_time, bool bpracticeloop)
{
  double  tstart, tstop;
  t_coord pos, dim;
  t_coord bstart;
  t_coord bstop;
  int     progress;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  tstart = pnsb->start_tcode < pnsb->stop_tcode? pnsb->start_tcode : pnsb->stop_tcode;
  tstop  = pnsb->start_tcode > pnsb->stop_tcode? pnsb->start_tcode : pnsb->stop_tcode;
  bstart.x = pplacement->get_appx_from_time(tstart);
  bstop.x  = pplacement->get_appx_from_time(tstop);
  bstart.y = pplacement->frequency2appy(pnsb->hif);
  bstop.y  = pplacement->frequency2appy(pnsb->lof);
  bstart.y = bstart.y > pos.y? bstart.y : pos.y;
  bstop.y = bstop.y < pos.y + dim.y? bstop.y : pos.y + dim.y;
  //printf("hif = %f lof = %f\n", pnsb->hif, pnsb->lof);
  //printf("hiy %d loy %d\n", bstart.y, bstop.y);
  //printf("elapsed time = %f sta %d stop %d\n", elapsed_time, bstart.x, bstop.x);
  progress = pplacement->get_appx_from_time(pnsb->start_tcode + elapsed_time);
  if (bstop.x <= pos.x)
    return;
  if (bstart.x >= pos.x + dim.x)
    return;
  bstart.x = (bstart.x < pos.x)? pos.x : bstart.x;
  bstop.x = (bstop.x > pos.x + dim.x)? pos.x + dim.x : bstop.x;
  draw_note_selbox(bstart, bstop, bpracticeloop, progress);
}

void CScoreRenderer::draw_texts(CScorePlacement *pplacement)
{
  t_string_placement *ptxtpl;

  ptxtpl = pplacement->get_first_txt();
  while (ptxtpl != NULL)
    {
      m_gfxprimitives->print((char*)ptxtpl->txt.c_str(), m_font, ptxtpl->pos, ptxtpl->dim, ptxtpl->color, true);
      ptxtpl = pplacement->get_next_txt();
    }
}

void CScoreRenderer::render(Cgfxarea *pw, CScorePlacement *pplacement, t_edit_state *peditstate, std::list<CNote*> *pplayed_note_list)
{
  t_measure_number *pmn;
  t_note_sketch *pns;
  t_bar_sketch  *pbs;
  char str[20];
  int  color;
  bool bedited;
  t_fcoord pos, dim, tailpos, orthosize;
  enote_active selectedfor;
  bool bantialiased;

  bantialiased = true;
  orthosize = pplacement->get_ortho_size();
  set_viewport(pw, orthosize);
  drawSegments(pplacement);
  drawPolygons(pplacement);
  drawMeshes(pplacement);
  pmn = pplacement->get_first_barnumber();
  while (pmn != NULL)
    {
      bedited = peditstate->bedit_meter && (pmn->pmesure == peditstate->pmodified_measure);
      sprintf(str, "%1d", pmn->pmesure->m_times);
      m_color = bedited? 0xFF0000FF : 0xFF000000;
      pos = pmn->pos;
      dim = pmn->dim;
      m_gfxprimitives->print(str, m_font, pos, dim, m_color, true);
      if (bedited)
	{
	  m_gfxprimitives->box(pos, dim, 0x3FF0E230, bantialiased);
	}
      pmn = pplacement->get_next_barnumber();
    }
  pbs = pplacement->get_first_barsk();
  while (pbs != NULL)
    {
      drawMeasure(pbs, pplacement, peditstate);
      pbs = pplacement->get_next_barsk();
    }
  pns = pplacement->get_first_notesk();
  while (pns != NULL)
    {
      selectedfor = enote_not_selected;
      if (pns->pnote == peditstate->pmodified_note)
	{
	  selectedfor = enote_edited;
	  bedited = true;
	}
      else
	bedited = false;
      if (pns->pnote == pplacement->get_tmp_note())
	selectedfor = enote_tmp;
      if (note_is_in_list(pns->pnote, pplayed_note_list))
	selectedfor = enote_played;
      drawNote(pns, pplacement, selectedfor);
      if (bedited)
	{
	  color = pns->pnote->m_lconnected? 0x8F3202F4 : 0x5FF2E230;
	  m_gfxprimitives->box(pns->pos, pns->beamboxdim, color, bantialiased);
	  tailpos = pos;
	  tailpos.y += dim.y;
	  m_gfxprimitives->box(pos, pns->tailboxdim, 0x3FF0E230, bantialiased);
	}
      pns = pplacement->get_next_notesk();
    }
  draw_texts(pplacement);
  reset_viewport();
  if (m_prevblrcursor != peditstate->blrcursor)
    {
      m_prevblrcursor = peditstate->blrcursor;
      SDL_ShowCursor(m_prevblrcursor? 0 : 1);
    }
  if (peditstate->blrcursor)
    {
      drawmovecursor(peditstate->mouse_stop);
    }
  else
    {
      if (peditstate->bbeamlinkcursor)
	drawbeamlink(peditstate->mouse_stop, false);
      else
	if (peditstate->bbrokenbeamlinkcursor)
	  drawbeamlink(peditstate->mouse_stop, true);
    }
  if (peditstate->btrashcan_cursor)
    {
      drawtrashcancursor(peditstate->mouse_stop);
    }
  if (peditstate->bwheel_cursor)
    {
      drawwheelcursor(peditstate->mouse_stop);
    }
  if (peditstate->drawselbox != noselbox)
    {
      draw_note_selbox(peditstate->mouse_start, peditstate->mouse_stop, peditstate->drawselbox == loopselbox);
    }
}

