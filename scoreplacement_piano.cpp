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
#include "scoreplacement_piano.h"
#include "scoreedit.h"
#include "scorerenderer.h"

CScorePlacementPiano::CScorePlacementPiano(int notenum):CScorePlacement(notenum)
{
  m_phand = new CInstrumentHandPiano();
}

CScorePlacementPiano::~CScorePlacementPiano()
{
  delete m_phand;
}

// -------------------------------------------------------
// Edit, screen coordinates
// -------------------------------------------------------

float CScorePlacementPiano::appy2notefrequ(int int_y)
{
  int   baseoctave;
  float basey;
  float octave4y;
  float octave3y;
  int   octave;
  int   note;
  float y, part;

  y = appy_2_placementy(int_y);
  // An octave is 7 times the radius of a note
  octave4y = note2y(0, 4);
  octave3y = note2y(11, 3);
  basey = octave3y;
  if (y >= octave3y)
    {
      baseoctave = 3;
      basey = note2y(0, 3);
    }
  if (y <= octave4y)
    {
      baseoctave = 4;
      basey = octave4y;
    }  
  if (y < octave3y && y > octave4y)
    {
      return ((y - octave4y) > ((octave3y - octave4y) / 2)? m_f2n.note2frequ(11, 3) : m_f2n.note2frequ(0, 4));
    }
  //printf("baseoctave = %d y=%f octave3=%f octave4=%f int_y=%d\n", baseoctave, y, octave3y, octave4y, int_y);
  part = (basey - y) / (BAROQUE_NOTES_PER_OCTAVE * m_zradius);
  //printf("part=%f\n", part);
  octave = baseoctave + (int)(floor(part));
  if (octave < 0)
    {
      octave = 0;
      note = 0;
      return m_f2n.note2frequ(note, octave);
    }
  octave = octave > 8? 8 : octave;
  //printf("octave=%d, part=%d\n", octave, (int)(floor(part)));
  // Ceci n'est pas linéaire 14 -> 12
  part = part - floor(part);
  note = (int)floor(part * 2. * BAROQUE_NOTES_PER_OCTAVE);
  note = note > 13? 13 : (note <  0?  0 : note);
  note = m_y2n[note];
  //printf("note=%d\n", note);
  if (octave == 8 && note > 0)
    note = 0;
  return m_f2n.note2frequ(note, octave);
}

// -------------------------------------------------------
// Placement, floating point coordinates
// -------------------------------------------------------

float CScorePlacementPiano::note2y(int note, int octave)
{
  float     y;
  float     yoffset;
  float     additional_offset;
  int       Cnote, Coctave;
  int       Cabsnote;

  // 2 cases:
  //
  // If note >= C4
  // The reference note is octave 4 'midi' note 0 = C4 / do 4. yoffset is on that note
  // The note is counted in a 7 note span interval
  //
  // If note < C4  <=>  note <= B3
  // The reference note is octave 3 'midi' note 0 = C3
  Cnote = 0;
  Coctave = (octave < 4)? 3 : 4;
  additional_offset = (octave < 4)? 5 : 0; // 1 note space down plus space between the 2 keys
  Cabsnote = (Coctave * BAROQUE_NOTES_PER_OCTAVE) + Cnote;
  yoffset = (float)(m_notenum - Cabsnote - 1 + additional_offset) * m_zradius;
  y = yoffset + (float)(BAROQUE_NOTES_PER_OCTAVE * (Coctave - octave) - m_n2k[note].notelevel) * m_zradius - m_vmove;
  //y = y < 0? 0 : y;
  return y;
}

void CScorePlacementPiano::place_portee()
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
  //
  octave = 3;
  // La
  place_segment_portee(9, octave, 0., m_fdim.x);
  // Fa
  place_segment_portee(5, octave, 0., m_fdim.x);
  // Ré
  place_segment_portee(2, octave, 0., m_fdim.x);
  octave = 2;
  // Si
  place_segment_portee(11, octave, 0., m_fdim.x);
  // Sol
  place_segment_portee(7, octave, 0., m_fdim.x);
}

void CScorePlacementPiano::place_keys()
{
  int color = BLACK;
  t_fcoord pos, dim;

  pos.x = m_zradius / 2.;
  pos.y = note2y(9, 5);
  dim.y = note2y(0, 4) - pos.y;
  dim.x = m_zradius * 4;
  place_mesh(pos, dim, color, (char*)"clefsol");
  //
  dim.x = m_zradius * 4;
  dim.y = note2y(9, 2) - note2y(9, 3);
  pos.y = note2y(9, 3);
  place_mesh(pos, dim, color, (char*)"cleffa");
}

