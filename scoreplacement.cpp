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

CScorePlacement::CScorePlacement(int notenum):
  m_color(0xFF000000),
  m_notenum(notenum),
  m_rest_staff_octave(4),
  m_bautobeam(false),
  m_bpracticing(false),
  m_phand(NULL)
{
  int i;

  // 12 notes to 6 notes + flat or sharp
  // C=0 D=1 E=2 F=3 G=4 A=5 B=6
  // Do
  i = 0;
  m_n2k[i].notelevel = 0;
  m_n2k[i++].type    = enormal;
  m_n2k[i].notelevel = 0;
  m_n2k[i++].type    = ediese;
  // Re
  m_n2k[i].notelevel = 1;
  m_n2k[i++].type    = enormal;
  // Mi
  m_n2k[i].notelevel = 2;
  m_n2k[i++].type    = ebemol;
  m_n2k[i].notelevel = 2;
  m_n2k[i++].type    = enormal;
  // Fa
  m_n2k[i].notelevel = 3;
  m_n2k[i++].type    = enormal;
  m_n2k[i].notelevel = 3;
  m_n2k[i++].type    = ediese;
  // Sol
  m_n2k[i].notelevel = 4;
  m_n2k[i++].type    = enormal;
  m_n2k[i].notelevel = 5;
  m_n2k[i++].type    = ebemol;
  // La
  m_n2k[i].notelevel = 5;
  m_n2k[i++].type    = enormal;
  // Si
  m_n2k[i].notelevel = 6;
  m_n2k[i++].type    = ebemol;
  m_n2k[i].notelevel = 6;
  m_n2k[i].type    = enormal;
  //
  // 12 notes to 14 spaces for the 7 note levels representation with
  // E to F space filled with E and B to C filled with B
  i = 0;
  m_y2n[i++] = 0; // C
  m_y2n[i++] = 1; // C#
  m_y2n[i++] = 2; // D
  m_y2n[i++] = 3; // Eb
  m_y2n[i++] = 4; // E
  m_y2n[i++] = 4; // E
  m_y2n[i++] = 5; // F
  m_y2n[i++] = 6; // F#
  m_y2n[i++] = 7; // G
  m_y2n[i++] = 8; // 
  m_y2n[i++] = 9;
  m_y2n[i++] = 10;
  m_y2n[i++] = 11; // B
  m_y2n[i++] = 11; // B
  //
  m_fdim.x = 1.;
  m_fdim.y = 1.;
  //m_fdim.x = (float)m_wdim.x;
  //m_fdim.y = (float)m_wdim.y;
  m_vzoom  = 1.; // note radius = 4 pixels
  //printf("notenum=%d\n", m_notenum);
  m_radius = m_fdim.y / (float)(m_notenum + 1);
  m_zradius = m_radius * m_vzoom;
  m_vmove = 0;
  //
  clear();
  //
  m_tmp_note = CNote(-1, -1, -1);
}

CScorePlacement::~CScorePlacement()
{
  clear();
}

void CScorePlacement::clear()
{
  m_rendered_notes_list.clear();
  m_rendered_barnumber_list.clear();
  m_rendered_bars_list.clear();
  m_lsegment_list.clear();
  m_rendered_meshes.clear();
  m_polygon_list.clear();
  m_txt_list.clear();
}

void CScorePlacement::add_bar_sketch(t_bar_sketch bar)
{
  m_rendered_bars_list.push_back(bar);
}

t_note_sketch* CScorePlacement::get_first_notesk()
{
  m_noteiter = m_rendered_notes_list.begin();
  if (m_noteiter == m_rendered_notes_list.end())
    return NULL;
  return &(*m_noteiter);
}

t_note_sketch* CScorePlacement::get_next_notesk()
{
  m_noteiter++;
  if (m_noteiter == m_rendered_notes_list.end())
    return NULL;
  return &(*m_noteiter);
}

t_measure_number* CScorePlacement::get_first_barnumber()
{
  m_barnumberiter = m_rendered_barnumber_list.begin();
  if (m_barnumberiter == m_rendered_barnumber_list.end())
    return NULL;
  return &(*m_barnumberiter);
}

t_measure_number* CScorePlacement::get_next_barnumber()
{
  m_barnumberiter++;
  if (m_barnumberiter == m_rendered_barnumber_list.end())
    return NULL;
  return &(*m_barnumberiter);
}

t_bar_sketch* CScorePlacement::get_first_barsk()
{
  m_bariter = m_rendered_bars_list.begin();
  if (m_bariter == m_rendered_bars_list.end())
    return NULL;
  return &(*m_bariter);
}

t_bar_sketch* CScorePlacement::get_next_barsk()
{
  m_bariter++;
  if (m_bariter == m_rendered_bars_list.end())
    return NULL;
  return &(*m_bariter);
}

