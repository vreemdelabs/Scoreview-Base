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
#include "scoreedit_guitar.h"


CscoreEditGuitar::CscoreEditGuitar(Ckeypress *pkstates):CscoreEdit(pkstates)
{
}

CscoreEditGuitar:: ~CscoreEditGuitar()
{
}

// Same as the others but never goes past an overflow note
bool CscoreEditGuitar::change_chord_frequency_element(CScorePlacement *pplacement, CNote *pnote, float frequency, int element_num, bool btimechange)
{
  std::list<t_notefreq>::iterator iter;
  int  i;
  bool bchange;
  CInstrumentHand* phand;

  phand = pplacement->get_hand();
  assert(phand != NULL);
  // Frequency modification
#ifdef _DEBUG
  printf("frequency modification, chordnum=%d\n", element_num);
#endif
  bchange = false;
  if (phand->get_first_string(frequency) != OUTOFRANGE)
    {
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
	      (*iter).string = phand->get_proper_string((*iter).f, (*iter).string);
	      break;
	    }
	  i++;
	  iter++;
	}
    }
#ifdef _DEBUG
  printf("freqmoddone\n");
#endif
  // Time modification
  if (bchange || btimechange)
    {
      m_editstate.changed_notes_list.push_front(pnote);
      return true;
    }
  return bchange;
}

