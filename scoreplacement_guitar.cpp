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
#include "scoreplacement_guitar.h"
#include "scoreedit.h"
#include "scorerenderer.h"

using namespace std;

CScorePlacementGuitar::CScorePlacementGuitar(int notenum):CScorePlacement(notenum)
{
  m_radius = m_fdim.y / (float)(m_notenum + 1 + 4);
  m_zradius = m_radius * m_vzoom;
  m_phand = new CInstrumentHandGuitar();
}

CScorePlacementGuitar::~CScorePlacementGuitar()
{
  delete m_phand;
}

float CScorePlacementGuitar::note2y(int note, int octave)
{
  float     y;
  float     yoffset;
  int       Cnote, Coctave;
  int       Cabsnote;

  //
  // The reference note is octave 3 'midi' note 0 = C3 / do 3. yoffset is on that note
  // The note is counted in a 7 note span interval
  //
  Cnote = 0;
  Coctave = 3;
  Cabsnote = (Coctave * BAROQUE_NOTES_PER_OCTAVE) + Cnote;
  yoffset = (float)(m_notenum - Cabsnote + 2) * m_zradius;
  y = yoffset + (float)(BAROQUE_NOTES_PER_OCTAVE * (Coctave - octave) - m_n2k[note].notelevel) * m_zradius - m_vmove;
  return y;
}

void CScorePlacementGuitar::place_note_segments(int octave, int note, float x)
{
  int i, absnote, tmpoctave, tmpnote;

  // If the note is above octave 4 sol / G4 then draw the missing lines
  if (octave > 4 || (octave == 4 && note >= 8))
    {
      absnote = m_f2n.notenum(octave, note);
      i       = m_f2n.notenum(4, 8);
      for (; i <= absnote; i++)
	{
	  m_f2n.noteoctave(i, &tmpnote, &tmpoctave);
	  place_segment_portee(tmpnote, tmpoctave, x - 2. * m_zradius, 4. * m_zradius);
	}
    }
  // Same thing below D3
  if (octave < 3 || (octave == 3 && note < 2))
    {
      absnote = m_f2n.notenum(octave, note);
      i       = m_f2n.notenum(3, 1);
      for (; i >= absnote; i--)
	{
	  m_f2n.noteoctave(i, &tmpnote, &tmpoctave);
	  place_segment_portee(tmpnote, tmpoctave, x - 2. * m_zradius, 4. * m_zradius);
	}
    }
}

float CScorePlacementGuitar::get_string_segment_y(int string)
{
  float E2y;

  E2y = note2y(4, 2) + 4 * m_zradius;
  return (E2y + string * 2 * m_zradius);
}

void CScorePlacementGuitar::place_TAB_segments(float xstart, float length)
{
  t_lsegment         lsegment;
  int                i;
  float              border;
  t_string_placement txtpl;

  lsegment.dim.x = length;
  lsegment.dim.y = 0.;
  lsegment.thickness = 1. * m_vzoom;
  border = 4 * m_zradius;
  lsegment.pos.x = xstart + border;
  txtpl.pos.x = xstart;
  txtpl.dim.x = border;
  txtpl.dim.y = 1.8 * m_zradius;
  for (i = 0; i < 6; i++)
    {
      lsegment.pos.y = get_string_segment_y(i);
      m_lsegment_list.push_back(lsegment);
      txtpl.pos.y = lsegment.pos.y - txtpl.dim.y / 2.;
      switch (i)
	{
	case 0:
	case 5:
	  txtpl.txt = string("E");
	  break;
	case 1:
	  txtpl.txt = string("B");
	  break;
	case 2:
	  txtpl.txt = string("G");
	  break;
	case 3:
	  txtpl.txt = string("D");
	  break;
	case 4:
	  txtpl.txt = string("A");
	  break;
	}
      txtpl.color = BLACK;
      txtpl.pnf = NULL;
      m_txt_list.push_back(txtpl);
    }
}

void CScorePlacementGuitar::place_segment_portee(int note, int octave, float xstart, float length)
{
  t_lsegment lsegment;
  bool       bDFA;

  bDFA = note == 2 || note == 5 || note == 6 || note == 8 || note == 9;
  if (((octave & 1) == 0 && bDFA) ||
      ((octave & 1) == 1 && !bDFA))
    {
      lsegment.pos.x = xstart;
      lsegment.pos.y = note2y(note, octave);
      lsegment.dim.x = length;
      lsegment.dim.y = 0.;
      lsegment.thickness = 1. * m_vzoom;
      m_lsegment_list.push_back(lsegment);
    }
}

