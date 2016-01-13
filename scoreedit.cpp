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
#include <assert.h>

#include <SDL2/SDL.h>

#include "gfxareas.h"
#include "mesh.h"
#include "score.h"
#include "f2n.h"
#include "keypress.h"
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreedit.h"

CscoreEdit::CscoreEdit(Ckeypress *pkstates):
  m_pks(pkstates),
  m_state(waiting)
{
  reset_edit_state();
}

CscoreEdit::~CscoreEdit()
{

}

void CscoreEdit::reset_edit_state()
{
  m_editstate.pmodified_note = NULL;
  m_editstate.pmodified_measure = NULL;
  m_editstate.bedit_meter = false;
  m_editstate.chordnotenum = -1;
  m_editstate.blrcursor = false;
  m_editstate.bbeamlinkcursor = false;
  m_editstate.bbrokenbeamlinkcursor = false;
  m_editstate.btrashcan_cursor = false;
  m_editstate.bwheel_cursor = false;
  m_editstate.drawselbox = noselbox;
}

t_edit_state* CscoreEdit::get_edit_state()
{
  return &m_editstate;
}

void CscoreEdit::mouse_left_down(CScore *pscore, CInstrument *pcur_instru, CScorePlacement *pplacement, t_coord mouse_coordinates)
{
  //t_measure_number *pmeasurenumber;
  t_note_sketch    *pnotesk;
  t_bar_sketch     *pbarsk;
  int              freqnum;
  int              note_element;

#ifdef _DEBUG
  printf("edit ldown\n");
#endif
  switch (m_state)
    {
    case waiting:
      {
	m_editstate.mouse_start = mouse_coordinates;
	m_editstate.btrashcan_cursor = m_pks->is_pressed('d');
	note_element = pplacement->is_on_note_element(mouse_coordinates, &freqnum, &pnotesk);
	if (pnotesk == NULL)
	  {
	    {
	      m_editstate.drawselbox = loopselbox;
	      if (m_pks->is_pressed('s'))
		m_state = playboxnonotes;
	      else
		if (m_pks->is_pressed('d'))
		  {
		    m_state = deletenotes;
		    m_editstate.drawselbox = deleteselbox;
		  }
		else
		  {
		    if (m_pks->is_pressed('l'))
		      {
			m_editstate.drawselbox = loopselbox;
			m_state = practiceloopbox;
		      }
		    else
		      m_state = playbox;
		  }
	    }
	    break;
	  }
	else
	  m_editstate.drawselbox = noselbox;
      }
    case onnote:
    case onbar:
      {
	m_editstate.mouse_start = mouse_coordinates;
	note_element = pplacement->is_on_note_element(mouse_coordinates, &freqnum, &pnotesk);
	if (note_element != error_no_element)
	  {
	    if (m_pks->is_pressed('d'))
	      {
		m_state = deletenote;
	      }
	    else
	      {
		switch (note_element)
		  {
		  case beam:
#ifdef _DEBUG
		    printf("placed on beam\n");
#endif
		    m_state = togglebeam;
		    break;
		  case tail:
#ifdef _DEBUG
		    printf("placed on tail\n");
#endif
		    m_editstate.changed_notes_list.push_front(pnotesk->pnote);
		    m_state = notelen;
		    break;
		  case chord:
		    {
#ifdef _DEBUG
		      printf("placed on chord chordnotenum=%d\n", freqnum);
#endif
		      // If higher frequency is selected, then split it from the chord, create a new note
		      // with the selected frequency.
		      if (freqnum > 0)
			{
#ifdef _DEBUG
			  printf("----------------Split chord-----------------------------------\n");
#endif
			  m_editstate.changed_notes_list.push_front(pnotesk->pnote); // Changed, save a copy before modification
			  CNote *pnew_note = split_chord(pnotesk->pnote, freqnum);
			  assert (pcur_instru != NULL);
			  pcur_instru->add_note(pnew_note);
			  m_editstate.changed_notes_list.push_front(pnew_note);
			  freqnum = 0;
			  m_state = movenote;
			}
		      else
			{
			  m_editstate.changed_notes_list.push_front(pnotesk->pnote);
			  m_state = movechord;
			}
		      m_editstate.chordnotenum = freqnum;
		    }
		    break;
		  case note:
#ifdef _DEBUG
		    printf("placed on note\n");
#endif
		    m_editstate.chordnotenum = freqnum;
		    m_state = movenote;
		    m_editstate.changed_notes_list.push_front(pnotesk->pnote);
		break;
		  default:
		    printf("note zone click error\n");
		    exit(EXIT_FAILURE);
		  };
	      }
	    assert(pnotesk != NULL);
	    printf("set modified note, onnote\n");
	    m_editstate.pmodified_note = pnotesk->pnote;
	    m_editstate.modified_note_start_cpy = *pnotesk->pnote;
	  }
	else
	  {
	    pbarsk = pplacement->is_on_measure(mouse_coordinates);
	    if (pbarsk != NULL)
	      {
		if (m_pks->is_pressed('d'))
		  {
		    m_state = deletebar;
		  }
		else
		  {
		    if (pbarsk->timecode == pbarsk->pmesure->m_start)
		      {
			// First bar = move
			m_state = movebar;
		      }
		    else
		      {
			// Change length
			m_state = barlen;
		      }
		  }
		m_editstate.pmodified_measure = pbarsk->pmesure;
		m_editstate.modified_bar_start_cpy = *pbarsk->pmesure;
	      }
	    else
	      {
		if (m_pks->is_pressed('s'))
		  m_state = playboxnonotes;
		else
		  {
		    if (m_pks->is_pressed('l'))
		      {
			m_editstate.drawselbox = loopselbox;
			m_state = practiceloopbox;
			break;
		      }
		    else
		      m_state = playbox;
		  }
	      }
	  }
      }
      break;
    case bartimes:
      break;      
    default:
      m_state = waiting;
    }
#ifdef _DEBUG
  printf("edit ldownend\n");
#endif
}