t_lsegment* CScorePlacement::get_first_lsegment()
{
  m_lsegment_iter = m_lsegment_list.begin();
  if (m_lsegment_iter == m_lsegment_list.end())
    return NULL;
  return &(*m_lsegment_iter);
}

t_lsegment* CScorePlacement::get_next_lsegment()
{
  m_lsegment_iter++;
  if (m_lsegment_iter == m_lsegment_list.end())
    return NULL;
  return &(*m_lsegment_iter);
}

t_polygon* CScorePlacement::get_first_polygon()
{
  m_polygon_iter = m_polygon_list.begin();
  if (m_polygon_iter == m_polygon_list.end())
    return NULL;
  return &(*m_polygon_iter);
}

t_polygon* CScorePlacement::get_next_polygon()
{
  m_polygon_iter++;
  if (m_polygon_iter == m_polygon_list.end())
    return NULL;
  return &(*m_polygon_iter);
}

t_mesh* CScorePlacement::get_first_mesh()
{
  m_mesh_iter = m_rendered_meshes.begin();
  if (m_mesh_iter == m_rendered_meshes.end())
    return NULL;
  return &(*m_mesh_iter);
}

t_mesh* CScorePlacement::get_next_mesh()
{
  m_mesh_iter++;
  if (m_mesh_iter == m_rendered_meshes.end())
    return NULL;
  return &(*m_mesh_iter);
}

t_string_placement *CScorePlacement::get_first_txt()
{
  m_txt_iter = m_txt_list.begin();
  if (m_txt_iter == m_txt_list.end())
    return NULL;
  return &(*m_txt_iter);
}

t_string_placement *CScorePlacement::get_next_txt()
{
  m_txt_iter++;
  if (m_txt_iter == m_txt_list.end())
    return NULL;
  return &(*m_txt_iter);
}

CInstrumentHand* CScorePlacement::get_hand()
{
  return m_phand;
}

bool CScorePlacement::is_in(CNote *pn)
{
  std::list<t_note_sketch>::iterator iter;

  iter = m_rendered_notes_list.begin();
  while (iter != m_rendered_notes_list.end())
    {
      if ((*iter).pnote == pn)
	return true;
      iter++;
    }
  return false;
}

bool CScorePlacement::is_in(CMesure *pm)
{
  std::list<t_bar_sketch>::iterator iter;

  iter = m_rendered_bars_list.begin();
  while (iter != m_rendered_bars_list.end())
    {
      if ((*iter).pmesure == pm)
	return true;
      iter++;
    }
  return false;
}

// -------------------------------------------------------
// Edit, screen coordinates
// -------------------------------------------------------

t_fcoord CScorePlacement::get_ortho_size()
{
  return m_fdim;
}

double CScorePlacement::get_time_from_appx(int x)
{
  double res;

  res = (double)(x - m_wpos.x) * m_viewtime / (double)m_wdim.x;
  res += m_starttime;
  return res;
}

double CScorePlacement::get_time_interval_from_appx(int xstart, int xstop)
{
  double res;

  res = (double)(xstop - xstart) * m_viewtime / (double)m_wdim.x;
  return res;
}

int CScorePlacement::get_appx_from_time(double tcode)
{
  int x;

  if (tcode > m_starttime && tcode < m_starttime + m_viewtime)
    {
      x = (int)((tcode - m_starttime)* (double)m_wdim.x  / m_viewtime);
      x += m_wpos.x;
    }
  else
    x = (tcode >= m_starttime + m_viewtime)? m_wpos.x + m_wdim.x : -1;
  return x;
}

bool CScorePlacement::is_in_area(t_fcoord coord, t_fcoord pos, t_fcoord dim)
{
  return (pos.x <= coord.x && coord.x <= pos.x + dim.x &&
	  pos.y <= coord.y && coord.y <= pos.y + dim.y);
}

