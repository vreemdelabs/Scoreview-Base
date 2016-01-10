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

#define TXTSZ 1024
#define HARMONICS 10

#define MAX_INSTRUMENTS  8

#define COUNTDOWN 3.

#define MIN_WINDOW_W 786 
#define MIN_WINDOW_H 569

#define TXTCOLOR 0xFFFEFEFE

#define MAX_INTRACK_PLAY_TIME 3.
#define PRACTICE_LOOP_DELAY 1.

enum eloopstate
  {
    loopstate_waiting4countdown,
    loopstate_playing,
    loopstate_stop
  };

typedef struct              s_note_selection
{
  t_note_selection_box      box;
  double                    play_timecode;
  double                    practice_speed;
  eloopstate                state;
  std::list<t_audioOutCmd*> cmdlist;
}                           t_note_selection;

typedef struct s_card_states
{
  bool         bhelpers_enabled;
  bool         brecording;
  bool         autobeam;
  bool         chordfuse;
}              t_card_states;

class Cappdata
{
 public:
  Cappdata(int width, int height);
  ~Cappdata();

  int  init_the_network_messaging();
  void create_layout();
  void render_gui();
  void wakeup_spectrogram_thread();
  void wakeup_bandpass_thread();
  void key_on(int code);
  void key_off(int code);
  void handle_weel(int x, int y, int v);
  void mouseclickright(int x, int y);
  void mouseclickleft(int x, int y);
  void mouseupright(int x, int y);
  void mouseupleft(int x, int y);
  void mousemove(int x, int y);
  void mousedidnotmove(bool bmoved);
  void setlooptime(double time);
  void process_network_messages();
  void process_audio_events();
  void change_window_size(int newWidth, int newHeight);
  void timed_events();

 private:
  void set_coord(t_fcoord *coord, float prx, float pry);
  void draw_basic_rectangle(Cgfxarea *pw, int color);
  void rendertext(t_coord pos, int color, char *text);
  void draw_h_line(t_coord pos, int length, int color);
  void draw_v_line(t_coord pos, int length, int color);
  void draw_scale(Cgfxarea *pw, int color, float minf, float maxf, float tensfactor);
  void draw_freq_scale(Cgfxarea *pw, int color, float minf, float maxf);
  void draw_piano_scale(Cgfxarea *pw, int color, float fmin, float fmax, t_coord mousepos);
  void draw_attacks(Cgfxarea *pw, int color);
  void draw_time_scale(Cgfxarea *pw, int color, float viewtime, double timecode);
  void copy_spectrogram(Cgfxarea *pw);
  void copy_track(Cgfxarea *pw);
  void zoom_frequency(int v);
  void zoom_viewtime(int v);
  float  getSpectrum_freq_fromY(int y, t_coord pos, t_coord dim, float fbase, float fmax);
  double getSpectrum_time_fromX(int x, t_coord pos, t_coord dim, double timecode, double viewtime);
  int    getSpectrumY_from_Frequ(float f, t_coord pos, t_coord dim, float fbase, float fmax);
  int    getSpectrumX_from_time(double time, t_coord pos, t_coord dim, double timecode, double viewtime);
  void note_from_spectrumy(t_coord pos, t_coord dim, int y, int *pnote, int *poctave, float fbase, float fmax);
  void set_cmd_start_stop(t_audioOutCmd &cmd, t_coord pos, t_coord dim, int xstart, int x);
  void create_filter_play_message(t_coord boxstart, int x, int y, t_coord pos, t_coord dim);
  bool set_cmd_bands(t_audioOutCmd &cmd, float basef, float fbase, float fmax);
  void create_filter_play_harmonics_message(int x, int y, t_coord pos, t_coord dim);
  void create_filter_play_harmonics_message(CNote *pnote, double playdelay, bool bloopdata, std::list<t_audioOutCmd*> *pcmdlist);
  void render_sound_strips_progress(Cgfxarea *pw, double currentime, double starttime, double viewtime);
  void render_sound_strips_progress_on_track(Cgfxarea *pw, double tracklen, double currentime);
  void render_sel_note_square(Cgfxarea *pw, int note, int octave, int xstart, int xstop, bool bbasefreqonly = false);
  void render_sel_note_square(Cgfxarea *pw, int x, int y, bool bbasefreqonly = false);
  void render_sel_note_square(Cgfxarea *pw, CNote *pn, double starttime, double viewtime, bool bbasefreqonly = false);
  void print_harmonic_note_info(Cgfxarea *pw, float f, int xsta, int xsto, int ysta, int ysto, int curoctave);
  void render_measure_selection(Cgfxarea *pw, int measure_times, int xstart, int xstop);
  void create_cards_layout();
  void card_mousemove(int x, int y);
  void card_mouseclickleft(int x, int y);
  void card_mouseclickright(int x, int y);
  void card_mouseupright(int x, int y);
  void enable_card_function(Ccard *pc);
  void render_cards();
  void check_card_inspection(int x, int y, erclick brclick, bool bmove = false);
  void toggle_card(const char *card_name);
  void set_record_card(bool active);
  void stick_to_area(Cgfxarea *pw, int x, int y, int &cx, int &cy);
  void play_edition_sel_notes(t_edit_state *pedit_state);
  void set_played_note_list_for_score_highlight(std::list<CNote*> *pplayed_note_list);
  void render_measures_edition(Cgfxarea *pw, CMesure *pmeasure, double starttime, double viewtime);
  bool save_sound_buffer(string file_name);
  bool load_sound_buffer(string file_name);
  void load_score(string path);
  void save_score(string path);
  void save_xml_score(string path);
  void clear_sound();
  void send_instrument_list();
  int  instrument_tab_height(Cgfxarea *pw);
  void render_instrument_tabs(Cgfxarea *pw);
  void select_instrument(int mouse_x, int mouse_y);
  void play_record_enabled(bool enabled);
  void play_practice_enabled_in_locked_area(bool enabled);
  void send_practice_message(int mtype, double timecode, double stoptimecode, float hif, float lowf, float viewtime, float practicespeed);
  void send_score_to_practice(CScore *pscore);
  void update_practice_view();
  void update_practice_stop_time(bool reload);
  void send_note_to_practice_view(CNote *pn, bool deletenote);
  void send_measure_to_practice_view(CMesure *pm, bool deletemeasure);
  void draw_countdown(Cgfxarea *pw, double countdown_time);
  void set_practice_card(bool bactive);
  void renderTextBlended(t_coord pos, int color, string text, bool blended = false);
  void render_background_tile();
  void read_config_data(int *xpos, int *ypos, t_card_states *pcs);
  void save_config_data(int xpos, int ypos, t_card_states *pcs);
  void move_trackpos_to(int mousex, Cgfxarea *pw, bool buseviewtime);
  void drawhandcursor(t_coord mousepos);
  void reopen_stream(t_channel_select_strings *pchs);
  void send_apidevice_list();
  void send_configuration();
  CScorePlacement* get_instrumentPlacement(std::string instrument_name);
  CscoreEdit* get_instrumentEdit(std::string instrument_name);
  void draw_edited_times(Cgfxarea *pw, CInstrument *pinstrument, float tracklen);
  void render_notes_on_spectrum(Cgfxarea *pw, CInstrument *pinstrument, double starttime, double viewtime);
  void render_measures_on_spectrum(Cgfxarea *pw, CScore *pscore, double starttime, double viewtime);
  void draw_measures_on_track(Cgfxarea *pw, CScore *pscore, double tracklen);
  bool load_meshes(CMeshList *pml);
  void  render_key_texture(int color, t_fcoord pos, t_fcoord dim, string name);
  float getSpectrumFloatY_from_Frequ(float f, t_fcoord pos, t_fcoord dim, float fbase, float fmax);
  void  Draw_keys(t_fcoord pos, t_fcoord dim, bool bblack, int curoctave, int curnote, float fmin, float fmax);
  void  add_note(int cx, int cy, t_coord pos, t_coord dim);
  void create_filter_play_message_from_track(int xstart, int xstop, t_coord pos, t_coord dim);
  void add_sound_selection_to_fifo(t_audioOutCmd *pcmd);
  void play_fifo_sound_selection(int fifo_index);
  void set_score_renderer(std::string instrument_name);
  void create_note_box(t_note_selbox *pnsb, t_note_selection *pappsel);
  bool check_if_notes_just_filtered(std::list<t_audioOutCmd*> *pcmdlist);
  void delete_filter_commands(std::list<t_audioOutCmd*> *pcmdlist, bool bkeep_loop_cmds);
  void reset_filter_on_loop_commands(t_note_selection *pnote_selection);
  void render_selected_notes_box(Cgfxarea *pw, CScorePlacement *pplacement, double viewtime);
  void flush_filtered_strips();
  void stop_note_selection(t_note_selection *psel);
  void remove_note_selection(t_note_selection *psel);
  void printfscore();
  bool queued_filtered_band();
  void send_start_stop(audio_track_command cmd);
  void draw_fps(double last_time);
  
