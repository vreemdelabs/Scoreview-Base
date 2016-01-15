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

enum eeselbox
  {
    noselbox,
    playselbox,
    loopselbox,
    deleteselbox
  };

typedef struct s_note_selection_box
{
  double       start_tcode;
  double       stop_tcode;
  float        hif;
  float        lof;
}              t_note_selection_box;

typedef struct         s_enote_selbox
{
  t_note_selection_box nsb;
  std::list<CNote*>    Note_list;
  bool                 bupdate;
}                      t_note_selbox;

typedef struct s_edit_state
{
  // Edited note
  CNote         *pmodified_note;
  CNote          modified_note_start_cpy;
  //std::string    hand_text;
  // Edited bar
  CMesure       *pmodified_measure;
  CMesure        modified_bar_start_cpy;
  bool           bedit_meter;
  int            chordnotenum;
  t_coord        mouse_start;
  t_coord        mouse_stop;
  bool           blrcursor;
  bool           bbeamlinkcursor;
  bool           bbrokenbeamlinkcursor;
  eeselbox       drawselbox;
  bool           btrashcan_cursor;
  bool           bwheel_cursor;
  // Play a selection of notes
  t_note_selbox  note_sel;
  //
  // To update the practice view 
  std::list<CNote*> changed_notes_list;
  std::list<CNote*> deleted_notes_list;
  std::list<CMesure*> Measure_delete_list;
  std::list<CMesure*> Measure_add_list;
}              t_edit_state;

enum eeditstate
  {
    waiting,
    onnote,
    movenote,
    movechord,
    notelen,
    togglebeam,
    onbar,
    movebar,
    barlen,
    bartimes,
//    addnote, done in the spectrometer
    deletenote,
    deletenotes,
//    addbar,
    deletebar,
    playbox,
    playboxnonotes,
    practiceloopbox,
    move_visual,
    copymeasure,
    pastemeasure
  };

class CscoreEdit
{
 public:
  CscoreEdit(Ckeypress *pkstates);
  virtual ~CscoreEdit();

  void   reset_edit_state();
  t_edit_state* get_edit_state();
  void   mouse_left_down(CScore *pscore, CInstrument *pcur_instru, CScorePlacement *pplacement, t_coord mouse_coordinates);
  bool   mouse_left_up(CScore *pscore, CInstrument *pcur_instru, CScorePlacement *pplacement, t_coord mouse_coordinates);
  void   mouse_right_down(t_coord mouse_coordinates);
  bool   mouse_right_up(t_coord mouse_coordinates);
  bool   mouse_move(CScore *pscore, CScorePlacement *pplacement, t_coord mouse_coordinates);
  bool   mouse_wheel(CScorePlacement *pplacement, t_coord mouse_coordinates, int inc);
  void   set_chord_fuse(bool bactive);
  bool   on_note_change(int *pnote_id);

 protected:
  virtual void fuse_chords(CScorePlacement *pplacement, CInstrument *pinst);
  static bool compare_note_sketches(const t_note_sketch *pfirst, const t_note_sketch *psecond);
  void   fuse_into_chord(CNote *pref_note, CNote *pdel_note, CInstrument *pinst);

 private:
  bool   set_played_notes_list(CInstrument *pinst, CScorePlacement *pplacement, t_note_selbox *pnsb);
  void   delete_selected_notes(CInstrument *pinst, CScorePlacement *pplacement);
  void   remove_duplicate_frequencies(CNote *pnote);
  virtual bool change_chord_frequency_element(CScorePlacement *pplacement, CNote *pnote, float frequency, int element_num, bool btimechange);
  bool   move_note_or_chord(CScorePlacement *pplacement);
  CNote* split_chord(CNote *pnote, int freqnum);
  //void   set_note_changes_list(CNote *pnote);

 protected:
  bool         m_bfuse_chords; 
  t_edit_state m_editstate;

 private:
  Ckeypress   *m_pks;
  int          m_state;
  bool         m_bonnote_chg;
  // reverse action
  //CNote        *m_pmodified_note;
  //CMesure      *m_pmodified_bar;
};

