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
#include <string>
#include <stdio.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
//#include <iostream>
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

bool Cmessage_coding::build_wire_message(char *message_out, int bsize, int *size, std::string input)
{
  int   strsize;
  int  *psz;
  char *po;
  int   i;

  strsize = input.size();
  if (strsize + 6 >= bsize)
    {
      return false;
    }
  *size = strsize + 6;
  message_out[0] = NMAGICN & 0xFF;
  message_out[1] = (NMAGICN >> 8) & 0xFF;
  printf("message outmagicnumber is %x %x\n", message_out[1], message_out[0]);
  psz = (int32_t*)&message_out[2];
  *psz = strsize;
  po = &message_out[6];
  for (i = 0; i < strsize; i++)
    po[i] = input.c_str()[i];
  return true;
}

std::string Cmessage_coding::pack_message(string *message, eWiremessages type)
{
  string output;
  bool   res;
  scoreview::wireMessage wiremessage;

  if (message == NULL)
    return NULL;
  wiremessage.set_type(type);
  if (message->size() > 0) // If 0 then no message data, only the type
    wiremessage.set_messagedata(message->c_str(), message->size());
  res = wiremessage.SerializeToString(&output);
  if (!res)
    return NULL;
  return output;
}

scoreview::instrument* allocate_instrument(t_instrument *pinstru)
{
  scoreview::instrument *pinstrument;

  pinstrument = new scoreview::instrument();
  pinstrument->set_instrumentname(pinstru->name);
  pinstrument->set_voiceindex(pinstru->voice_index);
  return pinstrument;
}

std::string Cmessage_coding::create_practice_message(t_practice_params *pp)
{
  string output;
  bool   res;
  scoreview::practice    pr;
  scoreview::instrument *pins;

  pr.set_timecode(pp->curtimecode);
  pr.set_viewtime(pp->viewtime);
  pr.set_dopractice(pp->start_practice);
  pr.set_reload(pp->reload);
  pr.set_loop(pp->loop);
  pr.set_speedfactor(pp->speedfactor);
  pr.set_countdowndaytime(pp->countdownDaytime);
  pr.set_looptime(pp->looptime);
  pr.set_hifreq(pp->hifrequency);
  pr.set_lofreq(pp->lofrequency);
  pins = allocate_instrument(&pp->instru);
  pr.set_allocated_instru(pins);
  res = pr.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_practice);
}

std::string Cmessage_coding::create_close_message()
{
  string output("");

  return pack_message(&output, network_message_close);
}

void set_note(scoreview::note* pnote, CNote *pn)
{
  std::list<t_notefreq>::iterator iter;
  scoreview::noteFrequency       *pf;

  pnote->set_time(pn->m_time);
  pnote->set_duration(pn->m_duration);
  pnote->set_uniqueid(pn->identifier());
  iter = pn->m_freq_list.begin();
  while (iter != pn->m_freq_list.end())
    {
      pf = pnote->add_freqs();
      pf->set_frequency((*iter).f);
      pf->set_stringnum((*iter).string);
      iter++;
    }
  pnote->set_connected(pn->m_lconnected);
}

scoreview::note* allocate_note(CNote *pn)
{
  scoreview::note                *pnote;

  pnote = new scoreview::note();
  set_note(pnote, pn);
  return pnote;
}

std::string Cmessage_coding::create_remadd_note_message(CNote *pn, bool bdelete)
{
  string output;
  bool   res;
  scoreview::noteRemAdd nr;

  nr.set_todelete(bdelete);
  nr.set_allocated_notedesc(allocate_note(pn));
  res = nr.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_remadd_note);
}

std::string Cmessage_coding::create_note_highlight_message(int identifier)
{
  string output;
  bool   res;
  scoreview::noteHighlight nh;

  nh.set_uniqueid(identifier);
  res = nh.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_note_highlight);
}

void set_measure(scoreview::measure *pme, CMesure *pm)
{
  pme->set_times(pm->m_times);
  pme->set_duration(fabs(pm->m_stop - pm->m_start));
  pme->set_starttimecode(pm->m_start);  
}

scoreview::measure* allocate_measure(CMesure *pm)
{
  scoreview::measure *pme;

  pme = new scoreview::measure();
  set_measure(pme, pm);
  return pme;
}

std::string Cmessage_coding::create_remadd_measure_message(CMesure *pm, bool bdelete)
{
  string output;
  bool   res;
  scoreview::measureRemAdd ra;
  scoreview::measure      *psm;

  ra.set_todelete(bdelete);
  psm = allocate_measure(pm);
  ra.set_allocated_mesdesc(psm);
  res = ra.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_remadd_measure);
}

std::string Cmessage_coding::create_instrument_list_message(std::list<t_instrument> *plist)
{
  string output;
  bool   res;
  scoreview::instrumentList         il;
  scoreview::instrument            *pelt;
  std::list<t_instrument>::iterator iter;

  iter = plist->begin();
  while (iter != plist->end())
    {
      pelt = il.add_instruelts();
      pelt->set_instrumentname((*iter).name);
      pelt->set_voiceindex((*iter).voice_index);
      iter++;
    }
  res = il.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_instrument_list);
}