 public:
  int               m_width;
  int               m_height;
  SDL_Window       *m_sdlwindow;
  int               m_window_xpos;
  int               m_window_ypos;
  SDL_GLContext     m_GLContext;
  CGL2Dprimitives  *m_gfxprimitives;
  TTF_Font         *m_font;
  TTF_Font         *m_bigfont;
  CMeshList        *m_mesh_list;

  CgfxareaList   *m_layout;
  CCardList      *m_cardlayout;

  t_shared       m_shared_data;

  pthread_t      m_threadspectrometre;
  pthread_t      m_threadbandpass;
  pthread_t      m_threadaudio;
  pthread_t      m_threaddialogs;

  CScore         *m_pscore;
  CInstrument    *m_pcurrent_instrument;
  CNote           m_empty_note;

  CpictureList    *m_picturelist;
  CScorePlacement *m_pScorePlacement;
  CScoreRenderer  *m_pScoreRenderer;
  CscoreEdit      *m_pScoreEdit;

  CScorePlacementViolin *m_pScorePlacementViolin;
  CscoreEdit            *m_pScoreEditViolin;
  CScorePlacementPiano  *m_pScorePlacementPiano;
  CscoreEdit            *m_pScoreEditPiano;
  CScorePlacementGuitar *m_pScorePlacementGuitar;
  CscoreEdit            *m_pScoreEditGuitar;

  t_note_selection m_note_selection;

  // Network messaging
  enum epractice_msg_type
    {
      practice_area,
      practice_stop_at,
      practice_update,
      practice_update_countdown,
      practice_area_loop_countdown
    };

  CnetworkMessaging *m_pserver;

  // App state machine

  enum appstate
    {
      statewait = 1,
      stateaddnote,
      stateaddnotedirect,
      statemove,
      statemovehand,
      stateselectsound,
      stateselectsoundontrack,
      stateselectharmonics,
      stateaddmeasure,
      statefindsound,
      stateeditscore,
      statepractice,
      statepracticeloop,
      stateselectinstrument
    };

  appstate       m_appstate;
  t_card_states  m_card_states;
  t_coord        m_cstart;
  t_coord        m_ccurrent;
  Ckeypress      m_kstates;

  double       m_last_time;
  bool         m_pianofrequdisplay;
  Cf2n         m_f2n;
  float        m_localfbase;
  float        m_localfmax;

  bool         m_brecord_at_start;
  bool         m_bdonotappendtoopen;
  bool         m_opened_a_file;
};

