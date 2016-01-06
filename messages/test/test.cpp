#include <string>
#include <stdio.h>
#include <iostream>
#include <stdio.h>
#include <math.h>
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

bool diff(float a, float b)
{
  return fabs(a - b) < 0.01;
}

// Create each type of message, decode it, check it
int main(void)
{
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];
  int             size;
  Cmessage_coding coder;
  Cmessage_coding decoder;
  string          str;
  bool            res;

  // Close message
  printf("Close message: ");
  str = coder.create_close_message();
  res = false;
  if (coder.build_wire_message(msg, cmsgsize, &size, str))
    {
      decoder.get_next_wire_message(msg, size);
      res = (decoder.message_type() == network_message_close);
    }
  if (!res)
    {
      printf(" failed\n");
      exit(1);
    }
  else
    printf(" passed\n");
  // Practice
  {
    t_practice_params prac, pracout;

    printf("Practice message: ");
    prac.curtimecode = 3154.235;
    prac.viewtime = 6.45;
    prac.reload = true;
    prac.start_practice = false;
    prac.loop = true;
    prac.speedfactor = 0.75;
    prac.countdownDaytime = 46482.194;
    prac.looptime = 28.6;
    prac.hifrequency = 4650.4;
    prac.lofrequency = 35.3;
    prac.instru.name = string("violin");
    prac.instru.voice_index = 2;
    str = coder.create_practice_message(&prac);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_practice);
	{
	  if (decoder.get_practice(&pracout))
	    {
	      res = pracout.curtimecode == 3154.235;
	      res = res && pracout.viewtime == 6.45;
	      res = res && pracout.reload == true;
	      res = res && pracout.start_practice == false;
	      res = res && pracout.loop == true;
	      res = res && pracout.speedfactor == prac.speedfactor;
	      res = res && (pracout.countdownDaytime - 46482.194) < 0.001;
	      res = res && pracout.looptime == 28.6;
	      res = res && fabs(pracout.hifrequency - 4650.4) < 0.01;
	      res = res && fabs(pracout.lofrequency - 35.3) < 0.01;
	      res = res && pracout.instru.name == string("violin");
	      res = res && pracout.instru.voice_index == 2;
	    }
	}
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // Note del message
  {
    CNote      note(456.5, 2.3, 880., 1);
    CNote      noteout;
    t_notefreq f;
    bool       bdelete = true;

    printf("Note message: ");
    f.f = 440.;
    f.string = 2;
    note.m_freq_list.push_front(f);
    str = coder.create_remadd_note_message(&note, bdelete);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_remadd_note)
	  {
	    decoder.get_remadd_note(&noteout, &bdelete);
	    res = bdelete;
	    res = res && diff(noteout.m_time, note.m_time);
	    res = res && diff(noteout.m_duration, note.m_duration);
	    res = res && std::equal(noteout.m_freq_list.begin(), noteout.m_freq_list.end(), note.m_freq_list.begin());
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // Measure add message
  {
    CMesure  m(124.5, 128.1);
    CMesure  mout;
    bool     bdelete = false;

    printf("Measure message: ");
    //m.m_bpm = 2;
    m.m_times = 4;
    str = coder.create_remadd_measure_message(&m, bdelete);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_remadd_measure)
	  {
	    decoder.get_remadd_measure(&mout, &bdelete);
	    res = !bdelete;
	    res = res && diff(m.m_start, mout.m_start);
	    res = res && diff(m.m_stop, mout.m_stop);
	    res = res && m.m_times == mout.m_times;
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // Instrument list
  {
    t_instrument            ins;
    std::list<t_instrument> ilist;
    std::list<t_instrument> ilistout;
    std::list<t_instrument>::iterator it1, it2;

    printf("Instrument list message: ");
    ins.name = string("clavitron4000");
    ins.voice_index = 1;
    ilist.push_back(ins);
    ins.name = string("gaz");
    ins.voice_index = 2;
    ilist.push_back(ins);
    str = coder.create_instrument_list_message(&ilist);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_instrument_list)
	  {
	    res = decoder.get_instrument_list(&ilistout);
	    it1 = ilist.begin();
	    it2 = ilistout.begin();
	    res = res && ilist.size() == ilistout.size();
	    while (it1 != ilist.end() && it2 != ilistout.end())
	    {
	      res = res && (*it1).name == (*it2).name;
	      res = res && (*it1).voice_index == (*it2).voice_index;
	      it1++;
	      it2++;
	    }
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // IO configuration
  {
    t_portaudio_api            api;
    std::list<t_portaudio_api> apis;
    t_channel_select_strings   cs;
    t_pa_device                dev;

    printf("Audio IO configuration message: ");
    api.name = string("fast sound interface");
    dev.name = string("speakers");
    dev.device_api_code = 1;
    dev.inputs = 0;
    dev.outputs = 2;
    api.device_list.push_back(dev);
    dev.name = string("stereo microphone");
    dev.device_api_code = 2;
    dev.inputs = 1;
    dev.outputs = 0;
    api.device_list.push_back(dev);
    apis.push_back(api);
    //
    api.device_list.clear();
    api.name = string("jack");
    dev.name = string("stereo mix");
    dev.device_api_code = 0;
    dev.inputs = 4;
    dev.outputs = 2;
    api.device_list.push_back(dev);
    apis.push_back(api);
    //
    cs.api_name = string("fast sound interface");
    cs.in_device_name = string("stereo microphone");
    cs.out_device_name = string("speakers");
    str = coder.create_audioIO_config_message(&apis, &cs);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_pa_dev_selection)
	  {
	    std::list<t_portaudio_api> outapis;
	    t_channel_select_strings   outcs;

	    res = decoder.get_audioIO_config(&outapis, &outcs);
	    res = res && outcs.api_name == cs.api_name && outcs.in_device_name == cs.in_device_name && outcs.out_device_name == cs.out_device_name;
	    if (res)
	      {
		std::list<t_portaudio_api>::iterator  iter1, iter2;
		std::list<t_pa_device>::iterator      diter1, diter2;

		iter1 = apis.begin();
		iter2 = outapis.begin();
		res = res && (apis.size() == outapis.size());
		while (iter1 != apis.end() && iter2 != outapis.end())
		  {
		    res = res && ((*iter1).name == (*iter2).name);
		    diter1 = (*iter1).device_list.begin();
		    diter2 = (*iter2).device_list.begin();
		    res = res && (*iter1).device_list.size() == (*iter1).device_list.size();
		    while (diter1 != (*iter1).device_list.end() && diter2 != (*iter2).device_list.end())
		      {
			res = res && (*diter1).name == (*diter2).name;
			res = res && (*diter1).device_api_code == (*diter2).device_api_code;
			res = res && (*diter1).inputs == (*diter2).inputs;
			res = res && (*diter1).outputs == (*diter2).outputs;
			diter1++;
			diter2++;
		      }
		    iter1++;
		    iter2++;
		  }
	      }
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }  
  // General configuration
  {
    t_appconfig cfg;

    printf("App config message: ");
    cfg.recordAtStart = true;
    cfg.doNotChangeOpenedFiles = false;
    str = coder.create_config_message(&cfg);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_configuration)
	  {
	    t_appconfig outcfg;

	    res = decoder.get_config(&outcfg);
	    res = res && outcfg.recordAtStart == cfg.recordAtStart;
	    res = res && outcfg.doNotChangeOpenedFiles == cfg.doNotChangeOpenedFiles;
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // File IO messages (save open)
  {
    std::string path("/usr/fond/plus/machine/bublemem/");
    std::string file_name("Canonball Aderley - Autumn Leaves.xml");
    
    printf("File io message: ");
    str = coder.create_file_os_message(path, file_name, efilesave);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_file)
	  {
	    string         outpath, outfile;
	    efileOperation fo;
	    
	    res = decoder.get_fileIO_message(&outpath, &outfile, &fo);
	    res = res && fo == efilesave;
	    res = res && outpath == path && outfile == file_name;
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // +/- instrument
  {
    t_instrument instru;
    bool         bdelete;

    printf("Add Remove instrument message: ");
    instru.name = string("clavitron4000");
    instru.voice_index = 12;
    bdelete = true;
    str = coder.create_remadd_instrument_message(&instru, bdelete);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_remadd_instrument)
	  {
	    t_instrument outinstru;
	    
	    res = decoder.get_remadd_instrument(&outinstru, &bdelete);
	    res = res && bdelete;
	    res = res && outinstru.name == instru.name && outinstru.voice_index == instru.voice_index;
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // Dialog closed
  {
    string dialogname(STORAGE_DIALOG_CODE_NAME);

    printf("Closed dialog message: ");

    str = coder.create_dialog_closed_message(dialogname);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_closed_dialog)
	  {
	    string outdialogname;
	    
	    res = decoder.get_dialog_name(&outdialogname);
	    res = res && dialogname == outdialogname;
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // Dialog opened
  {
    string dialogname(STORAGE_DIALOG_CODE_NAME);

    printf("Opened dialog message: ");

    str = coder.create_dialog_opened_message(dialogname);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_dialog_opened)
	  {
	    string outdialogname;
	    
	    res = decoder.get_dialog_name(&outdialogname);
	    res = res && dialogname == outdialogname;
	  }
      }
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
  // Score message
  {
    CScore      *pscore;
    CScore      *pscoreout;
    CInstrument *pins;
    CInstrument *pinsout;
    CMesure     *pm;
    CMesure     *pmout;
    std::list<CMesure*>::iterator iter, iterout;

    printf("Score transfer message: ");
    pscore = new CScore("violin");
    pscoreout = new CScore();
    pins = new CInstrument("pianoooo");
    pscore->add_instrument(pins);
    pm = new CMesure(24.1, 26.4);
    pm->m_times = 4;
    pscore->m_mesure_list.push_front(pm);
    pm = new CMesure(111.2, 120.0);
    pm->m_times = 16;
    pscore->m_mesure_list.push_front(pm);
    str = coder.create_score_message(pscore);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_score_transfer)
	  {
	    res = decoder.get_score_message(pscoreout);
	    pins = pscore->get_first_instrument();
	    pinsout = pscoreout->get_first_instrument();
	    if (pins != NULL && pinsout != NULL)
	      {
		res = res && (pins->get_name() == pinsout->get_name() && pinsout->get_name() == string("violin"));
	      }
	    pins = pscore->get_next_instrument();
	    pinsout = pscoreout->get_next_instrument();
	    if (pins != NULL && pinsout != NULL)
	      {
		res = res && (pins->get_name() == pinsout->get_name() && pinsout->get_name() == string("pianoooo"));
	      }
	    else
	      res = false;
	    pinsout = pscoreout->get_next_instrument();
	    res = res && pinsout == NULL;
	    //
	    res = res &&  pscore->m_mesure_list.size() == pscoreout->m_mesure_list.size();
	    iter = pscore->m_mesure_list.begin();
	    iterout = pscoreout->m_mesure_list.begin();
	    while (iter != pscore->m_mesure_list.end() &&
		   iterout != pscoreout->m_mesure_list.begin())
	      {
		pm = (*iter);
		pmout = (*iterout);
		res = res && pm->m_times == pmout->m_times;
		res = res && pm->m_start == pmout->m_start;
		res = res && pm->duration() == pmout->duration();
		iter++;
		iterout++;
	      }
	  }
      }
    delete pscore;
    delete pscoreout;
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
    
  }
  // Notes
  {
    CScore      *pscore;
    CScore      *pscoreout;
    CInstrument *pins;
    CInstrument *pinsout;
    int          i;
    const int    cnlsize = NOTES_PER_NOTE_TRANSFER;
    CNote       *pnoteout;
    CNote       *pnote;
    int          index;

    printf("Note transfer message: ");
    pscore = new CScore("guitar");
    pscoreout = new CScore("guitar");
    index = 0;
    pins = pscore->get_instrument(string("guitar"), index);
    pinsout = pscoreout->get_instrument(string("guitar"), index);
    if (pins == NULL || pinsout == NULL)
      {
	printf("get instrument error.\n");
	exit(1);
      }
    for (i = 0; i < cnlsize; i++)
      {
	pnote = new CNote(4000.5 + i, 4.1 + i, 780. + i, i);
	pins->add_note(pnote);
      }
    str = coder.create_notelist_message(pins, 0, cnlsize);
    res = false;
    if (coder.build_wire_message(msg, cmsgsize, &size, str))
      {
	//printf("note message size=%d.\n", size);
	assert(size < 2048);
	decoder.get_next_wire_message(msg, size);
	if (decoder.message_type() == network_message_note_transfer)
	  {
	    res = decoder.get_notelist_message(pscoreout);
	    i = 0;
	    pnote = pins->get_first_note(0);
	    pnoteout = pinsout->get_first_note(0);
	    while (pnote != NULL && pnoteout != NULL)
	      {
		res = res && diff(pnoteout->m_time, pnote->m_time);
		res = res && diff(pnoteout->m_duration, pnote->m_duration);
		res = res && std::equal(pnoteout->m_freq_list.begin(), pnoteout->m_freq_list.end(), pnote->m_freq_list.begin());
		res =res && pnoteout->identifier() == pnote->identifier();
		pnote = pins->get_next_note();
		pnoteout = pinsout->get_next_note();
		i++;
	      }
	  }
      }
    delete pscore;
    delete pscoreout;
    if (!res)
      {
	printf(" failed\n");
	exit(1);
      }
    else
      printf(" passed\n");
  }
}