enoteitempos CScorePlacement::is_on_note_element(t_coord mouse_pos, int *pfreq_number, t_note_sketch** pns_out)
{
  std::list<t_note_sketch>::iterator    iter;
  t_note_sketch                        *pns;
  std::list<t_note_draw_info>::iterator note_info_iter;
  t_fcoord                              fmouse_pos;
  t_fcoord                              circledim;
  t_fcoord                              topleft;
  int                                   freqnumcnt;
  enoteitempos                          ret;
  t_note_sketch                        *last_pns_out;

  fmouse_pos.x = appx_2_placementx(mouse_pos.x);
  fmouse_pos.y = appy_2_placementy(mouse_pos.y);
  circledim.y = 2. * m_zradius;
  circledim.x = 2. * m_zradius;
  last_pns_out = NULL;
  *pfreq_number = -1;
  ret = error_no_element;
  freqnumcnt = 0;
  iter = m_rendered_notes_list.begin();
  while (iter != m_rendered_notes_list.end())
    {
      pns = &(*iter);
      // First check if it is in a note circle
      {
	freqnumcnt = 0;
	note_info_iter = pns->noteinfolist.begin();
	//printf("noteinfo list size = %d\n", (int)pns->noteinfolist.size());
	while (note_info_iter != pns->noteinfolist.end())
	  {
	    topleft.x = (*note_info_iter).circle.center.x - circledim.x / 2.;
	    topleft.y = (*note_info_iter).circle.center.y - circledim.y / 2.;
	    //printf("checking %f, %f in %f, %f sx=%f, sy=%f\n", fmouse_pos.x, fmouse_pos.y, topleft.x, topleft.y, circledim.x, circledim.y);
	    if (is_in_area(fmouse_pos, topleft, circledim))
	      {
		ret = (pns->noteinfolist.size() > 1? chord : note);
		*pns_out = pns;
		*pfreq_number = freqnumcnt;
		return ret; // Return directly the chord or note
	      }
	    freqnumcnt += 1;
	    note_info_iter++;
	  }
      }
      // Top tail or beam zone
      if (is_in_area(fmouse_pos, pns->pos, pns->dim))
	{
	  if (is_in_area(fmouse_pos, pns->pos, pns->beamboxdim))
	    {
	      //printf("beam\n");
	      last_pns_out = pns;
	      ret = beam;
	    }
	  topleft = pns->pos;
	  topleft.y += pns->beamboxdim.y;
	  if (is_in_area(fmouse_pos, topleft, pns->tailboxdim))
	    {
	      //printf("tail\n");
	      last_pns_out = pns;
	      ret = tail;
	    }
	}
      iter++;
    }
  // The return value is nothing or a note tailbox area
  *pns_out = last_pns_out;
  return ret;
}

t_measure_number* CScorePlacement::is_on_bar_number(t_coord mouse_pos)
{
  std::list<t_measure_number>::iterator iter;
  t_measure_number *pbn;
  t_fcoord          fmouse_pos;

  fmouse_pos.x = appx_2_placementx(mouse_pos.x);
  fmouse_pos.y = appy_2_placementy(mouse_pos.y);
  iter = m_rendered_barnumber_list.begin();
  while (iter != m_rendered_barnumber_list.end())
    {
      pbn = &(*iter);
      if (is_in_area(fmouse_pos, pbn->pos, pbn->dim))
	{
	  return pbn;
	}
      iter++;
    }
  return NULL;
}

t_bar_sketch* CScorePlacement::is_on_measure(t_coord mouse_pos)
{
  std::list<t_bar_sketch>::iterator iter;
  t_bar_sketch *pbs;
  t_fcoord      fmouse_pos;

  fmouse_pos.x = appx_2_placementx(mouse_pos.x);
  fmouse_pos.y = appy_2_placementy(mouse_pos.y);
  iter = m_rendered_bars_list.begin();
  while (iter != m_rendered_bars_list.end())
    {
      pbs = &(*iter);
      if (is_in_area(fmouse_pos, pbs->boxpos, pbs->boxdim))
	{
	  return pbs;
	}
      iter++;
    }
  return NULL;
}

int CScorePlacement::note2appy(int note, int octave)
{
  float   fy;
  int     y;

  fy = note2y(note, octave);
  y = fy * (float)m_wdim.y / m_fdim.y;
  y += m_wpos.y;
  return y;
}

int CScorePlacement::frequency2appy(float freq)
{
  int   note, octave;
  float error;

  m_f2n.convert(freq, &octave, &note, &error);
  return note2appy(note, octave);
}

float CScorePlacement::appy2notefrequ(int int_y)
{
  int   baseoctave;
  float octave4y;
  int   octave;
  int   note;
  float y, part, freq;

  y = appy_2_placementy(int_y);
  // An octave is 7 times the radius of a note
  baseoctave = 4; // FIX if baseoctave is 0 then id drifts down 4 notes because of the drawzone limit condition
  octave4y = note2y(0, baseoctave);
  part = (octave4y - y) / (BAROQUE_NOTES_PER_OCTAVE * m_zradius);
  octave = baseoctave + (int)(floor(part));
  if (octave < 0)
    {
      octave = 0;
      note = 0;
      return m_f2n.note2frequ(note, octave);
    }
  octave = octave > 8? 8 : octave;
  //printf("octave=%d, part=%d\n", octave, (int)(floor(part)));
  // Ceci n'est pas lin�aire 14 -> 12
  part = part - floor(part);
  note = (int)floor(part * 2. * BAROQUE_NOTES_PER_OCTAVE);
  note = note > 13? 13 : (note <  0?  0 : note);
  note = m_y2n[note];
  if (octave == 8 && note > 0)
    note = 0;
  freq = m_f2n.note2frequ(note, octave);
  printf("note=%d octave=%d f=%f\n", note, octave, freq);
  return freq;
}