void CScorePlacementGuitar::place_portee()
{
  int octave;

  // C=0   D=1  E=2  F=3   G=4  A=5  B=6
  // Do=0 Re=1 Mi=2 Fa=3 Sol=4 La=5 Si=6
  //
  // Do=0 Re=2 Mi=4 Fa=5 Sol=7 La=9 Si=11
  octave = 4;
  // Fa
  place_segment_portee(5, octave, 0., m_fdim.x);
  // Re
  place_segment_portee(2, octave, 0., m_fdim.x);
  octave = 3;
  // Si
  place_segment_portee(11, octave, 0., m_fdim.x);
  // Sol
  place_segment_portee(7, octave, 0., m_fdim.x);
  // Mi
  place_segment_portee(4, octave, 0., m_fdim.x);
  //
  place_TAB_segments(0., m_fdim.x);
}

void CScorePlacementGuitar::place_keys()
{
  int color = BLACK;
  t_fcoord pos, dim;
  t_string_placement txtpl;

  pos.x = m_zradius / 2.;
  pos.y = note2y(9, 4);
  dim.y = note2y(0, 3) - pos.y;
  dim.x = m_zradius * 4;
  place_mesh(pos, dim, color, (char*)"clefsol");
  //
  txtpl.pos.x = m_zradius;
  txtpl.pos.y = note2y(0, 3);
  txtpl.dim.x = m_zradius * 2.5;
  txtpl.dim.y = m_zradius * 2.5;
  txtpl.txt = string("8");
  txtpl.color = BLACK;
  txtpl.pnf = NULL;
  m_txt_list.push_back(txtpl);
}

