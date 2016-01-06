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

//#define BACK_COLOR 0xFF7C7F35
//#define BACK_COLOR 0xFFC13411
#define BACK_COLOR 0xFFCC0404
//#define BACK_COLOR 0xFFFF0000

#define MAXBANDS 16

#define REPLAY_FIFO_SIZE 7

#define PRACTICE_BETWEEN_REPLAY_TIME 2.

enum eplay_state
  {
    state_stop,
    state_record,
    state_play,
    state_wait_practice,
    state_practice,
    state_wait_practiceloop,
    state_practiceloop,
    state_practiceloop_rewind
  };

// For spectrogram texture updates, avoiding rewrites of same data
typedef struct s_update_segment
{
  int xstart;
  int xstop;
}              t_update_segment;

enum enotestate
  {
    state_wait_filtering,
    state_filtered,
    state_ready_2play,
    state_playing,
    state_done
  };

typedef struct s_audioOutCmd
{
  double start;
  double stop;
  double playstart;
  double playdelay;
  int    bands;
  float  fhicut[MAXBANDS];
  float  flocut[MAXBANDS];
  bool   bsubstract;
  bool   bloop_data;
  int    notestate;
  // Note pointer used only to display the note in the score in green when played
  void   *pnote;
}              t_audioOutCmd;

typedef struct s_audio_strip
{
  double       playtimecode;
  float        *samples;
  int          samplenum;
  int          current_sample;
}              t_audio_strip;

enum audio_track_command
  {
    ac_play,
    ac_stop,
    ac_record,
    ac_move,
    ac_reset,
    ac_change_io,
    ac_flush_strips,
    ac_practice_loop
  };

enum audio_track_direction
  {
    ad_thread2app,
    ad_app2threadaudio,
    ad_app2threadspectr
  };

typedef struct          s_audio_track_cmd
{
  audio_track_command   command;
  audio_track_direction direction;
  double                v;
}                       t_audio_track_cmd;

typedef struct s_shared
{
  bool  bquit;
  pthread_mutex_t datamutex;
  pthread_mutex_t dftmutex;    // Locks only *ptrackimg and *poutimg
  pthread_mutex_t filtermutex; // Locks only filtered_sound_list
  pthread_mutex_t cond_drawspectremutex;
  pthread_cond_t  cond_drawspectre;
  bool            bOpenClEnabled;
  float zoom_f;
  float zoom_t;
  float tfprecision;
  // w and h of the dft image
  int   dft_w;
  int   dft_h;
  int   *poutimg;
  //int   circularindex;
  std::list<t_update_segment> update_segment_list;
  int   circularcut;
  bool  bspectre_img_updated;
  float fbase;     // Base frequency
  float fmax;      // Changing max frequency
  float viewtime;  // Mult that by samplerate + 4 seconds to get the samples needed by the GPU
  bool  blogdb;    // Calibrated view
  // Sound image
  int   *ptrackimg;
  int   trackimgw;
  int   trackimgh;
  bool  btrack_img_updated;
  // Attack data for the current spectrometer
  float *pattackdata;
  // Audio parameters
  t_channel_select_strings chs;
  std::list<t_portaudio_api> pa_api_list;
  // In
  bool   baudioready;
  int    samplerate;
  double timecode; // Like PaTime in seconds
  double trackend;
  double lastimgtimecode;
  double practice_countdown_tick;
  double practice_start;
  double practice_loop_duration;
  float  practicespeed;
  Caudiodata *pad; // Audio input and edition buffer
  eplay_state play_State;
  std::list<t_audio_track_cmd> audio_cmds;
  int                          audio_cmd_sdlevent;
  // Out
  pthread_mutex_t soundmutex;
  pthread_mutex_t cond_soundmutex;
  pthread_cond_t  cond_sound;
  std::list<t_audioOutCmd> filter_cmd_list;
  std::list<t_audio_strip> filtered_sound_list;
  // Used to replay selections from the keyboard keys r to p
  std::list<t_audioOutCmd> replay_cmd_fifo;
}              t_shared;

//#define blu
#ifdef blu
extern int g_ct;
extern int g_tm;
extern int g_dur;
extern int g_lcount;
#include "assert.h"

//#define LOCK   pthread_mutex_lock(&pshared_data->datamutex); g_ct++; printf("file %s line %d - %d\n", __FILE__, __LINE__, g_lcount);assert(g_lcount++ == 0); g_tm = SDL_GetTicks(); g_tm = g_tm == 0? 1 : g_tm;
//#define UNLOCK assert(g_lcount-- == 1); assert(g_tm != 0); g_tm = SDL_GetTicks() - g_tm; g_dur = g_dur < g_tm? g_tm : g_dur; if (g_dur > 15000) assert(false); printf("unlock file %s line %d - %d\n", __FILE__, __LINE__, g_lcount); pthread_mutex_unlock(&pshared_data->datamutex);

#define ILOCK   pthread_mutex_lock(&pshared_data->datamutex); g_ct++; assert(g_lcount++ == 0); g_tm = SDL_GetTicks(); g_tm = g_tm == 0? 1 : g_tm;
#define IUNLOCK assert(g_lcount-- == 1); assert(g_tm != 0); g_tm = SDL_GetTicks() - g_tm; g_dur = g_dur < g_tm? g_tm : g_dur; if (g_dur > 15000) assert(false); pthread_mutex_unlock(&pshared_data->datamutex);
#endif

#define LOCK   pthread_mutex_lock(&pshared_data->datamutex);
#define UNLOCK pthread_mutex_unlock(&pshared_data->datamutex);

#define ILOCK   pthread_mutex_lock(&pshared_data->dftmutex);
#define IUNLOCK pthread_mutex_unlock(&pshared_data->dftmutex);

#define SLOCK   pthread_mutex_lock(&pshared_data->soundmutex);
#define SUNLOCK pthread_mutex_unlock(&pshared_data->soundmutex);

#define FLOCK   pthread_mutex_lock(&pshared_data->filtermutex);
#define FUNLOCK pthread_mutex_unlock(&pshared_data->filtermutex);

#define MAX_VIEW_TIME    20

