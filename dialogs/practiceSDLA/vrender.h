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

#define VBACKGROUND         0xFF404040

#define PEGBOX_COLOR        0xFF011016

class CvRenderer : public CsiRenderer
{
 public:
  CvRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list);
  ~CvRenderer();

 public:
  void render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl, int hnote_id);

 private:

  void draw_finger_board(Cgfxarea *pw, erlvl level);
  void add_drawings(t_coord dim, int y, CNote *pn, t_notefreq *pnf, t_limits *pl, int hnote_id);
  int  note_2_y(t_coord dim, t_notefreq *pnotef);
  int  string_width(int string_number);
  void draw_note_drawings(erlvl level, int hnote_id);
 private:
};
