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

typedef struct s_circle
{
  t_fcoord     center;
  float        radius;
}              t_circle;

typedef struct s_note_draw_info
{
  t_circle     circle;
  bool         isdiese;
  bool         isbemol;
  int          string_num;
  // Time coordinates, used in edition
  float        xstart;
  float        len;
}              t_note_draw_info;

// Important structure used to link notes and measures
typedef struct s_measure_note_info
{
  int          note_type;     // enotelen
  bool         bpointed;      // Adds half note
  int          measure_index;
  int          note_index;
  //
  CMesure     *pmesure;  
  //
  double       inmtimecode;  // note timecode given the position in measure  
  int          mnstart;      // in time / 16
  int          mnduration;   // same
  //
  int          absolute_index; // Index in the emasures from the firs bar
}              t_measure_note_info;

typedef struct       s_note_sketch
{
  t_fcoord           pos;
  t_fcoord           dim;
  t_fcoord           tailboxdim;
  t_fcoord           beamboxdim;
  std::list<t_note_draw_info> noteinfolist;
  CNote              *pnote;
  // For overlaping and edition
  t_measure_note_info nf;
}                     t_note_sketch;

typedef struct s_bar_sketch
{
  t_fcoord boxpos;
  t_fcoord boxdim;
  double   timecode;
  bool     doublebar;
  // Data pointer
  CMesure *pmesure;
}              t_bar_sketch;

typedef struct s_string_placement
{
  t_fcoord     pos;
  t_fcoord     dim;
  int          color;
  std::string  txt;
  t_notefreq  *pnf; // NULL if not used for a string
}              t_string_placement;

enum ekey_type
  {
    clefsol,
    cleffa
  };

enum enoteitempos
  {
    error_no_element,
    beam,
    tail,
    note,
    chord
  };

typedef struct s_lsegment
{
  t_fcoord     pos;
  t_fcoord     dim;
  float        thickness;
}              t_lsegment;

typedef struct s_beam
{
  std::list<t_note_sketch*> beam_notes_list;
}              t_beam;

#define POLYG_ARRSIZE 32

typedef struct s_polygon
{
  t_fcoord     point[POLYG_ARRSIZE];
  int          nbpoints;
  int          color;
}              t_polygon;

typedef struct s_mesh
{
  std::string  name;
  t_fcoord     pos;
  t_fcoord     dim;
  int          color;
}              t_mesh;

typedef struct s_measure_number
{
  CMesure     *pmesure;
  t_fcoord     pos;
  t_fcoord     dim;
}              t_measure_number;

#define MAXMTIMESPACES (16 * 16)

typedef struct s_measure_occupancy
{
  CMesure     *pm;
  int          index;
  bool         spaces[MAXMTIMESPACES];
}              t_measure_occupancy;

enum   epracticestate
  {
    epracticeclosed,
    enotpracticing,
    epracticing,
    epracticingloop
  };

#define KEY_OFFSET   (14. *  m_zradius)
#define TEMPO_OFFSET 0.014
#define POSITION_THRESHOLD (8.)

class CScorePlacement
{
 public:
  CScorePlacement(int notenum = 65);
  virtual ~CScorePlacement();

  void clear();
  void set_metrics(Cgfxarea *pw, double starttime, double viewtime, epracticestate practicing_state);
 // void change_vertical_zoom(int inc);
  void change_vertical_zoom(int inc, t_coord mousepos);
  void change_voffset(int inc);

  // Placement
  void place_notes_and_bars(CScore *pscore, CInstrument *pinstrument);

  // Edition
  t_fcoord get_ortho_size();
  double   get_time_from_appx(int x);
  int      get_appx_from_time(double tcode);
  double   get_time_interval_from_appx(int xstart, int xstop);
  int      frequency2appy(float freq);
  int      note2appy(int note, int octave);
  virtual float appy2notefrequ(int x);

  // Used in scorerenderer
  t_note_sketch *get_first_notesk();
  t_note_sketch *get_next_notesk();
  t_measure_number *get_first_barnumber();
  t_measure_number *get_next_barnumber();
  t_bar_sketch  *get_first_barsk();
  t_bar_sketch  *get_next_barsk();
  t_lsegment    *get_first_lsegment();
  t_lsegment    *get_next_lsegment();
  t_polygon  *get_first_polygon();
  t_polygon  *get_next_polygon();
  t_mesh    *get_first_mesh();
  t_mesh    *get_next_mesh();
  t_string_placement *get_first_txt();
  t_string_placement *get_next_txt();

  // Edition
  bool is_in(CNote *pn);
  bool is_in(CMesure *pm);

  t_measure_number *is_on_bar_number(t_coord mouse_pos);
  t_bar_sketch *is_on_measure(t_coord mouse_pos);
  enoteitempos  is_on_note_element(t_coord mouse_pos, int *pfreq_number, t_note_sketch** pns_out);

  void   set_tmp_note(CNote *pnote);
  void   change_tmp_note(double timecode, double duration, float f);
  CNote* get_tmp_note();

  t_fcoord app_area_size2placement_size(t_fcoord coord);
  t_fcoord placement_size_2app_area_size(t_fcoord coord);
  float    placement_y_2app_area_y(float v);