float CScorePlacement::appx_2_placementx(int int_x)
{
  float x;

  // Convert from app coordinates to independent float coordinates
  int_x -= m_wpos.x;
  x = (float)(int_x) * m_fdim.x / (float)m_wdim.x;
  return x;
}

float CScorePlacement::appy_2_placementy(int int_y)
{
  float y;

  // Convert from app coordinates to independent float coordinates
  int_y -= m_wpos.y;
  y = (float)(int_y) * m_fdim.y / (float)m_wdim.y;
  return y;
}

float CScorePlacement::placement_y_2app_area_y(float y)
{
  return (y * (float)m_wdim.y / m_fdim.y);
}

t_fcoord CScorePlacement::placement_size_2app_area_size(t_fcoord coord)
{
  t_fcoord out;

  out.x = (coord.x * (float)m_wdim.x / m_fdim.x);
  out.y = placement_y_2app_area_y(coord.y);
  return out;
}

t_fcoord CScorePlacement::app_area_size2placement_size(t_fcoord coord)
{
  t_fcoord out;

  out.x = (coord.x * m_fdim.x / (float)m_wdim.x);
  out.y = (coord.y * m_fdim.y / (float)m_wdim.y);
  return out;
}

void CScorePlacement::change_vertical_zoom(int inc, t_coord mousepos)
{
  float border, prevborder;
  float mousey, dimy, ratio;

  if (inc == 0)
    return ;
  prevborder = (m_fdim.y * m_vzoom - m_fdim.y);
  m_vzoom += ((float)inc) / 20.;
  m_vzoom = m_vzoom < 1.? 1. : m_vzoom;
  m_vzoom = m_vzoom > 2.? 2. : m_vzoom;
  m_zradius = m_radius * m_vzoom;
  mousey = appy_2_placementy(mousepos.y);
  dimy = m_fdim.y;
  ratio = (mousey) / dimy;
  border = (m_fdim.y * m_vzoom - m_fdim.y);
  m_vmove += (border - prevborder) * ratio;
  //printf("ratio %f border %f move %f\n", ratio, border, m_vmove);
  m_vmove = m_vmove > border? border : m_vmove;
  m_vmove = m_vmove >= 0? m_vmove : 0;
}

void CScorePlacement::change_voffset(int inc)
{
  float border;

  if (inc == 0)
    return ;
  border = (m_fdim.y * m_vzoom - m_fdim.y);
  //printf("change vmove = %f border=%f inc=%d\n", m_vmove, border, inc);
  m_vmove += (float)inc * m_fdim.y / (float)m_wdim.y;
  m_vmove = m_vmove < border? m_vmove : border;
  m_vmove = m_vmove >= 0? m_vmove : 0;
}

void CScorePlacement::set_metrics(Cgfxarea *pw, double starttime, double viewtime, bool bpracticing)
{
  pw->get_pos(&m_wpos);
  pw->get_dim(&m_wdim);
  m_yxratio = (float)m_wdim.y / (float)m_wdim.x;
  m_fdim.y = 1;
  m_fdim.x = 1 / m_yxratio;
  m_viewtime = viewtime;
  m_starttime = starttime;
  m_bpracticing = bpracticing;
}

// Only usefull with the piano and his two keys
int CScorePlacement::get_key(int absnote)
{
  return 0;
}

int CScorePlacement::get_key(CNote *pnote)
{
  return 0;
}

// -------------------------------------------------------
// Placement, floating point coordinates
// -------------------------------------------------------

float CScorePlacement::note2y(int note, int octave)
{
  float     y;
  float     yoffset;
  int       C4note, C4octave;
  int       C4absnote;

  // The reference note is octave 4 'midi' note 0 = C4 / do 4. yoffset is on that note
  // The note is counted in a 7 note span interval
  C4note = 0;
  C4octave = 4;
  C4absnote = (C4octave * BAROQUE_NOTES_PER_OCTAVE) + C4note;
  yoffset = (float)(m_notenum - C4absnote - 1) * m_zradius; // 1 note space down
  y = yoffset + (float)(BAROQUE_NOTES_PER_OCTAVE * (C4octave - octave) - m_n2k[note].notelevel) * m_zradius - m_vmove;
  //y = y < 0? 0 : y;
  //y = y > m_fdim.y? m_fdim.y : y;
  return y;
}

