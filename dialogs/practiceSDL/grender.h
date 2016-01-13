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

#define GBACKGROUND             0xFF404040
#define GUITAR_STRINGS          6
#define GUITAR_NOTES_PER_STRING 24

#define FRET_COLOR              0xFFDFDFBF

typedef struct s_note_segment
{
  int          xbegin;
  int          xend;
  int          y;
  int          height;
  int          fret;
  t_notefreq   nf;
}              t_note_segment;

// Overlaping notes
typedef struct          s_string_segment
{
  int                   x0;
  int                   length;
  std::list<t_notefreq> string_lst;
}                       t_string_segment;

typedef struct                s_chord_segment
{
  t_note_segment              chord_segment;
  std::list<t_string_segment> string_segment_list;
}                             t_chord_segment;

class CgRenderer : public CFingerRenderer
{
 public:
  CgRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list);
  ~CgRenderer();

 public:
  void render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl);

 private:
  enum erlvl
    {
      lvl_background,
      lvl_fingerboard,
      lvl_top
    };

  t_fcoord resize(t_coord wsize, t_fcoord pos, t_fcoord dim);
  void draw_finger_board(Cgfxarea *pw, erlvl level);
  void draw_strings(Cgfxarea *pw);
  void render_string(Cgfxarea *pw, int string_number, int played_note);
  int  blend(float xpos, int color, bool left);
  int  string_to_color(t_notefreq *pnf);
  void add_drawings(t_coord dim, int y, CNote *pn, t_notefreq *pnf, t_limits *pl);
  int  note_2_y(t_coord dim, t_notefreq *pnotef);
  t_string_equation get_string(t_coord dim, int string_number);
  float string_x(t_coord dim, int string_number, int y);
  int  get_time0_x_limit_from_y(t_coord dim, int y, bool left);
  void check_visible_notes(Cgfxarea *pw, CScore *pscore, t_limits *pl);
  void draw_note_drawings(erlvl level);
  int  string_width(int string_number);
  int  reducecolor(int color);
  //
  bool overlaping(t_note_segment *p1, t_note_segment *p2);
  static bool compare_note_segments(const t_note_segment first, const t_note_segment second);
  static bool compare_xpos(const int first, const int second);
  void count_played_strings(t_string_segment *pstrsiter, int frety);
  void concat_segments();
  void add_coming_notes_rectangles();
  std::list<t_note_segment>::iterator get_first_fret(int fret);
  std::list<t_note_segment>::iterator get_next_on_fret(std::list<t_note_segment>::iterator iter, int frety);
  //
  void Draw_the_measure_bars(CScore *pscore, t_coord pos, t_coord dim, double timecode);
  void print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime);
  void Draw_the_practice_end(t_coord pos, t_coord dim, t_limits *pl);

 protected:
  int  m_baseAbsNote[GUITAR_STRINGS];
 private:
  int  m_playedy[GUITAR_STRINGS];

  t_fcoord m_string_center; // The point where the strings would join
  double   m_s0angle;       // Angle of the string 0

  typedef struct s_note_drawing
  {
    erlvl lvl;
    bool  bcircle;
    int   color;
    int   string;
    float y;
    float x;
    float w;
    float h;
    float rad;
  }              t_note_drawing;

  std::list<t_note_drawing>  m_drawlist;

  std::list<t_note_segment>  m_segments;
  std::list<t_chord_segment> m_chords;

  CMeshList *m_mesh_list;
};

class CgRendererDropD : public CgRenderer
{
 public:
  CgRendererDropD(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list);
  ~CgRendererDropD();
};

