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
// Switched from xml to rpotocol buffers
/*
  Dialog names in "messages.h"
*/

// The messages will be sent like:
// 16b    Magic number
// 32b    Size
// n x 8b Protocol buffer wiremessage
//
#define NMAGICN     0x217A
#define HEADER_SIZE      6

namespace scmsg
{

typedef struct s_intrument
{
  std::string  name;
  int          voice_index;
}              t_instrument;

enum efileOperation
  {
    edonothing,
    efileopen,
    efilesave,
    enewfile,
    enewfromfile
  };

typedef struct s_practice_params
{
  double       curtimecode;
  double       viewtime;
  bool         reload;
  bool         start_practice;
  bool         loop;
  float        speedfactor;
  double       countdownDaytime;
  double       looptime;
  float        hifrequency;
  float        lofrequency;
  t_instrument instru;
}              t_practice_params;

typedef struct s_app_config
{
  bool         recordAtStart;
  bool         doNotChangeOpenedFiles;
}              t_appconfig;

class Cmessage_coding
{
 public:
  Cmessage_coding();
  ~Cmessage_coding();

  //-------------------------------------------------------
  // Receive
  //-------------------------------------------------------
  int            get_next_wire_message(char *buffer, int received_size);
  eWiremessages  message_type();

  edialogs_codes app_code();
  bool get_dialog_name(std::string *name);

  bool get_practice(t_practice_params *pp);
  bool get_remadd_note(CNote *pn, bool *pbdelete);
  bool get_note_highlight_message(int *pidentifier);
  bool get_remadd_measure(CMesure *pm, bool *pbdelete);
  bool get_instrument_list(std::list<t_instrument> *plist);
  bool get_score_message(CScore *pscore);
  bool get_notelist_message(CScore *pscore);
  //
  bool get_audioIO_config(std::list<t_portaudio_api> *papis, t_channel_select_strings *pcs);
  bool get_config(t_appconfig *pcfg);
  //
  bool get_fileIO_message(std::string *ppath, std::string *pfile_name, efileOperation *pfo);
  bool get_remadd_instrument(t_instrument *pinstru, bool *pbdelete);

  //-------------------------------------------------------
  // Send
  //-------------------------------------------------------
  bool build_wire_message(char *message_out, int bsize, int *size, std::string input);
  //
  std::string create_practice_message(t_practice_params *pp);
  std::string create_close_message();
  std::string create_remadd_note_message(CNote *pn, bool bdelete);
  std::string create_note_highlight_message(int identifier);
  std::string create_remadd_measure_message(CMesure *pm, bool bdelete);
  std::string create_instrument_list_message(std::list<t_instrument> *plist);
  std::string create_score_message(CScore *pscore);
  std::string create_notelist_message(CInstrument *pins, int firstnote, int lastnote);
  // Bidir
  std::string create_audioIO_config_message(std::list<t_portaudio_api> *papis, t_channel_select_strings *pcs);
  std::string create_config_message(t_appconfig *pcfg);
  // From dialogs
  std::string create_dialog_closed_message(std::string dialog_name);
  std::string create_dialog_opened_message(std::string dialog_name);
  std::string create_file_os_message(std::string path, std::string file_name, efileOperation fo);
  std::string create_remadd_instrument_message(t_instrument *pinstru, bool bdelete);

 private:
  std::string pack_message(std::string *message, eWiremessages type);
  // Get
  edialogs_codes dialog_name_to_code(std::string dialog_name);
/* Not seen outside the class
  void get_instrument(const scoreview::instrument *pinstru, t_instrument *pinstructure);
  bool get_note(const scoreview::note *pnotedesc, CNote *pn);
  void get_measure(const scoreview::measure *psm, CMesure *pm);
  void get_channel_selection(const scoreview::currentAudioIO *piochannels, t_channel_select *pcs);
  void get_api(const scoreview::audioApi *papi, std::list<t_portaudio_api> *papis);
  // Set
  scoreview::instrument* allocate_instrument(t_instrument *pinstru);
  scoreview::note* allocate_note(CNote *pn);
  scoreview::measure* allocate_measure(CMesure *pm);
  scoreview::audioChannel* allocate_audio_channelCfg(int api, int device);
  scoreview::currentAudioIO* allocate_current_audioIOcfg(t_channel_select *pcs);
*/
 private:
  eWiremessages  m_message_type;
  edialogs_codes m_app_code;
  std::string    m_msg_data;
  std::string    m_dialog_name;
};

}