void CScorePlacement::place_segment_portee(int note, int octave, float xstart, float length)
{
  t_lsegment lsegment;
  bool       bDFA;

  bDFA = note == 2 || note == 5 || note == 6 || note == 8 || note == 9;
  if (((octave & 1) == 1 && bDFA) ||
      ((octave & 1) == 0 && !bDFA))
    {
      lsegment.pos.x = xstart;
      lsegment.pos.y = note2y(note, octave);
      lsegment.dim.x = length;
      lsegment.dim.y = 0.;
      lsegment.thickness = 1. * m_vzoom;
      m_lsegment_list.push_back(lsegment);
    }
}

void CScorePlacement::place_portee()
{
  int octave;

  // C=0   D=1  E=2  F=3   G=4  A=5  B=6
  // Do=0 Re=1 Mi=2 Fa=3 Sol=4 La=5 Si=6
  //
  // Do=0 Re=2 Mi=4 Fa=5 Sol=7 La=9 Si=11
  octave = 5;
  // Fa
  place_segment_portee(5, octave, 0., m_fdim.x);
  // Re
  place_segment_portee(2, octave, 0., m_fdim.x);
  octave = 4;
  // Si
  place_segment_portee(11, octave, 0., m_fdim.x);
  // Sol
  place_segment_portee(7, octave, 0., m_fdim.x);
  // Mi
  place_segment_portee(4, octave, 0., m_fdim.x);
}

void CScorePlacement::place_keys()
{
  int color = BLACK;
  t_fcoord pos, dim;

  pos.x = m_zradius / 2.;
  pos.y = note2y(9, 5);
  dim.y = note2y(0, 4) - pos.y;
  dim.x = m_zradius * 4;
  place_mesh(pos, dim, color, (char*)"clefsol");
}

void CScorePlacement::place_sharpflat(t_fcoord pos, float radius, int color, bool bdiese)
{
  t_fcoord dim;

  pos.x += 2.3 * m_zradius;
  pos.y -= 0.8 * m_zradius;
  dim.x = bdiese? 2.5 * m_zradius : 1.7 * m_zradius;
  dim.y = 2.5 * m_zradius;
  if (bdiese)
    place_mesh(pos, dim, color, (char*)"diese");
  else
    place_mesh(pos, dim, color, (char*)"bemol");
}

void CScorePlacement::place_bars(std::list<CMesure*> *pml)
{
  t_bar_sketch b;
  std::list<CMesure*>::reverse_iterator it;
  double  timecode;
  double  interval;
  double  next_measure_start;
  double  last_limit;
  float   x, y, length;
  bool    bfirstbar;
  t_measure_number mn;

  place_keys();
  place_portee();
  y = note2y(5, 5);
  length = fabs(note2y(4, 4) - y);
  it = pml->rbegin();
  last_limit = m_starttime + m_viewtime;
  next_measure_start = 1000000000;
  while (it != pml->rend())
    {
      if ((*it)->m_start < last_limit)
	{
	  bfirstbar = true;
	  last_limit = (*it)->m_start;
	  //printf("start %f stop %f\n", (*it)->m_start, (*it)->m_stop);
	  interval = (*it)->m_stop - (*it)->m_start;
	  //printf("drawing shit y=%d length=%d interval=%f\n", y, length, interval);
	  timecode = (*it)->m_start;
	  while (timecode < m_starttime + m_viewtime &&
		 timecode < next_measure_start)
	    {
	      if (timecode > m_starttime)
		{
		  x = ((timecode - m_starttime) * m_fdim.x / m_viewtime);
 		  if (x > KEY_OFFSET)
		    {
		      b.timecode = timecode;
		      b.boxpos.x = x - m_zradius;
		      b.boxpos.y = y;
		      b.boxdim.y = length;
		      b.boxdim.x = 2. * m_zradius;
		      b.pmesure = (*it);
		      // Double bar if on the first one
		      b.doublebar = (bfirstbar && timecode == (*it)->m_start);
		      add_bar_sketch(b);
		      if (bfirstbar)
			{
			  mn.pos.x = b.boxpos.x - 4. * m_zradius;
			  mn.pos.y = y;
			  mn.dim.x = 4. * m_zradius;
			  mn.dim.y = 4. * m_zradius;
			  mn.pmesure = (*it);
			  m_rendered_barnumber_list.push_front(mn);
			  bfirstbar = false;
			}
		    }
		}
	      timecode += interval;
	    }
	}
      next_measure_start = (*it)->m_start;
      it++;
    }
}

