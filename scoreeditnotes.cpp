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


void CscoreEdit::remove_duplicate_frequencies(CNote *pnote)
{
#ifdef _DEBUG
  printf("removeduplicates\n");
#endif
  pnote->m_freq_list.sort();
  pnote->m_freq_list.unique();
  assert(pnote->m_freq_list.size() > 0);
}

void CscoreEdit::fuse_into_chord(CNote *pref_note, CNote *pdel_note, CInstrument *pinst)
{
  std::list<t_notefreq>::iterator iter;

#ifdef _DEBUG
  printf("chordfuse\n");
#endif
  iter = pdel_note->m_freq_list.begin();
  while (iter != pdel_note->m_freq_list.end())
    {
      if (!pref_note->is_frequ_in((*iter).f))
	{
	  pref_note->m_freq_list.push_back(*iter);
	}
      iter++;
    }
  remove_duplicate_frequencies(pref_note);
  // Remove the note from the score, but it will be deleted later, when sending the update to the practice view
  pinst->m_Note_list.remove(pdel_note);
  //  delete pdel_note;
#ifdef _DEBUG
  printf("chordfuse done\n");
  assert(pref_note->m_freq_list.size() > 0);
  assert(pdel_note->m_freq_list.size() > 0);
  int i = 0;
  iter = pref_note->m_freq_list.begin();
  while (iter != pref_note->m_freq_list.end())
    {
      printf("element %d is =%f\n", i, (*iter).f);
      i++;
      iter++;
    }
#endif
}

bool CscoreEdit::change_chord_frequency_element(CScorePlacement *pplacement, CNote *pnote, float frequency, int element_num, bool btimechange)
{
  std::list<t_notefreq>::iterator iter;
  int  i;
  bool bchange;

  // Frequency modification
#ifdef _DEBUG
  printf("frequency modification, chordnum=%d\n", element_num);
#endif
  bchange = false;
  i = 0;
  iter = pnote->m_freq_list.begin();
  while (iter != pnote->m_freq_list.end())
    {
      if (i == element_num)
	{
	  if ((*iter).f != frequency) // Practice update
	    {
	      bchange = true;
	    }
	  (*iter).f = frequency;
	  (*iter).string = pplacement->get_hand()->get_proper_string((*iter).f, (*iter).string);
	  break;
	}
      i++;
      iter++;
    }
#ifdef _DEBUG
  printf("freqmoddone\n");
#endif
  // Time modification
  if (bchange || btimechange)
    {
      //printf(" changing %x\n", pnote);
      m_editstate.changed_notes_list.push_front(pnote);
      return true;
    }
  return bchange;
}

bool CscoreEdit::move_note_or_chord(CScorePlacement *pplacement)
{
  t_note_sketch *pmodified_note_sketch;
  t_note_sketch *pns;
  float          new_freq;
  double         move_time, tchange;
  bool           boverlaping;
  bool           btimechange;

#ifdef _DEBUG
  printf("movenote\n");
#endif
  // Time modification
  move_time = m_editstate.modified_note_start_cpy.m_time;
  tchange = pplacement->get_time_interval_from_appx(m_editstate.mouse_start.x, m_editstate.mouse_stop.x);
  btimechange = (tchange != 0);
  move_time += tchange;
#ifdef _DEBUG
  printf("newtime == %f\n", move_time);
#endif
  // Frequency modification
  new_freq = pplacement->appy2notefrequ(m_editstate.mouse_stop.y);
  // Find the modified note coordinates in the current placement
  pmodified_note_sketch = NULL;
  pns = pplacement->get_first_notesk();
  while (pns != NULL)
    {
      if (pns->pnote == m_editstate.pmodified_note)
	{
	  pmodified_note_sketch = pns;
	  break ;
	}
      pns = pplacement->get_next_notesk();
    }
  // Rely on the measure data if found
  boverlaping = false;
  // If it belongs to a measure, then look of other notes on the same spot
  if (pmodified_note_sketch != NULL)
    {
      if (pmodified_note_sketch->nf.pmesure != NULL)
	{
	  pns = pplacement->get_first_notesk();
	  while (pns != NULL)
	    {
	      // Overlaping?	      
	      if (pns->pnote != m_editstate.pmodified_note &&
		  pns->nf.pmesure == pmodified_note_sketch->nf.pmesure &&
		  pns->nf.absolute_index == pmodified_note_sketch->nf.absolute_index &&
		  pns->pnote->is_frequ_in(new_freq) &&
		  pns->nf.note_type != pmodified_note_sketch->nf.note_type &&
		  pns->nf.bpointed != pmodified_note_sketch->nf.bpointed)
		{
		  boverlaping = true;
		}
#ifdef _DEBUG
	      printf("nextnote\n");
#endif
	      pns = pplacement->get_next_notesk();
	    }
	}
    }
  if (!boverlaping)
    m_editstate.pmodified_note->m_time = move_time;
#ifdef _DEBUG
  printf("change\n");
#endif
  return (change_chord_frequency_element(pplacement, m_editstate.pmodified_note, new_freq, m_editstate.chordnotenum, btimechange));
}

