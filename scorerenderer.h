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

class CScoreRenderer
{
 public:
  CScoreRenderer(CGL2Dprimitives *pgfxprimitives, CpictureList *picturelist, TTF_Font *pfont, CMeshList *pmeshlist);

  void render(Cgfxarea *pw, CScorePlacement *pplacement, t_edit_state *pedit_state, std::list<CNote*> *pplayed_note_list);
  void render_sel_box(Cgfxarea *pw, CScorePlacement *pplacement, t_note_selection_box *pnsb, double elapsed_time, bool bpracticeloop);

enum enote_active
  {
    enote_not_selected,
    enote_edited, // string
    enote_played, // green
    enote_tmp     // string
  };

 private:
  void drawMeshes(CScorePlacement *pplacement);
  void drawmovecursor(t_coord mousepos);
  void drawtrashcancursor(t_coord mousepos);
  void drawwheelcursor(t_coord mousepos);
  void drawbeamlink(t_coord mousepos, bool bset);
  void drawSegments(CScorePlacement *pplacement);
  void drawPolygons(CScorePlacement *pplacement);
  void diese(int x, int y);
  void bemol(int x, int y);
  int  note2y(int note, int octave, int yoffset);
//  void drawdiese(int x, int y, int radius, int color);
//  void drawbemol(int x, int y, int radius, int color);
  int  get_in_measure_rank(CScore *pscore, CNote *pn, double &tcode, int *ptype);
  void drawNote(t_note_sketch *pns, CScorePlacement *pplacement, enote_active selectedfor);
  void drawMeasure(t_bar_sketch *pbs, CScorePlacement *pplacement, t_edit_state *peditstate);
  bool note_is_in_list(CNote* pn, std::list<CNote*> *pplayed_note_list);
  int  noteColor(CScorePlacement *pplacement, enote_active selectedfor, int string_num);
  void draw_hand_text(CScorePlacement *pplacement, t_note_draw_info *pnoteinfo, t_fcoord pos);
  //
  void set_viewport(Cgfxarea *pw, t_fcoord orthosize);
  void reset_viewport();
  //void opengl_draw_poly_line(t_fcoord start, t_fcoord stop, float thickness, int color);
  //void opengl_draw_circle(t_fcoord pos, float radius, bool bfilled);
  void draw_note_selbox(t_coord bstart, t_coord bstop, bool bloop, int progress = -1);
  void draw_texts(CScorePlacement *pplacement);

 private:
  CGL2Dprimitives *m_gfxprimitives;
  CpictureList    *m_picturelist;
  int              m_color;
  TTF_Font        *m_font;
  CMeshList       *m_meshList;
  bool             m_prevblrcursor;
};