void CScorePlacementPiano::place_bars(std::list<CMesure*> *pml)
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
  length = fabs(note2y(4, 4) - note2y(5, 5));
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
		      b.boxpos.y = note2y(5, 5);
		      add_bar_sketch(b);
		      b.boxpos.y = note2y(9, 3);	
		      add_bar_sketch(b);
		      if (bfirstbar)
			{
			  mn.pos.x = b.boxpos.x - 4. * m_zradius;
			  mn.dim.x = 4. * m_zradius;
			  mn.dim.y = 4. * m_zradius;
			  mn.pmesure = (*it);
			  mn.pos.y = note2y(5, 5);
			  m_rendered_barnumber_list.push_front(mn);
			  mn.pos.y = note2y(9, 3);
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

void CScorePlacementPiano::place_note_segments(int octave, int note, float x)
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
  if (octave == 4 && note < 2)
    {
      place_segment_portee(0, 4, x - 2. * m_zradius, 4. * m_zradius);
    }
  if (octave <= 2 || (octave == 2 && note < 7))
    {
      absnote = m_f2n.notenum(octave, note);
      i       = m_f2n.notenum(2, 6);
      for (; i >= absnote; i--)
	{
	  m_f2n.noteoctave(i, &tmpnote, &tmpoctave);
	  place_segment_portee(tmpnote, tmpoctave, x - 2. * m_zradius, 4. * m_zradius);
	}
    }  
}

int CScorePlacementPiano::get_key(int absnote)
{
  int key;
  int octave;
  int note;

  octave = 4;
  note = 0; // C4
  key = absnote >= m_f2n.notenum(octave, note)? 1 : 0;
  return key;
}

int CScorePlacementPiano::get_key(CNote *pnote)
{
  std::list<t_notefreq>::iterator fiter;
  int  absnote;

  fiter = pnote->m_freq_list.begin();
  while (fiter != pnote->m_freq_list.end())
    {
      absnote = m_f2n.frequ2note((*fiter).f);
      return (get_key(absnote));
      fiter++;
    }
  printf("Warning in piano edition: key selection error.\n");
  return -1;
}

std::list<t_note_sketch>::iterator CScorePlacementPiano::get_next_note_iter_by_key(std::list<t_note_sketch>::iterator noteiter, std::list<t_note_sketch> *plist, int key)
{
  noteiter++;
  while (noteiter != plist->end())
    {      
      if (get_key((*noteiter).pnote) == key)
	return noteiter;
      noteiter++;
    }
  return noteiter;
}

std::list<t_note_sketch>::iterator CScorePlacementPiano::get_first_note_iter_by_key(std::list<t_note_sketch> *plist, int key)
{
  std::list<t_note_sketch>::iterator noteiter;

  noteiter = plist->begin();
  while (noteiter != plist->end() && get_key((*noteiter).pnote) != key)
    {      
      noteiter++;
    }
  return noteiter;
}

// Same thing but on two different keys
void CScorePlacementPiano::place_beams(std::list<t_note_sketch> *pnsk_list, int color, int key)
{
  enum{statenobeam, stateonbeam};
  std::list<t_note_sketch>::iterator noteiter;
  std::list<t_note_sketch>::iterator next_noteiter;
  t_beam                             beam;
  bool                               bnext;
  int                                state;

  state = statenobeam;
  noteiter = get_first_note_iter_by_key(pnsk_list, key);
  while (noteiter != pnsk_list->end())
    {
      switch (state)
	{
	case statenobeam:
	  {
	    if ((*noteiter).pnote->m_lconnected || m_bautobeam)
	      {
		next_noteiter = get_next_note_iter_by_key(noteiter, pnsk_list, key);
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
	    next_noteiter = get_next_note_iter_by_key(noteiter, pnsk_list, key);
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
      noteiter = get_next_note_iter_by_key(noteiter, pnsk_list, key);
    }
}

void CScorePlacementPiano::place_notes(std::list<CMesure*> *pml, CInstrument *pinstrument, std::list<t_note_sketch> *pnote_placement_list)
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
      place_note(pml, pn, &m_rendered_notes_list);
      pn = pinstrument->get_next_note();
    }
  pn = get_tmp_note();
  if (pn != NULL)
    {
      place_note(pml, pn, &m_rendered_notes_list);
    }
  place_beams(pnote_placement_list, BLACK, 0);
  place_beams(pnote_placement_list, BLACK, 1);
  // Shows how a measure is divided
  //fill_unused_times(pscore, pinstrument);
}