// Note index in times / 4 from the first bar (the double bar)
void CScorePlacement::calculate_absolute_note_index(t_measure_note_info *pmni)
{
  int absolute_note_index;

  if (pmni->pmesure == NULL)
    absolute_note_index = 0;
  else
    {
      absolute_note_index = pmni->measure_index * pmni->pmesure->m_times * 16;
      absolute_note_index += pmni->mnstart;
    }
  pmni->absolute_index = absolute_note_index;
}

void CScorePlacement::place_note_segments(int octave, int note, float x)
{
  int i, absnote, tmpoctave, tmpnote;

  // If the note is above octave 5 sol / G5 then draw the missing lines
  if (octave > 5 || (octave == 5 && note >= 8))
    {
      absnote = m_f2n.notenum(octave, note);
      i       = m_f2n.notenum(5, 8);
      for (; i <= absnote; i++)
	{
	  m_f2n.noteoctave(i, &tmpnote, &tmpoctave);
	  place_segment_portee(tmpnote, tmpoctave, x - 2. * m_zradius, 4. * m_zradius);
	}
    }
  // Same thing below D4
  if (octave < 4 || (octave == 4 && note < 2))
    {
      absnote = m_f2n.notenum(octave, note);
      i       = m_f2n.notenum(4, 1);
      for (; i >= absnote; i--)
	{
	  m_f2n.noteoctave(i, &tmpnote, &tmpoctave);
	  place_segment_portee(tmpnote, tmpoctave, x - 2. * m_zradius, 4. * m_zradius);
	}
    }  
}

void CScorePlacement::find_note_type(t_measure_note_info *pmni, double notelen)
{
  double lothr, hithr;

  // A note is 1. and the overlaping threshold is 1/8note below 1 time, and 1/4note over 1 time
  lothr = 1. / 8.;
  hithr  = lothr * 2.;
  // note or more
  if (notelen >= 1. - lothr)
    {
      // ronde              blanche     noire
      // double whole note, whole note, note
      if (notelen >= 1. - lothr && notelen < 2. - hithr)
	{
	  pmni->note_type = noire;
	  pmni->bpointed = notelen >= 1.5 - hithr;
	}
      else
	{
	  if (notelen >= 2. - hithr && notelen < 4. - hithr)
	    {
	      pmni->note_type = blanche;
	      pmni->bpointed = notelen >= 3. - hithr;
	    }
	  else
	    {
	      pmni->note_type = ronde;
	      pmni->bpointed = notelen >= 6. - hithr;
	    }
	}
    }
  else
    {
      // croche
      // eighth note
      if (notelen >= 0.5 - lothr)
	{
	  pmni->note_type = croche;
	  pmni->bpointed = notelen >= 0.75 - lothr;
	}
      else
	{
	  // double croche 1/16
	  // semiquaver
	  pmni->note_type = doublecroche;
	}
    }
}

void CScorePlacement::find_in_measure_note_index(t_measure_note_info *pmni, double notetimecode)
{
  double tolerance_duration;
  double notemtime;
  double noteindex;
  double timediv;
  double measure_timecode;
  double measure_duration;
  double time_duration;
  CMesure *pmesure;

  timediv = 1.;
  switch (pmni->note_type)
    {
    case ronde:
    case blanche:
    case noire:
      timediv = 1.;
      break;
    case croche:
      timediv = 2.;
      break;
    case doublecroche:
      timediv = 4.;
      break;
    case triplecroche:
      timediv = 8.;
      break;
    case quadruplecroche:
      timediv = 16.;
      break;
    default:
      break;
    };
  pmesure = pmni->pmesure;
  measure_duration = pmesure->duration();
  time_duration = measure_duration / (double)pmesure->m_times;
  tolerance_duration = time_duration / POSITION_THRESHOLD; // The note can be slightly before the measure (1 / 8 note)
  notemtime = (notetimecode + tolerance_duration - pmesure->m_start) / measure_duration;
  // Measure index
  pmni->measure_index = (int)floor(notemtime);
  // Note index in the measure
  measure_timecode = pmesure->m_start + (double)pmni->measure_index * measure_duration;
  noteindex = floor((notetimecode + tolerance_duration - measure_timecode) / (time_duration / timediv));
  noteindex = noteindex < 0? 0 : noteindex;
  pmni->note_index = (int)noteindex;
  //
  pmni->inmtimecode = measure_timecode + ((round(noteindex) + 0.5) * (time_duration / timediv));
  //printf("note tcode=%f index=%f\n", *ptcode, noteindex);
}