bool CscoreEdit::set_played_notes_list(CInstrument *pinst, CScorePlacement *pplacement, t_note_selbox *pnsb)
{
  std::list<t_notefreq>::iterator f_iter;
  CNote         *pn;
  t_note_sketch *pns;
  int            y;
  int            x;
  float          hifreq;
  float          lofreq;
  double         start, stop;
  double         newstart, newstop;

  y = m_editstate.mouse_start.y < m_editstate.mouse_stop.y? m_editstate.mouse_start.y : m_editstate.mouse_stop.y;
  hifreq = pplacement->appy2notefrequ(y);
  y = m_editstate.mouse_start.y > m_editstate.mouse_stop.y? m_editstate.mouse_start.y : m_editstate.mouse_stop.y;
  lofreq = pplacement->appy2notefrequ(y);
  x = m_editstate.mouse_start.x < m_editstate.mouse_stop.x? m_editstate.mouse_start.x : m_editstate.mouse_stop.x;
  newstart = start = pplacement->get_time_from_appx(x);
  x = m_editstate.mouse_start.x > m_editstate.mouse_stop.x? m_editstate.mouse_start.x : m_editstate.mouse_stop.x;
  newstop = stop = pplacement->get_time_from_appx(x);
  //
  pns = pplacement->get_first_notesk();
  while (pns != NULL)
    {
      //printf("comparing note=%f to start%f stop=%f\n", pn->m_time, start, stop);
      if (pns->nf.inmtimecode >= start && pns->nf.inmtimecode <= stop) // Use the in measure timecode
	{
	  pn = pns->pnote;
	  newstart = start > pns->pnote->m_time? pns->pnote->m_time : start;
	  newstop = stop < pns->pnote->m_time + pns->pnote->m_duration? pns->pnote->m_time + pns->pnote->m_duration : stop;
	  f_iter = pn->m_freq_list.begin();
	  while (f_iter != pn->m_freq_list.end())
	    {
	      //printf("comparinf note f=%f to lo=%f hi=%f\n", *f_iter, lofreq, hifreq);
	      if ((*f_iter).f >= lofreq && (*f_iter).f <= hifreq)
		{
		  //printf("edit adding note\n");
		  pnsb->Note_list.push_back(pn);
		  f_iter = pn->m_freq_list.end();
		}
	      else
		f_iter++;
	    }
	}
      pns = pplacement->get_next_notesk();
    }
  //printf("hifreq=%f lofreq=%f\n", hifreq, lofreq);
  pnsb->nsb.start_tcode = newstart;
  pnsb->nsb.stop_tcode  = newstop;
  pnsb->nsb.hif = hifreq;
  pnsb->nsb.lof = lofreq;
  pnsb->bupdate = true;
  return (pnsb->Note_list.size() > 0);
}