bool CscoreEdit::compare_note_sketches(const t_note_sketch *pfirst, const t_note_sketch *psecond)
{
  return (pfirst->pnote->m_time < psecond->pnote->m_time);
}

// Called when a note duration is changed or when a note is moved on another one
void CscoreEdit::fuse_chords(CScorePlacement *pplacement, CInstrument *pinst)
{
  std::list<t_note_sketch*>           nsklist;
  std::list<t_note_sketch*>::iterator iter;
  std::list<t_note_sketch*>::iterator iter_next;
  t_note_sketch *pns;
  t_note_sketch *pns_next;

  if (!m_bfuse_chords)
    return;
#ifdef _DEBUG
  printf("fuschords\n");
#endif
  pns = pplacement->get_first_notesk();
  while (pns != NULL)
    {
      nsklist.push_back(pns);
      pns = pplacement->get_next_notesk();
    }
  nsklist.sort(CscoreEdit::compare_note_sketches);
  iter = nsklist.begin();
  while (iter != nsklist.end())
    {
      iter_next = iter;
      iter_next++;
      if (iter_next != nsklist.end())
	{
	  pns = *iter;
	  pns_next = *iter_next;
	  if (pns_next != NULL)
	    {
	      if (pns->nf.pmesure != NULL && pns_next->nf.pmesure != NULL)
		{
		  // Overlaping?
		  if (pns->pnote != pns_next->pnote &&
		      pns->nf.pmesure == pns_next->nf.pmesure &&
		      pns->nf.absolute_index == pns_next->nf.absolute_index &&
		      pns->nf.note_type == pns_next->nf.note_type &&
		      pns->nf.bpointed == pns_next->nf.bpointed)
		    {
#ifdef _DEBUG
		      printf("fusing\n");
#endif
		      fuse_into_chord(pns->pnote, pns_next->pnote, pinst);
		      //printf("deleting a note with %d frequencies\n", (int)pns_next->pnote->m_freq_list.size());
		      //printf (" pointer del = %x\n", pns_next->pnote);
		      m_editstate.deleted_notes_list.push_front(pns_next->pnote); // Deleted note
		      // If the deleted note is also in the modified notes becaise it also moved, then
		      // remove it form the modified notes
		      m_editstate.changed_notes_list.remove(pns_next->pnote);
		      //printf("changing a note with %d frequencies\n", (int)pns->pnote->m_freq_list.size());
		      //printf (" pointer add = %x\n", pns->pnote);
		      m_editstate.changed_notes_list.push_front(pns->pnote); // This note received the frequency of the previous
		      return ; // Because the internal list iterator is a mess here, and only one note moved
		    }
		}
	    }
	}
      iter++;
    }
}

CNote* CscoreEdit::split_chord(CNote *pnote, int freqnum)
{
  CNote *pnew_note;
  std::list<t_notefreq>::iterator iter;
  t_notefreq nf;
  int   i;

  pnew_note = new CNote(pnote->m_time, pnote->m_duration, -1, -1);
  // Copy all frequencies but freqnum freq to the new note
  pnew_note->m_freq_list.clear();
  iter = pnote->m_freq_list.begin();
  i = 0;
  while (iter != pnote->m_freq_list.end())
    {
      if (i != freqnum)
	{
	  pnew_note->m_freq_list.push_front(*iter);
	}
      else
	{
	  nf = (*iter);
	}
      iter++;
      i++;
    };
  pnote->m_freq_list.clear();
  pnote->m_freq_list.push_front(nf);
  assert(pnew_note->m_freq_list.size() > 0);
  assert(pnew_note->identifier() != pnote->identifier());
  return pnew_note;
}

void CscoreEdit::set_chord_fuse(bool bactive)
{
  m_bfuse_chords = bactive;
}

