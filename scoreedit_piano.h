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

class CscoreEditPiano: public CscoreEdit
{
 public:
  CscoreEditPiano(Ckeypress *pkstates);
  ~CscoreEditPiano();

 private:
  void fuse_chords(CScorePlacement *pplacement, CInstrument *pinst);
  //
  std::list<t_note_sketch*>::iterator get_first_note_iter_by_key(std::list<t_note_sketch*> *plist, int key, CScorePlacement *pplacement);
  std::list<t_note_sketch*>::iterator get_next_note_iter_by_key(std::list<t_note_sketch*>::iterator noteiter, std::list<t_note_sketch*> *plist, int key, CScorePlacement *pplacement);
  void fuse_chords_by_key(CScorePlacement *pplacement, CInstrument *pinst, int key);

};