void CScorePlacementGuitar::place_bars(std::list<CMesure*> *pml)
{
  t_bar_sketch b;
  std::list<CMesure*>::reverse_iterator it;
  double  timecode;
  double  interval;
  double  next_measure_start;
  double  last_limit;
  float   x, length;
  bool    bfirstbar;
  t_measure_number mn;

  place_keys();
  place_portee();
  length = fabs(note2y(4, 3) - note2y(5, 4));
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
		      b.boxdim.y = length;
		      b.boxdim.x = 2. * m_zradius;
		      b.pmesure = (*it);
		      // Double bar if on the first one
		      b.doublebar = (bfirstbar && timecode == (*it)->m_start);
		      b.boxpos.y = note2y(5, 4);
		      add_bar_sketch(b);
		      b.boxdim.y = get_string_segment_y(5) - get_string_segment_y(0);
		      b.boxpos.y = get_string_segment_y(0);
		      add_bar_sketch(b);
		      if (bfirstbar)
			{
			  mn.pos.x = b.boxpos.x - 4. * m_zradius;
			  mn.dim.x = 4. * m_zradius;
			  mn.dim.y = 4. * m_zradius;
			  mn.pmesure = (*it);
			  mn.pos.y = note2y(5, 4);
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

float CScorePlacementGuitar::appy2notefrequ(int int_y)
{
  int   baseoctave;
  float octave3y;
  int   octave;
  int   note;
  float y, part, freq;

  y = appy_2_placementy(int_y);
  // An octave is 7 times the radius of a note
  baseoctave = 3; // FIX if baseoctave is 0 then id drifts down 4 notes because of the drawzone limit condition
  octave3y = note2y(0, baseoctave);
  part = (octave3y - y) / (BAROQUE_NOTES_PER_OCTAVE * m_zradius);
  octave = baseoctave + (int)(floor(part));
  if (octave < 0)
    {
      octave = 0;
      note = 0;
      return m_f2n.note2frequ(note, octave);
    }
  octave = octave > 6? 6 : octave;
  //printf("octave=%d, part=%d\n", octave, (int)(floor(part)));
  // Not linear, 14 intervals -> 12 notes
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

void CScorePlacementGuitar::place_TAB_notes(std::list<CMesure*> *pml, CNote *pn)
{
  std::list<t_notefreq>::iterator iter;
  float                xpos, ypos;
  int                  note, string_number;
  int                  basenote;
  bool                 bfirstoctave;
  t_string_placement   txtpl;
  const int            ctxtsz = 256;
  char                 txt[ctxtsz];
  t_note_sketch        ns;
  t_measure_note_info *pnf;
  double               tcode;

  if (pn->m_time >= m_starttime + m_viewtime)  
    return;
  pnf = &ns.nf;
  get_note_type_from_measure(pml, pn, &tcode, pnf); // Get the in measure timecode
  xpos = (tcode - m_starttime) * (float)m_fdim.x / m_viewtime;
  if (xpos >= KEY_OFFSET && xpos < m_fdim.x + m_zradius)
    {
      iter = pn->m_freq_list.begin();
      while (iter != pn->m_freq_list.end())
	{
	  assert((*iter).f != -1);
	  string_number = (*iter).string;
	  if (string_number >= 0 && string_number < m_phand->get_string_num())
	    {
	      // The string
	      ypos = get_string_segment_y(string_number) - m_zradius;
	      basenote = m_phand->get_string_abs_note(string_number);
	      // The absnote
	      note = m_f2n.frequ2note((*iter).f);
	      note -= basenote;
	      bfirstoctave = note < 12;
	      if (!bfirstoctave)
		{
		  note = note - 12;
		}
	      txtpl.pos.x = xpos;
	      txtpl.pos.y = ypos;
	      txtpl.dim.x = txtpl.dim.y = 2. * m_zradius;
	      txtpl.color = bfirstoctave? BLACK : 0xFF0264A5;
	      txtpl.pnf = &(*iter);
	      snprintf(txt, ctxtsz, "%d", note);
	      txtpl.txt = std::string(txt);
	      m_txt_list.push_back(txtpl);
	    }
	  iter++;
	}
    }
}

void CScorePlacementGuitar::place_notes(std::list<CMesure*> *pml, CInstrument *pinstrument, std::list<t_note_sketch> *pnote_placement_list)
{
  CNote                     *pn;
  std::list<float>::iterator iter;

  //CScorePlacement::place_notes(pml, pinstrument, pnote_placement_list);
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
      place_TAB_notes(pml, pn);
      pn = pinstrument->get_next_note();
    }
  pn = get_tmp_note();
  if (pn != NULL)
    {
      place_note(pml, pn, pnote_placement_list);
      place_TAB_notes(pml, pn);
    }
  place_beams(pnote_placement_list, BLACK); // Also used for flags
}

//----------------------------------------------------------------------------------------------------------------------
// Drop D version

CScorePlacementGuitarDropD::CScorePlacementGuitarDropD(int notenum):CScorePlacementGuitar(notenum)
{
  delete m_phand;
  m_phand = new CInstrumentHandGuitarDropD();
}

CScorePlacementGuitarDropD::~CScorePlacementGuitarDropD()
{
  delete m_phand;
}

void CScorePlacementGuitarDropD::place_TAB_segments(float xstart, float length)
{
  t_lsegment         lsegment;
  int                i;
  float              border;
  t_string_placement txtpl;

  lsegment.dim.x = length;
  lsegment.dim.y = 0.;
  lsegment.thickness = 1. * m_vzoom;
  border = 4 * m_zradius;
  lsegment.pos.x = xstart + border;
  txtpl.pos.x = xstart;
  txtpl.dim.x = border;
  txtpl.dim.y = 1.8 * m_zradius;
  for (i = 0; i < 6; i++)
    {
      lsegment.pos.y = get_string_segment_y(i);
      m_lsegment_list.push_back(lsegment);
      txtpl.pos.y = lsegment.pos.y - txtpl.dim.y / 2.;
      switch (i)
	{
	case 0:
	  txtpl.txt = string("E");
	  break;
	case 1:
	  txtpl.txt = string("B");
	  break;
	case 2:
	  txtpl.txt = string("G");
	  break;
	case 3:
	  txtpl.txt = string("D");
	  break;
	case 4:
	  txtpl.txt = string("A");
	  break;
	case 5:
	  txtpl.txt = string("D");
	  break;
	}
      txtpl.color = BLACK;
      txtpl.pnf = NULL;
      m_txt_list.push_back(txtpl);
    }
}
