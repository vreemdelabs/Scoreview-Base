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
#include "scoreedit_piano.h"


CscoreEditPiano::CscoreEditPiano(Ckeypress *pkstates):CscoreEdit(pkstates)
{
}

CscoreEditPiano:: ~CscoreEditPiano()
{
}

std::list<t_note_sketch*>::iterator CscoreEditPiano::get_next_note_iter_by_key(std::list<t_note_sketch*>::iterator noteiter, std::list<t_note_sketch*> *plist, int key, CScorePlacement *pplacement)
{
  noteiter++;
  while (noteiter != plist->end())
    {      
      if (pplacement->get_key((*noteiter)->pnote) == key)
	return noteiter;
      noteiter++;
    }
  return noteiter;
}

std::list<t_note_sketch*>::iterator CscoreEditPiano::get_first_note_iter_by_key(std::list<t_note_sketch*> *plist, int key, CScorePlacement *pplacement)
{
  std::list<t_note_sketch*>::iterator noteiter;

  noteiter = plist->begin();
  while (noteiter != plist->end() && pplacement->get_key((*noteiter)->pnote) != key)
    {      
      noteiter++;
    }
  return noteiter;
}

// Called when a note duration is changed or when a note is moved suposedly on another one
void CscoreEditPiano::fuse_chords_by_key(CScorePlacement *pplacement, CInstrument *pinst, int key)
{
  std::list<t_note_sketch*>           nsklist;
  std::list<t_note_sketch*>::iterator iter;
  std::list<t_note_sketch*>::iterator iter_next;
  t_note_sketch *pns;
  t_note_sketch *pns_next;

  if (!m_bfuse_chords)
    return;
  //printf("fusechords\n");
  pns = pplacement->get_first_notesk();
  while (pns != NULL)
    {
      nsklist.push_back(pns);
      pns = pplacement->get_next_notesk();
    }
  nsklist.sort(CscoreEdit::compare_note_sketches);
  iter = get_first_note_iter_by_key(&nsklist, key, pplacement);
  while (iter != nsklist.end())
    {
      iter_next = get_next_note_iter_by_key(iter, &nsklist, key, pplacement);
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
		      pns->nf.bpointed == pns_next->nf.bpointed &&
		      pplacement->get_key(pns->pnote) == pplacement->get_key(pns_next->pnote))
		    {
		      printf("fusing\n");
		      fuse_into_chord(pns->pnote, pns_next->pnote, pinst);
		      m_editstate.deleted_notes_list.push_front(pns_next->pnote); // Deleted notex
		      m_editstate.changed_notes_list.push_front(pns->pnote); // This note received the frequency of the previous
		      return ; // Because the internal list iterator is a mess here, and only one note moved
		    }
		}
	    }
	}
      iter = get_next_note_iter_by_key(iter, &nsklist, key, pplacement);
    }
}

void CscoreEditPiano::fuse_chords(CScorePlacement *pplacement, CInstrument *pinst)
{
  fuse_chords_by_key(pplacement, pinst, 0);
  fuse_chords_by_key(pplacement, pinst, 1);
}