void CscoreEdit::delete_selected_notes(CInstrument *pinst, CScorePlacement *pplacement)
{
  std::list<t_notefreq>::iterator  f_iter;
  CNote         *pn;
  t_note_sketch *pns;
  int            y;
  int            x;
  float          hifreq;
  float          lofreq;
  double         start, stop;
  bool           bdelete_note;

  y = m_editstate.mouse_start.y < m_editstate.mouse_stop.y? m_editstate.mouse_start.y : m_editstate.mouse_stop.y;
  hifreq = pplacement->appy2notefrequ(y);
  y = m_editstate.mouse_start.y > m_editstate.mouse_stop.y? m_editstate.mouse_start.y : m_editstate.mouse_stop.y;
  lofreq = pplacement->appy2notefrequ(y);
#ifdef _DEBUG
  printf("hifreq=%f lofreq=%f\n", hifreq, lofreq);
#endif
  x = m_editstate.mouse_start.x < m_editstate.mouse_stop.x? m_editstate.mouse_start.x : m_editstate.mouse_stop.x;
  start = pplacement->get_time_from_appx(x);
  x = m_editstate.mouse_start.x > m_editstate.mouse_stop.x? m_editstate.mouse_start.x : m_editstate.mouse_stop.x;
  stop = pplacement->get_time_from_appx(x);
  //
  pns = pplacement->get_first_notesk();
  while (pns != NULL)
    {
      bdelete_note = false;
      pn = pns->pnote;
      if (pns->nf.inmtimecode >= start && pns->nf.inmtimecode <= stop) // Use the in measure timecode
	{
	  f_iter = pn->m_freq_list.begin();
	  while (f_iter != pn->m_freq_list.end())
	    {
	      if ((*f_iter).f >= lofreq && (*f_iter).f <= hifreq)
		{
		  bdelete_note = true;
		  f_iter = pn->m_freq_list.end();
		}
	      else
		f_iter++;
	    }
	}
      if (bdelete_note == true)
	{
	  m_editstate.deleted_notes_list.push_front(pn); // To practice
	  //pinst->delete_note(pn);
	}
      pns = pplacement->get_next_notesk();
    }
}

// Move view or select a loop area
void CscoreEdit::mouse_right_down(t_coord mouse_coordinates)
{
#ifdef _DEBUG
  printf("edit rdown\n");
#endif
  switch (m_state)
    {
    case waiting:
      {
	m_editstate.mouse_start = mouse_coordinates;
	// Cursor hidden in the main app, the two "share" the "move" state. Malpractice, but it's alive. FIXME
	m_state = move_visual;
      }
      break;
    default:
      break;
    };
}

bool CscoreEdit::mouse_right_up(t_coord mouse_coordinates)
{
  bool ret;

  ret = false;
  switch (m_state)
    {
    case move_visual:
      {
	// Up or down only here, time is changed in the main app
	m_state = waiting;
      }
      break;
    default:
      break;
    };
  return ret;
}

