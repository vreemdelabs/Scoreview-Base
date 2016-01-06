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


class CScorePlacementGuitar: public CScorePlacement
{
 public:
  CScorePlacementGuitar(int notenum = 48);
  ~CScorePlacementGuitar();

  // Placement
  void place_bars(std::list<CMesure*> *pml);
  void place_notes(std::list<CMesure*> *pml, CInstrument *pinstrument, std::list<t_note_sketch> *pnote_placement_list);

  // Edition
  float  appy2notefrequ(int x);

 private:
  // Virtual functions
  float note2y(int note, int octave);
  void  place_keys();
  void  place_portee();
  void  place_note_segments(int octave, int note, float x);

  float get_string_segment_y(int string);
  void  place_TAB_segments(float xstart, float length);
  void  place_segment_portee(int note, int octave, float xstart, float length);
  void  place_TAB_notes(std::list<CMesure*> *pml, CNote *pn);
};