void CScorePlacement::calculate_in_measure_note_characteristics(t_measure_note_info *pnf)
{
  int du;

  //
  switch (pnf->note_type)
    {
    case ronde:
      du = 4 * 16;
      break;
    case blanche:
      du = 2 * 16;
      break;
    case noire:
      du = 16;
      break;
    case croche:
      du = 16 / 2;
      break;
    case doublecroche:
      du = 16 / 4;
      break;
    case triplecroche:
      du = 16 / 8;
      break;
    case quadruplecroche:
      du = 16 / 16;
      break;
    default:
      du = 16;
      break;
    }
  pnf->mnduration = du;
  pnf->mnduration = pnf->mnduration + (pnf->bpointed? pnf->mnduration / 2 : 0);
  // If longer than quarter notes, the positioning is like a Quarter note positioning (whole note...)
  pnf->mnstart = pnf->note_type < noire? pnf->note_index * 16 : pnf->note_index * du;
}

int CScorePlacement::get_note_type_from_measure(std::list<CMesure*> *pml, CNote *pn, double *ptcode, t_measure_note_info *pmni)
{
  std::list<CMesure*>::reverse_iterator it;
  double                                timeduration;

  pmni->measure_index = -1;
  pmni->note_index = -1;
  pmni->bpointed = false;   
  // Assuming the list is sorted then the last measure segment begining
  // before the note is the needed mesasure. Hence reverse order iterator.
  it = pml->rbegin();
  while (it != pml->rend())
    {
      pmni->pmesure = (*it);
      timeduration = pmni->pmesure->duration() / (double)(pmni->pmesure->m_times);
      if (pmni->pmesure->m_start < pn->m_time + (timeduration / POSITION_THRESHOLD))
	{
	  // Find the note type depending on the
	  // measure duration and times in the measure.
	  find_note_type(pmni, pn->m_duration / timeduration);
	  // Find in which measure the note is, counting from the first bar with a slight tolerance to the left of the bar.
	  // Sets *ptode with the note time in the measure available positions.
	  find_in_measure_note_index(pmni, pn->m_time);
	  // Start time n duration in 16th of time based on note type
	  calculate_in_measure_note_characteristics(pmni);
	  *ptcode = pmni->inmtimecode;
	  return 0;
	}
      it++;
    }
  // No corresponding measure
  *ptcode = pn->m_time;
  pmni->inmtimecode = pn->m_time; // Used for editing
  pmni->note_type = noire;
  pmni->pmesure = NULL;
  pmni->mnstart = 0;
  pmni->mnduration = 16;
  return -1;
}

// Used to insert properly the edited note, in the middle of the other notes
void CScorePlacement::insert_note_sketch(std::list<t_note_sketch> *pnoteinflist, t_note_sketch *pns)
{
  std::list<t_note_sketch>::iterator iter;

  iter = pnoteinflist->begin();
  while (iter != pnoteinflist->end())
    {
      if (compare(pns, &(*iter)))
	{
	  iter++;
	  pnoteinflist->insert(iter, *pns);
	  return;
	}
      iter++;
    }
  // Empty list case or last note case
  pnoteinflist->push_back(*pns);
}