bool CscoreEdit::mouse_move(CScore *pscore, CScorePlacement *pplacement, t_coord mouse_coordinates)
{
  std::list<float>::iterator iter;
  float interval;
  t_note_sketch    *pnotesk;
  t_measure_number *pmeasurenumber;
  t_bar_sketch     *pbarsk;
  int   freqnum;
  int   note_element;
  int   y;
  bool  bchange;

  //printf("edit move\n");
  bchange = false;
  m_editstate.mouse_stop = mouse_coordinates;
  switch (m_state)
    {
    case waiting:
      m_editstate.btrashcan_cursor = m_pks->is_pressed('d');
      note_element = pplacement->is_on_note_element(mouse_coordinates, &freqnum, &pnotesk);
      if (pnotesk != NULL)
	{
	  m_state = onnote;
	  printf("set modified note wait\n");
	  m_editstate.pmodified_note = pnotesk->pnote;
	  if (!m_pks->is_pressed('d'))
	    {
	      switch (note_element)
		{
		case tail:
		  m_editstate.blrcursor = true;
		  break;
		case beam:
		  m_editstate.bbeamlinkcursor = true;
		case chord:
		case note:
		  break;
		default:
		  break;
		};
	    }
	}
      else
	{
	  pmeasurenumber = pplacement->is_on_bar_number(mouse_coordinates);
	  if (pmeasurenumber != NULL)
	    {
	      m_editstate.bedit_meter = true;
	      m_editstate.bwheel_cursor = true;
	      m_editstate.pmodified_measure = pmeasurenumber->pmesure;
	      m_state = bartimes;
	    }
	  else
	    {
	      pbarsk = pplacement->is_on_measure(mouse_coordinates);
	      if (pbarsk != NULL)
		{
		  if (m_pks->is_pressed('d'))
		    m_editstate.btrashcan_cursor = true;
		  else
		    {
		      m_editstate.blrcursor = true;
		    }
		  m_state = onbar;
		  m_editstate.pmodified_measure = pbarsk->pmesure;
		}
	    }
	}
      break;
    case onnote:
      m_editstate.btrashcan_cursor = m_pks->is_pressed('d');
      note_element = pplacement->is_on_note_element(mouse_coordinates, &freqnum, &pnotesk);
      if (pnotesk == NULL)
	{
	  m_state = waiting;
	  m_editstate.pmodified_note = NULL;
	  m_editstate.blrcursor = false;
	  m_editstate.bbeamlinkcursor = false;
	  m_editstate.bbrokenbeamlinkcursor = false;
	}
      else
	{
	  printf("set modified note\n");
	  m_editstate.pmodified_note = pnotesk->pnote;
	  if (!m_pks->is_pressed('d'))
	    {
	      switch (note_element)
		{
		case tail:
		  m_editstate.blrcursor = true;
		  m_editstate.bbeamlinkcursor = false;
		  m_editstate.bbrokenbeamlinkcursor = false;
		  break;
		case beam:
		  {
		    m_editstate.blrcursor = false;
		    if (pnotesk->pnote->m_lconnected)
		      {
			m_editstate.bbeamlinkcursor = false;
			m_editstate.bbrokenbeamlinkcursor = true;
		      }
		    else
		      {
			m_editstate.bbeamlinkcursor = true;
			m_editstate.bbrokenbeamlinkcursor = false;
		      }
		  }
		  break;
		default:
		  m_editstate.blrcursor = false;
		  break;
		};
	    }
	  else
	    m_editstate.btrashcan_cursor = true;
	}
      break;
    case onbar:
      {
	m_editstate.btrashcan_cursor = m_pks->is_pressed('d');
	pbarsk = pplacement->is_on_measure(mouse_coordinates);
	if (pbarsk == NULL)
	  {
	    m_state = waiting;
	    m_editstate.pmodified_measure = NULL;
	    m_editstate.blrcursor = false;
	  }
	else
	  {
	    if (!m_pks->is_pressed('d'))
	      {
		m_editstate.blrcursor = true;
	      }
	  }
      }
      break;
    case movenote:
    case movechord:
      assert(m_editstate.pmodified_note != NULL);
      bchange = move_note_or_chord(pplacement);
      break;
    case notelen:
      assert(m_editstate.pmodified_note != NULL);
      interval = pplacement->get_time_interval_from_appx(m_editstate.mouse_start.x, m_editstate.mouse_stop.x);
      //printf("interval=%f\n", interval);
      m_editstate.pmodified_note->m_duration = m_editstate.modified_note_start_cpy.m_duration;
      m_editstate.pmodified_note->m_duration += interval;
      if (m_editstate.pmodified_note->m_duration < 0.025)
	m_editstate.pmodified_note->m_duration = 0.025;
      //printf("duration=%f\n", m_editstate.pmodified_note->m_duration);
      break;
    case movebar:
      assert(m_editstate.pmodified_measure != NULL);
      interval = pplacement->get_time_interval_from_appx(m_editstate.mouse_start.x, m_editstate.mouse_stop.x);
      m_editstate.pmodified_measure->m_start = m_editstate.modified_bar_start_cpy.m_start;
      m_editstate.pmodified_measure->m_start += interval;
      m_editstate.pmodified_measure->m_stop = m_editstate.modified_bar_start_cpy.m_stop;
      m_editstate.pmodified_measure->m_stop += interval;
      pscore->sort_measures();
      break;
    case barlen:
      assert(m_editstate.pmodified_measure != NULL);
      interval = pplacement->get_time_interval_from_appx(m_editstate.mouse_start.x, m_editstate.mouse_stop.x);
      m_editstate.pmodified_measure->m_stop = m_editstate.modified_bar_start_cpy.m_stop;
      m_editstate.pmodified_measure->m_stop += interval / 8;
      break;
    case bartimes:
      pmeasurenumber = pplacement->is_on_bar_number(mouse_coordinates);
      if (pmeasurenumber == NULL)
	{
	  m_editstate.bwheel_cursor = false;
	  m_editstate.pmodified_measure = NULL;
	  m_editstate.bedit_meter = false;
	  m_state = waiting;
	}
      break;
    case move_visual:
      {
	y = m_editstate.mouse_start.y - mouse_coordinates.y;
	m_editstate.mouse_start = mouse_coordinates;
	pplacement->change_voffset(y);
      }
      break;
    // Nothing to do
    case togglebeam:
    case deletenote:
    case deletenotes:
    case deletebar:
    case playbox:
    case playboxnonotes:
    case practiceloopbox:
    case copymeasure:
    case pastemeasure:
    default:
      break;
    };
  //printf("endit move end, state==%d\n", m_state);
  return bchange;
}