std::string Cmessage_coding::create_score_message(CScore *pscore)
{
  string                        output;
  bool                          res;
  scoreview::score              score;
  scoreview::measure           *psm;
  scoreview::instrument        *pelt;
  CInstrument                  *pins;
  std::list<CMesure*>::iterator iter;

  // Measures
  iter = pscore->m_mesure_list.begin();
  while (iter != pscore->m_mesure_list.end())
    {
      psm = score.add_mesdesc();
      set_measure(psm, (*iter));
      iter++;
    }
  // Score
  pins = pscore->get_first_instrument();
  while (pins != NULL)
    {
      pelt = score.add_instruelts();
      pelt->set_instrumentname(pins->get_name());
      pelt->set_voiceindex(pins->identifier());
      pins = pscore->get_next_instrument();
    }
  res = score.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_score_transfer);  
}

std::string Cmessage_coding::create_notelist_message(CInstrument *pins, int firstnote, int lastnote)
{
  string                 output;
  bool                   res;
  t_instrument           instru;
  scoreview::scoreNotes  scorenotes;
  scoreview::note       *pnote;
  CNote                 *pn;
  int                    cnt;

  instru.name = pins->get_name();
  instru.voice_index = pins->identifier();
  scorenotes.set_allocated_instru(allocate_instrument(&instru));
  pn = pins->get_first_note(0);
  cnt = 0;
  while (pn != NULL)
    {
      if (cnt >= firstnote && cnt < lastnote)
	{
	  pnote = scorenotes.add_notelist();
	  set_note(pnote, pn);
	}
      cnt++;
      pn = pins->get_next_note();
    }
  res = scorenotes.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_note_transfer);
}

scoreview::currentAudioIO* allocate_current_audioIOcfg(t_channel_select_strings *pcs)
{
  scoreview::currentAudioIO *paio;

  paio = new scoreview::currentAudioIO();
  paio->set_apiname(pcs->api_name);
  paio->set_inputdevicename(pcs->in_device_name);
  paio->set_outputdevicename(pcs->out_device_name);
  return paio;
}

std::string Cmessage_coding::create_audioIO_config_message(std::list<t_portaudio_api> *papis, t_channel_select_strings *pcs)
{
  string output;
  bool   res;
  scoreview::audioIOConfig   ac;
  scoreview::currentAudioIO  aio;
  scoreview::audioApi       *papi;
  scoreview::audioDevice    *dev;
  std::list<t_portaudio_api>::iterator iter;
  std::list<t_pa_device>::iterator     diter;

  if (pcs != NULL)
    {
      ac.set_allocated_current(allocate_current_audioIOcfg(pcs));
    }
  if (papis)
    {
      iter = papis->begin();
      while (iter != papis->end())
	{
	  papi = ac.add_apis();
	  papi->set_name((*iter).name);
	  diter = (*iter).device_list.begin();
	  while (diter != (*iter).device_list.end())
	    {
	      dev = papi->add_devlist();
	      dev->set_name((*diter).name);
	      dev->set_out((*diter).outputs);
	      dev->set_in((*diter).inputs);
	      dev->set_deviceapicode((*diter).device_api_code);
	      diter++;
	    }
	  iter++;
	}
    }
  res = ac.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_pa_dev_selection);
}

std::string Cmessage_coding::create_config_message(t_appconfig *pcfg)
{
  string output;
  bool   res;
  scoreview::configuration cfg;

  cfg.set_recordatstart(pcfg->recordAtStart);
  cfg.set_donotchangeopenedfiles(pcfg->doNotChangeOpenedFiles);
  res = cfg.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_configuration);  
}

std::string Cmessage_coding::create_dialog_closed_message(std::string dialog_name)
{
  string output;
  bool   res;
  scoreview::dialogName name;

  name.set_dialogname(dialog_name);
  res = name.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_closed_dialog);
}

std::string Cmessage_coding::create_dialog_opened_message(std::string dialog_name)
{
  string output;
  bool   res;
  scoreview::dialogName name;

  name.set_dialogname(dialog_name);
  res = name.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_dialog_opened);
}

std::string Cmessage_coding::create_file_os_message(std::string path, std::string file_name, efileOperation fo)
{
  string output;
  bool   res;
  scoreview::openScoreFiles sf;

  sf.set_path(path);
  sf.set_filename(file_name);
  sf.set_operation(fo);
  res = sf.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_file);
}

std::string Cmessage_coding::create_remadd_instrument_message(t_instrument *pinstru, bool bdelete)
{
  string output;
  bool   res;
  scoreview::InstrumentRemAdd ira;

  ira.set_todelete(bdelete);
  ira.set_allocated_instru(allocate_instrument(pinstru));
  res = ira.SerializeToString(&output);
  if (!res)
    return NULL;
  return pack_message(&output, network_message_remadd_instrument);
}