  void  set_autobeam(bool bactive);

  CInstrumentHand* get_hand();

  virtual int get_key(int absnote);
  virtual int get_key(CNote *pnote);

  //CMesh* get_mesh(std::string name);

 protected:
  float appy_2_placementy(int int_y);
  virtual void  place_segment_portee(int note, int octave, float xstart, float length);  
  void  add_bar_sketch(t_bar_sketch bar);
  void  place_beams(std::list<t_note_sketch> *pnoteinf_list, int color);
  void  place_note(std::list<CMesure*> *pml, CNote *pn, std::list<t_note_sketch> *prendered_notes_list);
//  void  fill_unused_times(CScore *pscore, CInstrument *pinstrument);
  void  place_mesh(t_fcoord pos, t_fcoord dim, int color, char *meshname);
  virtual void place_bars(std::list<CMesure*> *pml);
  virtual void place_notes(std::list<CMesure*> *pml, CInstrument *pinstrument, std::list<t_note_sketch> *pnp_list);
  bool  same_measure_close_time(t_measure_note_info *pref_note, t_measure_note_info *pnext_note);
  void  place_flag(t_note_sketch *pnotesk, int color);
  void  create_beam(t_beam *pbeam);
  int   get_note_type_from_measure(std::list<CMesure*> *pml, CNote *pn, double *ptcode, t_measure_note_info *pmni);

 private:
  bool  is_in_area(t_fcoord coord, t_fcoord pos, t_fcoord dim);
  void  find_note_type(t_measure_note_info *pmni, double notelen);
  void  insert_note_sketch(std::list<t_note_sketch> *pnoteinflist, t_note_sketch *pns);
  virtual void  place_portee();
  virtual float note2y(int note, int octave);
  float appx_2_placementx(int int_x);
  virtual void  place_keys();
  float xpixel_2_float(int pixel_thickness);
  float ypixel_2_float(int pixel_thickness);
  static bool compare(const t_note_sketch *pfirst, const t_note_sketch *psecond);
  void  change_tail(t_note_sketch *pns, float y);
  void  create_beam_polygons(t_beam *pbeam, float hi);
  void  find_in_measure_note_index(t_measure_note_info *pmni, double notetimecode);
  void  calculate_absolute_note_index(t_measure_note_info *pmni);
  virtual void place_note_segments(int octave, int note, float x);
  void  place_rest_mesh(double start, float y, double duration, int color, char *meshname);
  void  place_whole_or_half_rest(double start, double duration, int color, int timedivs);
  void  add_rectangle_polygon(float x, float y, float dimx, float dimy, int color);
  void  place_sharpflat(t_fcoord pos, float radius, int color, bool bdiese);
  void  calculate_in_measure_note_characteristics(t_measure_note_info *pmeasure_note_info);
  t_measure_occupancy* find_measure_occupancy_element(std::list<t_measure_occupancy> *pmol, CMesure *pm, int index);
  void rest_placement(t_measure_occupancy *po);
  void place_rests(std::list<CMesure*> *pml, std::list<t_note_sketch> *prendered_notes_list);
  void place_practice_index();

 protected:
  Cf2n          m_f2n;
  double        m_starttime;
  double        m_viewtime;
  float         m_radius;   // note_spacing
  float         m_zradius;
  int           m_color;
  // Tmp note
  CNote         m_tmp_note;

  enum notetype
    {
      enormal = 0,
      ebemol,
      ediese
    };

  typedef struct s_n2key
  {
    int          notelevel;
    notetype     type;
  }              t_n2key;

  t_n2key m_n2k[12];
  int     m_y2n[14];

  //
  t_coord m_wpos;
  t_coord m_wdim;
  //
  int      m_notenum;
  t_fcoord m_fdim;    // floating point sizes, very good
  float    m_yxratio; // Used to keep the vertical size on the horizontal dimension
  float    m_vzoom;   // vertical zoom factor
  float    m_vmove;   // vertical offset when moving the view down

  int      m_rest_staff_octave;

  bool           m_bautobeam;
  epracticestate m_practicing;

  CInstrumentHand *m_phand;

  std::list<t_note_sketch>           m_rendered_notes_list;
  std::list<t_note_sketch>::iterator m_noteiter;
  //
  std::list<t_measure_number>           m_rendered_barnumber_list;
  std::list<t_measure_number>::iterator m_barnumberiter;

  std::list<t_bar_sketch>           m_rendered_bars_list;
  std::list<t_bar_sketch>::iterator m_bariter;
  //
  std::list<t_lsegment>             m_lsegment_list;
  std::list<t_lsegment>::iterator   m_lsegment_iter;
  //
  std::list<t_polygon>              m_polygon_list;
  std::list<t_polygon>::iterator    m_polygon_iter;
  //
  //CMeshList                     *m_mesh_list;
  std::list<t_mesh>                 m_rendered_meshes;
  std::list<t_mesh>::iterator       m_mesh_iter;
  //
  std::list<t_string_placement>           m_txt_list;
  std::list<t_string_placement>::iterator m_txt_iter;
};