bool CscoreEdit::mouse_wheel(CScorePlacement *pplacement, t_coord mouse_coordinates, int inc)
{
  CMesure *pm;
  t_measure_number *pmeasurenumber;
  int           i, string_num, prevstring;
  int           freq_number;
  t_note_sketch *pns;
  CNote         *pnote;
  std::list<t_notefreq>::iterator iter;

  pmeasurenumber = pplacement->is_on_bar_number(mouse_coordinates);
  if (pmeasurenumber != NULL &&
      m_state == bartimes)
    {
      pm = pmeasurenumber->pmesure;
      pm->m_times += inc;
      if (pm->m_times < 1)
	pm->m_times = 1;
      if (pm->m_times > 16)
	pm->m_times = 16;
      return true;
    }
  if (m_pks->is_pressed('s'))
    {
      if (pplacement->is_on_note_element(mouse_coordinates, &freq_number, &pns))
	{
	  pnote = pns->pnote;
	  i = 0;
	  iter = pnote->m_freq_list.begin();
	  while (iter != pnote->m_freq_list.end())
	    {
	      if (i == freq_number)
		{
		  string_num = prevstring = (*iter).string;
#ifdef _DEBUG
		  printf("wheel on string old=%d", string_num);
#endif
		  if (inc > 0)
		    string_num = pplacement->get_hand()->get_next_string((*iter).f, string_num);
		  else
		    string_num = pplacement->get_hand()->get_prev_string((*iter).f, string_num);
#ifdef _DEBUG
		  printf(" new=%d inc=%d\n", string_num, inc);
#endif
		  //m_editstate.hand_text = pplacement->get_hand()->get_string_text((*iter).string);
		  if (string_num != prevstring)
		    {
		      (*iter).string = string_num;
		      m_editstate.changed_notes_list.push_front(pnote);
		    }
		  return true;
		}
	      iter++;
	    }
	}
      // Disables time zoom
      return true;
    }
  else
  //if (m_pks->is_pressed('z'))
    {
      pplacement->change_vertical_zoom(inc, mouse_coordinates);
      return true;
    }
  return false;
}

