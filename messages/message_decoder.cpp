/*
 Scoreview (R)
 Copyright (C) 2015-2016 Patrick Areny
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
#include <string>
#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <iterator>
#include <list>

#include "scoreview.pb.h"
#include "score.h"
#include "audioapi.h"
#include "messages.h"
#include "message_decoder.h"

using namespace std;
using namespace scmsg;

Cmessage_coding::Cmessage_coding():
  m_message_type(network_message_void),
  m_app_code(unknown_dialog)
{
  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

Cmessage_coding::~Cmessage_coding()
{
  //google::protobuf::ShutdownProtobufLibrary();
}

#define CHECK_DIALOG_TYPE(A, T)  if (dialog_name == string(A)) dialog_code = T;

edialogs_codes Cmessage_coding::dialog_name_to_code(std::string dialog_name)
{
  edialogs_codes dialog_code;

  dialog_code = unknown_dialog;
  CHECK_DIALOG_TYPE(STORAGE_DIALOG_CODE_NAME, load_save_dialog);
  CHECK_DIALOG_TYPE(ADDINSTRUMENT_DIALOG_CODENAME, add_instrument_dialog);
  CHECK_DIALOG_TYPE(PRACTICE_DIALOG_CODENAME, practice_dialog);
  CHECK_DIALOG_TYPE(CONFIG_DIALOG_CODENAME, config_dialog);
  if (dialog_code == unknown_dialog)
    {
      printf("An unknown dialog has connected\n");
      exit(EXIT_FAILURE);
    }
  return dialog_code;
}

string messagetypestr(int type)
{
  switch (type)
    {
    case network_message_close:
      return string("network_message_close");
    case network_message_remadd_note:
      return string("network_message_remadd_note");
    case network_message_remadd_measure:
      return string("network_message_remadd_measure");
    case network_message_practice:
      return string("network_message_practice");
    case network_message_instrument_list:
      return string("network_message_instrument_list");
    //----------------------------------------------------------------------
    // bidirectional
    //----------------------------------------------------------------------
    case network_message_pa_dev_selection:
      return string("network_message_pa_dev_selection");
    case network_message_configuration:
      return string("network_message_configuration");
    //----------------------------------------------------------------------
    // Messages sent by dialog boxes
    //----------------------------------------------------------------------
    case network_message_closed_dialog:
      return string("network_message_closed_dialog");
    case network_message_dialog_opened:
      return string("network_message_dialog_opened");
    case network_message_file:
      return string("network_message_file");
    case network_message_remadd_instrument:
      return string("network_message_remadd_instrument");
    default:
      break;
    }
  return string("unknown message");
}

#define CHECK_MSG_TYPE(A, T) 	      if (typestr == string(A)) {m_message_type = T; btype_found = true;}

// Gets the next message of type "wireMessage" from a char buffer and returns the size of it, .
// Returns zero if no message is found in the data input
int Cmessage_coding::get_next_wire_message(char *msg_data, int received_size)
{
//#define SHOW_MESSAGES
  int32_t               *psize;
  std::string            str;
  scoreview::wireMessage msg;

  if (received_size <= 0)
    return 0;
  if (received_size < 8)
    {
      printf("Error: message too short to be consistent.\n");
      exit(EXIT_FAILURE);
    }
  if (msg_data[0] != (NMAGICN & 0xFF) ||
      msg_data[1] != ((NMAGICN >> 8) & 0xFF))
    {
      printf("Error: wrong message's magic number: 0x%x %x.\n", msg_data[1], msg_data[0]);
      exit(EXIT_FAILURE);
    }
  psize = (int32_t*)&msg_data[2];
  if (*psize < 1)
    {
      printf("Error: wire message size must be wrong.\n");
      exit(EXIT_FAILURE);
    }
  if (received_size < *psize + HEADER_SIZE)
    {
      return 0;
    }
  // Recover the string
  str = string(&msg_data[6], *psize);
  // Parse it
  if (!msg.ParseFromString(str))
    {
      printf("Failed to parse a message data.\nMessage:\n");
      printf("%s\n", msg.DebugString().c_str());
      return 0;
    }
#ifdef SHOW_MESSAGES
  else
    {
      printf("Correct message: \"%s\".\n", msg.ShortDebugString().c_str());
    }
#endif
  if (!msg.IsInitialized())
    {
      printf("Critical error: a message data is not properly initialised. Cannot allow this. Goodbye.\n");
      exit(EXIT_FAILURE);
    }
  // Get the message type for further processing.
  m_message_type = (eWiremessages)msg.type();
#ifdef SHOW_MESSAGES
  string typestr = messagetypestr((eWiremessages)msg.type());
  printf("message type = %s.\n", typestr.c_str());
#endif
  // Get the message raw data.
  if (msg.has_messagedata())
    {
      m_msg_data = msg.messagedata();
    }
#ifdef SHOW_MESSAGES
  int size = msg.ByteSize();
  printf("message size is 6 bytes header + %dbytes\n", size);
#endif
  return *psize + HEADER_SIZE;
}

eWiremessages Cmessage_coding::message_type()
{
  return m_message_type;
}

// Special case for dialog open/closes message, returns the enum value
edialogs_codes Cmessage_coding::app_code()
{
  edialogs_codes code;

  switch (m_message_type)
    {
    case network_message_closed_dialog:
    case network_message_dialog_opened:
      {
	scoreview::dialogName dname;

	if (!dname.ParseFromString(m_msg_data))
	  {
	    printf("Failed to parse a message data.\nMessage:\n");
	    printf("%s\n", dname.DebugString().c_str());
	    return unknown_dialog;
	  }
	if (!dname.IsInitialized())
	  {
	    printf("Critical error: a message data is not properly initialised. Cannot allow this. Goodbye.\n");
	    exit(EXIT_FAILURE);
	  }
	if (dname.has_dialogname())
	  {
	    m_dialog_name = dname.dialogname();
	    code = dialog_name_to_code(m_dialog_name);
	  }
      }
      break;
    default:
      {
	printf("Warning: inconsistent dialog open/close message.\n");
	exit(EXIT_FAILURE);
      }
      break;
    };
  return code;
}

bool Cmessage_coding::get_dialog_name(string *pname)
{
  app_code();
  *pname = m_dialog_name;
  return true;
}

void get_instrument(const scoreview::instrument *pinstru, t_instrument *pinstructure)
{
  if (pinstru->has_instrumentname())
    {
      pinstructure->name = pinstru->instrumentname();
    }
  else
    pinstructure->name = string("none");
  if (pinstru->has_voiceindex())
    {
      pinstructure->voice_index = pinstru->voiceindex();
    }
  else
    pinstructure->voice_index = -1;
}

bool Cmessage_coding::get_practice(t_practice_params *pp)
{
  scoreview::practice   pr;
  scoreview::instrument instru;
  t_instrument          instructure;

  if (!pr.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the practice message data.\nMessage:\n");
      printf("%s\n", pr.DebugString().c_str());
      return false;
    }
  if (!pr.IsInitialized())
    {
      printf("Critical error: practice message data is not properly initialised. Cannot allow this. Goodbye.\n");
      exit(EXIT_FAILURE);
    }
  pp->curtimecode = pr.timecode();
  pp->viewtime = pr.viewtime();
  if (pr.has_reload())
    {
      pp->reload = pr.reload();
    }
  else
    pp->reload = false;
  if (pr.has_dopractice())
    {
      pp->start_practice = pr.dopractice();
    }
  else
    pp->start_practice = false;
  if (pr.has_loop())
    {
      pp->loop = pr.loop();
    }
  else
    pp->loop = false;
  if (pr.has_speedfactor())
    {
      pp->speedfactor = pr.speedfactor();
    }
  else
    pp->speedfactor = 1.;
  if (pr.has_countdowndaytime())
    {
      pp->countdownDaytime = pr.countdowndaytime();
    }
  else
    pp->countdownDaytime = 0;
  if (pr.has_looptime())
    {
      pp->looptime = pr.looptime();
    }
  if (pr.has_hifreq())
    {
      pp->hifrequency = pr.hifreq();
    }
  else
    pp->hifrequency = 23000.;
  if (pr.has_lofreq())
    {
      pp->lofrequency = pr.lofreq();
    }
  else
    pp->lofrequency = 16;
  if (pr.has_instru())
    {
      instru = pr.instru();
      get_instrument(&instru, &pp->instru);
    }
  else
    pp->instru.voice_index = -1;
  return true;
}

bool get_note(const scoreview::note *pnotedesc, CNote *pn)
{
  int                      size;
  scoreview::noteFrequency nf;
  t_notefreq               freq;

  if (pnotedesc->has_time())
    {
      pn->m_time = pnotedesc->time();
    }
  else
    printf("Warning, critical: note timecode not found.\n");
  if (pnotedesc->has_duration())
    {
      pn->m_duration = pnotedesc->duration();
    }
  else
    printf("Warning, critical: note duraiton not found.\n");
  if (pnotedesc->has_uniqueid())
    {
      pn->set_identifier((int)pnotedesc->uniqueid());
    }
  else
    printf("Warning, critical: note unique identifier not found.\n");
  // Frequency list
  pn->m_freq_list.clear();
  size = pnotedesc->freqs_size();
  for (int i = 0; i < size; i++)
    {
      nf = pnotedesc->freqs(i);
      if (nf.has_frequency())
	freq.f = nf.frequency();
      else
	freq.f = 0;
      if (nf.has_stringnum())
	freq.string = nf.stringnum();
      else
	{
	  freq.string = -1;
	  printf("Warning: note string number not found.\n");
	}
      pn->m_freq_list.push_back(freq);
    }
  return true;
}

bool Cmessage_coding::get_remadd_note(CNote *pn, bool *pbdelete)
{
  scoreview::noteRemAdd  noteRA;
  const scoreview::note *pnotedesc;

  if (!noteRA.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the note message data.\nMessage:\n");
      printf("%s\n", noteRA.DebugString().c_str());
      return false;
    }
  if (!noteRA.IsInitialized())
    {
      printf("Critical error: note message data is not properly initialised. Cannot allow this. Goodbye.\n");
      exit(EXIT_FAILURE);
    }
  *pbdelete = noteRA.todelete();
  if (noteRA.has_notedesc())
    {
      pnotedesc = &noteRA.notedesc();
      get_note(pnotedesc, pn);
    }
  return true;
}

bool Cmessage_coding::get_note_highlight_message(int *pidentifier)
{
  string output;
  scoreview::noteHighlight nh;

  if (!nh.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the note message data.\nMessage:\n");
      printf("%s\n", nh.DebugString().c_str());
      return false;
    }
  if (!nh.IsInitialized())
    {
      printf("Critical error: note message data is not properly initialised. Cannot allow this. Goodbye.\n");
      exit(EXIT_FAILURE);
    }
  *pidentifier = -1;
  if (nh.has_uniqueid())
    {
      *pidentifier = nh.uniqueid();
    }
  return true;
}

void get_measure(const scoreview::measure *psm, CMesure *pm)
{
  if (psm->has_times())
    {
      pm->m_times = psm->times();
    }
  if (psm->has_starttimecode())
    {
      pm->m_start = psm->starttimecode();
    }
  if (psm->has_duration())
    {
      pm->m_stop = pm->m_start + psm->duration();
    }
}

bool Cmessage_coding::get_remadd_measure(CMesure *pm, bool *pbdelete)
{
  scoreview::measureRemAdd mra;

  if (!mra.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the measure message data.\nMessage:\n");
      printf("%s\n", mra.DebugString().c_str());
      return false;
    }
  if (!mra.IsInitialized())
    {
      printf("Critical error: note message data is not properly initialised. Cannot allow this. Goodbye.\n");
      exit(EXIT_FAILURE);
    }
  *pbdelete = mra.todelete();
  if (mra.has_mesdesc())
    {
      get_measure(&mra.mesdesc(), pm);
    }
  return true;
}

void get_instruments(scoreview::score *pscsc, CScore *pscore)
{
  int          size;
  t_instrument instructure;
  CInstrument* pins;

  size = pscsc->instruelts_size();
  for (int i = 0; i < size; i++)
    {
      const scoreview::instrument& instru = pscsc->instruelts(i);
      get_instrument(&instru, &instructure);
      pins = new CInstrument(instructure.name);
      pins->set_identifier(instructure.voice_index);
      pscore->add_instrument(pins);
    }
}

bool Cmessage_coding::get_score_message(CScore *pscore)
{
  scoreview::score score;
  int              size;
  CMesure         *pm;

  if (!score.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the score message data.\nMessage:\n");
      printf("%s\n", score.DebugString().c_str());
      return false;
    }
  if (!score.IsInitialized())
    {
      printf("Critical error: note message data is not properly initialised. Cannot allow this. Goodbye.\n");
      exit(EXIT_FAILURE);
    }
  if (score.instruelts_size() > 0)
    {
      get_instruments(&score, pscore);
    }
  else
    {
      printf("Error: no instrument available.\n");
      return false;
    }
  if (score.mesdesc_size() > 0)
    {
      size = score.mesdesc_size();
      for (int i = 0; i < size; i++)
	{
	  pm = new CMesure();
	  get_measure(&score.mesdesc(i), pm);
	  pscore->add_measure(pm);
	}
    }
  return true;
}

// 20 or so notes per message
bool Cmessage_coding::get_notelist_message(CScore *pscore)
{
  scoreview::scoreNotes scorenote;
  t_instrument          instru;
  string                instrument_name;
  int                   instrument_id;
  CInstrument          *pins;
  int                   size;
  CNote                *pn;

  if (!scorenote.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the scorenote message data.\nMessage:\n");
      printf("%s\n", scorenote.DebugString().c_str());
      return false;
    }
  if (!scorenote.IsInitialized())
    {
      printf("Critical error: note message data is not properly initialised. Cannot allow this. Goodbye.\n");
      exit(EXIT_FAILURE);
    }
  if (scorenote.has_instru())
    { 
      get_instrument(&scorenote.instru(), &instru);
      instrument_name = instru.name;
      instrument_id = instru.voice_index;
    }
  else
    {
      return false;
    }
  pins = pscore->get_instrument(instrument_name, instrument_id);
  if (pins != NULL && scorenote.notelist_size() > 0)
    {
      size = scorenote.notelist_size();
      for (int i = 0; i < size; i++)
	{
	  const scoreview::note *pnotedesc = &scorenote.notelist(i);
	  pn = new CNote();
	  get_note(pnotedesc, pn);
	  pins->add_note(pn);
	}
    }
  else
    {
#ifdef _DEBUG
      printf("Practice, error: failed to find the propre instrument to store a note.\n");
#endif
      return false;
    }
  return true;
}

bool Cmessage_coding::get_instrument_list(std::list<t_instrument> *plist)
{
  scoreview::instrumentList inslist;
  int                       size;
  t_instrument              instructure;

  if (!inslist.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the instrument list message data.\nMessage:\n");
      printf("%s\n", inslist.DebugString().c_str());
      return false;
    }
  plist->clear();
  size = inslist.instruelts_size();
  for (int i = 0; i < size; i++)
    {
      const scoreview::instrument& instru = inslist.instruelts(i);
      get_instrument(&instru, &instructure);
      plist->push_back(instructure);
    }
  return true;
}

void get_channel_selection(const scoreview::currentAudioIO *piochannels, t_channel_select_strings *pcs)
{
  pcs->api_name = string("");
  if (piochannels->has_apiname())
    {
      pcs->api_name = piochannels->apiname();
    }
  if (piochannels->has_inputdevicename())
    {
      pcs->in_device_name = piochannels->inputdevicename();
    }
  if (piochannels->has_outputdevicename())
    {
      pcs->out_device_name = piochannels->outputdevicename();
    }
}

void get_api(const scoreview::audioApi *papi, std::list<t_portaudio_api> *papis)
{
  t_portaudio_api       pauapi;
  int                   size;
  scoreview::audioDevice dev;
  t_pa_device           padev;

  if (papi->has_name())
    {
      pauapi.name = papi->name();
    }
  else
    pauapi.name = string("unknown error");
  pauapi.device_list.clear();
  // Device list
  size = papi->devlist_size();
  for (int i = 0; i < size; i++)
    {
      dev = papi->devlist(i);
      if (dev.has_name())
	padev.name = dev.name();
      else
	{
	  printf("Message error: device name missing.\n");
	  exit(EXIT_FAILURE);
	}
      padev.inputs = padev.outputs = 0;
      if (dev.has_out())
	padev.outputs = dev.out();
      if (dev.has_in())
	padev.inputs = dev.in();
      if (dev.has_deviceapicode())
	padev.device_api_code = dev.deviceapicode();
      pauapi.device_list.push_back(padev);
    }
  papis->push_back(pauapi);
}

bool Cmessage_coding::get_audioIO_config(std::list<t_portaudio_api> *papis, t_channel_select_strings *pcs)
{
  t_portaudio_api          api;
  scoreview::audioIOConfig ioc;
  scoreview::audioApi      audioapi;
  int                      size;

  if (!ioc.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the audio input/output config message data.\nMessage:\n");
      printf("%s\n", ioc.DebugString().c_str());
      return false;
    }
  if (ioc.has_current())
    {
      get_channel_selection(&ioc.current(), pcs);
    }
  size = ioc.apis_size();
  for (int i = 0; i < size; i++)
    {
      get_api(&ioc.apis(i), papis);
    }
  return true;
}

bool Cmessage_coding::get_config(t_appconfig *pcfg)
{
  scoreview::configuration config;

  if (!config.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the config message data.\nMessage:\n");
      printf("%s\n", config.DebugString().c_str());
      return false;
    }
  if (config.has_recordatstart())
    {
      pcfg->recordAtStart = config.recordatstart();
    }
  else
    pcfg->recordAtStart = true;
  if (config.has_donotchangeopenedfiles())
    {
      pcfg->doNotChangeOpenedFiles = config.donotchangeopenedfiles();
    }
  else
    pcfg->doNotChangeOpenedFiles = false;
  return true;
}

bool Cmessage_coding::get_fileIO_message(std::string *ppath, std::string *pfile_name, efileOperation *pfo)
{
  scoreview::openScoreFiles fileio;

  if (!fileio.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the config message data.\nMessage:\n");
      printf("%s\n", fileio.DebugString().c_str());
      return false;
    }
  if (fileio.has_operation())
    {
      *pfo = (efileOperation)fileio.operation();
    }
  else
    {
      printf("Error: missing required field in file io message.\n");
      exit(EXIT_FAILURE);
      return false;
    }
  if (fileio.has_path())
    {
      *ppath = fileio.path();
    }
  else
    {
      printf("Warning: missing file io path.\n");
      return false;
    }
  if (fileio.has_filename())
    {
      *pfile_name = fileio.filename();
    }
  else
    {
      printf("Warning: missing file io path.\n");
      return false;
    }
  return true;
}

bool Cmessage_coding::get_remadd_instrument(t_instrument *pinstru, bool *pbdelete)
{
  scoreview::InstrumentRemAdd ira;

  if (!ira.ParseFromString(m_msg_data))
    {
      printf("Failed to parse the config message data.\nMessage:\n");
      printf("%s\n", ira.DebugString().c_str());
      return false;
    }
  *pbdelete = false;
  if (ira.has_todelete())
    {
      *pbdelete = ira.todelete();
    }
  if (ira.has_instru())
    {
      get_instrument(&ira.instru(), pinstru);
    }
  return true;
}