void CScorePlacement::place_note(std::list<CMesure*> *pml, CNote *pn, std::list<t_note_sketch> *prendered_notes_list)
{
  t_note_sketch        ns;
  double               tcode;
  t_note_draw_info     noteinfo;
  t_measure_note_info *pnf;
  float                f;
  int                  octave;
  int                  note;
  float                error;
  float                lpoint;
  float                hpoint;
  float                x, y;
  float                tail_length;
  std::list<t_notefreq>::iterator iter;

  //printf("notetime=%f\n", pn->m_time);
  if (pn->m_time >= m_starttime + m_viewtime)
    return;
  //printf("note %d start time=%f notetime=%f viewtime=%f\n", i++, starttime, pn->m_time, m_viewtime);
  // "tcode" is based on the measure position if the note is in a measure.
  pnf = &ns.nf;
  get_note_type_from_measure(pml, pn, &tcode, pnf);
  calculate_absolute_note_index(pnf);
/*
  switch (ns.type)
    {
    case ronde:
      printf("ronde\n");
      break;
    case blanche:
      printf("blanche\n");
      break;
    case noire:
      printf("noire\n");
      break;
    case croche:
      printf("croche\n");
      break;
    case doublecroche:
      printf("doublecroche\n");
      break;
    default:
      printf("defaultnotetype=%d\n", ns.type);
      break;
    };
  printf("note timecode=%f\n", tcode);
*/
  // position of the center of the note
  x = ((tcode - m_starttime) * (float)m_fdim.x / m_viewtime);
  if (x >= KEY_OFFSET && x < m_fdim.x + m_zradius)
    {
      // Note circles encasing boxes
      lpoint = -1;
      hpoint = -1;
      ns.noteinfolist.clear();
      iter = pn->m_freq_list.begin();
      while (iter != pn->m_freq_list.end())
	{
	  f = (*iter).f;
	  m_f2n.convert(f, &octave, &note, &error);
	  // 
	  y = note2y(note, octave);
	  lpoint = lpoint == -1? y : (y > lpoint? y : lpoint);
	  hpoint = hpoint == -1? y : (y < hpoint? y : hpoint);
	  noteinfo.circle.center.x = x;
	  noteinfo.circle.center.y = y;
	  noteinfo.circle.radius = m_zradius;
	  noteinfo.isdiese = m_n2k[note].type == ediese;
	  noteinfo.isbemol = m_n2k[note].type == ebemol;
	  if (noteinfo.isdiese)
	    {
	      place_sharpflat(noteinfo.circle.center, m_zradius, BLACK, noteinfo.isdiese);
	    }
	  if (noteinfo.isbemol)
	    {
	      place_sharpflat(noteinfo.circle.center, m_zradius, BLACK, noteinfo.isdiese);
	    }
	  noteinfo.string_num = (*iter).string;
	  //noteinfo.xstart = x;
	  // Here xstart is based on the real note time, and not the measure related time
	  noteinfo.xstart = (pn->m_time - m_starttime) * (float)m_fdim.x / m_viewtime;
	  noteinfo.len = pn->m_duration * m_fdim.x / m_viewtime;
	  noteinfo.len = noteinfo.len > m_fdim.x? m_fdim.x : noteinfo.len;
	  ns.noteinfolist.push_back(noteinfo);
	  place_note_segments(octave, note, x);
	  iter++;
	}
      // Tail
      ns.pos.x = x - m_zradius;
      ns.dim.x = 2. * m_zradius; // Must be scaled on the x axis, because it is zoomed on y and would look like an ellipse if not scaled
      tail_length = 5. * m_zradius;
      ns.pos.y = hpoint - tail_length;
      ns.dim.y = tail_length - m_zradius;
      ns.beamboxdim.x = ns.dim.x;
      ns.tailboxdim.x = ns.dim.x;
      ns.beamboxdim.y = 1.5 * m_zradius;
      ns.tailboxdim.y = ns.dim.y - ns.beamboxdim.y;
      ns.pnote = pn;
      // Time & frequencybox

      // Note placement per measure, used to place the beams and find the rests
      insert_note_sketch(prendered_notes_list, &ns);
    }
}

// Used when adding a note from the spectrometer
void CScorePlacement::set_tmp_note(CNote *pnote)
{
  //printf("set tmp note\n");
  m_tmp_note = *pnote;
}

void CScorePlacement::change_tmp_note(double timecode, double duration, float f)
{
  t_notefreq fr;

  //printf("set tmp note\n");
  m_tmp_note.m_time = timecode;
  m_tmp_note.m_duration = duration;
  m_tmp_note.m_freq_list.clear();
  fr.f = f;
  fr.string = m_phand->get_first_string(fr.f);
  m_tmp_note.m_freq_list.push_front(fr);
}

CNote* CScorePlacement::get_tmp_note()
{
  if (m_tmp_note.m_time != -1)
    return &m_tmp_note;
  return NULL;
}

void CScorePlacement::place_notes(std::list<CMesure*> *pml, CInstrument *pinstrument, std::list<t_note_sketch> *pnote_placement_list)
{
  CNote                     *pn;
  std::list<float>::iterator iter;

  //printf("radisu=%f->%fpixels\n", m_radius, m_zradius * (float)m_wdim.x / m_fdim.x);
  if (pinstrument == NULL)
    {
      printf("Instrument identification error\n");
      return ;
    }
  pn = pinstrument->get_first_note(m_starttime);
  while (pn != NULL)
    {
      place_note(pml, pn, pnote_placement_list);
      pn = pinstrument->get_next_note();
    }
  pn = get_tmp_note();
  if (pn != NULL)
    {
      place_note(pml, pn, pnote_placement_list);
    }
  place_beams(pnote_placement_list, BLACK); // Also used for flags
  // Shows how a measure is divided
  //fill_unused_times(pscore, pinstrument);
}

void CScorePlacement::place_practice_index()
{
  float x;
  float y;
  float dimx;
  float dimy;

  if (m_bpracticing)
    {
      x = m_fdim.x / 2;
      y = 0;
      dimx = m_fdim.x / (float)m_wdim.x;
      dimy = m_fdim.y;
      add_rectangle_polygon(x, y, dimx, dimy, BLACK);
    }
}

void CScorePlacement::place_notes_and_bars(CScore *pscore, CInstrument *pinstrument)
{
  std::list<CMesure*>   *pml;

  pml = &pscore->m_mesure_list;
  place_notes(pml, pinstrument, &m_rendered_notes_list);
  place_rests(pml, &m_rendered_notes_list);
  place_bars(pml);
  place_practice_index();
}