bool CscoreEdit::mouse_left_up(CScore *pscore, CInstrument *pcur_instru, CScorePlacement *pplacement, t_coord mouse_coordinates)
{
  CInstrument *pinst;
  bool         bchange;

#ifdef _DEBUG
  printf("edit lup\n");
#endif
  bchange = false;
  pinst = pcur_instru;
  assert(pinst != NULL);
  switch (m_state)
    {
    case movenote:
    case movechord:
      {
	mouse_move(pscore, pplacement, m_editstate.mouse_stop);
	remove_duplicate_frequencies(m_editstate.pmodified_note);
	// Sort the note list after modifications
	pinst->sort();
	fuse_chords(pplacement, pinst);
	//
	//m_editstate.changed_notes_list.push_front(pnotem_editstate.pmodified_note); Bug because of duplicates: change in freq & time + change into a chord
	bchange = true;
      }
      break;
    case notelen:
      mouse_move(pscore, pplacement, mouse_coordinates);
      m_editstate.blrcursor = false;
      bchange = true;
      break;
    case movebar:
    case barlen:
      mouse_move(pscore, pplacement, m_editstate.mouse_stop);
      // Sort the note list after modifications
      pinst->sort();
      break;
      //-------------------------------
    case bartimes:
      break;
    case togglebeam:
      assert(m_editstate.pmodified_note != NULL);
      m_editstate.pmodified_note->m_lconnected = !m_editstate.pmodified_note->m_lconnected;
      break;
    case deletenote:
      assert(m_editstate.pmodified_note != NULL);
      if (!m_pks->is_pressed('d'))
	break;
      if (pinst)
	{
	  m_editstate.deleted_notes_list.push_front(m_editstate.pmodified_note);
	}
      bchange = true;
      break;
    case deletenotes:
      m_editstate.btrashcan_cursor = false;
      if (!m_pks->is_pressed('d'))
	break;
      if (pinst != NULL)
	delete_selected_notes(pinst, pplacement);
      bchange = true;
      break;
    case deletebar:
      assert(m_editstate.pmodified_measure != NULL);
      m_editstate.btrashcan_cursor = false;
      if (!m_pks->is_pressed('d'))
	break;
      pscore->delete_measure(m_editstate.pmodified_measure);
      bchange = true;
      break;
      //-------------------------------
    case playbox:
      {
	if (pinst != NULL)
	  set_played_notes_list(pinst, pplacement, &m_editstate.note_sel);
      }
      break;
    case practiceloopbox:
      {
	m_editstate.mouse_stop = mouse_coordinates;
	if (m_pks->is_pressed('l'))
	  {
	    if (pcur_instru != NULL)
	      {
		set_played_notes_list(pcur_instru, pplacement, &m_editstate.note_sel);
	      }
	  }
	m_editstate.drawselbox = noselbox; // Like unselect
      }
      break;
    case playboxnonotes:
      // FIXME substract the notes from the sound -> must implement exact phase play before

      break;
      //-------------------------------
    case copymeasure:
      
      break;
    case pastemeasure:
      
      break;
      //-------------------------------
    default:
      break;
    };
  m_state = waiting;
  reset_edit_state();
  return bchange;
}

